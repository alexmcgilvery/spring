#include "GLRendererCore.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/GL/StreamBuffer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/UniformConstants.h"
#include "Rendering/Fonts/glFont.h"
#include "System/bitops.h"
#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/WindowManagerHelper.h"

#include <SDL.h>

CONFIG(bool, DebugGL).defaultValue(false).description("Enables GL debug-context and output. (see GL_ARB_debug_output)");
CONFIG(bool, DebugGLStacktraces).defaultValue(false).description("Create a stacktrace when an OpenGL error occurs");

CONFIG(int, GLContextMajorVersion).defaultValue(3).minimumValue(3).maximumValue(4);
CONFIG(int, GLContextMinorVersion).defaultValue(0).minimumValue(0).maximumValue(5);

CONFIG(int, ForceDisableGL4).defaultValue(0).safemodeValue(1).minimumValue(0).maximumValue(1);
CONFIG(int, AtiHacks).defaultValue(-1).headlessValue(0).minimumValue(-1).maximumValue(1).description("Enables graphics drivers workarounds for users with AMD proprietary drivers.\n -1:=runtime detect, 0:=off, 1:=on");

/*
CR_REG_METADATA(CGLRendererCore, (
	CR_IGNORED(msaaLevel),
	CR_MEMBER(glDebug),
	CR_MEMBER(glDebugErrors),

	CR_IGNORED(forceDisableGL4),
	
	CR_IGNORED(haveAMD),
	CR_IGNORED(haveMesa),
	CR_IGNORED(haveIntel),
	CR_IGNORED(haveNvidia),

	CR_IGNORED(amdHacks),

	CR_IGNORED(compressTextures),

	CR_IGNORED(haveGL4),

	CR_IGNORED(glContext),

	CR_IGNORED(supportPersistentMapping),
	CR_IGNORED(supportExplicitAttribLoc),
	CR_IGNORED(supportNonPowerOfTwoTex),
	CR_IGNORED(supportTextureQueryLOD),
	CR_IGNORED(supportMSAAFrameBuffer),
	CR_IGNORED(supportDepthBufferBitDepth),
	CR_IGNORED(supportRestartPrimitive),
	CR_IGNORED(supportClipSpaceControl),
	CR_IGNORED(supportSeamlessCubeMaps),
	CR_IGNORED(supportFragDepthLayout),
	CR_IGNORED(haveGLSL),
	CR_IGNORED(glslMaxVaryings),
	CR_IGNORED(glslMaxAttributes),
	CR_IGNORED(glslMaxDrawBuffers),
	CR_IGNORED(glslMaxRecommendedIndices),
	CR_IGNORED(glslMaxRecommendedVertices),
	CR_IGNORED(glslMaxUniformBufferBindings),
	CR_IGNORED(glslMaxUniformBufferSize),
	CR_IGNORED(glslMaxStorageBufferBindings),
	CR_IGNORED(glslMaxStorageBufferSize),
	CR_IGNORED(dualScreenMode),
	CR_IGNORED(dualScreenMiniMapOnLeft),

	CR_IGNORED(glTimerQueries)
)
*/
CGLRendererCore* glRenderer;

CGLRendererCore::CGLRendererCore()
	: forceDisableGL4(configHandler->GetInt("ForceDisableGL4"))
	, forceCoreContext(configHandler->GetInt("ForceCoreContext"))

	, glContext{nullptr}

	, glTimerQueries{0}
{
	rendererDebug = configHandler->GetBool("DebugGL");
	rendererDebugErrors = false;
}

CGLRendererCore::~CGLRendererCore()
{
	// protect against aborted startup
	if (glContext) {
		glDeleteQueries(glTimerQueries.size(), glTimerQueries.data());
	}
	RendererDestroyWindow();
}

void CGLRendererCore::InitStatic()
{ 
	alignas(CGLRendererCore) static std::byte globalRenderingMem[sizeof(CGLRendererCore)];
	glRenderer = new (globalRenderingMem) CGLRendererCore(); 
	globalRendering = (IGlobalRendering*) glRenderer;
}

void CGLRendererCore::KillStatic()
{ 
	globalRendering->PreKill();
	spring::SafeDestruct(glRenderer);
}

void CGLRendererCore::RendererPreWindowInit()
{

}

void CGLRendererCore::RendererPostWindowInit()
{
	#ifndef HEADLESS
		glewExperimental = true;
	#endif

	glewInit();
	// glewInit sets GL_INVALID_ENUM, get rid of it
	glGetError();

	char sdlVersionStr[64] = "";
	char glVidMemStr[64] = "unknown";

	QueryGLVersionInfo(sdlVersionStr, glVidMemStr);
	CheckGLExtensions();
	SetGLSupportFlags();
	QueryGLMaxVals();

	LogGLVersionInfo(sdlVersionStr, glVidMemStr);
	ToggleDebugOutput(0, 0, 0);

	UniformConstants::GetInstance().Init(); //FIXME Convert to Interface
	ModelUniformData::Init(); //FIXME Convert to Interface
	glGenQueries(glTimerQueries.size(), glTimerQueries.data());
	RenderBuffer::InitStatic();//FIXME Convert to Interface
}

