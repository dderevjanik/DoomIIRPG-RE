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
    bool drawBackground = true;    // if false and no image, draw nothing (text-only)
    std::string imgNormalName;
    std::string imgHighlightName;
    int renderMode = 0;
    int highlightRenderMode = 0;
    bool animateFocus = false;     // pulse text when focused
    int animatePeriodMs = 600;     // animation period in ms
    int textAlign = 0;             // 0=center, 1=left, 2=right
    int focusOffsetX = 0;          // pixel shift applied to text when focused
    std::string focusPrefix;       // string prepended to text when focused (e.g. ">")
    bool focusCursor = false;      // draw animated cursor glyph to the left of text when focused
    std::string actionName;

    std::function<void()> onClick;

    WButton();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;
};
