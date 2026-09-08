/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ApplicationLoop.h"

#include "Globals/Graphics/VisualOutput.h"
#include "Globals/Lifecycle/ApplicationHost.h"
#include "Globals/Snapshots/SnapshotManager.h"
#include "Modes/IMode.h"
#include "Modes/ModeBinding.h"

#include <stdexcept>

namespace runtime {

ApplicationLoop::ApplicationLoop(ApplicationContext context)
	: context(context)
{
}

void ApplicationLoop::Run()
{
	/*
	 * Responsibility and why: lifecycle owns resources around one serial scheduler.
	 * Logical work can run without visual execution. Separate iteration IDs and
	 * owning publications prepare independent scheduling without adding threads.
	 * Inputs and outputs: application services supply platform facts and an initial
	 * binding; stage publications remain owned by the snapshot manager.
	 * Scheduling and lifetime: stage/iteration leases unwind before shutdown.
	 * Expected legacy sources: [SpringApp.cpp](../System/SpringApp.cpp),
	 * SpringApp::Run(), Init(), Reload(), Kill(). No production redirection exists yet.
	 */
	std::uint64_t logical = 0;
	std::uint64_t visual = 0;

	try {
		context.host.Initialize();
		if (context.activeMode.mode) {
			context.activeMode.mode->RegisterSnapshots(context.snapshots);
			context.activeMode.identity = context.snapshots.Activate(context.activeMode.mode->kind);
		}

		while (!exitRequested && context.host.BeginIteration()) {
			// [ logical ] Interpret actions, advance the session, commit decisions.
			const bool visualReady = UpdateLogic(LogicalIterationId {++logical});

			// [ visual ] Headless has no subsystem, resources, or substitute calls.
			if (visualReady && context.visualOutput && !exitRequested)
				UpdateVisuals(VisualIterationId {++visual});

			context.host.FlushDiagnostics();
		}
	} catch (...) {
		Shutdown();
		throw;
	}

	Shutdown();
}

bool ApplicationLoop::UpdateLogic(LogicalIterationId id)
{
	/*
	 * Why: Input and Session belong to one activation. A replacement must interpret
	 * its own input in a fresh iteration, rather than consume retired mode actions.
	 * Application event collection precedes mode interpretation exactly once.
	 * Required Session inputs gate work; unavailable data never becomes success.
	 */
	auto application = context.host.CollectInput();

	if (context.host.ExitRequested()) {
		exitRequested = true;
		return false;
	}
	if (context.host.ReloadRequested()) {
		CommitLifecycle({LifecycleAction::Reload, ModeKind::Inactive, {}});
		return false;
	}
	if (!context.activeMode.mode)
		return false;

	auto iteration = context.snapshots.BeginLogical(id, std::move(application));

	// [ input ] Publish interpreted intent.
	Input(id);

	if (context.host.ExitRequested()) {
		exitRequested = true;
		return false;
	}
	if (context.host.ReloadRequested()) {
		CommitLifecycle({LifecycleAction::Reload, ModeKind::Inactive, {}});
		return false;
	}

	// [ session / simulation / publication ] Decide logical consequences.
	Session(id);
	if (context.host.ExitRequested()) {
		exitRequested = true;
		return false;
	}
	if (context.host.ReloadRequested()) {
		CommitLifecycle({LifecycleAction::Reload, ModeKind::Inactive, {}});
		return false;
	}

	// [ lifecycle ] Commit only after Session returned and published.
	if (const auto request = context.snapshots.TakeLifecycleRequest(id)) {
		CommitLifecycle(*request);
		return false;
	}

	const auto status = context.snapshots.Status(id, Stage::Session);
	return status == StageStatus::Completed || status == StageStatus::NoPublication || status == StageStatus::Omitted;
}

void ApplicationLoop::UpdateVisuals(VisualIterationId id)
{
	/*
	 * Why: one visual iteration pins one completed logical association and activation.
	 * Display, Render and Present each acquire their declared immutable views.
	 * A mode without Display can render from Session directly. No fabricated display
	 * frame is necessary. Retirement prevents admission of subsequent old-mode work.
	 */
	const auto timing = context.host.CaptureVisualTiming();
	auto iteration = context.snapshots.BeginVisual(id, timing);
	if (!iteration)
		return;

	const auto selected = context.activeMode;
	const InvocationContext invocation {selected.identity, Flow::Visual, id.value, timing.sampledAt, timing.realDelta};
	const auto schedule = context.visualOutput->PlanIteration(invocation);
	if (schedule == VisualSchedule::None || context.host.ExitRequested() || context.host.ReloadRequested())
		return;

	// [ display ] Derive owned visual state when this mode has that concern.
	Display(id);
	if (context.host.ExitRequested() || context.host.ReloadRequested())
		return;

	// [ render ] Describe commands and execute them through application resources.
	if (schedule == VisualSchedule::Offscreen || schedule == VisualSchedule::Present)
		Render(id);
	if (context.host.ExitRequested() || context.host.ReloadRequested())
		return;

	// [ present ] Only an actual rendered publication permits delivery.
	if (schedule == VisualSchedule::Present)
		Present(id);
}

void ApplicationLoop::Input(LogicalIterationId id)
{
	/*
	 * Responsibility and why: mode Input interprets already-collected events into
	 * owning actions. It cannot request activation or advance authoritative state.
	 * Inputs: generated Application.Current and declared optional history.
	 * Outputs: a mode-specific Input publication. Session reads this iteration's
	 * Current; historical reads do not replay the input batch.
	 * Expected legacy sources: [SpringApp.cpp](../System/SpringApp.cpp),
	 * SpringApp::MainEventHandler(). Actual platform/input adaptation remains outlined.
	 */
	const auto selected = context.activeMode;
	if (selected.mode && selected.identity == context.snapshots.Active())
		selected.mode->ExecuteInput(context.snapshots, id);
}

void ApplicationLoop::Session(LogicalIterationId id)
{
	/*
	 * Responsibility and why: logical decisions need one authority which works
	 * without graphics. Input intent is accepted here, including normal mode changes.
	 * Inputs: declared Input.Current, activation handoff and optional visual history.
	 * Outputs: owning Session state and at most one lifecycle request; Game may
	 * publish several completed authoritative frames during this stage.
	 * Relationships: lifecycle commits after return; a switch ends the iteration.
	 * Expected legacy sources: [Game.cpp](../Game/Game.cpp), CGame::Update();
	 * [NetCommands.cpp](../Net/NetCommands.cpp), CGame::ClientReadNet().
	 */
	const auto selected = context.activeMode;
	if (selected.mode && selected.identity == context.snapshots.Active())
		selected.mode->ExecuteSession(context.snapshots, id);
}

void ApplicationLoop::Display(VisualIterationId id)
{
	/*
	 * Responsibility and why: visual preparation derives owned data from a fixed
	 * logical association. It cannot mutate logical state or activate another mode.
	 * Inputs and outputs: each mode declares its logical/history reads and emits
	 * a display snapshot. An absent concern emits nothing; Render may declare direct
	 * Session reads instead. Headless never admits this stage.
	 * Expected legacy sources: [Game.cpp](../Game/Game.cpp), CGame::UpdateUnsynced().
	 */
	const auto selected = context.activeMode;
	if (selected.mode && selected.identity == context.snapshots.Active())
		selected.mode->ExecuteDisplay(context.snapshots, id);
}

void ApplicationLoop::Render(VisualIterationId id)
{
	/*
	 * Responsibility and why: mode commands and backend execution form one admitted
	 * rendering invocation. Its lease owns frame lifetime across both operations,
	 * including exceptions. No mutable scratch frame survives on the mode.
	 * Inputs: declared Display.Current or direct logical publications.
	 * Output: an owning rendered output, never a success-only boolean.
	 * Expected legacy sources: [Game.cpp](../Game/Game.cpp), CGame::Draw().
	 */
	const auto selected = context.activeMode;
	if (!selected.mode || selected.identity != context.snapshots.Active())
		return;

	auto work = selected.mode->DescribeRender(context.snapshots, id);
	if (!work)
		return;

	auto output = context.visualOutput->Render(std::move(work->commands));
	if (!output) {
		work->invocation.Finish(StageStatus::NoPublication);
		return;
	}
	if (!output->resource || output->outputId == 0)
		throw std::logic_error("Renderer returned an invalid owning output");

	const bool committed = context.snapshots.Publish(work->invocation, std::move(*output));
	work->invocation.Finish(committed ? StageStatus::Completed : StageStatus::NoPublication);
}

void ApplicationLoop::Present(VisualIterationId id)
{
	/*
	 * Responsibility and why: delivery consumes one exact completed render output.
	 * The backend owns target synchronization. Presentation never queries a live
	 * mode, advances simulation, or silently swaps from inside rendering.
	 * Inputs: required Render.Current for this visual iteration and activation.
	 * Output: a receipt with output/target identity, outcome and timing. Missing
	 * output prevents execution. An already admitted presentation may finish during
	 * retirement, but its receipt cannot enter replacement-mode history.
	 * Expected legacy sources: [SpringApp.cpp](../System/SpringApp.cpp),
	 * SpringApp::Update(); [LoadScreen.cpp](../Game/LoadScreen.cpp),
	 * CLoadScreen::SetLoadMessage(). Progress publication is not recursive Present.
	 */
	auto invocation = context.snapshots.BeginStage(id, Stage::Present);
	if (!invocation)
		return;

	const auto inputs = context.snapshots.Acquire<ModeContracts, Stage::Present>(*invocation);
	if (!inputs) {
		invocation->Finish(StageStatus::Unavailable);
		return;
	}

	const auto& output = inputs->Render().Current();
	auto receipt = context.visualOutput->Present(output);
	if (receipt.outputId != output.outputId || receipt.targetId != output.targetId || receipt.completedAt < receipt.startedAt)
		throw std::logic_error("Presentation receipt does not identify the delivered output");

	const bool committed = context.snapshots.Publish(*invocation, std::move(receipt));
	invocation->Finish(committed ? StageStatus::Completed : StageStatus::NoPublication);
}

void ApplicationLoop::CommitLifecycle(const LifecycleRequest& request)
{
	/*
	 * Why: Session decides normal progression; lifecycle constructs/retire resources.
	 * Prepare a replacement before activation. Failure propagates to shutdown and
	 * cannot masquerade as a successful switch. Handoff owns startup data, while new
	 * activation history starts empty. Exit is monotonic and needs no active mode.
	 */
	if (exitRequested)
		return;

	if (request.action == LifecycleAction::Exit || context.host.ExitRequested()) {
		exitRequested = true;
		context.snapshots.Retire();
		return;
	}

	auto replacement = context.host.CreateMode(request);
	if (!replacement || (request.action == LifecycleAction::SwitchMode && replacement->kind != request.target))
		throw std::runtime_error("Requested mode activation failed");

	replacement->RegisterSnapshots(context.snapshots);
	const auto identity = context.snapshots.Activate(replacement->kind, request.handoff);
	context.activeMode = {std::move(replacement), identity};
}

void ApplicationLoop::Shutdown()
{
	/*
	 * Why: close publication acceptance before retiring dependencies. Owning views
	 * and admitted backend output leases remain readable independently of this host.
	 * Cleanup also follows failed initialization or stage exceptions.
	 */
	exitRequested = true;
	context.snapshots.Retire();
	context.activeMode = {};
	context.host.Shutdown();
}

} // namespace runtime
