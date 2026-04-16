#pragma once
#include "ICanvasState.h"
#include "Widget.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

class WidgetScreen : public ICanvasState {
public:
    std::string screenId;
    int backgroundColor = 0xFF000000;
    std::string backgroundImageName;

    std::vector<std::unique_ptr<Widget>> widgets;
    std::vector<Widget*> focusOrder;
    int focusIndex = 0;

    // Action handlers: action string -> callback
    std::unordered_map<std::string, std::function<void()>> actionHandlers;

    // Called after a screen YAML is loaded (for wiring game-specific callbacks)
    std::function<void(WidgetScreen*)> onScreenLoaded;

    void onAction(const std::string& actionName, std::function<void()> handler);

    bool loadFromYAML(Applet* app, const char* yamlPath);

    // ICanvasState interface
    void onEnter(Canvas* canvas) override;
    void onExit(Canvas* canvas) override;
    void update(Canvas* canvas) override;
    void render(Canvas* canvas, Graphics* graphics) override;
    bool handleInput(Canvas* canvas, int key, int action) override;

    // Touch forwarding (called from Canvas touch methods)
    void touchStart(int x, int y);
    void touchMove(int x, int y);
    void touchEnd(int x, int y);

    void buildFocusOrder();
    void dispatchAction(const std::string& actionName);

    Widget* findWidget(const std::string& id);

private:
    void wireActions();
    Applet* app = nullptr;
    Canvas* canvas = nullptr;

    // Screen stack for goto: navigation
    struct ScreenStackEntry {
        std::string yamlPath;
        int focusIndex;
    };
    std::vector<ScreenStackEntry> screenStack;

    // Pending touch state
    WidgetInput pendingTouch;
    bool hasPendingTouch = false;

    WidgetInput translateInput(int key, int action);
    Widget* hitTest(int x, int y);
};
