/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TestSupport.h"

#ifdef UNDECLARED_SOURCE
void Invalid(const SnapshotView<TestContracts, Stage::Session>& view) { (void)view.Render(); }
#endif

#ifdef UNDECLARED_DEPTH
void Invalid(const SnapshotView<TestContracts, Stage::Session>& view) { (void)view.Input().Older(); }
#endif

#ifdef CURRENT_DISPLAY
void Invalid(const SnapshotView<TestContracts, Stage::Session>& view) { (void)view.Display().Current(); }
#endif

#ifdef MUTABLE_SNAPSHOT
void Invalid(const SnapshotView<TestContracts, Stage::Session>& view) { view.Input().Current().value = 5; }
#endif

#ifdef INPUT_TRANSITION
void Invalid(std::optional<Value>& output) { output.lifecycle = LifecycleRequest {}; }
#endif

#ifdef FORWARD_CURRENT
struct Invalid : TestContracts {
	using SessionReads = SnapshotReads<Optional<Stage::Display, Slot::Current>>;
};
#endif

#ifdef REQUIRED_VISUAL
struct Invalid : TestContracts {
	using SessionReads = SnapshotReads<Required<Stage::Display, Slot::Previous>>;
};
#endif

#ifdef DUPLICATE_READ
struct Invalid : TestContracts {
	using SessionReads = SnapshotReads<Required<Stage::Input, Slot::Current>, Optional<Stage::Input, Slot::Current>>;
};
#endif

#ifdef MISSING_PAYLOAD
struct Invalid : ModeContracts {
	using SessionReads = SnapshotReads<Optional<Stage::Input, Slot::Previous>>;
};
#endif

#ifdef UNKNOWN_SOURCE
struct Invalid : TestContracts {
	using SessionReads = SnapshotReads<Optional<Stage::Count, Slot::Previous>>;
};
#endif

#ifdef CURRENT_CYCLE
struct Invalid : TestContracts {
	using InputReads = SnapshotReads<Required<Stage::Session, Slot::Current>>;
	using SessionReads = SnapshotReads<Required<Stage::Input, Slot::Current>>;
};
#endif

#if defined(FORWARD_CURRENT) || defined(REQUIRED_VISUAL) || defined(DUPLICATE_READ) || defined(MISSING_PAYLOAD) || defined(UNKNOWN_SOURCE) || defined(CURRENT_CYCLE)
void InvalidRegistration(SnapshotManager& manager) { manager.Register<Invalid>(ModeKind::Game); }
#endif
