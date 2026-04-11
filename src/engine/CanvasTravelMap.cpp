#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include "Log.h"

#include "SDLGL.h"
#include "App.h"
#include "Image.h"
#include "CAppContainer.h"
#include "Canvas.h"
#include "Graphics.h"
#include "MayaCamera.h"
#include "Game.h"
#include "GLES.h"
#include "TinyGL.h"
#include "Hud.h"
#include "Render.h"
#include "Combat.h"
#include "Player.h"
#include "MenuSystem.h"
#include "CAppContainer.h"
#include "IMinigame.h"
#include "HackingGame.h"
#include "SentryBotGame.h"
#include "VendingMachine.h"
#include "ParticleSystem.h"
#include "Text.h"
#include "Button.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Resource.h"
#include "Enums.h"
#include "SpriteDefs.h"
#include "Utils.h"
#include "Menus.h"
#include "Input.h"
#include "ICanvasState.h"

void Canvas::drawTravelMap(Graphics* graphics) {


	if (this->stateVars[5] == 1) {
		this->drawStarFieldPage(graphics);
		this->staleView = true;
		return;
	}

	graphics->drawImage(this->imgTravelBG, this->SCR_CX, (this->displayRect[3] - this->imgTravelBG->height) / 2, 17, 0, 0);

	if (this->xDiff > 1 || this->yDiff > 1) {
		graphics->fillRegion(this->imgFabricBG, 0, 0, this->displayRect[2], this->yDiff);
		graphics->fillRegion(this->imgFabricBG, 0, this->yDiff, this->xDiff, this->displayRect[3] - this->yDiff);
		graphics->fillRegion(this->imgFabricBG, this->xDiff, this->yDiff + this->mapHeight, this->mapWidth, this->yDiff);
		graphics->fillRegion(this->imgFabricBG, this->xDiff + this->mapWidth, this->yDiff, this->xDiff, this->displayRect[3] - this->yDiff);
		graphics->drawRect(this->xDiff + -1, this->yDiff + -1, this->mapWidth + 1, this->mapHeight + 1, 0xFF000000);
		graphics->clipRect(this->xDiff, this->yDiff, this->imgTravelBG->width, this->imgTravelBG->height);
	}

	int time = app->upTimeMs - this->stateVars[0];
	this->drawGridLines(graphics, time + this->totalTMTimeInPastAnimations);

	bool levelSamePlanet = this->newLevelSamePlanet();
	if (this->stateVars[1] != 1) {
		if (time > (levelSamePlanet ? 1500 : 700)) {
			this->totalTMTimeInPastAnimations += time;
			this->stateVars[0] += time;
			this->stateVars[1] = 1;
			time = 0;
			if (levelSamePlanet) {
				this->stateVars[2] = 1;
				this->stateVars[3] = 1;
			}
		}
		else if (levelSamePlanet) {
			this->drawAppropriateCloseup(graphics, this->TM_LastLevelId, false);
			Text* smallBuffer = app->localization->getSmallBuffer();
			smallBuffer->setLength(0);
			app->localization->composeText((short)3, app->game->levelNames[this->TM_LastLevelId - 1], smallBuffer);
			smallBuffer->dehyphenate();
			this->drawLocatorBoxAndName(graphics, (time & 0x200) == 0x0, this->TM_LastLevelId, smallBuffer);
			smallBuffer->dispose();
		}
	}

	if (this->stateVars[1] == 1) {
		if (this->stateVars[2] != 1) {
			if (this->drawDottedLine(graphics, time)) {
				this->totalTMTimeInPastAnimations += time;
				this->stateVars[0] += time;
				this->stateVars[2] = 1;
			}
		}
		else if (this->stateVars[2] == 1) {
			if (this->stateVars[3] != 1) {
				this->drawDottedLine(graphics);
				if (time > 500 || levelSamePlanet) {
					this->totalTMTimeInPastAnimations += time;
					this->stateVars[0] += time;
					this->stateVars[3] = 1;
				}
			}
			else if (this->stateVars[3] == 1) {
				this->drawAppropriateCloseup(graphics, this->TM_LoadLevelId, this->stateVars[8] == 1);
				if (this->stateVars[6] == 1) {
					Text* smallBuffer2 = app->localization->getSmallBuffer();
					smallBuffer2->setLength(0);
					app->localization->composeText((short)3, app->game->levelNames[this->TM_LoadLevelId - 1], smallBuffer2);
					smallBuffer2->dehyphenate();

					drawLocatorBoxAndName(graphics, (time & 0x200) == 0x0, this->TM_LoadLevelId, smallBuffer2);
					smallBuffer2->setLength(0);
					app->localization->composeText((short)0, (short)96, smallBuffer2);
					smallBuffer2->wrapText(24);
					smallBuffer2->dehyphenate();
					graphics->drawString(smallBuffer2, this->SCR_CX, this->displayRect[3] - 21, 17);
					smallBuffer2->dispose();
				}
				else if (this->stateVars[4] != 1) {
					if (this->drawLocatorLines(graphics, time, levelSamePlanet, this->stateVars[8] == 1)) {
						this->totalTMTimeInPastAnimations += time;
						this->stateVars[0] += time;
						this->stateVars[4] = 1;
					}
				}
				else {
					this->drawLocatorLines(graphics, -1, levelSamePlanet, this->stateVars[8] == 1);
					if (time > 800) {
						this->totalTMTimeInPastAnimations += time;
						this->stateVars[0] += time;
						if (this->stateVars[8] == 0) {
							this->stateVars[6] = 1;
						}
						else {
							this->stateVars[4] = 0;
							this->stateVars[8] = 0;
							int n7 = 123;
							int n8 = 150;
							short n9 = (short)(2 * (this->TM_LoadLevelId - 1));
							this->targetX = Canvas::CROSS_HAIR_CORDS[n9] + this->xDiff + n7;
							this->targetY = Canvas::CROSS_HAIR_CORDS[n9 + 1] + this->yDiff + n8;
						}
					}
				}
			}
		}
	}

	this->staleView = true;
}


