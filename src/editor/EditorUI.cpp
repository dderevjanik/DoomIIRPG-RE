#include "EditorUI.h"
#include "Camera.h"
#include "MapData.h"
#include "imgui.h"

#include <SDL.h>
#include <cmath>

#include "CAppContainer.h"
#include "App.h"
#include "Render.h"

void EditorUI::init(void* unused) {
    (void)unused;
}

void EditorUI::draw(int currentMapID, const Camera& camera, bool noclip) {
    drawMenuBar();

    ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse);

    drawMapInfo(currentMapID);
    ImGui::Separator();
    drawMapList();

    if (mapData_ && selectedTileIdx_ >= 0) {
        ImGui::Separator();
        drawTileInspector();
    }

    ImGui::End();

    if (showAutomap && currentMapID > 0) {
        drawAutomap(camera);
    }

    drawStatusBar(camera, noclip);

    if (showAbout_) {
        ImGui::OpenPopup("About");
        showAbout_ = false;
    }
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("DoomIIRPG Map Editor");
        ImGui::Text("Uses game rendering engine");
        ImGui::Separator();
        ImGui::Text("Part of the DoomIIRPG-RE project");
        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void EditorUI::drawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Map", "Ctrl+S", false, mapData_ != nullptr && mapData_->dirty)) {
                if (saveCallback_) saveCallback_();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
                SDL_Event quit;
                quit.type = SDL_QUIT;
                SDL_PushEvent(&quit);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About"))
                showAbout_ = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EditorUI::drawMapInfo(int currentMapID) {
    if (currentMapID <= 0) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No map loaded");
        return;
    }

    auto* app = CAppContainer::getInstance()->app;
    auto* render = app->render;

    ImGui::Text("Map: %d (map%02d.bin)", currentMapID, currentMapID - 1);
    ImGui::Text("Nodes: %d", render->numNodes);
    ImGui::Text("Lines: %d", render->numLines);
    ImGui::Text("Sprites: %d + %dz",
                render->numNormalSprites, render->numZSprites);

    if (mapData_ && mapData_->dirty) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), " [Modified]");
    }
}

void EditorUI::drawMapList() {
    ImGui::Text("Maps:");
    ImGui::BeginChild("MapList", ImVec2(0, 150), ImGuiChildFlags_Borders);
    for (int i = 1; i <= 10; i++) {
        char label[32];
        std::snprintf(label, sizeof(label), "Map %02d", i - 1);
        bool selected = (i - 1 == selectedMapIdx_);
        if (ImGui::Selectable(label, selected)) {
            selectedMapIdx_ = i - 1;
            selectedTileIdx_ = -1;
            if (loadCallback_) loadCallback_(i);
        }
    }
    ImGui::EndChild();
}

