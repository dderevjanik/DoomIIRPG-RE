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

void Canvas::loadPrologueText() {

	Text* text;

	this->storyPage = 0;
	this->storyTotalPages = 0;
	app->localization->resetTextArgs();

	short n = 0;
	switch (app->player->characterChoice) {
	case 1: {
		n = 218;
		break;
	}
	case 2: {
		n = 219;
		break;
	}
	default: {
		n = 220;
		break;
	}
	}

	app->localization->addTextArg(3, n);
	text = app->localization->getLargeBuffer();
	app->localization->loadText(2);
	app->localization->composeText(2, 12, text);
	app->localization->unloadText(2);

	int n2 = (this->displayRect[2] - 30) / 10;
	int n3 = (this->displayRect[3] - 40) / 21;
	text->wrapText(n2);

	this->storyIndexes[this->storyTotalPages++] = 0;

	int n4 = 0;
	int first = 0;
	while ((first = text->findFirstOf('|', first)) != -1) {
		++first;
		if (++n4 % n3 == 0) {
			this->storyIndexes[this->storyTotalPages++] = first;
		}
	}

	this->dialogBuffer = text;
	this->storyIndexes[this->storyTotalPages] = text->length();
	this->storyX = this->displayRect[0] + 15;
	this->storyY = this->displayRect[1] + 20;
}


void Canvas::loadEpilogueText() {

	this->imgProlog = app->loadImage("prolog.bmp", true);
	this->initScrollingText((short)0, (short)134, false, 32, 1, 1000);
}


void Canvas::disposeIntro() {
	this->dialogBuffer->dispose();
	this->dialogBuffer = nullptr;
	this->loadMap(this->startupMap, false, true);
}


void Canvas::disposeEpilogue() {

	this->dialogBuffer->dispose();
	this->dialogBuffer = nullptr;
	app->sound->soundStop();
	app->menuSystem->setMenu(Menus::MENU_LEVEL_STATS);
	app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_LEVEL_END),'\x01',3,false);
}


void Canvas::initScrollingText(short i, short i2, bool dehyphenate, int spacingHeight, int numLines, int textMSLine) {


	if (this->dialogBuffer == nullptr) {
		this->dialogBuffer = app->localization->getLargeBuffer();
	}
	else {
		this->dialogBuffer->setLength(0);
	}

	app->localization->composeText(i, i2, this->dialogBuffer);

	if (dehyphenate) {
		this->dialogBuffer->dehyphenate();
	}

	this->dialogBuffer->wrapText((this->displayRect[2] - 8) / Applet::CHAR_SPACING[app->fontType]);
	this->scrollingTextSpacing = spacingHeight;
	this->scrollingTextStart = -1;
	this->scrollingTextLines = (this->dialogBuffer->getNumLines() + numLines);
	this->scrollingTextMSLine = textMSLine;
	this->scrollingTextDone = false;
	this->scrollingTextFontHeight = Applet::FONT_HEIGHT[app->fontType] + 2;
	this->scrollingTextSpacingHeight = spacingHeight * ((316 - (Applet::FONT_HEIGHT[app->fontType] * 2)) / spacingHeight);
}


void Canvas::drawCredits(Graphics* graphics) {

	Text* textBuff;

	this->drawScrollingText(graphics);
	if (this->scrollingTextDone != false) {
		textBuff = app->localization->getSmallBuffer();
		textBuff->setLength(0);
		app->localization->composeText(0, 43, textBuff);
		textBuff->dehyphenate();
		graphics->drawString(textBuff, this->SCR_CX - 24, this->screenRect[3] - 32, 2);
		textBuff->dispose();
	}
}


