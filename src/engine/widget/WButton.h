#pragma once
#include "Widget.h"
#include <string>
#include <functional>

class WButton : public Widget {
public:
    std::string text;
    int stringId = -1;
    int color = 0xFFFFFFFF;
    int highlightColor = 0xFF3366FF;
    Image* imgNormal = nullptr;
    Image* imgHighlight = nullptr;
    std::string actionName;

    std::function<void()> onClick;

    WButton();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;
};
