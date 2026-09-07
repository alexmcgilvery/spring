/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <catch_amalgamated.hpp>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include "SystemNext/ApplicationLoop.h"
#include "SystemNext/Presentation/SerialVisualFrame.h"
#include "SystemNext/Modes/Mode.h"
#include "SystemNext/Modes/ModeBinding.h"
#include "SystemNext/Session/Session.h"

namespace {
struct Host;
struct Graphics final : runtime::SerialVisualFrame {
	explicit Graphics(Host& host): host(host) {}
	Host& host;
	void LockDraw() override;
	void UnlockDraw() noexcept override;
	void Present(bool allowSwap) override;
};
struct Host final : runtime::ILoopInput, runtime::ILoopLifecycle, runtime::ILoopPlatform, runtime::ILoopDiagnostics {
	Graphics graphics{*this};
	runtime::Session session;
	std::vector<std::string> calls;
	std::vector<runtime::Phase> scopes;
	std::vector<std::string> blocks;
	std::string failure;
	int controller = 1;
	int replacement = 1;
	int replacementOnLock = -1;
	std::uint64_t generation = 1;
	bool held = false;
	bool exit = false;
	bool inputExit = false;
	bool reload = false;
	void Select(int id) { controller = id; ++generation; }
	void Call(const std::string& call) {
		calls.push_back(call);
		if (failure == call) throw std::runtime_error(call);
	}
	void ProcessEvents() override { Call("input"); exit |= inputExit; }
	bool ExitRequested() const override { return exit; }
	bool ReloadRequested() const override { return reload; }
	void ServiceWatchdog() override { Call("watchdog"); }
	void ProcessQueuedSave() override { Call("save"); }
	void ReloadSession() override { Call("reload"); reload = false; ++generation; }
	void RequestExit() override { Call("exit"); exit = true; }
	void UpdateConfiguration() override { Call("config"); }
	void UpdateWindow() override { Call("window"); }
	void UpdateClock() override { Call("clock"); }
	runtime::PhaseToken BeginPhase(runtime::Phase phase) noexcept override {
		scopes.push_back(phase);
		return {phase, 1, 0, std::uncaught_exceptions()};
	}
	void EndPhase(runtime::PhaseToken token) noexcept override {
		CHECK(scopes.back() == token.phase); scopes.pop_back();
	}
	void Flush() noexcept override { calls.push_back("flush"); }
	void ReportBlocked(const runtime::BlockedFlow& block) noexcept override {
		blocks.push_back(block.id); calls.push_back("blocked:" + block.id);
	}
};
void Graphics::LockDraw() { host.Call("lock"); host.held = true; if (host.replacementOnLock >= 0) host.Select(host.replacementOnLock); }
void Graphics::UnlockDraw() noexcept { host.held = false; host.calls.push_back("unlock"); }
void Graphics::Present(bool allowSwap) { CHECK(host.held); host.Call(allowSwap ? "present:true" : "present:false"); }
struct FrameData final : runtime::ModeFrameData {
	explicit FrameData(Host& host): host(host) {}
	Host& host;
	~FrameData() override { CHECK(host.held); host.calls.push_back("frame-destroy"); }
};
struct TestMode final : runtime::Mode {
	TestMode(Host& host, int id, bool session, runtime::DisplayPhase phase): host(host), id(id), session(session), phase(phase) {}
	Host& host;
	int id;
	bool session;
	runtime::DisplayPhase phase;
	bool replaceSession = false;
	bool replaceDisplay = false;
	bool replaceRender = false;
	bool trackFrame = false;
	bool skip = false;
	bool exit = false;
	std::string block;
	bool HandlesSession() const override { return session; }
	runtime::DisplayPhase GetDisplayPhase() const override { return phase; }
	runtime::SessionUpdate UpdateSession(runtime::Session&) override {
		CHECK_FALSE(host.held); host.Call("session:" + std::to_string(id));
		if (replaceSession) host.Select(host.replacement);
		if (block == "session") return runtime::SessionUpdate::Incomplete("TEST-SESSION", "required session work missing");
		return runtime::SessionUpdate::FromContinuation(!exit);
	}
	runtime::ApplicationStatus UpdateDisplay(runtime::ModeFrame& frame) override {
		CHECK(host.held == (phase == runtime::DisplayPhase::WithGraphics));
		host.Call("display:" + std::to_string(id));
		if (trackFrame) frame.data = std::make_unique<FrameData>(host);
		if (replaceDisplay) host.Select(host.replacement);
		if (block == "display") return frame.Block("TEST-DISPLAY", "required display work missing");
		frame.allowRender &= !skip;
		return exit ? runtime::ApplicationStatus::ExitRequested : runtime::ApplicationStatus::Continue;
	}
	runtime::RenderResult Render(runtime::ModeFrame& frame) override {
		CHECK(host.held); CHECK(frame.bindingGeneration == host.generation);
		if (trackFrame) CHECK(frame.data != nullptr);
		host.Call("render:" + std::to_string(id));
		if (replaceRender) host.Select(host.replacement);
		if (block == "render") return runtime::RenderResult::Blocked("TEST-RENDER", "required render work missing");
		return skip ? runtime::RenderResult::Skipped() : runtime::RenderResult::Ready();
	}
};
struct Bindings final : runtime::ModeBinding {
	Host& host;
	TestMode game{host, 1, true, runtime::DisplayPhase::WithGraphics};
	TestMode menu{host, 2, false, runtime::DisplayPhase::BeforeGraphics};
	TestMode connection{host, 3, true, runtime::DisplayPhase::None};
	explicit Bindings(Host& host): host(host) {}
	runtime::Mode* Resolve() override {
		switch (host.controller) { case 1: return &game; case 2: return &menu; case 3: return &connection; default: return nullptr; }
	}
	std::uint64_t Generation() const override { return host.generation; }
	runtime::LoopServices Services() { return {host, host, host, host, host.session, host.graphics, *this}; }
};
int Count(const Host& host, const std::string& call) { return std::count(host.calls.begin(), host.calls.end(), call); }
}

