/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "IPreGameServices.h"

//FIXME PRE-BIND: This proposed binding needs private CPreGame access, external
// task/retirement ownership, and access to the existing checksum message buffers.
// Its full operation bodies are retained in the companion source under #if 0;
// they are NOT typechecked or active. Do not enable by adding friendship alone.
#if 0
#include <memory>
#include <string>

class CPreGame;
class CGameSetup;
namespace netcode { class RawPacket; }

namespace runtime {
/**
 * Reuse pregame state while separating input, session setup and connection visuals.
 * The reference documents current ownership; it is not safe across retirement.
 */
class LegacyPreGameServices final : public IPreGameServices {
public:
	explicit LegacyPreGameServices(CPreGame& backing);

	void EnterSyncedCode() override;
	void CheckFloatingPointControl() override;
	void LeaveSyncedCode() override;
	bool HasPendingSetupTask() override;
	void UpdateConnectionTransport() override;
	bool HandleConnectionTimeout() override;
	PreGamePacketResult ProcessNextConnectionPacket() override;

	bool HasConnectionScreen() const override;
	void ClearConnectionScreen() override;
	void BeginConnectionText() override;
	void DrawConnectionStatus() override;
	void DrawConnectionIdentity() override;
	void DrawArchiveChecksumProgress() override;
	void DrawAbortInstructionsAndCredits() override;
	void EndConnectionText() override;

	// Input preserves cancellation policy; it does not advance the connection.
	int HandleConnectionCancel(int keyCode, int scanCode, bool isRepeat);

	// Session entry/exit operations remain separate from repeated servicing.
	void InitializeConnection();
	void ReleaseConnectionScreen();
	using AsyncExecFuncType = void (LegacyPreGameServices::*)(const std::string&);
	void AsyncExecute(AsyncExecFuncType execFunc, const std::string& argument);
	void LoadSetupScript(const std::string& script);
	void LoadDemoFile(const std::string& demo);
	void LoadSaveFile(const std::string& save);

private:
	void AddMapArchivesToVFS(const CGameSetup* setup);
	void AddModArchivesToVFS(const CGameSetup* setup);
	void StartServer(const std::string& setupscript);
	void StartServerForDemo(const std::string& demoName);
	void ReadDataFromDemo(const std::string& demoName);
	void GameDataReceived(std::shared_ptr<const netcode::RawPacket> packet);

	CPreGame& backing;
};
}
#endif
