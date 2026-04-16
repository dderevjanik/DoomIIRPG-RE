#include "WLabel.h"
#include "Graphics.h"
#include "App.h"
#include "Text.h"

WLabel::WLabel() {
    focusable = false;
}

void WLabel::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible) return;

    Text* buf = app->localization->getLargeBuffer();
    buf->setLength(0);

    if (stringId >= 0) {
        app->localization->composeTextField(stringId, buf);
        buf->dehyphenate();
    } else {
        buf->append(text.c_str());
    }

    graphics->setColor(effectiveColor(color));
    graphics->drawString(buf, bounds.x, bounds.y, anchor);
    buf->dispose();
}
