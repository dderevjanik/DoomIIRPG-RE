#include "WAutomap.h"
#include "AutomapRenderer.h"
#include "Graphics.h"
#include "App.h"

WAutomap::WAutomap() {
	focusable = false;
}

void WAutomap::draw(Graphics* graphics, Applet* app, bool focused) {
	if (!visible) return;

	graphics->setClipRect(bounds.x, bounds.y, bounds.w, bounds.h);

	AutomapRenderParams params;
	params.originX = bounds.x;
	params.originY = bounds.y;
	params.cellSize = cellSize;
	params.showCursor = showCursor;
	params.showQuestMarkers = showQuestMarkers;
	params.showEntities = showEntities;
	params.showWalls = showWalls;
	renderAutomapContent(graphics, app, params);

	graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
}
