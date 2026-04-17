#pragma once
#include <memory>
#include <string>
#include <unordered_map>

class Widget;
class Applet;
class Image;
class DataNode;
class WidgetScreen;

class WidgetLoader {
public:
    static bool loadScreen(WidgetScreen* screen, Applet* app, const char* yamlPath);
    static Image* resolveImage(Applet* app, const std::string& name);

    // Load widget definitions from widgets.yaml (called lazily on first loadScreen)
    static void loadWidgetDefs(const char* path);

private:
    static std::unique_ptr<Widget> parseWidget(const DataNode& node, Applet* app);
    static int parseAnchor(const std::string& str);

    // Resolve a widget node: if it has `widget: <name>`, merge the definition as base
    static DataNode resolveWidgetNode(const DataNode& node);
    static DataNode resolveWidgetDef(const std::string& name);

    // Cached widget definitions (resolved, with inheritance applied)
    static DataNode s_screenDefaults;
    static std::unordered_map<std::string, DataNode> s_widgetDefs;
    static bool s_loaded;
};
