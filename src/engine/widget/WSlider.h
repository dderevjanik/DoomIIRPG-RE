#pragma once
#include "Widget.h"
#include <string>
#include <functional>

class WSlider : public Widget {
public:
    std::string text;
    int stringId = -1;
    int color = 0xFFFFFFFF;
    int highlightColor = 0xFF3366FF;
    int trackColor = 0xFF444444;
    int fillColor = 0xFF00AA00;
    int trackHeight = 6;
    int labelWidthRatio = 40;   // percentage of bounds.w for label
    int valueTextWidth = 40;    // pixels reserved for value text
    int textPadding = 4;        // padding for label and value text

    int value = 50;
    int minValue = 0;
    int maxValue = 100;
    int step = 5;

    std::function<void(int)> onChange;

    WSlider();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;

private:
    void clampValue();
};
