#pragma once
#include "Widget.h"
#include <string>

class WLabel : public Widget {
public:
    std::string text;
    int stringId = -1;  // localized string ID, -1 = use literal text
    int color = 0xFFFFFFFF;
    int anchor = 0;     // Graphics::ANCHORS_* flags

    WLabel();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
};
