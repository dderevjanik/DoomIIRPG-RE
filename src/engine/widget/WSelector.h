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

    struct Option {
        std::string label;
        int value = 0;      // arbitrary int value for the option
    };

    std::vector<Option> options;
    int selectedOption = 0;

    std::function<void(int index, int value)> onChange;

    WSelector();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;

private:
    static constexpr int LABEL_WIDTH_RATIO = 40; // percentage of bounds.w for label
};
