#pragma once
#include <SDL.h>
#include <SDL_opengl.h>

struct SDLResVidModes {
	int width, height;
};
extern SDLResVidModes sdlResVideoModes[18];

class SDLGL
{
private:
	bool initialized;
	SDL_GLContext glcontext;

public:
	SDL_Window* window;
	int resolutionIndex;
	int oldResolutionIndex;
	int winVidWidth, winVidHeight;
	int vidWidth, vidHeight;
	int windowMode, oldWindowMode;
	bool vSync, oldVSync;

	SDLGL();
	// Destructor
	~SDLGL();
	bool Initialize();
	bool InitializeHeadless();
	void Error(const char* fmt, ...);

	void transformCoord2f(float* x, float* y);
	// Aspect-correct centered content rect inside the drawable window. The 480×320
	// logical canvas is letterboxed/pillarboxed to preserve its 3:2 aspect on any
	// window. Outputs are in drawable-pixel coordinates.
	void getContentRect(int* x, int* y, int* w, int* h);
	// Same math but for window-pixel coords (use with mouse events, which arrive in
	// window-pixel space rather than drawable pixels — the two differ on Retina).
	void getContentRectInWindow(int* x, int* y, int* w, int* h);
	// Static math helper: compute the largest 3:2-aspect centered rect inside
	// (totalW × totalH). Used by both getContentRect variants and by anyone who
	// already has the dimensions in hand.
	static void computeContentRect(int totalW, int totalH, int* x, int* y, int* w, int* h);
	void centerMouse(int x, int y);
	void updateWinVid(int w, int h);
	void updateVideo();
	void restore();
};
