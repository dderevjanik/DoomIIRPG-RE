#include "EditorUI.h"
#include "Camera.h"
#include "MapData.h"
#include "imgui.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <cmath>

#include "CAppContainer.h"
#include "App.h"
#include "Render.h"
#include "Enums.h"
#include "EntityDef.h"

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

    if (selectedTileIdx_ >= 0) {
        ImGui::Separator();
        drawTileInspector();
    }

    ImGui::End();

    if (showAutomap && currentMapID > 0) {
        drawAutomap(camera);
    }

    if (showEntities && currentMapID > 0) {
        drawEntityList(currentMapID);
    }

    drawStatusBar(camera, noclip);

    if (showAbout_) {
        ImGui::OpenPopup("About");
        showAbout_ = false;
    }
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("DRPG Map Editor");
        ImGui::Text("Uses game rendering engine");
        ImGui::Separator();
        ImGui::Text("Part of the DRPG Engine project");
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
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[Modified]");
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

            // Show height as brightness overlay
            uint8_t h = render->heightMap[idx];
            if (h > 0) {
                uint8_t alpha = (uint8_t)(h * 80 / 255);
                dl->AddRectFilled(ImVec2(x, y), ImVec2(x + tileSize - 0.5f, y + tileSize - 0.5f),
                                  IM_COL32(255, 255, 200, alpha));
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

    // Draw entity markers on automap
    if (showEntities) {
        for (int i = 0; i < render->numMapSprites; i++) {
            int tileNum = render->mapSpriteInfo[i] & 0xFF;
            if ((render->mapSpriteInfo[i] & 0x400000) != 0)
                tileNum += 257;

            EntityDef* def = app->entityDefManager->lookup(tileNum);
            int eType = def ? def->eType : -1;

            // Sprite coords are in 64-per-tile scale
            float sprCol = render->mapSprites[render->S_X + i] / 64.0f;
            float sprRow = render->mapSprites[render->S_Y + i] / 64.0f;
            float mx = canvasPos.x + sprCol * tileSize;
            float my = canvasPos.y + sprRow * tileSize;

            ImU32 entColor = entityTypeColor(eType);
            float radius = (selectedSpriteIdx_ == i) ? 4.0f : 2.5f;
            dl->AddCircleFilled(ImVec2(mx, my), radius, entColor);

            if (selectedSpriteIdx_ == i)
                dl->AddCircle(ImVec2(mx, my), 6.0f, IM_COL32(255, 255, 255, 255), 0, 1.5f);
        }
    }

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
    if (selectedTileIdx_ < 0 || selectedTileIdx_ >= 1024)
        return;

    auto* app = CAppContainer::getInstance()->app;
    auto* render = app->render;

    int col = selectedTileIdx_ % 32;
    int row = selectedTileIdx_ / 32;

    ImGui::Text("Tile [%d, %d] (#%d)", col, row, selectedTileIdx_);

    if (mapData_) {
        // Editable mode
        TileData& tile = mapData_->tiles[selectedTileIdx_];

        int height = tile.height;
        if (ImGui::InputInt("Height", &height)) {
            if (height < 0) height = 0;
            if (height > 255) height = 255;
            tile.height = (uint8_t)height;
            mapData_->dirty = true;
        }
        ImGui::Text("World height: %d", tile.height << 3);

        bool solid = (tile.flags & 0x1) != 0;
        if (ImGui::Checkbox("Solid", &solid)) {
            if (solid)
                tile.flags |= 0x1;
            else
                tile.flags &= ~0x1;
            mapData_->dirty = true;
        }

        bool hasEvent = (tile.flags & 0x40) != 0;
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::Checkbox("Event", &hasEvent);
        ImGui::EndDisabled();

        int wallTex = tile.wallTexture;
        if (ImGui::InputInt("Wall Tex", &wallTex)) {
            if (wallTex < 0) wallTex = 0;
            if (wallTex > 511) wallTex = 511;
            tile.wallTexture = wallTex;
            mapData_->dirty = true;
        }

        int floorTex = tile.floorTexture;
        if (ImGui::InputInt("Floor Tex", &floorTex)) {
            if (floorTex < 0) floorTex = 0;
            if (floorTex > 511) floorTex = 511;
            tile.floorTexture = floorTex;
            mapData_->dirty = true;
        }

        if (ImGui::Button("Set Spawn Here")) {
            mapData_->spawnIndex = (uint16_t)selectedTileIdx_;
            mapData_->dirty = true;
        }
    } else {
        // Read-only fallback (no MapData loaded)
        uint8_t flags = render->mapFlags[selectedTileIdx_];
        ImGui::Text("Flags: 0x%02X %s%s", flags,
                    (flags & 0x1) ? "[Solid] " : "",
                    (flags & 0x40) ? "[Event]" : "");

        int height = render->heightMap[selectedTileIdx_];
        ImGui::Text("Height: %d (world: %d)", height, height << 3);
    }
}

