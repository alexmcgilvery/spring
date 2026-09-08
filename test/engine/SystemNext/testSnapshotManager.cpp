/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TestSupport.h"

static void AssociationAndMissingCurrent()
{
	SnapshotManager manager;
	manager.Register<TestContracts>(ModeKind::Game);
	manager.Activate(ModeKind::Game);
	PublishLogical(manager, 96);

	auto visual = manager.BeginVisual({}, TestOutput());
	Check(visual.has_value(), "visual association exists");
	const auto visualId = visual->VisualId();
	Check(manager.AssociatedLogical(visualId) == LogicalIterationId {1}, "visual and logical IDs are independent");

	auto display = manager.BeginStage(visualId, Stage::Display);
	auto before = manager.Acquire<TestContracts, Stage::Display>(*display);
	Check(before->Session().Current().value == 96, "selected logical value");

	PublishLogical(manager, 97);
	auto after = manager.Acquire<TestContracts, Stage::Display>(*display);
	Check(before->Session().Current().value == 96 && after->Session().Current().value == 96, "association and acquired view freeze");
	Check(!before->Display().Previous(), "bootstrap history absent");
	manager.Publish(*display, Value {81, {}});
	display->Finish(StageStatus::Completed);
	visual->Close();
	Check(before->Session().Current().value == 96, "closed iteration leaves view readable");
	Check(!manager.BeginStage(visualId, Stage::Render), "closed ID cannot admit");

	PublishLogical(manager, 98, false);
	auto missing = manager.BeginVisual({}, TestOutput());
	const auto missingId = missing->VisualId();
	auto missingDisplay = manager.BeginStage(missingId, Stage::Display);
	Check(!manager.Acquire<TestContracts, Stage::Display>(*missingDisplay), "absent current never promotes 97");
	missingDisplay->Finish(StageStatus::Unavailable);
	Check(!manager.BeginStage(missingId, Stage::Render), "unavailable display stops rendering");
	auto issued = manager.BeginLogical(TestInput(), TestWindow());
	Check(issued.LogicalId() == LogicalIterationId {4}, "manager allocates monotonic logical IDs");
}

static void StageOrderingAndExceptions()
{
	SnapshotManager manager;
	manager.Register<TestContracts>(ModeKind::Game);
	manager.Activate(ModeKind::Game);
	auto iteration = manager.BeginLogical(TestInput(), TestWindow());
	Check(!manager.BeginStage(LogicalIterationId {1}, Stage::Session), "session cannot precede input");
	Check(!manager.BeginSimulation({1}), "snapshot storage grants no independent tick authority");

	{
		auto input = manager.BeginStage(LogicalIterationId {1}, Stage::Input);
		auto view = manager.Acquire<TestContracts, Stage::Input>(*input);
		Check(view.has_value(), "input has application and activation data");
		Check(!view->Session().Previous(), "fresh activation has no session history");
		Check(manager.Publish(*input, Value {1, {}}), "publish input");
		Throws([&] { manager.Publish(*input, Value {2, {}}); }, "one publication per invocation");
		Throws([&] { manager.RequestLifecycle(*input, {}); }, "input cannot issue lifecycle request");
		input->Finish(StageStatus::Completed);
	}
	Check(!manager.BeginStage(LogicalIterationId {1}, Stage::Input), "input executes once");
	{
		auto session = manager.BeginStage(LogicalIterationId {1}, Stage::Session);
		Throws([&] { manager.RequestLifecycle(*session, {}); }, "transition needs owning state");
		// Simulate unwinding before completion.
	}
	Check(manager.Status(LogicalIterationId {1}, Stage::Session) == StageStatus::Failed, "unfinished scope failed");
	iteration.Close();
	Check(!manager.BeginVisual({}, TestOutput()), "failed logical iteration cannot supply visuals");
}

