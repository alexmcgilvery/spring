/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "ApplicationLoop.h"
#include "Globals/Lifecycle/ApplicationHost.h"
#include "Globals/Lifecycle/LifecycleContext.h"
#include "Modes/ModeBinding.h"
#include "Modes/ModeContextProvider.h"
#include "Modes/Game/GameMode.h"
#include "Modes/SelectMenu/SelectMenuMode.h"
#include "Modes/LuaMenu/LuaMenuMode.h"
#include "Modes/PreGame/PreGameMode.h"
#include "Modes/Loading/LoadingMode.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Stand-ins only provide valid lvalues for opaque borrowed types. No legacy engine
// implementation is linked; this executable tests orchestration, not gameplay.
union SDL_Event { int type; };
class CGame {}; class CGlobalSynced {}; class CGlobalUnsynced {};
class CCamera {}; class CMouseHandler {}; class CEventHandler {};
class CInfoConsole {}; class ISound {}; class IVideoCapturing {};
class CUnitHandler {}; class CFeatureHandler {}; class CProjectileHandler {};
class JobDispatcher {}; class SelectMenu {}; class ClientSetup {};
class ConfigHandler {}; class CLuaMenuController {}; class CLuaMenu {};
class CLuaIntro {}; class CPreGame {}; class CLoadScreen {}; class CGlobalRendering {};
class CglFont {};
namespace agui { class Gui {}; }
namespace netcode { class CNetProtocol {}; }
namespace runtime { struct GameFrame { std::uint64_t generation = 0; }; }
using namespace runtime;
namespace {
struct State {
	std::vector<std::string> trace;
	ActiveModeBinding active;
	bool held = false;
	bool reload = false;
	bool noGuard = false;
	int iterations = 1;
	std::function<void()> duringLock;
	void Select(IMode* mode) { active = {mode, active.generation + 1}; }
	void Add(const std::string& text) { trace.push_back(text); }
};
struct Provider final : ModeContextProvider {
	State& state;
	CGame game; CGlobalSynced synced; CGlobalUnsynced client;
	CCamera camera; CMouseHandler mouse; CEventHandler events; CInfoConsole console;
	ISound sound; IVideoCapturing capture; CUnitHandler units; CFeatureHandler features;
	CProjectileHandler projectiles; JobDispatcher jobs; SelectMenu select; ClientSetup setup;
	ConfigHandler config; CLuaMenuController luaController; CLuaMenu lua; CPreGame pregame;
	CLoadScreen loading; CGlobalRendering rendering; CglFont font; agui::Gui gui;
	netcode::CNetProtocol network; LifecycleRequests requests;
	GraphicsAccess graphics{rendering}; GameWorldView world{synced, units, features, projectiles};
	GameFrame frame;
	explicit Provider(State& state): state(state) {}
	void Check(const InvocationContext& invocation) {
		assert(state.active.mode != nullptr);
		assert(invocation.mode.kind == state.active.mode->kind);
		assert(invocation.mode.generation == state.active.generation);
	}
	void BeginIteration(std::uint64_t) override { state.Add("contexts.begin"); }
	void EndIteration() noexcept override { state.Add("contexts.end"); frame.generation = 0; }
	ModeInputContext Input(const InvocationContext& i) override {
		Check(i);
		switch (i.mode.kind) {
			case ModeKind::SelectMenu: return SelectMenuInputContext{i, {}, select, setup, config, gui, requests};
			case ModeKind::LuaMenu: return LuaMenuInputContext{i, {}, luaController, events, requests};
			case ModeKind::PreGame: return PreGameInputContext{i, {}, pregame, requests};
			case ModeKind::Loading: return LoadingInputContext{i, {}, loading, requests};
			default: return GameInputContext{i, {}, game, mouse, events, network, requests};
		}
	}
	ModeSessionContext Session(const InvocationContext& i) override {
		Check(i);
		if (i.mode.kind == ModeKind::PreGame) return PreGameSessionContext{i, pregame, setup, network, requests};
		if (i.mode.kind == ModeKind::Loading) return LoadingSessionContext{i, loading, game, LoadingInvocation::OrdinaryIteration, requests};
		return GameSessionContext{i, game, network, jobs, synced, client, capture, requests};
	}
	ModeDisplayContext Display(const InvocationContext& i) override {
		Check(i);
		if (i.mode.kind == ModeKind::LuaMenu) return LuaMenuDisplayContext{i, lua, events, mouse, console};
		if (i.mode.kind == ModeKind::Loading) return LoadingDisplayContext{i, loading, graphics, LoadingInvocation::OrdinaryIteration, nullptr, &lua};
		return GameDisplayContext{i, world, game, client, camera, mouse, events, console, sound, graphics, {}, frame};
	}
	ModeRenderContext Render(const InvocationContext& i) override {
		Check(i);
		switch (i.mode.kind) {
			case ModeKind::SelectMenu: return SelectMenuRenderContext{i, graphics, gui};
			case ModeKind::LuaMenu: return LuaMenuRenderContext{i, luaController, lua, mouse, graphics};
			case ModeKind::PreGame: return PreGameRenderContext{i, setup, network, font, graphics};
			case ModeKind::Loading: return LoadingRenderContext{i, graphics, LoadingInvocation::OrdinaryIteration, nullptr, &lua};
			default:
				assert(frame.generation == i.mode.generation);
				return GameRenderContext{i, world, frame, events, capture, graphics};
		}
	}
};
struct FakeMode final : IMode {
	State& state;
	std::function<void(const std::string&)> action;
	FakeMode(State& state, ModeKind kind, bool session, DisplayPhase display): IMode(kind, session, display), state(state) {}
	void Call(const std::string& phase) {
		state.Add(phase + ":" + std::to_string(static_cast<int>(kind)));
		if (action) action(phase);
	}
	void Input(const ModeInputContext&) override { assert(!state.held); Call("input"); }
	void Session(const ModeSessionContext&) override { assert(!state.held); assert(handlesSession); Call("session"); }
	void Display(const ModeDisplayContext& context) override {
		assert(state.held == (displayPhase == DisplayPhase::WithGraphics));
		if (kind == ModeKind::Game) {
			const auto& game = std::get<GameDisplayContext>(context);
			game.output.generation = game.invocation.mode.generation;
		}
		Call("display");
	}
	void Render(const ModeRenderContext&) override { assert(state.held); Call("render"); }
};
struct Guard final : GraphicsScope {
	State& state;
	explicit Guard(State& state): state(state) { assert(!state.held); state.held = true; state.Add("lock"); }
	~Guard() override { state.held = false; state.Add("unlock"); }
};
struct Host final : ApplicationHost {
	State& state;
	explicit Host(State& state): state(state) {}
	void Initialize() override { state.Add("initialize"); }
	bool BeginIteration() override {
		if (state.iterations-- <= 0) return false;
		state.Add("begin"); return true;
	}
	void CollectInput() override { state.Add("collect"); }
	bool ReloadRequested() const override { return state.reload; }
	void Reload() override { state.Add("reload"); state.reload = false; }
	InvocationContext CaptureInvocation(ModeIdentity mode, std::uint64_t iteration) override {
		return {mode, iteration, std::chrono::nanoseconds(iteration), std::chrono::nanoseconds(1)};
	}
	std::unique_ptr<GraphicsScope> AcquireGraphics() override {
		if (state.noGuard) return {};
		auto scope = std::make_unique<Guard>(state);
		if (state.duringLock) state.duringLock();
		return scope;
	}
	void Present(PresentationOrigin origin) override { assert(state.held); assert(origin == PresentationOrigin::OrdinaryIteration); state.Add("present"); }
	void FlushDiagnostics() override { assert(!state.held); state.Add("flush"); }
	void Shutdown() override { state.Add("shutdown"); }
};
struct Fixture {
	State state; Provider contexts{state}; Host host{state}; ApplicationLoop loop;
	FakeMode game{state, ModeKind::Game, true, DisplayPhase::WithGraphics};
	FakeMode loading{state, ModeKind::Loading, true, DisplayPhase::WithGraphics};
	FakeMode pregame{state, ModeKind::PreGame, true, DisplayPhase::Absent};
	FakeMode lua{state, ModeKind::LuaMenu, false, DisplayPhase::BeforeGraphics};
	FakeMode menu{state, ModeKind::SelectMenu, false, DisplayPhase::Absent};
	Fixture() { state.Select(&game); }
	void Run() { loop.Run({host, contexts, state.active}); }
	int Count(const std::string& text) const { return std::count(state.trace.begin(), state.trace.end(), text); }
};
}
int main()
{
	{
		SelectMenuMode menu; LuaMenuMode lua; PreGameMode pregame; LoadingMode loading; GameMode game;
		assert(!menu.handlesSession && menu.displayPhase == DisplayPhase::Absent);
		assert(!lua.handlesSession && lua.displayPhase == DisplayPhase::BeforeGraphics);
		assert(pregame.handlesSession && pregame.displayPhase == DisplayPhase::Absent);
		assert(loading.handlesSession && loading.displayPhase == DisplayPhase::WithGraphics);
		assert(game.handlesSession && game.displayPhase == DisplayPhase::WithGraphics);
	}
	{
		Fixture f; f.Run();
		assert((f.state.trace == std::vector<std::string>{"initialize","begin","collect","contexts.begin","input:5","session:5","lock","display:5","render:5","present","contexts.end","unlock","flush","shutdown"}));
	}
	for (int mode = 1; mode <= 4; ++mode) {
		Fixture f; IMode* selected[] = {&f.menu, &f.lua, &f.pregame, &f.loading};
		f.state.Select(selected[mode-1]); f.Run();
		assert(f.Count("session:" + std::to_string(mode)) == int(selected[mode-1]->handlesSession));
		assert(f.Count("display:" + std::to_string(mode)) == int(selected[mode-1]->displayPhase != DisplayPhase::Absent));
		assert(f.Count("present") == 1);
	}
	{
		Fixture f; f.state.Select(nullptr); f.Run(); assert(f.Count("present") == 0); assert(f.Count("lock") == 0);
	}
	{
		Fixture f; f.game.action = [&](auto phase) { if (phase == "input") f.state.Select(&f.lua); };
		f.Run(); assert(f.Count("input:2") == 0); assert(f.Count("session:5") == 0); assert(f.Count("render:2") == 1);
	}
	{
		Fixture f; f.game.action = [&](auto phase) { if (phase == "session") f.state.Select(&f.pregame); };
		f.Run(); assert(f.Count("session:3") == 0); assert(f.Count("render:3") == 1);
	}
	{
		Fixture f; f.state.duringLock = [&] { f.state.Select(&f.loading); };
		f.Run(); assert(f.Count("session:4") == 0); assert(f.Count("display:4") == 1); assert(f.Count("render:4") == 1);
	}
	{
		Fixture f; f.state.Select(&f.lua); f.state.duringLock = [&] { f.state.Select(&f.game); };
		f.Run(); assert(f.Count("display:2") == 1); assert(f.Count("display:5") == 1); assert(f.Count("render:5") == 1);
	}
	{
		Fixture f; f.state.iterations = 2; bool changed = false;
		f.state.duringLock = [&] { if (!changed) { changed = true; f.state.Select(&f.lua); } };
		f.Run(); assert(f.Count("present") == 1); assert(f.Count("display:2") == 1); assert(f.Count("render:5") == 0);
	}
	for (const std::string phase : {"display", "render"}) {
		Fixture f; f.game.action = [&](auto current) { if (current == phase) f.state.Select(&f.game); };
		f.Run(); assert(f.Count("present") == 0); assert(!f.state.held);
	}
	{
		Fixture f; f.game.action = [](auto phase) { if (phase == "display") throw std::runtime_error("callback"); };
		bool caught = false; try { f.Run(); } catch (const std::runtime_error&) { caught = true; }
		assert(caught); assert(!f.state.held); assert(f.Count("contexts.end") == 1); assert(f.Count("present") == 0);
		assert(f.state.trace.back() == "unlock"); // Emergency shutdown belongs to the outer owner.
	}
	{
		Fixture f; f.state.noGuard = true;
		bool caught = false; try { f.Run(); } catch (const std::logic_error&) { caught = true; }
		assert(caught); assert(f.Count("contexts.end") == 1); assert(f.Count("present") == 0);
	}
	{
		Fixture f; f.state.reload = true; f.Run();
		assert(f.Count("reload") == 1); assert(f.Count("contexts.begin") == 0); assert(f.Count("lock") == 0);
	}
	{
		Fixture f; f.state.iterations = 2; f.game.action = [&](auto phase) { if (phase == "input") f.state.reload = true; };
		f.Run(); assert(f.Count("reload") == 1); assert(f.Count("contexts.end") == 1); assert(f.Count("session:5") == 0);
	}
	{
		Fixture f; f.state.Select(&f.menu);
		const InvocationContext invocation{{ModeKind::SelectMenu, f.state.active.generation}, 1, {}, {}};
		GameMode realGame;
		bool caught = false;
		try { realGame.Input(f.contexts.Input(invocation)); } catch (const std::bad_variant_access&) { caught = true; }
		assert(caught);
		SelectMenuMode realMenu;
		const InvocationContext gameInvocation{{ModeKind::Game, f.state.active.generation}, 1, {}, {}};
		f.state.active.mode = &f.game;
		caught = false;
		try { realMenu.Session(f.contexts.Session(gameInvocation)); } catch (const std::logic_error&) { caught = true; }
		assert(caught);
	}
	std::cout << "PASS: interface, ordering, capabilities, transitions, contexts, graphics and exception scenarios\n";
}
