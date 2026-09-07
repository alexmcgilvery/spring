/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyPreGameServices.h"

//FIXME PRE-BIND: The exact proposed service bodies below are blocked, not empty
// implementations. Private fields still belong to CPreGame; original controller
// code remains the active owner. Resolve PRE-001..007 before extraction. This
// disabled draft has not been typechecked and must not be mistaken for a binding.
#if 0
#include <cinttypes>
#include <cfloat>
#include <functional>

#include <SDL_keycode.h>

#include "Game/PreGame.h"

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

//FIXME PRE-BUFFERS: mapChecksumMsgBuf/modChecksumMsgBuf are file-static in
// Game/PreGame.cpp. Keep their single existing ownership when extracting the
// producer/consumer below; do not add duplicate buffers to satisfy this draft.

//FIXME PRE-SCOPES: Moving operations changes __func__ in the retained LOG calls
// and loses CPreGame Update/Draw's original profiler envelope. Preserve labels
// and measured intervals explicitly at owned flow boundaries before activation.
LegacyPreGameServices::LegacyPreGameServices(CPreGame& controller)
	: backing(controller)
{
}

/** Keep sync entry/FPU checking and normal exit at the session scope boundary. */
void LegacyPreGameServices::EnterSyncedCode()
{
	ENTER_SYNCED_CODE();
}

void LegacyPreGameServices::CheckFloatingPointControl()
{
	good_fpu_control_registers("CPreGame::Update");
}

void LegacyPreGameServices::LeaveSyncedCode()
{
	//FIXME PRE-003: Exceptions skip this explicit exit in the current code.
	// Do not silently add RAII restoration while extracting the operation.
	LEAVE_SYNCED_CODE();
}

void LegacyPreGameServices::UpdateConnectionTransport()
{
	clientNet->Update();
}

/** Resolve timeout before consuming packets; a menu transition can retire backing. */
bool LegacyPreGameServices::HandleConnectionTimeout()
{
	if (clientNet->CheckTimeout(0, true)) {
		if (CLuaMenuController::ActivateInstance("[PreGame] Server Connection Timeout")) {
			assert(pregame == &backing);
			//FIXME PRE-002: This statement retires backing; no subsequent access is safe.
			// Worker-triggered save failure also requires a non-self-joining transition.
			spring::SafeDelete(pregame);
			return true;
		}

		LOG_L(L_ERROR, "[PreGame::%s] server connection timeout", __func__);

		spring::exitCode = spring::EXIT_CODE_TIMEOUT;
		gu->globalQuit = true;
		return true;
	}

	return false;
}

