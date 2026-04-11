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

void Canvas::setupCharacterSelection() {


	app->beginImageLoading();
	this->imgCharacter_select_stat_bar = app->loadImage("Character_select_stat_bar.bmp", true);
	this->imgCharacter_select_stat_header = app->loadImage("Character_select_stat_header.bmp", true);
	this->imgTopBarFill = app->loadImage("Character_select_top_bar.bmp", true);
	this->imgCharacter_upperbar = app->loadImage("character_upperbar.bmp", true);
	this->imgCharacterSelectionAssets = app->loadImage("charSelect.bmp", true);
	this->imgCharSelectionBG = app->loadImage("charSelectionBG.bmp", true);
	this->imgMajorMugs = app->loadImage("Hud_Player.bmp", true);
	this->imgSargeMugs = app->loadImage("Hud_PlayerDoom.bmp", true);
	this->imgScientistMugs = app->loadImage("Hud_PlayerScientist.bmp", true);
	this->imgMajor_legs = app->loadImage("Major_legs.bmp", true);
	this->imgMajor_torso = app->loadImage("Major_torso.bmp", true);
	this->imgRiley_legs = app->loadImage("Riley_legs.bmp", true);
	this->imgRiley_torso = app->loadImage("Riley_torso.bmp", true);
	this->imgSarge_legs = app->loadImage("Sarge_legs.bmp", true);
	this->imgSarge_torso = app->loadImage("Sarge_torso.bmp", true);
	app->endImageLoading();
}


int Canvas::getCharacterConstantByOrder(int i) {
	switch (i) {
	case 0: {
		return 1;
	}
	case 1: {
		return 3;
	}
	case 2: {
		return 2;
	}
	}
	return 0;
}


