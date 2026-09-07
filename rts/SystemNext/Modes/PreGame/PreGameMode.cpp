/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PreGameMode.h"

#include <cinttypes>
#include <cfloat>
#include <functional>

#include <SDL_keycode.h>

#include "Game/PreGame.h"
#include "Rendering/GL/myGL.h"
#include "SystemNext/Modes/FlowResult.h"

#include "Game/ClientData.h"
#include "Game/ClientSetup.h"
#include "System/Sync/FPUCheck.h"
#include "Game/Game.h"
#include "Game/GameData.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/InfoConsole.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Map/Generation/BlankMapGenerator.h"
#include "Menu/LuaMenuController.h"
#include "Net/GameServer.h"
#include "Net/Protocol/NetProtocol.h"

#include "aGui/Gui.h"

#include "Rendering/Fonts/glFont.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/SpringExitCode.h"
#include "System/TimeProfiler.h"
#include "System/TdfParser.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/LoadSave/DemoReader.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/Log/ILog.h"
#include "System/Net/RawPacket.h"
#include "System/Net/UnpackPacket.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Misc.h"
#include "System/Sync/SyncedPrimitiveBase.h"
#include "System/Misc/UnfreezeSpring.h"
#include "lib/luasocket/src/restrictions.h"
#ifdef SYNCDEBUG
	#include "System/Sync/SyncDebugger.h"
#endif

#include "System/Misc/TracyDefs.h"


