#pragma once
#include <span>
#include "SDLGL.h"
#include "Shader.h"

using PFNGLACTIVETEXTUREPROC = void (APIENTRYP)(GLenum texture);
using PFNGLCLIENTACTIVETEXTUREPROC = void (APIENTRYP)(GLenum texture);

class Render;
class TinyGL;
class TGLVert;

struct glChain {
	glChain* next;
	glChain* prev;
	GLuint texnum;
	GLuint width;
	GLuint height;
};

struct Vertex {
	float xyzw[4];
	float st[2];
};

struct GLVert {
	int x;
	int y;
	int z;
	int w;
	int s;
	int t;
};

class Applet;

class gles
{
public:
	Applet* app = nullptr;  // Set in startup(), replaces CAppContainer::getInstance()->app
	bool headless = false;
	SDLGL* sdlGL = nullptr;
	bool isInit = false;
	static constexpr int scale = 1;
	static constexpr int MAX_GLVERTS = 16;
	static constexpr int MAX_MEDIA = 1024;

private:
	Render* render = nullptr;
	TinyGL* tinyGL = nullptr;
	int activeTexels = 0;
	Vertex immediate[MAX_GLVERTS] = {};
	uint16_t quad_indexes[42] = {};
	glChain chains[MAX_MEDIA] = {};
	glChain activeChain = {};
	float fogScale = 0.0f;
	float modelViewMatrix[MAX_GLVERTS] = {};
	float projectionMatrix[MAX_GLVERTS] = {};
	int vPortRect[4] = {};
	int fogMode = 0;
	int renderMode = 0;
	int flags = 0;
	float fogStart = 0.0f;
	float fogEnd = 0.0f;
	float fogColor[4] = {};
	float fogBlack[4] = {};
public:
	// Programmable-pipeline migration (B2). Once compiled and validated, these
	// shaders replace the fixed-function calls (glMatrixMode, glColor4f,
	// glFog*, glTexEnvi, glVertexPointer) one mesh category at a time. While
	// migrating, isShaderReady gates which path each draw uses.
	//   textureShader — flat 2D textured quads (HUD/menus/sprites). B2.2.
	//   worldShader   — perspective 3D with per-pixel linear fog.        B2.3+
	Shader textureShader;
	Shader worldShader;
	bool isShaderReady = false;

	// Per-mesh state captured by SetupTexture(). The shader path reads these
	// instead of glGetFloatv(GL_CURRENT_COLOR), since (a) it's faster than a
	// pipeline stall and (b) some renderMode branches (e.g. RENDER_NONE) leave
	// glColor untouched, so we can't trust the GL state to reflect the current
	// SetupTexture call.
	//
	// meshAddColor: additive RGB tint set by TexCombineShift (the non-multiply
	// J2ME RED/GREEN/BLUE shifts). Replaces fixed-function GL_COMBINE_RGB +
	// GL_ADD path which shaders cannot read. Zero in all other cases.
	float meshColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float meshAddColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float meshFogColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	bool meshFogEnabled = false;

	// Constructor
	gles();
	// Destructor
	~gles();

	void WindowInit();
	void SwapBuffers();
	void GLInit(Render* render);
	bool ClearBuffer(int color);
	void SetGLState();
	void BeginFrame(int x, int y, int w, int h, int* mtxView, int* mtxProjection);
	void ResetGLState();
	void CreateFadeTexture(int mediaID);
	void CreateAllActiveTextures();
	bool RasterizeConvexPolygon(std::span<TGLVert> verts);
	bool RasterizeConvexPolygon(std::span<GLVert> verts);
	void UnloadSkyMap();
	void ResetTextureChains();
	bool DrawWorldSpaceSpriteLine(TGLVert* vert1, TGLVert* vert2, TGLVert* vert3, int flags);
	bool DrawModelVerts(std::span<TGLVert> verts);
	void SetupTexture(int n, int n2, int renderMode, int flags);
	void CreateTextureForMediaID(int n, int mediaID, bool b);
	// Get OpenGL texture ID for a media index. If create=false, only returns already-loaded textures.
	GLuint getTextureID(int tileNum, int frame = 0, bool create = true);
	void getTextureDims(int tileNum, int frame, int& w, int& h);
	bool DrawSkyMap();
	void DrawPortalTexture(Image* img, int x, int y, int w, int h, float tx, float ty, float scale, float angle, char mode);
	void TexCombineShift(int r, int g, int b); // [GEC]
};

extern gles* _glesObj;
