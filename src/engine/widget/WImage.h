#pragma once
#include "Widget.h"
#include <string>

class WImage : public Widget {
public:
    std::string imageName;  // resolved at render time
    int anchor = 0;         // Graphics::ANCHORS_* flags
    int renderMode = 0;

    WImage();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
};
