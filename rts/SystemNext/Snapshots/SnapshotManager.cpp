/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SnapshotManager.h"

#include <algorithm>
#include <array>
#include <map>
#include <stdexcept>

namespace runtime {
namespace detail {

constexpr std::size_t StageCount = static_cast<std::size_t>(Stage::Count);
using Key = std::pair<Flow, std::uint64_t>;

static std::size_t Index(Stage stage)
{
	if (stage >= Stage::Count)
		throw std::invalid_argument("Unknown stage");
	return static_cast<std::size_t>(stage);
}

struct IterationState {
public:
	InvocationMetadata context;
	std::optional<LogicalIterationId> logical;
	std::array<PublicationLease, StageCount> current;
	std::array<std::vector<PublicationLease>, StageCount> previous;
	std::array<StageStatus, StageCount> status {};
	std::optional<LifecycleRequest> request;
	bool requestTaken = false;
	bool open = true;
};

struct ManagerState {
public:
	std::size_t Depth(Stage stage) const
	{
		std::size_t depth = 0;
		for (const auto& [producer, requirement] : retention) {
			if (producer.second == stage && (producer.first == active.kind || stage <= Stage::GraphicsOutput))
				depth = std::max(depth, requirement);
		}
		return depth;
	}

	void Trim(Stage stage)
	{
		const auto index = Index(stage);
		const auto depth = Depth(stage);
		auto& records = history[index];
		if (records.size() > depth)
			records.resize(depth);
	}

	void Close(const std::shared_ptr<IterationState>& iteration) noexcept
	{
		if (!iteration || !iteration->open)
			return;

		iteration->open = false;
		const auto& context = iteration->context;
		if (context.mode == active && context.flow == Flow::Logical) {
			// Only a fully completed logical iteration is eligible for new visuals.
			const auto input = iteration->status[Index(Stage::Input)];
			const auto session = iteration->status[Index(Stage::Session)];
			const auto ended = [](StageStatus status) {
				return status == StageStatus::Completed || status == StageStatus::NoPublication || status == StageStatus::Omitted;
			};
			if (ended(input) && ended(session))
				latestLogical = iteration;
		}
		iterations.erase({context.flow, context.iteration});
	}

