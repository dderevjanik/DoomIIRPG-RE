#include "WImage.h"
#include "Graphics.h"
#include "Image.h"
#include "App.h"
#include "MenuSystem.h"

WImage::WImage() {
    focusable = false;
}

void WImage::draw(Graphics* graphics, Applet* app, bool focused) {
    if (!visible || imageName.empty()) return;
    Image* img = app->menuSystem->resolveMenuImage(imageName);
    if (img) {
        graphics->drawImage(img, bounds.x, bounds.y, anchor, 0, renderMode);
    }
}
