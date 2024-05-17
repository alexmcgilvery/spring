/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <sstream>
#include <iomanip>

#include <SDL.h>

#include "GlobalRendering.h"
#include "GlobalRenderingInfo.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/GL/StreamBuffer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/UniformConstants.h"
#include "Rendering/Fonts/glFont.h"
#include "System/bitops.h"
#include "System/EventHandler.h"
#include "System/type2.h"
#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/StringHash.h"
#include "System/Matrix44f.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/MessageBox.h"
#include "System/Platform/Threading.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Platform/errorhandler.h"
#include "System/ScopedResource.h"
#include "System/QueueToMain.h"
#include "System/creg/creg_cond.h"
#include "Game/Game.h"

#include <SDL_syswm.h>
#include <SDL_rect.h>

#include "System/Misc/TracyDefs.h"

CONFIG(bool, DebugGL).defaultValue(false).description("Enables GL debug-context and output. (see GL_ARB_debug_output)");
CONFIG(bool, DebugGLStacktraces).defaultValue(false).description("Create a stacktrace when an OpenGL error occurs");

CONFIG(int, GLContextMajorVersion).defaultValue(3).minimumValue(3).maximumValue(4);
CONFIG(int, GLContextMinorVersion).defaultValue(0).minimumValue(0).maximumValue(5);
CONFIG(int, MSAALevel).defaultValue(0).minimumValue(0).maximumValue(32).description("Enables multisample anti-aliasing; 'level' is the number of samples used.");

CONFIG(int, ForceDisablePersistentMapping).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableExplicitAttribLocs).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableClipCtrl).defaultValue(0).minimumValue(0).maximumValue(1);
//CONFIG(int, ForceDisableShaders).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceDisableGL4).defaultValue(0).safemodeValue(1).minimumValue(0).maximumValue(1);

CONFIG(int, ForceCoreContext).defaultValue(0).minimumValue(0).maximumValue(1);
CONFIG(int, ForceSwapBuffers).defaultValue(1).minimumValue(0).maximumValue(1);
CONFIG(int, AtiHacks).defaultValue(-1).headlessValue(0).minimumValue(-1).maximumValue(1).description("Enables graphics drivers workarounds for users with AMD proprietary drivers.\n -1:=runtime detect, 0:=off, 1:=on");

// enabled in safemode, far more likely the gpu runs out of memory than this extension causes crashes!
CONFIG(bool, CompressTextures).defaultValue(false).safemodeValue(true).description("Runtime compress most textures to save VideoRAM.");
CONFIG(bool, DualScreenMode).defaultValue(false).description("Sets whether to split the screen in half, with one half for minimap and one for main screen. Right side is for minimap unless DualScreenMiniMapOnLeft is set.");
CONFIG(bool, DualScreenMiniMapOnLeft).defaultValue(false).description("When set, will make the left half of the screen the minimap when DualScreenMode is set.");
CONFIG(bool, TeamNanoSpray).defaultValue(true).headlessValue(false);

CONFIG(int, MinimizeOnFocusLoss).defaultValue(0).minimumValue(0).maximumValue(1).description("When set to 1 minimize Window if it loses key focus when in fullscreen mode.");

CONFIG(bool, Fullscreen).defaultValue(true).headlessValue(false).description("Sets whether the game will run in fullscreen, as opposed to a window. For Windowed Fullscreen of Borderless Window, set this to 0, WindowBorderless to 1, and WindowPosX and WindowPosY to 0.");
CONFIG(bool, WindowBorderless).defaultValue(false).description("When set and Fullscreen is 0, will put the game in Borderless Window mode, also known as Windowed Fullscreen. When using this, it is generally best to also set WindowPosX and WindowPosY to 0");
CONFIG(bool, BlockCompositing).defaultValue(false).safemodeValue(true).description("Disables kwin compositing to fix tearing, possible fixes low FPS in windowed mode, too.");

