/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "IMode.h"
#include "../Globals/Snapshots/SnapshotManager.h"

#include <concepts>
#include <stdexcept>

namespace runtime {

namespace detail {

struct AbsentPayload {
public:
};

template<class T>
using DeclaredPayload = std::conditional_t<std::is_void_v<T>, AbsentPayload, T>;

} // namespace detail

/**
 * Generated bridge between runtime selection and compile-time concern inputs.
 *
 * Why: virtual functions share a signature, but different modes need different
 * snapshot views. Erasure stays here. Concern bodies receive owning typed views
 * and return owned products, with no manager/global lookup or borrowed output.
 * The inherited aliases keep the declarations readable in each mode's header.
 */
template<class Derived, class Declarations>
class Mode : public IMode {
public:
	using Contracts = Declarations;
	using InputSnapshots = SnapshotView<Contracts, Stage::Input>;
	using SessionSnapshots = SnapshotView<Contracts, Stage::Session>;
	using DisplaySnapshots = SnapshotView<Contracts, Stage::Display>;
	using RenderSnapshots = SnapshotView<Contracts, Stage::Render>;

	using InputPublication = std::optional<detail::DeclaredPayload<typename Contracts::InputData>>;
	using SessionPublication = SessionOutput<detail::DeclaredPayload<typename Contracts::SessionData>>;
	using DisplayPublication = std::optional<detail::DeclaredPayload<typename Contracts::DisplayData>>;
	using RenderPublication = std::unique_ptr<const RenderCommands>;

	void RegisterSnapshots(SnapshotManager& snapshots) const final
	{
		snapshots.Register<Contracts>(kind);
	}

	void ExecuteInput(SnapshotManager& snapshots, LogicalIterationId iteration) final
	{
		auto invocation = snapshots.BeginStage(iteration, Stage::Input);
		if (!invocation)
			return;

		if constexpr (requires(Derived& mode, const InputSnapshots& inputs) { mode.Input(inputs); }) {
			static_assert(requires(Derived& mode, const InputSnapshots& inputs) {
				{ mode.Input(inputs) } -> std::same_as<InputPublication>;
			});
			const auto inputs = snapshots.Acquire<Contracts, Stage::Input>(*invocation);
			if (!inputs) {
				invocation->Finish(StageStatus::Unavailable);
				return;
			}

			auto output = static_cast<Derived&>(*this).Input(*inputs);
			const bool committed = output && snapshots.Publish(*invocation, std::move(*output));
			invocation->Finish(committed ? StageStatus::Completed : StageStatus::NoPublication);
		} else {
			static_assert(Contracts::InputReads::size == 0, "Absent Input cannot consume snapshots");
			invocation->Finish(StageStatus::Omitted);
		}
	}

	void ExecuteSession(SnapshotManager& snapshots, LogicalIterationId iteration) final
	{
		auto invocation = snapshots.BeginStage(iteration, Stage::Session);
		if (!invocation)
			return;

		if constexpr (requires(Derived& mode, const SessionSnapshots& inputs) { mode.Session(inputs); }) {
			static_assert(requires(Derived& mode, const SessionSnapshots& inputs) {
				{ mode.Session(inputs) } -> std::same_as<SessionPublication>;
			});
			const auto inputs = snapshots.Acquire<Contracts, Stage::Session>(*invocation);
			if (!inputs) {
				invocation->Finish(StageStatus::Unavailable);
				return;
			}

			auto output = static_cast<Derived&>(*this).Session(*inputs);
			if (output.lifecycle && !output.snapshot)
				throw std::logic_error("Lifecycle request has no Session publication");
			const bool committed = output.snapshot && snapshots.Publish(*invocation, std::move(*output.snapshot));
			if (committed && output.lifecycle)
				snapshots.RequestLifecycle(*invocation, std::move(*output.lifecycle));
			invocation->Finish(committed ? StageStatus::Completed : StageStatus::NoPublication);
		} else {
			static_assert(Contracts::SessionReads::size == 0, "Absent Session cannot consume snapshots");
			invocation->Finish(StageStatus::Omitted);
		}
	}

	void ExecuteDisplay(SnapshotManager& snapshots, VisualIterationId iteration) final
	{
		auto invocation = snapshots.BeginStage(iteration, Stage::Display);
		if (!invocation)
			return;

		if constexpr (requires(Derived& mode, const DisplaySnapshots& inputs) { mode.Display(inputs); }) {
			static_assert(requires(Derived& mode, const DisplaySnapshots& inputs) {
				{ mode.Display(inputs) } -> std::same_as<DisplayPublication>;
			});
			const auto inputs = snapshots.Acquire<Contracts, Stage::Display>(*invocation);
			if (!inputs) {
				invocation->Finish(StageStatus::Unavailable);
				return;
			}

			auto output = static_cast<Derived&>(*this).Display(*inputs);
			const bool committed = output && snapshots.Publish(*invocation, std::move(*output));
			invocation->Finish(committed ? StageStatus::Completed : StageStatus::NoPublication);
		} else {
			static_assert(Contracts::DisplayReads::size == 0, "Absent Display cannot consume snapshots");
			invocation->Finish(StageStatus::Omitted);
		}
	}

	std::optional<RenderWork> DescribeRender(SnapshotManager& snapshots, VisualIterationId iteration) final
	{
		auto invocation = snapshots.BeginStage(iteration, Stage::Render);
		if (!invocation)
			return {};

		if constexpr (requires(Derived& mode, const RenderSnapshots& inputs) { mode.Render(inputs); }) {
			static_assert(requires(Derived& mode, const RenderSnapshots& inputs) {
				{ mode.Render(inputs) } -> std::same_as<RenderPublication>;
			});
			const auto inputs = snapshots.Acquire<Contracts, Stage::Render>(*invocation);
			if (!inputs) {
				invocation->Finish(StageStatus::Unavailable);
				return {};
			}

			// The application completes the admitted stage after backend execution.
			auto output = static_cast<Derived&>(*this).Render(*inputs);
			if (!output) {
				invocation->Finish(StageStatus::NoPublication);
				return {};
			}
			return RenderWork {std::move(*invocation), std::move(output)};
		} else {
			static_assert(Contracts::RenderReads::size == 0, "Absent Render cannot consume snapshots");
			invocation->Finish(StageStatus::Omitted);
			return {};
		}
	}

protected:
	explicit Mode(ModeKind kind)
		: IMode(kind)
	{
	}

};

} // namespace runtime
