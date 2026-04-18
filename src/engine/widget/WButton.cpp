#include "WButton.h"
#include "Graphics.h"
#include "App.h"
#include "MenuSystem.h"
#include "Canvas.h"
#include "Text.h"
#include "Image.h"
#include <cmath>

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
    } else if (drawBackground) {
        // Fallback: filled rect with border
        int bg = focused ? highlightColor : bgColor;
        graphics->fillRect(bounds.x, bounds.y, bounds.w, bounds.h, bg);
        graphics->drawRect(bounds.x, bounds.y, bounds.w, bounds.h, borderColor);
    }

    // Draw text centered in bounds
    Text* buf = app->localization->getLargeBuffer();
    buf->setLength(0);

    // Prepend focus prefix (e.g. ">" or pointer char) when focused
    if (focused && !focusPrefix.empty()) {
        buf->append(focusPrefix.c_str());
    }

    if (stringId >= 0) {
        app->localization->composeTextField(stringId, buf);
        buf->dehyphenate();
    } else {
        buf->append(text.c_str());
    }

    // Use highlightColor for text when focused
    int textColor = focused ? highlightColor : color;

    // Pulse between highlightColor and color when focused (brightness-based)
    if (focused && animateFocus && animatePeriodMs > 0) {
        float t = (float)(app->upTimeMs % animatePeriodMs) / (float)animatePeriodMs;
        float pulse = 0.5f + 0.5f * std::sin(t * 6.2831853f);  // 0..1
        // Lerp each RGB channel between `color` (t=0) and `highlightColor` (t=1)
        int r0 = (color >> 16) & 0xFF, g0 = (color >> 8) & 0xFF, b0 = color & 0xFF;
        int r1 = (highlightColor >> 16) & 0xFF, g1 = (highlightColor >> 8) & 0xFF, b1 = highlightColor & 0xFF;
        int r = r0 + static_cast<int>((r1 - r0) * pulse);
        int g = g0 + static_cast<int>((g1 - g0) * pulse);
        int b = b0 + static_cast<int>((b1 - b0) * pulse);
        textColor = (highlightColor & 0xFF000000) | (r << 16) | (g << 8) | b;
    }

    graphics->setColor(effectiveColor(textColor));

    // Text alignment: 0=center, 1=left, 2=right
    int textX, anchorFlags;
    int textY = bounds.y + bounds.h / 2;
    int shift = focused ? focusOffsetX : 0;
    if (textAlign == 1) {  // left
        textX = bounds.x + shift;
        anchorFlags = Graphics::ANCHORS_LEFT | Graphics::ANCHORS_VCENTER;
    } else if (textAlign == 2) {  // right
        textX = bounds.x + bounds.w + shift;
        anchorFlags = Graphics::ANCHORS_RIGHT | Graphics::ANCHORS_VCENTER;
    } else {  // center
        textX = bounds.x + bounds.w / 2 + shift;
        anchorFlags = Graphics::ANCHORS_CENTER;
    }
    graphics->drawString(buf, textX, textY, anchorFlags);
    buf->dispose();

    // Draw animated cursor glyph to the left of the text when focused.
    // Uses Canvas::OSC_CYCLE for the classic [-1, 0, +1, 0] horizontal wiggle.
    if (focused && focusCursor) {
        int oscillation = 0;
        auto* osc = app->canvas->OSC_CYCLE;
        if (osc) {
            oscillation = osc[(app->upTimeMs / 100) % 4];
        }
        int cursorX = bounds.x + shift - 14 + oscillation;
        int cursorY = bounds.y + bounds.h / 2;
        graphics->drawCursor(cursorX, cursorY, Graphics::ANCHORS_LEFT | Graphics::ANCHORS_VCENTER);
    }
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
