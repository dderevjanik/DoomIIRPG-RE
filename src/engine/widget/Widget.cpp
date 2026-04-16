#include "Widget.h"

bool Widget::containsPoint(int x, int y) const {
    return x >= bounds.x && x < bounds.x + bounds.w &&
           y >= bounds.y && y < bounds.y + bounds.h;
}

void Widget::setBounds(int x, int y, int w, int h) {
    bounds.x = x;
    bounds.y = y;
    bounds.w = w;
    bounds.h = h;
}

int Widget::effectiveColor(int c) const {
    if (!disabled) return c;
    // Replace alpha channel with DISABLED_ALPHA
    return (c & 0x00FFFFFF) | (DISABLED_ALPHA << 24);
}
