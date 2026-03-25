#pragma once

#include <string>
#include <vector>
#include <functional>

struct Camera;
struct MapData;

class EditorUI {
public:
    using LoadCallback = std::function<void(int)>;
    using SaveCallback = std::function<void()>;

    void init(void* unused);
    void draw(int currentMapID, const Camera& camera, bool noclip = false);

    void setLoadCallback(LoadCallback cb) { loadCallback_ = std::move(cb); }
    void setSaveCallback(SaveCallback cb) { saveCallback_ = std::move(cb); }
    void setMapData(MapData* md) { mapData_ = md; }

    bool showAutomap = true;
    bool showEntities = true;
    int selectedTileIdx() const { return selectedTileIdx_; }

private:
    void drawMenuBar();
    void drawMapInfo(int currentMapID);
    void drawMapList();
    void drawAutomap(const Camera& camera);
    void drawTileInspector();
    void drawEntityList(int currentMapID);
    void drawStatusBar(const Camera& camera, bool noclip);

    // Entity helpers
    static const char* entityTypeName(int eType);
    static unsigned int entityTypeColor(int eType);

    LoadCallback loadCallback_;
    SaveCallback saveCallback_;
    MapData* mapData_ = nullptr;
    int selectedMapIdx_ = -1;
    int selectedTileIdx_ = -1;
    int selectedSpriteIdx_ = -1;
    bool showAbout_ = false;
};
