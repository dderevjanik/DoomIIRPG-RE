#include "WSlider.h"
#include "Graphics.h"
#include "App.h"
#include "Text.h"

WSlider::WSlider() {
    focusable = true;
}

void WSlider::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible) return;

    // Layout: [label text] [===track====] [value]
    int labelW = bounds.w * labelWidthRatio / 100;
    int trackX = bounds.x + labelW;
    int trackW = bounds.w - labelW - valueTextWidth;
    int trackY = bounds.y + (bounds.h - trackHeight) / 2;

    // Focus highlight
    if (focused) {
        graphics->fillRect(bounds.x, bounds.y, bounds.w, bounds.h, highlightColor & 0x40FFFFFF);
    }

    // Draw label
    Text* buf = app->localization->getLargeBuffer();
    buf->setLength(0);
    if (stringId >= 0) {
        app->localization->composeTextField(stringId, buf);
        buf->dehyphenate();
    } else {
        buf->append(text.c_str());
    }
    graphics->setColor(effectiveColor(color));
    graphics->drawString(buf, bounds.x + textPadding, bounds.y + bounds.h / 2,
                         Graphics::ANCHORS_LEFT | Graphics::ANCHORS_VCENTER);

    // Draw value text
    buf->setLength(0);
    if (focused) buf->append("< ");
    buf->append(value);
    if (focused) buf->append(" >");
    graphics->drawString(buf, bounds.x + bounds.w - textPadding, bounds.y + bounds.h / 2,
                         Graphics::ANCHORS_RIGHT | Graphics::ANCHORS_VCENTER);
    buf->dispose();

    // Draw track background
    graphics->fillRect(trackX, trackY, trackW, trackHeight, effectiveColor(trackColor));

    // Draw fill
    int range = maxValue - minValue;
    if (range > 0) {
        int fillW = trackW * (value - minValue) / range;
        graphics->fillRect(trackX, trackY, fillW, trackHeight, effectiveColor(fillColor));
    }
}

bool WSlider::handleInput(const WidgetInput& input) {
    if (disabled) return false;
    if (input.action == WidgetAction::Left) {
        value -= step;
        clampValue();
        if (onChange) onChange(value);
        return true;
    }
    if (input.action == WidgetAction::Right) {
        value += step;
        clampValue();
        if (onChange) onChange(value);
        return true;
    }
    if (input.action == WidgetAction::TouchUp) {
        if (containsPoint(input.touchX, input.touchY)) {
            int labelW = bounds.w * labelWidthRatio / 100;
            int trackX = bounds.x + labelW;
            int trackW = bounds.w - labelW - valueTextWidth;
            if (trackW > 0 && input.touchX >= trackX) {
                int range = maxValue - minValue;
                value = minValue + range * (input.touchX - trackX) / trackW;
                clampValue();
                if (onChange) onChange(value);
            }
            return true;
        }
    }
    return false;
}

void WSlider::clampValue() {
    if (value < minValue) value = minValue;
    if (value > maxValue) value = maxValue;
}
