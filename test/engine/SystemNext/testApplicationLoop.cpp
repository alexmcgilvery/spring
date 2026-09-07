/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "SystemNext/ApplicationLoop.h"
#include "SystemNext/Presentation/SerialVisualFrame.h"
#include "SystemNext/Session/IRuntimeMode.h"
#include "SystemNext/Session/Session.h"

namespace {
struct Services;
struct Visuals final : runtime::SerialVisualFrame {
	explicit Visuals(Services& services): services(services) {}
	runtime::ApplicationStatus UpdateClientMode() override;
	void LockDraw() override;
	void UnlockDraw() noexcept override;
	bool Draw() override;
	void Present(bool allowSwap) override;
	Services& services;
};

struct Services final : runtime::ILoopInput, runtime::ILoopLifecycle, runtime::ILoopPlatform,
	runtime::ILoopDiagnostics, runtime::IRuntimeMode, runtime::Session {
	Visuals visuals{*this};
	std::vector<std::string> calls;
	std::vector<runtime::Phase> scopes;
	std::string failure;
	int controller = 1;
	int replacement = 1;
	int replacementOnLock = -1;
	int sessionUpdates = 0;
	int clientUpdates = 0;
	int reloads = 0;
	int actualSwaps = 0;
	int exceptionalScopes = 0;
	int flushes = 0;
	bool exit = false;
	bool reload = false;
	bool inputExit = false;
	bool updateExit = false;
	bool inputReload = false;
	bool reloadExit = false;
	bool updateResult = true;
	bool drawResult = true;
	bool forceSwap = false;
	bool held = false;

	runtime::LoopServices Bind() { return {*this, *this, *this, *this, *this, *this, visuals}; }
	void Call(const std::string& name) {
		calls.push_back(name);
		if (failure == name)
			throw std::runtime_error(name);
	}
	bool UpdateController() {
		CHECK_FALSE(held);
		if (controller == 0)
			return true;
		Call("update:" + std::to_string(controller));
		controller = replacement;
		exit |= updateExit;
		return updateResult;
	}
	void ProcessEvents() override { Call("input"); exit |= inputExit; reload |= inputReload; }
	bool ExitRequested() const override { return exit; }
	bool ReloadRequested() const override { return reload; }
	void ServiceWatchdog() override { Call("watchdog"); }
	void ProcessQueuedSave() override { Call("save"); }
	void ReloadSession() override { Call("reload"); ++reloads; reload = false; exit |= reloadExit; }
	void RequestExit() override { Call("exit"); exit = true; }
	void UpdateConfiguration() override { Call("config"); }
	void UpdateWindow() override { Call("window"); }
	void UpdateClock() override { Call("clock"); }
	runtime::PhaseToken BeginPhase(runtime::Phase phase) noexcept override {
		scopes.push_back(phase);
		return {phase, 1, 0, std::uncaught_exceptions()};
	}
	void EndPhase(runtime::PhaseToken token) noexcept override {
		CHECK(scopes.back() == token.phase);
		scopes.pop_back();
		exceptionalScopes += std::uncaught_exceptions() > token.exceptions;
	}
	void Flush() noexcept override {
		CHECK(scopes == std::vector<runtime::Phase>{runtime::Phase::Host});
		++flushes;
		calls.push_back("flush");
	}
	bool HandlesSession() const override { return controller == 1 || controller == 3 || controller == 4; }
	runtime::SessionUpdate UpdateSession(runtime::Session& session) override { return session.Advance(); }
	runtime::SessionUpdate Advance() override {
		++sessionUpdates;
		return runtime::SessionUpdate::FromContinuation(UpdateController());
	}
};

runtime::ApplicationStatus Visuals::UpdateClientMode()
{
	++services.clientUpdates;
	return services.UpdateController() ? runtime::ApplicationStatus::Continue : runtime::ApplicationStatus::ExitRequested;
}

void Visuals::LockDraw()
{
	services.Call("lock");
	services.held = true;
	if (services.replacementOnLock >= 0)
		services.controller = services.replacementOnLock;
}

void Visuals::UnlockDraw() noexcept
{
	services.held = false;
	services.calls.push_back("unlock");
}

bool Visuals::Draw()
{
	CHECK(services.held);
	if (services.controller == 0)
		return false;
	services.Call("draw:" + std::to_string(services.controller));
	return services.drawResult;
}

void Visuals::Present(bool allowSwap)
{
	CHECK(services.held);
	services.Call(allowSwap ? "present:true" : "present:false");
	services.actualSwaps += allowSwap || services.forceSwap;
}
}

TEST_CASE("Application iteration preserves service and visual ordering")
{
	Services services;
	runtime::ApplicationLoop loop(services.Bind());
	loop.RunIteration();
	CHECK(services.calls == std::vector<std::string>{"watchdog", "input", "save", "config", "window", "clock", "update:1", "lock", "draw:1", "present:true", "unlock", "flush"});
	CHECK(services.sessionUpdates == 1);
	CHECK(services.clientUpdates == 0);
	CHECK_FALSE(services.held);
	CHECK(services.scopes.empty());
}