void Canvas::drawScrollingText(Graphics* graphics) {


	int n = this->scrollingTextMSLine * ((this->scrollingTextSpacing << 16) >> 4) >> 16;
	if (this->scrollingTextStart == -1) {
		this->scrollingTextStart = app->gameTime;
		int n2 = this->screenRect[3] / this->scrollingTextSpacing;
		if (this->state == Canvas::ST_CREDITS) {
			this->scrollingTextEnd = n * this->scrollingTextLines;
		}
		else {
			this->scrollingTextEnd = n * (this->scrollingTextLines + (n2 - 2));
		}
	}

	int gameTime = app->gameTime;
	int scrollingTextStart = this->scrollingTextStart;
	if (gameTime - scrollingTextStart > this->scrollingTextEnd) {
		scrollingTextStart = gameTime - this->scrollingTextEnd;
		this->scrollingTextDone = true;
	}

	graphics->eraseRgn(this->displayRect);

	if (this->state == Canvas::ST_EPILOGUE || this->state == Canvas::ST_INTRO_MOVIE) {
		int rect[4];
		rect[0] = this->displayRect[0];
		rect[1] = this->displayRect[1];
		rect[2] = this->displayRect[2] - rect[0];
		rect[3] = this->displayRect[3]; // missing line code?
		graphics->clipRect(0, (this->displayRect[3] - 220) / 2, this->displayRect[2], 220);
		graphics->drawRegion(this->imgProlog, 0, 65, Applet::IOS_WIDTH, 307, 0, 0, 0, 0, 0);
		graphics->fade(rect, 192, 0);
		graphics->setScreenSpace(0, (this->displayRect[3] - 220) / 2, this->displayRect[2], 220);
	}

	graphics->drawString(this->dialogBuffer, this->SCR_CX, this->cinRect[3] - ((((gameTime - scrollingTextStart) << 8) / n) * (this->scrollingTextSpacing << 8) >> 16), this->scrollingTextSpacing, 1, 0, -1);
	graphics->resetScreenSpace();
}


void Canvas::changeStoryPage(int i) {
	if (i < 0 && this->storyPage == 0) {
		if (this->state == Canvas::ST_INTRO) {
			this->setState(Canvas::ST_CHARACTER_SELECTION);
			this->dialogBuffer->dispose();
			this->dialogBuffer = nullptr;
		}
	}
	else {
		this->storyPage += i;
	}
}


void Canvas::drawStory(Graphics* graphics)
{


	Text* this_00;
	Text* this_01;
	short i2;
	Text* text;

	if (this->storyPage < this->storyTotalPages) {
		graphics->drawImage(app->hackingGame->imgHelpScreenAssets, 0, 0, 0, 0, 0);

		this_00 = app->localization->getLargeBuffer();
		if ((this->state != Canvas::ST_EPILOGUE) || (0 < this->storyPage)) {
			this_00->setLength(0);
			app->localization->composeText(3, 80, this_00); // "Back"
			this_00->dehyphenate();
			app->setFontRenderMode(2);
			if (this->m_storyButtons->GetButton(0)->highlighted != false) {
				app->setFontRenderMode(0);
			}
			graphics->drawString(this_00, 17, 310, 36); // Old -> 2, 319, 36
			app->setFontRenderMode(0);
		}

		this_00->setLength(0);
		this_01 = app->localization->getSmallBuffer();
		if (this->storyPage < this->storyTotalPages + -1) {
			app->localization->composeText(0, 56, this_00); // more
			i2 = 40; // Skip
			text = this_01;
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.x = 420; // [GEC], ajusta la posicion X de la caja de toque
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.w = 60; // [GEC], ajusta el ancho de la caja de toque
		}
		else {
			i2 = 43; // Continue
			text = this_00;
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.x = 380; // [GEC], ajusta la posicion X de la caja de toque
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.w = 100; // [GEC], ajusta el ancho de la caja de toque
		}

		app->localization->composeText(0, i2, text);
		this_00->dehyphenate();
		this_01->dehyphenate();
		app->setFontRenderMode(2);
		if (this->m_storyButtons->GetButton(1)->highlighted != false) {
			app->setFontRenderMode(0);
		}
		graphics->drawString(this_00, 463, 310, 40); // Old -> 478, 319, 40);
		app->setFontRenderMode(0);

		app->setFontRenderMode(2);
		if (this->m_storyButtons->GetButton(2)->highlighted != false) {
			app->setFontRenderMode(0);
		}
		graphics->drawString(this_01, 463, 10, 8); // Old -> 478, 1, 8);

		app->setFontRenderMode(0);
		this_00->dispose();
		this_01->dispose();

		graphics->drawString(this->dialogBuffer, this->storyX, this->storyY, 21, 0, this->storyIndexes[0],
			this->storyIndexes[1] - this->storyIndexes[0]);
	}
}