static void SimulationCadenceAndHistory()
{
	SnapshotManager manager;
	manager.Register<TestContracts>(ModeKind::Game);
	manager.Activate(ModeKind::Game);
	{
		auto iteration = manager.BeginLogical(TestInput(), TestWindow());
		auto input = manager.BeginStage(LogicalIterationId {1}, Stage::Input);
		manager.Publish(*input, Value {1, {}});
		input->Finish(StageStatus::Completed);
		auto session = manager.BeginStage(LogicalIterationId {1}, Stage::Session);
		for (int tick : {4095, 4096}) {
			auto frame = manager.BeginSimulation({1});
			Check(frame.has_value(), "session admits completed simulation work");
			manager.Publish(*frame, Value {tick, {}}, tick);
			frame->Finish(StageStatus::Completed);
		}
		auto revision = manager.BeginSimulation({1});
		manager.Publish(*revision, Value {5000, {}}, 4096);
		revision->Finish(StageStatus::Completed);
		manager.Publish(*session, Value {1, {}});
		session->Finish(StageStatus::Completed);
	}
	auto visual = manager.BeginVisual({}, TestOutput());
	auto display = manager.BeginStage(VisualIterationId {1}, Stage::Display);
	auto view = manager.Acquire<TestContracts, Stage::Display>(*display);
	Check(view->Simulation().Current()->value == 5000, "latest observation revision is independent of tick");
	Check(view->Simulation().Previous()->value == 4096, "multiple publications in one logical iteration stay distinct");
	Check(view->Simulation().Identity<0>()->tick == 4096, "completed tick preserved");
	Check(view->Simulation().Identity<0>()->revision != view->Simulation().Identity<1>()->revision, "revision identity differs");
	display->Finish(StageStatus::NoPublication);
	visual->Close();

	PublishLogical(manager, 2); // No tick.
	auto next = manager.BeginVisual({}, TestOutput());
	auto nextDisplay = manager.BeginStage(VisualIterationId {2}, Stage::Display);
	auto nextView = manager.Acquire<TestContracts, Stage::Display>(*nextDisplay);
	Check(nextView->Simulation().Current()->value == 5000, "no-tick visual association explicitly retains completed simulation");
	Check(nextView->Simulation().Identity<0>()->iteration == 1, "retained state is not relabeled");
	nextDisplay->Finish(StageStatus::NoPublication);
}

static void RetentionAndRetirement()
{
	std::optional<SnapshotView<TestContracts, Stage::Display>> retained;
	std::weak_ptr<const int> weak;
	{
		SnapshotManager manager;
		manager.Register<TestContracts>(ModeKind::Game);
		const auto original = manager.Activate(ModeKind::Game);
		{
			auto iteration = manager.BeginLogical(TestInput(), TestWindow());
			auto input = manager.BeginStage(LogicalIterationId {1}, Stage::Input);
			manager.Publish(*input, Value {1, {}});
			input->Finish(StageStatus::Completed);
			auto session = manager.BeginStage(LogicalIterationId {1}, Stage::Session);
			auto lifetime = std::make_shared<const int>(42);
			weak = lifetime;
			manager.Publish(*session, Value {42, lifetime});
			session->Finish(StageStatus::Completed);
		}
		auto visual = manager.BeginVisual({}, TestOutput());
		auto display = manager.BeginStage(VisualIterationId {1}, Stage::Display);
		retained = manager.Acquire<TestContracts, Stage::Display>(*display);
		manager.Publish(*display, Value {1, {}});
		display->Finish(StageStatus::Completed);
		auto render = manager.BeginStage(VisualIterationId {1}, Stage::Render);
		Check(render.has_value(), "render admitted before retirement");

		const auto next = manager.Activate(ModeKind::Game, Handoff::Own(std::string("owned setup")));
		Check(next.generation != original.generation, "same kind gets a distinct activation");
			Check(!manager.Publish(*render, RenderedOutput {1, 9, 1, std::make_shared<TestResource>()}), "late render cannot commit");
		render->Finish(StageStatus::NoPublication);
		Check(!manager.BeginStage(VisualIterationId {1}, Stage::Present), "old activation cannot present");
		Check(!manager.BeginVisual({}, TestOutput()), "new activation needs its own logical iteration");

		{
			auto logical = manager.BeginLogical(TestInput(), TestWindow());
				auto input = manager.BeginStage(LogicalIterationId {2}, Stage::Input);
				auto view = manager.Acquire<TestContracts, Stage::Input>(*input);
				Check(!view->Session().Previous(), "mode history does not cross activation");
				Check(view->PlatformInput().Previous() != nullptr, "platform continuity survives activation");
				Check(view->Window().Previous() != nullptr, "window continuity survives activation");
				Check(view->Window().Identity<1>()->mode.kind == ModeKind::Inactive,
					"application publications do not inherit mode identity");
				Check(*view->Activation().Current().handoff.Get<std::string>() == "owned setup", "typed owning handoff");
				Check(!view->Activation().Current().handoff.Get<int>(), "handoff rejects incorrect type");
				manager.Publish(*input, Value {2, {}});
				input->Finish(StageStatus::Completed);
				auto session = manager.BeginStage(LogicalIterationId {2}, Stage::Session);
				manager.Publish(*session, Value {2, {}});
				session->Finish(StageStatus::Completed);
			}
			{
				auto visual = manager.BeginVisual({}, TestOutput(2));
				auto display = manager.BeginStage(visual->VisualId(), Stage::Display);
				auto view = manager.Acquire<TestContracts, Stage::Display>(*display);
				Check(view->GraphicsOutput().Current().generation == 2, "new target generation is current");
				Check(view->GraphicsOutput().Previous()->generation == 1, "output history survives mode activation");
				Check(view->GraphicsOutput().Identity<1>()->mode.kind == ModeKind::Inactive,
					"graphics output does not inherit mode identity");
				display->Finish(StageStatus::NoPublication);
			}
		for (std::uint64_t id = 3; id < 80; ++id)
			PublishLogical(manager, static_cast<int>(id));
		Check(manager.RetainedHistory(Stage::Session) == 2, "history capacity comes from reads");
		Check(manager.RetainedHistory(Stage::Input) == 1, "different producers have different capacities");
		Check(!weak.expired(), "delayed reader pins old epoch data");

		for (std::uint64_t id = 80; id < 100; ++id) {
			manager.Activate(ModeKind::Game);
			PublishLogical(manager, static_cast<int>(id));
			Check(manager.RetainedHistory(Stage::Session) == 1, "reload does not allocate an accumulating history pool");
		}
	}
	Check(retained->Session().Current().value == 42, "view survives manager destruction");
	retained.reset();
	Check(weak.expired(), "last owning reader releases retired data");
}

