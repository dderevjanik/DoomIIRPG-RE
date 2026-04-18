#include "WButton.h"
#include "Graphics.h"
#include "App.h"
#include "MenuSystem.h"
#include "Text.h"
#include "Image.h"

WButton::WButton() {
    focusable = true;
}

void WButton::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible) return;

    // Resolve images by name at render time (avoids stale pointers)
    Image* imgN = imgNormalName.empty() ? nullptr : app->menuSystem->resolveMenuImage(imgNormalName);
    Image* imgH = imgHighlightName.empty() ? nullptr : app->menuSystem->resolveMenuImage(imgHighlightName);

    // Draw background
    if (focused && imgH) {
        graphics->drawImage(imgH, bounds.x, bounds.y, 0, 0, highlightRenderMode);
    } else if (imgN) {
        graphics->drawImage(imgN, bounds.x, bounds.y, 0, 0, renderMode);
    } else {
        // Fallback: draw a filled rect
        int bg = focused ? highlightColor : bgColor;
        graphics->fillRect(bounds.x, bounds.y, bounds.w, bounds.h, bg);
        graphics->drawRect(bounds.x, bounds.y, bounds.w, bounds.h, borderColor);
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

    // Use highlightColor for text when focused so text stays visible
    // on bright highlight backgrounds (e.g. menu_btn_bg_on)
    int textColor = focused ? highlightColor : color;
    graphics->setColor(effectiveColor(textColor));
    int textX = bounds.x + bounds.w / 2;
    int textY = bounds.y + bounds.h / 2;
    graphics->drawString(buf, textX, textY, Graphics::ANCHORS_CENTER);
    buf->dispose();
}

bool WButton::handleInput(const WidgetInput& input) {
    if (disabled) return false;
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
