#include "TravelMapState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Player.h"
#include "Render.h"
#include "Graphics.h"
#include "Image.h"
#include "Text.h"
#include "Button.h"
#include "TinyGL.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Enums.h"
#include "Menus.h"

// Forward declarations for static helpers
static bool onMoon(int n) { return n >= 1 && n <= 3; }
static bool onEarth(int n) { return n >= 4 && n <= 6; }
static bool inHell(int n) { return n >= 7; }
static void drawStarFieldPage(Canvas* canvas, Graphics* graphics);
static void drawStarField(Canvas* canvas, Graphics* graphics, int x, int y);
static void runStarFieldFrame(Canvas* canvas);
static void drawGridLines(Canvas* canvas, Graphics* graphics, int i);
static void drawAppropriateCloseup(Canvas* canvas, Graphics* graphics, int n, bool b);
static void drawLocatorBoxAndName(Canvas* canvas, Graphics* graphics, bool b, int n, Text* text);
static bool drawDottedLine(Canvas* canvas, Graphics* graphics, int n);
static bool drawDottedLine(Canvas* canvas, Graphics* graphics);
static bool drawMarsToMoonLinePlusSpaceShip(Canvas* canvas, Graphics* graphics, int n);
static int yCoordOfSpaceShip(Canvas* canvas, int n);
static bool drawMoonToEarthLine(Canvas* canvas, Graphics* graphics, int n, bool b);
static bool drawEarthToHellLine(Canvas* canvas, Graphics* graphics, int n, bool b);
static bool drawLocatorLines(Canvas* canvas, Graphics* graphics, int n, bool b, bool b2);
static bool newLevelSamePlanet(Canvas* canvas);
static void finishTravelMapAndLoadLevel(Canvas* canvas);
static void disposeTravelMap(Canvas* canvas);
static void initTravelMap(Canvas* canvas);
static void handleTravelMapInput(Canvas* canvas, int key, int action);
static void drawTravelMap(Canvas* canvas, Graphics* graphics);


static void drawTravelMap(Canvas* canvas, Graphics* graphics) {


	if (canvas->stateVars[5] == 1) {
		drawStarFieldPage(canvas, graphics);
		canvas->staleView = true;
		return;
	}

	graphics->drawImage(canvas->imgTravelBG, canvas->SCR_CX, (canvas->displayRect[3] - canvas->imgTravelBG->height) / 2, 17, 0, 0);

	if (canvas->xDiff > 1 || canvas->yDiff > 1) {
		graphics->fillRegion(canvas->imgFabricBG, 0, 0, canvas->displayRect[2], canvas->yDiff);
		graphics->fillRegion(canvas->imgFabricBG, 0, canvas->yDiff, canvas->xDiff, canvas->displayRect[3] - canvas->yDiff);
		graphics->fillRegion(canvas->imgFabricBG, canvas->xDiff, canvas->yDiff + canvas->mapHeight, canvas->mapWidth, canvas->yDiff);
		graphics->fillRegion(canvas->imgFabricBG, canvas->xDiff + canvas->mapWidth, canvas->yDiff, canvas->xDiff, canvas->displayRect[3] - canvas->yDiff);
		graphics->drawRect(canvas->xDiff + -1, canvas->yDiff + -1, canvas->mapWidth + 1, canvas->mapHeight + 1, 0xFF000000);
		graphics->clipRect(canvas->xDiff, canvas->yDiff, canvas->imgTravelBG->width, canvas->imgTravelBG->height);
	}

	int time = canvas->app->upTimeMs - canvas->stateVars[0];
	drawGridLines(canvas, graphics, time + canvas->totalTMTimeInPastAnimations);

	bool levelSamePlanet = newLevelSamePlanet(canvas);
	if (canvas->stateVars[1] != 1) {
		if (time > (levelSamePlanet ? 1500 : 700)) {
			canvas->totalTMTimeInPastAnimations += time;
			canvas->stateVars[0] += time;
			canvas->stateVars[1] = 1;
			time = 0;
			if (levelSamePlanet) {
				canvas->stateVars[2] = 1;
				canvas->stateVars[3] = 1;
			}
		}
		else if (levelSamePlanet) {
			drawAppropriateCloseup(canvas, graphics, canvas->TM_LastLevelId, false);
			Text* smallBuffer = canvas->app->localization->getSmallBuffer();
			smallBuffer->setLength(0);
			canvas->app->localization->composeText((short)3, canvas->app->game->levelNames[canvas->TM_LastLevelId - 1], smallBuffer);
			smallBuffer->dehyphenate();
			drawLocatorBoxAndName(canvas, graphics, (time & 0x200) == 0x0, canvas->TM_LastLevelId, smallBuffer);
			smallBuffer->dispose();
		}
	}

	if (canvas->stateVars[1] == 1) {
		if (canvas->stateVars[2] != 1) {
			if (drawDottedLine(canvas, graphics, time)) {
				canvas->totalTMTimeInPastAnimations += time;
				canvas->stateVars[0] += time;
				canvas->stateVars[2] = 1;
			}
		}
		else if (canvas->stateVars[2] == 1) {
			if (canvas->stateVars[3] != 1) {
				drawDottedLine(canvas, graphics);
				if (time > 500 || levelSamePlanet) {
					canvas->totalTMTimeInPastAnimations += time;
					canvas->stateVars[0] += time;
					canvas->stateVars[3] = 1;
				}
			}
			else if (canvas->stateVars[3] == 1) {
				drawAppropriateCloseup(canvas, graphics, canvas->TM_LoadLevelId, canvas->stateVars[8] == 1);
				if (canvas->stateVars[6] == 1) {
					Text* smallBuffer2 = canvas->app->localization->getSmallBuffer();
					smallBuffer2->setLength(0);
					canvas->app->localization->composeText((short)3, canvas->app->game->levelNames[canvas->TM_LoadLevelId - 1], smallBuffer2);
					smallBuffer2->dehyphenate();

					drawLocatorBoxAndName(canvas, graphics, (time & 0x200) == 0x0, canvas->TM_LoadLevelId, smallBuffer2);
					smallBuffer2->setLength(0);
					canvas->app->localization->composeText((short)0, (short)96, smallBuffer2);
					smallBuffer2->wrapText(24);
					smallBuffer2->dehyphenate();
					graphics->drawString(smallBuffer2, canvas->SCR_CX, canvas->displayRect[3] - 21, 17);
					smallBuffer2->dispose();
				}
				else if (canvas->stateVars[4] != 1) {
					if (drawLocatorLines(canvas, graphics, time, levelSamePlanet, canvas->stateVars[8] == 1)) {
						canvas->totalTMTimeInPastAnimations += time;
						canvas->stateVars[0] += time;
						canvas->stateVars[4] = 1;
					}
				}
				else {
					drawLocatorLines(canvas, graphics, -1, levelSamePlanet, canvas->stateVars[8] == 1);
					if (time > 800) {
						canvas->totalTMTimeInPastAnimations += time;
						canvas->stateVars[0] += time;
						if (canvas->stateVars[8] == 0) {
							canvas->stateVars[6] = 1;
						}
						else {
							canvas->stateVars[4] = 0;
							canvas->stateVars[8] = 0;
							int n7 = 123;
							int n8 = 150;
							short n9 = (short)(2 * (canvas->TM_LoadLevelId - 1));
							canvas->targetX = Canvas::CROSS_HAIR_CORDS[n9] + canvas->xDiff + n7;
							canvas->targetY = Canvas::CROSS_HAIR_CORDS[n9 + 1] + canvas->yDiff + n8;
						}
					}
				}
			}
		}
	}

	canvas->staleView = true;
}