static void FrozenVisualFeedbackAndLifecycle()
{
	SnapshotManager manager;
	manager.Register<TestContracts>(ModeKind::Game);
	manager.Activate(ModeKind::Game);
	PublishLogical(manager, 1);
	{
		auto visual = manager.BeginVisual({}, TestOutput());
		PublishDisplay(manager, {1}, 10);
	}
	auto logical = manager.BeginLogical(TestInput(), TestWindow());
	auto input = manager.BeginStage(LogicalIterationId {2}, Stage::Input);
	manager.Publish(*input, Value {2, {}});
	input->Finish(StageStatus::Completed);
	{
		auto visual = manager.BeginVisual({}, TestOutput());
		PublishDisplay(manager, {2}, 20);
	}
	auto session = manager.BeginStage(LogicalIterationId {2}, Stage::Session);
	auto view = manager.Acquire<TestContracts, Stage::Session>(*session);
	Check(view->Display().Previous()->value == 10, "logical visual feedback freezes at logical begin");
	manager.Publish(*session, Value {2, {}});
	manager.RequestLifecycle(*session, {LifecycleAction::SwitchMode, ModeKind::Loading, Handoff::Own(123)});
	Check(!manager.TakeLifecycleRequest({2}), "request not accepted while session running");
	session->Finish(StageStatus::Completed);
	auto request = manager.TakeLifecycleRequest({2});
	Check(request && *request->handoff.Get<int>() == 123, "completed session supplies request once");
	Check(!manager.TakeLifecycleRequest({2}), "historical reading cannot replay transition");
}

struct DeepContracts : TestContracts {
	using InputReads = SnapshotReads<
		Required<Stage::PlatformInput, Slot::Current>,
		HistoryRead<Stage::PlatformInput, 4>,
		HistoryRead<Stage::Session, 4>
	>;
};

