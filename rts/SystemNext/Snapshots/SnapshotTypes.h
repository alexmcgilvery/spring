/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../Application/Lifecycle/LifecycleRequest.h"
#include "InvocationMetadata.h"

#include <memory>
#include <optional>
#include <typeindex>
#include <vector>

namespace runtime {

/** PlatformInput, Window, GraphicsOutput and Activation are publication sources. */
enum class Stage {
	PlatformInput,
	Window,
	GraphicsOutput,
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

struct ActivationSnapshot {
public:
	ModeIdentity origin;
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
	Stage stage = Stage::PlatformInput;
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
	InvocationMetadata invocation;
	std::vector<PublicationLease> publications;
};

} // namespace detail
} // namespace runtime