static bool newLevelSamePlanet(Canvas* canvas) {
	return canvas->TM_LastLevelId != canvas->TM_LoadLevelId && ((onMoon(canvas->TM_LastLevelId) && onMoon(canvas->TM_LoadLevelId)) || (onEarth(canvas->TM_LastLevelId) && onEarth(canvas->TM_LoadLevelId)) || (inHell(canvas->TM_LastLevelId) && inHell(canvas->TM_LoadLevelId)));
}


static void drawAppropriateCloseup(Canvas* canvas, Graphics* graphics, int n, bool b) {
	if (onMoon(n)) {
		graphics->drawImage(canvas->imgTierCloseUp, Canvas::moonCoords[0], Canvas::moonCoords[1], 0, 0, 0);
	}
	else if (onEarth(n)) {
		if (b) {
			graphics->drawImage(canvas->imgEarthCloseUp, Canvas::earthCoords[0], Canvas::earthCoords[1], 0, 0, 0);
		}
		else {
			graphics->drawImage(canvas->imgTierCloseUp, Canvas::earthCoords[2], Canvas::earthCoords[3], 0, 0, 0);
		}
	}
	else {
		graphics->drawImage(canvas->imgTierCloseUp, Canvas::hellCoords[0], Canvas::hellCoords[1], 0, 0, 0);
	}
}


static bool drawDottedLine(Canvas* canvas, Graphics* graphics, int n) {
	int n2 = n / 22;
	bool b;
	if (canvas->TM_LastLevelId == 0 && onMoon(canvas->TM_LoadLevelId)) {
		b = drawMarsToMoonLinePlusSpaceShip(canvas, graphics, n2);
	}
	else if (onMoon(canvas->TM_LastLevelId) && onEarth(canvas->TM_LoadLevelId)) {
		b = drawMoonToEarthLine(canvas, graphics, n2, false);
	}
	else if (onEarth(canvas->TM_LastLevelId) && onMoon(canvas->TM_LoadLevelId)) {
		b = drawMoonToEarthLine(canvas, graphics, n2, true);
	}
	else if (onEarth(canvas->TM_LastLevelId) && inHell(canvas->TM_LoadLevelId)) {
		b = drawEarthToHellLine(canvas, graphics, n2, false);
	}
	else {
		b = (!inHell(canvas->TM_LastLevelId) || !onEarth(canvas->TM_LoadLevelId) || drawEarthToHellLine(canvas, graphics, n2, true));
	}
	return b;
}


