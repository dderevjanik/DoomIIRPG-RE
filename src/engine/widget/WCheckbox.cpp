#include "WCheckbox.h"
#include "Graphics.h"
#include "App.h"
#include "Text.h"

WCheckbox::WCheckbox() {
    focusable = true;
}

void WCheckbox::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible) return;

    int checkY = bounds.y + (bounds.h - CHECK_SIZE) / 2;

    // Draw focus highlight behind entire widget
    if (focused) {
        graphics->fillRect(bounds.x, bounds.y, bounds.w, bounds.h, highlightColor & 0x40FFFFFF);
    }

    // Draw checkbox box
    graphics->drawRect(bounds.x, checkY, CHECK_SIZE, CHECK_SIZE, 0xFF999999);
    if (checked) {
        graphics->fillRect(bounds.x + 2, checkY + 2, CHECK_SIZE - 4, CHECK_SIZE - 4, 0xFF00FF00);
    }

    // Draw label text
    Text* buf = app->localization->getLargeBuffer();
    buf->setLength(0);

    if (stringId >= 0) {
        app->localization->composeTextField(stringId, buf);
        buf->dehyphenate();
    } else {
        buf->append(text.c_str());
    }

    graphics->setColor(effectiveColor(color));
    int textX = bounds.x + CHECK_SIZE + CHECK_PADDING;
    int textY = bounds.y + bounds.h / 2;
    graphics->drawString(buf, textX, textY, Graphics::ANCHORS_LEFT | Graphics::ANCHORS_VCENTER);
    buf->dispose();
}

bool WCheckbox::handleInput(const WidgetInput& input) {
    if (disabled) return false;
    if (input.action == WidgetAction::Confirm) {
        toggle();
        return true;
    }
    if (input.action == WidgetAction::TouchUp) {
        if (containsPoint(input.touchX, input.touchY)) {
            toggle();
            return true;
        }
    }
    return false;
}

void WCheckbox::toggle() {
    checked = !checked;
    if (onToggle) onToggle(checked);
}
