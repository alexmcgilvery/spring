/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include <chrono>
#include <filesystem>
#include <sstream>

#include "System/Log/FileSink.h"
#include "System/Log/StreamSink.h"

TEST_CASE("Runtime file-open failure cannot broadcast to other logging sinks")
{
	std::stringstream capture;
	struct StreamScope {
		StreamScope(std::stringstream& stream) { log_sink_stream_setLogStream(&stream); }
		~StreamScope() { log_sink_stream_setLogStream(nullptr); }
	} scope(capture);
	const auto missing = std::filesystem::temp_directory_path() / ("rfc0-missing-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	REQUIRE_FALSE(std::filesystem::exists(missing));
	const auto quiet = (missing / "quiet.jsonl").string();
	log_file_addLogFile(quiet.c_str(), "__runtime__", LOG_LEVEL_NONE, LOG_LEVEL_NONE, false);
	CHECK(log_file_getLogFileStream(quiet.c_str()) == nullptr);
	CHECK(capture.str().empty());
	const auto ordinary = (missing / "ordinary.log").string();
	log_file_addLogFile(ordinary.c_str());
	CHECK(log_file_getLogFileStream(ordinary.c_str()) == nullptr);
	CHECK_FALSE(capture.str().empty());
}
