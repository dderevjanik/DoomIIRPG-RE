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

void Canvas::loadMiniGameImages() {


	app->beginImageLoading();
	app->hackingGame->imgEnergyCore = app->loadImage("hackerBG.bmp", true);
	app->hackingGame->imgGameColors = app->loadImage("blockGameColors.bmp", true);
	app->hackingGame->imgHelpScreenAssets = app->loadImage("gameHelpBG.bmp", true);
	app->sentryBotGame->imgMatrixSkip_BG = app->loadImage("matrixSkip_BG.bmp", true);
	app->sentryBotGame->imgGameAssets = app->loadImage("sentryGame.bmp", true);
	app->sentryBotGame->imgButton = app->loadImage("matrixSkip_grid_pressed.bmp", true);
	app->sentryBotGame->imgButton2 = app->loadImage("matrixSkip_grid_pressed_2.bmp", true);
	app->sentryBotGame->imgButton3 = app->loadImage("matrixSkip_grid_pressed_3.bmp", true);
	app->vendingMachine->imgVendingGame = app->loadImage("vendingGame.bmp", true);
	app->vendingMachine->imgVending_BG = app->loadImage("vending_BG.bmp", true);
	app->vendingMachine->imgVendingBG = app->loadImage("vendingBG.bmp", true);
	app->vendingMachine->imgSubmitButton = app->loadImage("vending_submit.bmp", true);
	app->vendingMachine->imgVending_submit_pressed = app->loadImage("vending_submit_pressed.bmp", true);
	app->vendingMachine->imgVending_arrow_up = app->loadImage("vending_arrow_up.bmp", true);
	app->vendingMachine->imgVending_arrow_up_pressed = app->loadImage("vending_arrow_up_pressed.bmp", true);
	app->vendingMachine->imgVending_arrow_down = app->loadImage("vending_arrow_down.bmp", true);
	app->vendingMachine->imgVending_arrow_down_pressed = app->loadImage("vending_arrow_down_pressed.bmp", true);
	app->vendingMachine->imgVending_button_small = app->loadImage("vending_button_small.bmp", true);
	app->endImageLoading();

	app->vendingMachine->imgHelpScreenAssets = app->sentryBotGame->imgHelpScreenAssets = app->hackingGame->imgHelpScreenAssets;


	// [GEC] Fix the image
	fixImage(app->hackingGame->imgGameColors);
	fixImage(app->vendingMachine->imgVending_arrow_down);
}


// treadmillState, handleTreadmillEvents, treadmillFall, drawTreadmillReadout moved to TreadmillState.cpp


void Canvas::drawTargetPracticeScore(Graphics* graphics) {

	if (app->hud->msgCount != 0 && (app->hud->messageFlags[0] & 0x4) != 0x0) {
		return;
	}
	Text* smallBuffer = app->localization->getSmallBuffer();
	app->localization->resetTextArgs();
	app->localization->composeText((short)0, (short)229, smallBuffer);
	smallBuffer->append(app->player->targetPracticeScore);
	app->hud->drawImportantMessage(graphics, smallBuffer, 0xFF7F0000);
	smallBuffer->dispose();
}


void Canvas::initMiniGameHelpScreen() {
	this->miniGameHelpScrollPosition = 0;
}


void Canvas::drawMiniGameHelpScreen(Graphics* graphics, int i, int i2, Image* image) {
	graphics->drawImage(image, 0, 0, 0, 0, 0);
	this->drawMiniGameHelpText(graphics, i, i2);
}