static bool drawDottedLine(Canvas* canvas, Graphics* graphics) {
	return drawDottedLine(canvas, graphics, 22 * std::max(canvas->screenRect[2], canvas->screenRect[3]));
}


static bool drawMarsToMoonLinePlusSpaceShip(Canvas* canvas, Graphics* graphics, int n) {


	bool b = false;
	int width = canvas->imgTravelPath->width;
	int n2 = n;
	if (n2 > width) {
		b = true;
		n2 = width;
	}
	if (n > width / 3) {
		Text* smallBuffer = canvas->app->localization->getSmallBuffer();
		smallBuffer->setLength(0);
		canvas->app->localization->composeText((short)3, (short)165, smallBuffer);
		smallBuffer->dehyphenate();
		graphics->drawString(smallBuffer, Canvas::moonNameCoords[0], Canvas::moonNameCoords[1], 4);
		smallBuffer->dispose();
	}
	graphics->drawRegion(canvas->imgTravelPath, width - n2, 0, n2, canvas->imgTravelPath->height, Canvas::moonPathCoords[0] + width - n2, Canvas::moonPathCoords[1], 0, 0, 0);
	int n3 = width - n2;
	graphics->drawImage(canvas->imgSpaceShip, Canvas::moonPathCoords[0] + n3 - canvas->imgSpaceShip->width, yCoordOfSpaceShip(canvas, n3) + Canvas::moonPathCoords[1] - (canvas->imgSpaceShip->height >> 1), 0, 0, 0);
	return b;
}


static int yCoordOfSpaceShip(Canvas* canvas, int n) {
	int iVar1;
	iVar1 = (n * 11) / 15;
	return ((-851 * (iVar1 * iVar1) / 110880 + 6115 * iVar1 / 22176 + 40) * 15) / 11;
}


static bool drawMoonToEarthLine(Canvas* canvas, Graphics* graphics, int n, bool b) {


	bool b2 = false;
	int height = canvas->imgTravelPath->height;
	int n2 = n;
	if (n2 > height) {
		b2 = true;
		n2 = height;
	}
	int n3 = b ? 0 : (height - n2);
	int width = canvas->imgTravelPath->width;
	if (n > height / 2) {
		Text* smallBuffer = canvas->app->localization->getSmallBuffer();
		smallBuffer->setLength(0);
		if (!b) {
			canvas->app->localization->composeText((short)3, (short)164, smallBuffer);
			smallBuffer->dehyphenate();
			graphics->drawString(smallBuffer, Canvas::earthNameCoords[0], Canvas::earthNameCoords[1], 4);
		}
		else {
			canvas->app->localization->composeText((short)3, (short)165, smallBuffer);
			smallBuffer->dehyphenate();
			graphics->drawString(smallBuffer, Canvas::moonNameCoords[2], Canvas::moonNameCoords[3], 4);
		}
		smallBuffer->dispose();
	}
	graphics->drawRegion(canvas->imgTravelPath, 0, n3, width, n2, Canvas::earthPathCoords[0], Canvas::earthPathCoords[1] + n3, 0, 0, 0);
	return b2;
}


static bool drawEarthToHellLine(Canvas* canvas, Graphics* graphics, int n, bool b) {


	bool b2 = false;
	int height = canvas->imgTravelPath->height;
	int n2 = n;
	if (n2 > height) {
		b2 = true;
		n2 = height;
	}
	int n3 = b ? 0 : (height - n2);
	int width = canvas->imgTravelPath->width;
	if (n > 2 * height / 3) {
		Text* smallBuffer = canvas->app->localization->getSmallBuffer();
		smallBuffer->setLength(0);
		if (!b) {
			canvas->app->localization->composeText((short)3, (short)166, smallBuffer);
			smallBuffer->dehyphenate();
			graphics->drawString(smallBuffer, Canvas::hellNameCoords[0], Canvas::hellNameCoords[1], 4);
		}
		else {
			canvas->app->localization->composeText((short)3, (short)164, smallBuffer);
			smallBuffer->dehyphenate();
			graphics->drawString(smallBuffer, Canvas::earthNameCoords[2], Canvas::earthNameCoords[3], 4);
		}
		smallBuffer->dispose();
	}
	graphics->drawRegion(canvas->imgTravelPath, 0, n3, width, n2, Canvas::hellPathCoords[0], Canvas::hellPathCoords[1] + n3, 0, 0, 0);
	return b2;
}


