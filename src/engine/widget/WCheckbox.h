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
    bool checked = false;

    std::function<void(bool)> onToggle;

    WCheckbox();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;

private:
    void toggle();
    static constexpr int CHECK_SIZE = 12;
    static constexpr int CHECK_PADDING = 6;
};
