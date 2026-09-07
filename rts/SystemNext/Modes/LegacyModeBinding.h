/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once
#include "ModeBinding.h"
namespace runtime {
class SelectMenuMode;
class LuaMenuMode;
class PreGameMode;
class LoadingMode;
class GameMode;
class Session;
SelectMenuMode& GetSelectMenuMode();
LuaMenuMode& GetLuaMenuMode();
PreGameMode& GetPreGameMode();
LoadingMode& GetLoadingMode();
GameMode& GetGameMode();
Session& GetRuntimeSession();
Mode* ResolveActiveMode();
class LegacyModeBinding final : public ModeBinding {
public:
	Mode* Resolve() override;
	std::uint64_t Generation() const override;
};
}
