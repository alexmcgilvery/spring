/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include "SystemNext/Diagnostics/BoundedTraceBuffer.h"

TEST_CASE("Runtime trace preserves bounded records and invalidates on overflow")
{
	runtime::BoundedTraceBuffer buffer;
	using Mode = runtime::BoundedTraceBuffer::Mode;
	const std::size_t budget = 2 * sizeof(runtime::TraceRecord) + 1;
	REQUIRE(buffer.Configure(Mode::Detailed, budget));
	CHECK(buffer.RetainedBytes() <= budget);
	runtime::TraceRecord record;
	record.values[0] = 42;
	buffer.Append(record);
	record.values[0] = 99;
	buffer.Append(record);
	REQUIRE(buffer.Records().size() == 2);
	CHECK(buffer.Records()[0].values[0] == 42);
	CHECK(buffer.Records()[1].values[0] == 99);
	buffer.Append(record);
	CHECK_FALSE(buffer.Valid());
	CHECK(buffer.Dropped() == 1);
	buffer.Clear();
	buffer.Append(record);
	CHECK(buffer.Records().empty());
	CHECK(buffer.Dropped() == 2);
	CHECK(buffer.Counts()[0] == 4);
	REQUIRE(buffer.Configure(Mode::Detailed, budget));
	CHECK(buffer.Valid());
	CHECK(buffer.Dropped() == 0);
}

TEST_CASE("Runtime summary and off modes retain no record storage")
{
	runtime::BoundedTraceBuffer buffer;
	using Mode = runtime::BoundedTraceBuffer::Mode;
	REQUIRE(buffer.Configure(Mode::Summary, 0));
	for (int i = 0; i < 100; ++i)
		buffer.Append({});
	CHECK(buffer.Counts()[0] == 100);
	CHECK(buffer.Records().empty());
	CHECK(buffer.RetainedBytes() == 0);
	CHECK(buffer.Valid());
	REQUIRE(buffer.Configure(Mode::Off, 0));
	buffer.Append({});
	CHECK(buffer.Counts()[0] == 0);
	CHECK_FALSE(buffer.Configure(Mode::Detailed, sizeof(runtime::TraceRecord) - 1));
	CHECK_FALSE(buffer.Valid());
	CHECK_FALSE(buffer.Enabled());
}
