#pragma once
#include "Widget.h"
#include <string>
#include <vector>
#include <functional>

class WSelector : public Widget {
public:
    std::string text;
    int stringId = -1;
    int color = 0xFFFFFFFF;
    int highlightColor = 0xFF3366FF;
    int textPadding = 4;

    struct Option {
        std::string label;
        int value = 0;
    };

    std::vector<Option> options;
    int selectedOption = 0;

    std::function<void(int index, int value)> onChange;

    WSelector();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;
};
