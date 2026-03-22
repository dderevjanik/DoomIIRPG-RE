#pragma once

#include <string>
#include <vector>
#include <functional>

struct Camera;

class EditorUI {
public:
    using LoadCallback = std::function<void(int)>;

    void init(void* unused);
    void draw(int currentMapID, const Camera& camera);

    void setLoadCallback(LoadCallback cb) { loadCallback_ = std::move(cb); }

    bool showAutomap = true;

private:
    void drawMenuBar();
    void drawMapInfo(int currentMapID);
    void drawMapList();
    void drawAutomap(const Camera& camera);
    void drawStatusBar(const Camera& camera);

    LoadCallback loadCallback_;
    int selectedMapIdx_ = -1;
    bool showAbout_ = false;
};
