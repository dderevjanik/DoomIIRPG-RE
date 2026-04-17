#pragma once
#include "Widget.h"
#include <vector>
#include <memory>

class WScrollPanel : public Widget {
public:
    std::vector<std::unique_ptr<Widget>> children;
    int scrollY = 0;
    int contentHeight = 0;
    int scrollSpeed = 20;
    int scrollbarColor = 0xFF666666;
    int scrollbarWidth = 3;
    int scrollbarOffset = 4;    // from right edge
    int scrollbarMinHeight = 8;

    WScrollPanel();
    void draw(Graphics* graphics, Applet* app, bool focused) override;
    bool handleInput(const WidgetInput& input) override;
    void update(Applet* app) override;

    void addChild(std::unique_ptr<Widget> child);
    void recalcContentHeight();

private:
    void clampScroll();
    int touchStartY = 0;
    int scrollAtTouchStart = 0;
    bool dragging = false;
};