void Canvas::drawCharacterSelection(Graphics* graphics) {

	fmButton* button;
	Text* textBuff;
	Image* img;
	int textID;


	graphics->clipRect(0, 0, this->screenRect[2], this->screenRect[3]);
	graphics->drawImage(this->imgCharSelectionBG, 0, 0, 0, 0, 0);
	graphics->drawImage(this->imgTopBarFill, this->SCR_CX - this->imgTopBarFill->width / 2, 0, 0, 0, 0);

	textBuff = app->localization->getSmallBuffer();
	textBuff->setLength(0);

	switch (this->stateVars[0]) {
		case 1: {
			app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::CHARACTER_SELECT_MAJOR_NAME, textBuff);
			break;
		}
		case 3: {
			app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::CHARACTER_SELECT_SCIENTIST_NAME, textBuff);
			break;
		}
		case 2: {
			app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::CHARACTER_SELECT_SERGEANT_NAME, textBuff);
			break;
		}
	}

	textBuff->dehyphenate();
	graphics->drawString(textBuff, this->SCR_CX, 3, 1);

	int j = 152;
	for (int i = 0; i < ((this->stateVars[1] == 0) ? 3 : 1); i++) {
		bool b = this->getCharacterConstantByOrder(i) == this->stateVars[0] || this->stateVars[1] != 0;

		//iVar2 = j + -0xf;
		graphics->drawRegion(this->imgCharacterSelectionAssets, 0, 0, 31, 83, j - 15, 55, 0, 0, 0);
		graphics->drawRegion(this->imgCharacterSelectionAssets, 0, 0, 31, 83, j + 16, 55, 0, 4, 0);

		button = this->m_characterButtons->GetButton(i);
		if (button->highlighted)
		{
			graphics->fillRect(j - 15, 55, 62, 83, 0x2896ff);
			graphics->drawRegion(this->imgCharacterSelectionAssets, 31, 0, 27, 60, j - 11, 64, 0, 0, 0);
			graphics->drawRegion(this->imgCharacterSelectionAssets, 31, 0, 27, 60, j + 16, 64, 0, 4, 0);
		}

		button->SetTouchArea(j - 15, 55, 62, 83);

		if (i < 2 && this->stateVars[1] == 0) {
			for (int x = (j + 46); x < (j + 64); x += 6) {
				graphics->drawRegion(this->imgCharacterSelectionAssets, 51, 60, 6, 19, x, 81, 0, 0, 0);
			}
		}

		if (b != 0) {
			graphics->drawRegion(this->imgCharacterSelectionAssets, 31, 60, 20, 32, j - 3, 161, 0, 0, 0);
			graphics->drawRegion(this->imgCharacterSelectionAssets, 31, 60, 19, 32, j + 17, 161, 0, 4, 0);
		}

		switch ((this->stateVars[1] == 0) ? this->getCharacterConstantByOrder(i) : this->stateVars[0]) {
			case 1: {
				img = this->imgMajorMugs;
				textID = MenuStrings::CHARACTER_SELECT_MAJOR;
				this->graphics.currentCharColor = 5;
				break;
			}
			case 3: {
				img = this->imgScientistMugs;
				textID = MenuStrings::CHARACTER_SELECT_SCIENTIST;
				this->graphics.currentCharColor = 5;
				break;
			}
			case 2: {
				img = this->imgSargeMugs;
				textID = MenuStrings::CHARACTER_SELECT_SERGEANT;
				this->graphics.currentCharColor = 5;
				break;
			}
		}

		graphics->drawRegion(img, 0, 0, 0x20, 0x20, j, 0x4b, 0, 0, 0);
		textBuff->setLength(0);
		app->localization->composeText(Strings::FILE_MENUSTRINGS, textID, textBuff);
		textBuff->dehyphenate();
		graphics->drawString(textBuff, j + (img->width / 2), 142, 1);

		j += 75;
	}

	this->drawCharacterSelectionAvatar(this->stateVars[0], -10, 0x8c, graphics);
	this->drawCharacterSelectionStats(this->stateVars[0], textBuff, 0x16a, 0x73, graphics);

	textBuff->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::CHARACTER_SELECT_CONFIRM, textBuff);
	textBuff->wrapText(0x18, '\n');
	graphics->drawString(textBuff, this->SCR_CX, this->screenRect[3] - 85, 1);

	textBuff->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::BACK_ITEM, textBuff);
	button = this->m_characterButtons->GetButton(4);
	graphics->fillRect(this->SCR_CX - 85, this->screenRect[3] - 60, 70, 30, button->highlighted ? 0x2896ff : 0x646464);
	graphics->drawRect(this->SCR_CX - 85, this->screenRect[3] - 60, 70, 30);
	button->SetTouchArea(this->SCR_CX - 85, this->screenRect[3] - 60, 70, 30);
	graphics->drawString(textBuff, this->SCR_CX - 50, this->screenRect[3] - 50, 1);

	int8_t b = this->OSC_CYCLE[app->time / 100 % 4];

	// [GEC]
	if (this->stateVars[2] == 0 && this->stateVars[8] == 1) { // J2ME/BREW
		graphics->drawCursor((this->SCR_CX - 50 - 6) - (textBuff->getStringWidth() / 2) + b, this->screenRect[3] - 50, 0x18, true);
	}

	textBuff->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::YES_LABEL, textBuff);
	button = this->m_characterButtons->GetButton(3);
	graphics->fillRect(this->SCR_CX + 15, this->screenRect[3] - 60, 70, 30, button->highlighted ? 0x2896ff : 0x646464);
	graphics->drawRect(this->SCR_CX + 15, this->screenRect[3] - 60, 70, 30);
	button->SetTouchArea(this->SCR_CX + 15, this->screenRect[3] - 60, 70, 30);
	graphics->drawString(textBuff, this->SCR_CX + 50, this->screenRect[3] - 50, 1);

	// [GEC]
	if (this->stateVars[2] == 1 && this->stateVars[8] == 1) { // J2ME/BREW
		graphics->drawCursor((this->SCR_CX + 50 - 6) - (textBuff->getStringWidth() / 2) + b, this->screenRect[3] - 50, 0x18, true);
	}

	textBuff->dispose();
}


