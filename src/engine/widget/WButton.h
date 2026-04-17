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
    int bgColor = 0xFF333333;
    int borderColor = 0xFF666666;
    std::string imgNormalName;
    std::string imgHighlightName;
    std::string actionName;

    std::function<void()> onClick;

    WButton();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;
};
