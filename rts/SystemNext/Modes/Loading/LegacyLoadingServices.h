/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

//FIXME [LOAD-008] This complete adapter proposal is disabled, not typechecked.
// CLoadScreen owns private fields and singleton; no friend/read interface or
// retirement-safe binding exists yet. Enable only after extracting the matching
// controller bodies into these operations, removing their original bodies, and
// validating partial initialization, callbacks and teardown. No second owner or
// production dispatcher is introduced by this proposal.
#if 0
#include "ILoadingServices.h"
#include "Game/LoadScreen.h"

namespace runtime {
class LegacyLoadingServices final : public ILoadingServices {
public:
	void BeginLoading(std::string&& mapFileName, std::string&& modFileName, ILoadSaveHandler* saveFile);
	void DeliverProgressNotifications() override;
	bool IsGameLoadingComplete() const override;
	bool IsMultithreadedLoading() const override;
	void KeepWindowResponsive() override;
	void RetireLoadingController() override;
	void AnnounceLoadingCompletion() override;
	std::unique_ptr<LoadingProgressScope> QueueProgressNotification(std::string_view text, bool replaceLast) override;
	void PaceLoadingFrame() override;
	void AdvanceDrawCounter() override;
	void MaintainLobbyConnection() override;
	bool HasLoadingIntro() const override;
	void UpdateLoadingIntro() override;
	void DrawLoadingGenesis() override;
	void ClearLoadingScreen() override;
	void DrawLoadingScreen() override;
	void PresentSynchronousLoadingFrame() override;
	void ResizeLoadingIntro();
	int PressLoadingKey(int keyCode, int scanCode, bool isRepeat);
	int ReleaseLoadingKey(int keyCode, int scanCode);

private:
	CLoadScreen& Backing() const;
	bool InitializeLoading();
	void ConfigureLoadingThread();
	void StartHeartbeatAndGame();
	void StartLoadingWorker();
	void InitializeLoadingIntro();
	void StopLoadingResources();
	void StopHeartbeatAndActivateGame();
};
}
#endif