TEST_CASE("Mode transitions never update a replacement twice")
{
	Services services;
	SECTION("menu enters game") { services.controller = 2; services.replacement = 1; }
	SECTION("loading enters game") { services.controller = 3; services.replacement = 1; }
	SECTION("game returns to menu") { services.replacement = 2; }
	SECTION("game replaces game") { services.replacement = 4; }
	SECTION("controller removed") { services.replacement = 0; }
	SECTION("controller replaced while acquiring loading guard") { services.replacementOnLock = 4; }
	const bool wasSession = services.HandlesSession();
	runtime::ApplicationLoop loop(services.Bind());
	loop.RunIteration();
	CHECK(services.sessionUpdates == int(wasSession));
	CHECK(services.clientUpdates == int(!wasSession));
	CHECK(services.calls[7] == "lock");
	CHECK(services.calls[8] == (services.controller == 0 ? "present:false" : "draw:" + std::to_string(services.controller)));
}

TEST_CASE("Visual decisions preserve skipped draws and forced presentation")
{
	Services services;
	SECTION("session requests exit") { services.updateResult = false; }
	SECTION("menu requests exit") { services.controller = 2; services.replacement = 2; services.updateResult = false; }
	SECTION("draw returns false") { services.drawResult = false; }
	SECTION("absent controller") { services.controller = 0; }
	SECTION("update false with forced swap") { services.updateResult = false; services.forceSwap = true; }
	SECTION("draw false with forced swap") { services.drawResult = false; services.forceSwap = true; }
	runtime::ApplicationLoop loop(services.Bind());
	loop.RunIteration();
	CHECK(services.exit == !services.updateResult);
	CHECK(services.actualSwaps == int(services.forceSwap));
	CHECK_FALSE(services.held);
	CHECK(services.calls.back() == "flush");
	const auto present = std::find(services.calls.begin(), services.calls.end(), "present:false");
	REQUIRE(present != services.calls.end());
	CHECK(*(present + 1) == "unlock");
	if (!services.updateResult)
		CHECK(*(present + 2) == "exit");
}

TEST_CASE("Lifecycle requests keep the current iteration ordering")
{
	Services services;
	runtime::ApplicationLoop loop(services.Bind());
	SECTION("failed initialization or preexisting exit does no work") {
		services.exit = true;
		loop.Run();
		CHECK(services.calls.empty());
	}
	SECTION("input exit does not truncate the iteration") {
		services.inputExit = true;
		loop.Run();
		CHECK(services.sessionUpdates == 1);
		CHECK(services.actualSwaps == 1);
		CHECK(services.flushes == 1);
	}
	SECTION("exit set during successful update is not cleared") {
		services.updateExit = true;
		loop.Run();
		CHECK(services.exit);
		CHECK(services.actualSwaps == 1);
		CHECK(services.flushes == 1);
	}
	SECTION("input reload runs save first and excludes update and draw") {
		services.inputReload = true;
		services.reloadExit = true;
		loop.Run();
		CHECK(services.calls == std::vector<std::string>{"watchdog", "input", "save", "reload", "flush"});
		CHECK(services.sessionUpdates == 0);
		CHECK(services.clientUpdates == 0);
	}
	SECTION("bindings continue to resolve new modes across reloads") {
		for (int i = 0; i < 4; ++i) {
			services.reload = true;
			loop.RunIteration();
			services.controller = (i % 2) ? 1 : 2;
			services.replacement = services.controller;
			loop.RunIteration();
		}
		CHECK(services.reloads == 4);
		CHECK(services.sessionUpdates == 2);
		CHECK(services.clientUpdates == 2);
	}
}

TEST_CASE("Engine exceptions propagate and unwind scopes without a diagnostic drain")
{
	for (const auto* failure: {"watchdog", "input", "save", "config", "window", "clock", "update:1", "lock", "draw:1", "present:true", "reload"}) {
		Services services;
		services.failure = failure;
		services.reload = services.failure == "reload";
		runtime::ApplicationLoop loop(services.Bind());
		CHECK_THROWS_AS(loop.RunIteration(), std::runtime_error);
		CHECK_FALSE(services.held);
		CHECK(services.scopes.empty());
		CHECK(services.exceptionalScopes >= 1);
		CHECK(services.flushes == 0);
		if (services.failure == "draw:1" || services.failure == "present:true")
			CHECK(services.calls.back() == "unlock");
	}
}

TEST_CASE("Session results expose independent continuation and visual facts")
{
	const auto [noSessionStatus, noSessionContext] = runtime::SessionUpdate::NoSession();
	CHECK(noSessionStatus == runtime::ApplicationStatus::Continue);
	CHECK(noSessionContext.sessionOutcome == runtime::SessionOutcome::NoSession);
	const auto [continueStatus, continueContext] = runtime::SessionUpdate::FromContinuation(true);
	CHECK(continueStatus == runtime::ApplicationStatus::Continue);
	CHECK(continueContext.sessionOutcome == runtime::SessionOutcome::Continue);
	const auto [exitStatus, exitContext] = runtime::SessionUpdate::FromContinuation(false);
	CHECK(exitStatus == runtime::ApplicationStatus::ExitRequested);
	CHECK(exitContext.sessionOutcome == runtime::SessionOutcome::ExitRequested);
}
