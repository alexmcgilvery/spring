/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_RENDERING_H
#define _GLOBAL_RENDERING_H

#include <string>
#include <memory>
#include <array>

#include "System/Matrix44f.h"
#include "System/creg/creg_cond.h"
#include "System/Misc/SpringTime.h"
#include "System/UnorderedSet.hpp"
#include "System/type2.h"

struct IRendererCore;

struct SDL_version;
struct SDL_Rect;
struct SDL_Window;
typedef void* SDL_GLContext;

/**
 * @brief Globally accessible unsynced, rendering related data
 *
 * Contains globally accessible rendering related data
 * that does not remain synced.
 */
class CGlobalRendering {
	CR_DECLARE_STRUCT(CGlobalRendering)
protected:

	/**
	 * @brief Pre-window initialization function.
	 *
	 * This function is called before the window is created.
	 * It can be used to initialize any necessary resources by the renderer required before window creation.
	 */
	virtual void RendererPreWindowInit() = 0;

	/**
	 * @brief Post-window initialization function.
	 *
	 * This function is called after the window has been created.
	 * It can be used to perform any additional initialization or setup not critical immediately follow window creation.
	 */
	virtual void RendererPostWindowInit() = 0;

	/**
	 * @brief Function for creating a window with a given title.
	 *
	 * Renderer Implementation should do the following:
	 * 1. Verify necessary features for graphical window are present.
	 * 2. Call Superclass' CreateSDLWindow
	 * 3. Set SDL_SetHint for necessary configs
	 * 4. Call Superclass' SetWindowAttributes
	 * 5. Do initialization that is required immediately after creating
	 *
	 * @param title The title of the window to create.
	 * @return boolean result representing if window creation was successful
	 */
	virtual bool RendererCreateWindow(const char* title) = 0;

	/**
	 * @brief Function for creating an SDL window with a given title.
	 *
	 * Renderer Implementation should do the following:
	 * 1.Prepare necessary sdl flags for window creation.
	 * 2.Call SDL_CreateWindow with given flags
	 *
	 * @param title The title of the window to create.
	 * @return A pointer to the created window.
	 */
	virtual SDL_Window* RendererCreateSDLWindow(const char* title) = 0;

	/**
	 * @brief Function for setting the start state of the renderer.
	 *
	 * This function is called to let renderer set intial state for the first frame.
	 */
	virtual void RendererSetStartState() = 0;

	/**
	 * @brief Function for destroying window resources.
	 *
	 * This function should be used to destroy renderer resources immediately prior to window destruction.
	 */
	virtual void RendererDestroyWindow() = 0;

	/**
	 * @brief Function for updating a window.
	 *
	 * This function lets the renderer know a window update is started.
	 * This could mean a resize event or a new frame.
	 */
	virtual void RendererUpdateWindow() = 0;

	/**
	 * @brief Function for presenting a frame.
	 *
	 * This function presents a frame, which includes swapping buffers and clearing any errors.
	 * Renderer Implementation should do the following:
	 * 1. Clear renderer errors if clearErrors is true.
	 * 2. Call RenderBuffer impl swapbuffers function.
	 * 3. Call Stream Buffers impl lock function.
	 * 4. API specific frame presention.
	 *
	 * @param allowSwapBuffers Whether to allow the swap buffers operation.
	 * @param clearErrors Whether to clear any errors that may have occurred during rendering.
	 */
	virtual void RendererPresentFrame(bool allowSwapBuffers, bool clearErrors) = 0;

	SDL_Window* CreateSDLWindow(const char* title);

public:
	virtual void UpdateViewport() = 0;

	virtual void SetTimeStamp(uint32_t queryIdx) const = 0;
	virtual uint64_t CalculateFrameTimeDelta(uint32_t queryIdx0, uint32_t queryIdx1) const = 0;

	virtual void ToggleMultisampling() const = 0;
	virtual bool ToggleDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const = 0;

	virtual void AquireThreadContext() = 0; //FIXME thread hack for GL
	virtual void ReleaseThreadContext() = 0; //FIXME thread hack for GL
public:
	CGlobalRendering();
	~CGlobalRendering();

	void PreKill();

	/**
	 * @return whether setting the video mode was successful
	 *
	 * Sets SDL video mode options/settings
	 */
	bool CreateWindow(const char* title);
	SDL_Window* GetWindow() { return sdlWindow; }

	void DestroyWindow();
	void KillSDL() const;
	void PostWindowInit();

	void SwapBuffers(bool allowSwapBuffers, bool clearErrors);

	void GetAllDisplayBounds(SDL_Rect& r) const;

	void GetWindowPosSizeBounded(int& x, int& y, int& w, int& h) const;

	void SetWindowTitle(const std::string& title);
	void SetWindowAttributes(SDL_Window* window);