/** Consume one authoritative connection packet; the caller polls setup between packets. */
PreGamePacketResult LegacyPreGameServices::ProcessNextConnectionPacket()
{
	const auto packet = clientNet->GetData(gs->frameNum);
	if (!packet)
		return PreGamePacketResult::NoPacket;

	const unsigned char* inbuf = packet->data;

	//FIXME PRE-004: Warning reads byte zero on this branch; validate ingress
	// bounds before retaining or correcting the operation.
	if (packet->length <= 0) {
		LOG_L(L_WARNING, "[PreGame::%s] zero-length packet (header: %i)", __func__, inbuf[0]);
		return PreGamePacketResult::Continue;
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
					//FIXME PRE-002: This statement retires backing; no subsequent access is safe.
					// Worker-triggered save failure also requires a non-self-joining transition.
					spring::SafeDelete(pregame);
					return PreGamePacketResult::Stop;
				}

				// force exit to system if no menu
				LOG("[PreGame::%s] server requested quit or rejected connection (reason \"%s\")", __func__, message.c_str());
				handleerror(nullptr, "server requested quit or rejected connection", "Quit message", MBF_OK | MBF_EXCL);
			} catch (const netcode::UnpackPacketException& ex) {
				LOG_L(L_ERROR, "[PreGame::%s][NETMSG_{QUIT,REJECT_CONNECT}] exception \"%s\"", __func__, ex.what());
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

				LOG("[PreGame::%s] added new player \"%s\" with number %d to team %d (#active=%d)", __func__, name.c_str(), player.playerNum, player.team, playerHandler.ActivePlayers());
			} catch (const netcode::UnpackPacketException& ex) {
				LOG_L(L_ERROR, "[PreGame::%s][NETMSG_CREATE_NEWPLAYER] exception \"%s\"", __func__, ex.what());
			}
		} break;

		case NETMSG_GAMEDATA: {
			// server first sends this to let us know about teams, allyteams
			// etc. (not if we are joining mid-game as an extra player), see
			// NETMSG_SETPLAYERNUM
			//FIXME PRE-001: Worker captures service this and reaches backing by reference.
			// Keep this exact launch blocked until cancellation/join outlive both objects.
			backing.pendingTask = std::async(std::launch::async,
				&LegacyPreGameServices::GameDataReceived, this,
				packet
			);
		} break;

		case NETMSG_SETPLAYERNUM: {
			// this is sent after NETMSG_GAMEDATA, to let us know which
			// player number we have (server assigns them based on order
			// of connection)
			if (!CGameSetup::ScriptLoaded())
				throw content_error("No game data received from server");

			//FIXME PRE-004: No local length guard exists for this byte read.
			const uint8_t playerNum = packet->data[1];

			if (!playerHandler.IsValidPlayer(playerNum))
				throw content_error("Invalid player number received from server");

			// respond with the client data and content checksums
			gu->SetMyPlayer(playerNum);
			clientNet->Send(CBaseNetProtocol::Get().SendClientData(playerNum, ClientData::GetCompressed()));

			LOG("[PreGame::%s] received local player number %i (team %i, allyteam %i), creating LoadScreen", __func__, gu->myPlayerNum, gu->myTeam, gu->myAllyTeam);
			CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, mapChecksumMsgBuf);
			CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, modChecksumMsgBuf);

			//FIXME PRE-006: Raw save handler transfers through loading into game.
			// Establish exception/cancel ownership before enabling this boundary.
			CLoadScreen::CreateDeleteInstance(gameSetup->MapFileName(), std::move(backing.modFileName), backing.saveFileHandler);

			assert(pregame == &backing);
			//FIXME PRE-002: This statement retires backing; no subsequent access is safe.
			// Worker-triggered save failure also requires a non-self-joining transition.
			spring::SafeDelete(pregame);
			return PreGamePacketResult::Stop;
		} break;

		default: {
			LOG_L(L_WARNING, "[PreGame::%s] unknown packet type (header: %i)", __func__, inbuf[0]);
		} break;
	}
	return PreGamePacketResult::Continue;
}

/** Publish worker writes and exceptions before transport or another packet is observed. */
bool LegacyPreGameServices::HasPendingSetupTask()
{
	if (!backing.pendingTask.valid())
		return false;

	using namespace std::chrono_literals;
	if (backing.pendingTask.wait_for(0ms) != std::future_status::ready)
		return true;

	backing.pendingTask.get();
	backing.pendingTask = {};
	return false;
}

/** INPUT: cancellation preserves menu activation versus process-exit behavior. */
int LegacyPreGameServices::HandleConnectionCancel(int keyCode, int scanCode, bool isRepeat)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (keyCode != SDLK_ESCAPE)
		return 0;

	if (!KeyInput::GetKeyModState(KMOD_SHIFT)) {
		LOG("[PreGame::%s] press shift+escape to abort loading or exit", __func__);
		return 0;
	}

	if (CLuaMenuController::ActivateInstance("[PreGame] User Aborted Loading")) {
		assert(pregame == &backing);
		//FIXME PRE-002: This statement retires backing; no subsequent access is safe.
		// Worker-triggered save failure also requires a non-self-joining transition.
		spring::SafeDelete(pregame);
		return 0;
	}

	LOG("[PreGame::%s] user exited", __func__);
	gu->globalQuit = true;
	return 0;
}

/** DISPLAY/RENDER: headless skips this complete body while preserving true result. */
bool LegacyPreGameServices::HasConnectionScreen() const
{
#ifndef HEADLESS
	return true;
#else
	return false;
#endif
}

void LegacyPreGameServices::ClearConnectionScreen()
{
	//FIXME PRE-CLEAR: ClearScreen is a CGameController operation. Extract or
	// grant narrow access without retaining whole Draw dispatch or moving its
	// GL state side effects across text preparation.
	backing.ClearScreen();
}