bool Canvas::newLevelSamePlanet() {
	return this->TM_LastLevelId != this->TM_LoadLevelId && ((onMoon(this->TM_LastLevelId) && onMoon(this->TM_LoadLevelId)) || (onEarth(this->TM_LastLevelId) && onEarth(this->TM_LoadLevelId)) || (inHell(this->TM_LastLevelId) && inHell(this->TM_LoadLevelId)));
}


void Canvas::drawAppropriateCloseup(Graphics* graphics, int n, bool b) {
	if (this->onMoon(n)) {
		graphics->drawImage(this->imgTierCloseUp, Canvas::moonCoords[0], Canvas::moonCoords[1], 0, 0, 0);
	}
	else if (this->onEarth(n)) {
		if (b) {
			graphics->drawImage(this->imgEarthCloseUp, Canvas::earthCoords[0], Canvas::earthCoords[1], 0, 0, 0);
		}
		else {
			graphics->drawImage(this->imgTierCloseUp, Canvas::earthCoords[2], Canvas::earthCoords[3], 0, 0, 0);
		}
	}
	else {
		graphics->drawImage(this->imgTierCloseUp, Canvas::hellCoords[0], Canvas::hellCoords[1], 0, 0, 0);
	}
}


bool Canvas::drawDottedLine(Graphics* graphics, int n) {
	int n2 = n / 22;
	bool b;
	if (this->TM_LastLevelId == 0 && onMoon(this->TM_LoadLevelId)) {
		b = this->drawMarsToMoonLinePlusSpaceShip(graphics, n2);
	}
	else if (this->onMoon(this->TM_LastLevelId) && this->onEarth(this->TM_LoadLevelId)) {
		b = this->drawMoonToEarthLine(graphics, n2, false);
	}
	else if (this->onEarth(this->TM_LastLevelId) && this->onMoon(this->TM_LoadLevelId)) {
		b = this->drawMoonToEarthLine(graphics, n2, true);
	}
	else if (this->onEarth(this->TM_LastLevelId) && inHell(this->TM_LoadLevelId)) {
		b = this->drawEarthToHellLine(graphics, n2, false);
	}
	else {
		b = (!inHell(this->TM_LastLevelId) || !onEarth(this->TM_LoadLevelId) || this->drawEarthToHellLine(graphics, n2, true));
	}
	return b;
}


bool Canvas::drawDottedLine(Graphics* graphics) {
	return drawDottedLine(graphics, 22 * std::max(this->screenRect[2], this->screenRect[3]));
}