	void UpdateWindow();
	void UpdateTimer();

	void ConfigNotify(const std::string& key, const std::string& value);

	bool GetWindowInputGrabbing();
	bool SetWindowInputGrabbing(bool enable);
	bool ToggleWindowInputGrabbing();

	bool SetWindowPosHelper(int displayIdx, int winRPosX, int winRPosY, int winSizeX_, int winSizeY_, bool fs, bool bl) const;

	bool SetWindowMaximized() const { return SetWindowMinMaximized(true ); };
	bool SetWindowMinimized() const { return SetWindowMinMaximized(false); };

	void SetFullScreen(bool cliWindowed, bool cliFullScreen);
	void SetDualScreenParams();
	void UpdateViewPortGeometry();
	void UpdatePixelGeometry();
	void ReadWindowPosAndSize();
	void SaveWindowPosAndSize();
	void UpdateRendererConfigs();
	void UpdateRendererGeometry();
	void UpdateScreenMatrices();
	void LogDisplayMode(SDL_Window* window) const;

	void UpdateWindowBorders(SDL_Window* window) const;

	int2 GetMaxWinRes() const;
	int2 GetCfgWinRes() const;

	int GetCurrentDisplayIndex() const;
	void GetDisplayBounds(SDL_Rect& r, const int* di = nullptr) const;
	void GetUsableDisplayBounds(SDL_Rect& r, const int* di = nullptr) const;

	bool CheckGLMultiSampling() const;
	bool CheckGLContextVersion(const int2& minCtx) const;
	bool ToggleGLDebugOutput(unsigned int msgSrceIdx, unsigned int msgTypeIdx, unsigned int msgSevrIdx) const;
	void InitGLState();
	void ToggleMultisampling() const;

	bool CheckShaderGL4() const;
public:
	//helper function
	static int DepthBitsToFormat(int bits);
public:
	/**
	 * @brief time offset
	 *
	 * Time in number of frames since last update
	 * (for interpolation)
	 */
	float timeOffset;
	float lastTimeOffset;
	/**
	 * @brief last frame time
	 *
	 * How long the last draw cycle took in real time (MILLIseconds)
	 */
	float lastFrameTime;

	/// the starting time in tick for last draw frame
	spring_time lastFrameStart;

	spring_time lastSwapBuffersEnd;

	/// 0.001f * gu->simFPS, used for rendering
	float weightedSpeedFactor;

	/// the draw frame number (never 0)
	unsigned int drawFrame;

	/// Frames Per Second
	float FPS;

	/// the number of displays
	int numDisplays;

	/// the screen size in pixels
	int screenSizeX;
	int screenSizeY;

	/// the screen offsets in pixels (in case display is not the first one)
	int screenPosX;
	int screenPosY;

	/// the window position relative to the screen's top-left corner
	int winPosX;
	int winPosY;

	/// the window size in pixels
	int winSizeX;
	int winSizeY;

	/// the viewport position relative to the window's top-left corner
	int viewPosX;
	int viewPosY;

	/// the viewport size in pixels
	int viewSizeX;
	int viewSizeY;

	/// the y offset in relation to window top border
	int viewWindowOffsetY;
	int dualWindowOffsetY;

	/// the dual viewport position relative to the window's top-left corner (DualScreenMode = 1)
	int dualViewPosX;
	int dualViewPosY;

	/// the dual viewport size in pixels (DualScreenMode = 1)
	int dualViewSizeX;
	int dualViewSizeY;

	/// the window borders
	mutable std::array<int, 4> winBorder;

	/// Some settings got changed need to adjust the way window is
	unsigned int winChgFrame;
	unsigned int gmeChgFrame;

	/// screen {View,Proj} matrices for rendering in pixel coordinates
	CMatrix44f screenViewMatrix;
	CMatrix44f screenProjMatrix;

	spring_time grTime;

	/// size of one pixel in viewport coordinates, i.e. 1/viewSizeX and 1/viewSizeY
	float pixelX;
	float pixelY;

	float minViewRange;
	float maxViewRange;
	/**
	 * @brief aspect ratio
	 *
	 * (float)viewSizeX / (float)viewSizeY
	 */
	float aspectRatio;

	int forceDisablePersistentMapping;
	int forceDisableShaders;
	int forceSwapBuffers;

	/**
	 * @brief MSAA
	 *
	 * Level of multisample anti-aliasing
	 */
	int msaaLevel;
	float minSampleShadingRate;

	/**
	 * @brief maxTextureSize
	 *
	 * maximum 2D texture size
	 */
	int maxTextureSize;
	int maxTexSlots;
	int maxFragShSlots;
	int maxCombShSlots;

	float maxTexAnisoLvl;


	bool drawSky;
	bool drawWater;
	bool drawGround;
	bool drawMapMarks;