/** RENDER: keep one font batch and white text state. */
void LegacyPreGameServices::BeginConnectionText()
{
	static constexpr const float4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

	font->Begin();
	font->SetTextColor(color.x, color.y, color.z, color.w);

}

//FIXME PRE-005: These exact live connection/setup reads remain interleaved with
// font emission. Moving them into display preparation changes observation time;
// retaining them on a render worker requires synchronized lifetime/access. Keep
// this block disabled until a copied display payload and capture point are agreed.
/** DISPLAY/RENDER: retain host, remote join and connected response branches. */
void LegacyPreGameServices::DrawConnectionStatus()
{
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
}

/** DISPLAY/RENDER: preserve server-name versus endpoint/username policy. */
void LegacyPreGameServices::DrawConnectionIdentity()
{
	if (backing.clientSetup->showServerName.empty()) {
		font->glFormat(0.60f, 0.50f, 1.0f, FONT_SCALE | FONT_NORM, "Connecting to: %s", clientNet->ConnectionStr().c_str());
		font->glFormat(0.60f, 0.45f, 1.0f, FONT_SCALE | FONT_NORM, "User name: %s", backing.clientSetup->myPlayerName.c_str());
	}
	else {
		font->glFormat(0.60f, 0.50f, 1.0f, FONT_SCALE | FONT_NORM, "Connecting to: %s", backing.clientSetup->showServerName.c_str());
	}

}

/** DISPLAY/RENDER: progress reads the existing atomic counter at its current point. */
void LegacyPreGameServices::DrawArchiveChecksumProgress()
{
	if (archiveScanner->GetNumFilesHashed() > 0) {
		font->glFormat(0.60f, 0.35f, 1.0f, FONT_SCALE | FONT_NORM, "[Performing necessary checksum calculations]");
		font->glFormat(0.60f, 0.30f, 1.0f, FONT_SCALE | FONT_NORM, "Number of files checked: %u", archiveScanner->GetNumFilesHashed());
	}

}

/** RENDER: preserve input instructions and credit placement within the batch. */
void LegacyPreGameServices::DrawAbortInstructionsAndCredits()
{
	font->glFormat(0.5f, 0.15f, 0.8f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Press SHIFT + ESC to quit");

	// credits
	font->glFormat(0.5f, 0.06f, 1.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "Recoil %s", SpringVersion::GetFull().c_str());
	font->glPrint(0.5f, 0.02f, 0.6f, FONT_CENTER | FONT_SCALE | FONT_NORM, "This program is distributed under the GNU General Public License, see doc/LICENSE for more info");

}

/** RENDER: finish the batch before the runtime presents output. */
void LegacyPreGameServices::EndConnectionText()
{
	font->End();
}

/** SESSION ENTRY: preserve remote/local transport initialization after backing fields initialize. */
//FIXME PRE-ENTRY: Constructor field initialization remains in CPreGame:
// clientSetup(setup), saveFileHandler(nullptr), connectTimer(spring_gettime()),
// wantDemo(true). Extract this body from construction only after binding order
// guarantees those initialized fields and prevents duplicate transport creation.
void LegacyPreGameServices::InitializeConnection()
{
	assert(clientNet == nullptr);

	clientNet = new CNetProtocol();
	activeController = &backing;

#ifdef SYNCDEBUG
	CSyncDebugger::GetInstance()->Initialize(backing.clientSetup->isHost, 64); //FIXME: add actual number of player
#endif

	if (!backing.clientSetup->isHost) {
		LOG("[%s] using client IP %s and port %i", __func__, backing.clientSetup->hostIP.c_str(), backing.clientSetup->hostPort);
		// don't allow luasocket to connect to the host
		luaSocketRestrictions->addRule(CLuaSocketRestrictions::UDP_CONNECT, backing.clientSetup->hostIP, backing.clientSetup->hostPort, false);
		clientNet->InitClient(backing.clientSetup, SpringVersion::GetSync(), Platform::GetPlatformStr());
	} else {
		LOG("[%s] using server IP %s and port %i", __func__, backing.clientSetup->hostIP.c_str(), backing.clientSetup->hostPort);
		clientNet->InitLocalClient();
	}
}

/** SESSION EXIT: retain GUI cleanup and global pregame clearing; task retirement is unresolved. */
void LegacyPreGameServices::ReleaseConnectionScreen()
{
	#ifndef HEADLESS
	// delete leftover aGUI elements but not infoconsole, it is reused by CGame
	agui::gui->Clean();
	#endif

	pregame = nullptr;
}

/** SETUP WORKER: preserve copied arguments, explicit async launch and FPU initialization. */
void LegacyPreGameServices::AsyncExecute(AsyncExecFuncType execFunc, const std::string& argument)
{
	//FIXME PRE-001: Worker captures service this and reaches backing by reference.
	// Keep this exact launch blocked until cancellation/join outlive both objects.
	backing.pendingTask = std::async(std::launch::async,
		[execFunc, argument/*copy the argument explicitly*/, this]() {
			const auto InitStuffAndExecute = [execFunc, argument/*copy the argument explicitly*/, this]() {
				Threading::SetThreadName("pregame");
				streflop::streflop_init<streflop::Simple>();
				std::invoke(execFunc, this, argument);
			};
			std::invoke(InitStuffAndExecute);
		}
	);
}

/** HOST SETUP: use the selected script without adding a separate host mode. */
void LegacyPreGameServices::LoadSetupScript(const std::string& script)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(backing.clientSetup->isHost);
	StartServer(script);
}

