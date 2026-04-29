#pragma once
#include <climits>

#include "TGLVert.h"
#include "TGLEdge.h"

class TGLVert;
class TGLEdge;

class Applet;

class TinyGL
{
private:

public:
    Applet* app = nullptr;  // Set in startup(), replaces CAppContainer::getInstance()->app

    static constexpr int UNIT_SCALE = 65536;
    static constexpr int MATRIX_ONE = 16384;
    static constexpr int SCREEN_SHIFT = 3;
    static constexpr int SCREEN_ONE = 8;
    static constexpr int NEAR_CLIP = 256;
    static constexpr int CULL_EXTRA = NEAR_CLIP + 16;
    static constexpr int COLUMN_SCALE_INIT = INT_MAX;

    int imageBounds[4] = {};
    int sWidth = 0;
    int tHeight = 0;
    uint32_t textureBaseSize = 0;
    int screenWidth = 0;
    int screenHeight = 0;
    int* columnScale = nullptr;
    int view[16] = {};
    int view2D[16] = {};
    int projection[16] = {};
    int mvp[16] = {};
    int mvp2D[16] = {};
    TGLVert cv[32] = {};
    TGLVert mv[20] = {};
    TGLEdge edges[2] = {};
    int fogMin = 0;
    int fogRange = 0;
    int fogColor = 0;
    int countBackFace = 0;
    int countDrawn = 0;
    int spanPixels = 0;
    int spanCalls = 0;
    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    int viewportClampX1 = 0;
    int viewportClampY1 = 0;
    int viewportClampX2 = 0;
    int viewportClampY2 = 0;
    int viewportX2 = 0;
    int viewportY2 = 0;
    int viewportXScale = 0;
    int viewportXBias = 0;
    int viewportYScale = 0;
    int viewportYBias = 0;
    int viewportZScale = 0;
    int viewportZBias = 0;
    int viewX = 0;
    int viewY = 0;
    int viewZ = 0;
    int viewYaw = 0;
    int viewPitch = 0;
    int c_totalQuad = 0;
    int c_clippedQuad = 0;

	// Constructor
	TinyGL();
	// Destructor
	~TinyGL();

	bool startup(int screenWidth, int screenHeight);
    void clearColorBuffer(int color);
    void buildViewMatrix(int x, int y, int z, int yaw, int pitch, int roll, int* matrix);
    void buildProjectionMatrix(int fov, int aspect, int* matrix);
    void multMatrix(int* matrix1, int* matrix2, int* destMtx);
    void _setViewport(int viewportX, int viewportY, int viewportWidth, int viewportHeight);
    void setViewport(int x, int y, int w, int h);
    void resetViewPort();
    void setView(int viewX, int viewY, int viewZ, int viewYaw, int viewPitch, int viewRoll, int viewFov, int viewAspect);
    void viewMtxMove(TGLVert* tglVert, int n, int n2, int n3);
    void drawModelVerts(TGLVert* array, int n);
    TGLVert* transform3DVerts(TGLVert* array, int n);
    TGLVert* transform2DVerts(TGLVert* array, int n);
    void ClipQuad(TGLVert* tglVert, TGLVert* tglVert2, TGLVert* tglVert3, TGLVert* tglVert4);
    void ClipPolygon(int i, int n);
    bool clipLine(TGLVert* array);
    void projectVerts(TGLVert* array, int n);
    void RasterizeConvexPolygon(int n);
    bool clippedLineVisCheck(TGLVert* tglVert, TGLVert* tglVert2, bool b);
    bool occludeClippedLine(TGLVert* tglVert, TGLVert* tglVert2);
    void resetCounters();
};
