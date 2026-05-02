#pragma once
#include <climits>

#include "TGLVert.h"

class Applet;

// Vestigial holder for the legacy CPU pipeline constants. All methods and
// state have moved to Render (B3.x); the only thing keeping TinyGL alive
// today is `mv[20]` (still 70+ external refs in render code as a scratch
// buffer) and the constants below (referenced from RenderView.cpp and
// RenderBSP.cpp). To be deleted in B3.14c + B3.15.
class TinyGL
{
public:
    Applet* app = nullptr;  // Set in startup(); points at the singleton Applet.

    static constexpr int MATRIX_ONE = 16384;
    static constexpr int SCREEN_SHIFT = 3;
    static constexpr int SCREEN_ONE = 8;
    static constexpr int NEAR_CLIP = 256;
    static constexpr int CULL_EXTRA = NEAR_CLIP + 16;
    static constexpr int COLUMN_SCALE_INIT = INT_MAX;

    // Sprite/wall vertex scratch — populated by RenderSprite/RenderBSP, fed
    // into Render::transform3DVerts/transform2DVerts.
    TGLVert mv[20] = {};

    TinyGL();
    ~TinyGL();

    bool startup(int screenWidth);
};