void Canvas::drawCharacterSelectionAvatar(int i, int x, int y, Graphics* graphics)
{

	Image* troso, * legs;

	switch (i) {
	case 1: {
		legs = this->imgMajor_legs;
		troso = this->imgMajor_torso;
		break;
	}
	case 3: {
		legs = this->imgRiley_legs;
		troso = this->imgRiley_torso;
		break;
	}
	case 2: {
		legs = this->imgSarge_legs;
		troso = this->imgSarge_torso;
		break;
	}
	}

	graphics->drawImage(legs, x, y, 0, 0, 0);
	graphics->drawImage(troso, x, y - (app->time / 1000 & 1U), 0, 0, 0);
}


void Canvas::drawCharacterSelectionStats(int i, Text* text, int x, int y, Graphics* graphics) {

	int defense;
	int strength;
	int accuracy;
	int agility;
	int iq;

	switch (i) {
	case 1: {
		defense = 8;
		strength = 9;
		accuracy = 97;
		agility = 12;
		iq = 110;
		break;
	}
	case 3: {
		defense = 8;
		strength = 8;
		accuracy = 87;
		agility = 6;
		iq = 150;
		break;
	}
	case 2: {
		defense = 12;
		strength = 14;
		accuracy = 92;
		agility = 6;
		iq = 100;
		break;
	}
	}

	if (app->game->difficulty == Enums::DIFFICULTY_NORMAL) {
		defense = 0;
	}

	graphics->drawImage(this->imgCharacter_select_stat_header, x - 2, y - 4, 0, 0, 0);
	graphics->drawImage(this->imgCharacter_select_stat_bar, x - 2, y + 14, 0, 0, 0);

	int yFix = -2; // [GEC] ajusta el texto

	text->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::DEFENSE_LABEL, text);
	text->dehyphenate();
	text->append(defense);
	this->graphics.currentCharColor = 5;
	graphics->drawString(text, x, y + 18 + yFix, 20);
	graphics->drawImage(this->imgCharacter_select_stat_bar, x - 2, y + 34, 0, 0, 0);

	text->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::STRENGTH_LABEL, text);
	text->dehyphenate();
	text->append(strength);
	this->graphics.currentCharColor = 5;
	graphics->drawString(text, x, y + 38 + yFix, 20);
	graphics->drawImage(this->imgCharacter_select_stat_bar, x - 2, y + 54, 0, 0, 0);

	text->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::ACCURACY_LABEL, text);
	text->dehyphenate();
	text->append(accuracy);
	this->graphics.currentCharColor = 5;
	graphics->drawString(text, x, y + 58 + yFix, 20);
	graphics->drawImage(this->imgCharacter_select_stat_bar, x - 2, y + 74, 0, 0, 0);

	text->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::AGILITY_LABEL, text);
	text->dehyphenate();
	text->append(agility);
	this->graphics.currentCharColor = 5;
	graphics->drawString(text, x, y + 78 + yFix, 20);
	graphics->drawImage(this->imgCharacter_select_stat_bar, x - 2, y + 94, 0, 0, 0);

	text->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::IQ_LABEL, text);
	text->dehyphenate();
	text->append(iq);
	this->graphics.currentCharColor = 5;
	graphics->drawString(text, x, y + 98 + yFix, 20);
}


