#pragma once
#include <climits>

#include "TGLVert.h"

class TGLVert;

class Applet;

// Legacy CPU vertex transform / clip / occlusion pipeline. Most of TinyGL has
// been migrated into Render (B3.x); what remains is the visibility pipeline
// (transform/clip/project/visCheck/occlude) used for the BSP traversal and
// automap fog-of-war. State (view/mvp/viewport) lives on Render now.
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
    int screenWidth = 0;
    int* columnScale = nullptr;
    TGLVert cv[32] = {};
    TGLVert mv[20] = {};

	// Constructor
	TinyGL();
	// Destructor
	~TinyGL();

	bool startup(int screenWidth);
    void viewMtxMove(TGLVert* tglVert, int n, int n2, int n3);
    TGLVert* transform3DVerts(TGLVert* array, int n);
    TGLVert* transform2DVerts(TGLVert* array, int n);
    bool clipLine(TGLVert* array);
    void projectVerts(TGLVert* array, int n);
    bool clippedLineVisCheck(TGLVert* tglVert, TGLVert* tglVert2, bool b);
    bool occludeClippedLine(TGLVert* tglVert, TGLVert* tglVert2);
};
