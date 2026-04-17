#pragma once
#include "Widget.h"
#include <string>
#include <functional>

class WCheckbox : public Widget {
public:
    std::string text;
    int stringId = -1;
    int color = 0xFFFFFFFF;
    int highlightColor = 0xFF3366FF;
    int checkBoxColor = 0xFF999999;
    int checkFillColor = 0xFF00FF00;
    int checkSize = 12;
    int checkPadding = 6;
    bool checked = false;

    std::function<void(bool)> onToggle;

    WCheckbox();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;

private:
    void toggle();
};
