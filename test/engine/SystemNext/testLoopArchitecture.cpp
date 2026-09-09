/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TestSupport.h"

#include "Application/Diagnostics/Diagnostics.h"
#include "Application/Graphics/Graphics.h"
#include "Application/Lifecycle/ApplicationLifecycle.h"
#include "Application/Platform/Platform.h"
#include "ApplicationLoop.h"
#include "Modes/Game/GameMode.h"
#include "Modes/Loading/LoadingMode.h"
#include "Modes/LuaMenu/LuaMenuMode.h"
#include "Modes/PreGame/PreGameMode.h"
#include "Modes/SelectMenu/SelectMenuMode.h"

#include <algorithm>
#include <functional>

struct FakeMode : Mode<FakeMode, TestContracts> {
	FakeMode(ModeKind kind, std::string label, std::vector<std::string>& trace)
		: Mode(kind)
		, label(std::move(label))
		, trace(trace)
	{
	}

	InputPublication Input(const InputSnapshots& inputs)
	{
		trace.push_back(label + ".input");
		Check(inputs.PlatformInput().Current().events.size() == 1, "one immutable event batch");
		Check(inputs.Window().Current().windowId == 7, "input receives associated window facts");
		if (onInput)
			onInput();
		return Value {value, {}};
	}

	SessionPublication Session(const SessionSnapshots& inputs)
	{
		trace.push_back(label + ".session");
		Check(inputs.Input().Current().value == value, "session uses activation input");
		if (onSession)
			onSession();
		return {Value {value, {}}, request};
	}

	DisplayPublication Display(const DisplaySnapshots& inputs)
	{
		trace.push_back(label + ".display");
		Check(inputs.Session().Current().value == value, "display uses selected logical state");
		Check(inputs.GraphicsOutput().Current().generation == 1, "display receives frozen output facts");
		if (onDisplay)
			onDisplay();
		if (skipDisplay)
			return {};
		return Value {value, {}};
	}

	RenderPublication Render(const RenderSnapshots& inputs)
	{
		trace.push_back(label + ".render");
		Check(inputs.Display().Current().value == value, "render uses prepared display state");
		Check(inputs.GraphicsOutput().Current().generation == 1, "render uses selected output generation");
		if (onRender)
			onRender();
		if (skipRender)
			return {};
		return std::make_unique<TestCommands>(value);
	}

	std::string label;
	std::vector<std::string>& trace;
	int value = 7;
	bool skipDisplay = false;
	bool skipRender = false;
	std::optional<LifecycleRequest> request;
	std::function<void()> onInput;
	std::function<void()> onSession;
	std::function<void()> onDisplay;
	std::function<void()> onRender;
};

struct FakePlatform : Platform {
	explicit FakePlatform(std::vector<std::string>& trace)
		: trace(trace)
	{
	}

	bool BeginIteration() override
	{
		if (iterations-- <= 0)
			return false;
		trace.push_back("begin");
		return true;
	}

	PlatformPublications CollectPublications() override
	{
		trace.push_back("collect");
		return {TestInput(++eventSequence), TestWindow(windowGeneration)};
	}

	bool ExitRequested() const override { return exit; }
	bool ReloadRequested() const override { return reload; }

	std::vector<std::string>& trace;
	int iterations = 1;
	std::uint64_t eventSequence = 0;
	std::uint64_t windowGeneration = 1;
	bool exit = false;
	bool reload = false;
};

struct FakeLifecycle : ApplicationLifecycle {
	explicit FakeLifecycle(std::vector<std::string>& trace)
		: trace(trace)
	{
	}

	void Initialize() override
	{
		trace.push_back("initialize");
		if (failInitialization)
			throw std::runtime_error("initialization");
	}

	std::shared_ptr<IMode> CreateInitialMode() override
	{
		return initial;
	}

	std::shared_ptr<IMode> CreateMode(const LifecycleRequest& request) override
	{
		trace.push_back("activate");
		if (clearReload)
			clearReload();
		return create ? create(request) : nullptr;
	}

	void Shutdown() override { trace.push_back("shutdown"); }

	std::vector<std::string>& trace;
	std::shared_ptr<IMode> initial;
	bool failInitialization = false;
	std::function<void()> clearReload;
	std::function<std::shared_ptr<IMode>(const LifecycleRequest&)> create;
};

struct FakeDiagnostics : Diagnostics {
	explicit FakeDiagnostics(std::vector<std::string>& trace)
		: trace(trace)
	{
	}

	void Report() override { trace.push_back("report"); }

	std::vector<std::string>& trace;
};

struct FakeGraphics : Graphics {
	explicit FakeGraphics(std::vector<std::string>& trace)
		: trace(trace)
	{
	}

