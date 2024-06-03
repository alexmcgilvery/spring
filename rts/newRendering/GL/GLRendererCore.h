
#include "newRendering/GlobalRendering.h"
#include "System/type2.h"

typedef void* SDL_GLContext;

class CGLRendererCore final : CGlobalRendering
{
public:
	CGLRendererCore();
	~CGLRendererCore();

	void RendererPreWindowInit() override;
	void RendererPostWindowInit() override;
	bool RendererCreateWindow(const char* title) override;
	SDL_Window* RendererCreateSDLWindow(const char* title)override;
	void RendererSetStartState() override;
	void RendererDestroyWindow() override;
	void RendererUpdateWindow() override;
	void RendererPresentFrame(bool allowSwapBuffers, bool clearErrors) override;

public:
	void UpdateViewport() override;
	void SetTimeStamp(uint32_t queryIdx) const override;
	uint64_t CalculateFrameTimeDelta(uint32_t queryIdx0, uint32_t queryIdx1) const override;

	bool ToggleDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const override;

	// Only one thread can be bound to OpenGL context
	void AquireThreadContext() override;
	void ReleaseThreadContext() override;

public:
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