CONFIG(int, XResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the width of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, YResolution).defaultValue(0).headlessValue(8).minimumValue(0).description("Sets the height of the game screen. If set to 0 Spring will autodetect the current resolution of your desktop.");
CONFIG(int, XResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See XResolution, just for windowed.");
CONFIG(int, YResolutionWindowed).defaultValue(0).headlessValue(8).minimumValue(0).description("See YResolution, just for windowed.");
CONFIG(int, WindowPosX).defaultValue(0 ).description("Sets the horizontal position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");
CONFIG(int, WindowPosY).defaultValue(32).description("Sets the vertical position of the game window, if Fullscreen is 0. When WindowBorderless is set, this should usually be 0.");

//deprecated stuff
CONFIG(int, RendererHash).deprecated(true);
CONFIG(bool, FSAA).deprecated(true);
CONFIG(int, FSAALevel).deprecated(true);
CONFIG(bool, ForceDisableShaders).deprecated(true);


#define WINDOWS_NO_INVISIBLE_GRIPS 1

/**
 * @brief global rendering
 *
 * Global instance of CGlobalRendering
 */

CGlobalRendering* globalRendering = nullptr;
GlobalRenderingInfo globalRenderingInfo;


CR_BIND_INTERFACE(CGlobalRendering)

CR_REG_METADATA(CGlobalRendering, (
	CR_MEMBER(teamNanospray),
	CR_MEMBER(drawSky),
	CR_MEMBER(drawWater),
	CR_MEMBER(drawGround),
	CR_MEMBER(drawMapMarks),
	CR_MEMBER(drawFog),

	CR_MEMBER(drawDebug),
	CR_MEMBER(drawDebugTraceRay),
	CR_MEMBER(drawDebugCubeMap),

	CR_MEMBER(timeOffset),
	CR_MEMBER(lastTimeOffset),
	CR_MEMBER(lastFrameTime),
	CR_MEMBER(lastFrameStart),
	CR_MEMBER(lastSwapBuffersEnd),
	CR_MEMBER(weightedSpeedFactor),
	CR_MEMBER(drawFrame),
	CR_MEMBER(FPS),

	CR_IGNORED(numDisplays),

	CR_IGNORED(screenSizeX),
	CR_IGNORED(screenSizeY),
	CR_IGNORED(screenPosX),
	CR_IGNORED(screenPosY),

	CR_IGNORED(winPosX),
	CR_IGNORED(winPosY),
	CR_IGNORED(winSizeX),
	CR_IGNORED(winSizeY),
	CR_IGNORED(viewPosX),
	CR_IGNORED(viewPosY),
	CR_IGNORED(viewSizeX),
	CR_IGNORED(viewSizeY),
	CR_IGNORED(viewWindowOffsetY),
	CR_IGNORED(dualViewPosX),
	CR_IGNORED(dualViewPosY),
	CR_IGNORED(dualViewSizeX),
	CR_IGNORED(dualViewSizeY),
	CR_IGNORED(dualWindowOffsetY),
	CR_IGNORED(winBorder),
	CR_IGNORED(winChgFrame),
	CR_IGNORED(gmeChgFrame),
	CR_IGNORED(screenViewMatrix),
	CR_IGNORED(screenProjMatrix),
	CR_MEMBER(grTime),
	CR_IGNORED(pixelX),
	CR_IGNORED(pixelY),

	CR_IGNORED(minViewRange),
	CR_IGNORED(maxViewRange),
	CR_IGNORED(aspectRatio),

	CR_IGNORED(forceDisablePersistentMapping),
	CR_IGNORED(forceDisableShaders),
	CR_IGNORED(forceSwapBuffers),

	CR_IGNORED(msaaLevel),
	CR_IGNORED(maxTextureSize),
	CR_IGNORED(maxFragShSlots),
	CR_IGNORED(maxCombShSlots),
	CR_IGNORED(maxTexAnisoLvl),

	CR_IGNORED(active),
	CR_IGNORED(compressTextures),

	CR_IGNORED(haveAMD),
	CR_IGNORED(haveMesa),
	CR_IGNORED(haveIntel),
	CR_IGNORED(haveNvidia),

	CR_IGNORED(amdHacks),
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

	CR_IGNORED(fullScreen),
	CR_IGNORED(borderless),

	CR_IGNORED(underExternalDebug),
	CR_IGNORED(sdlWindow)
))

CGlobalRendering::CGlobalRendering()
	: timeOffset(0.0f)
	, lastTimeOffset(0.0f)
	, lastFrameTime(0.0f)
	, lastFrameStart(spring_notime)
	, lastSwapBuffersEnd(spring_notime)
	, weightedSpeedFactor(0.0f)
	, drawFrame(1)
	, FPS(1.0f)

	, numDisplays(1)

	, screenSizeX(1)
	, screenSizeY(1)

	// window geometry
	, winPosX(configHandler->GetInt("WindowPosX"))
	, winPosY(configHandler->GetInt("WindowPosY"))
	, winSizeX(1)
	, winSizeY(1)

	// viewport geometry
	, viewPosX(0)
	, viewPosY(0)
	, viewSizeX(1)
	, viewSizeY(1)
	, viewWindowOffsetY(0)

	// dual viewport geometry (DualScreenMode = 1)
	, dualWindowOffsetY(0)
	, dualViewPosX(0)
	, dualViewPosY(0)
	, dualViewSizeX(0)
	, dualViewSizeY(0)


	, winBorder{ 0 }

	, winChgFrame(0)
	, gmeChgFrame(0)

	, screenViewMatrix()
	, screenProjMatrix()

	, grTime()

	// pixel geometry
	, pixelX(0.01f)
	, pixelY(0.01f)

	// sane defaults
	, minViewRange(MIN_ZNEAR_DIST * 8.0f)
	, maxViewRange(MAX_VIEW_RANGE * 0.5f)
	, aspectRatio(1.0f)

	, forceDisableShaders(/*configHandler->GetInt("ForceDisableShaders")*/ false)
	, forceSwapBuffers(configHandler->GetInt("ForceSwapBuffers"))

	// fallback
	, maxTextureSize(2048)
	, maxFragShSlots(8)
	, maxCombShSlots(8)
	, maxTexAnisoLvl(0.0f)

	, drawSky(true)
	, drawWater(true)
	, drawGround(true)
	, drawMapMarks(true)
	, drawFog(true)

	, drawDebug(false)
	, drawDebugTraceRay(false)
	, drawDebugCubeMap(false)

	, teamNanospray(configHandler->GetBool("TeamNanoSpray"))
	, active(true)

	, dualScreenMode(false)
	, dualScreenMiniMapOnLeft(false)
	, fullScreen(configHandler->GetBool("Fullscreen"))
	, borderless(configHandler->GetBool("WindowBorderless"))
	, underExternalDebug(false)
	, sdlWindow{nullptr}
{
	verticalSync->WrapNotifyOnChange();
	configHandler->NotifyOnChange(this, {
		"DualScreenMode",
		"DualScreenMiniMapOnLeft",
		"Fullscreen",
		"WindowBorderless",
		"XResolution",
		"YResolution",
		"XResolutionWindowed",
		"YResolutionWindowed",
		"WindowPosX",
		"WindowPosY"
	});
	SetDualScreenParams();
}

CGlobalRendering::~CGlobalRendering()
{
	configHandler->RemoveObserver(this);
	verticalSync->WrapRemoveObserver();

	DestroyWindow();
	KillSDL();
}

void CGlobalRendering::PreKill()
{
	UniformConstants::GetInstance().Kill(); //unsafe to kill in ~CGlobalRendering()
	RenderBuffer::KillStatic();
	CShaderHandler::FreeInstance();
}

SDL_Window* CGlobalRendering::CreateSDLWindow(const char* title)
{
	SDL_Window* newWindow = nullptr;
	
	newWindow = RendererCreateSDLWindow(title);

	if (newWindow == nullptr) {
		auto buf = fmt::sprintf("[GR::%s] could not create SDL-window\n", __func__);
		handleerror(nullptr, buf.c_str(), "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	UpdateWindowBorders(newWindow);

	return newWindow;
}

bool CGlobalRendering::CreateWindow(const char* title)
{
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		LOG_L(L_FATAL, "[GR::%s] error \"%s\" initializing SDL", __func__, SDL_GetError());
		return false;
	}

	bool result = RendererCreateWindow(title);

	SDL_DisableScreenSaver();
	return result;
}

void CGlobalRendering::DestroyWindow() {
	if (!sdlWindow)
		return;

	WindowManagerHelper::SetIconSurface(sdlWindow, nullptr);
	SetWindowInputGrabbing(false);

	SDL_DestroyWindow(sdlWindow);

	sdlWindow = nullptr;
}

void CGlobalRendering::KillSDL() const {
	#if !defined(HEADLESS)
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	#endif

	SDL_EnableScreenSaver();
	SDL_Quit();
}

void CGlobalRendering::PostWindowInit() { 
	RendererPostWindowInit();

	UpdateTimer();
}

void CGlobalRendering::SwapBuffers(bool allowSwapBuffers, bool clearErrors)
{
	RendererPresentFrame(allowSwapBuffers, clearErrors);
}

void CGlobalRendering::LogDisplayMode(SDL_Window* window) const
{
	// print final mode (call after SetupViewportGeometry, which updates viewSizeX/Y)
	SDL_DisplayMode dmode;
	SDL_GetWindowDisplayMode(window, &dmode);

	constexpr const std::array names = {
		"windowed::decorated",       // fs=0,bl=0
		"windowed::borderless",	     // fs=0,bl=1
		"fullscreen::exclusive",     // fs=1,bl=0
		"fullscreen::non-exclusive", // fs=1,bl=1
	};

	const int fs = fullScreen;
	const int bl = borderless;

	LOG("[GR::%s] display-mode set to %ix%ix%ibpp@%iHz (%s)", __func__, viewSizeX, viewSizeY, SDL_BITSPERPIXEL(dmode.format), dmode.refresh_rate, names[fs * 2 + bl]);
}

void CGlobalRendering::GetAllDisplayBounds(SDL_Rect& r) const
{
	int displayIdx = 0;
	GetDisplayBounds(r, &displayIdx);

	std::array<int, 4> mb = { r.x, r.y, r.x + r.w, r.y + r.h }; //L, T, R, B

	for (displayIdx = 1; displayIdx < numDisplays; ++displayIdx) {
		SDL_Rect db;
		GetDisplayBounds(db, &displayIdx);
		std::array<int, 4> b = { db.x, db.y, db.x + db.w, db.y + db.h }; //L, T, R, B

		if (b[0] < mb[0]) mb[0] = b[0];
		if (b[1] < mb[1]) mb[1] = b[1];
		if (b[2] > mb[2]) mb[2] = b[2];
		if (b[3] > mb[3]) mb[3] = b[3];
	}

	r = { mb[0], mb[1], mb[2] - mb[0], mb[3] - mb[1] };
}

void CGlobalRendering::GetWindowPosSizeBounded(int& x, int& y, int& w, int& h) const
{
	SDL_Rect r;
	GetAllDisplayBounds(r);

	x = std::clamp(x, r.x, r.x + r.w);
	y = std::clamp(y, r.y, r.y + r.h);
	w = std::max(w, minRes.x * (1 - fullScreen)); w = std::min(w, r.w - x);
	h = std::max(h, minRes.y * (1 - fullScreen)); h = std::min(h, r.h - y);
}

void CGlobalRendering::SetWindowTitle(const std::string& title)
{
	// SDL_SetWindowTitle deadlocks in case it's called from non-main thread (during the MT loading).

	static auto SetWindowTitleImpl = [](SDL_Window* sdlWindow, const std::string& title) {
		SDL_SetWindowTitle(sdlWindow, title.c_str());
	};

	if (Threading::IsMainThread())
		SetWindowTitleImpl(sdlWindow, title);
	else
		spring::QueuedFunction::Enqueue<decltype(SetWindowTitleImpl), SDL_Window*, const std::string&>(SetWindowTitleImpl, sdlWindow, title);
}

void CGlobalRendering::SetWindowAttributes(SDL_Window* window)
{
	// Get wanted state
	borderless = configHandler->GetBool("WindowBorderless");
	fullScreen = configHandler->GetBool("Fullscreen");
	winPosX = configHandler->GetInt("WindowPosX");
	winPosY = configHandler->GetInt("WindowPosY");

	// update display count
	numDisplays = SDL_GetNumVideoDisplays();

	// get desired resolution
	// note that the configured fullscreen resolution is just
	// ignored by SDL if not equal to the user's screen size
	const int2 maxRes = GetMaxWinRes();
	      int2 newRes = GetCfgWinRes();

	LOG("[GR::%s][1] cfgFullScreen=%d numDisplays=%d winPos=<%d,%d> newRes=<%d,%d>", __func__, fullScreen, numDisplays, winPosX, winPosY, newRes.x, newRes.y);
	GetWindowPosSizeBounded(winPosX, winPosY, newRes.x, newRes.y);
	LOG("[GR::%s][2] cfgFullScreen=%d numDisplays=%d winPos=<%d,%d> newRes=<%d,%d>", __func__, fullScreen, numDisplays, winPosX, winPosY, newRes.x, newRes.y);

//	if (SDL_SetWindowFullscreen(window, 0) != 0)
//		LOG("[GR::%s][3][SDL_SetWindowFullscreen] err=\"%s\"", __func__, SDL_GetError());

	SDL_RestoreWindow(window);
	SDL_SetWindowMinimumSize(window, minRes.x, minRes.y);
	SDL_SetWindowPosition(window, winPosX, winPosY);
	SDL_SetWindowSize(window, newRes.x, newRes.y);
	SDL_SetWindowBordered(window, borderless ? SDL_FALSE : SDL_TRUE);

	if (SDL_SetWindowFullscreen(window, (borderless ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) * fullScreen) != 0)
		LOG("[GR::%s][4][SDL_SetWindowFullscreen] err=\"%s\"", __func__, SDL_GetError());

	if (newRes == maxRes)
		SDL_MaximizeWindow(window);

	WindowManagerHelper::SetWindowResizable(window, !borderless && !fullScreen);
}

void CGlobalRendering::ConfigNotify(const std::string& key, const std::string& value)
{
	LOG("[GR::%s][1] key=%s val=%s", __func__, key.c_str(), value.c_str());
	if (key == "DualScreenMode" || key == "DualScreenMiniMapOnLeft") {
		SetDualScreenParams();
		UpdateRendererGeometry();

		if (game != nullptr)
			gmeChgFrame = drawFrame + 1; //need to do on next frame since config mutex is locked inside ConfigNotify

		return;
	}
	winChgFrame = drawFrame + 1; //need to do on next frame since config mutex is locked inside ConfigNotify
}

void CGlobalRendering::UpdateWindow()
{
	ZoneScoped;
	if (!spring::QueuedFunction::Empty()) {
		for (const auto& qf : spring::QueuedFunction::GetQueuedFunctions()) {
			qf->Execute();
		}
		spring::QueuedFunction::Clear();
	}

	if (gmeChgFrame == drawFrame)
		game->ResizeEvent();

	if (winChgFrame != drawFrame)
		return;

	if (sdlWindow == nullptr)
		return;

	SetWindowAttributes(sdlWindow);
	
	RendererUpdateWindow();
}

void CGlobalRendering::UpdateTimer()
{
	grTime = spring_now();
}

bool CGlobalRendering::GetWindowInputGrabbing()
{
	return static_cast<bool>(SDL_GetWindowGrab(sdlWindow));
}

bool CGlobalRendering::SetWindowInputGrabbing(bool enable)
{
	// SDL_SetWindowGrab deadlocks in case it's called from non-main thread (during the MT loading).

	static auto SetWindowGrabImpl = [](SDL_Window* sdlWindow, bool enable) {
		SDL_SetWindowGrab(sdlWindow, enable ? SDL_TRUE : SDL_FALSE);
	};

	if (Threading::IsMainThread())
		SetWindowGrabImpl(sdlWindow, enable);
	else
		spring::QueuedFunction::Enqueue(SetWindowGrabImpl, sdlWindow, enable);

	return enable;
}

bool CGlobalRendering::ToggleWindowInputGrabbing()
{
	if (GetWindowInputGrabbing())
		return (SetWindowInputGrabbing(false));

	return (SetWindowInputGrabbing(true));
}

bool CGlobalRendering::SetWindowPosHelper(int displayIdx, int winRPosX, int winRPosY, int winSizeX_, int winSizeY_, bool fs, bool bl) const
{
#ifndef HEADLESS
	if (displayIdx < 0 || displayIdx >= numDisplays) {
		LOG_L(L_ERROR, "[GR::%s] displayIdx(%d) is out of bounds (%d,%d)", __func__, displayIdx, 0, numDisplays - 1);
		return false;
	}

	SDL_Rect db;
	GetDisplayBounds(db, &displayIdx);

	const int2 tlPos = { db.x + winRPosX            , db.y + winRPosY             };
	const int2 brPos = { db.x + winRPosX + winSizeX_, db.y + winRPosY + winSizeY_ };

	configHandler->Set("WindowPosX", tlPos.x);
	configHandler->Set("WindowPosY", tlPos.y);

	configHandler->Set(xsKeys[fs], winSizeX_);
	configHandler->Set(ysKeys[fs], winSizeY_);
	configHandler->Set("Fullscreen", fs);
	configHandler->Set("WindowBorderless", bl);
#endif

	return true;
}

int2 CGlobalRendering::GetMaxWinRes() const {
	SDL_DisplayMode dmode;
	SDL_GetDesktopDisplayMode(GetCurrentDisplayIndex(), &dmode);
	return {dmode.w, dmode.h};
}

int2 CGlobalRendering::GetCfgWinRes() const
{
	int2 res = {configHandler->GetInt(xsKeys[fullScreen]), configHandler->GetInt(ysKeys[fullScreen])};

	// copy Native Desktop Resolution if user did not specify a value
	// SDL2 can do this itself if size{X,Y} are set to zero but fails
	// with Display Cloning and similar, causing DVI monitors to only
	// run at (e.g.) 640x400 and HDMI devices at full-HD
	// TODO: make screen configurable?
	if (res.x <= 0 || res.y <= 0)
		res = GetMaxWinRes();

	return res;
}

int CGlobalRendering::GetCurrentDisplayIndex() const
{
	return sdlWindow ? SDL_GetWindowDisplayIndex(sdlWindow) : 0;
}

void CGlobalRendering::GetDisplayBounds(SDL_Rect& r, const int* di) const
{
	const int displayIndex = di ? *di : GetCurrentDisplayIndex();
	SDL_GetDisplayBounds(displayIndex, &r);
}

void CGlobalRendering::GetUsableDisplayBounds(SDL_Rect& r, const int* di) const
{
	const int displayIndex = di ? *di : GetCurrentDisplayIndex();
	SDL_GetDisplayUsableBounds(displayIndex, &r);
}


// only called on startup; change the config based on command-line args
void CGlobalRendering::SetFullScreen(bool cliWindowed, bool cliFullScreen)
{
	const bool cfgFullScreen = configHandler->GetBool("Fullscreen");

	fullScreen = (cfgFullScreen && !cliWindowed  );
	fullScreen = (cfgFullScreen ||  cliFullScreen);

	configHandler->Set("Fullscreen", fullScreen);
}

void CGlobalRendering::SetDualScreenParams()
{
	dualScreenMode = configHandler->GetBool("DualScreenMode");
	dualScreenMiniMapOnLeft = dualScreenMode && configHandler->GetBool("DualScreenMiniMapOnLeft");
}

static const auto compareSDLRectPosX = [](const SDL_Rect& a, const SDL_Rect& b) {
  return (a.x < b.x);
};

void CGlobalRendering::UpdateViewPortGeometry()
{
	viewPosY = 0;
	viewSizeY = winSizeY;
	viewWindowOffsetY = 0;

	if (!dualScreenMode) {
		viewPosX = 0;
		viewSizeX = winSizeX;

		return;
	}

	dualViewPosY = 0;
	dualViewSizeY = viewSizeY;
	dualWindowOffsetY = 0;

	// Use halfscreen dual and view if only 1 display or fullscreen
	if (numDisplays == 1 || fullScreen) {
		const int halfWinSize = winSizeX >> 1;

		viewPosX = halfWinSize * dualScreenMiniMapOnLeft;
		viewSizeX = halfWinSize;

		dualViewPosX = halfWinSize - viewPosX;
		dualViewSizeX = halfWinSize;

		return;
	}

	std::vector<SDL_Rect> screenRects;
	SDL_Rect winRect = { winPosX, winPosY, winSizeX, winSizeY };

	for(int i = 0 ; i < numDisplays ; ++i)
	{
		SDL_Rect screen, interRect;
		GetDisplayBounds(screen, &i);
		LOG("[GR::%s] Raw Screen %i: pos %dx%d | size %dx%d", __func__, i, screen.x, screen.y, screen.w, screen.h);
		// we only care about screenRects that overlap window
		if (!SDL_IntersectRect(&screen, &winRect, &interRect)) {
			LOG("[GR::%s] No intersection: pos %dx%d | size %dx%d", __func__, screen.x, screen.y, screen.w, screen.h);
			continue;
		}

		// make screen positions relative to each other
		interRect.x -= winPosX;
		interRect.y -= winPosY;

		screenRects.push_back(interRect);
	}

	std::sort(screenRects.begin(), screenRects.end(), compareSDLRectPosX);

	int i = 0;
	for (auto screen : screenRects) {
		LOG("[GR::%s] Screen %i: pos %dx%d | size %dx%d", __func__, ++i, screen.x, screen.y, screen.w, screen.h);
	}

	if (screenRects.size() == 1) {
		SDL_Rect screenRect = screenRects.front();

		const int halfWinSize = screenRect.w >> 1;

		viewPosX = halfWinSize * dualScreenMiniMapOnLeft;
		viewSizeX = halfWinSize;

		dualViewPosX = halfWinSize - viewPosX;
		dualViewSizeX = halfWinSize;

		return;
	}

	SDL_Rect dualScreenRect = dualScreenMiniMapOnLeft ? screenRects.front() : screenRects.back();

	if (dualScreenMiniMapOnLeft) {
		screenRects.erase(screenRects.begin());
	} else {
		screenRects.pop_back();
	}

	const SDL_Rect first = screenRects.front();
	const SDL_Rect last = screenRects.back();

	viewPosX = first.x;
	viewSizeX = last.x + last.w - first.x;
	viewSizeY = first.h;

	dualViewPosX = dualScreenRect.x;
	dualViewSizeX = dualScreenRect.w;
	dualViewSizeY = dualScreenRect.h;

	// We store the offset in relation to window top border for sdl mouse translation
	viewWindowOffsetY = first.y;
	dualWindowOffsetY = dualScreenRect.y;

	// In-game and GL coords y orientation is inverse of SDL screen coords
	viewPosY = winSizeY - (viewSizeY + viewWindowOffsetY);
	dualViewPosY = winSizeY - (dualViewSizeY + dualWindowOffsetY);

	LOG("[GR::%s] Wind: pos %dx%d | size %dx%d", __func__, winPosX, winPosY, winSizeX, winSizeY);
	LOG("[GR::%s] View: pos %dx%d | size %dx%d | yoff %d", __func__,  viewPosX, viewPosY, viewSizeX, viewSizeY, viewWindowOffsetY);
	LOG("[GR::%s] Dual: pos %dx%d | size %dx%d | yoff %d", __func__,  dualViewPosX, dualViewPosY, dualViewSizeX, dualViewSizeY, dualWindowOffsetY);
}

void CGlobalRendering::UpdatePixelGeometry()
{
	pixelX = 1.0f / viewSizeX;
	pixelY = 1.0f / viewSizeY;

	aspectRatio = viewSizeX / float(viewSizeY);
}

void CGlobalRendering::ReadWindowPosAndSize()
{
#ifdef HEADLESS
	screenSizeX = 8;
	screenSizeY = 8;
	winSizeX = 8;
	winSizeY = 8;
	winPosX = 0;
	winPosY = 0;
	winBorder = { 0 };
#else

	SDL_Rect screenSize;
	GetDisplayBounds(screenSize);

	// no other good place to set these
	screenSizeX = screenSize.w;
	screenSizeY = screenSize.h;
	screenPosX  = screenSize.x;
	screenPosY  = screenSize.y;

	//probably redundant
	if (!borderless)
		UpdateWindowBorders(sdlWindow);

	SDL_GetWindowSize(sdlWindow, &winSizeX, &winSizeY);
	SDL_GetWindowPosition(sdlWindow, &winPosX, &winPosY);

	//enforce >=0 https://github.com/beyond-all-reason/spring/issues/23
	//winPosX = std::max(winPosX, 0);
	//winPosY = std::max(winPosY, 0);
#endif

	// should be done by caller
	// UpdateViewPortGeometry();
}

void CGlobalRendering::SaveWindowPosAndSize()
{
#ifdef HEADLESS
	return;
#endif

	if (fullScreen)
		return;

	// do not save if minimized
	// note that maximized windows are automagically restored; SDL2
	// apparently detects if the resolution is maximal and sets the
	// flag (but we also check if winRes equals maxRes to be safe)
	if ((SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_MINIMIZED) != 0)
		return;

	// do not notify about changes to block update loop
	configHandler->Set("WindowPosX", winPosX, false, false);
	configHandler->Set("WindowPosY", winPosY, false, false);
	configHandler->Set("XResolutionWindowed", winSizeX, false, false);
	configHandler->Set("YResolutionWindowed", winSizeY, false, false);
}


void CGlobalRendering::UpdateRendererConfigs()
{
	LOG("[GR::%s]", __func__);

	// re-read configuration value
	verticalSync->SetInterval();
}

void CGlobalRendering::UpdateScreenMatrices()
{
	// .x := screen width (meters), .y := eye-to-screen (meters)
	static float2 screenParameters = { 0.36f, 0.60f };

	const int remScreenSize = screenSizeY - winSizeY; // remaining desktop size (ssy >= wsy)
	const int bottomWinCoor = remScreenSize - winPosY; // *bottom*-left origin

	const float vpx = viewPosX + winPosX;
	const float vpy = viewPosY + bottomWinCoor;
	const float vsx = viewSizeX; // same as winSizeX except in dual-screen mode
	const float vsy = viewSizeY; // same as winSizeY
	const float ssx = screenSizeX;
	const float ssy = screenSizeY;
	const float hssx = 0.5f * ssx;
	const float hssy = 0.5f * ssy;

	const float zplane = screenParameters.y * (ssx / screenParameters.x);
	const float znear = zplane * 0.5f;
	const float zfar = zplane * 2.0f;
	constexpr float zfact = 0.5f;

	const float left = (vpx - hssx) * zfact;
	const float bottom = (vpy - hssy) * zfact;
	const float right = ((vpx + vsx) - hssx) * zfact;
	const float top = ((vpy + vsy) - hssy) * zfact;

	LOG("[GR::%s] vpx=%f, vpy=%f, vsx=%f, vsy=%f, ssx=%f, ssy=%f, screenPosX=%d, screenPosY=%d", __func__, vpx, vpy, vsx, vsy, ssx, ssy, screenPosX, screenPosY);

	// translate s.t. (0,0,0) is on the zplane, on the window's bottom-left corner
	screenViewMatrix = CMatrix44f{ float3{left / zfact, bottom / zfact, -zplane} };
	screenProjMatrix = CMatrix44f::ClipPerspProj(left, right, bottom, top, znear, zfar, supportClipSpaceControl * 1.0f);
}

void CGlobalRendering::UpdateWindowBorders(SDL_Window* window) const
{
#ifndef HEADLESS
	assert(window);

	SDL_GetWindowBordersSize(window, &winBorder[0], &winBorder[1], &winBorder[2], &winBorder[3]);

	#if defined(_WIN32) && (WINDOWS_NO_INVISIBLE_GRIPS == 1)
	// W/A for 8 px Aero invisible borders https://github.com/libsdl-org/SDL/commit/7c60bec493404905f512c835f502f1ace4eff003
	{
		auto scopedLib = spring::ScopedResource(
			LoadLibrary("dwmapi.dll"),
			[](HMODULE lib) { if (lib) FreeLibrary(lib); }
		);

		if (scopedLib == nullptr)
			return;

		using DwmGetWindowAttributeT = HRESULT WINAPI(
			HWND,
			DWORD,
			PVOID,
			DWORD
		);

		static auto* DwmGetWindowAttribute = reinterpret_cast<DwmGetWindowAttributeT*>(GetProcAddress(scopedLib, "DwmGetWindowAttribute"));

		if (!DwmGetWindowAttribute)
			return;

		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(window, &wmInfo);
		HWND& hwnd = wmInfo.info.win.window;

		RECT rect, frame;

		static constexpr DWORD DWMWA_EXTENDED_FRAME_BOUNDS = 9; // https://docs.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute
		DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT));
		GetWindowRect(hwnd, &rect);

		winBorder[0] -= std::max(0l, frame.top   - rect.top    );
		winBorder[1] -= std::max(0l, frame.left  - rect.left   );
		winBorder[2] -= std::max(0l, rect.bottom - frame.bottom);
		winBorder[3] -= std::max(0l, rect.right  - frame.right);

		LOG_L(L_DEBUG, "[GR::%s] Working around Windows 10+ thick borders SDL2 issue, borders are slimmed by TLBR(%d,%d,%d,%d)", __func__,
			static_cast<int>(std::max(0l, frame.top   - rect.top    )),
			static_cast<int>(std::max(0l, frame.left  - rect.left   )),
			static_cast<int>(std::max(0l, rect.bottom - frame.bottom)),
			static_cast<int>(std::max(0l, rect.right  - frame.right ))
		);
	}
	LOG_L(L_DEBUG, "[GR::%s] Storing window borders {%d, %d, %d, %d}", __func__, winBorder[0], winBorder[1], winBorder[2], winBorder[3]);
	#endif
#endif
}

