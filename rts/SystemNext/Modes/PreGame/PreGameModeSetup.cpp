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

/** Create transport after the backing constructor initializes setup fields. */
void PreGameMode::InitializeConnection()
{
	auto& backing = Backing();

	assert(clientNet == nullptr);

	clientNet = new CNetProtocol();
	SetActiveController(&backing);

#ifdef SYNCDEBUG
	CSyncDebugger::GetInstance()->Initialize(backing.clientSetup->isHost, 64); //FIXME: add actual number of player
#endif

	if (!backing.clientSetup->isHost) {
		LOG("[%s] using client IP %s and port %i", "CPreGame", backing.clientSetup->hostIP.c_str(), backing.clientSetup->hostPort);
		// don't allow luasocket to connect to the host
		luaSocketRestrictions->addRule(CLuaSocketRestrictions::UDP_CONNECT, backing.clientSetup->hostIP, backing.clientSetup->hostPort, false);
		clientNet->InitClient(backing.clientSetup, SpringVersion::GetSync(), Platform::GetPlatformStr());
	} else {
		LOG("[%s] using server IP %s and port %i", "CPreGame", backing.clientSetup->hostIP.c_str(), backing.clientSetup->hostPort);
		clientNet->InitLocalClient();
	}
}

/** Release GUI and global binding without hiding a task wait in visual teardown. */
void PreGameMode::ReleaseConnectionScreen()
{
	#ifndef HEADLESS
	// delete leftover aGUI elements but not infoconsole, it is reused by CGame
	agui::gui->Clean();
	#endif

	pregame = nullptr;
}

/** Start setup work only when its lifetime can be retained through cancellation. */
void PreGameMode::AsyncExecute(AsyncExecFuncType execFunc, const std::string& argument)
{
	auto& backing = Backing();

	//FIXME PRE-001: This raw capture can race backing destruction, and bad-save
	// handling can request retirement from its own worker. Keep the exact launch
	// visible until tasks have an owner that safely joins before field teardown.
#if 0

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
#endif
	throw IncompleteFlow{"PRE-001", "Startup task lifetime is not bound"};
}

/** Start hosting from the selected script. */
void PreGameMode::LoadSetupScript(const std::string& script)
{
	auto& backing = Backing();

	RECOIL_DETAILED_TRACY_ZONE;
	assert(backing.clientSetup->isHost);
	StartServer(script);
}

/** Apply recording policy before interpreting the original demo setup. */
void PreGameMode::LoadDemoFile(const std::string& demo)
{
	auto& backing = Backing();

	RECOIL_DETAILED_TRACY_ZONE;
	assert(backing.clientSetup->isHost);
	backing.wantDemo &= configHandler->GetBool("DemoFromDemo");

	ReadDataFromDemo(demo);
}

/** Validate save startup information before transferring its handler to loading. */
void PreGameMode::LoadSaveFile(const std::string& save)
{
	auto& backing = Backing();

	RECOIL_DETAILED_TRACY_ZONE;
	assert(backing.clientSetup->isHost);

	backing.saveFileHandler = ILoadSaveHandler::CreateHandler(save);

	if (backing.saveFileHandler->LoadGameStartInfo(save) || configHandler->GetBool("LoadBadSaves")) {
		StartServer(backing.saveFileHandler->GetScriptText());
		return;
	}

	LOG_L(L_ERROR, "[PreGame::%s] incompatible save-file specified", "LoadSaveFile");

	spring::SafeDelete(backing.saveFileHandler);

	if (CLuaMenuController::ActivateInstance("[PreGame] incompatible save-file")) {
		assert(pregame == &backing);
		//FIXME PRE-002: The retained statement destroys backing inside this call.
		// Stop until mode binding and task retirement are coordinated.
#if 0
		spring::SafeDelete(pregame);
#endif
		throw IncompleteFlow{"PRE-002", "Pregame retirement is not bound to mode lifetime"};
		return;
	}

	spring::exitCode = spring::EXIT_CODE_BADSAVE;
	gu->globalQuit = true;
}

/** Mount the map before Lua/start-position consumers use its data. */
void PreGameMode::AddMapArchivesToVFS(const CGameSetup* setup)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// map gets added in StartServer if we are the host, so this can show twice
	// StartServerForDemo does *not* add the map but waits for GameDataReceived
	LOG("[PreGame::%s][server=%p] using map \"%s\" (loaded=%d cached=%d)", "AddMapArchivesToVFS", gameServer, setup->mapName.c_str(), vfsHandler->HasArchive(setup->mapName), vfsHandler->HasTempArchive(setup->mapName));

	// load map archive
	vfsHandler->AddArchiveWithDeps(setup->mapName, false);
}