bool Canvas::drawMarsToMoonLinePlusSpaceShip(Graphics* graphics, int n) {


	bool b = false;
	int width = this->imgTravelPath->width;
	int n2 = n;
	if (n2 > width) {
		b = true;
		n2 = width;
	}
	if (n > width / 3) {
		Text* smallBuffer = app->localization->getSmallBuffer();
		smallBuffer->setLength(0);
		app->localization->composeText((short)3, (short)165, smallBuffer);
		smallBuffer->dehyphenate();
		graphics->drawString(smallBuffer, Canvas::moonNameCoords[0], Canvas::moonNameCoords[1], 4);
		smallBuffer->dispose();
	}
	graphics->drawRegion(this->imgTravelPath, width - n2, 0, n2, this->imgTravelPath->height, Canvas::moonPathCoords[0] + width - n2, Canvas::moonPathCoords[1], 0, 0, 0);
	int n3 = width - n2;
	graphics->drawImage(this->imgSpaceShip, Canvas::moonPathCoords[0] + n3 - this->imgSpaceShip->width, this->yCoordOfSpaceShip(n3) + Canvas::moonPathCoords[1] - (this->imgSpaceShip->height >> 1), 0, 0, 0);
	return b;
}


int Canvas::yCoordOfSpaceShip(int n) {
	int iVar1;
	iVar1 = (n * 11) / 15;
	return ((-851 * (iVar1 * iVar1) / 110880 + 6115 * iVar1 / 22176 + 40) * 15) / 11;
}


bool Canvas::drawMoonToEarthLine(Graphics* graphics, int n, bool b) {


	bool b2 = false;
	int height = this->imgTravelPath->height;
	int n2 = n;
	if (n2 > height) {
		b2 = true;
		n2 = height;
	}
	int n3 = b ? 0 : (height - n2);
	int width = this->imgTravelPath->width;
	if (n > height / 2) {
		Text* smallBuffer = app->localization->getSmallBuffer();
		smallBuffer->setLength(0);
		if (!b) {
			app->localization->composeText((short)3, (short)164, smallBuffer);
			smallBuffer->dehyphenate();
			graphics->drawString(smallBuffer, Canvas::earthNameCoords[0], Canvas::earthNameCoords[1], 4);
		}
		else {
			app->localization->composeText((short)3, (short)165, smallBuffer);
			smallBuffer->dehyphenate();
			graphics->drawString(smallBuffer, Canvas::moonNameCoords[2], Canvas::moonNameCoords[3], 4);
		}
		smallBuffer->dispose();
	}
	graphics->drawRegion(this->imgTravelPath, 0, n3, width, n2, Canvas::earthPathCoords[0], Canvas::earthPathCoords[1] + n3, 0, 0, 0);
	return b2;
}


bool Canvas::drawEarthToHellLine(Graphics* graphics, int n, bool b) {


	bool b2 = false;
	int height = this->imgTravelPath->height;
	int n2 = n;
	if (n2 > height) {
		b2 = true;
		n2 = height;
	}
	int n3 = b ? 0 : (height - n2);
	int width = this->imgTravelPath->width;
	if (n > 2 * height / 3) {
		Text* smallBuffer = app->localization->getSmallBuffer();
		smallBuffer->setLength(0);
		if (!b) {
			app->localization->composeText((short)3, (short)166, smallBuffer);
			smallBuffer->dehyphenate();
			graphics->drawString(smallBuffer, Canvas::hellNameCoords[0], Canvas::hellNameCoords[1], 4);
		}
		else {
			app->localization->composeText((short)3, (short)164, smallBuffer);
			smallBuffer->dehyphenate();
			graphics->drawString(smallBuffer, Canvas::earthNameCoords[2], Canvas::earthNameCoords[3], 4);
		}
		smallBuffer->dispose();
	}
	graphics->drawRegion(this->imgTravelPath, 0, n3, width, n2, Canvas::hellPathCoords[0], Canvas::hellPathCoords[1] + n3, 0, 0, 0);
	return b2;
}