static void drawLocatorBoxAndName(Canvas* canvas, Graphics* graphics, bool b, int n, Text* text) {
	int n2;
	int n3;
	if (onMoon(n)) {
		n2 = Canvas::moonCoords[0];
		n3 = Canvas::moonCoords[1];
	}
	else if (onEarth(n)) {
		n2 = Canvas::earthCoords[0];
		n3 = Canvas::earthCoords[1];
	}
	else {
		n2 = Canvas::hellCoords[0];
		n3 = Canvas::hellCoords[1];
	}
	short n4 = (short)(2 * (n - 1));
	int n5 = Canvas::CROSS_HAIR_CORDS[n4] + canvas->xDiff + n2;
	int n6 = Canvas::CROSS_HAIR_CORDS[n4 + 1] + canvas->yDiff + n3;
	if (b) {
		graphics->drawImage(canvas->imgMagGlass, n5, n6, 3, 0, 0);
	}

	int x = Canvas::LOCATOR_BOX_CORDS[n4] + canvas->xDiff + n2;
	int y = Canvas::LOCATOR_BOX_CORDS[n4 + 1] + canvas->yDiff + n3 + (canvas->imgNameHighlight->height >> 1);
	graphics->drawImage(canvas->imgNameHighlight, x, y, 6, 0, 0);
	canvas->graphics.currentCharColor = 3;
	graphics->drawString(text, x + (canvas->imgNameHighlight->width >> 1), y, 3);
}


static void drawGridLines(Canvas* canvas, Graphics* graphics, int i) {
	int iVar1;

	for (iVar1 = (i / 200) % 44 + canvas->displayRect[0]; iVar1 < canvas->displayRect[2]; iVar1 += 44) {
		graphics->drawImage(canvas->imgMapVertGridLines, iVar1, canvas->displayRect[1], 0x14, 0, 2);
	}
	for (iVar1 = canvas->displayRect[1] + 5; iVar1 < canvas->displayRect[3]; iVar1 += 44) {
		graphics->drawImage(canvas->imgMapHorzGridLines, canvas->displayRect[0], iVar1, 20, 0, 2);
		graphics->drawImage(canvas->imgMapHorzGridLines, 240, iVar1, 20, 0, 2);
	}
}








static void handleTravelMapInput(Canvas* canvas, int key, int action) {
#if 0 // IOS
	if ((canvas->stateVars[6] == 1) || (key == 18) || (action == Enums::ACTION_FIRE)) {
		finishTravelMapAndLoadLevel(canvas);
	}
	else {
		canvas->stateVars[6] = 1;
	}
#else // J2ME/BREW

	bool hasSavedState = canvas->app->game->hasSavedState();

	if (action == Enums::ACTION_MENU) { // [GEC] skip all
		finishTravelMapAndLoadLevel(canvas);
		return;
	}

	if (action == Enums::ACTION_FIRE) {
		if (canvas->stateVars[6] == 1) {
#if 0 // [GEC] no esta disponible en ningina de las versiones del juego
			if (canvas->TM_LastLevelId == 0 && onMoon(canvas->TM_LoadLevelId)) {
				canvas->stateVars[5] = 1; // Draw start field
			}
			else {
				finishTravelMapAndLoadLevel(canvas);
			}
#else
			finishTravelMapAndLoadLevel(canvas);
#endif
		}
		else if (hasSavedState) {
			if (canvas->stateVars[2] == 1) {
				if (canvas->stateVars[8] == 0) {
					canvas->stateVars[3] = 1;
					canvas->stateVars[4] = 1;
					canvas->stateVars[6] = 1;
				}
				else {
					int n3 = canvas->app->upTimeMs - canvas->stateVars[0];
					canvas->totalTMTimeInPastAnimations += n3;
					canvas->stateVars[0] += n3;
					if (canvas->stateVars[4] == 1) {
						canvas->stateVars[4] = 0;
						canvas->stateVars[8] = 0;
						int n5 = canvas->earthCoords[0];
						int n6 = canvas->earthCoords[1];
						short n7 = (short)(2 * (canvas->TM_LoadLevelId - 1));
						canvas->targetX = canvas->CROSS_HAIR_CORDS[n7] + canvas->xDiff + n5;
						canvas->targetY = canvas->CROSS_HAIR_CORDS[n7 + 1] + canvas->yDiff + n6;
					}
					else {
						canvas->stateVars[3] = 1;
						canvas->stateVars[4] = 1;
					}
				}
			}
			else {
				canvas->totalTMTimeInPastAnimations += canvas->app->upTimeMs - canvas->stateVars[0];
				canvas->stateVars[0] = canvas->app->upTimeMs;
				canvas->stateVars[1] = 1;
				canvas->stateVars[2] = 1;
				if (newLevelSamePlanet(canvas)) {
					canvas->stateVars[3] = 1;
				}
			}
		}
	}
	else if (action == Enums::ACTION_AUTOMAP && canvas->stateVars[5] == 1) {
		finishTravelMapAndLoadLevel(canvas);
	}
#endif
}

static void finishTravelMapAndLoadLevel(Canvas* canvas) {
	canvas->clearSoftKeys();
	disposeTravelMap(canvas);
	canvas->setLoadingBarText((short)0, (short)41);
	canvas->setState(Canvas::ST_LOADING);
}


