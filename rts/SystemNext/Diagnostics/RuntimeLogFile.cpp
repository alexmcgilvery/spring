/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "RuntimeLogFile.h"
#include <nowide/cstdio.hpp>
namespace runtime {
RuntimeLogFile::~RuntimeLogFile() { Close(); }
bool RuntimeLogFile::Open(const char* path)
{
	if (!Close())
		return false;
	stream = nowide::fopen(path, "wb");
	return stream != nullptr;
}
bool RuntimeLogFile::Close() noexcept
{
	FILE* previous = stream;
	stream = nullptr;
	return previous == nullptr || std::fclose(previous) == 0;
}
}