const char* EditorUI::entityTypeName(int eType) {
    switch (eType) {
        case 0: return "World";
        case 1: return "Player";
        case 2: return "Monster";
        case 3: return "NPC";
        case 4: return "PlayerClip";
        case 5: return "Door";
        case 6: return "Item";
        case 7: return "Decor";
        case 8: return "EnvDamage";
        case 9: return "Corpse";
        case 10: return "Interactive";
        case 11: return "MonsterBlock";
        case 12: return "SpriteWall";
        case 13: return "NoBlockWall";
        case 14: return "DecorNoclip";
        default: return "Unknown";
    }
}

unsigned int EditorUI::entityTypeColor(int eType) {
    switch (eType) {
        case 2:  return IM_COL32(255, 60, 60, 255);    // Monster - red
        case 3:  return IM_COL32(60, 180, 255, 255);    // NPC - light blue
        case 5:  return IM_COL32(255, 200, 50, 255);    // Door - yellow
        case 6:  return IM_COL32(50, 255, 50, 255);     // Item - green
        case 7:  return IM_COL32(150, 150, 150, 255);   // Decor - gray
        case 8:  return IM_COL32(255, 100, 0, 255);     // EnvDamage - orange
        case 9:  return IM_COL32(120, 80, 60, 255);     // Corpse - brown
        case 12: return IM_COL32(180, 180, 255, 255);   // SpriteWall - light purple
        case 13: return IM_COL32(140, 140, 200, 255);   // NoBlockWall - dim purple
        case 14: return IM_COL32(130, 130, 130, 255);   // DecorNoclip - dim gray
        default: return IM_COL32(200, 200, 200, 255);   // Other - white-ish
    }
}

void EditorUI::drawEntityList(int currentMapID) {
    if (currentMapID <= 0) return;

    auto* app = CAppContainer::getInstance()->app;
    auto* render = app->render;

    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Entities", &showEntities);

    ImGui::Text("Sprites: %d (%d normal + %d z)",
                render->numMapSprites, render->numNormalSprites, render->numZSprites);
    ImGui::Separator();

    // Filter buttons
    static int filterType = -1; // -1 = show all
    if (ImGui::Button("All")) filterType = -1;
    ImGui::SameLine();
    if (ImGui::Button("Monsters")) filterType = 2;
    ImGui::SameLine();
    if (ImGui::Button("Items")) filterType = 6;
    ImGui::SameLine();
    if (ImGui::Button("Doors")) filterType = 5;
    ImGui::SameLine();
    if (ImGui::Button("NPCs")) filterType = 3;

    ImGui::Separator();

    ImGui::BeginChild("EntityList", ImVec2(0, 0), ImGuiChildFlags_Borders);

    for (int i = 0; i < render->numMapSprites; i++) {
        int tileNum = render->mapSpriteInfo[i] & 0xFF;
        if ((render->mapSpriteInfo[i] & 0x400000) != 0)
            tileNum += 257;

        EntityDef* def = app->entityDefManager->lookup(tileNum);
        int eType = def ? def->eType : -1;

        if (filterType >= 0 && eType != filterType)
            continue;

        int sx = render->mapSprites[render->S_X + i];
        int sy = render->mapSprites[render->S_Y + i];
        int tileCol = sx >> 6;
        int tileRow = sy >> 6;

        ImU32 color = entityTypeColor(eType);
        ImVec4 colVec = ImGui::ColorConvertU32ToFloat4(color);

        char label[64];
        const char* typeName = def ? entityTypeName(eType) : "???";
        std::snprintf(label, sizeof(label), "#%d [%d,%d] %s (tile %d)",
                      i, tileCol, tileRow, typeName, tileNum);

        ImGui::PushStyleColor(ImGuiCol_Text, colVec);
        bool selected = (selectedSpriteIdx_ == i);
        if (ImGui::Selectable(label, selected)) {
            selectedSpriteIdx_ = i;
            // Also select the tile this entity is on
            if (tileCol >= 0 && tileCol < 32 && tileRow >= 0 && tileRow < 32)
                selectedTileIdx_ = tileRow * 32 + tileCol;
        }
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && def) {
            ImGui::BeginTooltip();
            ImGui::Text("Type: %s (eType=%d, sub=%d)", entityTypeName(eType), eType, def->eSubType);
            ImGui::Text("Tile: %d  Parm: %d", tileNum, def->parm);
            ImGui::Text("Pos: %d, %d  Tile: [%d,%d]", sx, sy, tileCol, tileRow);
            bool isWall = (render->mapSpriteInfo[i] & 0x800000) != 0;
            if (isWall) ImGui::Text("(Sprite Wall)");
            ImGui::EndTooltip();
        }
    }

    ImGui::EndChild();
    ImGui::End();
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