void CGlobalRendering::UpdateRendererGeometry()
{
	LOG("[GR::%s][1] winSize=<%d,%d>", __func__, winSizeX, winSizeY);

	ReadWindowPosAndSize();
	UpdateViewPortGeometry();
	UpdatePixelGeometry();
	UpdateScreenMatrices();

	LOG("[GR::%s][2] winSize=<%d,%d>", __func__, winSizeX, winSizeY);
}

void CGlobalRendering::SetRendererStartState()
{
	RendererSetStartState();
	LogDisplayMode(sdlWindow);
}


int CGlobalRendering::DepthBitsToFormat(int bits)
{
	switch (bits)
	{
	case 16:
		return GL_DEPTH_COMPONENT16;
	case 24:
		return GL_DEPTH_COMPONENT24;
	case 32:
		return GL_DEPTH_COMPONENT32;
	default:
		return GL_DEPTH_COMPONENT; //should never hit this
	}
}


bool CGlobalRendering::SetWindowMinMaximized(bool maximize) const
{
	static constexpr uint32_t mmFlags[] = {
		SDL_WINDOW_MINIMIZED,
		SDL_WINDOW_MAXIMIZED
	};
	if ((SDL_GetWindowFlags(sdlWindow) & mmFlags[maximize]) != 0)
		return false; //already in desired state

	if (maximize)
		SDL_MaximizeWindow(sdlWindow);
	else
		SDL_MinimizeWindow(sdlWindow);

	return (SDL_GetWindowFlags(sdlWindow) & mmFlags[maximize]) != 0;
}