void Canvas::drawLocatorBoxAndName(Graphics* graphics, bool b, int n, Text* text) {
	int n2;
	int n3;
	if (this->onMoon(n)) {
		n2 = Canvas::moonCoords[0];
		n3 = Canvas::moonCoords[1];
	}
	else if (this->onEarth(n)) {
		n2 = Canvas::earthCoords[0];
		n3 = Canvas::earthCoords[1];
	}
	else {
		n2 = Canvas::hellCoords[0];
		n3 = Canvas::hellCoords[1];
	}
	short n4 = (short)(2 * (n - 1));
	int n5 = Canvas::CROSS_HAIR_CORDS[n4] + this->xDiff + n2;
	int n6 = Canvas::CROSS_HAIR_CORDS[n4 + 1] + this->yDiff + n3;
	if (b) {
		graphics->drawImage(this->imgMagGlass, n5, n6, 3, 0, 0);
	}

	int x = Canvas::LOCATOR_BOX_CORDS[n4] + this->xDiff + n2;
	int y = Canvas::LOCATOR_BOX_CORDS[n4 + 1] + this->yDiff + n3 + (this->imgNameHighlight->height >> 1);
	graphics->drawImage(this->imgNameHighlight, x, y, 6, 0, 0);
	this->graphics.currentCharColor = 3;
	graphics->drawString(text, x + (this->imgNameHighlight->width >> 1), y, 3);
}


void Canvas::drawGridLines(Graphics* graphics, int i) {
	int iVar1;

	for (iVar1 = (i / 200) % 44 + this->displayRect[0]; iVar1 < this->displayRect[2]; iVar1 += 44) {
		graphics->drawImage(this->imgMapVertGridLines, iVar1, this->displayRect[1], 0x14, 0, 2);
	}
	for (iVar1 = this->displayRect[1] + 5; iVar1 < this->displayRect[3]; iVar1 += 44) {
		graphics->drawImage(this->imgMapHorzGridLines, this->displayRect[0], iVar1, 20, 0, 2);
		graphics->drawImage(this->imgMapHorzGridLines, 240, iVar1, 20, 0, 2);
	}
}


bool Canvas::onMoon(int n) {
	return n >= 1 && n <= 3;
}


bool Canvas::onEarth(int n) {
	return n >= 4 && n <= 6;
}


bool Canvas::inHell(int n) {
	return n >= 7;
}


void Canvas::handleTravelMapInput(int key, int action) {
#if 0 // IOS
	if ((this->stateVars[6] == 1) || (key == 18) || (action == Enums::ACTION_FIRE)) {
		this->finishTravelMapAndLoadLevel();
	}
	else {
		this->stateVars[6] = 1;
	}
#else // J2ME/BREW

	bool hasSavedState = app->game->hasSavedState();

	if (action == Enums::ACTION_MENU) { // [GEC] skip all
		this->finishTravelMapAndLoadLevel();
		return;
	}

	if (action == Enums::ACTION_FIRE) {
		if (this->stateVars[6] == 1) {
#if 0 // [GEC] no esta disponible en ningina de las versiones del juego
			if (this->TM_LastLevelId == 0 && onMoon(this->TM_LoadLevelId)) {
				this->stateVars[5] = 1; // Draw start field
			}
			else {
				this->finishTravelMapAndLoadLevel();
			}
#else
			this->finishTravelMapAndLoadLevel();
#endif
		}
		else if (hasSavedState) {
			if (this->stateVars[2] == 1) {
				if (this->stateVars[8] == 0) {
					this->stateVars[3] = 1;
					this->stateVars[4] = 1;
					this->stateVars[6] = 1;
				}
				else {
					int n3 = app->upTimeMs - this->stateVars[0];
					this->totalTMTimeInPastAnimations += n3;
					this->stateVars[0] += n3;
					if (this->stateVars[4] == 1) {
						this->stateVars[4] = 0;
						this->stateVars[8] = 0;
						int n5 = this->earthCoords[0];
						int n6 = this->earthCoords[1];
						short n7 = (short)(2 * (this->TM_LoadLevelId - 1));
						this->targetX = this->CROSS_HAIR_CORDS[n7] + this->xDiff + n5;
						this->targetY = this->CROSS_HAIR_CORDS[n7 + 1] + this->yDiff + n6;
					}
					else {
						this->stateVars[3] = 1;
						this->stateVars[4] = 1;
					}
				}
			}
			else {
				this->totalTMTimeInPastAnimations += app->upTimeMs - this->stateVars[0];
				this->stateVars[0] = app->upTimeMs;
				this->stateVars[1] = 1;
				this->stateVars[2] = 1;
				if (this->newLevelSamePlanet()) {
					this->stateVars[3] = 1;
				}
			}
		}
	}
	else if (action == Enums::ACTION_AUTOMAP && this->stateVars[5] == 1) {
		this->finishTravelMapAndLoadLevel();
	}
#endif
}