namespace runtime {

CPreGame& PreGameMode::Backing() const
{
	auto* controller = dynamic_cast<CPreGame*>(Controller());
	if (controller == nullptr)
		throw IncompleteFlow{"PRE-BIND", "Pregame operation has no matching backing controller"};
	return *controller;
}

bool PreGameMode::HandlesSession() const
{
	return true;
}

DisplayPhase PreGameMode::GetDisplayPhase() const
{
	// No separate client update exists. Render retains live connection display
	// decisions until an owned display payload can preserve their read timing.
	return DisplayPhase::None;
}

/**
 * Keep setup processing within its established sync/FPU domain.
 * Terminal connection outcomes affect lifecycle state, while the original
 * successful update result remains true. Existing engine errors propagate.
 */
SessionUpdate PreGameMode::UpdateSession(Session&)
{
	ZoneScoped;
	ENTER_SYNCED_CODE();
	good_fpu_control_registers("CPreGame::Update");
	try {
		ProcessConnectionPackets();
	} catch (const IncompleteFlow& error) {
		// New incomplete paths are explicit outcomes, not engine exceptions.
		// Leave the entered scope before returning that outcome to the runtime.
		LEAVE_SYNCED_CODE();
		const auto failure = error.Failure();
		return SessionUpdate::Incomplete(failure.id, failure.reason);
	}
	//FIXME PRE-003: Existing engine exceptions skip this explicit exit. Preserve
	// that behavior pending sync-debug exception validation; no broad catch here.
	LEAVE_SYNCED_CODE();
	return SessionUpdate::FromContinuation(true);
}

/**
 * Wait for setup completion before transport and between authoritative packets.
 * A game-data packet starts setup before player assignment may consume its data.
 * Terminal outcomes stop immediately; no subsequent backing access is permitted.
 */
void PreGameMode::ProcessConnectionPackets()
{
	auto& backing = Backing();

	RECOIL_DETAILED_TRACY_ZONE;

	if (HasPendingSetupTask())
		return;

	clientNet->Update();

	if (clientNet->CheckTimeout(0, true)) {
		if (CLuaMenuController::ActivateInstance("[PreGame] Server Connection Timeout")) {
			assert(pregame == &backing);
			//FIXME PRE-002: The retained statement destroys backing inside this call.
			// Stop until mode binding and task retirement are coordinated.
#if 0
			spring::SafeDelete(pregame);
#endif
			throw IncompleteFlow{"PRE-002", "Pregame retirement is not bound to mode lifetime"};
			return;
		}

		LOG_L(L_ERROR, "[PreGame::%s] server connection timeout", "UpdateClientNet");

		spring::exitCode = spring::EXIT_CODE_TIMEOUT;
		gu->globalQuit = true;
		return;
	}

	std::shared_ptr<const netcode::RawPacket> packet;

	while (!HasPendingSetupTask() && (packet = clientNet->GetData(gs->frameNum))) {
		const unsigned char* inbuf = packet->data;

		if (packet->length <= 0) {
			//FIXME PRE-004: There is no header byte to format for an empty packet.
#if 0
			LOG_L(L_WARNING, "[PreGame::%s] zero-length packet (header: %i)", "UpdateClientNet", inbuf[0]);
#endif
			LOG_L(L_WARNING, "[PreGame::%s] zero-length packet", "UpdateClientNet");
			continue;
		}

		switch (inbuf[0]) {
			case NETMSG_REJECT_CONNECT:
			case NETMSG_QUIT: {
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string message;

					pckt >> message;

					// (re)activate LuaMenu if user failed to connect
					if (CLuaMenuController::ActivateInstance(message)) {
						assert(pregame == &backing);
						//FIXME PRE-002: The retained statement destroys backing inside this call.
						// Stop until mode binding and task retirement are coordinated.
#if 0
						spring::SafeDelete(pregame);
#endif
						throw IncompleteFlow{"PRE-002", "Pregame retirement is not bound to mode lifetime"};
						return;
					}

					// force exit to system if no menu
					LOG("[PreGame::%s] server requested quit or rejected connection (reason \"%s\")", "UpdateClientNet", message.c_str());
					handleerror(nullptr, "server requested quit or rejected connection", "Quit message", MBF_OK | MBF_EXCL);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[PreGame::%s][NETMSG_{QUIT,REJECT_CONNECT}] exception \"%s\"", "UpdateClientNet", ex.what());
				}
			} break;

			case NETMSG_CREATE_NEWPLAYER: {
				// server will send this first if we're using mid-game join
				// feature to let us know about ourselves (we won't be in
				// gamedata), otherwise skip to gamedata
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string name;

					uint8_t playerNum;
					uint8_t spectator;
					uint8_t team;

					// since the >> operator uses dest size to extract data from
					// the packet, we need to use temp variables of the same
					// size of the packet before converting to dest variable
					pckt >> playerNum;
					pckt >> spectator;
					pckt >> team;
					pckt >> name;

					CPlayer player;
					player.name = name;
					player.spectator = spectator;
					player.team = team;
					player.playerNum = playerNum;

					// add ourselves to avoid crashing if our player-num gets queried
					// we will receive this message a second time (the global broadcast
					// version) which will overwrite the player with the same values as
					// set here
					playerHandler.AddPlayer(player);

					LOG("[PreGame::%s] added new player \"%s\" with number %d to team %d (#active=%d)", "UpdateClientNet", name.c_str(), player.playerNum, player.team, playerHandler.ActivePlayers());
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[PreGame::%s][NETMSG_CREATE_NEWPLAYER] exception \"%s\"", "UpdateClientNet", ex.what());
				}
			} break;

			case NETMSG_GAMEDATA: {
				// server first sends this to let us know about teams, allyteams
				// etc. (not if we are joining mid-game as an extra player), see
				// NETMSG_SETPLAYERNUM
				//FIXME PRE-001: This raw-this task can outlive the backing object.
				// Retain the launch until external task retirement is implemented.
#if 0
				backing.pendingTask = std::async(std::launch::async,
					&PreGameMode::GameDataReceived, this,
					packet
				);
#endif
				throw IncompleteFlow{"PRE-001", "Received setup task lifetime is not bound"};
			} break;

			case NETMSG_SETPLAYERNUM: {
				// this is sent after NETMSG_GAMEDATA, to let us know which
				// player number we have (server assigns them based on order
				// of connection)
				if (!CGameSetup::ScriptLoaded())
					throw content_error("No game data received from server");

				if (packet->length < 2)
					throw content_error("Missing player number in SETPLAYERNUM packet");

				const uint8_t playerNum = packet->data[1];

				if (!playerHandler.IsValidPlayer(playerNum))
					throw content_error("Invalid player number received from server");

				// respond with the client data and content checksums
				gu->SetMyPlayer(playerNum);
				clientNet->Send(CBaseNetProtocol::Get().SendClientData(playerNum, ClientData::GetCompressed()));

				LOG("[PreGame::%s] received local player number %i (team %i, allyteam %i), creating LoadScreen", "UpdateClientNet", gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);
				CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, mapChecksumMsgBuf);
				CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, modChecksumMsgBuf);

				//FIXME PRE-006: Loading can reenter before this raw-handler handoff and
				// backing retirement complete. Stop before either ownership changes.
#if 0
				CLoadScreen::CreateDeleteInstance(gameSetup->MapFileName(), std::move(backing.modFileName), backing.saveFileHandler);

				assert(pregame == &backing);
				spring::SafeDelete(pregame);
