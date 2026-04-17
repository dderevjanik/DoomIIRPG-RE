#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include "Log.h"

#include "App.h"
#include "Image.h"
#include "CAppContainer.h"
#include "Canvas.h"
#include "Graphics.h"
#include "MayaCamera.h"
#include "Game.h"
#include "Hud.h"
#include "Player.h"
#include "MenuSystem.h"
#include "HackingGame.h"
#include "Text.h"
#include "Button.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Enums.h"
#include "Menus.h"
#include "StoryRenderer.h"

void StoryRenderer::loadPrologueText(Canvas* canvas) {
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

	int n2 = (canvas->displayRect[2] - 30) / 10;
	int n3 = (canvas->displayRect[3] - 40) / 21;
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

	canvas->dialogBuffer = text;
	this->storyIndexes[this->storyTotalPages] = text->length();
	this->storyX = canvas->displayRect[0] + 15;
	this->storyY = canvas->displayRect[1] + 20;
}


void StoryRenderer::loadEpilogueText(Canvas* canvas) {
	this->imgProlog = app->loadImage("prolog.bmp", true);
	this->initScrollingText(canvas, (short)0, (short)134, false, 32, 1, 1000);
}


void StoryRenderer::disposeIntro(Canvas* canvas) {
	canvas->dialogBuffer->dispose();
	canvas->dialogBuffer = nullptr;
	canvas->loadMap(canvas->startupMap, false, true);
}


void StoryRenderer::disposeEpilogue(Canvas* canvas) {
	canvas->dialogBuffer->dispose();
	canvas->dialogBuffer = nullptr;
	app->sound->soundStop();
	app->menuSystem->setMenu(Menus::MENU_LEVEL_STATS);
	app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_LEVEL_END),'\x01',3,false);
}


void StoryRenderer::initScrollingText(Canvas* canvas, short i, short i2, bool dehyphenate, int spacingHeight, int numLines, int textMSLine) {
	if (canvas->dialogBuffer == nullptr) {
		canvas->dialogBuffer = app->localization->getLargeBuffer();
	}
	else {
		canvas->dialogBuffer->setLength(0);
	}

	app->localization->composeText(i, i2, canvas->dialogBuffer);

	if (dehyphenate) {
		canvas->dialogBuffer->dehyphenate();
	}

	canvas->dialogBuffer->wrapText((canvas->displayRect[2] - 8) / Applet::CHAR_SPACING[app->fontType]);
	this->scrollingTextSpacing = spacingHeight;
	this->scrollingTextStart = -1;
	this->scrollingTextLines = (canvas->dialogBuffer->getNumLines() + numLines);
	this->scrollingTextMSLine = textMSLine;
	this->scrollingTextDone = false;
	this->scrollingTextFontHeight = Applet::FONT_HEIGHT[app->fontType] + 2;
	this->scrollingTextSpacingHeight = spacingHeight * ((316 - (Applet::FONT_HEIGHT[app->fontType] * 2)) / spacingHeight);
}


void StoryRenderer::drawCredits(Canvas* canvas, Graphics* graphics) {
	Text* textBuff;

	this->drawScrollingText(canvas, graphics);
	if (this->scrollingTextDone) {
		textBuff = app->localization->getSmallBuffer();
		textBuff->setLength(0);
		app->localization->composeText(0, 43, textBuff);
		textBuff->dehyphenate();
		graphics->drawString(textBuff, canvas->SCR_CX - 24, canvas->screenRect[3] - 32, 2);
		textBuff->dispose();
	}
}


