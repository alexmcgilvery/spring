#pragma once

#include "Rendering/GlobalRendering.h"
#include "System/type2.h"

typedef void* SDL_GLContext;

class CGLRendererCore : CGlobalRendering
{
public:
	CGLRendererCore();
	~CGLRendererCore();

	void RendererPreWindowInit();
	void RendererPostWindowInit();
	bool RendererCreateWindow(const char* title);
	SDL_Window* RendererCreateSDLWindow(const char* title);
	void RendererSetStartState();
	void RendererDestroyWindow();
	void RendererUpdateWindow();
	void RendererPresentFrame(bool allowSwapBuffers, bool clearErrors);

	void UpdateViewport();
	void SetTimeStamp(uint32_t queryIdx) const;
	uint64_t CalculateFrameTimeDelta(uint32_t queryIdx0, uint32_t queryIdx1) const;

	bool ToggleDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const;

	// Only one thread can be bound to OpenGL context
	void AquireThreadContext();
	void ReleaseThreadContext();

	static void InitStatic();
	static void KillStatic();

private:

	SDL_GLContext CreateGLContext(const int2& minCtx);
	SDL_GLContext GetGLContext() { return glContext; }

	void CheckGLExtensions() const;
	void SetGLSupportFlags();
	void QueryGLVersionInfo(char (&sdlVersionStr)[64], char (&glVidMemStr)[64]);
	void QueryGLMaxVals();
	void LogGLVersionInfo(const char* sdlVersionStr, const char* glVidMemStr) const;

	void UpdateDualViewport();

	bool CheckGLMultiSampling() const;
	bool CheckGLContextVersion(const int2& minCtx) const;
	
	void ToggleMultisampling() const;

	bool CheckShaderGL4() const;

private:
	SDL_GLContext glContext;

	int forceDisableGL4;
	int forceCoreContext;

public:
	static constexpr uint32_t NUM_OPENGL_TIMER_QUERIES = 8;
	static constexpr uint32_t FRAME_REF_TIME_QUERY_IDX = 0;
	static constexpr uint32_t FRAME_END_TIME_QUERY_IDX = NUM_OPENGL_TIMER_QUERIES - 1;

private:
	// double-buffered; results from frame N become available on frame N+1
	std::array<uint32_t, NUM_OPENGL_TIMER_QUERIES * 2> glTimerQueries;

};

extern CGLRendererCore* glRenderer;