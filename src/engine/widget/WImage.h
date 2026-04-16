#pragma once
#include "Widget.h"

class WImage : public Widget {
public:
    Image* image = nullptr;
    int anchor = 0;         // Graphics::ANCHORS_* flags
    int renderMode = 0;

    WImage();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
};
