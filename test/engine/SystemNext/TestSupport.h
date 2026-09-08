/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Modes/Mode.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace runtime;

inline void Check(bool condition, const char* message)
{
	if (!condition)
		throw std::runtime_error(message);
}

template<class Function>
void Throws(Function function, const char* message)
{
	bool caught = false;
	try {
		function();
	} catch (const std::exception&) {
		caught = true;
	}
	Check(caught, message);
}

struct Value {
	int value = 0;
	std::shared_ptr<const int> lifetime;
};

struct TestContracts : ModeContracts {
	using InputData = Value;
	using SessionData = Value;
	using SimulationData = Value;
	using DisplayData = Value;

	using InputReads = SnapshotReads<
		Required<Stage::PlatformInput, Slot::Current>,
		Required<Stage::Window, Slot::Current>,
		Required<Stage::Activation, Slot::Current>,
		Optional<Stage::PlatformInput, Slot::Previous>,
		Optional<Stage::Window, Slot::Previous>,
		Optional<Stage::Session, Slot::Previous>
	>;
	using SessionReads = SnapshotReads<
		Required<Stage::Input, Slot::Current>,
		Optional<Stage::Display, Slot::Previous>
	>;
	using DisplayReads = SnapshotReads<
		Required<Stage::Window, Slot::Current>,
		Required<Stage::GraphicsOutput, Slot::Current>,
		Optional<Stage::GraphicsOutput, Slot::Previous>,
		Required<Stage::Session, Slot::Current>,
		Optional<Stage::Display, Slot::Previous>,
		Optional<Stage::Simulation, Slot::Current>,
		Optional<Stage::Simulation, Slot::Previous>,
		Optional<Stage::Present, Slot::Previous>
	>;
	using RenderReads = SnapshotReads<
		Required<Stage::GraphicsOutput, Slot::Current>,
		Required<Stage::Display, Slot::Current>
	>;
};

inline PlatformInputSnapshot TestInput(std::uint64_t sequence = 1)
{
	PlatformInputSnapshot input;
	input.events.push_back({sequence, "test", {}, {}});
	return input;
}

inline WindowSnapshot TestWindow(std::uint64_t generation = 1)
{
	return WindowSnapshot {7, generation, 1280, 720, true, true};
}

inline GraphicsOutputSnapshot TestOutput(std::uint64_t generation = 1)
{
	return GraphicsOutputSnapshot {9, generation, 1280, 720, true};
}

inline LogicalIterationId PublishLogical(SnapshotManager& manager, int value, bool publishSession = true)
{
	auto iteration = manager.BeginLogical(TestInput(), TestWindow());
	const auto id = iteration.LogicalId();
	auto input = manager.BeginStage(id, Stage::Input);
	Check(input.has_value(), "input admitted");
	Check(manager.Publish(*input, Value {value, {}}), "input committed");
	input->Finish(StageStatus::Completed);

	auto session = manager.BeginStage(id, Stage::Session);
	Check(session.has_value(), "session admitted");
	if (publishSession) {
		Check(manager.Publish(*session, Value {value, {}}), "session committed");
		session->Finish(StageStatus::Completed);
	} else {
		session->Finish(StageStatus::NoPublication);
	}
	return id;
}

struct TestCommands : RenderCommands {
	explicit TestCommands(int value) : value(value) {}
	int value;
};

struct TestResource : RenderResource {};

inline void PublishDisplay(SnapshotManager& manager, VisualIterationId id, int value)
{
	auto display = manager.BeginStage(id, Stage::Display);
	Check(display.has_value(), "display admitted");
	Check(manager.Publish(*display, Value {value, {}}), "display committed");
	display->Finish(StageStatus::Completed);
}
