#pragma once
#include <memory>
#include <string>

class Widget;
class Applet;
class Image;
class DataNode;
class WidgetScreen;

class WidgetLoader {
public:
    static bool loadScreen(WidgetScreen* screen, Applet* app, const char* yamlPath);

    // Resolve an image name to an Image* using MenuSystem's image registry
    static Image* resolveImage(Applet* app, const std::string& name);

private:
    static std::unique_ptr<Widget> parseWidget(const DataNode& node, Applet* app);
    static int parseAnchor(const std::string& str);
};