#endif
				throw IncompleteFlow{"PRE-006", "Loading handoff and pregame retirement are not bound"};
				return;
			} break;

			default: {
				LOG_L(L_WARNING, "[PreGame::%s] unknown packet type (header: %i)", "UpdateClientNet", inbuf[0]);
			} break;
		}
	}
}

/** Acquire ready task results before transport; never spin or wait here. */
bool PreGameMode::HasPendingSetupTask()
{
	auto& backing = Backing();

	if (!backing.pendingTask.valid())
		return false;

	using namespace std::chrono_literals;
	if (backing.pendingTask.wait_for(0ms) != std::future_status::ready)
		return true;

	backing.pendingTask.get();
	backing.pendingTask = {};
	return false;
}

/** Input cancellation does not advance session state or render output. */
int PreGameMode::KeyPressed(int keyCode, int scanCode, bool isRepeat)
{
	auto& backing = Backing();

	RECOIL_DETAILED_TRACY_ZONE;
	if (keyCode != SDLK_ESCAPE)
		return 0;

	if (!KeyInput::GetKeyModState(KMOD_SHIFT)) {
		LOG("[PreGame::%s] press shift+escape to abort loading or exit", "KeyPressed");
		return 0;
	}

	if (CLuaMenuController::ActivateInstance("[PreGame] User Aborted Loading")) {
		assert(pregame == &backing);
		//FIXME PRE-002: The retained statement destroys backing inside this call.
		// Stop until mode binding and task retirement are coordinated.
#if 0
		spring::SafeDelete(pregame);
#endif
		throw IncompleteFlow{"PRE-002", "Pregame retirement is not bound to mode lifetime"};
		return 0;
	}

	LOG("[PreGame::%s] user exited", "KeyPressed");
	gu->globalQuit = true;
	return 0;
}

/**
 * Render the connection screen within the caller's loading/graphics scope.
 * Text decisions remain interleaved with emission to preserve observation order.
 * Headless skips graphics but retains the original permission to present.
 */
RenderResult PreGameMode::Render(ModeFrame& frame)
{
	if (!frame.allowRender)
		return RenderResult::Skipped();
	auto& backing = Backing();

#ifndef HEADLESS
	RECOIL_DETAILED_TRACY_ZONE;

	ClearScreen();

	static constexpr const float4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

	font->Begin();
	font->SetTextColor(color.x, color.y, color.z, color.w);

	//FIXME PRE-005: Connection/setup reads remain inside render. Before threaded
	// rendering, establish synchronized ownership and an equivalent capture point.
	if (!clientNet->Connected()) {
		if (backing.clientSetup->isHost) {
			if (backing.clientSetup->hostIP == "localhost")
				font->glFormat(0.5f, 0.60f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Waiting for game to start");
			else
				font->glFormat(0.5f, 0.60f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Waiting for server to start");
		}
		else {
			font->glFormat(0.5f, 0.60f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Connecting to server (%ds)", (spring_gettime() - backing.connectTimer).toSecsi());
		}
	} else {
		font->glPrint(0.5f, 0.60f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Waiting for server response");
	}
	if (backing.clientSetup->showServerName.empty()) {
		font->glFormat(0.60f, 0.50f, 1.0f, FONT_SCALE | FONT_NORM, "Connecting to: %s", clientNet->ConnectionStr().c_str());
		font->glFormat(0.60f, 0.45f, 1.0f, FONT_SCALE | FONT_NORM, "User name: %s", backing.clientSetup->myPlayerName.c_str());
	}
	else {
		font->glFormat(0.60f, 0.50f, 1.0f, FONT_SCALE | FONT_NORM, "Connecting to: %s", backing.clientSetup->showServerName.c_str());
	}

	if (archiveScanner->GetNumFilesHashed() > 0) {
		font->glFormat(0.60f, 0.35f, 1.0f, FONT_SCALE | FONT_NORM, "[Performing necessary checksum calculations]");
		font->glFormat(0.60f, 0.30f, 1.0f, FONT_SCALE | FONT_NORM, "Number of files checked: %u", archiveScanner->GetNumFilesHashed());
	}

	font->glFormat(0.5f, 0.15f, 0.8f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Press SHIFT + ESC to quit");

	// credits
	font->glFormat(0.5f, 0.06f, 1.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Recoil %s", SpringVersion::GetFull().c_str());
	font->glPrint(0.5f, 0.02f, 0.6f, FONT_CENTER | FONT_SCALE | FONT_NORM, "This program is distributed under the GNU General Public License, see doc/LICENSE for more info");

	font->End();
#endif
	return RenderResult::Ready();
}

}
