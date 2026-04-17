#pragma once
#include "Widget.h"

class WAutomap : public Widget {
public:
	int cellSize = 0;          // 0 = use gc.automapCellSize
	bool showCursor = true;
	bool showQuestMarkers = true;
	bool showEntities = true;
	bool showWalls = true;

	WAutomap();
	void draw(Graphics* graphics, Applet* app, bool focused) override;
};
