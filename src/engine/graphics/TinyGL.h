#pragma once
#include <climits>

#include "TGLVert.h"

class TGLVert;

class Applet;

class TinyGL
{
public:
    Applet* app = nullptr;  // Set in startup(), replaces CAppContainer::getInstance()->app

    static constexpr int MATRIX_ONE = 16384;
    static constexpr int SCREEN_SHIFT = 3;
    static constexpr int SCREEN_ONE = 8;
    static constexpr int NEAR_CLIP = 256;
    static constexpr int CULL_EXTRA = NEAR_CLIP + 16;
    static constexpr int COLUMN_SCALE_INIT = INT_MAX;

    // External read/write surface — game / render code touches these.
    int imageBounds[4] = {};
    int sWidth = 0;
    int tHeight = 0;
    uint32_t textureBaseSize = 0;
    int screenWidth = 0;
    int* columnScale = nullptr;
    // 4x4 column-major matrices, regular float (no Q-format). Layout:
    // [0..2,4..6,8..10] = rotation/scale, [12..14] = translation in world
    // units, [3,7,11] = 0, [15] = 1 (or 0 for the perspective projection's
    // homogeneous w cell).
    float view[16] = {};
    float mvp[16] = {};
    TGLVert cv[32] = {};
    TGLVert mv[20] = {};
    int fogMin = 0;
    int fogRange = 0;
    int fogColor = 0;
    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    int viewportClampX1 = 0;
    int viewportClampX2 = 0;
    int viewX = 0;
    int viewY = 0;
    int viewZ = 0;
    int viewPitch = 0;

	// Constructor
	TinyGL();
	// Destructor
	~TinyGL();

	bool startup(int screenWidth);
    void setViewport(int x, int y, int w, int h);
    void resetViewPort();
    void setView(int viewX, int viewY, int viewZ, int viewYaw, int viewPitch, int viewRoll, int viewFov, int viewAspect);
    void viewMtxMove(TGLVert* tglVert, int n, int n2, int n3);
    void drawModelVerts(TGLVert* array, int n);
    TGLVert* transform3DVerts(TGLVert* array, int n);
    TGLVert* transform2DVerts(TGLVert* array, int n);
    bool clipLine(TGLVert* array);
    void projectVerts(TGLVert* array, int n);
    bool clippedLineVisCheck(TGLVert* tglVert, TGLVert* tglVert2, bool b);
    bool occludeClippedLine(TGLVert* tglVert, TGLVert* tglVert2);

private:
    // Internal state — written/read only by TinyGL.cpp.
    float view2D[16] = {};
    float projection[16] = {};
    float mvp2D[16] = {};
    int viewportX2 = 0;
    int viewportXScale = 0;
    int viewportXBias = 0;
    int viewportYScale = 0;
    int viewportYBias = 0;
    int viewYaw = 0;

    // Internal helpers — matrix construction for setView, viewport math.
    void buildViewMatrix(int x, int y, int z, int yaw, int pitch, int roll, float* matrix);
    void buildProjectionMatrix(int fov, int aspect, float* matrix);
    void multMatrix(const float* matrix1, const float* matrix2, float* destMtx);
    void _setViewport(int viewportX, int viewportY, int viewportWidth, int viewportHeight);
};