static bool drawLocatorLines(Canvas* canvas, Graphics* graphics, int n, bool b, bool b2) {
	int targetX;
	int targetY;
	int targetX2;
	int targetY2;
	if (n >= 0) {
		int n2 = n / (b ? 15 : 7);
		if (b) {
			int n3;
			int n4;
			if (onMoon(canvas->TM_LastLevelId)) {
				n3 = Canvas::moonCoords[0];
				n4 = Canvas::moonCoords[1];
			}
			else if (onEarth(canvas->TM_LastLevelId)) {
				if (b2) {
					n3 = Canvas::earthCoords[2];
					n4 = Canvas::earthCoords[3];
				}
				else {
					n3 = Canvas::earthCoords[0];
					n4 = Canvas::earthCoords[1];
				}
			}
			else {
				n3 = Canvas::hellCoords[0];
				n4 = Canvas::hellCoords[1];
			}
			short n5 = (short)(2 * (canvas->TM_LastLevelId - 1));
			targetX = Canvas::CROSS_HAIR_CORDS[n5] + canvas->xDiff + n3;
			targetY = Canvas::CROSS_HAIR_CORDS[n5 + 1] + canvas->yDiff + n4;
		}
		else {
			targetX = canvas->displayRect[2] - canvas->xDiff;
			targetY = canvas->displayRect[3] - canvas->yDiff;
		}
		int n6 = (canvas->targetX > targetX) ? 1 : ((canvas->targetX == targetX) ? 0 : -1);
		int n7 = (canvas->targetY > targetY) ? 1 : ((canvas->targetY == targetY) ? 0 : -1);
		targetX2 = targetX + n6 * n2;
		targetY2 = targetY + n7 * n2;
	}
	else {
		targetX2 = (targetX = canvas->targetX);
		targetY2 = (targetY = canvas->targetY);
	}
	bool b3 = false;
	bool b4 = false;
	if ((canvas->targetX - targetX) * (canvas->targetX - targetX2) < 0 || canvas->targetX == targetX2) {
		targetX2 = canvas->targetX;
		b3 = true;
	}
	if ((canvas->targetY - targetY) * (canvas->targetY - targetY2) < 0 || canvas->targetY == targetY2) {
		targetY2 = canvas->targetY;
		b4 = true;
	}
	graphics->drawLine(targetX2 + 1, canvas->displayRect[1], targetX2 + 1, canvas->displayRect[3], 0xFF000000);
	graphics->drawLine(canvas->displayRect[0], targetY2 + 1, canvas->displayRect[2], targetY2 + 1, 0xFF000000);
	graphics->drawLine(targetX2, canvas->displayRect[1], targetX2, canvas->displayRect[3], 0xFFBDFD80);
	graphics->drawLine(canvas->displayRect[0], targetY2, canvas->displayRect[2], targetY2, 0xFFBDFD80);
	return b3 && b4;
}


