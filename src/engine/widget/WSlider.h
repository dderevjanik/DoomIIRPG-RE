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

    int value = 50;     // current value
    int minValue = 0;
    int maxValue = 100;
    int step = 5;       // increment per left/right press

    std::function<void(int)> onChange;

    WSlider();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;

private:
    static constexpr int TRACK_HEIGHT = 6;
    static constexpr int LABEL_WIDTH_RATIO = 40; // percentage of bounds.w for label
    void clampValue();
};
