/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include <cstdio>
#include <memory>
#include <string>
#include <json/json.h>

#include "SystemNext/Diagnostics/JsonLinesTraceWriter.h"

TEST_CASE("JSON Lines diagnostics preserve sequence and reserve incomplete termination")
{
	std::unique_ptr<FILE, decltype(&std::fclose)> file(std::tmpfile(), &std::fclose);
	REQUIRE(file);
	runtime::BoundedTraceBuffer buffer;
	REQUIRE(buffer.Configure(runtime::BoundedTraceBuffer::Mode::Detailed, 4096));
	Json::Value header;
	header["run_id"] = "test";
	header["engine_revision"] = "test-revision";
	header["diagnostics"] = "detailed";
	std::size_t limit = 8192;
	SECTION("normal complete capture") {
		runtime::JsonLinesTraceWriter writer(file.get(), limit);
		REQUIRE(writer.Start(header));
		runtime::TraceRecord record;
		record.kind = runtime::TraceKind::SessionBegin;
		record.values[0] = 1;
		buffer.Append(record);
		record.kind = runtime::TraceKind::SessionEnd;
		buffer.Append(record);
		REQUIRE(writer.Drain(buffer));
		REQUIRE(writer.Finish(buffer, true));
		CHECK(writer.Bytes() <= limit);
		std::rewind(file.get());
		char line[4096];
		unsigned sequence = 0;
		Json::Value last;
		while (std::fgets(line, sizeof(line), file.get()) != nullptr) {
			Json::Reader reader;
			REQUIRE(reader.parse(line, last));
			CHECK(last["seq"].asUInt() == sequence++);
		}
		CHECK(sequence == 5);
		CHECK(last["type"].asString() == "run_end");
		CHECK(last["complete"].asBool());
	}
	SECTION("limit produces explicit incomplete footer") {
		runtime::JsonLinesTraceWriter writer(file.get(), 4200);
		CHECK_FALSE(writer.Start(header));
		CHECK_FALSE(writer.Finish(buffer, true));
		CHECK(writer.Bytes() <= 4200);
		std::rewind(file.get());
		char line[4096];
		Json::Value last;
		while (std::fgets(line, sizeof(line), file.get()) != nullptr) {
			Json::Reader reader;
			REQUIRE(reader.parse(line, last));
		}
		CHECK(last["type"].asString() == "run_end");
		CHECK_FALSE(last["complete"].asBool());
	}
	SECTION("absent output cannot pass") {
		runtime::JsonLinesTraceWriter writer(nullptr, limit);
		CHECK_FALSE(writer.Start(header));
		CHECK_FALSE(writer.Finish(buffer, true));
		CHECK(writer.Failed());
	}
}
