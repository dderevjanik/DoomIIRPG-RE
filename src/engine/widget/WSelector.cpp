#include "WSelector.h"
#include "Graphics.h"
#include "App.h"
#include "Text.h"

WSelector::WSelector() {
    focusable = true;
}

void WSelector::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible || options.empty()) return;

    // Focus highlight
    if (focused) {
        graphics->fillRect(bounds.x, bounds.y, bounds.w, bounds.h, highlightColor & 0x40FFFFFF);
    }

    Text* buf = app->localization->getLargeBuffer();

    // Draw label on the left
    buf->setLength(0);
    if (stringId >= 0) {
        app->localization->composeTextField(stringId, buf);
        buf->dehyphenate();
    } else {
        buf->append(text.c_str());
    }
    graphics->setColor(effectiveColor(color));
    graphics->drawString(buf, bounds.x + 4, bounds.y + bounds.h / 2,
                         Graphics::ANCHORS_LEFT | Graphics::ANCHORS_VCENTER);

    // Draw selected value on the right: < value >
    buf->setLength(0);
    if (focused) buf->append("< ");
    buf->append(options[selectedOption].label.c_str());
    if (focused) buf->append(" >");
    graphics->drawString(buf, bounds.x + bounds.w - 4, bounds.y + bounds.h / 2,
                         Graphics::ANCHORS_RIGHT | Graphics::ANCHORS_VCENTER);

    buf->dispose();
}

bool WSelector::handleInput(const WidgetInput& input) {
    if (disabled || options.empty()) return false;

    if (input.action == WidgetAction::Left) {
        if (selectedOption > 0) {
            selectedOption--;
        } else {
            selectedOption = static_cast<int>(options.size()) - 1; // wrap
        }
        if (onChange) onChange(selectedOption, options[selectedOption].value);
        return true;
    }
    if (input.action == WidgetAction::Right || input.action == WidgetAction::Confirm) {
        if (input.action == WidgetAction::Right) {
            if (selectedOption < static_cast<int>(options.size()) - 1) {
                selectedOption++;
            } else {
                selectedOption = 0; // wrap
            }
        }
        if (onChange) onChange(selectedOption, options[selectedOption].value);
        return true;
    }
    if (input.action == WidgetAction::TouchUp) {
        if (containsPoint(input.touchX, input.touchY)) {
            // Cycle forward on touch
            selectedOption = (selectedOption + 1) % static_cast<int>(options.size());
            if (onChange) onChange(selectedOption, options[selectedOption].value);
            return true;
        }
    }
    return false;
}
