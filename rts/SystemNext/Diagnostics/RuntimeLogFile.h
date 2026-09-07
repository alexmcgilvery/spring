/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once
#include <cstdio>
namespace runtime {
/**
 * Own the diagnostic stream without registering it with console/Lua log sinks.
 * Opening failures return false and never broadcast. The host supplies a path
 * in the existing write directory; this owner closes after the writer finishes.
 * Separate ownership avoids changing the legacy file-sink API or behavior.
 */
class RuntimeLogFile {
public:
	RuntimeLogFile() = default;
	~RuntimeLogFile();
	RuntimeLogFile(const RuntimeLogFile&) = delete;
	RuntimeLogFile& operator=(const RuntimeLogFile&) = delete;
	bool Open(const char* path);
	bool Close() noexcept;
	FILE* Stream() const { return stream; }
private:
	FILE* stream = nullptr;
};
}
