#include "WidgetScreen.h"
#include "WidgetLoader.h"
#include "WButton.h"
#include "WSlider.h"
#include "WVerticalMenu.h"
#include "Canvas.h"
#include "App.h"
#include "MenuSystem.h"
#include "Graphics.h"
#include "Image.h"
#include "Input.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Log.h"

void WidgetScreen::onAction(const std::string& actionName, std::function<void()> handler) {
    actionHandlers[actionName] = std::move(handler);
}

bool WidgetScreen::loadFromYAML(Applet* app, const char* yamlPath) {
    this->app = app;
    widgets.clear();
    focusOrder.clear();
    focusIndex = 0;
    backgroundImageName.clear();
    if (!WidgetLoader::loadScreen(this, app, yamlPath)) return false;
    wireActions();
    if (onScreenLoaded) onScreenLoaded(this);
    return true;
}

void WidgetScreen::wireActions() {
    for (auto& w : widgets) {
        if (auto* btn = dynamic_cast<WButton*>(w.get())) {
            if (!btn->actionName.empty()) {
                std::string action = btn->actionName;
                btn->onClick = [this, action]() { dispatchAction(action); };
            }
        } else if (auto* menu = dynamic_cast<WVerticalMenu*>(w.get())) {
            menu->onSelect = [this](int, const std::string& action) {
                if (!action.empty()) dispatchAction(action);
            };
        }
    }
}

void WidgetScreen::onEnter(Canvas* canvas) {
    this->canvas = canvas;
    this->app = canvas->app;
    if (!focusOrder.empty()) {
        focusIndex = 0;
    }
}

void WidgetScreen::onExit(Canvas* canvas) {
    // nothing to clean up
}

void WidgetScreen::update(Canvas* canvas) {
    for (auto& w : widgets) {
        w->update(app);
    }

    // Process pending touch events
    if (hasPendingTouch) {
        Widget* focused = (!focusOrder.empty() && focusIndex >= 0 &&
                          focusIndex < static_cast<int>(focusOrder.size()))
                          ? focusOrder[focusIndex] : nullptr;

        if (pendingTouch.action == WidgetAction::TouchDown) {
            Widget* hit = hitTest(pendingTouch.touchX, pendingTouch.touchY);
            if (hit && hit->focusable) {
                for (int i = 0; i < static_cast<int>(focusOrder.size()); i++) {
                    if (focusOrder[i] == hit) {
                        focusIndex = i;
                        break;
                    }
                }
            }
        }

        if (focused) {
            focused->handleInput(pendingTouch);
        }

        hasPendingTouch = false;
    }
}

void WidgetScreen::render(Canvas* canvas, Graphics* graphics) {
    // Draw background
    graphics->fillRect(0, 0, 480, 320, backgroundColor);
    if (!backgroundImageName.empty()) {
        Image* bgImg = app->menuSystem->resolveMenuImage(backgroundImageName);
        if (bgImg) {
            graphics->drawImage(bgImg, 240, 160, Graphics::ANCHORS_CENTER, 0, 0);
        }
    }

    Widget* focused = (!focusOrder.empty() && focusIndex >= 0 &&
                      focusIndex < static_cast<int>(focusOrder.size()))
                      ? focusOrder[focusIndex] : nullptr;

    for (auto& w : widgets) {
        if (w->visible) {
            w->draw(graphics, app, w.get() == focused);
        }
    }
}

bool WidgetScreen::handleInput(Canvas* canvas, int key, int action) {
    WidgetInput input = translateInput(key, action);
    if (input.action == WidgetAction::None) return false;

    // Back action: use per-screen back action (may save config, etc.)
    if (input.action == WidgetAction::Back) {
        app->sound->playSound(Sounds::getResIDByName(SoundName::SWITCH_EXIT), 0, 5, false);
        dispatchAction(backAction);
        return true;
    }

    // Pass input to focused widget first
    Widget* focused = (!focusOrder.empty() && focusIndex >= 0 &&
                      focusIndex < static_cast<int>(focusOrder.size()))
                      ? focusOrder[focusIndex] : nullptr;

    if (focused && focused->handleInput(input)) {
        // Play confirm/adjust sound for consumed inputs
        if (input.action == WidgetAction::Confirm) {
            app->sound->playSound(Sounds::getResIDByName(SoundName::SWITCH_EXIT), 0, 5, false);
        } else if (input.action == WidgetAction::Left || input.action == WidgetAction::Right) {
            app->sound->playSound(Sounds::getResIDByName(SoundName::MENU_SCROLL), 0, 5, false);
        }
        return true;
    }

    // Widget didn't consume it - handle focus navigation
    if (input.action == WidgetAction::Up && !focusOrder.empty()) {
        if (focusIndex > 0) {
            focusIndex--;
            app->sound->playSound(Sounds::getResIDByName(SoundName::MENU_SCROLL), 0, 5, false);
            return true;
        }
    }
    if (input.action == WidgetAction::Down && !focusOrder.empty()) {
        if (focusIndex < static_cast<int>(focusOrder.size()) - 1) {
            focusIndex++;
            app->sound->playSound(Sounds::getResIDByName(SoundName::MENU_SCROLL), 0, 5, false);
            return true;
        }
    }

    return false;
}

