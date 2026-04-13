#include "CharacterSelectionState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Player.h"
#include "Combat.h"
#include "MenuSystem.h"
#include "Graphics.h"
#include "Image.h"
#include "Text.h"
#include "Button.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Enums.h"
#include "Menus.h"
#include "MenuStrings.h"

static int getCharacterConstantByOrder(int i) {
	switch (i) {
	case 0: return 1;
	case 1: return 3;
	case 2: return 2;
	}
	return 0;
}

static void setupCharacterSelection(Canvas* canvas) {
	Applet* app = canvas->app;
	app->beginImageLoading();
	canvas->imgCharacter_select_stat_bar = app->loadImage("Character_select_stat_bar.bmp", true);
	canvas->imgCharacter_select_stat_header = app->loadImage("Character_select_stat_header.bmp", true);
	canvas->imgTopBarFill = app->loadImage("Character_select_top_bar.bmp", true);
	canvas->imgCharacter_upperbar = app->loadImage("character_upperbar.bmp", true);
	canvas->imgCharacterSelectionAssets = app->loadImage("charSelect.bmp", true);
	canvas->imgCharSelectionBG = app->loadImage("charSelectionBG.bmp", true);
	canvas->imgMajorMugs = app->loadImage("Hud_Player.bmp", true);
	canvas->imgSargeMugs = app->loadImage("Hud_PlayerDoom.bmp", true);
	canvas->imgScientistMugs = app->loadImage("Hud_PlayerScientist.bmp", true);
	canvas->imgMajor_legs = app->loadImage("Major_legs.bmp", true);
	canvas->imgMajor_torso = app->loadImage("Major_torso.bmp", true);
	canvas->imgRiley_legs = app->loadImage("Riley_legs.bmp", true);
	canvas->imgRiley_torso = app->loadImage("Riley_torso.bmp", true);
	canvas->imgSarge_legs = app->loadImage("Sarge_legs.bmp", true);
	canvas->imgSarge_torso = app->loadImage("Sarge_torso.bmp", true);
	app->endImageLoading();
}

static void disposeCharacterSelection(Canvas* canvas) {
	canvas->imgCharacter_select_stat_bar->~Image();
	canvas->imgCharacter_select_stat_bar = nullptr;
	canvas->imgCharacter_select_stat_header->~Image();
	canvas->imgCharacter_select_stat_header = nullptr;
	canvas->imgTopBarFill->~Image();
	canvas->imgTopBarFill = nullptr;
	canvas->imgCharacter_upperbar->~Image();
	canvas->imgCharacter_upperbar = nullptr;
	canvas->imgCharacterSelectionAssets->~Image();
	canvas->imgCharacterSelectionAssets = nullptr;
	canvas->imgCharSelectionBG->~Image();
	canvas->imgCharSelectionBG = nullptr;
	canvas->imgMajorMugs->~Image();
	canvas->imgMajorMugs = nullptr;
	canvas->imgSargeMugs->~Image();
	canvas->imgSargeMugs = nullptr;
	canvas->imgScientistMugs->~Image();
	canvas->imgScientistMugs = nullptr;
	canvas->imgMajor_legs->~Image();
	canvas->imgMajor_legs = nullptr;
	canvas->imgMajor_torso->~Image();
	canvas->imgMajor_torso = nullptr;
	canvas->imgRiley_legs->~Image();
	canvas->imgRiley_legs = nullptr;
	canvas->imgRiley_torso->~Image();
	canvas->imgRiley_torso = nullptr;
	canvas->imgSarge_legs->~Image();
	canvas->imgSarge_legs = nullptr;
	canvas->imgSarge_torso->~Image();
	canvas->imgSarge_torso = nullptr;
}

static void drawAvatar(Canvas* canvas, int i, int x, int y, Graphics* graphics) {
	Image* torso;
	Image* legs;
	switch (i) {
	case 1: legs = canvas->imgMajor_legs; torso = canvas->imgMajor_torso; break;
	case 3: legs = canvas->imgRiley_legs; torso = canvas->imgRiley_torso; break;
	case 2: legs = canvas->imgSarge_legs; torso = canvas->imgSarge_torso; break;
	default: return;
	}
	graphics->drawImage(legs, x, y, 0, 0, 0);
	graphics->drawImage(torso, x, y - (canvas->app->time / 1000 & 1U), 0, 0, 0);
}

