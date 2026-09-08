/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../InvocationContext.h"

#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

namespace runtime {

/** Application and Activation are owned sources, not additional loop stages. */
enum class Stage {
	Application,
	Activation,
	Input,
	Session,
	Simulation,
	Display,
	Render,
	Present,
	Count,
};
enum class Slot : unsigned {
	Current,
	Previous,
	Older,
};
enum class StageStatus {
	Pending,
	Running,
	Completed,
	NoPublication,
	Omitted,
	Unavailable,
	Failed,
};

/** Collected facts cross by value; concrete platform event adaptation remains unimplemented. */
struct InputEvent {
public:
	std::uint64_t sequence = 0;
	std::string kind;
	std::string text;
	std::vector<double> values;
};

struct ApplicationSnapshot {
public:
	std::vector<InputEvent> events;
	std::chrono::nanoseconds sampledAt {};
	std::chrono::nanoseconds realDelta {};
	unsigned width = 0;
	unsigned height = 0;
	bool focused = false;
};

/**
 * Owning startup data with checked typed access. Mutable world/resource ownership
 * stays with lifecycle; publication never grants it to a reader. Payloads must
 * own their values or use immutable owning leases. Type erasure cannot prove deep
 * immutability of arbitrary user-defined payloads; mutable aliases are forbidden
 * by the payload contract and must be checked when a concrete schema is added.
 */
class Handoff {
public:
	Handoff() = default;

	template<class T>
	static Handoff Own(T value)
	{
		return Handoff(std::make_shared<const T>(std::move(value)), typeid(T));
	}

	template<class T>
	const T* Get() const
	{
		return type == typeid(T) ? static_cast<const T*>(data.get()) : nullptr;
	}

private:
	Handoff(std::shared_ptr<const void> data, std::type_index type)
		: data(std::move(data)), type(type)
	{
	}

private:
	std::shared_ptr<const void> data;
	std::type_index type {typeid(void)};
};

struct ActivationSnapshot {
public:
	ModeIdentity origin;
	Handoff handoff;
};

enum class LifecycleAction {
	SwitchMode,
	Reload,
	Exit,
};

/** Only completed Session work may issue a normal lifecycle request. */
struct LifecycleRequest {
public:
	LifecycleAction action = LifecycleAction::SwitchMode;
	ModeKind target = ModeKind::Inactive;
	Handoff handoff;
};

template<class T>
struct SessionOutput {
public:
	std::optional<T> snapshot;
	std::optional<LifecycleRequest> lifecycle;
};

struct PublicationIdentity {
public:
	bool operator==(const PublicationIdentity&) const = default;

public:
	ModeIdentity mode;
	Stage stage = Stage::Application;
	Flow flow = Flow::Logical;
	std::uint64_t iteration = 0;
	std::uint64_t revision = 0;
	std::optional<int> tick;
};

namespace detail {

// Type erasure is private infrastructure. Modes see only generated typed views.
struct StoredPublication {
public:
	PublicationIdentity identity;
	std::type_index type {typeid(void)};
	std::shared_ptr<const void> data;
};

using PublicationLease = std::shared_ptr<const StoredPublication>;

struct ReadDescriptor {
public:
	Stage source;
	unsigned slot;
	bool required;
	std::type_index type;
};

struct ResolvedSnapshots {
public:
	InvocationContext invocation;
	std::vector<PublicationLease> publications;
};

} // namespace detail
} // namespace runtime