void CGLRendererCore::RendererSetStartState()
{
	LOG("[GR::%s]", __func__);

	glShadeModel(GL_SMOOTH);

	glClearDepth(1.0f);
	glDepthRange(0.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	#ifdef GLEW_ARB_clip_control
	// avoid precision loss with default DR transform
	if (supportClipSpaceControl)
		glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	#endif

	#ifdef GLEW_ARB_seamless_cube_map
	if (supportSeamlessCubeMaps)
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	#endif

	// MSAA rasterization
	msaaLevel *= CheckGLMultiSampling();
	ToggleMultisampling();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	UpdateViewport();
	gluPerspective(45.0f, aspectRatio, minViewRange, maxViewRange);

	// this does not accomplish much
	// SwapBuffers(true, true);
}

bool CGLRendererCore::RendererCreateWindow(const char* title)
{
	if (!CheckAvailableVideoModes()) {
		handleerror(nullptr, "desktop color-depth should be at least 24 bits per pixel, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	// should be set to "3.0" (non-core Mesa is stuck there), see below
	const char* mesaGL = getenv("MESA_GL_VERSION_OVERRIDE");
	const char* softGL = getenv("LIBGL_ALWAYS_SOFTWARE");

	// get wanted resolution and context-version
	const int2 minCtx = (mesaGL != nullptr && std::strlen(mesaGL) >= 3)?
		int2{                  std::max(mesaGL[0] - '0', 3),                   std::max(mesaGL[2] - '0', 0)}:
		int2{configHandler->GetInt("GLContextMajorVersion"), configHandler->GetInt("GLContextMinorVersion")};

	// start with the standard (R8G8B8A8 + 24-bit depth + 8-bit stencil + DB) format
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// create GL debug-context if wanted (more verbose GL messages, but runs slower)
	// note:
	//   requesting a core profile explicitly is needed to get versions later than
	//   3.0/1.30 for Mesa, other drivers return their *maximum* supported context
	//   in compat and do not make 3.0 itself available in core (though this still
	//   suffices for most of Spring)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, forceCoreContext? SDL_GL_CONTEXT_PROFILE_CORE: SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, minCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minCtx.y);


	if (msaaLevel > 0) {
		if (softGL != nullptr)
			LOG_L(L_WARNING, "MSAALevel > 0 and LIBGL_ALWAYS_SOFTWARE set, this will very likely crash!");

		make_even_number(msaaLevel);
	}

	if ((sdlWindow = CreateSDLWindow(title)) == nullptr)
	return false;

	if (configHandler->GetInt("MinimizeOnFocusLoss") == 0)
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

	SetWindowAttributes(sdlWindow);

	#if !defined(HEADLESS)
		// disable desktop compositing to fix tearing
		// (happens at 300fps, neither fullscreen nor vsync fixes it, so disable compositing)
		// On Windows Aero often uses vsync, and so when Spring runs windowed it will run with
		// vsync too, resulting in bad performance.
		if (configHandler->GetBool("BlockCompositing"))
			WindowManagerHelper::BlockCompositing(sdlWindow);
	#endif

	if ((glContext = CreateGLContext(minCtx)) == nullptr)
		return false;

	if (!CheckGLContextVersion(minCtx)) {
		handleerror(nullptr, "minimum required OpenGL version not supported, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	AquireThreadContext();

	return true;
}

SDL_Window* CGLRendererCore::RendererCreateSDLWindow(const char* title)
{

	SDL_Window* newWindow = nullptr;

	const int msaaLevel = msaaLevel;
	
	const std::array aaLvls = {msaaLevel, msaaLevel / 2, msaaLevel / 4, msaaLevel / 8, msaaLevel / 16, msaaLevel / 32, 0};
	const std::array zbBits = {24, 32, 16};

	const char* wpfName = "";

	const char* frmts[2] = {
		"[GR::%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer for main window",
		"[GR::%s] using %dx anti-aliasing and %d-bit depth-buffer (PF=\"%s\") for main window",
	};

	bool borderless_ = configHandler->GetBool("WindowBorderless");
	bool fullScreen_ = configHandler->GetBool("Fullscreen");
	int winPosX_ = configHandler->GetInt("WindowPosX");
	int winPosY_ = configHandler->GetInt("WindowPosY");
	int2 newRes = GetCfgWinRes();

	// note:
	//   passing the minimized-flag is useless (state is not saved if minimized)
	//   and has no effect anyway, setting a minimum size for a window overrides
	//   it while disabling the SetWindowMinimumSize call still results in a 1x1
	//   window on the desktop
	//
	//   SDL_WINDOW_FULLSCREEN, for "real" fullscreen with a videomode change;
	//   SDL_WINDOW_FULLSCREEN_DESKTOP for "fake" fullscreen that takes the size of the desktop;
	//   and 0 for windowed mode.

	uint32_t sdlFlags  = (borderless_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) * fullScreen_;
	         sdlFlags |= (SDL_WINDOW_BORDERLESS * borderless_);

	sdlFlags |= (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	for (size_t i = 0; i < (aaLvls.size()) && (newWindow == nullptr); i++) {
		if (i > 0 && aaLvls[i] == aaLvls[i - 1])
			break;

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, aaLvls[i] > 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, aaLvls[i]    );

		for (size_t j = 0; j < (zbBits.size()) && (newWindow == nullptr); j++) {
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbBits[j]);

			if ((newWindow = SDL_CreateWindow(title, winPosX_, winPosY_, newRes.x, newRes.y, sdlFlags)) == nullptr) {
				LOG_L(L_WARNING, frmts[0], __func__, SDL_GetError(), aaLvls[i], zbBits[j]);
				continue;
			}

			LOG(frmts[1], __func__, aaLvls[i], zbBits[j], wpfName = SDL_GetPixelFormatName(SDL_GetWindowPixelFormat(newWindow)));
		}
	}

	return newWindow;
}

void CGLRendererCore::RendererDestroyWindow()
{
	SDL_GL_MakeCurrent(sdlWindow, nullptr);

	#if !defined(HEADLESS)
	if (glContext)
		SDL_GL_DeleteContext(glContext);
	#endif
}

void CGLRendererCore::RendererUpdateWindow()
{
	AquireThreadContext();
}

void CGLRendererCore::RendererPresentFrame(bool allowSwapBuffers, bool clearErrors)
{
	spring_time pre;
	{
		SCOPED_TIMER("Misc::SwapBuffers");
		assert(sdlWindow);

		// silently or verbosely clear queue at the end of every frame
		if (clearErrors || rendererDebugErrors)
			glClearErrors("GR", __func__, rendererDebugErrors);

		if (!allowSwapBuffers && !forceSwapBuffers)
			return;

		pre = spring_now();

		RenderBuffer::SwapRenderBuffers(); //all RBs are swapped here
		IStreamBufferConcept::PutBufferLocks();

		//https://stackoverflow.com/questions/68480028/supporting-opengl-screen-capture-by-third-party-applications
		glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0);

		SDL_GL_SwapWindow(sdlWindow);
		FrameMark;
	}
	// exclude debug from SCOPED_TIMER("Misc::SwapBuffers");
	eventHandler.DbgTimingInfo(TIMING_SWAP, pre, spring_now());
	lastSwapBuffersEnd = spring_now();
}



void CGLRendererCore::AquireThreadContext() // equivalent to prior MakeCurrentContext(false)
{
	SDL_GL_MakeCurrent(sdlWindow, glContext);
}

void CGLRendererCore::ReleaseThreadContext() // equivalent to prior MakeCurrentContext(true)
{
	SDL_GL_MakeCurrent(sdlWindow, nullptr);
}

void CGLRendererCore::SetTimeStamp(uint32_t queryIdx) const
{
	if (!GLEW_ARB_timer_query)
		return;

	glQueryCounter(glTimerQueries[(NUM_OPENGL_TIMER_QUERIES * (drawFrame & 1)) + queryIdx], GL_TIMESTAMP);
}

uint64_t CGLRendererCore::CalculateFrameTimeDelta(uint32_t queryIdx0, uint32_t queryIdx1) const
{
	if (!GLEW_ARB_timer_query)
		return 0;

	const uint32_t queryBase = NUM_OPENGL_TIMER_QUERIES * (1 - (drawFrame & 1));

	assert(queryIdx0 < NUM_OPENGL_TIMER_QUERIES);
	assert(queryIdx1 < NUM_OPENGL_TIMER_QUERIES);
	assert(queryIdx0 < queryIdx1);

	GLuint64 t0 = 0;
	GLuint64 t1 = 0;

	GLint res = 0;

	// results from the previous frame should already (or soon) be available
	while (!res) {
		glGetQueryObjectiv(glTimerQueries[queryBase + queryIdx1], GL_QUERY_RESULT_AVAILABLE, &res);
	}

	glGetQueryObjectui64v(glTimerQueries[queryBase + queryIdx0], GL_QUERY_RESULT, &t0);
	glGetQueryObjectui64v(glTimerQueries[queryBase + queryIdx1], GL_QUERY_RESULT, &t1);

	// nanoseconds between timestamps
	return (t1 - t0);
}

SDL_GLContext CGLRendererCore::CreateGLContext(const int2& minCtx)
{
	SDL_GLContext newContext = nullptr;

	constexpr int2 glCtxs[] = {{2, 0}, {2, 1},  {3, 0}, {3, 1}, {3, 2}, {3, 3},  {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6}};
	          int2 cmpCtx;

	if (std::find(&glCtxs[0], &glCtxs[0] + (sizeof(glCtxs) / sizeof(int2)), minCtx) == (&glCtxs[0] + (sizeof(glCtxs) / sizeof(int2)))) {
		handleerror(nullptr, "illegal OpenGL context-version specified, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	if ((newContext = SDL_GL_CreateContext(sdlWindow)) != nullptr)
		return newContext;

	const char* frmts[] = {"[GR::%s] error (\"%s\") creating main GL%d.%d %s-context", "[GR::%s] created main GL%d.%d %s-context"};
	const char* profs[] = {"compatibility", "core"};

	char buf[1024] = {0};
	SNPRINTF(buf, sizeof(buf), frmts[false], __func__, SDL_GetError(), minCtx.x, minCtx.y, profs[forceCoreContext]);

	for (const int2 tmpCtx: glCtxs) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, tmpCtx.x);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, tmpCtx.y);

		for (uint32_t mask: {SDL_GL_CONTEXT_PROFILE_CORE, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY}) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, mask);

			if ((newContext = SDL_GL_CreateContext(sdlWindow)) == nullptr) {
				LOG_L(L_WARNING, frmts[false], __func__, SDL_GetError(), tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			} else {
				// save the lowest successfully created fallback compatibility-context
				if (mask == SDL_GL_CONTEXT_PROFILE_COMPATIBILITY && cmpCtx.x == 0 && tmpCtx.x >= minCtx.x)
					cmpCtx = tmpCtx;

				LOG_L(L_WARNING, frmts[true], __func__, tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			}

			// accepts nullptr's
			SDL_GL_DeleteContext(newContext);
		}
	}

	if (cmpCtx.x == 0) {
		handleerror(nullptr, buf, "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, cmpCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, cmpCtx.y);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	// should never fail at this point
	return (newContext = SDL_GL_CreateContext(sdlWindow));
}

void CGLRendererCore::CheckGLExtensions() const
{
	char extMsg[ 128] = {0};
	char errMsg[2048] = {0};
	char* ptr = &extMsg[0];

	if (!GLEW_ARB_multitexture       ) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " multitexture ");
	if (!GLEW_ARB_texture_env_combine) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " texture_env_combine ");
	if (!GLEW_ARB_texture_compression) ptr += snprintf(ptr, sizeof(extMsg) - (ptr - extMsg), " texture_compression ");

	if (extMsg[0] == 0)
		return;

	SNPRINTF(errMsg, sizeof(errMsg),
		"OpenGL extension(s) GL_ARB_{%s} not found; update your GPU drivers!\n"
		"  GL renderer: %s\n"
		"  GL  version: %s\n",
		extMsg,
		globalRenderingInfo.glRenderer,
		globalRenderingInfo.glVersion);

	throw unsupported_error(errMsg);
}

void CGLRendererCore::SetGLSupportFlags()
{
	const std::string& glVendor = StringToLower(globalRenderingInfo.glVendor);
	const std::string& glRenderer = StringToLower(globalRenderingInfo.glRenderer);
	const std::string& glVersion = StringToLower(globalRenderingInfo.glVersion);

	haveGLSL  = (glGetString(GL_SHADING_LANGUAGE_VERSION) != nullptr);
	haveGLSL &= static_cast<bool>(GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader);
	haveGLSL &= static_cast<bool>(GLEW_VERSION_2_0); // we want OpenGL 2.0 core functions

	#ifndef HEADLESS
	if (!haveGLSL)
		throw unsupported_error("OpenGL shaders not supported, aborting");
	#endif

	// useful if a GPU claims to support GL4 and shaders but crashes (Intels...)
	haveGLSL &= !forceDisableShaders;

	haveAMD    = (  glVendor.find(   "ati ") != std::string::npos) || (  glVendor.find("amd ") != std::string::npos) ||
				 (glRenderer.find("radeon ") != std::string::npos) || (glRenderer.find("amd ") != std::string::npos); //it's amazing how inconsistent AMD detection can be
	haveIntel  = (  glVendor.find(  "intel") != std::string::npos);
	haveNvidia = (  glVendor.find("nvidia ") != std::string::npos);
	haveMesa   = (glRenderer.find("mesa ") != std::string::npos) || (glRenderer.find("gallium ") != std::string::npos) || (glVersion.find(" mesa ") != std::string::npos);

	if (haveAMD) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "AMD";
	} else if (haveIntel) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "Intel";
	} else if (haveNvidia) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = "Nvidia";
	} else if (haveMesa) {
		globalRenderingInfo.gpuName   = globalRenderingInfo.glRenderer;
		globalRenderingInfo.gpuVendor = globalRenderingInfo.glVendor;
	} else {
		globalRenderingInfo.gpuName   = "Unknown";
		globalRenderingInfo.gpuVendor = "Unknown";
	}

	supportPersistentMapping = GLEW_ARB_buffer_storage;
	supportPersistentMapping &= (configHandler->GetInt("ForceDisablePersistentMapping") == 0);

	supportExplicitAttribLoc = GLEW_ARB_explicit_attrib_location;
	supportExplicitAttribLoc &= (configHandler->GetInt("ForceDisableExplicitAttribLocs") == 0);

	// ATI's x-series doesn't support NPOTs, hd-series does
	supportNonPowerOfTwoTex = GLEW_ARB_texture_non_power_of_two /* && (!haveAMD || (glRenderer.find(" x") == std::string::npos && glRenderer.find(" 9") == std::string::npos))*/;
	supportTextureQueryLOD = GLEW_ARB_texture_query_lod;


	for (size_t n = 0; (n < sizeof(globalRenderingInfo.glVersionShort) && globalRenderingInfo.glVersion[n] != 0); n++) {
		if ((globalRenderingInfo.glVersionShort[n] = globalRenderingInfo.glVersion[n]) == ' ') {
			globalRenderingInfo.glVersionShort[n] = 0;
			break;
		}
	}

	for (size_t n = 0; (n < sizeof(globalRenderingInfo.glslVersionShort) && globalRenderingInfo.glslVersion[n] != 0); n++) {
		if ((globalRenderingInfo.glslVersionShort[n] = globalRenderingInfo.glslVersion[n]) == ' ') {
			globalRenderingInfo.glslVersionShort[n] = 0;
			break;
		}
	}
	if (int2 glslVerNum = { 0, 0 }; sscanf(globalRenderingInfo.glslVersionShort.data(), "%d.%d", &glslVerNum.x, &glslVerNum.y) == 2) {
		globalRenderingInfo.glslVersionNum = glslVerNum.x * 100 + glslVerNum.y;
	}

	haveGL4 = static_cast<bool>(GLEW_ARB_multi_draw_indirect);
	haveGL4 &= static_cast<bool>(GLEW_ARB_uniform_buffer_object);
	haveGL4 &= static_cast<bool>(GLEW_ARB_shader_storage_buffer_object);
	haveGL4 &= CheckShaderGL4();
	haveGL4 &= !forceDisableGL4;

	{
		// use some ATI bugfixes?
		const int amdHacksCfg = configHandler->GetInt("AtiHacks");
		amdHacks = haveAMD && !haveMesa;
		amdHacks &= (amdHacksCfg < 0); // runtime detect
		amdHacks |= (amdHacksCfg > 0); // user override
	}

	// runtime-compress textures? (also already required for SMF ground textures)
	// default to off because it reduces quality, smallest mipmap level is bigger
	if (GLEW_ARB_texture_compression)
		compressTextures = configHandler->GetBool("CompressTextures");


	#ifdef GLEW_NV_primitive_restart
	// not defined for headless builds
	supportRestartPrimitive = GLEW_NV_primitive_restart;
	#endif
	#ifdef GLEW_ARB_clip_control
	supportClipSpaceControl = GLEW_ARB_clip_control;
	#endif
	#ifdef GLEW_ARB_seamless_cube_map
	supportSeamlessCubeMaps = GLEW_ARB_seamless_cube_map;
	#endif
	#ifdef GLEW_EXT_framebuffer_multisample
	supportMSAAFrameBuffer = GLEW_EXT_framebuffer_multisample;
	#endif
	// CC did not exist as an extension before GL4.5, too recent to enforce

	//stick to the theory that reported = exist
	//supportClipSpaceControl &= ((globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y) >= 45);
	supportClipSpaceControl &= (configHandler->GetInt("ForceDisableClipCtrl") == 0);

	//supportFragDepthLayout = ((globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y) >= 42);
	#ifdef GLEW_ARB_conservative_depth
	supportFragDepthLayout = GLEW_ARB_conservative_depth; //stick to the theory that reported = exist
	#endif

	//stick to the theory that reported = exist
	//supportMSAAFrameBuffer &= ((globalRenderingInfo.glContextVersion.x * 10 + globalRenderingInfo.glContextVersion.y) >= 32);

	for (const int bits : {16, 24, 32}) {
		bool supported = false;
		if (FBO::IsSupported()) {
			FBO fbo;
			fbo.Bind();
			fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, 16, 16);
			const GLint format = IGlobalRendering::DepthBitsToFormat(bits);
			fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, format, 16, 16);
			supported = (fbo.GetStatus() == GL_FRAMEBUFFER_COMPLETE_EXT);
			fbo.Unbind();
		}

		if (supported)
			supportDepthBufferBitDepth = std::max(supportDepthBufferBitDepth, bits);
	}

	//TODO figure out if needed
	if (amdHacks) {
		supportDepthBufferBitDepth = 24;
	}

	if (haveGL4) {
		supportUniformData = true;
		supportModelUniformData = true;
	}
}