/** DEMO SETUP: preserve recording policy before scanning original game data. */
void LegacyPreGameServices::LoadDemoFile(const std::string& demo)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(backing.clientSetup->isHost);
	backing.wantDemo &= configHandler->GetBool("DemoFromDemo");

	ReadDataFromDemo(demo);
}

/** SAVE SETUP: retain handler validation and the menu/exit failure branches. */
void LegacyPreGameServices::LoadSaveFile(const std::string& save)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(backing.clientSetup->isHost);

	backing.saveFileHandler = ILoadSaveHandler::CreateHandler(save);

	if (backing.saveFileHandler->LoadGameStartInfo(save) || configHandler->GetBool("LoadBadSaves")) {
		StartServer(backing.saveFileHandler->GetScriptText());
		return;
	}

	LOG_L(L_ERROR, "[PreGame::%s] incompatible save-file specified", __func__);

	spring::SafeDelete(backing.saveFileHandler);

	if (CLuaMenuController::ActivateInstance("[PreGame] incompatible save-file")) {
		assert(pregame == &backing);
		//FIXME PRE-002: This statement retires backing; no subsequent access is safe.
		// Worker-triggered save failure also requires a non-self-joining transition.
		spring::SafeDelete(pregame);
		return;
	}

	spring::exitCode = spring::EXIT_CODE_BADSAVE;
	gu->globalQuit = true;
}

/** CONTENT: mount the map before consumers read map data. */
void LegacyPreGameServices::AddMapArchivesToVFS(const CGameSetup* setup)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// map gets added in StartServer if we are the host, so this can show twice
	// StartServerForDemo does *not* add the map but waits for GameDataReceived
	LOG("[PreGame::%s][server=%p] using map \"%s\" (loaded=%d cached=%d)", __func__, gameServer, setup->mapName.c_str(), vfsHandler->HasArchive(setup->mapName), vfsHandler->HasTempArchive(setup->mapName));

	// load map archive
	vfsHandler->AddArchiveWithDeps(setup->mapName, false);
}

/** CONTENT: mutator overrides precede the game archive and model filename selection. */
void LegacyPreGameServices::AddModArchivesToVFS(const CGameSetup* setup)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LOG("[PreGame::%s][server=%p] using game \"%s\" (loaded=%d cached=%d)", __func__, gameServer, setup->modName.c_str(), vfsHandler->HasArchive(setup->modName), vfsHandler->HasTempArchive(setup->modName));

	// load mutators (if any); use WithDeps since mutators depend on the archives they override
	for (const std::string& mut: setup->GetMutatorsCont()) {
		LOG("[PreGame::%s] using mutator \"%s\"", __func__, mut.c_str());

		vfsHandler->AddArchiveWithDeps(mut, true);
	}

	// load game archive
	vfsHandler->AddArchiveWithDeps(setup->modName, false);

	backing.modFileName = archiveScanner->ArchiveFromName(setup->modName);
}