void Canvas::finishTravelMapAndLoadLevel() {
	this->clearSoftKeys();
	this->disposeTravelMap();
	this->setLoadingBarText((short)0, (short)41);
	this->setState(Canvas::ST_LOADING);
}


bool Canvas::drawLocatorLines(Graphics* graphics, int n, bool b, bool b2) {
	int targetX;
	int targetY;
	int targetX2;
	int targetY2;
	if (n >= 0) {
		int n2 = n / (b ? 15 : 7);
		if (b) {
			int n3;
			int n4;
			if (this->onMoon(this->TM_LastLevelId)) {
				n3 = Canvas::moonCoords[0];
				n4 = Canvas::moonCoords[1];
			}
			else if (this->onEarth(this->TM_LastLevelId)) {
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
			short n5 = (short)(2 * (this->TM_LastLevelId - 1));
			targetX = Canvas::CROSS_HAIR_CORDS[n5] + this->xDiff + n3;
			targetY = Canvas::CROSS_HAIR_CORDS[n5 + 1] + this->yDiff + n4;
		}
		else {
			targetX = this->displayRect[2] - this->xDiff;
			targetY = this->displayRect[3] - this->yDiff;
		}
		int n6 = (this->targetX > targetX) ? 1 : ((this->targetX == targetX) ? 0 : -1);
		int n7 = (this->targetY > targetY) ? 1 : ((this->targetY == targetY) ? 0 : -1);
		targetX2 = targetX + n6 * n2;
		targetY2 = targetY + n7 * n2;
	}
	else {
		targetX2 = (targetX = this->targetX);
		targetY2 = (targetY = this->targetY);
	}
	bool b3 = false;
	bool b4 = false;
	if ((this->targetX - targetX) * (this->targetX - targetX2) < 0 || this->targetX == targetX2) {
		targetX2 = this->targetX;
		b3 = true;
	}
	if ((this->targetY - targetY) * (this->targetY - targetY2) < 0 || this->targetY == targetY2) {
		targetY2 = this->targetY;
		b4 = true;
	}
	graphics->drawLine(targetX2 + 1, this->displayRect[1], targetX2 + 1, this->displayRect[3], 0xFF000000);
	graphics->drawLine(this->displayRect[0], targetY2 + 1, this->displayRect[2], targetY2 + 1, 0xFF000000);
	graphics->drawLine(targetX2, this->displayRect[1], targetX2, this->displayRect[3], 0xFFBDFD80);
	graphics->drawLine(this->displayRect[0], targetY2, this->displayRect[2], targetY2, 0xFFBDFD80);
	return b3 && b4;
}


void Canvas::initTravelMap() {


	this->TM_LoadLevelId = (short)std::max(1, std::min(this->loadMapID, 9));
	this->TM_LastLevelId = ((getRecentLoadType() == 1 && !this->TM_NewGame) ? this->TM_LoadLevelId : ((short)std::max(0, std::min(this->lastMapID, 9))));

	if (this->TM_LastLevelId == 0 && this->TM_LoadLevelId > 1) {
		this->TM_LastLevelId = (short)(this->TM_LoadLevelId - 1);
	}
	short n;
	if (this->TM_LoadLevelId > this->TM_LastLevelId) {
		n = 0;
	}
	else if (this->TM_LoadLevelId == this->TM_LastLevelId) {
		n = 1;
	}
	else {
		n = 2;
	}
	app->game->scriptStateVars[15] = n;

	app->beginImageLoading();
	this->imgNameHighlight = app->loadImage("highlight.bmp", true);
	this->imgMagGlass = app->loadImage("magnifyingGlass.bmp", true);
	this->imgSpaceShip = app->loadImage("spaceShip.bmp", true);

	bool b = false;
	if (onMoon(this->TM_LoadLevelId)) {
		this->imgTierCloseUp = app->loadImage("TM_Levels1.bmp", true);
	}
	else if (onEarth(this->TM_LoadLevelId)) {
		if (onMoon(this->TM_LastLevelId) || inHell(this->TM_LastLevelId)) {
			this->imgEarthCloseUp = app->loadImage("TM_Levels2.bmp", true);
			b = true;
		}
		this->imgTierCloseUp = app->loadImage("TM_Levels4.bmp", true);
	}
	else {
		this->imgTierCloseUp = app->loadImage("TM_Levels3.bmp", true);
	}

	if (onMoon(this->TM_LoadLevelId) && this->TM_LastLevelId == 0) {
		this->imgTravelPath = app->loadImage("toMoon.bmp", true);
	}
	else if ((onEarth(this->TM_LoadLevelId) && onMoon(this->TM_LastLevelId)) || (onMoon(this->TM_LoadLevelId) && onEarth(this->TM_LastLevelId))) {
		this->imgTravelPath = app->loadImage("toEarth.bmp", true);
	}
	else if ((inHell(this->TM_LoadLevelId) && onEarth(this->TM_LastLevelId)) || (onEarth(this->TM_LoadLevelId) && inHell(this->TM_LastLevelId))) {
		this->imgTravelPath = app->loadImage("toHell.bmp", true);
	}

	this->imgTravelBG = app->loadImage("TravelMap.bmp", true);
	this->imgMapHorzGridLines = app->loadImage("travelMapHorzGrid.bmp", true);
	this->imgMapVertGridLines = app->loadImage("travelMapVertGrid.bmp", true);
	app->endImageLoading();

	this->totalTMTimeInPastAnimations = 0;
	this->mapWidth = this->imgTravelBG->width;
	this->mapHeight = this->imgTravelBG->height;
	this->xDiff = std::max(0, (this->displayRect[2] - this->imgTravelBG->width) / 2);
	this->yDiff = std::max(0, (this->displayRect[3] - this->imgTravelBG->height) / 2);

	int n2;
	int n3;
	if (onMoon(this->TM_LoadLevelId)) {
		n2 = 137;
		n3 = 216;
	}
	else if (onEarth(this->TM_LoadLevelId)) {
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
		short n4 = (short)(2 * (this->TM_LoadLevelId - 1));
		this->targetX = Canvas::CROSS_HAIR_CORDS[n4] + this->xDiff + n2;
		this->targetY = Canvas::CROSS_HAIR_CORDS[n4 + 1] + this->yDiff + n3;
	}
	else {
		this->targetX = Canvas::UAC_BUILDING_LOCATION_ON_EARTH[0] + this->xDiff + n2;
		this->targetY = Canvas::UAC_BUILDING_LOCATION_ON_EARTH[1] + this->yDiff + n3;
	}

	this->stateVars[0] = app->upTimeMs;;
	if (b) {
		this->stateVars[8] = 1;
	}

	// Unused
	this->imgStarField = app->loadImage("cockpit.bmp", true);
	this->_field_0xf24 = 480;
	this->_field_0xf20 = this->imgStarField->height;
	this->_field_0xf28 = 0;
	this->_field_0xf2c = 1;
}


void Canvas::disposeTravelMap() {
	this->imgNameHighlight->~Image();
	this->imgNameHighlight = nullptr;
	this->imgMagGlass->~Image();
	this->imgMagGlass = nullptr;
	this->imgTravelBG->~Image();
	this->imgTravelBG = nullptr;
	this->imgTravelPath->~Image();
	this->imgTravelPath = nullptr;
	this->imgSpaceShip->~Image();
	this->imgSpaceShip = nullptr;
	this->imgTierCloseUp->~Image();
	this->imgTierCloseUp = nullptr;
	this->imgEarthCloseUp->~Image();
	this->imgEarthCloseUp = nullptr;
	this->imgMapHorzGridLines->~Image();
	this->imgMapHorzGridLines = nullptr;
	this->imgMapVertGridLines->~Image();
	this->imgMapVertGridLines = nullptr;

	// Unused
	this->imgStarField->~Image();
	this->imgStarField = nullptr;
}


void Canvas::drawStarFieldPage(Graphics* graphics) {


	if (app->upTimeMs - this->stateVars[0] > 5250) {
		this->_field_0xf2c = 2u;
		this->finishTravelMapAndLoadLevel();
	}
	else {
		int yPos = this->screenRect[3] - this->screenRect[1] - this->imgStarField->height;
		graphics->fillRect(0, 0, this->menuRect[2], this->menuRect[3], 0xFF000000);
		this->drawStarField(graphics, 1, yPos / 2);
		graphics->drawImage(this->imgStarField, 1, yPos / 2, 0, 0, 0);
		graphics->drawImage(this->imgStarField, this->screenRect[2] - 1, yPos / 2, 24, 4, 0);
		app->canvas->softKeyRightID = -1;
		app->canvas->softKeyLeftID = -1;
		app->canvas->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
		if (app->canvas->displaySoftKeys) {
			app->canvas->softKeyRightID = 40;
			app->canvas->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
		}
	}
}


void Canvas::drawStarField(Graphics* graphics, int x, int y) {

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

	graphics->fillRect(x, y, this->_field_0xf24, this->_field_0xf20, 0xFF000000);
	if (app->upTimeMs - this->stateVars[7] > 59) {
		this->stateVars[7] = app->upTimeMs;
		this->runStarFieldFrame();
	}

	v23 = this->_field_0xf24 / 2;
	field_0xf20 = this->_field_0xf20;
	v26 = y;
	v25 = 0;
	v24 = field_0xf20 / 2;
	v27 = field_0xf20 / -2;
LABEL_20:
	if (v25 < field_0xf20 - this->_field_0xf28)
	{
		v17 = x;
		v18 = -v23;
		for (i = 0; ; ++i)
		{
			if (i >= this->_field_0xf24)
			{
				++v25;
				++v26;
				++v27;
				field_0xf20 = this->_field_0xf20;
				goto LABEL_20;
			}
			v8 = app->tinyGL->pixels[i + this->_field_0xf24 * v25];
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


void Canvas::runStarFieldFrame() {


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

	v1 = this->_field_0xf24;
	v38 = v1 / 2;
	v2 = this->_field_0xf20;
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
			tinyGL = app->tinyGL.get();
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
				v31 = this->_field_0xf2c;
				v8 = app->render->sinTable;
				v46 = (v8[(v47 + 256) & 0x3FF] * (v7 / v31) / v38) >> 16;
				v9 = (v8[v47] * (v6 / v31) / v39) >> 16;
				if (v48 + v9 >= -v39 && v39 > v48 + v9 && -v38 <= v11 + v46 && v38 > v11 + v46)
				{
					pixels[v10 + v1 * (i - v9) + v46] = v47 | ((short)v5 << 10);
					v1 = this->_field_0xf24;
					tinyGL = app->tinyGL.get();
				}
				tinyGL->pixels[v10 + v1 * i] = 0;
				v1 = this->_field_0xf24;
			}
			++v10;
			++v11;
		}
		v2 = this->_field_0xf20;
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
			v13 = app->tinyGL.get();
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
				v30 = this->_field_0xf2c;
				v18 = app->render->sinTable;
				v41 = (v18[(v40 + 256) & 0x3FF] * (v17 / v30) / v38) >> 16;
				v19 = (v18[v40] * (v16 / v30) / v39) >> 16;
				if (v43 + v19 >= -v39 && v39 > v43 + v19 && v21 + v41 >= -v38 && v38 > v21 + v41)
				{
					v44[v20 + v1 * (v49 - v19) + v41] = v40 | ((short)v15 << 10);
					v1 = this->_field_0xf24;
					v13 = app->tinyGL.get();
				}
				v13->pixels[v20 + v1 * v49] = 0;
				v1 = this->_field_0xf24;
			}
			++v20;
			++v21;
		}
		--v49;
		++v43;
		v2 = this->_field_0xf20;
	}
	v22 = 0;
	while (1)
	{
		result = 4 / this->_field_0xf2c;
		if (v22 >= result)
			break;
		++v22;
		v24 = app->nextInt() % 1023;
		v25 = v24 + 1;
		v26 = app->nextInt() % 3;
		v27 = app->nextInt() % 15 + 1;
		if ((unsigned int)(v24 - 256) <= 0x1FE)
			v27 = -v27;
		v28 = app->nextInt() % 15 + 1;
		if (v25 >= 512)
			v28 = -v28;
		app->tinyGL->pixels[v27 + v38 + this->_field_0xf24 * (v39 - v28)] = v25 | ((short)v26 << 10);
	}
}