	VisualPlan PlanVisuals(const ModeIdentity& mode) override
	{
		trace.push_back("plan");
		Check(mode.kind != ModeKind::Inactive, "visual planning names an activation");
		return {schedule, {std::chrono::nanoseconds(81), std::chrono::nanoseconds(16)}, TestOutput(liveGeneration)};
	}

	std::optional<RenderedOutput> Render(
		const GraphicsOutputSnapshot& target,
		std::unique_ptr<const RenderCommands> commands
	) override
	{
		trace.push_back("execute");
		Check(dynamic_cast<const TestCommands*>(commands.get()) != nullptr, "owning commands reach graphics");
		Check(target.targetId == 9 && target.generation == 1, "graphics receives frozen target");
		if (onRender)
			onRender();
		if (skipOutput)
			return {};
		return RenderedOutput {++outputId, target.targetId, target.generation, std::make_shared<TestResource>()};
	}

	PresentationReceipt Present(const RenderedOutput& output) override
	{
		trace.push_back("present");
		Check(output.outputId == outputId && output.resource, "present consumes exact owning output");
		if (onPresent)
			onPresent();
		return {
			badReceipt ? 999u : output.outputId,
			output.targetId,
			output.targetGeneration,
			liveGeneration == output.targetGeneration ? PresentationOutcome::Presented : PresentationOutcome::Skipped,
			{},
			{},
		};
	}

	std::vector<std::string>& trace;
	VisualSchedule schedule = VisualSchedule::Present;
	std::uint64_t outputId = 0;
	bool skipOutput = false;
	bool badReceipt = false;
	std::uint64_t liveGeneration = 1;
	std::function<void()> onRender;
	std::function<void()> onPresent;
};

static void RunLoop(
	FakePlatform& platform,
	FakeLifecycle& lifecycle,
	SnapshotManager& snapshots,
	FakeDiagnostics& diagnostics,
	Graphics* graphics
)
{
	ApplicationLoop loop(platform, lifecycle, snapshots, diagnostics, graphics);
	loop.Run();
}

static void SchedulesAndHeadless()
{
	for (const auto schedule : {VisualSchedule::Skip, VisualSchedule::Display, VisualSchedule::Offscreen, VisualSchedule::Present}) {
		std::vector<std::string> trace;
		FakePlatform platform(trace);
		FakeLifecycle lifecycle(trace);
		FakeDiagnostics diagnostics(trace);
		FakeGraphics graphics(trace);
		SnapshotManager snapshots;
		lifecycle.initial = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		graphics.schedule = schedule;

		RunLoop(platform, lifecycle, snapshots, diagnostics, &graphics);

		std::vector<std::string> expected {"initialize", "begin", "collect", "game.input", "game.session", "plan"};
		if (schedule != VisualSchedule::Skip)
			expected.push_back("game.display");
		if (schedule == VisualSchedule::Offscreen || schedule == VisualSchedule::Present)
			expected.insert(expected.end(), {"game.render", "execute"});
		if (schedule == VisualSchedule::Present)
			expected.push_back("present");
		expected.insert(expected.end(), {"report", "shutdown"});
		Check(trace == expected, "exact cumulative concern order");
	}

	std::vector<std::string> trace;
	FakePlatform platform(trace);
	FakeLifecycle lifecycle(trace);
	FakeDiagnostics diagnostics(trace);
	SnapshotManager snapshots;
	lifecycle.initial = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
	RunLoop(platform, lifecycle, snapshots, diagnostics, nullptr);
	Check(trace == std::vector<std::string> {
		"initialize", "begin", "collect", "game.input", "game.session", "report", "shutdown"
	}, "headless constructs no graphics path");
}

static void TransitionsAndApplicationRequests()
{
	std::vector<std::string> trace;
	FakePlatform platform(trace);
	platform.iterations = 2;
	FakeLifecycle lifecycle(trace);
	FakeDiagnostics diagnostics(trace);
	FakeGraphics graphics(trace);
	SnapshotManager snapshots;
	auto menu = std::make_shared<FakeMode>(ModeKind::SelectMenu, "menu", trace);
	auto loading = std::make_shared<FakeMode>(ModeKind::Loading, "loading", trace);
	menu->request = LifecycleRequest {
		LifecycleAction::SwitchMode,
		ModeKind::Loading,
		Handoff::Own(std::string("selected content")),
	};
	lifecycle.initial = menu;
	lifecycle.create = [&](const LifecycleRequest& request) {
		Check(*request.handoff.Get<std::string>() == "selected content", "lifecycle receives owning handoff");
		return loading;
	};

	RunLoop(platform, lifecycle, snapshots, diagnostics, &graphics);
	Check(trace == std::vector<std::string> {
		"initialize", "begin", "collect", "menu.input", "menu.session", "activate", "report",
		"begin", "collect", "loading.input", "loading.session", "plan", "loading.display",
		"loading.render", "execute", "present", "report", "shutdown"
	}, "switch closes old logic and replacement starts with fresh input");

	trace.clear();
	FakePlatform reloadPlatform(trace);
	reloadPlatform.iterations = 2;
	reloadPlatform.reload = true;
	FakeLifecycle reloadLifecycle(trace);
	FakeDiagnostics reloadDiagnostics(trace);
	SnapshotManager reloadSnapshots;
	auto replacement = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
	reloadLifecycle.clearReload = [&] { reloadPlatform.reload = false; };
	reloadLifecycle.create = [&](const LifecycleRequest& request) {
		Check(request.action == LifecycleAction::Reload, "reload is application lifecycle work");
		return replacement;
	};
	RunLoop(reloadPlatform, reloadLifecycle, reloadSnapshots, reloadDiagnostics, nullptr);
	Check(std::count(trace.begin(), trace.end(), "game.input") == 1, "reload replacement gets a fresh iteration");
}

