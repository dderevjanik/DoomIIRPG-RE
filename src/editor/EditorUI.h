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
    void setMapData(MapData* data) { mapData_ = data; }

    bool showAutomap = true;
    int selectedTileIdx() const { return selectedTileIdx_; }

private:
    void drawMenuBar();
    void drawMapInfo(int currentMapID);
    void drawMapList();
    void drawAutomap(const Camera& camera);
    void drawTileInspector();
    void drawStatusBar(const Camera& camera, bool noclip);

    LoadCallback loadCallback_;
    SaveCallback saveCallback_;
    MapData* mapData_ = nullptr;
    int selectedMapIdx_ = -1;
    int selectedTileIdx_ = -1;
    bool showAbout_ = false;
};
