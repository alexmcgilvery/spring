/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "JsonLinesTraceWriter.h"

#include <string>
#include <json/json.h>
#include <json/writer.h>

namespace runtime {
namespace {
const char* PhaseName(Phase phase)
{
	constexpr const char* names[] = {"host", "input", "save", "reload", "update", "presentation", "draw", "capture", "swap", "simulation"};
	const auto index = static_cast<std::size_t>(phase);
	return index < std::size(names) ? names[index] : "invalid";
}

Json::Value Encode(const TraceRecord& record)
{
	Json::Value out(Json::objectValue);
	const auto& v = record.values;
	out["time_ns"] = Json::UInt64(record.time);
	out["thread"] = "main";
	switch (record.kind) {
		case TraceKind::PhaseBegin:
		case TraceKind::PhaseEnd:
			out["type"] = record.kind == TraceKind::PhaseBegin ? "phase_begin" : "phase_end";
			out["phase"] = PhaseName(record.phase);
			out["scope"] = Json::UInt64(v[0]);
			out["parent"] = v[1] == 0 ? Json::Value() : Json::Value(Json::UInt64(v[1]));
			out["iteration"] = Json::UInt64(v[2]);
			out["unwinding"] = v[3] != 0;
			break;
		case TraceKind::GuardAcquire:
		case TraceKind::GuardRelease:
			out["type"] = "load_guard";
			out["action"] = record.kind == TraceKind::GuardAcquire ? "acquire" : "release";
			out["mode"] = v[0] ? "context" : "noop";
			break;
		case TraceKind::Controller:
			out["type"] = "controller";
			out["iteration"] = Json::UInt64(v[0]);
			out["before"] = v[1] == 0 ? Json::Value() : Json::Value(Json::UInt64(v[1]));
			out["after"] = v[2] == 0 ? Json::Value() : Json::Value(Json::UInt64(v[2]));
			break;
		case TraceKind::SessionBegin:
		case TraceKind::SessionEnd:
			out["type"] = "session";
			out["action"] = record.kind == TraceKind::SessionBegin ? "bootstrap" : "end";
			out["epoch"] = Json::UInt64(v[0]);
			break;
		case TraceKind::Message:
			out["type"] = "message";
			out["epoch"] = Json::UInt64(v[0]);
			out["tick"] = Json::Int64(v[1]);
			out["ordinal"] = Json::UInt64(v[2]);
			out["code"] = Json::UInt64(v[3]);
			out["length"] = Json::UInt64(v[4]);
			out["digest"] = std::to_string(v[5]);
			break;
		case TraceKind::Frame:
			out["type"] = "frame";
			out["epoch"] = Json::UInt64(v[0]);
			out["tick"] = Json::Int64(v[1]);
			out["checksum"] = v[3] ? Json::Value(Json::UInt64(v[2])) : Json::Value();
			out["ordinal"] = Json::UInt64(v[4]);
			break;
		case TraceKind::Publication:
			out["type"] = "publication";
			out["epoch"] = Json::UInt64(v[0]);
			out["tick"] = Json::Int64(v[1]);
			out["revision"] = Json::UInt64(v[2]);
			out["first_event"] = Json::UInt64(v[3]);
			out["end_event"] = Json::UInt64(v[4]);
			out["valid"] = v[5] != 0;
			out["retained_bytes"] = Json::UInt64(v[6]);
			out["cost_ns"] = Json::UInt64(v[7]);
			break;
		default:
			out["type"] = "incomplete";
			out["reason"] = "invalid buffered record kind";
	}
	return out;
}
}

bool JsonLinesTraceWriter::Write(Json::Value record, bool terminal) noexcept
{
	if (stream == nullptr || ended)
		return false;
	try {
		record["schema"] = 1;
		record["seq"] = Json::UInt64(sequence);
		Json::StreamWriterBuilder builder;
		builder["indentation"] = "";
		const std::string line = Json::writeString(builder, record) + '\n';
		const std::size_t reserve = terminal ? 0 : 4096;
		if (bytes > byteLimit || reserve > byteLimit - bytes || line.size() > byteLimit - bytes - reserve) {
			failed = true;
			return false;
		}
		const auto written = std::fwrite(line.data(), 1, line.size(), stream);
		bytes += written;
		if (written != line.size()) {
			failed = true;
			return false;
		}
		++sequence;
		return true;
	} catch (...) {
		failed = true;
		return false;
	}
}

bool JsonLinesTraceWriter::Start(Json::Value header) noexcept
{
	try {
		header["type"] = "run_start";
		return Write(std::move(header));
	} catch (...) {
		failed = true;
		return false;
	}
}

bool JsonLinesTraceWriter::Drain(BoundedTraceBuffer& buffer) noexcept
{
	try {
		if (!failed) {
			for (const auto& record: buffer.Records()) {
				if (!Write(Encode(record)))
					break;
			}
		}
	} catch (...) {
		failed = true;
	}
	buffer.Clear();
	if (failed)
		buffer.Invalidate();
	return !failed;
}

bool JsonLinesTraceWriter::Finish(const BoundedTraceBuffer& buffer, bool complete) noexcept
{
	try {
		Json::Value summary(Json::objectValue);
		summary["type"] = "summary";
		summary["observation_valid"] = buffer.Valid();
		summary["dropped"] = Json::UInt64(buffer.Dropped());
		for (const auto count: buffer.Counts())
			summary["counts"].append(Json::UInt64(count));
		Write(std::move(summary), true);
		if (stream == nullptr || std::fflush(stream) != 0)
			failed = true;
		Json::Value end(Json::objectValue);
		end["type"] = "run_end";
		end["complete"] = complete && !failed;
		end["observation_valid"] = buffer.Valid();
		end["dropped"] = Json::UInt64(buffer.Dropped());
		end["io_failed"] = failed;
		Write(std::move(end), true);
		if (stream == nullptr || std::fflush(stream) != 0)
			failed = true;
	} catch (...) {
		failed = true;
	}
	ended = true;
	return !failed;
}

}
