#pragma once
#include <span>
#include "SDLGL.h"

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
	Applet* app;  // Set in startup(), replaces CAppContainer::getInstance()->app
	bool isInit;
	static constexpr int scale = 1;
	static constexpr int MAX_GLVERTS = 16;
	static constexpr int MAX_MEDIA = 1024;

private:
	Render* render;
	TinyGL* tinyGL;
	int activeTexels;
	Vertex immediate[MAX_GLVERTS];
	uint16_t quad_indexes[42];
	glChain chains[MAX_MEDIA];
	glChain activeChain;
	float fogScale;
	float modelViewMatrix[MAX_GLVERTS];
	float projectionMatrix[MAX_GLVERTS];
	int vPortRect[4];
	int fogMode;
	int renderMode;
	int flags;
	float fogStart;
	float fogEnd;
	float fogColor[4];
	float fogBlack[4];
public:

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