	/**
	 * @brief draw fog
	 *
	 * Whether fog (of war) is drawn or not
	 */
	bool drawFog;

	/**
	 * @brief draw debug
	 *
	 * Whether debugging info is drawn
	 */
	bool drawDebug;
	bool drawDebugTraceRay;
	bool drawDebugCubeMap;

	/**
	 * Does the user want team colored nanospray?
	 */
	bool teamNanospray;


	/**
	 * @brief active video
	 *
	 * Whether the graphics need to be drawn
	 */
	bool active;

	/**
	 * @brief compressTextures
	 *
	 * If set, many (not all) textures will compressed on run-time.
	*/
	bool compressTextures;

	/**
	 * @brief GPU driver's vendor
	 *
	 * These can be used to enable workarounds for bugs in their drivers.
	 * Note, you should always give the user the possibility to override such workarounds via config-tags.
	 */
	bool haveAMD;
	bool haveMesa;
	bool haveIntel;
	bool haveNvidia;

	/**
	 * @brief collection of some ATI bugfixes
	 *
	 * enables some ATI bugfixes
	 */
	bool amdHacks;

	/**
	* @brief whether the GPU supports persistent buffer mapping
	*
	* ARB_buffer_storage or OpenGL 4.4
	*/
	bool supportPersistentMapping;

	// GLAD_GL_ARB_explicit_attrib_location
	bool supportExplicitAttribLoc;

	/**
	 * @brief if the GPU (drivers) support NonPowerOfTwoTextures
	 *
	 * Especially some ATI cards report that they support NPOTs, but don't (or just very limited).
	 */
	bool supportTextureQueryLOD;

	bool supportMSAAFrameBuffer;

	int supportDepthBufferBitDepth;

	bool supportRestartPrimitive;
	bool supportClipSpaceControl;
	bool supportSeamlessCubeMaps;
	bool supportFragDepthLayout;

	/**
	 * Shader capabilities
	 */
	bool supportUniformData;
	bool supportModelUniformData;

	bool haveGLSL;
	bool haveGL4;

	/**
	 * Shader capabilities
	 */
	int glslMaxVaryings;
	int glslMaxAttributes;
	int glslMaxDrawBuffers;
	int glslMaxRecommendedIndices;
	int glslMaxRecommendedVertices;
	int glslMaxUniformBufferBindings;
	int glslMaxUniformBufferSize; ///< in bytes
	int glslMaxStorageBufferBindings;
	int glslMaxStorageBufferSize; ///< in bytes

	/**
	 * @brief dual screen mode
	 * In dual screen mode, the screen is split up between a game screen and a minimap screen.
	 * In this case viewSizeX is half of the actual GL viewport width,
	 */
	bool dualScreenMode;

	/**
	 * @brief dual screen minimap on left
	 * In dual screen mode, allow the minimap to either be shown on the left or the right display.
	 * Minimap on the right is the default.
	 */
	bool dualScreenMiniMapOnLeft;

	/**
	 * @brief full-screen or windowed rendering
	 */
	bool fullScreen;
	bool borderless;

	bool underExternalDebug;
	/**
	 * @brief renderer debug mode active
	 */
	bool rendererDebug;
	bool rendererDebugErrors;

public:
	SDL_Window* sdlWindow;

public:
	/**
	* @brief maximum texture unit number
	*/
	static constexpr int MAX_TEXTURE_UNITS = 32;
	/**
	* @brief max view range in elmos
	*/
	static constexpr float MAX_VIEW_RANGE = 65536.0f;

	/**
	* @brief near z-plane distance in elmos
	*/
	static constexpr float MIN_ZNEAR_DIST = 0.5f;


	/// magic constant to reduce overblending on SMF maps
	/// (scales the MapInfo::light_t::ground*Color values;
	/// roughly equal to 210.0f / 255.0f)
	static constexpr float SMF_INTENSITY_MULT = (210.0f / 256.0f) + (1.0f / 256.0f) - (1.0f / 2048.0f) - (1.0f / 4096.0f);

	//minimum window resolution in non-fullscreen mode
	static constexpr int2 minRes = { 400, 400 };


	static constexpr uint32_t NUM_OPENGL_TIMER_QUERIES = 8;
	static constexpr uint32_t FRAME_REF_TIME_QUERY_IDX = 0;
	static constexpr uint32_t FRAME_END_TIME_QUERY_IDX = NUM_OPENGL_TIMER_QUERIES - 1;

private:
	void SetMinSampleShadingRate();
	bool SetWindowMinMaximized(bool maximize) const;

private:
	static constexpr inline const char* xsKeys[2] = { "XResolutionWindowed", "XResolution" };
	static constexpr inline const char* ysKeys[2] = { "YResolutionWindowed", "YResolution" };
};

extern CGlobalRendering* globalRendering;

#endif /* _GLOBAL_RENDERING_H */

