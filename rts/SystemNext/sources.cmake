# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# Source list, not a variant-independent object library: adapters inherit each
# engine executable's legacy/headless definitions.
set(sources_systemnext
	${CMAKE_CURRENT_LIST_DIR}/ApplicationLoop.cpp
	${CMAKE_CURRENT_LIST_DIR}/Session/SessionUpdate.cpp
	${CMAKE_CURRENT_LIST_DIR}/Session/Session.cpp
	${CMAKE_CURRENT_LIST_DIR}/Presentation/SerialVisualFrame.cpp
	${CMAKE_CURRENT_LIST_DIR}/Presentation/LegacyVisualFrame.cpp
	${CMAKE_CURRENT_LIST_DIR}/Diagnostics/LoopPhaseScope.cpp
	${CMAKE_CURRENT_LIST_DIR}/Diagnostics/BoundedTraceBuffer.cpp
	${CMAKE_CURRENT_LIST_DIR}/Diagnostics/RuntimeLogFile.cpp
	${CMAKE_CURRENT_LIST_DIR}/Diagnostics/JsonLinesTraceWriter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Diagnostics/LegacyRuntimeDiagnostics.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/Mode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/ModeBinding.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/FlowResult.cpp
	${CMAKE_CURRENT_LIST_DIR}/Presentation/ModeFrame.cpp
	${CMAKE_CURRENT_LIST_DIR}/Presentation/LegacyPresent.cpp
)

# FIXME ADAPTER-BACKING-ACCESS: Retained implementation copies below require
# private controller fields, removed lifetime APIs, or private host access.
# They are NOT compiled into production and are NOT validated adapters. Supply
# owned backing state/public interfaces in SystemNext before enabling them;
# do not modify legacy bodies or add friend access to make this list compile.
set(systemnext_unavailable_adapter_sources
	${CMAKE_CURRENT_LIST_DIR}/Lifecycle/LegacyLoopServices.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/LegacyModeBinding.cpp
	${CMAKE_CURRENT_LIST_DIR}/Simulation/Simulation.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/SelectMenu/SelectMenuMode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/LuaMenu/LuaMenuMode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/PreGame/PreGameMode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/PreGame/PreGameModeSetup.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/Loading/LoadingMode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/Game/GameMode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Modes/Game/GameModeInput.cpp
)
