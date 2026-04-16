#pragma once
#include "Widget.h"
#include <string>
#include <vector>
#include <functional>

class WVerticalMenu : public Widget {
public:
    struct Item {
        std::string id;
        std::string text;
        int stringId = -1;
        std::string actionName;
    };

    std::vector<Item> items;
    int selectedIndex = 0;
    int scrollOffset = 0;       // index of first visible item
    int visibleCount = 5;
    int itemHeight = 32;
    int highlightColor = 0xFF3366FF;
    int color = 0xFFFFFFFF;

    std::function<void(int index, const std::string& actionName)> onSelect;

    WVerticalMenu();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;

private:
    void ensureVisible();
};