TEST_CASE("One mode path orders input session display render and present")
{
	Host host; Bindings modes(host); runtime::ApplicationLoop loop(modes.Services()); loop.RunIteration();
	CHECK(host.calls == std::vector<std::string>{"watchdog","input","save","config","window","clock","session:1","lock","display:1","render:1","present:true","unlock","flush"});
	CHECK(host.blocks.empty()); CHECK_FALSE(host.held); CHECK(host.scopes.empty());
}
TEST_CASE("Session capability does not imply display work")
{
	Host host; Bindings modes(host);
	SECTION("menu") { host.controller=2; }
	SECTION("connection") { host.controller=3; }
	SECTION("inactive") { host.controller=0; }
	runtime::ApplicationLoop loop(modes.Services()); loop.RunIteration();
	CHECK(Count(host,"session:1")==0);
	CHECK(Count(host,"display:2")==int(host.controller==2));
	CHECK(Count(host,"display:3")==0);
	CHECK(Count(host,"session:3")==int(host.controller==3));
	CHECK(Count(host,"present:false")==int(host.controller==0));
}
TEST_CASE("Replacement before graphics receives no duplicate client or session update")
{
	Host host; Bindings modes(host);
	SECTION("session to menu") { host.replacement=2; modes.game.replaceSession=true; }
	SECTION("guard to menu") { host.replacementOnLock=2; }
	SECTION("menu to game") { host.controller=2; modes.menu.replaceDisplay=true; }
	runtime::ApplicationLoop loop(modes.Services()); loop.RunIteration();
	CHECK(Count(host,"display:2")==int(modes.menu.replaceDisplay));
	CHECK(Count(host,"session:1")==int(!modes.menu.replaceDisplay));
	CHECK(Count(host,"render:"+std::to_string(host.controller))==1);
	CHECK(host.blocks.empty());
}
TEST_CASE("Frame scope ends before present and unwinds under the graphics guard")
{
	Host host; Bindings modes(host); modes.game.trackFrame=true;
	SECTION("normal") {}
	SECTION("blocked render") { modes.game.block="render"; }
	SECTION("throwing render") { host.failure="render:1"; }
	SECTION("unguarded display state retained") { host.controller=2; modes.menu.trackFrame=true; }
	runtime::ApplicationLoop loop(modes.Services());
	if (host.failure.empty()) loop.RunIteration(); else CHECK_THROWS_AS(loop.RunIteration(),std::runtime_error);
	CHECK(Count(host,"frame-destroy")==1);
	const auto destroyed=std::find(host.calls.begin(),host.calls.end(),"frame-destroy");
	CHECK(destroyed<std::find(host.calls.begin(),host.calls.end(),"unlock"));
	CHECK(destroyed<std::find(host.calls.begin(),host.calls.end(),"present:true"));
}
TEST_CASE("Required blocked work never reaches dependent render or present")
{
	Host host; Bindings modes(host);
	SECTION("session") { modes.game.block="session"; }
	SECTION("display") { modes.game.block="display"; }
	SECTION("render") { modes.game.block="render"; }
	runtime::ApplicationLoop loop(modes.Services()); loop.RunIteration();
	REQUIRE(host.blocks.size()==1); CHECK(Count(host,"present:true")==0); CHECK(Count(host,"present:false")==0);
	CHECK_FALSE(host.held); CHECK(host.scopes.empty());
	const int sessionCalls=Count(host,"session:1"); loop.RunIteration();
	CHECK(Count(host,"session:1")==sessionCalls); CHECK(Count(host,"input")==2); CHECK(Count(host,"flush")==2);
	host.Select(2); loop.RunIteration(); CHECK(Count(host,"render:2")==1);
}
TEST_CASE("Binding generation prevents rendering reused mode state")
{
	Host host; Bindings modes(host); modes.game.trackFrame=true;
	SECTION("display replacement") { modes.game.replaceDisplay=true; host.replacement=2; }
	SECTION("same mode address new generation") { modes.game.replaceDisplay=true; }
	SECTION("render replacement") { modes.game.replaceRender=true; host.replacement=2; }
	runtime::ApplicationLoop loop(modes.Services()); loop.RunIteration();
	REQUIRE(host.blocks.size()==1); CHECK(Count(host,"present:true")==0);
	CHECK(Count(host,"frame-destroy")==1);
	modes.game.replaceDisplay=false; modes.game.replaceRender=false; loop.RunIteration();
	CHECK(Count(host,"present:true")==1); // New binding is not latched as the failed old binding.
}
TEST_CASE("Exit and skipped output remain distinct from blocked work")
{
	Host host; Bindings modes(host);
	SECTION("session exit") { modes.game.exit=true; }
	SECTION("menu exit") { host.controller=2; modes.menu.exit=true; }
	SECTION("display skip") { modes.game.skip=true; }
	SECTION("input exit") { host.inputExit=true; }
	runtime::ApplicationLoop loop(modes.Services()); loop.RunIteration();
	CHECK(host.blocks.empty()); CHECK(Count(host,"unlock")==1);
	CHECK(host.exit==(modes.game.exit||modes.menu.exit||host.inputExit));
	CHECK(Count(host,"present:true")==int(host.inputExit));
}
TEST_CASE("Ordinary engine failures propagate and incomplete callbacks are reported")
{
	Host host; Bindings modes(host);
	SECTION("session exception") { host.failure="session:1"; }
	SECTION("input exception") { host.failure="input"; }
	SECTION("guard exception") { host.failure="lock"; }
	SECTION("display exception") { host.failure="display:1"; }
	SECTION("present exception") { host.failure="present:true"; }
	runtime::ApplicationLoop loop(modes.Services()); CHECK_THROWS_AS(loop.RunIteration(),std::runtime_error);
	CHECK_FALSE(host.held); CHECK(host.scopes.empty()); CHECK(Count(host,"flush")==0);
}
TEST_CASE("Reload replaces normal concerns after input and queued saves")
{
	Host host; Bindings modes(host); host.reload=true;
	runtime::ApplicationLoop loop(modes.Services()); loop.RunIteration();
	CHECK(host.calls==std::vector<std::string>{"watchdog","input","save","reload","flush"});
}
TEST_CASE("Unsupported optional concerns are scheduling failures")
{
	runtime::ModeFrame frame;
	Host host; Bindings modes(host);
	CHECK(modes.menu.runtime::Mode::UpdateSession(host.session).applicationStatus==runtime::ApplicationStatus::Blocked);
	CHECK(modes.connection.runtime::Mode::UpdateDisplay(frame)==runtime::ApplicationStatus::Blocked);
}

TEST_CASE("Blocked startup leaves host lifecycle available until mode replacement")
{
	Host host; Bindings modes(host); runtime::ApplicationLoop loop(modes.Services());
	loop.BlockCurrentMode({"TEST-STARTUP", "mode initialization did not complete"});
	loop.RunIteration();
	CHECK(host.calls == std::vector<std::string>{"blocked:TEST-STARTUP","watchdog","input","save","config","window","clock","flush"});
	CHECK(host.blocks == std::vector<std::string>{"TEST-STARTUP"});
	CHECK_FALSE(host.exit);
	host.Select(2);
	loop.RunIteration();
	CHECK(Count(host, "display:2") == 1);
	CHECK(Count(host, "present:true") == 1);
}
