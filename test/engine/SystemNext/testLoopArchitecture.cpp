/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TestSupport.h"
#include "ApplicationLoop.h"
#include "Globals/Lifecycle/ApplicationHost.h"
#include "Modes/ModeBinding.h"
#include "Modes/SelectMenu/SelectMenuMode.h"
#include "Modes/LuaMenu/LuaMenuMode.h"
#include "Modes/PreGame/PreGameMode.h"
#include "Modes/Loading/LoadingMode.h"
#include "Modes/Game/GameMode.h"

#include <functional>

struct FakeMode : Mode<FakeMode, TestContracts> {
	FakeMode(ModeKind kind, std::string label, std::vector<std::string>& trace)
		: Mode(kind), label(std::move(label)), trace(trace)
	{
	}

	InputPublication Input(const InputSnapshots& inputs)
	{
		trace.push_back(label + ".input");
		Check(inputs.Application().Current().events.size() == 1, "collected exactly one input batch");
		if (onInput)
			onInput();
		return Value {value, {}};
	}

	SessionPublication Session(const SessionSnapshots& inputs)
	{
		trace.push_back(label + ".session");
		Check(inputs.Input().Current().value == value, "session uses own activation input");
		if (onSession)
			onSession();
		return {Value {value, {}}, request};
	}

	DisplayPublication Display(const DisplaySnapshots& inputs)
	{
		trace.push_back(label + ".display");
		Check(inputs.Session().Current().value == value, "display uses selected logical state");
		if (onDisplay)
			onDisplay();
		if (skipDisplay)
			return {};
		return Value {value, {}};
	}