static void DeclaredDepthAndSharedContinuity()
{
	SnapshotManager manager;
	manager.Register<TestContracts>(ModeKind::Game);
	manager.Register<DeepContracts>(ModeKind::Loading);
	manager.Activate(ModeKind::Game);
	for (std::uint64_t id = 1; id < 7; ++id)
		PublishLogical(manager, static_cast<int>(id));
	Check(manager.RetainedHistory(Stage::PlatformInput) == 5, "shared retention combines all registered consumers");
	manager.Activate(ModeKind::Loading);
	{
		auto iteration = manager.BeginLogical(TestInput(), TestWindow());
		auto input = manager.BeginStage(LogicalIterationId {7}, Stage::Input);
		auto view = manager.Acquire<DeepContracts, Stage::Input>(*input);
		Check(view->PlatformInput().Identity<4>()->iteration == 3, "indexed history works beyond aliases");
		Check(!view->Session().History<4>(), "mode history still stops at activation");
		manager.Publish(*input, Value {7, {}});
		input->Finish(StageStatus::Completed);
		auto session = manager.BeginStage(LogicalIterationId {7}, Stage::Session);
		Check(!manager.BeginSimulation({7}), "non-Game Session has no simulation publication scope");
		manager.Publish(*session, Value {7, {}});
		session->Finish(StageStatus::Completed);
	}
	for (std::uint64_t id = 8; id < 13; ++id)
		PublishLogical(manager, static_cast<int>(id));
	Check(manager.RetainedHistory(Stage::Session) == 5, "mode-local depth derives from deeper declaration");
}

static void PresentationReceiptsAndAdmittedLifetime()
{
	SnapshotManager manager;
	manager.Register<TestContracts>(ModeKind::Game);
	manager.Activate(ModeKind::Game);
	PublishLogical(manager, 1);
	{
		auto visual = manager.BeginVisual({}, TestOutput());
		PublishDisplay(manager, {1}, 1);
		auto render = manager.BeginStage(VisualIterationId {1}, Stage::Render);
		manager.Publish(*render, RenderedOutput {81, 9, 1, std::make_shared<TestResource>()});
		render->Finish(StageStatus::Completed);
		auto present = manager.BeginStage(VisualIterationId {1}, Stage::Present);
		auto inputs = manager.Acquire<ModeContracts, Stage::Present>(*present);
		Check(inputs->Render().Current().outputId == 81, "present gets exact registered render output");
		manager.Publish(*present, PresentationReceipt {81, 9, 1, PresentationOutcome::Presented, {}, {}});
		present->Finish(StageStatus::Completed);
		Check(!manager.BeginStage(VisualIterationId {1}, Stage::Present), "one presentation per visual invocation");
	}
	PublishLogical(manager, 2);
	auto visual = manager.BeginVisual({}, TestOutput());
	auto display = manager.BeginStage(VisualIterationId {2}, Stage::Display);
	auto inputs = manager.Acquire<TestContracts, Stage::Display>(*display);
	Check(inputs->Present().Previous()->outputId == 81, "receipt is readable as immutable history");
	manager.Publish(*display, Value {2, {}});
	display->Finish(StageStatus::Completed);
	auto render = manager.BeginStage(VisualIterationId {2}, Stage::Render);
	manager.Publish(*render, RenderedOutput {82, 9, 1, std::make_shared<TestResource>()});
	render->Finish(StageStatus::Completed);
	auto present = manager.BeginStage(VisualIterationId {2}, Stage::Present);
	auto frame = manager.Acquire<ModeContracts, Stage::Present>(*present);
	manager.Activate(ModeKind::Game);
	Check(frame->Render().Current().resource != nullptr, "admitted presentation retains retired output resources");
	Check(!manager.Publish(*present, PresentationReceipt {82, 9, 1, PresentationOutcome::Presented, {}, {}}), "late receipt cannot enter new activation");
	present->Finish(StageStatus::NoPublication);
}

int main()
{
	AssociationAndMissingCurrent();
	StageOrderingAndExceptions();
	SimulationCadenceAndHistory();
	RetentionAndRetirement();
	FrozenVisualFeedbackAndLifecycle();
	DeclaredDepthAndSharedContinuity();
	PresentationReceiptsAndAdmittedLifetime();
	std::cout << "PASS snapshot association, history, lifetime, authority and retirement\n";
}