static void TargetIdentityAndFailures()
{
	{
		std::vector<std::string> trace;
		FakePlatform platform(trace);
		FakeLifecycle lifecycle(trace);
		FakeDiagnostics diagnostics(trace);
		FakeGraphics graphics(trace);
		SnapshotManager snapshots;
		auto mode = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		lifecycle.initial = mode;
		mode->onDisplay = [&] { graphics.liveGeneration = 2; };
		RunLoop(platform, lifecycle, snapshots, diagnostics, &graphics);
		Check(trace == std::vector<std::string> {
			"initialize", "begin", "collect", "game.input", "game.session", "plan",
			"game.display", "game.render", "execute", "present", "report", "shutdown"
		}, "retired target records an explicit skip without retargeting");
	}

	for (const auto stage : {"input", "session", "display", "render", "execute", "present", "initialize", "activation"}) {
		std::vector<std::string> trace;
		FakePlatform platform(trace);
		FakeLifecycle lifecycle(trace);
		FakeDiagnostics diagnostics(trace);
		FakeGraphics graphics(trace);
		SnapshotManager snapshots;
		auto mode = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		lifecycle.initial = mode;
		const auto fail = [] { throw std::runtime_error("injected failure"); };
		const std::string selected = stage;
		if (selected == "input") mode->onInput = fail;
		if (selected == "session") mode->onSession = fail;
		if (selected == "display") mode->onDisplay = fail;
		if (selected == "render") mode->onRender = fail;
		if (selected == "execute") graphics.onRender = fail;
		if (selected == "present") graphics.onPresent = fail;
		if (selected == "initialize") lifecycle.failInitialization = true;
		if (selected == "activation") {
			mode->request = LifecycleRequest {LifecycleAction::SwitchMode, ModeKind::Loading, {}};
			lifecycle.create = [](const LifecycleRequest&) -> std::shared_ptr<IMode> { return {}; };
		}

		ApplicationLoop loop(platform, lifecycle, snapshots, diagnostics, &graphics);
		Throws([&] { loop.Run(); }, "failure propagates through application boundary");
		Check(trace.back() == "shutdown", "failure closes lifecycle");
		Check(snapshots.Active().kind == ModeKind::Inactive, "failure retires activation");
	}

	std::vector<std::string> trace;
	FakePlatform platform(trace);
	platform.exit = true;
	FakeLifecycle lifecycle(trace);
	FakeDiagnostics diagnostics(trace);
	SnapshotManager snapshots;
	RunLoop(platform, lifecycle, snapshots, diagnostics, nullptr);
	Check(trace == std::vector<std::string> {"initialize", "begin", "report", "shutdown"},
		"platform exit is monotonic and requires no active mode");
}

static void ActualModeOutlines()
{
	std::vector<std::shared_ptr<IMode>> modes {
		std::make_shared<SelectMenuMode>(),
		std::make_shared<LuaMenuMode>(),
		std::make_shared<PreGameMode>(),
		std::make_shared<LoadingMode>(),
		std::make_shared<GameMode>(),
	};

	for (const auto& mode : modes) {
		SnapshotManager snapshots;
		mode->RegisterSnapshots(snapshots);
		snapshots.Activate(mode->kind);
		auto iteration = snapshots.BeginLogical(TestInput(), TestWindow());
		const auto id = iteration.LogicalId();
		mode->ExecuteInput(snapshots, id);
		mode->ExecuteSession(snapshots, id);
		Check(snapshots.StageStatusOf(id, Stage::Input) == StageStatus::NoPublication,
			"actual Input remains an explicit outline");
		Check(snapshots.StageStatusOf(id, Stage::Session) == StageStatus::Unavailable,
			"outline never fabricates prerequisite state");
	}
}

int main()
{
	SchedulesAndHeadless();
	TransitionsAndApplicationRequests();
	TargetIdentityAndFailures();
	ActualModeOutlines();
	std::cout << "PASS explicit ownership, concern order, transitions, headless and target identity\n";
}
