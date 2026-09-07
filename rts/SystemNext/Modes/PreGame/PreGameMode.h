/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <memory>
#include <string>

#include "SystemNext/Modes/Mode.h"

class CPreGame;
class CGameSetup;
namespace netcode { class RawPacket; }

namespace runtime {
/**
 * Own connection input, authoritative setup and connection-screen rendering.
 *
 * CPreGame retains data ownership and compatibility entry points. Operations
 * resolve that backing at invocation boundaries; unresolved task or retirement
 * paths stop explicitly before unsafe statements rather than claim success.
 */
//FIXME ADAPTER-PREGAME-BACKING: The implementation copy currently
// reads private legacy fields/helpers. Its added friendship and legacy-side
// forwarding have been removed. Supply backing state/access entirely in
// SystemNext before compiling or connecting this adapter; legacy stays intact.
class PreGameMode final : public Mode {
public:
	bool HandlesSession() const final;
	DisplayPhase GetDisplayPhase() const final;
	SessionUpdate UpdateSession(Session& session) final;
	RenderResult Render(ModeFrame& frame) final;

	int KeyPressed(int keyCode, int scanCode, bool isRepeat) override;
	void InitializeConnection();
	void ReleaseConnectionScreen();
	using AsyncExecFuncType = void (CPreGame::*)(const std::string&);
	void AsyncExecute(AsyncExecFuncType execFunc, const std::string& argument);
	void LoadSetupScript(const std::string& script);
	void LoadDemoFile(const std::string& demo);
	void LoadSaveFile(const std::string& save);

private:
	CPreGame& Backing() const;
	void ProcessConnectionPackets();
	bool HasPendingSetupTask();
	void AddMapArchivesToVFS(const CGameSetup* setup);
	void AddModArchivesToVFS(const CGameSetup* setup);
	void StartServer(const std::string& setupscript);
	void StartServerForDemo(const std::string& demoName);
	void ReadDataFromDemo(const std::string& demoName);
	void GameDataReceived(std::shared_ptr<const netcode::RawPacket> packet);

	// Setup produces these messages before SETPLAYERNUM sends them. Singleton
	// mode lifetime preserves their former process lifetime without a second copy.
	char mapChecksumMsgBuf[1024] = {};
	char modChecksumMsgBuf[1024] = {};
};
}