static void initTravelMap(Canvas* canvas) {


	canvas->TM_LoadLevelId = (short)std::max(1, std::min(canvas->loadMapID, 9));
	canvas->TM_LastLevelId = ((canvas->getRecentLoadType() == 1 && !canvas->TM_NewGame) ? canvas->TM_LoadLevelId : ((short)std::max(0, std::min(canvas->lastMapID, 9))));

	if (canvas->TM_LastLevelId == 0 && canvas->TM_LoadLevelId > 1) {
		canvas->TM_LastLevelId = (short)(canvas->TM_LoadLevelId - 1);
	}
	short n;
	if (canvas->TM_LoadLevelId > canvas->TM_LastLevelId) {
		n = 0;
	}
	else if (canvas->TM_LoadLevelId == canvas->TM_LastLevelId) {
		n = 1;
	}
	else {
		n = 2;
	}
	canvas->app->game->scriptStateVars[15] = n;

	canvas->app->beginImageLoading();
	canvas->imgNameHighlight = canvas->app->loadImage("highlight.bmp", true);
	canvas->imgMagGlass = canvas->app->loadImage("magnifyingGlass.bmp", true);
	canvas->imgSpaceShip = canvas->app->loadImage("spaceShip.bmp", true);

	bool b = false;
	if (onMoon(canvas->TM_LoadLevelId)) {
		canvas->imgTierCloseUp = canvas->app->loadImage("TM_Levels1.bmp", true);
	}
	else if (onEarth(canvas->TM_LoadLevelId)) {
		if (onMoon(canvas->TM_LastLevelId) || inHell(canvas->TM_LastLevelId)) {
			canvas->imgEarthCloseUp = canvas->app->loadImage("TM_Levels2.bmp", true);
			b = true;
		}
		canvas->imgTierCloseUp = canvas->app->loadImage("TM_Levels4.bmp", true);
	}
	else {
		canvas->imgTierCloseUp = canvas->app->loadImage("TM_Levels3.bmp", true);
	}

	if (onMoon(canvas->TM_LoadLevelId) && canvas->TM_LastLevelId == 0) {
		canvas->imgTravelPath = canvas->app->loadImage("toMoon.bmp", true);
	}
	else if ((onEarth(canvas->TM_LoadLevelId) && onMoon(canvas->TM_LastLevelId)) || (onMoon(canvas->TM_LoadLevelId) && onEarth(canvas->TM_LastLevelId))) {
		canvas->imgTravelPath = canvas->app->loadImage("toEarth.bmp", true);
	}
	else if ((inHell(canvas->TM_LoadLevelId) && onEarth(canvas->TM_LastLevelId)) || (onEarth(canvas->TM_LoadLevelId) && inHell(canvas->TM_LastLevelId))) {
		canvas->imgTravelPath = canvas->app->loadImage("toHell.bmp", true);
	}

	canvas->imgTravelBG = canvas->app->loadImage("TravelMap.bmp", true);
	canvas->imgMapHorzGridLines = canvas->app->loadImage("travelMapHorzGrid.bmp", true);
	canvas->imgMapVertGridLines = canvas->app->loadImage("travelMapVertGrid.bmp", true);
	canvas->app->endImageLoading();

	canvas->totalTMTimeInPastAnimations = 0;
	canvas->mapWidth = canvas->imgTravelBG->width;
	canvas->mapHeight = canvas->imgTravelBG->height;
	canvas->xDiff = std::max(0, (canvas->displayRect[2] - canvas->imgTravelBG->width) / 2);
	canvas->yDiff = std::max(0, (canvas->displayRect[3] - canvas->imgTravelBG->height) / 2);

	int n2;
	int n3;
	if (onMoon(canvas->TM_LoadLevelId)) {
		n2 = 137;
		n3 = 216;
	}
	else if (onEarth(canvas->TM_LoadLevelId)) {
		if (b) {
			n2 = 123;
			n3 = 150;
		}
		else {
			n2 = 123;
			n3 = 150;
		}
	}
	else {
		n2 = 150;
		n3 = 11;
	}

	if (!b) {
		short n4 = (short)(2 * (canvas->TM_LoadLevelId - 1));
		canvas->targetX = Canvas::CROSS_HAIR_CORDS[n4] + canvas->xDiff + n2;
		canvas->targetY = Canvas::CROSS_HAIR_CORDS[n4 + 1] + canvas->yDiff + n3;
	}
	else {
		canvas->targetX = Canvas::UAC_BUILDING_LOCATION_ON_EARTH[0] + canvas->xDiff + n2;
		canvas->targetY = Canvas::UAC_BUILDING_LOCATION_ON_EARTH[1] + canvas->yDiff + n3;
	}

	canvas->stateVars[0] = canvas->app->upTimeMs;;
	if (b) {
		canvas->stateVars[8] = 1;
	}

	// Unused
	canvas->imgStarField = canvas->app->loadImage("cockpit.bmp", true);
	canvas->_field_0xf24 = Applet::IOS_WIDTH;
	canvas->_field_0xf20 = canvas->imgStarField->height;
	canvas->_field_0xf28 = 0;
	canvas->_field_0xf2c = 1;
}


static void disposeTravelMap(Canvas* canvas) {
	canvas->imgNameHighlight->~Image();
	canvas->imgNameHighlight = nullptr;
	canvas->imgMagGlass->~Image();
	canvas->imgMagGlass = nullptr;
	canvas->imgTravelBG->~Image();
	canvas->imgTravelBG = nullptr;
	canvas->imgTravelPath->~Image();
	canvas->imgTravelPath = nullptr;
	canvas->imgSpaceShip->~Image();
	canvas->imgSpaceShip = nullptr;
	canvas->imgTierCloseUp->~Image();
	canvas->imgTierCloseUp = nullptr;
	canvas->imgEarthCloseUp->~Image();
	canvas->imgEarthCloseUp = nullptr;
	canvas->imgMapHorzGridLines->~Image();
	canvas->imgMapHorzGridLines = nullptr;
	canvas->imgMapVertGridLines->~Image();
	canvas->imgMapVertGridLines = nullptr;

	// Unused
	canvas->imgStarField->~Image();
	canvas->imgStarField = nullptr;
}


static void drawStarFieldPage(Canvas* canvas, Graphics* graphics) {


	if (canvas->app->upTimeMs - canvas->stateVars[0] > 5250) {
		canvas->_field_0xf2c = 2u;
		finishTravelMapAndLoadLevel(canvas);
	}
	else {
		int yPos = canvas->screenRect[3] - canvas->screenRect[1] - canvas->imgStarField->height;
		graphics->fillRect(0, 0, canvas->menuRect[2], canvas->menuRect[3], 0xFF000000);
		drawStarField(canvas, graphics, 1, yPos / 2);
		graphics->drawImage(canvas->imgStarField, 1, yPos / 2, 0, 0, 0);
		graphics->drawImage(canvas->imgStarField, canvas->screenRect[2] - 1, yPos / 2, 24, 4, 0);
		canvas->app->canvas->softKeyRightID = -1;
		canvas->app->canvas->softKeyLeftID = -1;
		canvas->app->canvas->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
		if (canvas->app->canvas->displaySoftKeys) {
			canvas->app->canvas->softKeyRightID = 40;
			canvas->app->canvas->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
		}
	}
}