void StoryRenderer::drawScrollingText(Canvas* canvas, Graphics* graphics) {
	int n = this->scrollingTextMSLine * ((this->scrollingTextSpacing << 16) >> 4) >> 16;
	if (this->scrollingTextStart == -1) {
		this->scrollingTextStart = app->gameTime;
		int n2 = canvas->screenRect[3] / this->scrollingTextSpacing;
		if (canvas->state == Canvas::ST_CREDITS) {
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

	graphics->eraseRgn(canvas->displayRect);

	if (canvas->state == Canvas::ST_EPILOGUE || canvas->state == Canvas::ST_INTRO_MOVIE) {
		int rect[4];
		rect[0] = canvas->displayRect[0];
		rect[1] = canvas->displayRect[1];
		rect[2] = canvas->displayRect[2] - rect[0];
		rect[3] = canvas->displayRect[3]; // missing line code?
		graphics->clipRect(0, (canvas->displayRect[3] - 220) / 2, canvas->displayRect[2], 220);
		graphics->drawRegion(this->imgProlog, 0, 65, Applet::IOS_WIDTH, 307, 0, 0, 0, 0, 0);
		graphics->fade(rect, 192, 0);
		graphics->setScreenSpace(0, (canvas->displayRect[3] - 220) / 2, canvas->displayRect[2], 220);
	}

	graphics->drawString(canvas->dialogBuffer, canvas->SCR_CX, canvas->cinRect[3] - ((((gameTime - scrollingTextStart) << 8) / n) * (this->scrollingTextSpacing << 8) >> 16), this->scrollingTextSpacing, 1, 0, -1);
	graphics->resetScreenSpace();
}


void StoryRenderer::changeStoryPage(Canvas* canvas, int delta) {
	if (delta < 0 && this->storyPage == 0) {
		if (canvas->state == Canvas::ST_INTRO) {
			canvas->setState(Canvas::ST_CHARACTER_SELECTION);
			canvas->dialogBuffer->dispose();
			canvas->dialogBuffer = nullptr;
		}
	}
	else {
		this->storyPage += delta;
	}
}


void StoryRenderer::drawStory(Canvas* canvas, Graphics* graphics) {
	Text* this_00;
	Text* this_01;
	short i2;
	Text* text;

	if (this->storyPage < this->storyTotalPages) {
		graphics->drawImage(app->hackingGame->imgHelpScreenAssets, 0, 0, 0, 0, 0);

		this_00 = app->localization->getLargeBuffer();
		if ((canvas->state != Canvas::ST_EPILOGUE) || (0 < this->storyPage)) {
			this_00->setLength(0);
			app->localization->composeText(3, 80, this_00); // "Back"
			this_00->dehyphenate();
			app->setFontRenderMode(2);
			if (this->m_storyButtons->GetButton(0)->highlighted) {
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
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.x = 420;
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.w = 60;
		}
		else {
			i2 = 43; // Continue
			text = this_00;
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.x = 380;
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.w = 100;
		}

		app->localization->composeText(0, i2, text);
		this_00->dehyphenate();
		this_01->dehyphenate();
		app->setFontRenderMode(2);
		if (this->m_storyButtons->GetButton(1)->highlighted) {
			app->setFontRenderMode(0);
		}
		graphics->drawString(this_00, 463, 310, 40);
		app->setFontRenderMode(0);

		app->setFontRenderMode(2);
		if (this->m_storyButtons->GetButton(2)->highlighted) {
			app->setFontRenderMode(0);
		}
		graphics->drawString(this_01, 463, 10, 8);

		app->setFontRenderMode(0);
		this_00->dispose();
		this_01->dispose();

		graphics->drawString(canvas->dialogBuffer, this->storyX, this->storyY, 21, 0, this->storyIndexes[0],
			this->storyIndexes[1] - this->storyIndexes[0]);
	}
}


void StoryRenderer::handleStoryInput(Canvas* canvas, int key, int action) {
	if (action == Enums::ACTION_LEFT || action == Enums::ACTION_RIGHT) {
		if (canvas->stateVars[0] != 2) {
			canvas->stateVars[0] ^= 1;
		}
	}
	else if (action == Enums::ACTION_UP) {
		if (canvas->stateVars[0] != 2 && this->storyPage < this->storyTotalPages - 1) {
			canvas->stateVars[0] = 2;
		}
	}
	else if (action == Enums::ACTION_DOWN) {
		if (canvas->stateVars[0] == 2) {
			canvas->stateVars[0] = 1;
		}
	}
	else if (action == Enums::ACTION_FIRE) {
		switch (canvas->stateVars[0]) {
		case 0: {
			this->changeStoryPage(canvas, -1);
			if (canvas->state == Canvas::ST_CHARACTER_SELECTION) {
				canvas->stateVars[0] = app->player->characterChoice;
				break;
			}
			break;
		}
		case 1: {
			this->changeStoryPage(canvas, 1);
			break;
		}
		case 2: {
			this->storyPage = this->storyTotalPages;
			break;
		}
		}
	}
	else if (action == Enums::ACTION_AUTOMAP) {
		this->changeStoryPage(canvas, 1);
		canvas->stateVars[0] = 1;
	}
	else if (action == Enums::ACTION_BACK || action == Enums::ACTION_MENU) {
		this->changeStoryPage(canvas, -1);
		if (canvas->state == Canvas::ST_CHARACTER_SELECTION) {
			canvas->stateVars[0] = app->player->characterChoice;
		}
		else {
			canvas->stateVars[0] = 0;
		}
	}
}


void StoryRenderer::playIntroMovie(Canvas* canvas, Graphics* graphics) {
	if (app->canvas->skipIntro) {
		canvas->backToMain(true);
		return;
	}

	if (app->game->hasSeenIntro && app->game->skipMovie) {
		this->exitIntroMovie(canvas, false);
		return;
	}

	if (canvas->stateVars[1] == 0) {  // load table camera
		canvas->stateVars[1] = app->gameTime;
		app->game->loadTableCamera(14, 15);
		canvas->numEvents = 0;
		canvas->keyDown = false;
		canvas->keyDownCausedMove = false;
		canvas->ignoreFrameInput = 1;
		this->imgProlog = app->loadImage("prolog.bmp", true);
	}

	if (app->game) {
		if (app->game->mayaCameras)
		{
			if (canvas->stateVars[3] == 0) { // init movie prolog
				app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_GENERAL), 1, 6, false);
				if (canvas->stateVars[1] < app->gameTime) {
					app->game->activeCameraKey = -1;
					canvas->stateVars[3] = 0;
					canvas->stateVars[1] = app->gameTime;
					canvas->stateVars[2] = 0;
					canvas->stateVars[0] = 0;
					canvas->fadeRect = canvas->displayRect;
					canvas->fadeFlags = Canvas::FADE_FLAG_FADEIN;
					canvas->fadeColor = 0;
					canvas->fadeTime = app->time;
					canvas->fadeDuration = 1500;
					canvas->stateVars[3] = 1;
				}
			}
			else if (canvas->stateVars[3] == 1) { // draw movie prolog
				if (canvas->displayRect[3] > 220) {
					graphics->clipRect(0, (canvas->displayRect[3] - 220) / 2, canvas->displayRect[2], 220);
				}

				canvas->stateVars[0] = app->gameTime - app->game->activeCameraTime;
				canvas->stateVars[2] = app->gameTime - canvas->stateVars[1];

				app->game->mayaCameras->Update(app->game->activeCameraKey, canvas->stateVars[0]);

				int shiftAmount = app->game->posShift;
				int texW = canvas->displayRect[2];
				int texH = canvas->displayRect[3];

				int posX = app->game->mayaCameras->x >> (shiftAmount & 0xff);
				int posY = app->game->mayaCameras->y >> (shiftAmount & 0xff);

				int texX = posX - canvas->SCR_CX;
				if (texX < 0) {
					texX = 0;
				}

				int texY = posY - canvas->SCR_CY;
				if (texY < 0) {
					texY = 0;
				}

				posY = canvas->SCR_CY - posY;
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
					canvas->stateVars[3] += 1;
				}
			}
			else if (canvas->stateVars[3] == 2) { // init Scrolling Text
				this->initScrollingText(canvas, 0, 133, false, 32, 1, 800);
				this->drawScrollingText(canvas, graphics);
				canvas->stateVars[3] += 1;
			}
			else if (canvas->stateVars[3] == 3) { // draw Scrolling Text
				this->drawScrollingText(canvas, graphics);
				if (this->scrollingTextDone) {
					this->exitIntroMovie(canvas, false);
				}
			}

			canvas->staleView = true;
			return;
		}
	}
}


void StoryRenderer::exitIntroMovie(Canvas* canvas, bool skipExit) {
	app->game->cleanUpCamMemory();

	this->imgProlog->~Image();
	this->imgProlog = nullptr;

	if (canvas->dialogBuffer) {
		canvas->dialogBuffer->dispose();
		canvas->dialogBuffer = nullptr;
	}

	app->sound->soundStop();
	if (!skipExit) {
		app->game->hasSeenIntro = true;
		app->game->saveConfig();
		canvas->backToMain(false);
	}
}