	RenderPublication Render(const RenderSnapshots& inputs)
	{
		trace.push_back(label + ".render");
		Check(inputs.Display().Current().value == value, "render uses prepared frame");
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

struct FakeHost : ApplicationHost {
	explicit FakeHost(std::vector<std::string>& trace) : trace(trace) {}

	void Initialize() override
	{
		trace.push_back("initialize");
		if (failInitialization)
			throw std::runtime_error("initialization");
	}

	bool BeginIteration() override
	{
		if (iterations-- <= 0)
			return false;
		trace.push_back("begin");
		return true;
	}

	ApplicationSnapshot CollectInput() override
	{
		trace.push_back("collect");
		ApplicationSnapshot snapshot;
		snapshot.events.push_back({++eventSequence, "test", {}, {}});
		return snapshot;
	}

	bool ExitRequested() const override { return exit; }
	bool ReloadRequested() const override { return reload; }
	IterationTiming CaptureVisualTiming() override
	{
		++visualSamples;
		return {std::chrono::nanoseconds(81), std::chrono::nanoseconds(16)};
	}

	std::shared_ptr<IMode> CreateMode(const LifecycleRequest& request) override
	{
		trace.push_back("activate");
		reload = false;
		return create ? create(request) : nullptr;
	}

	void FlushDiagnostics() override { trace.push_back("report"); }
	void Shutdown() override { trace.push_back("shutdown"); }

	std::vector<std::string>& trace;
	int iterations = 1;
	std::uint64_t eventSequence = 0;
	unsigned visualSamples = 0;
	bool exit = false;
	bool reload = false;
	bool failInitialization = false;
	std::function<std::shared_ptr<IMode>(const LifecycleRequest&)> create;
};

struct FakeOutput : VisualOutput {
	explicit FakeOutput(std::vector<std::string>& trace) : trace(trace) {}

	VisualSchedule PlanIteration(const InvocationContext& invocation) override
	{
		trace.push_back("plan");
		Check(invocation.flow == Flow::Visual, "visual identity is explicit");
		Check(invocation.sampledAt == std::chrono::nanoseconds(81), "visual timing has its own sampled facts");
		return schedule;
	}

	std::optional<RenderedOutput> Render(std::unique_ptr<const RenderCommands> commands) override
	{
		trace.push_back("execute");
		Check(dynamic_cast<const TestCommands*>(commands.get()) != nullptr, "owning mode commands reach backend");
		if (onRender)
			onRender();
		if (skipOutput)
			return {};
		return RenderedOutput {++outputId, 3, std::make_shared<TestResource>()};
	}

	PresentationReceipt Present(const RenderedOutput& output) override
	{
		trace.push_back("present");
		Check(output.outputId == outputId && output.targetId == 3 && output.resource, "present consumes exact output");
		if (onPresent)
			onPresent();
		return {badReceipt ? 999 : output.outputId, output.targetId, PresentationOutcome::Presented, {}, {}};
	}

	std::vector<std::string>& trace;
	VisualSchedule schedule = VisualSchedule::Present;
	std::uint64_t outputId = 0;
	bool skipOutput = false;
	bool badReceipt = false;
	std::function<void()> onRender;
	std::function<void()> onPresent;
};

static void Schedules()
{
	for (const auto schedule : {VisualSchedule::None, VisualSchedule::Display, VisualSchedule::Offscreen, VisualSchedule::Present}) {
		std::vector<std::string> trace;
		FakeHost host(trace);
		FakeOutput output(trace);
		output.schedule = schedule;
		SnapshotManager manager;
		auto mode = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		ActiveModeBinding binding {mode, {}};
		ApplicationLoop loop({host, manager, binding, &output});
		loop.Run();

		std::vector<std::string> expected {"initialize", "begin", "collect", "game.input", "game.session", "plan"};
		if (schedule != VisualSchedule::None)
			expected.push_back("game.display");
		if (schedule == VisualSchedule::Offscreen || schedule == VisualSchedule::Present) {
			expected.push_back("game.render");
			expected.push_back("execute");
		}
		if (schedule == VisualSchedule::Present)
			expected.push_back("present");
		expected.insert(expected.end(), {"report", "shutdown"});
		Check(trace == expected, "exact cumulative stage ordering");
	}

	// No visual subsystem is constructed for headless execution.
	std::vector<std::string> trace;
	FakeHost host(trace);
	SnapshotManager manager;
	ActiveModeBinding binding {std::make_shared<FakeMode>(ModeKind::Game, "game", trace), {}};
	ApplicationLoop loop({host, manager, binding, nullptr});
	loop.Run();
	Check(trace == std::vector<std::string> {"initialize", "begin", "collect", "game.input", "game.session", "report", "shutdown"}, "headless logical flow without visual calls");
	Check(host.visualSamples == 0, "headless does not acquire visual invocation facts");
}

struct DirectContracts : ModeContracts {
	using InputData = Value;
	using SessionData = Value;
	using InputReads = SnapshotReads<Required<Stage::Application, Slot::Current>>;
	using SessionReads = SnapshotReads<Required<Stage::Input, Slot::Current>>;
	using RenderReads = SnapshotReads<Required<Stage::Session, Slot::Current>>;
};

struct DirectMode : Mode<DirectMode, DirectContracts> {
	DirectMode() : Mode(ModeKind::SelectMenu) {}
	InputPublication Input(const InputSnapshots&) { return Value {5, {}}; }
	SessionPublication Session(const SessionSnapshots&) { return {Value {5, {}}, {}}; }
	RenderPublication Render(const RenderSnapshots& input) { return std::make_unique<TestCommands>(input.Session().Current().value); }
};

struct EmptyMode : Mode<EmptyMode, ModeContracts> {
	EmptyMode() : Mode(ModeKind::SelectMenu) {}
};

static void AbsentAndSkipped()
{
	{
		std::vector<std::string> trace;
		FakeHost host(trace);
		FakeOutput output(trace);
		SnapshotManager manager;
		ActiveModeBinding binding {std::make_shared<DirectMode>(), {}};
		ApplicationLoop loop({host, manager, binding, &output});
		loop.Run();
		Check(trace == std::vector<std::string> {"initialize", "begin", "collect", "plan", "execute", "present", "report", "shutdown"}, "absent Display requires no dummy output");
	}
	{
		std::vector<std::string> trace;
		FakeHost host(trace);
		FakeOutput output(trace);
		SnapshotManager manager;
		ActiveModeBinding binding {std::make_shared<EmptyMode>(), {}};
		ApplicationLoop loop({host, manager, binding, &output});
		loop.Run();
		Check(output.outputId == 0, "all omitted concerns produce no visual output");
	}
	for (int skipped = 0; skipped < 3; ++skipped) {
		std::vector<std::string> trace;
		FakeHost host(trace);
		FakeOutput output(trace);
		SnapshotManager manager;
		auto mode = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		mode->skipDisplay = skipped == 0;
		mode->skipRender = skipped == 1;
		output.skipOutput = skipped == 2;
		ActiveModeBinding binding {mode, {}};
		ApplicationLoop loop({host, manager, binding, &output});
		loop.Run();
		Check(std::find(trace.begin(), trace.end(), "present") == trace.end(), "no partial/unprepared output is presented");
	}
}

static void TransitionAndHandoff()
{
	std::vector<std::string> trace;
	FakeHost host(trace);
	host.iterations = 2;
	FakeOutput output(trace);
	SnapshotManager manager;
	auto menu = std::make_shared<FakeMode>(ModeKind::SelectMenu, "menu", trace);
	auto loading = std::make_shared<FakeMode>(ModeKind::Loading, "loading", trace);
	menu->request = LifecycleRequest {LifecycleAction::SwitchMode, ModeKind::Loading, Handoff::Own(std::string("selected content"))};
	host.create = [&](const LifecycleRequest& request) {
		Check(*request.handoff.Get<std::string>() == "selected content", "factory receives owning handoff");
		return loading;
	};
	ActiveModeBinding binding {menu, {}};
	ApplicationLoop loop({host, manager, binding, &output});
	loop.Run();
	Check(trace == std::vector<std::string> {
		"initialize", "begin", "collect", "menu.input", "menu.session", "activate", "report",
		"begin", "collect", "loading.input", "loading.session", "plan", "loading.display",
		"loading.render", "execute", "present", "report", "shutdown"
	}, "switch follows Session; replacement starts fresh input before its own visual work");

	// Reactivating the same object still changes activation identity.
	trace.clear();
	FakeHost reuseHost(trace);
	reuseHost.iterations = 2;
	SnapshotManager reuseManager;
	auto same = std::make_shared<FakeMode>(ModeKind::Game, "same", trace);
	same->request = LifecycleRequest {LifecycleAction::SwitchMode, ModeKind::Game, {}};
	std::vector<std::uint64_t> generations;
	same->onInput = [&] { generations.push_back(reuseManager.Active().generation); };
	reuseHost.create = [&](const LifecycleRequest&) {
		same->request.reset();
		return same;
	};
	ActiveModeBinding reuseBinding {same, {}};
	ApplicationLoop reuse({reuseHost, reuseManager, reuseBinding, nullptr});
	reuse.Run();
	Check(generations.size() == 2 && generations[0] != generations[1], "address reuse cannot masquerade as continuity");
}

static void RetirementAndFailures()
{
	for (const auto stage : {"input", "session", "display", "render", "execute", "present", "initialize", "activation"}) {
		std::vector<std::string> trace;
		FakeHost host(trace);
		FakeOutput output(trace);
		SnapshotManager manager;
		auto mode = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		const auto fail = [] { throw std::runtime_error("injected stage failure"); };
		const std::string selected = stage;
		if (selected == "input") mode->onInput = fail;
		if (selected == "session") mode->onSession = fail;
		if (selected == "display") mode->onDisplay = fail;
		if (selected == "render") mode->onRender = fail;
		if (selected == "execute") output.onRender = fail;
		if (selected == "present") output.onPresent = fail;
		if (selected == "initialize") host.failInitialization = true;
		if (selected == "activation") mode->request = LifecycleRequest {LifecycleAction::SwitchMode, ModeKind::Loading, {}};
		ActiveModeBinding binding {mode, {}};
		ApplicationLoop loop({host, manager, binding, &output});
		Throws([&] { loop.Run(); }, "stage/factory failure propagates");
		Check(trace.back() == "shutdown", "exception cleanup reaches lifecycle");
		Check(manager.Active().kind == ModeKind::Inactive && !binding.mode, "exception retires activation");
	}
	for (bool duringDisplay : {false, true}) {
		std::vector<std::string> trace;
		FakeHost host(trace);
		FakeOutput output(trace);
		SnapshotManager manager;
		auto mode = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		const auto retire = [&] { manager.Activate(ModeKind::Game); };
		if (duringDisplay) mode->onDisplay = retire;
		else output.onRender = retire;
		ActiveModeBinding binding {mode, {}};
		ApplicationLoop loop({host, manager, binding, &output});
		loop.Run();
		Check(std::find(trace.begin(), trace.end(), "present") == trace.end(), "retired work cannot begin presentation");
		if (duringDisplay)
			Check(output.outputId == 0, "retired display does not begin rendering");
	}
}

static void LifecycleAndActualOutlines()
{
	{
		std::vector<std::string> trace;
		FakeHost host(trace);
		host.exit = true;
		SnapshotManager manager;
		ActiveModeBinding binding;
		ApplicationLoop loop({host, manager, binding, nullptr});
		loop.Run();
		Check(trace == std::vector<std::string> {"initialize", "begin", "collect", "report", "shutdown"}, "OS exit works without a mode");
	}
	{
		std::vector<std::string> trace;
		FakeHost host(trace);
		host.iterations = 3;
		SnapshotManager manager;
		auto mode = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		mode->request = LifecycleRequest {LifecycleAction::Exit, ModeKind::Inactive, {}};
		ActiveModeBinding binding {mode, {}};
		ApplicationLoop loop({host, manager, binding, nullptr});
		loop.Run();
		Check(std::count(trace.begin(), trace.end(), "begin") == 1, "Session exit is monotonic");
	}
	{
		std::vector<std::string> trace;
		FakeHost host(trace);
		host.iterations = 2;
		host.reload = true;
		SnapshotManager manager;
		auto replacement = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		host.create = [&](const LifecycleRequest& request) {
			Check(request.action == LifecycleAction::Reload, "reload remains lifecycle-owned");
			return replacement;
		};
		ActiveModeBinding binding;
		ApplicationLoop loop({host, manager, binding, nullptr});
		loop.Run();
		Check(std::count(trace.begin(), trace.end(), "game.input") == 1, "reload replacement begins next logical iteration");
	}
	std::vector<std::shared_ptr<IMode>> modes {
		std::make_shared<SelectMenuMode>(), std::make_shared<LuaMenuMode>(),
		std::make_shared<PreGameMode>(), std::make_shared<LoadingMode>(), std::make_shared<GameMode>()
	};
	for (const auto& mode : modes) {
		SnapshotManager manager;
		mode->RegisterSnapshots(manager);
		manager.Activate(mode->kind);
		auto iteration = manager.BeginLogical(LogicalIterationId {1}, {});
		mode->ExecuteInput(manager, {1});
		mode->ExecuteSession(manager, {1});
		Check(manager.Status(LogicalIterationId {1}, Stage::Input) == StageStatus::NoPublication, "actual Input remains an explicit outline");
		Check(manager.Status(LogicalIterationId {1}, Stage::Session) == StageStatus::Unavailable, "outline never fakes prerequisite state");
	}
}

static void ReceiptValidationAndImmediateLifecycle()
{
	{
		std::vector<std::string> trace;
		FakeHost host(trace);
		FakeOutput output(trace);
		output.badReceipt = true;
		SnapshotManager manager;
		ActiveModeBinding binding {std::make_shared<FakeMode>(ModeKind::Game, "game", trace), {}};
		ApplicationLoop loop({host, manager, binding, &output});
		Throws([&] { loop.Run(); }, "incorrect receipt cannot claim delivered output");
		Check(trace.back() == "shutdown", "receipt failure cleans up");
	}
	for (const auto concern : {"input", "session", "display"}) {
		std::vector<std::string> trace;
		FakeHost host(trace);
		FakeOutput output(trace);
		SnapshotManager manager;
		auto mode = std::make_shared<FakeMode>(ModeKind::Game, "game", trace);
		const auto exit = [&] { host.exit = true; };
		const std::string stage = concern;
		if (stage == "input") mode->onInput = exit;
		if (stage == "session") mode->onSession = exit;
		if (stage == "display") mode->onDisplay = exit;
		ActiveModeBinding binding {mode, {}};
		ApplicationLoop loop({host, manager, binding, &output});
		loop.Run();
		Check(output.outputId == 0, "OS exit prevents later visual work");
		if (stage == "input")
			Check(std::count(trace.begin(), trace.end(), "game.session") == 0, "OS exit can stop before Session");
	}
}

int main()
{
	Schedules();
	AbsentAndSkipped();
	TransitionAndHandoff();
	RetirementAndFailures();
	LifecycleAndActualOutlines();
	ReceiptValidationAndImmediateLifecycle();
	std::cout << "PASS connected loop, transitions, headless, output ownership and failure scopes\n";
}