static void drawStarField(Canvas* canvas, Graphics* graphics, int x, int y) {

	int upTimeMs; // r2
	int result; // r0
	int field_0xf20; // r2
	unsigned int v8; // r2
	unsigned int v9; // r11
	int v10; // r6
	int v11; // r0
	signed int v12; // r5
	int v13; // r6
	int v14; // r3
	int v15; // r0
	int i; // r4
	int v17; // r10
	int v18; // r8
	int v23; // [sp+18h] [bp-2Ch]
	int v24; // [sp+1Ch] [bp-28h]
	int v25; // [sp+20h] [bp-24h]
	int v26; // [sp+24h] [bp-20h]
	int v27; // [sp+28h] [bp-1Ch]

	graphics->fillRect(x, y, canvas->_field_0xf24, canvas->_field_0xf20, 0xFF000000);
	if (canvas->app->upTimeMs - canvas->stateVars[7] > 59) {
		canvas->stateVars[7] = canvas->app->upTimeMs;
		runStarFieldFrame(canvas);
	}

	v23 = canvas->_field_0xf24 / 2;
	field_0xf20 = canvas->_field_0xf20;
	v26 = y;
	v25 = 0;
	v24 = field_0xf20 / 2;
	v27 = field_0xf20 / -2;
LABEL_20:
	if (v25 < field_0xf20 - canvas->_field_0xf28)
	{
		v17 = x;
		v18 = -v23;
		for (i = 0; ; ++i)
		{
			if (i >= canvas->_field_0xf24)
			{
				++v25;
				++v26;
				++v27;
				field_0xf20 = canvas->_field_0xf20;
				goto LABEL_20;
			}
			v8 = canvas->app->tinyGL->pixels[i + canvas->_field_0xf24 * v25];
			v9 = v8 >> 10;
			if ((v8 & 0x3FF) != 0)
				break;
		LABEL_17:
			++v17;
			++v18;
		}
		v10 = (int)(abs(v18) << 10) / v23;
		v11 = (int)(abs(v27) << 10) / v24;
		v12 = v10 + v11 + ((unsigned int)(v10 + v11) >> 31);
		v13 = (v10 + v11) / 2;
		graphics->setColor(65793 * ((v13 << 7 >> 10) + 128) - 0x1000000);
		if (v9 == 1)
		{
			v15 = 8 * v13;
		}
		else
		{
			if (!v9 || v9 != 2)
			{
				v14 = v12 >> 11;
			LABEL_9:
				if (v14 <= 0 || v14 == 1)
				{
					graphics->fillRect(v17, v26, 1, 1);
				}
				else
				{
					graphics->fillCircle(v17, v26, v14);
				}
				goto LABEL_17;
			}
			v15 = 10 * v13;
		}
		v14 = v15 >> 10;
		goto LABEL_9;
	}
}