void CGLRendererCore::QueryGLMaxVals()
{
	// maximum 2D texture size
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxFragShSlots);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombShSlots);

	if (GLEW_EXT_texture_filter_anisotropic)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxTexAnisoLvl);

	// some GLSL relevant information
	if (GLEW_ARB_uniform_buffer_object) {
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &glslMaxUniformBufferBindings);
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,      &glslMaxUniformBufferSize);
	}

	if (GLEW_ARB_shader_storage_buffer_object) {
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &glslMaxStorageBufferBindings);
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,      &glslMaxStorageBufferSize);
	}

	glGetIntegerv(GL_MAX_VARYING_FLOATS,                 &glslMaxVaryings);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS,                 &glslMaxAttributes);
	glGetIntegerv(GL_MAX_DRAW_BUFFERS,                   &glslMaxDrawBuffers);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,               &glslMaxRecommendedIndices);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES,              &glslMaxRecommendedVertices);

	// GL_MAX_VARYING_FLOATS is the maximum number of floats, we count float4's
	glslMaxVaryings /= 4;
}

void CGLRendererCore::QueryGLVersionInfo(char (&sdlVersionStr)[64], char (&glVidMemStr)[64])
{
	auto& grInfo = globalRenderingInfo;

	auto& sdlVC = grInfo.sdlVersionCompiled;
	auto& sdlVL = grInfo.sdlVersionLinked;

	SDL_VERSION(&sdlVC);
	SDL_GetVersion(&sdlVL);

	if ((grInfo.glVersion   = (const char*) glGetString(GL_VERSION                 )) == nullptr) grInfo.glVersion   = "unknown";
	if ((grInfo.glVendor    = (const char*) glGetString(GL_VENDOR                  )) == nullptr) grInfo.glVendor    = "unknown";
	if ((grInfo.glRenderer  = (const char*) glGetString(GL_RENDERER                )) == nullptr) grInfo.glRenderer  = "unknown";
	if ((grInfo.glslVersion = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION)) == nullptr) grInfo.glslVersion = "unknown";
	if ((grInfo.glewVersion = (const char*) glewGetString(GLEW_VERSION             )) == nullptr) grInfo.glewVersion = "unknown";
	if ((grInfo.sdlDriverName = (const char*) SDL_GetCurrentVideoDriver(           )) == nullptr) grInfo.sdlDriverName = "unknown";
	// should never be null with any driver, no harm in an extra check
	// (absence of GLSL version string would indicate bigger problems)
	if (std::strcmp(globalRenderingInfo.glslVersion, "unknown") == 0)
		throw unsupported_error("OpenGL shaders not supported, aborting");

	if (!ShowDriverWarning(grInfo.glVendor))
		throw unsupported_error("OpenGL drivers not installed, aborting");

	constexpr const char* sdlFmtStr = "%d.%d.%d (linked) / %d.%d.%d (compiled)";
	constexpr const char* memFmtStr = "%iMB (total) / %iMB (available)";

	SNPRINTF(sdlVersionStr, sizeof(sdlVersionStr), sdlFmtStr,
		sdlVL.major, sdlVL.minor, sdlVL.patch,
		sdlVC.major, sdlVC.minor, sdlVC.patch
	);

	if (!GetAvailableVideoRAM(&grInfo.gpuMemorySize.x, grInfo.glVendor))
		return;

	const GLint totalMemMB = grInfo.gpuMemorySize.x / 1024;
	const GLint availMemMB = grInfo.gpuMemorySize.y / 1024;

	SNPRINTF(glVidMemStr, sizeof(glVidMemStr), memFmtStr, totalMemMB, availMemMB);
}

