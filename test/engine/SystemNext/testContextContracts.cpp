/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TestSupport.h"
#include "Modes/SelectMenu/SelectMenuMode.h"
#include "Modes/LuaMenu/LuaMenuMode.h"
#include "Modes/PreGame/PreGameMode.h"
#include "Modes/Loading/LoadingMode.h"
#include "Modes/Game/GameMode.h"

#include <type_traits>

using SessionView = SnapshotView<TestContracts, Stage::Session>;
using InputView = SnapshotView<TestContracts, Stage::Input>;

static_assert(std::is_same_v<decltype(std::declval<const SessionView&>().Input().Current()), const Value&>);
static_assert(std::is_same_v<decltype(std::declval<const SessionView&>().Display().Previous()), const Value*>);
static_assert(!std::is_convertible_v<LogicalIterationId, VisualIterationId>);
static_assert(std::is_same_v<SnapshotsFor<GameMode, Stage::Session>, GameMode::SessionSnapshots>);
static_assert(std::is_base_of_v<IMode, SelectMenuMode>);
static_assert(std::is_base_of_v<IMode, LuaMenuMode>);
static_assert(std::is_base_of_v<IMode, PreGameMode>);
static_assert(std::is_base_of_v<IMode, LoadingMode>);
static_assert(std::is_base_of_v<IMode, GameMode>);
static_assert(SelectMenuContracts::DisplayReads::size == 0);
static_assert(PreGameContracts::DisplayReads::size == 0);

template<class View>
concept HasDisplayCurrent = requires(const View& view) { view.Display().Current(); };

template<class View>
concept HasUndeclaredRender = requires(const View& view) { view.Render(); };

static_assert(!HasDisplayCurrent<SessionView>);
static_assert(!HasUndeclaredRender<SessionView>);
