/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyRuntimeDiagnostics.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <json/json.h>

#include "SystemNext/Diagnostics/JsonLinesTraceWriter.h"
#include "Game/GameVersion.h"
#include "Game/GameSetup.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/FileSink.h"
#include "System/LogOutput.h"
#include "System/Platform/Threading.h"

CONFIG(bool, RuntimePublication).defaultValue(false).description("Enable internal simulation publication validation.");
CONFIG(int, RuntimeDiagnostics).defaultValue(0).minimumValue(0).maximumValue(2).description("Runtime file diagnostics: 0 off, 1 summary, 2 detailed.");
CONFIG(int, RuntimePublicationMaxMiB).defaultValue(256).minimumValue(1).description("Runtime frame, catalog and identity memory limit.");
CONFIG(int, RuntimeEventMaxMiB).defaultValue(16).minimumValue(1).description("Runtime event journal memory limit.");
CONFIG(int, RuntimeTraceMaxMiB).defaultValue(8).minimumValue(1).description("Runtime trace buffer memory limit.");
CONFIG(int, RuntimeLogMaxMiB).defaultValue(256).minimumValue(1).description("Runtime diagnostic file size limit.");
CONFIG(std::string, RuntimeInputId).defaultValue("").description("Accepted demo SHA-256 for offline comparison; empty means comparison is incomplete.");

