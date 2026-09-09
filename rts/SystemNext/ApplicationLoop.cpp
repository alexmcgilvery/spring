/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ApplicationLoop.h"

#include "Application/Diagnostics/Diagnostics.h"
#include "Application/Graphics/Graphics.h"
#include "Application/Lifecycle/ApplicationLifecycle.h"
#include "Application/Platform/Platform.h"
#include "Modes/IMode.h"
#include "Snapshots/SnapshotManager.h"

#include <stdexcept>
#include <utility>

namespace runtime {

ApplicationLoop::ApplicationLoop(
	Platform& platform,
	ApplicationLifecycle& lifecycle,
	SnapshotManager& snapshots,
	Diagnostics& diagnostics,
	Graphics* graphics
)
	: platform(platform)
	, lifecycle(lifecycle)
	, snapshots(snapshots)
	, diagnostics(diagnostics)
	, graphics(graphics)
{
}

void ApplicationLoop::Run()
{
	/* Lifecycle surrounds one plainly ordered scheduler. Logical work can run
	 * without graphics. Independent identities and owning publications permit
	 * later scheduling separation without introducing it here. */
	try {
		// [ application lifecycle ] Construct resources and the initial mode.
		lifecycle.Initialize();
		Activate(lifecycle.CreateInitialMode());

		while (!exitRequested && platform.BeginIteration()) {
			// [ application lifecycle ] Platform requests can exist without a mode.
			if (const auto request = PlatformLifecycleRequest()) {
				CommitLifecycle(*request);
				diagnostics.Report();
				continue;
			}

			// [ input → session ] Advance one activation's logical work.
			const bool visualReady = UpdateLogic(platform.CollectPublications());

			// [ display → render → present ] Omitted when graphics is absent.
			if (visualReady && graphics != nullptr && !exitRequested)
				UpdateVisuals();

			// [ application lifecycle ] Commit requests observed during visual work.
			if (const auto request = PlatformLifecycleRequest())
				CommitLifecycle(*request);

			// [ diagnostics ] Report buffered observations at the outer boundary.
			diagnostics.Report();
		}
	} catch (...) {
		Shutdown();
		throw;
	}

	Shutdown();
}

bool ApplicationLoop::UpdateLogic(PlatformPublications publications)
{
	/* Input and Session execute against one activation and one immutable pair of
	 * platform-input/window publications. Lifecycle changes commit only after the
	 * logical lease closes, so a replacement starts a fresh iteration. */
	if (!activeMode.mode)
		return false;

	std::optional<LifecycleRequest> lifecycleRequest;
	bool visualReady = false;

	{
		auto iteration = snapshots.BeginLogical(
			std::move(publications.input),
			std::move(publications.window)
		);
		const auto id = iteration.LogicalId();

		// [ input ] Interpret the application-owned event publication.
		Input(id);
		lifecycleRequest = PlatformLifecycleRequest();

		// [ session / simulation / publication ] Accept logical consequences.
		if (!lifecycleRequest) {
			Session(id);
			lifecycleRequest = PlatformLifecycleRequest();
		}

		if (!lifecycleRequest)
			lifecycleRequest = snapshots.TakeLifecycleRequest(id);

		const auto status = snapshots.StageStatusOf(id, Stage::Session);
		visualReady = status == StageStatus::Completed
			|| status == StageStatus::NoPublication
			|| status == StageStatus::Omitted;
	}

	// [ application lifecycle ] The old logical association is closed first.
	if (lifecycleRequest) {
		CommitLifecycle(*lifecycleRequest);
		return false;
	}

	return visualReady;
}

void ApplicationLoop::UpdateVisuals()
{
	/* Graphics selects one schedule and immutable output target. The manager freezes
	 * those facts with one completed logical association for all visual stages.
	 * Rendering and presentation remain serial and blocking in this pass. */
	const auto plan = graphics->PlanVisuals(activeMode.identity);
	if (plan.schedule == VisualSchedule::Skip)
		return;

	auto iteration = snapshots.BeginVisual(plan.timing, plan.output);
	if (!iteration)
		return;
	const auto id = iteration->VisualId();

	// [ display ] Derive client-visible state when the mode defines the concern.
	Display(id);
	if (PlatformLifecycleRequest())
		return;

	// [ render ] Describe content, then execute it against the frozen target.
	if (plan.schedule == VisualSchedule::Offscreen || plan.schedule == VisualSchedule::Present)
		Render(id);
	if (PlatformLifecycleRequest())
		return;

	// [ present ] Deliver only the exact output published by Render.
	if (plan.schedule == VisualSchedule::Present)
		Present(id);
}

void ApplicationLoop::Input(LogicalIterationId id)
{
	/* Modes interpret ordered platform events into owned intent. Input cannot
	 * activate modes or advance authoritative state. Its compile-time declaration
	 * names PlatformInput, Window, Activation and permitted history. */
	const auto selected = activeMode;
	if (selected.mode && selected.identity == snapshots.Active())
		selected.mode->ExecuteInput(snapshots, id);
}

void ApplicationLoop::Session(LogicalIterationId id)
{
	/* Session is the graphics-independent authority for accepted intent,
	 * authoritative Game simulation and normal lifecycle decisions. */
	const auto selected = activeMode;
	if (selected.mode && selected.identity == snapshots.Active())
		selected.mode->ExecuteSession(snapshots, id);
}

void ApplicationLoop::Display(VisualIterationId id)
{
	/* Display prepares client-visible state from frozen logical, window and output
	 * publications. It cannot mutate Session or activate a mode. */
	const auto selected = activeMode;
	if (selected.mode && selected.identity == snapshots.Active())
		selected.mode->ExecuteDisplay(snapshots, id);
}

void ApplicationLoop::Render(VisualIterationId id)
{
	/* A mode describes content from declared snapshots; application Graphics
	 * executes it. RenderWork carries the exact target selected for this iteration. */
	const auto selected = activeMode;
	if (!selected.mode || selected.identity != snapshots.Active())
		return;

	auto work = selected.mode->DescribeRender(snapshots, id);
	if (!work)
		return;

	auto output = graphics->Render(work->target, std::move(work->commands));
	if (!output) {
		work->invocation.Finish(StageStatus::NoPublication);
		return;
	}
	if (!output->resource || output->outputId == 0
		|| output->targetId != work->target.targetId
		|| output->targetGeneration != work->target.generation) {
		throw std::logic_error("Graphics returned an invalid or retargeted output");
	}

	const bool committed = snapshots.Publish(work->invocation, std::move(*output));
	work->invocation.Finish(committed ? StageStatus::Completed : StageStatus::NoPublication);
}

void ApplicationLoop::Present(VisualIterationId id)
{
	/* Present consumes Render.Current and its original target generation. Graphics
	 * decides whether that destination is still presentable and reports a receipt. */
	auto invocation = snapshots.BeginStage(id, Stage::Present);
	if (!invocation)
		return;

	const auto inputs = snapshots.Acquire<ModeContracts, Stage::Present>(*invocation);
	if (!inputs) {
		invocation->Finish(StageStatus::Unavailable);
		return;
	}

	const auto& output = inputs->Render().Current();
	auto receipt = graphics->Present(output);
	if (receipt.outputId != output.outputId
		|| receipt.targetId != output.targetId
		|| receipt.targetGeneration != output.targetGeneration
		|| receipt.completedAt < receipt.startedAt) {
		throw std::logic_error("Presentation receipt does not identify the rendered output");
	}

	const bool committed = snapshots.Publish(*invocation, std::move(receipt));
	invocation->Finish(committed ? StageStatus::Completed : StageStatus::NoPublication);
}

std::optional<LifecycleRequest> ApplicationLoop::PlatformLifecycleRequest() const
{
	if (platform.ExitRequested())
		return LifecycleRequest {LifecycleAction::Exit, ModeKind::Inactive, {}};
	if (platform.ReloadRequested())
		return LifecycleRequest {LifecycleAction::Reload, ModeKind::Inactive, {}};
	return {};
}

void ApplicationLoop::CommitLifecycle(const LifecycleRequest& request)
{
	/* Session decides normal progression; lifecycle constructs replacements. Exit
	 * is monotonic, and snapshot retirement precedes dependency retirement. */
	if (exitRequested)
		return;

	if (request.action == LifecycleAction::Exit) {
		exitRequested = true;
		snapshots.Retire();
		activeMode = {};
		return;
	}

	Activate(lifecycle.CreateMode(request), &request);
}

void ApplicationLoop::Activate(std::shared_ptr<IMode> mode, const LifecycleRequest* request)
{
	if (!mode) {
		if (request != nullptr)
			throw std::runtime_error("Requested mode activation failed");
		return;
	}
	if (request != nullptr && request->action == LifecycleAction::SwitchMode && mode->kind != request->target)
		throw std::runtime_error("Lifecycle constructed the wrong requested mode");

	mode->RegisterSnapshots(snapshots);
	const auto identity = snapshots.Activate(mode->kind, request != nullptr ? request->handoff : Handoff {});
	activeMode = {std::move(mode), identity};
}

void ApplicationLoop::Shutdown()
{
	exitRequested = true;
	snapshots.Retire();
	activeMode = {};
	lifecycle.Shutdown();
}

} // namespace runtime
