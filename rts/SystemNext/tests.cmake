# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

set(runtime_test_dir "${CMAKE_SOURCE_DIR}/test/engine/SystemNext")
set(runtime_dir "${CMAKE_SOURCE_DIR}/rts/SystemNext")
set(runtime_flags "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")

add_spring_test(RuntimeCoordinator
	"${runtime_test_dir}/testApplicationLoop.cpp;${runtime_dir}/ApplicationLoop.cpp;${runtime_dir}/Session/SessionUpdate.cpp;${runtime_dir}/Session/Session.cpp;${runtime_dir}/Modes/Mode.cpp;${runtime_dir}/Modes/ModeBinding.cpp;${runtime_dir}/Modes/FlowResult.cpp;${runtime_dir}/Presentation/ModeFrame.cpp;${runtime_dir}/Presentation/SerialVisualFrame.cpp;${runtime_dir}/Diagnostics/LoopPhaseScope.cpp;${test_Catch_main}"
	"" "${runtime_flags}")
add_spring_test(RuntimeTrace
	"${runtime_test_dir}/Diagnostics/testBoundedTraceBuffer.cpp;${runtime_dir}/Diagnostics/BoundedTraceBuffer.cpp;${test_Catch_main}"
	"" "${runtime_flags}")
add_spring_test(RuntimeJsonLines
	"${runtime_test_dir}/Diagnostics/testJsonLinesTraceWriter.cpp;${runtime_dir}/Diagnostics/JsonLinesTraceWriter.cpp;${runtime_dir}/Diagnostics/BoundedTraceBuffer.cpp;${test_Catch_main}"
	"prd::jsoncpp" "${runtime_flags}")
add_spring_test(RuntimeFileSink
	"${runtime_test_dir}/Diagnostics/testLegacyFileSink.cpp;${runtime_dir}/Diagnostics/RuntimeLogFile.cpp;${CMAKE_SOURCE_DIR}/rts/System/Log/FileSink.cpp;${test_Common_sources}"
	"nowide::nowide" "${runtime_flags}")
add_spring_test(RuntimePublication
	"${runtime_test_dir}/Simulation/Publication/testSimFramePublicationStore.cpp;${runtime_dir}/Simulation/Publication/SimFramePublicationStore.cpp;${runtime_dir}/Simulation/Publication/PublicationMemoryBudget.cpp;${runtime_dir}/Simulation/Publication/PublishedSimFrame.cpp;${test_Catch_main}"
	"" "${runtime_flags}")
add_spring_test(RuntimeIdentity
	"${runtime_test_dir}/Simulation/Observation/testEntityIdentityRegistry.cpp;${runtime_dir}/Simulation/Observation/EntityIdentityRegistry.cpp;${runtime_dir}/Simulation/Publication/PublicationMemoryBudget.cpp;${test_Catch_main}"
	"" "${runtime_flags}")
add_spring_test(RuntimeEventJournal
	"${runtime_test_dir}/Simulation/Observation/testSimulationEventJournal.cpp;${runtime_dir}/Simulation/Observation/SimulationEventJournal.cpp;${runtime_dir}/Simulation/Observation/SimulationNotifications.cpp;${runtime_dir}/Simulation/Publication/PublicationMemoryBudget.cpp;${runtime_dir}/Simulation/Publication/PublishedSimFrame.cpp;${test_Catch_main}"
	"" "${runtime_flags}")

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME testRuntimeContracts
	COMMAND ${Python3_EXECUTABLE} ${runtime_dir}/tools/check_runtime_contracts.py)
add_test(NAME testRuntimeTools
	COMMAND ${Python3_EXECUTABLE} -B ${runtime_dir}/tools/test_runtime_tools.py)

add_test(NAME testRuntimeLegacySurface
	COMMAND ${Python3_EXECUTABLE} ${runtime_dir}/tools/check_legacy_surface.py)