static void drawStats(Canvas* canvas, int i, Text* text, int x, int y, Graphics* graphics) {
	Applet* app = canvas->app;
	int defense, strength, accuracy, agility, iq;
	switch (i) {
	case 1: defense = 8; strength = 9; accuracy = 97; agility = 12; iq = 110; break;
	case 3: defense = 8; strength = 8; accuracy = 87; agility = 6; iq = 150; break;
	case 2: defense = 12; strength = 14; accuracy = 92; agility = 6; iq = 100; break;
	default: return;
	}
	if (app->game->difficulty == Enums::DIFFICULTY_NORMAL) { defense = 0; }

	graphics->drawImage(canvas->imgCharacter_select_stat_header, x - 2, y - 4, 0, 0, 0);
	int yFix = -2;
	struct { int stringId; int value; } stats[] = {
		{MenuStrings::DEFENSE_LABEL, defense}, {MenuStrings::STRENGTH_LABEL, strength},
		{MenuStrings::ACCURACY_LABEL, accuracy}, {MenuStrings::AGILITY_LABEL, agility},
		{MenuStrings::IQ_LABEL, iq}
	};
	for (int s = 0; s < 5; s++) {
		graphics->drawImage(canvas->imgCharacter_select_stat_bar, x - 2, y + 14 + s * 20, 0, 0, 0);
		text->setLength(0);
		app->localization->composeText(Strings::FILE_MENUSTRINGS, stats[s].stringId, text);
		text->dehyphenate();
		text->append(stats[s].value);
		canvas->graphics.currentCharColor = 5;
		graphics->drawString(text, x, y + 18 + s * 20 + yFix, 20);
	}
}

void CharacterSelectionState::onEnter(Canvas* canvas) {
	canvas->clearSoftKeys();
	setupCharacterSelection(canvas);
	canvas->stateVars[0] = 1;
	canvas->stateVars[1] = 0;
	canvas->stateVars[2] = 1;
	canvas->stateVars[8] = 0;
}

void CharacterSelectionState::onExit(Canvas* canvas) {
}

void CharacterSelectionState::update(Canvas* canvas) {
}

