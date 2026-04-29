
#include <stdexcept>
#include "Log.h"
#include "Error.h"

#include "SDLGL.h"
#include "App.h"
#include "CAppContainer.h"

SDLResVidModes sdlResVideoModes[18] = {{480, 320},   {640, 480},   {720, 400},  {720, 480},   {720, 576},
                                       {800, 600},   {832, 624},   {960, 640},  {1024, 768},  {1152, 864},
                                       {1152, 872},  {1280, 720},  {1280, 800}, {1280, 1024}, {1440, 900},
                                       {1600, 1000}, {1680, 1050}, {1920, 1080}};

SDLGL::SDLGL() {
	initialized = false;
}

SDLGL::~SDLGL() {
	if (this) {
		if (initialized && this->window) {
			SDL_SetWindowFullscreen(this->window, 0);
			SDL_GL_DeleteContext(glcontext);
			SDL_DestroyWindow(window);
			SDL_Quit();
		}
	}
}

bool SDLGL::InitializeHeadless() {
	if (!this->initialized) {
		this->window = nullptr;
		this->resolutionIndex = 0;
		this->oldResolutionIndex = -1;
		this->winVidWidth = Applet::IOS_WIDTH;
		this->winVidHeight = Applet::IOS_HEIGHT;
		this->vidWidth = Applet::IOS_WIDTH;
		this->vidHeight = Applet::IOS_HEIGHT;
		this->windowMode = 0;
		this->oldWindowMode = -1;
		this->vSync = false;
		this->oldVSync = false;
		this->initialized = true;
	}
	return this->initialized;
}

bool SDLGL::Initialize() {
	Uint32 flags;

	if (!this->initialized) {

		SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			LOG_ERROR("SDL_Init failed: {}\n", SDL_GetError());
			return false;
		}

		flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /* | SDL_WINDOW_RESIZABLE*/;
		// Set the highdpi flags - this makes a big difference on Macs with
		// retina displays, especially when using small window sizes.
		flags |= SDL_WINDOW_ALLOW_HIGHDPI;

		this->oldResolutionIndex = -1;
		this->resolutionIndex = 0;
		this->winVidWidth = sdlResVideoModes[this->resolutionIndex].width;   // Applet::IOS_WIDTH*2;
		this->winVidHeight = sdlResVideoModes[this->resolutionIndex].height; // Applet::IOS_HEIGHT*2;

		// this->winVidWidth = 1440;
		// this->winVidHeight = 900;

		this->window =
		    SDL_CreateWindow(CAppContainer::getInstance()->gameConfig.windowTitle.c_str(), SDL_WINDOWPOS_UNDEFINED,
		                     SDL_WINDOWPOS_UNDEFINED, winVidWidth, winVidHeight, flags);
		if (!this->window) {
			LOG_ERROR("SDL_CreateWindow failed ({}x{}): {}\n", winVidWidth, winVidHeight, SDL_GetError());
			return false;
		}

		this->vidWidth = Applet::IOS_WIDTH;
		this->vidHeight = Applet::IOS_HEIGHT;
		this->oldWindowMode = -1;
		this->windowMode = 0;
		this->oldVSync = false;
		this->vSync = true;

		// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
		// SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		// SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");//sdlVideo.vSync ? "1" : "0");
		this->updateVideo();

		this->glcontext = SDL_GL_CreateContext(window);
		if (!this->glcontext) {
			LOG_ERROR("SDL_GL_CreateContext failed: {}\n", SDL_GetError());
			return false;
		}

		// now you can make GL calls.
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(this->window);

		// Check for joysticks
		SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
		this->initialized = true;
	}

	return this->initialized;
}

#include <stdarg.h> //va_list|va_start|va_end

void SDLGL::Error(const char* fmt, ...) {
	char errMsg[256];
	va_list ap;
	va_start(ap, fmt);
#ifdef _WIN32
	vsprintf_s(errMsg, sizeof(errMsg), fmt, ap);
#else
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
#endif
	va_end(ap);

	// Route through the global Error() which handles logging, crash.log, and MessageBox
	::Error("%s", errMsg);
}