	std::shared_ptr<IterationState> Find(Flow flow, std::uint64_t id) const
	{
		const auto found = iterations.find({flow, id});
		return found == iterations.end() ? nullptr : found->second;
	}

public:
	ModeIdentity active;
	std::uint64_t generation = 0;
	std::uint64_t revision = 0;
	std::uint64_t logicalHighWater = 0;
	std::uint64_t visualHighWater = 0;
	std::map<Key, std::shared_ptr<IterationState>> iterations;
	std::shared_ptr<IterationState> latestLogical;
	PublicationLease activation;
	std::array<std::vector<PublicationLease>, StageCount> history;
	std::map<ModeKind, std::type_index> contracts;
	std::map<std::pair<ModeKind, Stage>, std::type_index> payloads;
	std::map<std::pair<ModeKind, Stage>, std::size_t> retention;
};

} // namespace detail

IterationLease::IterationLease(std::shared_ptr<detail::ManagerState> state, std::shared_ptr<detail::IterationState> iteration)
	: state(std::move(state)), iteration(std::move(iteration))
{
}

IterationLease::IterationLease(IterationLease&& other) noexcept = default;

IterationLease& IterationLease::operator=(IterationLease&& other) noexcept
{
	if (this != &other) {
		Close();
		state = std::move(other.state);
		iteration = std::move(other.iteration);
	}
	return *this;
}

IterationLease::~IterationLease()
{
	Close();
}

void IterationLease::Close() noexcept
{
	if (state)
		state->Close(iteration);
	iteration.reset();
	state.reset();
}

LogicalIterationId IterationLease::LogicalId() const
{
	if (!iteration || iteration->context.flow != Flow::Logical)
		throw std::logic_error("Lease is not a logical iteration");
	return LogicalIterationId {iteration->context.iteration};
}

VisualIterationId IterationLease::VisualId() const
{
	if (!iteration || iteration->context.flow != Flow::Visual)
		throw std::logic_error("Lease is not a visual iteration");
	return VisualIterationId {iteration->context.iteration};
}

InvocationLease::InvocationLease(std::shared_ptr<detail::ManagerState> state, std::shared_ptr<detail::IterationState> iteration, Stage stage)
	: state(std::move(state)), iteration(std::move(iteration)), stage(stage)
{
}

InvocationLease::InvocationLease(InvocationLease&& other) noexcept
	: state(std::move(other.state)), iteration(std::move(other.iteration)), stage(other.stage), finished(other.finished), published(other.published)
{
	other.finished = true;
}

InvocationLease& InvocationLease::operator=(InvocationLease&& other) noexcept
{
	if (this != &other) {
		Finish(StageStatus::Failed);
		state = std::move(other.state);
		iteration = std::move(other.iteration);
		stage = other.stage;
		finished = other.finished;
		published = other.published;
		other.finished = true;
	}
	return *this;
}

InvocationLease::~InvocationLease()
{
	Finish(StageStatus::Failed);
}

void InvocationLease::Finish(StageStatus status) noexcept
{
	if (!finished && iteration) {
		iteration->status[static_cast<std::size_t>(stage)] = status;
		finished = true;
	}
}

const InvocationMetadata& InvocationLease::Metadata() const
{
	return iteration->context;
}

SnapshotManager::SnapshotManager()
	: state(std::make_shared<detail::ManagerState>())
{
}

SnapshotManager::~SnapshotManager()
{
	Retire();
}

void SnapshotManager::RegisterType(ModeKind kind, std::type_index type)
{
	const auto [entry, inserted] = state->contracts.emplace(kind, type);
	if (!inserted && entry->second != type)
		throw std::invalid_argument("Mode kind already has a different snapshot contract");
}

void SnapshotManager::RegisterPayload(ModeKind kind, Stage stage, std::type_index type)
{
	state->payloads.insert_or_assign({kind, stage}, type);
}

void SnapshotManager::RegisterConsumer(ModeKind kind, Stage, std::span<const detail::ReadDescriptor> reads)
{
	for (const auto& read : reads) {
		auto& depth = state->retention[{kind, read.source}];
		depth = std::max(depth, static_cast<std::size_t>(read.slot) + 1);
	}
}

ModeIdentity SnapshotManager::Activate(ModeKind kind, Handoff handoff)
{
	if (kind == ModeKind::Inactive || !state->contracts.contains(kind))
		throw std::invalid_argument("Activation requires a registered concrete mode");

	const auto origin = state->active;
	const ModeIdentity next {kind, state->generation + 1};
	auto data = std::make_shared<const ActivationSnapshot>(ActivationSnapshot {origin, std::move(handoff)});
	auto publication = std::make_shared<const detail::StoredPublication>(detail::StoredPublication {
		{next, Stage::Activation, Flow::Logical, 0, state->revision + 1, {}},
		typeid(ActivationSnapshot), std::move(data)
	});

	Retire();
	state->generation = next.generation;
	state->active = next;
	++state->revision;
	state->activation = std::move(publication);
	return next;
}

void SnapshotManager::Retire() noexcept
{
	state->active = {};
	state->latestLogical.reset();
	state->activation.reset();

	// Application facts have separate continuity; mode-local history does not cross activation.
	for (std::size_t i = detail::Index(Stage::Activation); i < detail::StageCount; ++i)
		state->history[i].clear();
}

ModeIdentity SnapshotManager::Active() const
{
	return state->active;
}

IterationLease SnapshotManager::BeginLogical(PlatformInputSnapshot input, WindowSnapshot window)
{
	if (state->active.kind == ModeKind::Inactive)
		throw std::invalid_argument("Logical iteration requires an active mode");

	for (const auto& [key, iteration] : state->iterations) {
		if (key.first == Flow::Logical && iteration->context.mode == state->active)
			throw std::logic_error("A logical iteration is already open");
	}

	const LogicalIterationId id {++state->logicalHighWater};
	auto iteration = std::make_shared<detail::IterationState>();
	iteration->context = {state->active, Flow::Logical, id.value, input.sampledAt, input.realDelta};
	iteration->logical = id;
	iteration->previous = state->history;
	iteration->current[detail::Index(Stage::Activation)] = state->activation;

	auto inputPublication = std::make_shared<const detail::StoredPublication>(detail::StoredPublication {
		{{}, Stage::PlatformInput, Flow::Logical, id.value, ++state->revision, {}},
		typeid(PlatformInputSnapshot), std::make_shared<const PlatformInputSnapshot>(std::move(input))
	});
	auto windowPublication = std::make_shared<const detail::StoredPublication>(detail::StoredPublication {
		{{}, Stage::Window, Flow::Logical, id.value, ++state->revision, {}},
		typeid(WindowSnapshot), std::make_shared<const WindowSnapshot>(std::move(window))
	});
	iteration->current[detail::Index(Stage::PlatformInput)] = inputPublication;
	iteration->current[detail::Index(Stage::Window)] = windowPublication;
	state->history[detail::Index(Stage::PlatformInput)].insert(state->history[detail::Index(Stage::PlatformInput)].begin(), inputPublication);
	state->history[detail::Index(Stage::Window)].insert(state->history[detail::Index(Stage::Window)].begin(), windowPublication);
	state->Trim(Stage::PlatformInput);
	state->Trim(Stage::Window);
	state->iterations.emplace(detail::Key {Flow::Logical, id.value}, iteration);
	return IterationLease(state, std::move(iteration));
}

std::optional<IterationLease> SnapshotManager::BeginVisual(IterationTiming timing, GraphicsOutputSnapshot output)
{
	const auto logical = state->latestLogical;
	if (!logical || logical->context.mode != state->active)
		return std::nullopt;

	const VisualIterationId id {++state->visualHighWater};
	auto iteration = std::make_shared<detail::IterationState>();
	iteration->context = logical->context;
	iteration->context.flow = Flow::Visual;
	iteration->context.iteration = id.value;
	iteration->context.sampledAt = timing.sampledAt;
	iteration->context.realDelta = timing.realDelta;
	iteration->logical = logical->logical;
	iteration->previous = state->history;

	// Freeze the selected logical publication family, including missing Current.
	for (std::size_t i = 0; i <= detail::Index(Stage::Simulation); ++i) {
		iteration->current[i] = logical->current[i];
		iteration->previous[i] = logical->previous[i];
	}

	auto outputPublication = std::make_shared<const detail::StoredPublication>(detail::StoredPublication {
		{{}, Stage::GraphicsOutput, Flow::Visual, id.value, ++state->revision, {}},
		typeid(GraphicsOutputSnapshot), std::make_shared<const GraphicsOutputSnapshot>(std::move(output))
	});
	iteration->current[detail::Index(Stage::GraphicsOutput)] = outputPublication;
	state->history[detail::Index(Stage::GraphicsOutput)].insert(state->history[detail::Index(Stage::GraphicsOutput)].begin(), outputPublication);
	state->Trim(Stage::GraphicsOutput);

	// Simulation has its own cadence. An association names the last completed tick,
	// even when the selected logical iteration did not advance simulation.
	const auto simulation = detail::Index(Stage::Simulation);
	if (!iteration->current[simulation] && !iteration->previous[simulation].empty()) {
		iteration->current[simulation] = iteration->previous[simulation].front();
		iteration->previous[simulation].erase(iteration->previous[simulation].begin());
	}

	state->iterations.emplace(detail::Key {Flow::Visual, id.value}, iteration);
	return IterationLease(state, std::move(iteration));
}

std::optional<LogicalIterationId> SnapshotManager::AssociatedLogical(VisualIterationId id) const
{
	const auto iteration = state->Find(Flow::Visual, id.value);
	return iteration ? iteration->logical : std::nullopt;
}

std::optional<InvocationLease> SnapshotManager::BeginStage(LogicalIterationId id, Stage stage)
{
	if (stage != Stage::Input && stage != Stage::Session)
		throw std::invalid_argument("Not an ordinary logical stage");
	return Admit(Flow::Logical, id.value, stage, false);
}

std::optional<InvocationLease> SnapshotManager::BeginStage(VisualIterationId id, Stage stage)
{
	if (!detail::IsVisual(stage))
		throw std::invalid_argument("Not a visual stage");
	return Admit(Flow::Visual, id.value, stage, false);
}

std::optional<InvocationLease> SnapshotManager::BeginSimulation(LogicalIterationId id)
{
	return Admit(Flow::Logical, id.value, Stage::Simulation, true);
}

std::optional<InvocationLease> SnapshotManager::Admit(Flow flow, std::uint64_t id, Stage stage, bool simulation)
{
	const auto iteration = state->Find(flow, id);
	if (!iteration || !iteration->open || iteration->context.mode != state->active)
		return std::nullopt;

	auto& status = iteration->status[detail::Index(stage)];
	if (simulation) {
		if (state->active.kind != ModeKind::Game || iteration->status[detail::Index(Stage::Session)] != StageStatus::Running || status == StageStatus::Running)
			return std::nullopt;
	} else if (status != StageStatus::Pending) {
		return std::nullopt;
	}

	if (stage == Stage::Session) {
		const auto input = iteration->status[detail::Index(Stage::Input)];
		if (input != StageStatus::Completed && input != StageStatus::NoPublication && input != StageStatus::Omitted)
			return std::nullopt;
	}
	if (stage == Stage::Render) {
		const auto display = iteration->status[detail::Index(Stage::Display)];
		if (display != StageStatus::Completed && display != StageStatus::NoPublication && display != StageStatus::Omitted)
			return std::nullopt;
	}
	if (stage == Stage::Present && iteration->status[detail::Index(Stage::Render)] != StageStatus::Completed)
		return std::nullopt;

	status = StageStatus::Running;
	return InvocationLease(state, iteration, stage);
}

std::optional<detail::ResolvedSnapshots> SnapshotManager::Resolve(
	const InvocationLease& invocation,
	Stage consumer,
	std::span<const detail::ReadDescriptor> reads
) const
{
	if (invocation.state != state || invocation.finished || invocation.stage != consumer)
		throw std::logic_error("Snapshot acquisition requires the matching admitted invocation");

	const auto& iteration = *invocation.iteration;
	detail::ResolvedSnapshots resolved {iteration.context, {}};
	resolved.publications.reserve(reads.size());

	for (const auto& read : reads) {
		const auto source = detail::Index(read.source);
		detail::PublicationLease selected;
		if (read.slot == 0) {
			selected = iteration.current[source];
		} else if (iteration.previous[source].size() >= read.slot) {
			selected = iteration.previous[source][read.slot - 1];
		}

		if (selected && selected->type != read.type)
			throw std::logic_error("Snapshot payload does not match the compiled declaration");
		if (!selected && read.required)
			return std::nullopt;
		resolved.publications.push_back(std::move(selected));
	}
	return resolved;
}

bool SnapshotManager::Commit(
	InvocationLease& invocation,
	std::type_index type,
	std::shared_ptr<const void> data,
	std::optional<int> tick
)
{
	if (invocation.state != state || invocation.finished)
		throw std::logic_error("Publication requires a live admitted invocation");
	if (invocation.published)
		throw std::logic_error("Invocation already published");

	auto& iteration = *invocation.iteration;
	if (!iteration.open || iteration.context.mode != state->active)
		return false;

	const auto stage = invocation.stage;
	const auto index = detail::Index(stage);
	const auto expected = state->payloads.find({state->active.kind, stage});
	if (expected == state->payloads.end() || expected->second != type || type == typeid(void))
		throw std::invalid_argument("Publication payload is not registered for this stage");

	if (stage != Stage::Simulation && iteration.current[index])
		throw std::logic_error("Stage already published");
	if (stage != Stage::Simulation && tick)
		throw std::invalid_argument("Only simulation publications carry completed ticks");

	auto publication = std::make_shared<const detail::StoredPublication>(detail::StoredPublication {
		{state->active, stage, iteration.context.flow, iteration.context.iteration, ++state->revision, tick},
		type, std::move(data)
	});
	if (stage == Stage::Simulation && iteration.current[index])
		iteration.previous[index].insert(iteration.previous[index].begin(), iteration.current[index]);

	auto& history = state->history[index];
	history.insert(history.begin(), publication);
	state->Trim(stage);
	const auto depth = state->Depth(stage);
	if (iteration.previous[index].size() > depth)
		iteration.previous[index].resize(depth);
	iteration.current[index] = std::move(publication);
	invocation.published = true;
	return true;
}

void SnapshotManager::RequestLifecycle(InvocationLease& session, LifecycleRequest request)
{
	if (session.state != state || session.stage != Stage::Session || session.finished)
		throw std::logic_error("Only an admitted Session can request lifecycle work");

	auto& iteration = *session.iteration;
	if (!iteration.open || iteration.context.mode != state->active)
		return;
	if (!iteration.current[detail::Index(Stage::Session)] || iteration.request)
		throw std::logic_error("Lifecycle request needs one owned Session publication");
	iteration.request = std::move(request);
}

std::optional<LifecycleRequest> SnapshotManager::TakeLifecycleRequest(LogicalIterationId id)
{
	const auto iteration = state->Find(Flow::Logical, id.value);
	if (!iteration || iteration->context.mode != state->active || iteration->requestTaken
		|| iteration->status[detail::Index(Stage::Session)] != StageStatus::Completed)
		return std::nullopt;

	iteration->requestTaken = true;
	return iteration->request;
}

StageStatus SnapshotManager::Status(LogicalIterationId id, Stage stage) const
{
	const auto iteration = state->Find(Flow::Logical, id.value);
	return iteration ? iteration->status[detail::Index(stage)] : StageStatus::Unavailable;
}

StageStatus SnapshotManager::Status(VisualIterationId id, Stage stage) const
{
	const auto iteration = state->Find(Flow::Visual, id.value);
	return iteration ? iteration->status[detail::Index(stage)] : StageStatus::Unavailable;
}

std::size_t SnapshotManager::RetainedHistory(Stage source) const
{
	return state->history[detail::Index(source)].size();
}

} // namespace runtime
