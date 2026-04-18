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
#include "WAutomap.h"
#include "DataNode.h"
#include "Graphics.h"
#include "Image.h"
#include "App.h"
#include "MenuSystem.h"
#include "Log.h"

// Static member definitions
DataNode WidgetLoader::s_screenDefaults;
std::unordered_map<std::string, DataNode> WidgetLoader::s_widgetDefs;
bool WidgetLoader::s_loaded = false;

// ---------------------------------------------------------------------------
// Widget definitions loading (widgets.yaml)
// ---------------------------------------------------------------------------

void WidgetLoader::loadWidgetDefs(const char* path) {
    DataNode root = DataNode::loadFile(path);
    if (!root) {
        LOG_WARN("[widget] no widget definitions at {}, using inline defaults\n", path);
        s_screenDefaults = DataNode();
        s_widgetDefs.clear();
        s_loaded = true;
        return;
    }

    s_screenDefaults = root["screen_defaults"];

    // First pass: store raw widget definitions
    DataNode widgetsNode = root["widgets"];
    std::unordered_map<std::string, DataNode> rawDefs;
    for (auto it = widgetsNode.begin(); it != widgetsNode.end(); ++it) {
        std::string name = it.key().asString();
        rawDefs[name] = it.value();
    }

    // Second pass: resolve single-level inheritance
    s_widgetDefs.clear();
    for (auto& [name, def] : rawDefs) {
        std::string parentName = def["widget"].asString("");
        if (!parentName.empty()) {
            auto pit = rawDefs.find(parentName);
            if (pit != rawDefs.end()) {
                // Merge: parent as base, this def as overlay, skip "widget" key
                s_widgetDefs[name] = DataNode::merge(pit->second, def, "widget");
            } else {
                LOG_WARN("[widget] definition '{}' references unknown parent '{}'\n", name, parentName);
                s_widgetDefs[name] = def;
            }
        } else {
            s_widgetDefs[name] = def;
        }
    }

    LOG_INFO("[widget] loaded {} widget definitions from {}\n", s_widgetDefs.size(), path);
    s_loaded = true;
}

DataNode WidgetLoader::resolveWidgetDef(const std::string& name) {
    auto it = s_widgetDefs.find(name);
    if (it != s_widgetDefs.end()) return it->second;
    return DataNode();
}

DataNode WidgetLoader::resolveWidgetNode(const DataNode& node) {
    std::string defName = node["widget"].asString("");
    if (defName.empty()) return node;

    DataNode def = resolveWidgetDef(defName);
    if (!def) {
        LOG_WARN("[widget] unknown widget definition: {}\n", defName);
        return node;
    }
    return DataNode::merge(def, node, "widget");
}

// ---------------------------------------------------------------------------
// Screen loading
// ---------------------------------------------------------------------------

bool WidgetLoader::loadScreen(WidgetScreen* screen, Applet* app, const char* yamlPath) {
    // Lazy load widget definitions on first call
    if (!s_loaded) {
        loadWidgetDefs("widgets.yaml");
    }

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

    // Apply screen defaults from widgets.yaml (screen's own values override)
    if (s_screenDefaults) {
        screenNode = DataNode::merge(s_screenDefaults, screenNode);
    }

    screen->screenId = screenNode["id"].asString();
    screen->backgroundColor = screenNode["background_color"].asInt(0xFF000000);
    screen->backgroundImageName = screenNode["background_image"].asString("");
    screen->backAction = screenNode["back_action"].asString("back");

    DataNode widgetsNode = screenNode["widgets"];
    for (int i = 0; i < widgetsNode.size(); i++) {
        DataNode widgetNode = resolveWidgetNode(widgetsNode[i]);
        auto widget = parseWidget(widgetNode, app);
        if (widget) {
            widget->disabled = widgetNode["disabled"].asBool(false);
            screen->widgets.push_back(std::move(widget));
        }
    }

    screen->buildFocusOrder();
    return true;
}

// ---------------------------------------------------------------------------
// Widget parsing
// ---------------------------------------------------------------------------

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
        w->bgColor = node["bg_color"].asInt(0xFF333333);
        w->borderColor = node["border_color"].asInt(0xFF666666);
        w->actionName = node["action"].asString();
        w->imgNormalName = node["image"].asString("");
        w->imgHighlightName = node["image_highlight"].asString("");
        w->renderMode = node["render_mode"].asInt(0);
        w->highlightRenderMode = node["highlight_render_mode"].asInt(0);
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
        w->checkBoxColor = node["check_box_color"].asInt(0xFF999999);
        w->checkFillColor = node["check_fill_color"].asInt(0xFF00FF00);
        w->checkSize = node["check_size"].asInt(12);
        w->checkPadding = node["check_padding"].asInt(6);
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
        w->trackHeight = node["track_height"].asInt(6);
        w->labelWidthRatio = node["label_width"].asInt(40);
        w->valueTextWidth = node["value_text_width"].asInt(40);
        w->textPadding = node["text_padding"].asInt(4);
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
        w->textPadding = node["text_padding"].asInt(4);
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
        w->itemTextPadding = node["item_text_padding"].asInt(8);
        w->highlightColor = node["highlight_color"].asInt(0xFF3366FF);
        w->color = node["color"].asInt(0xFFFFFFFF);
        w->scrollbarColor = node["scrollbar_color"].asInt(0xFF666666);
        w->scrollbarWidth = node["scrollbar_width"].asInt(3);
        w->scrollbarOffset = node["scrollbar_offset"].asInt(4);
        w->scrollbarMinHeight = node["scrollbar_min_height"].asInt(8);
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
        w->scrollbarColor = node["scrollbar_color"].asInt(0xFF666666);
        w->scrollbarWidth = node["scrollbar_width"].asInt(3);
        w->scrollbarOffset = node["scrollbar_offset"].asInt(4);
        w->scrollbarMinHeight = node["scrollbar_min_height"].asInt(8);
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(200), node["h"].asInt(200));

        DataNode childrenNode = node["children"];
        for (int i = 0; i < childrenNode.size(); i++) {
            DataNode childNode = resolveWidgetNode(childrenNode[i]);
            auto child = parseWidget(childNode, app);
            if (child) {
                w->addChild(std::move(child));
            }
        }
        return w;
    }

    if (type == "automap") {
        auto w = std::make_unique<WAutomap>();
        w->id = node["id"].asString();
        w->cellSize = node["cell_size"].asInt(0);
        w->showCursor = node["show_cursor"].asBool(true);
        w->showQuestMarkers = node["show_quest_markers"].asBool(true);
        w->showEntities = node["show_entities"].asBool(true);
        w->showWalls = node["show_walls"].asBool(true);
        w->setBounds(node["x"].asInt(), node["y"].asInt(),
                     node["w"].asInt(256), node["h"].asInt(256));
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
