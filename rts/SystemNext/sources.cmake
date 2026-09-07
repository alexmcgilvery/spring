# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# Source list, not a variant-independent object library: adapters inherit each
# engine executable's legacy/headless definitions.
set(sources_systemnext
	${CMAKE_CURRENT_LIST_DIR}/ApplicationLoop.cpp
	${CMAKE_CURRENT_LIST_DIR}/Session/SessionUpdate.cpp
	${CMAKE_CURRENT_LIST_DIR}/Session/LegacySession.cpp
	${CMAKE_CURRENT_LIST_DIR}/Presentation/SerialVisualFrame.cpp
	${CMAKE_CURRENT_LIST_DIR}/Presentation/LegacyVisualFrame.cpp
	${CMAKE_CURRENT_LIST_DIR}/Lifecycle/LegacyLoopServices.cpp
	${CMAKE_CURRENT_LIST_DIR}/Diagnostics/LoopPhaseScope.cpp
	${CMAKE_CURRENT_LIST_DIR}/Diagnostics/BoundedTraceBuffer.cpp
	${CMAKE_CURRENT_LIST_DIR}/Diagnostics/JsonLinesTraceWriter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Diagnostics/LegacyRuntimeDiagnostics.cpp
)