/** HOST SETUP: retain parsing, seed selection, map Lua, checksums and server/client creation. */
void LegacyPreGameServices::StartServer(const std::string& setupscript)
{
	assert(gameServer == nullptr);
	SCOPED_ONCE_TIMER("PreGame::StartServer");

	std::shared_ptr<GameData> startGameData(new GameData());
	std::shared_ptr<CGameSetup> startGameSetup(new CGameSetup());

	startGameSetup->Init(setupscript);
	if (startGameSetup->fixedRNGSeed == 0) {
		startGameData->SetRandomSeed(static_cast<unsigned>(guRNG.NextInt()));
	} else {
		startGameData->SetRandomSeed(startGameSetup->fixedRNGSeed);
	}

	if (startGameSetup->mapName.empty())
		throw content_error("No map selected in startscript");

	if (startGameSetup->initBlank) {
		CBlankMapGenerator gen(startGameSetup.get());
		gen.Generate();
	}


	// We must map the map into VFS this early, because server needs the start positions.
	// Take care that MapInfo isn't loaded here, as map options aren't available to it yet.
	AddMapArchivesToVFS(startGameSetup.get());

	// Loading the start positions executes the map's Lua.
	// This means start positions can NOT be influenced by map options.
	// (Which is OK, since unitsync does not have map options available either.)
	startGameSetup->LoadStartPositions();

	{
		const auto st = spring_gettime();
		archiveScanner->ResetNumFilesHashed();
		const std::string mapArchive = archiveScanner->ArchiveFromName(startGameSetup->mapName);
		const auto mapChecksum = archiveScanner->GetArchiveCompleteChecksumBytes(mapArchive);

		const std::string modArchive = archiveScanner->ArchiveFromName(startGameSetup->modName);
		const auto modChecksum = archiveScanner->GetArchiveCompleteChecksumBytes(modArchive);

		startGameData->SetMapChecksum(mapChecksum.data());
		startGameData->SetModChecksum(modChecksum.data());

		sha512::hex_digest mapChecksumHex;
		sha512::hex_digest modChecksumHex;
		sha512::dump_digest(mapChecksum, mapChecksumHex);
		sha512::dump_digest(modChecksum, modChecksumHex);

		archiveScanner->WriteCache(); // write the cache, useful in case the game loading crashes afterwards

		LOG("[PreGame::%s]\n\tmod-checksum=%s\n\tmap-checksum=%s", __func__, modChecksumHex.data(), mapChecksumHex.data());
		LOG("[PreGame::%s] Game/Map archives checksum acquisition took = %" PRId64 " microseconds", __func__, (spring_gettime() - backing.connectTimer).toMilliSecsi());

		archiveScanner->WriteCache();
	}

	good_fpu_control_registers("before CGameServer creation");
	startGameData->SetSetupText(startGameSetup->setupText);
	gameServer = new CGameServer(backing.clientSetup, startGameData, startGameSetup);

	gameServer->AddLocalClient(backing.clientSetup->myPlayerName, SpringVersion::GetSync(), Platform::GetPlatformStr());
	good_fpu_control_registers("after CGameServer creation");
}

