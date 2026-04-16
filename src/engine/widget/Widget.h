#pragma once
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include "Button.h" // GuiRect

class Graphics;
class Applet;
class Image;

enum class WidgetAction {
    None,
    Up, Down, Left, Right,
    Confirm,    // ACTION_FIRE / gamepad A
    Back,       // ACTION_BACK / gamepad B
    TouchDown,
    TouchMove,
    TouchUp
};

struct WidgetInput {
    WidgetAction action = WidgetAction::None;
    int touchX = 0;
    int touchY = 0;
};

class Widget {
public:
    std::string id;
    GuiRect bounds{};
    bool visible = true;
    bool focusable = false;
    bool disabled = false;  // dimmed, skipped in focus order, ignores input
    Widget* parent = nullptr;

    virtual ~Widget() = default;

    // Draw this widget. focused is true when this widget has keyboard/gamepad focus.
    virtual void draw(Graphics* graphics, Applet* app, bool focused) = 0;

    // Handle input. Returns true if consumed.
    virtual bool handleInput(const WidgetInput& input) { return false; }

    // Per-frame update (animations, scroll momentum, etc.)
    virtual void update(Applet* app) {}

    bool containsPoint(int x, int y) const;
    void setBounds(int x, int y, int w, int h);

    // Returns a dimmed version of a color when disabled
    int effectiveColor(int c) const;

    static constexpr int DISABLED_ALPHA = 0x60;
};
