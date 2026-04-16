#include "WImage.h"
#include "Graphics.h"
#include "Image.h"

WImage::WImage() {
    focusable = false;
}

void WImage::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible || !image) return;
    graphics->drawImage(image, bounds.x, bounds.y, anchor, 0, renderMode);
}
