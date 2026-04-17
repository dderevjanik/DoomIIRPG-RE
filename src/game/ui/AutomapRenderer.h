#pragma once

class Graphics;
class Applet;

struct AutomapRenderParams {
    int originX = 0;
    int originY = 0;
    int cellSize = 0;          // 0 = use gc.automapCellSize
    bool showCursor = true;
    bool showQuestMarkers = true;
    bool showEntities = true;
    bool showWalls = true;
};

void renderAutomapContent(Graphics* graphics, Applet* app, const AutomapRenderParams& params);