namespace runtime::legacy {
namespace {
BoundedTraceBuffer buffer;
std::unique_ptr<JsonLinesTraceWriter> writer;
std::string path;
std::uint64_t epoch = 0;
std::uint64_t ordinal = 0;
std::uint64_t scopeCounter = 0;
std::uint64_t currentScope = 0;
std::uint64_t iteration = 0;
std::uint64_t controllerId = 0;
const void* controller = nullptr;
bool sessionActive = false;
bool initialized = false;

std::uint64_t Now() noexcept
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool Enabled() noexcept
{
	return buffer.Enabled() && Threading::IsMainThread();
}

void Record(TraceKind kind, std::array<std::uint64_t, 8> values = {}, Phase phase = Phase::Host) noexcept
{
	if (!Enabled())
		return;
	buffer.Append({kind, phase, Now(), values});
}

std::size_t MiB(const char* key)
{
	const int value = configHandler->GetInt(key);
	if (value <= 0 || std::uint64_t(value) > std::numeric_limits<std::size_t>::max() / (1024 * 1024))
		throw std::overflow_error("invalid runtime memory limit");
	return std::size_t(value) * 1024 * 1024;
}
}

void InitializeDiagnostics() noexcept
{
	if (initialized)
		return;
	initialized = true;
	try {
		const auto mode = static_cast<BoundedTraceBuffer::Mode>(configHandler->GetInt("RuntimeDiagnostics"));
		if (mode == BoundedTraceBuffer::Mode::Off)
			return;
		if (!buffer.Configure(mode, MiB("RuntimeTraceMaxMiB")))
			return;
		const auto runId = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
		path = (std::filesystem::path(logOutput.GetFilePath()).parent_path() / ("vkfun-runtime-" + runId + ".jsonl")).string();
		log_file_addLogFile(path.c_str(), "__vkfun_runtime_file_only__", LOG_LEVEL_NONE, LOG_LEVEL_NONE, false);
		writer = std::make_unique<JsonLinesTraceWriter>(log_file_getLogFileStream(path.c_str()), MiB("RuntimeLogMaxMiB"));
		Json::Value header;
		header["run_id"] = runId;
		header["engine_revision"] = SpringVersion::GetFull();
		header["implementation_baseline"] = "9fbc1c3511274b367eacfe00020e26bddba5bfee";
		header["contract_version"] = 1;
		header["diagnostics"] = mode == BoundedTraceBuffer::Mode::Detailed ? "detailed" : "summary";
		header["input_id"] = configHandler->GetString("RuntimeInputId");
		header["game"] = gameSetup == nullptr ? "" : gameSetup->modName;
		header["map"] = gameSetup == nullptr ? "" : gameSetup->mapName;
		header["publication_enabled"] = configHandler->GetBool("RuntimePublication");
#ifdef HEADLESS
		header["variant"] = "headless";
#else
		header["variant"] = "legacy";
#endif
#ifdef NDEBUG
		header["build_mode"] = "release";
#else
		header["build_mode"] = "debug";
#endif
		if (!writer->Start(std::move(header)))
			buffer.Invalidate();
	} catch (...) {
		buffer.Invalidate();
	}
}

void BeginSession() noexcept
{
	if (!Enabled() || sessionActive)
		return;
	if (epoch == std::numeric_limits<std::uint64_t>::max()) {
		buffer.Invalidate();
		return;
	}
	sessionActive = true;
	ordinal = 0;
	Record(TraceKind::SessionBegin, {++epoch});
}

void EndSession() noexcept
{
	if (!Enabled() || !sessionActive)
		return;
	Record(TraceKind::SessionEnd, {epoch});
	sessionActive = false;
}

void AcceptedMessage(int tick, const std::uint8_t* data, std::size_t length) noexcept
{
	if (!Enabled() || !sessionActive || length == 0)
		return;
	// Supplemental order fingerprint; demos remain the accepted-input recording.
	std::uint64_t hash = 14695981039346656037ULL;
	for (std::size_t i = 0; i < length; ++i)
		hash = (hash ^ data[i]) * 1099511628211ULL;
	Record(TraceKind::Message, {epoch, std::uint64_t(tick), ordinal++, data[0], length, hash});
}

void CompletedFrame(int tick, std::uint32_t checksum, bool hasChecksum) noexcept
{
	if (!sessionActive)
		return;
	Record(TraceKind::Frame, {epoch, std::uint64_t(tick), checksum, hasChecksum, ordinal == 0 ? 0 : ordinal - 1});
}

void ObserveGuard(bool acquire, bool context) noexcept
{
	Record(acquire ? TraceKind::GuardAcquire : TraceKind::GuardRelease, {context});
}

void ObserveController(const void* before, const void* after) noexcept
{
	if (!Enabled())
		return;
	if (controller != before) {
		controller = before;
		++controllerId;
	}
	const auto beforeId = before == nullptr ? 0 : controllerId;
	if (controller != after) {
		controller = after;
		++controllerId;
	}
	Record(TraceKind::Controller, {iteration, beforeId, after == nullptr ? 0 : controllerId});
}

/** All scope callers share nesting state, including engine callback observers. */
PhaseToken BeginPhase(Phase phase) noexcept
{
	PhaseToken token;
	token.phase = phase;
	if (!Enabled())
		return token;
	if (phase == Phase::Host)
		++iteration;
	token.parent = currentScope;
	token.scope = ++scopeCounter;
	currentScope = token.scope;
	token.exceptions = std::uncaught_exceptions();
	Record(TraceKind::PhaseBegin, {token.scope, token.parent, iteration}, phase);
	return token;
}

void EndPhase(PhaseToken token) noexcept
{
	if (token.scope == 0)
		return;
	Record(TraceKind::PhaseEnd, {token.scope, token.parent, iteration, std::uncaught_exceptions() > token.exceptions}, token.phase);
	currentScope = token.parent;
}

PhaseScope::PhaseScope(Phase phase) noexcept: token(BeginPhase(phase))
{}

PhaseScope::~PhaseScope()
{
	EndPhase(token);
}

void DrainDiagnostics() noexcept
{
	if (writer)
		writer->Drain(buffer);
}

void FinishDiagnostics(bool complete) noexcept
{
	EndSession();
	if (writer) {
		writer->Drain(buffer);
		writer->Finish(buffer, complete);
		writer.reset();
	}
	if (!path.empty()) {
		log_file_removeLogFile(path.c_str());
		path.clear();
	}
}

}