/** DEMO SETUP: retain script rewriting and replay server initialization order. */
void LegacyPreGameServices::StartServerForDemo(const std::string& demoName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	TdfParser script((backing.gameData->GetSetupText()).c_str(), (backing.gameData->GetSetupText()).size());
	TdfParser::TdfSection* tgame = script.GetRootSection()->sections["game"];

	std::ostringstream moddedDemoScript;

	{
		// server will always use a modified copy of this
		assert(gameSetup->ScriptLoaded());

		// modify the demo's start-script so it can be used to watch the demo
		tgame->AddPair("MapName", gameSetup->mapName);
		tgame->AddPair("Gametype", gameSetup->modName);
		tgame->AddPair("Demofile", demoName);
		tgame->remove("OnlyLocal", false);
		tgame->remove("HostIP", false);
		tgame->remove("HostPort", false);
		tgame->remove("AutohostPort", false);
		tgame->remove("SourcePort", false);
		//tgame->remove("IsHost", false);

		for (auto& section: tgame->sections) {
			if (section.first.size() > 6 && section.first.substr(0, 6) == "player") {
				section.second->AddPair("isfromdemo", 1);
			}
		}

		// is this needed?
		TdfParser::TdfSection* modopts = tgame->construct_subsection("MODOPTIONS");
		modopts->remove("maxspeed", false);
		modopts->remove("minspeed", false);
	}

	script.print(moddedDemoScript);
	backing.gameData->SetSetupText(moddedDemoScript.str());

	// create the server-private demo GameSetup containing the additional player
	std::shared_ptr<CGameSetup> demoGameSetup(new CGameSetup());

	if (!demoGameSetup->Init(moddedDemoScript.str()))
		throw content_error("Demo contains incorrect script");

	LOG("[PreGame::%s] starting GameServer", __func__);
	good_fpu_control_registers("before CGameServer creation");

	gameServer = new CGameServer(backing.clientSetup, backing.gameData, demoGameSetup);
	gameServer->AddLocalClient(backing.clientSetup->myPlayerName, SpringVersion::GetSync(), Platform::GetPlatformStr());

	good_fpu_control_registers("after CGameServer creation");
	LOG("[PreGame::%s] started GameServer", __func__);
}

/** DEMO SETUP: use the original first packet and script before constructing replay server. */
void LegacyPreGameServices::ReadDataFromDemo(const std::string& demoName)
{
	SCOPED_ONCE_TIMER("PreGame::ReadDataFromDemo");
	assert(gameServer == nullptr);
	LOG("[PreGame::%s] pre-scanning demo file \"%s\" for game data...", __func__, demoName.c_str());
	CDemoReader scanner(demoName, 0.0f);

	{
		// this does not extract the RNG preseed, use first packet
		// backing.gameData.reset(new GameData(scanner.GetSetupScript()));
		backing.gameData.reset(new GameData(std::shared_ptr<netcode::RawPacket>(scanner.GetData(0.0f))));
		assert(backing.gameData->GetSetupText() == scanner.GetSetupScript());

		if (CGameSetup::LoadReceivedScript(backing.gameData->GetSetupText(), true)) {
			StartServerForDemo(demoName);
		} else {
			throw content_error("Demo contains incorrect script");
		}
	}

	assert(gameServer != nullptr);
}

