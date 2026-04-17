#include "WVerticalMenu.h"
#include "Graphics.h"
#include "App.h"
#include "Text.h"

WVerticalMenu::WVerticalMenu() {
    focusable = true;
}

void WVerticalMenu::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible || items.empty()) return;

    graphics->setClipRect(bounds.x, bounds.y, bounds.w, bounds.h);

    Text* buf = app->localization->getLargeBuffer();

    int endIdx = scrollOffset + visibleCount;
    if (endIdx > static_cast<int>(items.size())) endIdx = static_cast<int>(items.size());

    for (int i = scrollOffset; i < endIdx; i++) {
        int itemY = bounds.y + (i - scrollOffset) * itemHeight;

        // Draw highlight behind selected item
        if (focused && i == selectedIndex) {
            graphics->fillRect(bounds.x, itemY, bounds.w, itemHeight, highlightColor);
        }

        // Draw item text
        buf->setLength(0);
        if (items[i].stringId >= 0) {
            app->localization->composeTextField(items[i].stringId, buf);
            buf->dehyphenate();
        } else {
            buf->append(items[i].text.c_str());
        }

        graphics->setColor(effectiveColor(color));
        int textY = itemY + itemHeight / 2;
        graphics->drawString(buf, bounds.x + itemTextPadding, textY, Graphics::ANCHORS_LEFT | Graphics::ANCHORS_VCENTER);
    }

    // Draw scrollbar indicator if content overflows
    if (static_cast<int>(items.size()) > visibleCount) {
        int barX = bounds.x + bounds.w - scrollbarOffset;
        int barH = bounds.h * visibleCount / static_cast<int>(items.size());
        if (barH < scrollbarMinHeight) barH = scrollbarMinHeight;
        int barY = bounds.y + (bounds.h - barH) * scrollOffset / (static_cast<int>(items.size()) - visibleCount);
        graphics->fillRect(barX, barY, scrollbarWidth, barH, effectiveColor(scrollbarColor));
    }

    buf->dispose();
    graphics->clearClipRect();
}

bool WVerticalMenu::handleInput(const WidgetInput& input) {
    if (disabled || items.empty()) return false;

    if (input.action == WidgetAction::Up) {
        if (selectedIndex > 0) {
            selectedIndex--;
            ensureVisible();
            return true;
        }
        return false;
    }
    if (input.action == WidgetAction::Down) {
        if (selectedIndex < static_cast<int>(items.size()) - 1) {
            selectedIndex++;
            ensureVisible();
            return true;
        }
        return false;
    }
    if (input.action == WidgetAction::Confirm) {
        if (onSelect) onSelect(selectedIndex, items[selectedIndex].actionName);
        return true;
    }
    if (input.action == WidgetAction::TouchUp) {
        if (containsPoint(input.touchX, input.touchY)) {
            int itemIdx = scrollOffset + (input.touchY - bounds.y) / itemHeight;
            if (itemIdx >= 0 && itemIdx < static_cast<int>(items.size())) {
                selectedIndex = itemIdx;
                if (onSelect) onSelect(selectedIndex, items[selectedIndex].actionName);
            }
            return true;
        }
    }
    return false;
}

void WVerticalMenu::ensureVisible() {
    if (selectedIndex < scrollOffset) {
        scrollOffset = selectedIndex;
    } else if (selectedIndex >= scrollOffset + visibleCount) {
        scrollOffset = selectedIndex - visibleCount + 1;
    }
}