void CGLRendererCore::LogGLVersionInfo(const char* sdlVersionStr, const char* glVidMemStr) const
{
	LOG("[GR::%s]", __func__);
	LOG("\tSDL version : %s", sdlVersionStr);
	LOG("\tGL version  : %s", globalRenderingInfo.glVersion);
	LOG("\tGL vendor   : %s", globalRenderingInfo.glVendor);
	LOG("\tGL renderer : %s", globalRenderingInfo.glRenderer);
	LOG("\tGLSL version: %s", globalRenderingInfo.glslVersion);
	LOG("\tGLEW version: %s", globalRenderingInfo.glewVersion);
	LOG("\tGPU memory  : %s", glVidMemStr);
	LOG("\tSDL swap-int: %d", SDL_GL_GetSwapInterval());
	LOG("\tSDL driver  : %s", globalRenderingInfo.sdlDriverName);
	LOG("\t");
	LOG("\tInitialized OpenGL Context: %i.%i (%s)", globalRenderingInfo.glContextVersion.x, globalRenderingInfo.glContextVersion.y, globalRenderingInfo.glContextIsCore ? "Core" : "Compat");
	LOG("\tGLSL shader support       : %i", haveGLSL);
	LOG("\tGL4 support               : %i", haveGL4);
	LOG("\tFBO extension support     : %i", FBO::IsSupported());
	LOG("\tNVX GPU mem-info support  : %i", glewIsExtensionSupported("GL_NVX_gpu_memory_info"));
	LOG("\tATI GPU mem-info support  : %i", glewIsExtensionSupported("GL_ATI_meminfo"));
	LOG("\tTexture clamping to edge  : %i", glewIsExtensionSupported("GL_EXT_texture_edge_clamp"));
	LOG("\tNPOT-texture support      : %i (%i)", supportNonPowerOfTwoTex, glewIsExtensionSupported("GL_ARB_texture_non_power_of_two"));
	LOG("\tS3TC/DXT1 texture support : %i/%i", glewIsExtensionSupported("GL_EXT_texture_compression_s3tc"), glewIsExtensionSupported("GL_EXT_texture_compression_dxt1"));
	LOG("\ttexture query-LOD support : %i (%i)", supportTextureQueryLOD, glewIsExtensionSupported("GL_ARB_texture_query_lod"));
	LOG("\tMSAA frame-buffer support : %i (%i)", supportMSAAFrameBuffer, glewIsExtensionSupported("GL_EXT_framebuffer_multisample"));
	LOG("\tZ-buffer depth            : %i (-)" , supportDepthBufferBitDepth);
	LOG("\tprimitive-restart support : %i (%i)", supportRestartPrimitive, glewIsExtensionSupported("GL_NV_primitive_restart"));
	LOG("\tclip-space control support: %i (%i)", supportClipSpaceControl, glewIsExtensionSupported("GL_ARB_clip_control"));
	LOG("\tseamless cube-map support : %i (%i)",  supportSeamlessCubeMaps, glewIsExtensionSupported("GL_ARB_seamless_cube_map"));
	LOG("\tfrag-depth layout support : %i (%i)", supportFragDepthLayout, glewIsExtensionSupported("GL_ARB_conservative_depth"));
	LOG("\tpersistent maps support   : %i (%i)", supportPersistentMapping, glewIsExtensionSupported("GL_ARB_buffer_storage"));
	LOG("\texplicit attribs location : %i (%i)", supportExplicitAttribLoc, glewIsExtensionSupported("GL_ARB_explicit_attrib_location"));
	LOG("\tmulti draw indirect       : %i (-)" , glewIsExtensionSupported("GL_ARB_multi_draw_indirect"));
	LOG("\tarray textures            : %i (-)" , glewIsExtensionSupported("GL_EXT_texture_array"));
	LOG("\tbuffer copy support       : %i (-)" , glewIsExtensionSupported("GL_ARB_copy_buffer"));
	LOG("\tindirect draw             : %i (-)" , glewIsExtensionSupported("GL_ARB_draw_indirect"));
	LOG("\tbase instance             : %i (-)" , glewIsExtensionSupported("GL_ARB_base_instance"));

	LOG("\t");
	LOG("\tmax. FBO samples              : %i", FBO::GetMaxSamples());
	LOG("\tmax. FS/program texture slots : %i/%i", maxFragShSlots, maxCombShSlots);
	LOG("\tmax. texture size             : %i", maxTextureSize);
	LOG("\tmax. texture anisotropy level : %f", maxTexAnisoLvl);
	LOG("\tmax. vec4 varyings/attributes : %i/%i", glslMaxVaryings, glslMaxAttributes);
	LOG("\tmax. draw-buffers             : %i", glslMaxDrawBuffers);
	LOG("\tmax. rec. indices/vertices    : %i/%i", glslMaxRecommendedIndices, glslMaxRecommendedVertices);
	LOG("\tmax. uniform buffer-bindings  : %i", glslMaxUniformBufferBindings);
	LOG("\tmax. uniform block-size       : %iKB", glslMaxUniformBufferSize / 1024);
	LOG("\tmax. storage buffer-bindings  : %i", glslMaxStorageBufferBindings);
	LOG("\tmax. storage block-size       : %iMB", glslMaxStorageBufferSize / (1024 * 1024));
	LOG("\t");
	LOG("\tenable AMD-hacks : %i", amdHacks);
	LOG("\tcompress MIP-maps: %i", compressTextures);

	GLint numberOfTextureFormats = 0;
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numberOfTextureFormats);
	std::vector<GLint> textureFormats; textureFormats.resize(numberOfTextureFormats);
	glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, textureFormats.data());

	#define EnumToString(arg) { arg, #arg }
	std::unordered_map<GLenum, std::string> compressedEnumToString = {
		EnumToString(GL_COMPRESSED_RED_RGTC1),
		EnumToString(GL_COMPRESSED_SIGNED_RED_RGTC1),
		EnumToString(GL_COMPRESSED_RG_RGTC2),
		EnumToString(GL_COMPRESSED_SIGNED_RG_RGTC2),
		EnumToString(GL_COMPRESSED_RGBA_BPTC_UNORM),
		EnumToString(GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM),
		EnumToString(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT),
		EnumToString(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT),
		EnumToString(GL_COMPRESSED_RGB8_ETC2),
		EnumToString(GL_COMPRESSED_SRGB8_ETC2),
		EnumToString(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2),
		EnumToString(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2),
		EnumToString(GL_COMPRESSED_RGBA8_ETC2_EAC),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC),
		EnumToString(GL_COMPRESSED_R11_EAC),
		EnumToString(GL_COMPRESSED_SIGNED_R11_EAC),
		EnumToString(GL_COMPRESSED_RG11_EAC),
		EnumToString(GL_COMPRESSED_SIGNED_RG11_EAC),
		EnumToString(GL_COMPRESSED_RGB_S3TC_DXT1_EXT),
		EnumToString(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT),
		EnumToString(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT),
		EnumToString(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_4x4_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_5x4_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_5x5_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_6x5_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_6x6_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_8x5_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_8x6_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_8x8_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_10x5_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_10x6_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_10x8_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_10x10_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_12x10_KHR),
		EnumToString(GL_COMPRESSED_RGBA_ASTC_12x12_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR),
		EnumToString(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR),
	};
	#undef EnumToString

	LOG("\tNumber of compressed texture formats: %i", numberOfTextureFormats);
	std::ostringstream ss;
	for (auto tf : textureFormats) {
		auto it = compressedEnumToString.find(tf);
		if (it != compressedEnumToString.end())
			ss << it->second << ", ";
		else
			ss << "0x" << std::hex << tf << ", ";
	}
	ss.seekp(-2, std::ios_base::end);
	ss << ".";
	LOG("\tCompressed texture formats: %s", ss.str().c_str());
}