void SDLGL::getContentRect(int* x, int* y, int* w, int* h) {
	int dw, dh;
	SDL_GL_GetDrawableSize(this->window, &dw, &dh);
	const float windowAspect = (float)dw / (float)dh;
	const float canvasAspect = (float)Applet::IOS_WIDTH / (float)Applet::IOS_HEIGHT;
	if (windowAspect > canvasAspect) {
		// Window is wider — pillarbox (vertical bars on left/right).
		*h = dh;
		*w = (int)((float)dh * canvasAspect + 0.5f);
		*x = (dw - *w) / 2;
		*y = 0;
	} else {
		// Window is taller — letterbox (horizontal bars on top/bottom).
		*w = dw;
		*h = (int)((float)dw / canvasAspect + 0.5f);
		*x = 0;
		*y = (dh - *h) / 2;
	}
}

// Scale logical-canvas coordinates (480×320 space) to the centered aspect-correct
// content rect within the drawable. Pure float scalar multiply — no offset added —
// because gles::BeginFrame relies on this being a scalar multiply (it casts int* to
// float* and depends on small-int float bit patterns surviving a single multiply).
// Use getContentRect() directly when you actually need the centering offset.
void SDLGL::transformCoord2f(float* x, float* y) {
	int cx, cy, cw, ch;
	this->getContentRect(&cx, &cy, &cw, &ch);
	(void)cx;
	(void)cy;
	*x *= (float)cw / (float)Applet::IOS_WIDTH;
	*y *= (float)ch / (float)Applet::IOS_HEIGHT;
}

void SDLGL::centerMouse(int x, int y) {
	int cx, cy, cw, ch;
	this->getContentRect(&cx, &cy, &cw, &ch);
	int targetX = cx + (Applet::IOS_WIDTH / 2 + x) * cw / Applet::IOS_WIDTH;
	int targetY = cy + (Applet::IOS_HEIGHT / 2 + y) * ch / Applet::IOS_HEIGHT;
	SDL_WarpMouseInWindow(this->window, targetX, targetY);
}

void SDLGL::updateWinVid(int w, int h) {
	this->winVidWidth = w;
	this->winVidHeight = h;
}

void SDLGL::updateVideo() {
	if (this->vSync != this->oldVSync) {
		SDL_GL_SetSwapInterval(this->vSync ? SDL_TRUE : SDL_FALSE);
		this->oldVSync = this->vSync;
	}

	if (this->windowMode != this->oldWindowMode) {
		SDL_SetWindowFullscreen(this->window, 0);
		SDL_SetWindowBordered(this->window, SDL_TRUE);

		if (this->windowMode == 1) {
			SDL_SetWindowBordered(this->window, SDL_FALSE);
		} else if (this->windowMode == 2) {
			SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN);
		}
		this->oldWindowMode = this->windowMode;
	}

	if (this->resolutionIndex != this->oldResolutionIndex) {
		SDL_SetWindowSize(this->window, sdlResVideoModes[this->resolutionIndex].width,
		                  sdlResVideoModes[this->resolutionIndex].height);
		SDL_SetWindowPosition(this->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		this->updateWinVid(sdlResVideoModes[this->resolutionIndex].width,
		                   sdlResVideoModes[this->resolutionIndex].height);
		this->oldResolutionIndex = this->resolutionIndex;
	}

	int winVidWidth, winVidHeight;
	SDL_GetWindowSize(this->window, &winVidWidth, &winVidHeight);
	this->updateWinVid(winVidWidth, winVidHeight);
}

void SDLGL::restore() {
	if (this->vSync != this->oldVSync) {
		this->vSync = this->oldVSync;
	}

	if (this->windowMode != this->oldWindowMode) {
		this->windowMode = this->oldWindowMode;
	}

	if (this->resolutionIndex != this->oldResolutionIndex) {
		this->resolutionIndex = this->oldResolutionIndex;
	}
}
