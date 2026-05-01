#pragma once
#include <SDL_opengl.h>

class IDIB;

class Applet;

class Image
{
private:

public:
	Applet* app;  // Set lazily, replaces CAppContainer::getInstance()->app
	bool headless = false;
	IDIB* piDIB;
	int width;
	int height;
	int depth;
	bool isTransparentMask;
	int texWidth;
	int texHeight;
	GLuint texture;

	// Color modulator captured by setRenderMode(). The shader path reads this
	// instead of glGetFloatv(GL_CURRENT_COLOR), since GL_REPLACE modes (0/3)
	// don't bother setting glColor4f — the fixed-function pipeline ignores it
	// in REPLACE mode, but the shader always multiplies texColor * u_color and
	// would pick up stale state from a previous mode-8 (text) draw.
	float modColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	// Constructor
	Image();
	// Destructor
	~Image();

	void CreateTexture(uint16_t* data, uint32_t width, uint32_t height);
	void DrawTexture(int texX, int texY, int texW, int texH, int posX, int posY, int rotateMode, int renderMode);
	void DrawTextureAlpha(int posX, int posY, float alpha, bool rotated, bool flipUV);
	void setRenderMode(int renderMode);
};