void CGLRendererCore::ToggleMultisampling() const
{
	static constexpr std::array<void(*)(), 2> ToggleFuncs = { []() { glDisable(GL_MULTISAMPLE); }, []() { glEnable(GL_MULTISAMPLE); } };
	ToggleFuncs[msaaLevel > 0]();
}

bool CGLRendererCore::CheckShaderGL4() const
{
#ifndef HEADLESS
	//the code below doesn't make any sense, but here only to test if the shader can be compiled
	constexpr static const char* vsSrc = R"(
#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 6) in uvec4 instData;

layout(std140, binding = 0) readonly buffer MatrixBuffer {
	mat4 mat[];
};

out Data {
	float vFloat;
};
void main()
{
	vFloat = float(instData.y);
	gl_Position = mat[instData.x] * vec4(pos, 1.0);
}
)";

	constexpr static const char* fsSrc = R"(
#version 430 core

in Data {
	float vFloat;
};
out vec4 fragColor;
void main()
{
	fragColor = vec4(1.0, 1.0, 1.0, vFloat);
}
)";

	auto testShader = std::make_unique<Shader::GLSLProgramObject>("[GL-TestShader]");
	testShader->AttachShaderObject(new Shader::GLSLShaderObject(GL_VERTEX_SHADER  , vsSrc));
	testShader->AttachShaderObject(new Shader::GLSLShaderObject(GL_FRAGMENT_SHADER, fsSrc));
	testShader->SetLogReporting(false); //no need to spam guinea pig shader errors
	testShader->Link();
	testShader->Enable();
	testShader->Disable();
	testShader->Validate();

	return testShader->IsValid();
	//no need for explicit destuction here