/** RECEIVED SETUP: publish seeded world/setup/content state before player assignment. */
void LegacyPreGameServices::GameDataReceived(std::shared_ptr<const netcode::RawPacket> packet)
{
	SCOPED_ONCE_TIMER("PreGame::GameDataReceived");
	//FIXME PRE-007: Direct GAMEDATA worker differs from AsyncExecute FPU entry.
	// Preserve these statements until variant/setup checks establish equivalence.
	ENTER_SYNCED_CODE(); // because of async execution

	try {
		// in demos, backing.gameData is first new'ed in ReadDataFromDemo()
		// in live games it will always still be NULL at this point
		backing.gameData.reset(new GameData(packet));
	} catch (const netcode::UnpackPacketException& ex) {
		throw content_error(std::string("invalid GameData received: ") + ex.what());
	}

	// preseed the synced RNG until GameID-based NETMSG_RANDSEED arrives
	// allows proper randomness in LuaParser when executing defs.lua, etc
	gsRNG.SetSeed(backing.gameData->GetRandomSeed(), true);

	// for demos, ReadDataFromDemo precedes UpdateClientNet -> GameDataReceived
	// this means gameSetup contains data from the original game but we need the
	// modified version (cf StartServerForDemo) which the server already has that
	// contains an extra player
	gameSetup->ResetState();

	if (CGameSetup::LoadReceivedScript(backing.gameData->GetSetupText(), backing.clientSetup->isHost)) {
		assert(gameSetup->ScriptLoaded());
		gu->LoadFromSetup(gameSetup);
		gs->LoadFromSetup(gameSetup);
		// do we really need to do this so early?
		CPlayer::UpdateControlledTeams();
	} else {
		throw content_error("error loading received setup-script");
	}

	// some sanity checks
	for (int p = 0; p < playerHandler.ActivePlayers(); ++p) {
		const CPlayer* player = playerHandler.Player(p);

		if (!playerHandler.IsValidPlayer(player->playerNum))
			throw content_error("Invalid player in game-data");

		if (!teamHandler.IsValidTeam(player->team))
			throw content_error("Invalid team in game-data");

		// TODO: seems not to make sense really
		if (!teamHandler.IsValidAllyTeam(teamHandler.AllyTeam(player->team)))
			throw content_error("Invalid allyteam in game-data");
	}

	archiveScanner->ResetNumFilesHashed();

	// load archives into VFS
	AddMapArchivesToVFS(gameSetup);
	AddModArchivesToVFS(gameSetup);

	{
		// check checksums of map & game
		// mismatches happen on dedicated servers between host and clients
		// we want to know whether the *locally calculated* checksums also
		// differ among clients so use the opportunity
		// NOTE: gu->myPlayerNum is not valid yet, GameData arrives first
		sha512::raw_digest gdMapChecksum;
		sha512::raw_digest asMapChecksum;
		sha512::raw_digest gdModChecksum;
		sha512::raw_digest asModChecksum;
		sha512::hex_digest gdMapChecksumHex;
		sha512::hex_digest asMapChecksumHex;
		sha512::hex_digest gdModChecksumHex;
		sha512::hex_digest asModChecksumHex;

		std::copy(backing.gameData->GetMapChecksum(), backing.gameData->GetMapChecksum() + sha512::SHA_LEN, gdMapChecksum.begin());
		std::copy(backing.gameData->GetModChecksum(), backing.gameData->GetModChecksum() + sha512::SHA_LEN, gdModChecksum.begin());
		std::fill(asMapChecksum.begin(), asMapChecksum.end(), 0);
		std::fill(asModChecksum.begin(), asModChecksum.end(), 0);

		try {
			// gameSetup->MapFileName()
			archiveScanner->CheckArchive(gameSetup->mapName, gdMapChecksum, asMapChecksum);
		} catch (const content_error& ex) {
			LOG_L(L_WARNING, "[PreGame::%s] %s", __func__, ex.what());
		}
		try {
			archiveScanner->CheckArchive(backing.modFileName, gdModChecksum, asModChecksum);
		} catch (const content_error& ex) {
			LOG_L(L_WARNING, "[PreGame::%s] %s", __func__, ex.what());
		}

		sha512::dump_digest(gdMapChecksum, gdMapChecksumHex);
		sha512::dump_digest(gdModChecksum, gdModChecksumHex);
		sha512::dump_digest(asMapChecksum, asMapChecksumHex);
		sha512::dump_digest(asModChecksum, asModChecksumHex);

		std::memset(mapChecksumMsgBuf, 0, sizeof(mapChecksumMsgBuf));
		std::memset(modChecksumMsgBuf, 0, sizeof(modChecksumMsgBuf));
		std::snprintf(mapChecksumMsgBuf, sizeof(mapChecksumMsgBuf), "[PreGame::%s][map-checksums]\n\tserver=%s\n\tclient=%s", __func__, gdMapChecksumHex.data(), asMapChecksumHex.data());
		std::snprintf(modChecksumMsgBuf, sizeof(modChecksumMsgBuf), "[PreGame::%s][mod-checksums]\n\tserver=%s\n\tclient=%s", __func__, gdModChecksumHex.data(), asModChecksumHex.data());
	}

	// script.txt allows to disable demo file recording (host only, used for menu)
	if (backing.clientSetup->isHost && !gameSetup->recordDemo)
		backing.wantDemo = false;

	if (clientNet != nullptr && backing.wantDemo) {
		CDemoRecorder recorder = {gameSetup->mapName, gameSetup->modName, false};

		recorder.WriteSetupText(backing.gameData->GetSetupText());
		recorder.SaveToDemo(packet->data, packet->length, clientNet->GetPacketTime(gs->frameNum));

		assert(!clientNet->GetDemoRecorder()->IsValid());
		clientNet->SetDemoRecorder(std::move(recorder));
		assert(clientNet->GetDemoRecorder()->IsValid());

		LOG("[PreGame::%s] recording demo to \"%s\"", __func__, (clientNet->GetDemoRecorder()->GetName()).c_str());
	}

	//FIXME PRE-003: Exceptions skip this explicit exit in the current code.
	// Do not silently add RAII restoration while extracting the operation.
	LEAVE_SYNCED_CODE();
}

}
#endif