void Canvas::drawMiniGameHelpText(Graphics* graphics, int i, int i2) {


	int x;
	Text* textBuff1;
	int iVar1;
	Text* textBuff2;
	int iVar2;
	Text* textBuff3;
	int iVar3;

	textBuff1 = app->localization->getSmallBuffer();
	x = (this->screenRect[2] - this->screenRect[0]) / 2;
	textBuff1->setLength(0);
	app->localization->composeText(i, textBuff1);
	textBuff1->dehyphenate();
	graphics->drawString(textBuff1, x, this->screenRect[1] + 0x19, 3);

	textBuff1->setLength(0);
	app->localization->composeText(i2, textBuff1);
	textBuff1->wrapText(0x31, '\n');
	iVar1 = textBuff1->getNumLines();
	this->helpTextNumberOfLines = iVar1;

	textBuff2 = app->localization->getSmallBuffer();
	textBuff2->setLength(0);
	for (iVar1 = 0; iVar1 < this->miniGameHelpScrollPosition; iVar1 = iVar1 + 1) {
		iVar3 = textBuff1->findFirstOf('\n');
		textBuff3 = textBuff1;
		if (iVar3 != -1) {
			iVar2 = textBuff1->length();
			textBuff1->substring(textBuff2, iVar3 + 1, (iVar2 - iVar3) + -1);
			textBuff1->setLength(0);
			textBuff3 = textBuff2;
			textBuff2 = textBuff1;
		}
		textBuff1 = textBuff3;
	}
	for (iVar1 = this->miniGameHelpScrollPosition + 0x10; iVar1 < this->helpTextNumberOfLines;
		iVar1 = iVar1 + 1) {
		iVar3 = textBuff1->findLastOf('\n');
		textBuff3 = textBuff1;
		if (iVar3 != -1) {
			textBuff1->substring(textBuff2, 0, iVar3);
			textBuff1->setLength(0);
			textBuff3 = textBuff2;
			textBuff2 = textBuff1;
		}
		textBuff1 = textBuff3;
	}
	textBuff2->dispose();

	graphics->drawString(textBuff1, this->screenRect[0] + 0x12, this->screenRect[1] + 0x2d, 0x14);
	iVar3 = this->helpTextNumberOfLines;
	iVar1 = this->miniGameHelpScrollPosition + 0x10;
	if (iVar3 < iVar1) {
		iVar1 = iVar3;
	}
	this->drawScrollBar(graphics, this->screenRect[2] + -0x12, this->screenRect[1] + 0x2d, 0x100,
		this->miniGameHelpScrollPosition, iVar1, iVar3, 0x10);
	textBuff1->setLength(0);
	app->localization->composeText(0, 0x60, textBuff1);
	textBuff1->dehyphenate();
	graphics->drawString(textBuff1, x, this->screenRect[3] + -0x14, 3);
	textBuff1->dispose();
}


void Canvas::handleMiniGameHelpScreenScroll(int i) {
	int iVar1;
	int iVar2;

	if (i < 0) {
		iVar1 = i + this->miniGameHelpScrollPosition;
		if (iVar1 < 0) {
			iVar1 = 0;
		}
		this->miniGameHelpScrollPosition = iVar1;
		return;
	}
	if (0 < i) {
		iVar1 = this->helpTextNumberOfLines + -0x10;
		if (iVar1 < 0) {
			iVar1 = 0;
		}
		iVar2 = i + this->miniGameHelpScrollPosition;
		if (iVar1 < iVar2) {
			this->miniGameHelpScrollPosition = iVar1;
		}
		else {
			this->miniGameHelpScrollPosition = iVar2;
		}
		return;
	}
	return;
}


bool Canvas::startArmorRepair(ScriptThread* armorRepairThread) {


	if (app->player->showHelp((short)15, false)) {
		return false;
	}
	if (app->player->inventory[12] >= 5) {
		this->armorRepairThread = armorRepairThread;
		this->repairingArmor = true;
		Text* smallBuffer = app->localization->getSmallBuffer();
		app->localization->composeText((short)0, (short)211, smallBuffer);
		this->startDialog(nullptr, smallBuffer, 13, 1, false);
		smallBuffer->dispose();
		return true;
	}

	if (app->player->characterChoice == 1) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::NO_USE2), 0, 3, 0);
	}
	else if (app->player->characterChoice >= 1 && app->player->characterChoice <= 3){
		app->sound->playSound(Sounds::getResIDByName(SoundName::NO_USE), 0, 3, 0);
	}

	app->hud->addMessage((short)0, (short)210, 3);
	return false;
}


void Canvas::endArmorRepair() {
	if (this->armorRepairThread != nullptr) {
		this->setState(Canvas::ST_PLAYING);
		this->armorRepairThread->run();
		this->armorRepairThread = nullptr;
		this->repairingArmor = false;
	}
}


void Canvas::evaluateMiniGameResults(int n) {

	if (n == 1) {
		int modifyStat = app->player->modifyStat(7, 2);
		app->localization->resetTextArgs();
		if (modifyStat > 0) {
			app->localization->addTextArg(modifyStat);
			app->hud->addMessage((short)236, 3);
		}
		else {
			app->hud->addMessage((short)237, 3);
		}
	}
}