#else
	return false;
#endif
}

/**
 * @brief multisample verify
 * @return whether verification passed
 *
 * Tests whether FSAA was actually enabled
 */
bool CGLRendererCore::CheckGLMultiSampling() const
{
	if (msaaLevel == 0)
		return false;
	if (!GLEW_ARB_multisample)
		return false;

	GLint buffers = 0;
	GLint samples = 0;

	glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers);
	glGetIntegerv(GL_SAMPLES, &samples);

	return (buffers != 0 && samples != 0);
}

bool CGLRendererCore::CheckGLContextVersion(const int2& minCtx) const
{
	#ifdef HEADLESS
	return true;
	#else
	int2 tmpCtx = {0, 0};

	glGetIntegerv(GL_MAJOR_VERSION, &tmpCtx.x);
	glGetIntegerv(GL_MINOR_VERSION, &tmpCtx.y);

	GLint profile = 0;
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);

	if (profile != 0)
		globalRenderingInfo.glContextIsCore = (profile == GL_CONTEXT_CORE_PROFILE_BIT);
	else
		globalRenderingInfo.glContextIsCore = !GLEW_ARB_compatibility;

	// keep this for convenience
	globalRenderingInfo.glContextVersion = tmpCtx;

	// compare major * 10 + minor s.t. 4.1 evaluates as larger than 3.2
	return ((tmpCtx.x * 10 + tmpCtx.y) >= (minCtx.x * 10 + minCtx.y));
	#endif
}



