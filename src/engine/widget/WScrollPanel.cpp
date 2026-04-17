#include "WScrollPanel.h"
#include "Graphics.h"

WScrollPanel::WScrollPanel() {
    focusable = true;
}

void WScrollPanel::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible) return;

    graphics->setClipRect(bounds.x, bounds.y, bounds.w, bounds.h);

    for (auto& child : children) {
        if (!child->visible) continue;
        int origY = child->bounds.y;
        child->bounds.y -= scrollY;

        if (child->bounds.y + child->bounds.h > bounds.y &&
            child->bounds.y < bounds.y + bounds.h) {
            child->draw(graphics, app, false);
        }

        child->bounds.y = origY;
    }

    // Draw scrollbar if content overflows
    if (contentHeight > bounds.h) {
        int barX = bounds.x + bounds.w - scrollbarOffset;
        int barH = bounds.h * bounds.h / contentHeight;
        if (barH < scrollbarMinHeight) barH = scrollbarMinHeight;
        int maxScroll = contentHeight - bounds.h;
        int barY = bounds.y + (bounds.h - barH) * scrollY / maxScroll;
        graphics->fillRect(barX, barY, scrollbarWidth, barH, effectiveColor(scrollbarColor));
    }

    graphics->clearClipRect();
}

bool WScrollPanel::handleInput(const WidgetInput& input) {
    if (input.action == WidgetAction::Up) {
        if (scrollY > 0) {
            scrollY -= scrollSpeed;
            clampScroll();
            return true;
        }
        return false;
    }
    if (input.action == WidgetAction::Down) {
        int maxScroll = contentHeight - bounds.h;
        if (maxScroll > 0 && scrollY < maxScroll) {
            scrollY += scrollSpeed;
            clampScroll();
            return true;
        }
        return false;
    }
    if (input.action == WidgetAction::TouchDown) {
        if (containsPoint(input.touchX, input.touchY)) {
            touchStartY = input.touchY;
            scrollAtTouchStart = scrollY;
            dragging = true;
            return true;
        }
    }
    if (input.action == WidgetAction::TouchMove && dragging) {
        scrollY = scrollAtTouchStart - (input.touchY - touchStartY);
        clampScroll();
        return true;
    }
    if (input.action == WidgetAction::TouchUp) {
        dragging = false;
        return containsPoint(input.touchX, input.touchY);
    }
    return false;
}

void WScrollPanel::update(Applet* app) {
    for (auto& child : children) {
        child->update(app);
    }
}

void WScrollPanel::addChild(std::unique_ptr<Widget> child) {
    child->parent = this;
    children.push_back(std::move(child));
    recalcContentHeight();
}

void WScrollPanel::recalcContentHeight() {
    contentHeight = 0;
    for (auto& child : children) {
        int bottom = child->bounds.y + child->bounds.h;
        if (bottom > contentHeight) contentHeight = bottom;
    }
}

void WScrollPanel::clampScroll() {
    int maxScroll = contentHeight - bounds.h;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollY < 0) scrollY = 0;
    if (scrollY > maxScroll) scrollY = maxScroll;
}