void CharacterSelectionState::render(Canvas* canvas, Graphics* graphics) {
	Applet* app = canvas->app;
	fmButton* button;
	Text* textBuff;
	Image* img;
	int textID = 0;

	graphics->clipRect(0, 0, canvas->screenRect[2], canvas->screenRect[3]);
	graphics->drawImage(canvas->imgCharSelectionBG, 0, 0, 0, 0, 0);
	graphics->drawImage(canvas->imgTopBarFill, canvas->SCR_CX - canvas->imgTopBarFill->width / 2, 0, 0, 0, 0);

	textBuff = app->localization->getSmallBuffer();
	textBuff->setLength(0);
	switch (canvas->stateVars[0]) {
		case 1: app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::CHARACTER_SELECT_MAJOR_NAME, textBuff); break;
		case 3: app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::CHARACTER_SELECT_SCIENTIST_NAME, textBuff); break;
		case 2: app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::CHARACTER_SELECT_SERGEANT_NAME, textBuff); break;
	}
	textBuff->dehyphenate();
	graphics->drawString(textBuff, canvas->SCR_CX, 3, 1);

	int j = 152;
	for (int i = 0; i < ((canvas->stateVars[1] == 0) ? 3 : 1); i++) {
		bool b = getCharacterConstantByOrder(i) == canvas->stateVars[0] || canvas->stateVars[1] != 0;
		graphics->drawRegion(canvas->imgCharacterSelectionAssets, 0, 0, 31, 83, j - 15, 55, 0, 0, 0);
		graphics->drawRegion(canvas->imgCharacterSelectionAssets, 0, 0, 31, 83, j + 16, 55, 0, 4, 0);
		button = canvas->m_characterButtons->GetButton(i);
		if (button->highlighted) {
			graphics->fillRect(j - 15, 55, 62, 83, 0x2896ff);
			graphics->drawRegion(canvas->imgCharacterSelectionAssets, 31, 0, 27, 60, j - 11, 64, 0, 0, 0);
			graphics->drawRegion(canvas->imgCharacterSelectionAssets, 31, 0, 27, 60, j + 16, 64, 0, 4, 0);
		}
		button->SetTouchArea(j - 15, 55, 62, 83);
		if (i < 2 && canvas->stateVars[1] == 0) {
			for (int x = (j + 46); x < (j + 64); x += 6) {
				graphics->drawRegion(canvas->imgCharacterSelectionAssets, 51, 60, 6, 19, x, 81, 0, 0, 0);
			}
		}
		if (b != 0) {
			graphics->drawRegion(canvas->imgCharacterSelectionAssets, 31, 60, 20, 32, j - 3, 161, 0, 0, 0);
			graphics->drawRegion(canvas->imgCharacterSelectionAssets, 31, 60, 19, 32, j + 17, 161, 0, 4, 0);
		}
		switch ((canvas->stateVars[1] == 0) ? getCharacterConstantByOrder(i) : canvas->stateVars[0]) {
			case 1: img = canvas->imgMajorMugs; textID = MenuStrings::CHARACTER_SELECT_MAJOR; canvas->graphics.currentCharColor = 5; break;
			case 3: img = canvas->imgScientistMugs; textID = MenuStrings::CHARACTER_SELECT_SCIENTIST; canvas->graphics.currentCharColor = 5; break;
			case 2: img = canvas->imgSargeMugs; textID = MenuStrings::CHARACTER_SELECT_SERGEANT; canvas->graphics.currentCharColor = 5; break;
		}
		graphics->drawRegion(img, 0, 0, 0x20, 0x20, j, 0x4b, 0, 0, 0);
		textBuff->setLength(0);
		app->localization->composeText(Strings::FILE_MENUSTRINGS, textID, textBuff);
		textBuff->dehyphenate();
		graphics->drawString(textBuff, j + (img->width / 2), 142, 1);
		j += 75;
	}

	drawAvatar(canvas, canvas->stateVars[0], -10, 0x8c, graphics);
	drawStats(canvas, canvas->stateVars[0], textBuff, 0x16a, 0x73, graphics);

	textBuff->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::CHARACTER_SELECT_CONFIRM, textBuff);
	textBuff->wrapText(0x18, '\n');
	graphics->drawString(textBuff, canvas->SCR_CX, canvas->screenRect[3] - 85, 1);

	textBuff->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::BACK_ITEM, textBuff);
	button = canvas->m_characterButtons->GetButton(4);
	graphics->fillRect(canvas->SCR_CX - 85, canvas->screenRect[3] - 60, 70, 30, button->highlighted ? 0x2896ff : 0x646464);
	graphics->drawRect(canvas->SCR_CX - 85, canvas->screenRect[3] - 60, 70, 30);
	button->SetTouchArea(canvas->SCR_CX - 85, canvas->screenRect[3] - 60, 70, 30);
	graphics->drawString(textBuff, canvas->SCR_CX - 50, canvas->screenRect[3] - 50, 1);

	int8_t osc = canvas->OSC_CYCLE[app->time / 100 % 4];
	if (canvas->stateVars[2] == 0 && canvas->stateVars[8] == 1) {
		graphics->drawCursor((canvas->SCR_CX - 50 - 6) - (textBuff->getStringWidth() / 2) + osc, canvas->screenRect[3] - 50, 0x18, true);
	}

	textBuff->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::YES_LABEL, textBuff);
	button = canvas->m_characterButtons->GetButton(3);
	graphics->fillRect(canvas->SCR_CX + 15, canvas->screenRect[3] - 60, 70, 30, button->highlighted ? 0x2896ff : 0x646464);
	graphics->drawRect(canvas->SCR_CX + 15, canvas->screenRect[3] - 60, 70, 30);
	button->SetTouchArea(canvas->SCR_CX + 15, canvas->screenRect[3] - 60, 70, 30);
	graphics->drawString(textBuff, canvas->SCR_CX + 50, canvas->screenRect[3] - 50, 1);

	if (canvas->stateVars[2] == 1 && canvas->stateVars[8] == 1) {
		graphics->drawCursor((canvas->SCR_CX + 50 - 6) - (textBuff->getStringWidth() / 2) + osc, canvas->screenRect[3] - 50, 0x18, true);
	}

	textBuff->dispose();
}