void Canvas::handleStoryInput(int key, int action) {


	if (action == Enums::ACTION_LEFT || action == Enums::ACTION_RIGHT) {
		if (this->stateVars[0] != 2) {
			this->stateVars[0] ^= 1;
		}
	}
	else if (action == Enums::ACTION_UP) {
		if (this->stateVars[0] != 2 && this->storyPage < this->storyTotalPages - 1) {
			this->stateVars[0] = 2;
		}
	}
	else if (action == Enums::ACTION_DOWN) {
		if (this->stateVars[0] == 2) {
			this->stateVars[0] = 1;
		}
	}
	else if (action == Enums::ACTION_FIRE) {
		switch (this->stateVars[0]) {
		case 0: {
			this->changeStoryPage(-1);
			if (this->state == Canvas::ST_CHARACTER_SELECTION) {
				this->stateVars[0] = app->player->characterChoice;
				break;
			}
			break;
		}
		case 1: {
			this->changeStoryPage(1);
			break;
		}
		case 2: {
			this->storyPage = this->storyTotalPages;
			break;
		}
		}
	}
	else if (action == Enums::ACTION_AUTOMAP) {
		this->changeStoryPage(1);
		this->stateVars[0] = 1;
	}
	else if (action == Enums::ACTION_BACK || action == Enums::ACTION_MENU) {
		this->changeStoryPage(-1);
		if (this->state == Canvas::ST_CHARACTER_SELECTION) {
			this->stateVars[0] = app->player->characterChoice;
		}
		else {
			this->stateVars[0] = 0;
		}
	}
}


void Canvas::playIntroMovie(Graphics* graphics) {


	if (app->canvas->skipIntro != false) {
		this->backToMain(true);
		return;
	}

	if (app->game->hasSeenIntro && app->game->skipMovie) {
		this->exitIntroMovie(false);
		return;
	}

	if (this->stateVars[1] == 0) {  // load table camera
		this->stateVars[1] = app->gameTime;
		app->game->loadTableCamera(14, 15);
		this->numEvents = 0;
		this->keyDown = false;
		this->keyDownCausedMove = false;
		this->ignoreFrameInput = 1;
		this->imgProlog = app->loadImage("prolog.bmp", true);
	}

	if (app->game) {
		if (app->game->mayaCameras)
		{
			if (this->stateVars[3] == 0) { // init movie prolog
				app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_GENERAL), 1, 6, false);
				if (this->stateVars[1] < app->gameTime) {
					app->game->activeCameraKey = -1;
					this->stateVars[3] = 0;
					this->stateVars[1] = app->gameTime;
					this->stateVars[2] = 0;
					this->stateVars[0] = 0;
					this->fadeRect = this->displayRect;
					this->fadeFlags = Canvas::FADE_FLAG_FADEIN;
					this->fadeColor = 0;
					this->fadeTime = app->time;
					this->fadeDuration = 1500;
					this->stateVars[3] = 1;
				}
			}
			else if (this->stateVars[3] == 1) { // draw movie prolog
				if (this->displayRect[3] > 220) {
					graphics->clipRect(0, (this->displayRect[3] - 220) / 2, this->displayRect[2], 220);
				}

				this->stateVars[0] = app->gameTime - app->game->activeCameraTime;
				this->stateVars[2] = app->gameTime - this->stateVars[1];

				app->game->mayaCameras->Update(app->game->activeCameraKey, this->stateVars[0]);

				int uVar3 = app->game->posShift;
				int texW = this->displayRect[2];
				int texH = this->displayRect[3];

				int posX = app->game->mayaCameras->x >> (uVar3 & 0xff);
				int posY = app->game->mayaCameras->y >> (uVar3 & 0xff);

				int texX = posX - this->SCR_CX; 
				if (texX < 0) {
					texX = 0;
				}

				int texY = posY - this->SCR_CY;
				if (texY < 0) {
					texY = 0;
				}

				posY = this->SCR_CY - posY;
				if (posY < 0) {
					posY = 0;
				}

				if (texX + texW > this->imgProlog->width) {
					texW = this->imgProlog->width - texX;
				}

				if (texY + texH > this->imgProlog->height) {
					texH = this->imgProlog->height - texY;
				}

				graphics->drawRegion(this->imgProlog, texX, texY, texW, texH, 0, posY, 0, 0, 0);
				if (app->game->mayaCameras->complete) {
					this->stateVars[3] += 1;
				}
			}
			else if (this->stateVars[3] == 2) { // init Scrolling Text
				this->initScrollingText(0, 133, false, 32, 1, 800);
				this->drawScrollingText(graphics);
				this->stateVars[3] += 1;
			}
			else if (this->stateVars[3] == 3) { // draw Scrolling Text
				this->drawScrollingText(graphics);
				if (this->scrollingTextDone) {
					this->exitIntroMovie(false);
				}
			}

			this->staleView = true;
			return;
		}
	}
}


void Canvas::exitIntroMovie(bool b) {


	app->game->cleanUpCamMemory();

	this->imgProlog->~Image();
	this->imgProlog = nullptr;

	if (this->dialogBuffer) {
		this->dialogBuffer->dispose();
		this->dialogBuffer = nullptr;
	}

	app->sound->soundStop();
	if (b == false) {
		app->game->hasSeenIntro = true;
		app->game->saveConfig();
		this->backToMain(false);
	}
}


