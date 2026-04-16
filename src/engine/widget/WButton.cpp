#include "WButton.h"
#include "Graphics.h"
#include "App.h"
#include "Text.h"
#include "Image.h"

WButton::WButton() {
    focusable = true;
}

void WButton::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible) return;

    // Draw background
    if (focused && imgHighlight) {
        graphics->drawImage(imgHighlight, bounds.x, bounds.y, 0, 0, 0);
    } else if (imgNormal) {
        graphics->drawImage(imgNormal, bounds.x, bounds.y, 0, 0, 0);
    } else {
        // Fallback: draw a filled rect
        int bgColor = focused ? highlightColor : 0xFF333333;
        graphics->fillRect(bounds.x, bounds.y, bounds.w, bounds.h, bgColor);
        graphics->drawRect(bounds.x, bounds.y, bounds.w, bounds.h, 0xFF666666);
    }

    // Draw text centered in bounds
    Text* buf = app->localization->getLargeBuffer();
    buf->setLength(0);

    if (stringId >= 0) {
        app->localization->composeTextField(stringId, buf);
        buf->dehyphenate();
    } else {
        buf->append(text.c_str());
    }

    graphics->setColor(color);
    int textX = bounds.x + bounds.w / 2;
    int textY = bounds.y + bounds.h / 2;
    graphics->drawString(buf, textX, textY, Graphics::ANCHORS_CENTER);
    buf->dispose();
}

bool WButton::handleInput(const WidgetInput& input) {
    if (input.action == WidgetAction::Confirm) {
        if (onClick) onClick();
        return true;
    }
    if (input.action == WidgetAction::TouchUp) {
        if (containsPoint(input.touchX, input.touchY)) {
            if (onClick) onClick();
            return true;
        }
    }
    return false;
}
