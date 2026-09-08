/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SnapshotView.h"

#include <span>

namespace runtime {

namespace detail {

// Forward declarations: private storage outlives the manager while scopes retain it.
struct ManagerState;
struct IterationState;

} // namespace detail

class SnapshotManager;

/** Closing a logical/visual association cannot invalidate already issued views. */
class IterationLease {
public:
	IterationLease(IterationLease&& other) noexcept;
	IterationLease& operator=(IterationLease&& other) noexcept;
	~IterationLease();

	void Close() noexcept;
	LogicalIterationId LogicalId() const;
	VisualIterationId VisualId() const;

private:
	friend class SnapshotManager;

	IterationLease(std::shared_ptr<detail::ManagerState> state, std::shared_ptr<detail::IterationState> iteration);

private:
	std::shared_ptr<detail::ManagerState> state;
	std::shared_ptr<detail::IterationState> iteration;
};

/** Admission pins an invocation. Unfinished work becomes Failed during unwinding. */
class InvocationLease {
public:
	InvocationLease(InvocationLease&& other) noexcept;
	InvocationLease& operator=(InvocationLease&& other) noexcept;
	~InvocationLease();

	void Finish(StageStatus status) noexcept;
	const InvocationMetadata& Metadata() const;

private:
	friend class SnapshotManager;

	InvocationLease(std::shared_ptr<detail::ManagerState> state, std::shared_ptr<detail::IterationState> iteration, Stage stage);

private:
	std::shared_ptr<detail::ManagerState> state;
	std::shared_ptr<detail::IterationState> iteration;
	Stage stage;
	bool finished = false;
	bool published = false;
};

/**
 * Owns association, admission, immutable storage and consumer-derived history.
 *
 * Logical and visual IDs are independent monotonic names. Cross-flow associations
 * freeze at Begin; same-flow Current resolves at Acquire after preceding work.
 * A missing associated publication never becomes a previous publication. Retired
 * activations reject new admissions/commits, while issued owning views stay valid.
 *
 * This serial implementation has no worker synchronization or production overload
 * policy. Declared history bounds manager caches; external leases can retain more.
 */
class SnapshotManager {
public:
	SnapshotManager();
	~SnapshotManager();

	SnapshotManager(const SnapshotManager&) = delete;
	SnapshotManager& operator=(const SnapshotManager&) = delete;

	template<class Contracts>
	void Register(ModeKind kind)
	{
		RegisterType(kind, typeid(Contracts));
		RegisterProducer<Contracts, Stage::Input>(kind);
		RegisterProducer<Contracts, Stage::Session>(kind);
		RegisterProducer<Contracts, Stage::Simulation>(kind);
		RegisterProducer<Contracts, Stage::Display>(kind);
		RegisterProducer<Contracts, Stage::Render>(kind);
		RegisterProducer<Contracts, Stage::Present>(kind);
		RegisterReads<Contracts, Stage::Input>(kind);
		RegisterReads<Contracts, Stage::Session>(kind);
		RegisterReads<Contracts, Stage::Display>(kind);
		RegisterReads<Contracts, Stage::Render>(kind);
		RegisterReads<Contracts, Stage::Present>(kind);
	}

	ModeIdentity Activate(ModeKind kind, Handoff handoff = {});
	void Retire() noexcept;
	ModeIdentity Active() const;

	IterationLease BeginLogical(PlatformInputSnapshot input, WindowSnapshot window);
	std::optional<IterationLease> BeginVisual(IterationTiming timing, GraphicsOutputSnapshot output);
	std::optional<LogicalIterationId> AssociatedLogical(VisualIterationId id) const;

	std::optional<InvocationLease> BeginStage(LogicalIterationId id, Stage stage);
	std::optional<InvocationLease> BeginStage(VisualIterationId id, Stage stage);
	std::optional<InvocationLease> BeginSimulation(LogicalIterationId id);

	template<class Contracts, Stage Consumer>
	std::optional<SnapshotView<Contracts, Consumer>> Acquire(const InvocationLease& invocation) const
	{
		const auto reads = detail::CompiledReads<Contracts, Consumer, ReadsFor<Contracts, Consumer>>::Descriptors();
		auto resolved = Resolve(invocation, Consumer, reads);
		if (!resolved)
			return std::nullopt;
		return SnapshotView<Contracts, Consumer>(std::move(*resolved));
	}

	template<class T>
	bool Publish(InvocationLease& invocation, T value, std::optional<int> tick = {})
	{
		return Commit(invocation, typeid(T), std::make_shared<const T>(std::move(value)), tick);
	}

	void RequestLifecycle(InvocationLease& session, LifecycleRequest request);
	std::optional<LifecycleRequest> TakeLifecycleRequest(LogicalIterationId id);
	StageStatus Status(LogicalIterationId id, Stage stage) const;
	StageStatus Status(VisualIterationId id, Stage stage) const;
	std::size_t RetainedHistory(Stage source) const;

private:
	template<class Contracts, Stage Source>
	void RegisterProducer(ModeKind kind)
	{
		RegisterPayload(kind, Source, typeid(PayloadFor<Contracts, Source>));
	}

	template<class Contracts, Stage Consumer>
	void RegisterReads(ModeKind kind)
	{
		const auto reads = detail::CompiledReads<Contracts, Consumer, ReadsFor<Contracts, Consumer>>::Descriptors();
		RegisterConsumer(kind, Consumer, reads);
	}

	void RegisterType(ModeKind kind, std::type_index type);
	void RegisterPayload(ModeKind kind, Stage stage, std::type_index type);
	void RegisterConsumer(ModeKind kind, Stage consumer, std::span<const detail::ReadDescriptor> reads);

	std::optional<InvocationLease> Admit(Flow flow, std::uint64_t id, Stage stage, bool simulation);
	std::optional<detail::ResolvedSnapshots> Resolve(const InvocationLease& invocation, Stage consumer, std::span<const detail::ReadDescriptor> reads) const;
	bool Commit(InvocationLease& invocation, std::type_index type, std::shared_ptr<const void> data, std::optional<int> tick);

private:
	std::shared_ptr<detail::ManagerState> state;
};

} // namespace runtime