void Canvas::handleCharacterSelectionInput(int key, int action) {

	//printf("handleCharacterSelectionInput key %d, action %d\n", key, action);
	if (this->stateVars[1]) {
		if (this->stateVars[1] == 1) {

			if ((action == Enums::ACTION_LEFT) || (action == Enums::ACTION_RIGHT)) {
				this->stateVars[2] = this->stateVars[2] != 1;
			}
			else if (action == Enums::ACTION_FIRE) {
				if (this->stateVars[2] == 1) {
					app->player->setCharacterChoice(this->stateVars[0]);
					app->player->reset();
					app->canvas->setState(Canvas::ST_INTRO);
					this->disposeCharacterSelection();
				}
				else {
					this->stateVars[1] = 0;
					this->stateVars[2] = 1;
				}
			}
			
		}
	}
	else {
		for (int i = 0; i < 3; i++) { // Characters
			if (this->m_characterButtons->GetButton(i)->highlighted) {
				if (i == 0) {
					this->stateVars[0] = 1;
				}
				else if (i == 1) {
					this->stateVars[0] = 3;
				}
				else {
					this->stateVars[0] = 2;
				}
			}
		}

		if (this->m_characterButtons->GetButton(3)->highlighted) { // Yes
			app->player->setCharacterChoice(this->stateVars[0]);
			app->player->reset();
			app->canvas->setState(Canvas::ST_INTRO);
			this->disposeCharacterSelection();
		}

		if (this->m_characterButtons->GetButton(4)->highlighted) { // Back
			app->canvas->backToMain(false);
		}

		this->m_characterButtons->HighlightButton(0, 0, false);

		if (!this->touched) {
			if (this->stateVars[8] == 0) { // [GEC]
				if (action == Enums::ACTION_RIGHT) {
					for (int i = 0; i < 3; ++i) {
						if (this->stateVars[0] == this->getCharacterConstantByOrder(i)) {
							this->stateVars[0] = this->getCharacterConstantByOrder((i + 1) % 3);
							app->menuSystem->soundClick();
							break;
						}
					}
				}
				else if (action == Enums::ACTION_LEFT) {
					for (int j = 0; j < 3; ++j) {
						if (this->stateVars[0] == this->getCharacterConstantByOrder(j)) {
							this->stateVars[0] = this->getCharacterConstantByOrder((j + 2) % 3);
							app->menuSystem->soundClick();
							break;
						}
					}
				}
				else if (action == Enums::ACTION_FIRE) {
					this->stateVars[8] = 1; // [GEC]
					app->sound->playSound(Sounds::getResIDByName(SoundName::PISTOL), 0, 5, false);
				}
				else if (action == Enums::ACTION_MENU) {
					this->disposeCharacterSelection();
					this->backToMain(false);
				}
			}
			else if (this->stateVars[8] == 1) { // [GEC]
				if ((action == Enums::ACTION_LEFT) || (action == Enums::ACTION_RIGHT)) {
					this->stateVars[2] = this->stateVars[2] != 1;
					app->menuSystem->soundClick();
				}
				else if (action == Enums::ACTION_FIRE) {
					if (this->stateVars[2] == 1) {
						app->player->setCharacterChoice(this->stateVars[0]);
						app->player->reset();
						app->canvas->setState(Canvas::ST_INTRO);
						this->disposeCharacterSelection();
					}
					else {
						this->disposeCharacterSelection();
						this->backToMain(false);
					}
					app->sound->playSound(Sounds::getResIDByName(SoundName::PISTOL), 0, 5, false);
				}
				else if (action == Enums::ACTION_MENU) {
					this->stateVars[1] = 0;
					this->stateVars[2] = 1;
					this->stateVars[8] = 0; // [GEC]
					app->sound->playSound(Sounds::getResIDByName(SoundName::SWITCH_EXIT), 0, 5, false);
				}
			}
		}
	}
}


void Canvas::disposeCharacterSelection() {
	this->imgCharacter_select_stat_bar->~Image();
	this->imgCharacter_select_stat_bar = nullptr;
	this->imgCharacter_select_stat_header->~Image();
	this->imgCharacter_select_stat_header = nullptr;
	this->imgTopBarFill->~Image();
	this->imgTopBarFill = nullptr;
	this->imgCharacter_upperbar->~Image();
	this->imgCharacter_upperbar = nullptr;
	this->imgCharacterSelectionAssets->~Image();
	this->imgCharacterSelectionAssets = nullptr;
	this->imgCharSelectionBG->~Image();
	this->imgCharSelectionBG = nullptr;
	this->imgMajorMugs->~Image();
	this->imgMajorMugs = nullptr;
	this->imgSargeMugs->~Image();
	this->imgSargeMugs = nullptr;
	this->imgScientistMugs->~Image();
	this->imgScientistMugs = nullptr;
	this->imgMajor_legs->~Image();
	this->imgMajor_legs = nullptr;
	this->imgMajor_torso->~Image();
	this->imgMajor_torso = nullptr;
	this->imgRiley_legs->~Image();
	this->imgRiley_legs = nullptr;
	this->imgRiley_torso->~Image();
	this->imgRiley_torso = nullptr;
	this->imgSarge_legs->~Image();
	this->imgSarge_legs = nullptr;
	this->imgSarge_torso->~Image();
	this->imgSarge_torso = nullptr;
}


