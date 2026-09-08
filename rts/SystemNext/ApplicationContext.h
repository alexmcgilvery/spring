/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {

// Forward declarations: application-owned resources and activation.
class ApplicationHost;
class SnapshotManager;
class VisualOutput;
struct ActiveModeBinding;

/** Borrowed ownership boundaries. Headless execution supplies no VisualOutput. */
struct ApplicationContext {
public:
	ApplicationHost& host;
	SnapshotManager& snapshots;
	ActiveModeBinding& activeMode;
	VisualOutput* visualOutput = nullptr;
};

} // namespace runtime