#if defined(_WIN32) && !defined(HEADLESS)
	#if defined(_MSC_VER) && _MSC_VER >= 1600
		#define _GL_APIENTRY __stdcall
	#else
		#include <windef.h>
		#define _GL_APIENTRY APIENTRY
	#endif
#else
	#define _GL_APIENTRY
#endif


#if (defined(GL_ARB_debug_output) && !defined(HEADLESS))

#ifndef GL_DEBUG_SOURCE_API
#define GL_DEBUG_SOURCE_API                GL_DEBUG_SOURCE_API_ARB
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM      GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB
#define GL_DEBUG_SOURCE_SHADER_COMPILER    GL_DEBUG_SOURCE_SHADER_COMPILER_ARB
#define GL_DEBUG_SOURCE_THIRD_PARTY        GL_DEBUG_SOURCE_THIRD_PARTY_ARB
#define GL_DEBUG_SOURCE_APPLICATION        GL_DEBUG_SOURCE_APPLICATION_ARB
#define GL_DEBUG_SOURCE_OTHER              GL_DEBUG_SOURCE_OTHER_ARB

#define GL_DEBUG_TYPE_ERROR                GL_DEBUG_TYPE_ERROR_ARB
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR  GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR   GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB
#define GL_DEBUG_TYPE_PORTABILITY          GL_DEBUG_TYPE_PORTABILITY_ARB
#define GL_DEBUG_TYPE_PERFORMANCE          GL_DEBUG_TYPE_PERFORMANCE_ARB
#if (defined(GL_DEBUG_TYPE_MARKER_ARB) && defined(GL_DEBUG_TYPE_PUSH_GROUP_ARB) && defined(GL_DEBUG_TYPE_POP_GROUP_ARB))
#define GL_DEBUG_TYPE_MARKER               GL_DEBUG_TYPE_MARKER_ARB
#define GL_DEBUG_TYPE_PUSH_GROUP           GL_DEBUG_TYPE_PUSH_GROUP_ARB
#define GL_DEBUG_TYPE_POP_GROUP            GL_DEBUG_TYPE_POP_GROUP_ARB
#else
#define GL_DEBUG_TYPE_MARKER               -1u
#define GL_DEBUG_TYPE_PUSH_GROUP           -2u
#define GL_DEBUG_TYPE_POP_GROUP            -3u
#endif
#define GL_DEBUG_TYPE_OTHER                GL_DEBUG_TYPE_OTHER_ARB

#define GL_DEBUG_SEVERITY_HIGH             GL_DEBUG_SEVERITY_HIGH_ARB
#define GL_DEBUG_SEVERITY_MEDIUM           GL_DEBUG_SEVERITY_MEDIUM_ARB
#define GL_DEBUG_SEVERITY_LOW              GL_DEBUG_SEVERITY_LOW_ARB

#define GL_DEBUG_OUTPUT_SYNCHRONOUS        GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB
#define GLDEBUGPROC                        GLDEBUGPROCARB
#endif

#ifndef glDebugMessageCallback
#define glDebugMessageCallback  glDebugMessageCallbackARB
#define glDebugMessageControl   glDebugMessageControlARB
#endif