void WidgetScreen::touchStart(int x, int y) {
    pendingTouch.action = WidgetAction::TouchDown;
    pendingTouch.touchX = x;
    pendingTouch.touchY = y;
    hasPendingTouch = true;
}

void WidgetScreen::touchMove(int x, int y) {
    pendingTouch.action = WidgetAction::TouchMove;
    pendingTouch.touchX = x;
    pendingTouch.touchY = y;
    hasPendingTouch = true;
}

void WidgetScreen::touchEnd(int x, int y) {
    pendingTouch.action = WidgetAction::TouchUp;
    pendingTouch.touchX = x;
    pendingTouch.touchY = y;
    hasPendingTouch = true;
}

void WidgetScreen::buildFocusOrder() {
    focusOrder.clear();
    for (auto& w : widgets) {
        if (w->focusable && w->visible) {
            focusOrder.push_back(w.get());
        }
    }
    focusIndex = 0;
}

void WidgetScreen::dispatchAction(const std::string& actionName) {
    if (actionName.starts_with("goto:")) {
        std::string screenName = actionName.substr(5);
        std::string path = "screens/" + screenName + ".yaml";

        // Push current state
        ScreenStackEntry entry;
        entry.yamlPath = screenId;
        entry.focusIndex = focusIndex;
        screenStack.push_back(entry);

        // Load new screen
        widgets.clear();
        focusOrder.clear();
        focusIndex = 0;
        WidgetLoader::loadScreen(this, app, path.c_str());
        wireActions();
        if (onScreenLoaded) onScreenLoaded(this);
    } else if (actionName == "back" || actionName == "close") {
        if (!screenStack.empty()) {
            // Pop screen stack
            ScreenStackEntry entry = screenStack.back();
            screenStack.pop_back();

            std::string path = "screens/" + entry.yamlPath + ".yaml";
            widgets.clear();
            focusOrder.clear();
            WidgetLoader::loadScreen(this, app, path.c_str());
            wireActions();
            if (onScreenLoaded) onScreenLoaded(this);
            focusIndex = entry.focusIndex;
            if (focusIndex >= static_cast<int>(focusOrder.size())) {
                focusIndex = 0;
            }
        } else {
            // Exit to previous canvas state
            canvas->setState(canvas->oldState);
        }
    } else {
        auto it = actionHandlers.find(actionName);
        if (it != actionHandlers.end()) {
            it->second();
        } else {
            LOG_WARN("[widget] unhandled action: {}\n", actionName);
        }
    }
}

Widget* WidgetScreen::findWidget(const std::string& widgetId) {
    for (auto& w : widgets) {
        if (w->id == widgetId) return w.get();
    }
    return nullptr;
}

WidgetInput WidgetScreen::translateInput(int key, int action) {
    WidgetInput input;
    // action is the translated keyAction from Canvas::getKeyAction()
    // (Enums::ACTION_UP=1, ACTION_DOWN=2, ACTION_LEFT=3, ACTION_RIGHT=4,
    //  ACTION_FIRE=6, ACTION_BACK=15, ACTION_MENU=14)
    switch (action) {
        case 1:  input.action = WidgetAction::Up;      break; // ACTION_UP
        case 2:  input.action = WidgetAction::Down;    break; // ACTION_DOWN
        case 3:  input.action = WidgetAction::Left;    break; // ACTION_LEFT
        case 4:  input.action = WidgetAction::Right;   break; // ACTION_RIGHT
        case 6:  input.action = WidgetAction::Confirm; break; // ACTION_FIRE
        case 14: input.action = WidgetAction::Back;    break; // ACTION_MENU
        case 15: input.action = WidgetAction::Back;    break; // ACTION_BACK
    }
    return input;
}

Widget* WidgetScreen::hitTest(int x, int y) {
    // Reverse order so topmost widgets are hit first
    for (int i = static_cast<int>(widgets.size()) - 1; i >= 0; i--) {
        if (widgets[i]->visible && widgets[i]->containsPoint(x, y)) {
            return widgets[i].get();
        }
    }
    return nullptr;
}