/** Apply mutator overrides before game mounting and archive-name selection. */
void PreGameMode::AddModArchivesToVFS(const CGameSetup* setup)
{
	auto& backing = Backing();

	RECOIL_DETAILED_TRACY_ZONE;
	LOG("[PreGame::%s][server=%p] using game \"%s\" (loaded=%d cached=%d)", "AddModArchivesToVFS", gameServer, setup->modName.c_str(), vfsHandler->HasArchive(setup->modName), vfsHandler->HasTempArchive(setup->modName));

	// load mutators (if any); use WithDeps since mutators depend on the archives they override
	for (const std::string& mut: setup->GetMutatorsCont()) {
		LOG("[PreGame::%s] using mutator \"%s\"", "AddModArchivesToVFS", mut.c_str());

		vfsHandler->AddArchiveWithDeps(mut, true);
	}

	// load game archive
	vfsHandler->AddArchiveWithDeps(setup->modName, false);

	backing.modFileName = archiveScanner->ArchiveFromName(setup->modName);
}

/** Preserve parsing, seed, map Lua, hashing and server creation order. */
void PreGameMode::StartServer(const std::string& setupscript)
{
	auto& backing = Backing();

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

		LOG("[PreGame::%s]\n\tmod-checksum=%s\n\tmap-checksum=%s", "StartServer", modChecksumHex.data(), mapChecksumHex.data());
		LOG("[PreGame::%s] Game/Map archives checksum acquisition took = %" PRId64 " microseconds", "StartServer", (spring_gettime() - backing.connectTimer).toMilliSecsi());

		archiveScanner->WriteCache();
	}

	good_fpu_control_registers("before CGameServer creation");
	startGameData->SetSetupText(startGameSetup->setupText);
	gameServer = new CGameServer(backing.clientSetup, startGameData, startGameSetup);

	gameServer->AddLocalClient(backing.clientSetup->myPlayerName, SpringVersion::GetSync(), Platform::GetPlatformStr());
	good_fpu_control_registers("after CGameServer creation");
}

/** Rewrite replay setup before creating its server and local client. */
void PreGameMode::StartServerForDemo(const std::string& demoName)
{
	auto& backing = Backing();

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

	LOG("[PreGame::%s] starting GameServer", "StartServerForDemo");
	good_fpu_control_registers("before CGameServer creation");

	gameServer = new CGameServer(backing.clientSetup, backing.gameData, demoGameSetup);
	gameServer->AddLocalClient(backing.clientSetup->myPlayerName, SpringVersion::GetSync(), Platform::GetPlatformStr());

	good_fpu_control_registers("after CGameServer creation");
	LOG("[PreGame::%s] started GameServer", "StartServerForDemo");
}

/** Use original first game data before constructing replay setup. */
void PreGameMode::ReadDataFromDemo(const std::string& demoName)
{
	auto& backing = Backing();

	SCOPED_ONCE_TIMER("PreGame::ReadDataFromDemo");
	assert(gameServer == nullptr);
	LOG("[PreGame::%s] pre-scanning demo file \"%s\" for game data...", "ReadDataFromDemo", demoName.c_str());
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

/** Publish seeded setup and content before player assignment consumes them. */
void PreGameMode::GameDataReceived(std::shared_ptr<const netcode::RawPacket> packet)
{
	auto& backing = Backing();

	SCOPED_ONCE_TIMER("PreGame::GameDataReceived");
	//FIXME PRE-007: This worker entry differs from AsyncExecute's streflop
	// initialization. Task launch remains blocked until FPU behavior is verified.
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
			LOG_L(L_WARNING, "[PreGame::%s] %s", "GameDataReceived", ex.what());
		}
		try {
			archiveScanner->CheckArchive(backing.modFileName, gdModChecksum, asModChecksum);
		} catch (const content_error& ex) {
			LOG_L(L_WARNING, "[PreGame::%s] %s", "GameDataReceived", ex.what());
		}

		sha512::dump_digest(gdMapChecksum, gdMapChecksumHex);
		sha512::dump_digest(gdModChecksum, gdModChecksumHex);
		sha512::dump_digest(asMapChecksum, asMapChecksumHex);
		sha512::dump_digest(asModChecksum, asModChecksumHex);

		std::memset(mapChecksumMsgBuf, 0, sizeof(mapChecksumMsgBuf));
		std::memset(modChecksumMsgBuf, 0, sizeof(modChecksumMsgBuf));
		std::snprintf(mapChecksumMsgBuf, sizeof(mapChecksumMsgBuf), "[PreGame::%s][map-checksums]\n\tserver=%s\n\tclient=%s", "GameDataReceived", gdMapChecksumHex.data(), asMapChecksumHex.data());
		std::snprintf(modChecksumMsgBuf, sizeof(modChecksumMsgBuf), "[PreGame::%s][mod-checksums]\n\tserver=%s\n\tclient=%s", "GameDataReceived", gdModChecksumHex.data(), asModChecksumHex.data());
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

		LOG("[PreGame::%s] recording demo to \"%s\"", "GameDataReceived", (clientNet->GetDemoRecorder()->GetName()).c_str());
	}

	LEAVE_SYNCED_CODE();
}

}