void EditorUI::drawAutomap(const Camera& camera) {
    auto* app = CAppContainer::getInstance()->app;
    auto* render = app->render;

    ImGui::SetNextWindowSize(ImVec2(300, 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("Automap", &showAutomap);

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    float mapSize = std::min(canvasSize.x, canvasSize.y);
    float tileSize = mapSize / 32.0f;

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x + mapSize, canvasPos.y + mapSize),
                      IM_COL32(20, 20, 30, 255));

    // Draw tiles — game indexes mapFlags as [row * 32 + col]
    // where col = viewX/64, row = viewY/64
    // Automap screen: col -> X (right), row -> Y (down)
    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 32; col++) {
            int idx = row * 32 + col;
            uint8_t flags = render->mapFlags[idx];

            float x = canvasPos.x + col * tileSize;
            float y = canvasPos.y + row * tileSize;

            // In editor mode show all tiles: walkable vs wall
            bool walkable = (flags & 0x80) != 0; // visited/passable flag
            bool solid = (flags & 0x1) != 0;

            ImU32 color;
            if (render->mapEntranceAutomap == idx)
                color = IM_COL32(0, 200, 0, 255);
            else if (render->mapExitAutomap == idx)
                color = IM_COL32(200, 0, 0, 255);
            else if (walkable)
                color = IM_COL32(60, 70, 100, 255);
            else if (!solid)
                color = IM_COL32(40, 45, 65, 255);
            else
                color = IM_COL32(25, 28, 40, 255);

            dl->AddRectFilled(ImVec2(x, y), ImVec2(x + tileSize - 0.5f, y + tileSize - 0.5f), color);

            // Show height as brightness overlay when MapData is available
            if (mapData_) {
                uint8_t h = mapData_->tiles[idx].height;
                if (h > 0) {
                    uint8_t alpha = (uint8_t)(h * 80 / 255);
                    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + tileSize - 0.5f, y + tileSize - 0.5f),
                                      IM_COL32(255, 255, 200, alpha));
                }
            }
        }
    }

    // Highlight selected tile
    if (selectedTileIdx_ >= 0 && selectedTileIdx_ < 1024) {
        int selCol = selectedTileIdx_ % 32;
        int selRow = selectedTileIdx_ / 32;
        float sx = canvasPos.x + selCol * tileSize;
        float sy = canvasPos.y + selRow * tileSize;
        dl->AddRect(ImVec2(sx - 1, sy - 1),
                    ImVec2(sx + tileSize + 0.5f, sy + tileSize + 0.5f),
                    IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
    }

    // Draw wall lines
    for (int i = 0; i < render->numLines; i++) {
        int flagByte = render->lineFlags[i >> 1];
        int flag = (flagByte >> ((i & 1) << 2)) & 0xF;
        if ((flag & 0x8) == 0) continue;

        float x1 = canvasPos.x + render->lineXs[i * 2] * (mapSize / 256.0f);
        float y1 = canvasPos.y + render->lineYs[i * 2] * (mapSize / 256.0f);
        float x2 = canvasPos.x + render->lineXs[i * 2 + 1] * (mapSize / 256.0f);
        float y2 = canvasPos.y + render->lineYs[i * 2 + 1] * (mapSize / 256.0f);

        dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(42, 54, 87, 255), 1.5f);
    }

    // Draw camera position
    // posX -> engine viewX -> col (automap X), posZ -> engine viewY -> row (automap Y)
    float col = camera.posX * 128.0f / 64.0f;
    float row = camera.posZ * 128.0f / 64.0f;
    float playerMapX = canvasPos.x + col * tileSize;
    float playerMapY = canvasPos.y + row * tileSize;
    dl->AddCircleFilled(ImVec2(playerMapX, playerMapY), 4.0f, IM_COL32(255, 255, 0, 255));

    // Direction arrow
    float yawRad = camera.yaw * 3.14159265f / 180.0f;
    float arrowLen = 10.0f;
    float ax = playerMapX + std::cos(yawRad) * arrowLen;
    float ay = playerMapY - std::sin(yawRad) * arrowLen;
    dl->AddLine(ImVec2(playerMapX, playerMapY), ImVec2(ax, ay),
                IM_COL32(255, 255, 0, 255), 2.0f);

    // Reserve canvas space and handle click
    ImGui::InvisibleButton("AutomapCanvas", ImVec2(mapSize, mapSize));
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        int clickCol = (int)((mousePos.x - canvasPos.x) / tileSize);
        int clickRow = (int)((mousePos.y - canvasPos.y) / tileSize);
        if (clickCol >= 0 && clickCol < 32 && clickRow >= 0 && clickRow < 32) {
            selectedTileIdx_ = clickRow * 32 + clickCol;
        }
    }

    ImGui::End();
}

void EditorUI::drawTileInspector() {
    if (!mapData_ || selectedTileIdx_ < 0 || selectedTileIdx_ >= 1024)
        return;

    TileData& tile = mapData_->tiles[selectedTileIdx_];
    int col = selectedTileIdx_ % 32;
    int row = selectedTileIdx_ / 32;

    ImGui::Text("Tile [%d, %d] (#%d)", col, row, selectedTileIdx_);

    // Height slider
    int height = tile.height;
    if (ImGui::SliderInt("Height", &height, 0, 255)) {
        tile.height = (uint8_t)height;
        mapData_->dirty = true;
    }

    // Solid flag
    bool solid = (tile.flags & 0x1) != 0;
    if (ImGui::Checkbox("Solid", &solid)) {
        if (solid)
            tile.flags |= 0x1;
        else
            tile.flags &= ~0x1;
        mapData_->dirty = true;
    }

    // Texture numbers
    if (ImGui::InputInt("Wall Texture", &tile.wallTexture)) {
        mapData_->dirty = true;
    }
    if (ImGui::InputInt("Floor Texture", &tile.floorTexture)) {
        mapData_->dirty = true;
    }

    // Spawn point
    if (ImGui::Button("Set Spawn Here")) {
        mapData_->spawnIndex = (uint16_t)selectedTileIdx_;
        mapData_->dirty = true;
    }
}

void EditorUI::drawStatusBar(const Camera& camera, bool noclip) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float barHeight = ImGui::GetFrameHeight();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - barHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, barHeight));
    ImGui::Begin("##StatusBar", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    int worldX = static_cast<int>(camera.posX * 128.0f);
    int worldY = static_cast<int>(camera.posZ * 128.0f);
    int worldZ = static_cast<int>(camera.posY * 128.0f);
    int tileCol = worldX >> 6;
    int tileRow = worldY >> 6;
    int tileIndex = tileRow * 32 + tileCol;

    ImGui::Text("World: %d, %d, %d  |  Tile: [%d,%d] (#%d)  |  Yaw: %.0f  Pitch: %.0f  |  Noclip: %s",
                worldX, worldY, worldZ, tileCol, tileRow, tileIndex, camera.yaw, camera.pitch,
                noclip ? "ON" : "OFF");

    ImGui::End();
}