static void runStarFieldFrame(Canvas* canvas) {


	int v1; // r11
	int v2; // r3
	TinyGL* tinyGL; // r5
	unsigned int v4; // r3
	unsigned int v5; // r3
	int v6; // r6
	signed int v7; // r0
	int* v8; // r10
	int v9; // r0
	int v10; // r4
	int v11; // r8
	unsigned int v12; // r3
	TinyGL* v13; // r5
	unsigned int v14; // r3
	unsigned int v15; // r3
	int v16; // r6
	signed int v17; // r0
	int* v18; // r10
	int v19; // r0
	int v20; // r4
	int v21; // r8
	int v22; // r11
	unsigned int v23; // r3
	int v24; // r6
	int v25; // r10
	int v26; // r4
	int v27; // r5
	int v28; // r0
	int result; // r0
	int v30; // [sp+0h] [bp-78h]
	int v31; // [sp+4h] [bp-74h]
	int v32; // [sp+8h] [bp-70h]
	int v33; // [sp+Ch] [bp-6Ch]
	int v34; // [sp+10h] [bp-68h]
	int v35; // [sp+14h] [bp-64h]
	int v38; // [sp+20h] [bp-58h]
	int v39; // [sp+24h] [bp-54h]
	int v40; // [sp+28h] [bp-50h]
	int v41; // [sp+30h] [bp-48h]
	int i; // [sp+34h] [bp-44h]
	int v43; // [sp+3Ch] [bp-3Ch]
	unsigned short* v44; // [sp+44h] [bp-34h]
	unsigned short* pixels; // [sp+48h] [bp-30h]
	int v46; // [sp+4Ch] [bp-2Ch]
	int v47; // [sp+54h] [bp-24h]
	int v48; // [sp+58h] [bp-20h]
	int v49; // [sp+5Ch] [bp-1Ch]

	v1 = canvas->_field_0xf24;
	v38 = v1 / 2;
	v2 = canvas->_field_0xf20;
	v39 = v2 / 2;
	for (i = 0; i < v2 / 2; ++i)
	{
		v10 = 0;
		v48 = v39 - i;
		v12 = abs(v39 - i);
		v35 = 60 * v12;
		v34 = 20 * v12;
		v11 = -v38;
		while (v10 < v1)
		{
			tinyGL = canvas->app->tinyGL.get();
			pixels = tinyGL->pixels;
			v4 = pixels[v10 + v1 * i];
			v47 = v4 & 0x3FF;
			if ((v4 & 0x3FF) != 0)
			{
				v5 = v4 >> 10;
				if (v5 == 1)
				{
					v6 = v34;
					v7 = 20 * abs(v11);
				}
				else if (v5)
				{
					v6 = v35;
					v7 = 60 * abs(v11);
				}
				else
				{
					v6 = 300;
					v7 = 300;
				}
				v31 = canvas->_field_0xf2c;
				v8 = canvas->app->render->sinTable;
				v46 = (v8[(v47 + 256) & 0x3FF] * (v7 / v31) / v38) >> 16;
				v9 = (v8[v47] * (v6 / v31) / v39) >> 16;
				if (v48 + v9 >= -v39 && v39 > v48 + v9 && -v38 <= v11 + v46 && v38 > v11 + v46)
				{
					pixels[v10 + v1 * (i - v9) + v46] = v47 | ((short)v5 << 10);
					v1 = canvas->_field_0xf24;
					tinyGL = canvas->app->tinyGL.get();
				}
				tinyGL->pixels[v10 + v1 * i] = 0;
				v1 = canvas->_field_0xf24;
			}
			++v10;
			++v11;
		}
		v2 = canvas->_field_0xf20;
	}
	v49 = v2 - 1;
	v43 = v39 - (v2 - 1);
	while (v49 >= v2 / 2)
	{
		v20 = 0;
		v23 = abs(v43);
		v21 = -v38;
		v33 = 60 * v23;
		v32 = 20 * v23;
		while (v20 < v1)
		{
			v13 = canvas->app->tinyGL.get();
			v44 = v13->pixels;
			v14 = v44[v20 + v1 * v49];
			v40 = v14 & 0x3FF;
			if ((v14 & 0x3FF) != 0)
			{
				v15 = v14 >> 10;
				if (v15 == 1)
				{
					v16 = v32;
					v17 = 20 * abs(v21);
				}
				else if (v15)
				{
					v16 = v33;
					v17 = 60 * abs(v21);
				}
				else
				{
					v16 = 300;
					v17 = 300;
				}
				v30 = canvas->_field_0xf2c;
				v18 = canvas->app->render->sinTable;
				v41 = (v18[(v40 + 256) & 0x3FF] * (v17 / v30) / v38) >> 16;
				v19 = (v18[v40] * (v16 / v30) / v39) >> 16;
				if (v43 + v19 >= -v39 && v39 > v43 + v19 && v21 + v41 >= -v38 && v38 > v21 + v41)
				{
					v44[v20 + v1 * (v49 - v19) + v41] = v40 | ((short)v15 << 10);
					v1 = canvas->_field_0xf24;
					v13 = canvas->app->tinyGL.get();
				}
				v13->pixels[v20 + v1 * v49] = 0;
				v1 = canvas->_field_0xf24;
			}
			++v20;
			++v21;
		}
		--v49;
		++v43;
		v2 = canvas->_field_0xf20;
	}
	v22 = 0;
	while (1)
	{
		result = 4 / canvas->_field_0xf2c;
		if (v22 >= result)
			break;
		++v22;
		v24 = canvas->app->nextInt() % 1023;
		v25 = v24 + 1;
		v26 = canvas->app->nextInt() % 3;
		v27 = canvas->app->nextInt() % 15 + 1;
		if ((unsigned int)(v24 - 256) <= 0x1FE)
			v27 = -v27;
		v28 = canvas->app->nextInt() % 15 + 1;
		if (v25 >= 512)
			v28 = -v28;
		canvas->app->tinyGL->pixels[v27 + v38 + canvas->_field_0xf24 * (v39 - v28)] = v25 | ((short)v26 << 10);
	}
}



void TravelMapState::onEnter(Canvas* canvas) {
	if (CAppContainer::getInstance()->skipTravelMap) {
		canvas->setLoadingBarText((short)0, (short)41);
		canvas->setState(Canvas::ST_LOADING);
		return;
	}
	initTravelMap(canvas);
}

void TravelMapState::onExit(Canvas* canvas) {
}

void TravelMapState::update(Canvas* canvas) {
}

void TravelMapState::render(Canvas* canvas, Graphics* graphics) {
	drawTravelMap(canvas, graphics);
}

bool TravelMapState::handleInput(Canvas* canvas, int key, int action) {
	handleTravelMapInput(canvas, key, action);
	return true;
}
