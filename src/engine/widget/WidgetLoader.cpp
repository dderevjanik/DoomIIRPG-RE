#include "WidgetLoader.h"
#include "WidgetScreen.h"
#include "Widget.h"
#include "WLabel.h"
#include "WButton.h"
#include "WCheckbox.h"
#include "WVerticalMenu.h"
#include "WScrollPanel.h"
#include "WImage.h"
#include "WSlider.h"
#include "WSelector.h"
#include "DataNode.h"
#include "Graphics.h"
#include "Image.h"
#include "App.h"
#include "MenuSystem.h"
#include "Log.h"

bool WidgetLoader::loadScreen(WidgetScreen* screen, Applet* app, const char* yamlPath) {
    DataNode root = DataNode::loadFile(yamlPath);
    if (!root) {
        LOG_ERROR("[widget] failed to load {}\n", yamlPath);
        return false;
    }

    DataNode screenNode = root["screen"];
    if (!screenNode) {
        LOG_ERROR("[widget] no 'screen' node in {}\n", yamlPath);
        return false;
    }

    screen->screenId = screenNode["id"].asString();
    screen->backgroundColor = screenNode["background_color"].asInt(0xFF000000);

    screen->backgroundImageName = screenNode["background_image"].asString("");
    screen->backAction = screenNode["back_action"].asString("back");

    DataNode widgetsNode = screenNode["widgets"];
    for (int i = 0; i < widgetsNode.size(); i++) {
        auto widget = parseWidget(widgetsNode[i], app);
        if (widget) {
            widget->disabled = widgetsNode[i]["disabled"].asBool(false);
            screen->widgets.push_back(std::move(widget));
        }
    }

    screen->buildFocusOrder();
    return true;
}

std::unique_ptr<Widget> WidgetLoader::parseWidget(const DataNode& node, Applet* app) {
    std::string type = node["type"].asString();

    if (type == "label") {
        auto w = std::make_unique<WLabel>();
        w->id = node["id"].asString();
        w->text = node["text"].asString();
        w->stringId = node["string_id"].asInt(-1);
        w->color = node["color"].asInt(0xFFFFFFFF);
        w->anchor = parseAnchor(node["anchor"].asString("left"));
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(0), node["h"].asInt(0));
        return w;
    }

    if (type == "button") {
        auto w = std::make_unique<WButton>();
        w->id = node["id"].asString();
        w->text = node["text"].asString();
        w->stringId = node["string_id"].asInt(-1);
        w->color = node["color"].asInt(0xFFFFFFFF);
        w->highlightColor = node["highlight_color"].asInt(0xFF3366FF);
        w->actionName = node["action"].asString();
        w->imgNormal = resolveImage(app, node["image"].asString(""));
        w->imgHighlight = resolveImage(app, node["image_highlight"].asString(""));
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(120), node["h"].asInt(30));
        return w;
    }

    if (type == "image") {
        auto w = std::make_unique<WImage>();
        w->id = node["id"].asString();
        w->imageName = node["image"].asString("");
        w->anchor = parseAnchor(node["anchor"].asString("none"));
        w->renderMode = node["render_mode"].asInt(0);
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(0), node["h"].asInt(0));
        return w;
    }

    if (type == "checkbox") {
        auto w = std::make_unique<WCheckbox>();
        w->id = node["id"].asString();
        w->text = node["text"].asString();
        w->stringId = node["string_id"].asInt(-1);
        w->color = node["color"].asInt(0xFFFFFFFF);
        w->highlightColor = node["highlight_color"].asInt(0xFF3366FF);
        w->checked = node["checked"].asBool(false);
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(200), node["h"].asInt(24));
        return w;
    }

    if (type == "slider") {
        auto w = std::make_unique<WSlider>();
        w->id = node["id"].asString();
        w->text = node["text"].asString();
        w->stringId = node["string_id"].asInt(-1);
        w->color = node["color"].asInt(0xFFFFFFFF);
        w->highlightColor = node["highlight_color"].asInt(0xFF3366FF);
        w->trackColor = node["track_color"].asInt(0xFF444444);
        w->fillColor = node["fill_color"].asInt(0xFF00AA00);
        w->value = node["value"].asInt(50);
        w->minValue = node["min"].asInt(0);
        w->maxValue = node["max"].asInt(100);
        w->step = node["step"].asInt(5);
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(300), node["h"].asInt(24));
        return w;
    }

    if (type == "selector") {
        auto w = std::make_unique<WSelector>();
        w->id = node["id"].asString();
        w->text = node["text"].asString();
        w->stringId = node["string_id"].asInt(-1);
        w->color = node["color"].asInt(0xFFFFFFFF);
        w->highlightColor = node["highlight_color"].asInt(0xFF3366FF);
        w->selectedOption = node["selected"].asInt(0);
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(300), node["h"].asInt(24));

        DataNode optionsNode = node["options"];
        for (int i = 0; i < optionsNode.size(); i++) {
            WSelector::Option opt;
            opt.label = optionsNode[i]["label"].asString();
            opt.value = optionsNode[i]["value"].asInt(i);
            w->options.push_back(std::move(opt));
        }
        return w;
    }

    if (type == "vertical_menu") {
        auto w = std::make_unique<WVerticalMenu>();
        w->id = node["id"].asString();
        w->itemHeight = node["item_height"].asInt(32);
        w->visibleCount = node["visible_count"].asInt(5);
        w->highlightColor = node["highlight_color"].asInt(0xFF3366FF);
        w->color = node["color"].asInt(0xFFFFFFFF);
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(320), node["h"].asInt(160));

        DataNode itemsNode = node["items"];
        for (int i = 0; i < itemsNode.size(); i++) {
            WVerticalMenu::Item item;
            item.id = itemsNode[i]["id"].asString();
            item.text = itemsNode[i]["text"].asString();
            item.stringId = itemsNode[i]["string_id"].asInt(-1);
            item.actionName = itemsNode[i]["action"].asString();
            w->items.push_back(std::move(item));
        }
        return w;
    }

    if (type == "scroll_panel") {
        auto w = std::make_unique<WScrollPanel>();
        w->id = node["id"].asString();
        w->scrollSpeed = node["scroll_speed"].asInt(20);
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(200), node["h"].asInt(200));

        DataNode childrenNode = node["children"];
        for (int i = 0; i < childrenNode.size(); i++) {
            auto child = parseWidget(childrenNode[i], app);
            if (child) {
                w->addChild(std::move(child));
            }
        }
        return w;
    }

    LOG_ERROR("[widget] unknown widget type: {}\n", type);
    return nullptr;
}

Image* WidgetLoader::resolveImage(Applet* app, const std::string& name) {
    if (name.empty()) return nullptr;
    return app->menuSystem->resolveMenuImage(name);
}

int WidgetLoader::parseAnchor(const std::string& str) {
    if (str == "center")  return Graphics::ANCHORS_CENTER;
    if (str == "hcenter") return Graphics::ANCHORS_HCENTER;
    if (str == "vcenter") return Graphics::ANCHORS_VCENTER;
    if (str == "left")    return Graphics::ANCHORS_LEFT;
    if (str == "right")   return Graphics::ANCHORS_RIGHT;
    if (str == "top")     return Graphics::ANCHORS_TOP;
    if (str == "bottom")  return Graphics::ANCHORS_BOTTOM;
    return Graphics::ANCHORS_NONE;
}