constexpr static std::array<GLenum,  7> msgSrceEnums = {GL_DONT_CARE, GL_DEBUG_SOURCE_API, GL_DEBUG_SOURCE_WINDOW_SYSTEM, GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_SOURCE_OTHER};
constexpr static std::array<GLenum, 10> msgTypeEnums = {GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, GL_DEBUG_TYPE_PORTABILITY, GL_DEBUG_TYPE_PERFORMANCE, GL_DEBUG_TYPE_MARKER, GL_DEBUG_TYPE_PUSH_GROUP, GL_DEBUG_TYPE_POP_GROUP, GL_DEBUG_TYPE_OTHER};
constexpr static std::array<GLenum,  4> msgSevrEnums = {GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, GL_DEBUG_SEVERITY_MEDIUM, GL_DEBUG_SEVERITY_HIGH};

static inline const char* glDebugMessageSourceName(GLenum msgSrce) {
	switch (msgSrce) {
		case GL_DEBUG_SOURCE_API            : return             "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM  : return   "WINDOW_SYSTEM"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER_COMPILER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY    : return     "THIRD_PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION    : return     "APPLICATION"; break;
		case GL_DEBUG_SOURCE_OTHER          : return           "OTHER"; break;
		case GL_DONT_CARE                   : return       "DONT_CARE"; break;
		default                             :                         ; break;
	}

	return "UNKNOWN";
}

static inline const char* glDebugMessageTypeName(GLenum msgType) {
	switch (msgType) {
		case GL_DEBUG_TYPE_ERROR              : return       "ERROR"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return  "DEPRECATED"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR : return   "UNDEFINED"; break;
		case GL_DEBUG_TYPE_PORTABILITY        : return "PORTABILITY"; break;
		case GL_DEBUG_TYPE_PERFORMANCE        : return "PERFORMANCE"; break;
		case GL_DEBUG_TYPE_MARKER             : return      "MARKER"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP         : return  "PUSH_GROUP"; break;
		case GL_DEBUG_TYPE_POP_GROUP          : return   "POP_GROUP"; break;
		case GL_DEBUG_TYPE_OTHER              : return       "OTHER"; break;
		case GL_DONT_CARE                     : return   "DONT_CARE"; break;
		default                               :                     ; break;
	}

	return "UNKNOWN";
}

static inline const char* glDebugMessageSeverityName(GLenum msgSevr) {
	switch (msgSevr) {
		case GL_DEBUG_SEVERITY_HIGH  : return      "HIGH"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: return    "MEDIUM"; break;
		case GL_DEBUG_SEVERITY_LOW   : return       "LOW"; break;
		case GL_DONT_CARE            : return "DONT_CARE"; break;
		default                      :                   ; break;
	}

	return "UNKNOWN";
}

static void _GL_APIENTRY glDebugMessageCallbackFunc(
	GLenum msgSrce,
	GLenum msgType,
	GLuint msgID,
	GLenum msgSevr,
	GLsizei length,
	const GLchar* dbgMessage,
	const GLvoid* userParam
) {
	switch (msgID) {
		case 131169: { return; } break; // "Framebuffer detailed info: The driver allocated storage for renderbuffer N."
		case 131185: { return; } break; // "Buffer detailed info: Buffer object 260 (bound to GL_PIXEL_UNPACK_BUFFER_ARB, usage hint is GL_STREAM_DRAW) has been mapped in DMA CACHED memory."
		default: {} break;
	}

	const char* msgSrceStr = glDebugMessageSourceName(msgSrce);
	const char* msgTypeStr = glDebugMessageTypeName(msgType);
	const char* msgSevrStr = glDebugMessageSeverityName(msgSevr);

	LOG_L(L_WARNING, "[OPENGL_DEBUG] id=%u source=%s type=%s severity=%s msg=\"%s\"", msgID, msgSrceStr, msgTypeStr, msgSevrStr, dbgMessage);

	if ((userParam == nullptr) || !(*reinterpret_cast<const bool*>(userParam)))
		return;

	CrashHandler::PrepareStacktrace();
	CrashHandler::Stacktrace(Threading::GetCurrentThread(), "rendering", LOG_LEVEL_WARNING);
	CrashHandler::CleanupStacktrace();
}
#endif


bool CGLRendererCore::ToggleDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const
{
#if (defined(GL_ARB_debug_output) && !defined(HEADLESS))
	if (!(GLEW_ARB_debug_output || GLEW_KHR_debug))
		return false;

	if (rendererDebug) {
		const char* msgSrceStr = glDebugMessageSourceName(msgSrceEnums[msgSrceIdx %= msgSrceEnums.size()]);
		const char* msgTypeStr = glDebugMessageTypeName(msgTypeEnums[msgTypeIdx %= msgTypeEnums.size()]);
		const char* msgSevrStr = glDebugMessageSeverityName(msgSevrEnums[msgSevrIdx %= msgSevrEnums.size()]);

		const static bool dbgTraces = configHandler->GetBool("DebugGLStacktraces");
		// install OpenGL debug message callback; typecast is a workaround
		// for #4510 (change in callback function signature with GLEW 1.11)
		// use SYNCHRONOUS output, we want our callback to run in the same
		// thread as the bugged GL call (for proper stacktraces)
		// CB userParam is const, but has to be specified sans qualifiers
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback((GLDEBUGPROC)&glDebugMessageCallbackFunc, (void*)&dbgTraces);
		glDebugMessageControl(msgSrceEnums[msgSrceIdx], msgTypeEnums[msgTypeIdx], msgSevrEnums[msgSevrIdx], 0, nullptr, GL_TRUE);

		LOG("[GR::%s] OpenGL debug-message callback enabled (source=%s type=%s severity=%s)", __func__, msgSrceStr, msgTypeStr, msgSevrStr);
	}
	else {
		glDebugMessageCallback(nullptr, nullptr);
		glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		LOG("[GR::%s] OpenGL debug-message callback disabled", __func__);
	}
	configHandler->Set("DebugGL", rendererDebug);
#endif
	return true;
}

void CGLRendererCore::UpdateViewport()
{
	glViewport(viewPosX, viewPosY, viewSizeX, viewSizeY);
}

void CGLRendererCore::UpdateDualViewport()
{
	glViewport(dualViewPosX, dualViewPosY, dualViewSizeX, dualViewSizeY);
}