bool CharacterSelectionState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = canvas->app;
	if (canvas->stateVars[1]) {
		if (canvas->stateVars[1] == 1) {
			if ((action == Enums::ACTION_LEFT) || (action == Enums::ACTION_RIGHT)) {
				canvas->stateVars[2] = canvas->stateVars[2] != 1;
			}
			else if (action == Enums::ACTION_FIRE) {
				if (canvas->stateVars[2] == 1) {
					app->player->setCharacterChoice(canvas->stateVars[0]);
					app->player->reset();
					canvas->setState(Canvas::ST_INTRO);
					disposeCharacterSelection(canvas);
				}
				else {
					canvas->stateVars[1] = 0;
					canvas->stateVars[2] = 1;
				}
			}
		}
	}
	else {
		for (int i = 0; i < 3; i++) {
			if (canvas->m_characterButtons->GetButton(i)->highlighted) {
				canvas->stateVars[0] = (i == 0) ? 1 : (i == 1) ? 3 : 2;
			}
		}
		if (canvas->m_characterButtons->GetButton(3)->highlighted) {
			app->player->setCharacterChoice(canvas->stateVars[0]);
			app->player->reset();
			canvas->setState(Canvas::ST_INTRO);
			disposeCharacterSelection(canvas);
		}
		if (canvas->m_characterButtons->GetButton(4)->highlighted) {
			canvas->backToMain(false);
		}
		canvas->m_characterButtons->HighlightButton(0, 0, false);

		if (!canvas->touched) {
			if (canvas->stateVars[8] == 0) {
				if (action == Enums::ACTION_RIGHT) {
					for (int i = 0; i < 3; ++i) {
						if (canvas->stateVars[0] == getCharacterConstantByOrder(i)) {
							canvas->stateVars[0] = getCharacterConstantByOrder((i + 1) % 3);
							app->menuSystem->soundClick();
							break;
						}
					}
				}
				else if (action == Enums::ACTION_LEFT) {
					for (int j = 0; j < 3; ++j) {
						if (canvas->stateVars[0] == getCharacterConstantByOrder(j)) {
							canvas->stateVars[0] = getCharacterConstantByOrder((j + 2) % 3);
							app->menuSystem->soundClick();
							break;
						}
					}
				}
				else if (action == Enums::ACTION_FIRE) {
					canvas->stateVars[8] = 1;
					app->sound->playSound(Sounds::getResIDByName(SoundName::PISTOL), 0, 5, false);
				}
				else if (action == Enums::ACTION_MENU) {
					disposeCharacterSelection(canvas);
					canvas->backToMain(false);
				}
			}
			else if (canvas->stateVars[8] == 1) {
				if ((action == Enums::ACTION_LEFT) || (action == Enums::ACTION_RIGHT)) {
					canvas->stateVars[2] = canvas->stateVars[2] != 1;
					app->menuSystem->soundClick();
				}
				else if (action == Enums::ACTION_FIRE) {
					if (canvas->stateVars[2] == 1) {
						app->player->setCharacterChoice(canvas->stateVars[0]);
						app->player->reset();
						canvas->setState(Canvas::ST_INTRO);
						disposeCharacterSelection(canvas);
					}
					else {
						disposeCharacterSelection(canvas);
						canvas->backToMain(false);
					}
					app->sound->playSound(Sounds::getResIDByName(SoundName::PISTOL), 0, 5, false);
				}
				else if (action == Enums::ACTION_MENU) {
					canvas->stateVars[1] = 0;
					canvas->stateVars[2] = 1;
					canvas->stateVars[8] = 0;
					app->sound->playSound(Sounds::getResIDByName(SoundName::SWITCH_EXIT), 0, 5, false);
				}
			}
		}
	}
	return true;
}
