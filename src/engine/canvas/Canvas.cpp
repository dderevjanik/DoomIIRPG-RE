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

Canvas::Canvas() = default;

Canvas::~Canvas() {
	delete[] this->dialogStyleDefs;
}

void Canvas::registerStateHandler(int stateId, ICanvasState* handler) {
	if (stateId >= 0 && stateId < MAX_STATE_ID) {
		stateHandlers[stateId] = handler;
	}
}

bool Canvas::isLoaded;

bool Canvas::startup() {
	this->app = CAppContainer::getInstance()->app;
	this->gameConfig = &CAppContainer::getInstance()->gameConfig;
	this->headless = this->headless;
	this->sdlGL = this->sdlGL;
	this->graphics.app = this->app;
	Applet* app = this->app;
	int viewWidth, viewHeight;
	fmButton* button;

	LOG_INFO("[canvas] startup\n");

	this->displayRect[0] = 0;
	this->displayRect[1] = 0;
	this->displayRect[2] = app->backBuffer->width;
	this->displayRect[3] = app->backBuffer->height;

	//printf("this->displayRect[0] %d\n", this->displayRect[0]);
	//printf("this->displayRect[1] %d\n", this->displayRect[1]);
	//printf("this->displayRect[2] %d\n", this->displayRect[2]);
	//printf("this->displayRect[3] %d\n", this->displayRect[3]);

	this->graphics.setGraphics();
	this->specialLootIcon = -1;
	this->pacLogoTime = -1;
	this->vibrateEnabled = true;
	this->loadMapStringID = -1;
	this->startupMap = 1;
	this->loadType = 0;
	this->saveType = 0;
	this->st_count = 0;
	this->knockbackDist = 0;
	this->numHelpMessages = 0;
	this->destZ = 36;
	this->viewZ = 36;
	this->screenRect[2] = 0;
	this->screenRect[3] = 0;
	this->dialogThread = nullptr;
	this->ignoreFrameInput = false;
	this->blockInputTime = 0;
	this->showLocation = false;
	this->lastPacifierUpdate = 0;
	this->numEvents = 0;
	this->dialogItem = nullptr;
	this->dialogViewLines = 0;
	this->lastMapID = 0;
	this->loadMapID = 0;
	this->automapDrawn = false;

	this->displayRect[2] &= 0xFFFFFFFE;
	this->screenRect[2] = this->displayRect[2];
	this->screenRect[3] = this->displayRect[3];

	this->dialogMaxChars = (this->displayRect[2] - DIALOG_PADDING) / TEXT_CHAR_WIDTH;
	this->scrollMaxChars = (this->displayRect[2] - DIALOG_PADDING) / TEXT_CHAR_WIDTH;
	this->dialogWithBarMaxChars		= (this->displayRect[2] - SCROLLBAR_PADDING) / TEXT_CHAR_WIDTH;
	this->scrollWithBarMaxChars		= (this->displayRect[2] - SCROLLBAR_PADDING) / TEXT_CHAR_WIDTH;
	this->menuScrollWithBarMaxChars = (this->displayRect[2] - SCROLLBAR_PADDING) / TEXT_CHAR_WIDTH;
	this->ingameScrollWithBarMaxChars = (this->displayRect[2] - INGAME_SCROLLBAR_PADDING) / TEXT_CHAR_WIDTH;
	this->menuHelpMaxChars = (this->displayRect[2] - HELP_PADDING) / TEXT_CHAR_WIDTH;
	this->subtitleMaxChars = this->displayRect[2] / TEXT_CHAR_WIDTH;

	if (app->hud->startup()) {

		int n2 = this->screenRect[3] - STATUS_BAR_HEIGHT - STATUS_BAR_HEIGHT;
		if (this->displayRect[3] >= 128) {
			this->displaySoftKeys = true;
			this->softKeyY = this->displayRect[3] - 0;
			if (this->displayRect[3] - this->screenRect[3] < 0) {
				n2 -= 0 - (this->displayRect[3] - this->screenRect[3]);
			}
		}
		else {
			this->softKeyY = this->displayRect[3];
		}

		int n3 = n2 & 0xFFFFFFFE;
		this->screenRect[3] = n3 + STATUS_BAR_HEIGHT + STATUS_BAR_HEIGHT;
		this->screenRect[0] = (this->displayRect[2] - this->screenRect[2]) / 2;
		if (this->displaySoftKeys) {
			this->screenRect[1] = (this->softKeyY - this->screenRect[3]) / 2;
		}
		else {
			this->screenRect[1] = (this->displayRect[3] - this->screenRect[3]) / 2;
		}

		this->SCR_CX = this->screenRect[2] / 2;
		this->SCR_CY = this->screenRect[3] / 2;
		this->viewRect[0] = this->screenRect[0];
		this->viewRect[1] = VIEW_RECT_Y;//this->screenRect[1] + STATUS_BAR_HEIGHT;
		this->viewRect[2] = this->screenRect[2];
		this->viewRect[3] = n3;

		// custom
		viewWidth = 0;
		viewHeight = 0;
		if (viewWidth != 0 && viewHeight != 0) {
			this->viewRect[0] += ((this->viewRect[2] - viewWidth) >> 1);
			this->viewRect[1] += ((this->viewRect[3] - viewHeight) >> 1);
			this->viewRect[2] = viewWidth;
			this->viewRect[3] = viewHeight;
		}

		this->hudRect[0] = this->displayRect[0];
		this->hudRect[1] = this->screenRect[1];
		this->hudRect[2] = this->displayRect[2];
		this->hudRect[3] = this->screenRect[3];

		this->menuRect[0] = this->displayRect[0];
		this->menuRect[1] = this->displayRect[1];
		this->menuRect[2] = this->displayRect[2];
		this->menuRect[3] = this->screenRect[3];

		this->CAMERAVIEW_BAR_HEIGHT = 20;
		
		this->cinRect[0] = this->viewRect[0];
		this->cinRect[1] = 42;
		this->cinRect[2] = this->viewRect[2];
		this->cinRect[3] = this->viewRect[3];

		if (this->screenRect[1] + this->screenRect[3] == this->softKeyY + -1) {
			this->softKeyY = this->screenRect[1] + this->screenRect[3];
		}

		this->setAnimFrames(10);

		this->startupMap = 1;
		this->skipIntro = CAppContainer::getInstance()->skipIntro;
		this->tellAFriend = false;

		app->beginImageLoading();
		this->imgDialogScroll = app->loadImage("DialogScroll.bmp", true);
		this->imgFabricBG = app->loadImage("FabricBG.bmp", true);
		this->imgFont = app->loadImage("Font.bmp", true);
		this->imgEndOfLevelStatsBG = app->loadImage("endOfLevelStatsBG.bmp", true);
		this->imgGameHelpBG = app->loadImage("gameHelpBG.bmp", true);
		this->imgIcons_Buffs = app->loadImage("Icons_Buffs.bmp", true);
		this->imgInventoryBG = app->loadImage("inventoryBG.bmp", true);
		this->imgLoadingFire = app->loadImage("loadingFire.bmp", true);
		this->imgFont_16p_Light = app->loadImage("Font_16p_Light.bmp", true);
		this->imgFont_16p_Dark = app->loadImage("Font_16p_Dark.bmp", true);
		this->imgFont_18p_Light = app->loadImage("Font_18p_Light.bmp", true);
		this->imgWarFont = app->loadImage("WarFont.bmp", true);
		this->fontRenderMode = 0;
		app->endImageLoading();

		this->lootSource = -1;
		this->m_controlLayout = 2;
		this->isFlipControls = false;
		this->m_controlMode = 1;
		this->lastBacklightRefresh = 0;
		this->vibrateTime = 0;
		this->areSoundsAllowed = false;
		this->m_controlAlpha = 50;
		this->m_controlGraphic = 0;

		char* arrowsFiles[] = {
			"arrow-up.bmp",
			"greenArrow_up.bmp",
			"arrow-up_pressed.bmp",
			"greenArrow_up-pressed.bmp",
			"arrow-down.bmp",
			"greenArrow_down.bmp",
			"arrow-down_pressed.bmp",
			"greenArrow_down-pressed.bmp",
			"arrow-left.bmp",
			"greenArrow_left.bmp",
			"arrow-left_pressed.bmp",
			"greenArrow_left-pressed.bmp",
			"arrow-right.bmp",
			"greenArrow_right.bmp",
			"arrow-right_pressed.bmp",
			"greenArrow_right-pressed.bmp"
		};

		char** files = arrowsFiles;
		Image **imgArrows = this->imgArrows;
		for (int i = 0; i < 2; i++) {
			imgArrows[0] = app->loadImage(files[0], true);
			imgArrows[2] = app->loadImage(files[2], true);
			imgArrows[4] = app->loadImage(files[4], true);
			imgArrows[6] = app->loadImage(files[6], true);
			imgArrows[8] = app->loadImage(files[8], true);
			imgArrows[10] = app->loadImage(files[10], true);
			imgArrows[12] = app->loadImage(files[12], true);
			imgArrows[14] = app->loadImage(files[14], true);
			imgArrows++;
			files++;
		}

		this->imgDpad_default = app->loadImage("dpad_default.bmp", true);
		this->imgDpad_up_press = app->loadImage("dpad_up-press.bmp", true);
		this->imgDpad_down_press = app->loadImage("dpad_down-press.bmp", true);
		this->imgDpad_left_press = app->loadImage("dpad_left-press.bmp", true);
		this->imgDpad_right_press = app->loadImage("dpad_right-press.bmp", true);
		this->imgPageUP_Icon = app->loadImage("pageUP_Icon.bmp", true);
		this->imgPageDOWN_Icon = app->loadImage("pageDOWN_Icon.bmp", true);
		this->imgPageOK_Icon = app->loadImage("pageOK_Icon.bmp", true);
		this->imgSniperScope_Dial = app->loadImage("SniperScope_Dial.bmp", true);
		this->imgSniperScope_Knob = app->loadImage("SniperScope_Knob.bmp", true);

		this->touched = false;

		// Setup Sniper Scope Dial Scroll Button
		{
			this->m_sniperScopeDialScrollButton = new fmScrollButton(400, 67, this->imgSniperScope_Dial->width, this->imgSniperScope_Dial->height, true, 1113);
			this->m_sniperScopeDialScrollButton->SetScrollBox(0, 0, 1, 1, 16);
			this->m_sniperScopeDialScrollButton->field_0x0_ = 1;
		}

		// Setup Sniper Scope Buttons
		{
			this->m_sniperScopeButtons = new fmButtonContainer();
			static const ButtonDef sniperDef = {.id = 6, .x = 122, .y = 20, .w = 236, .h = 236, .soundResID = -1};
			createButtons(this->m_sniperScopeButtons, std::span(&sniperDef, 1));
		}

		// Setup Control Buttons
		{
			fmButtonContainer** m_controlButtons = this->m_controlButtons;
			for (int i = 0; i < 2; i++) {
				m_controlButtons[0] = new fmButtonContainer();
				m_controlButtons[2] = new fmButtonContainer();
				m_controlButtons[4] = new fmButtonContainer();

				if (i == 1) {
					int v53 = 5;
					int v49 = 117;
					while (1)
					{
						button = new fmButton(5, v53, v53 + 116, 13, v49, -1);
						//button->drawTouchArea = true; // Test
						m_controlButtons[2]->AddButton(button);
						button = new fmButton(7, 140 - v53, v53 + 116, 13, v49, -1);
						//button->drawTouchArea = true; // Test
						m_controlButtons[2]->AddButton(button);
						button = new fmButton(3, v53 + 13, v53 + 103, v49, 13, -1);
						//button->drawTouchArea = true; // Test
						m_controlButtons[0]->AddButton(button);
						button = new fmButton(9, v53 + 13, 243 - v53, v49, 13, -1);
						//button->drawTouchArea = true; // Test
						m_controlButtons[0]->AddButton(button);
						v49 -= 26;
						if (v53 == 44)
							break;
						v53 += 13;
					}
					button = new fmButton(6, 153, 25, 322, 226, -1);
					m_controlButtons[2]->AddButton(button);
					this->m_swipeArea[1] = new fmSwipeArea(0, 0, this->screenRect[2], this->screenRect[3] - 64, 50, 50);
				}
				else {
					button = new fmButton(3, 42, 31, 70, 70, -1);
					button->SetImage(&this->imgArrows[0], 0, true);
					button->SetHighlightImage(&this->imgArrows[2], 0, true);
					button->normalRenderMode = 13;
					button->highlightRenderMode = 13;
					m_controlButtons[0]->AddButton(button);
					button = new fmButton(9, 42, 181, 70, 70, -1);
					button->SetImage(&this->imgArrows[4], 0, true);
					button->SetHighlightImage(&this->imgArrows[6], 0, true);
					button->normalRenderMode = 13;
					button->highlightRenderMode = 13;
					m_controlButtons[0]->AddButton(button);
					button = new fmButton(5, 5, 106, 70, 70, -1);
					button->SetImage(&this->imgArrows[8], 0, true);
					button->SetHighlightImage(&this->imgArrows[10], 0, true);
					button->normalRenderMode = 13;
					button->highlightRenderMode = 13;
					m_controlButtons[2]->AddButton(button);
					button = new fmButton(7, 80, 106, 70, 70, -1);
					button->SetImage(&this->imgArrows[12], 0, true);
					button->SetHighlightImage(&this->imgArrows[14], 0, true);
					button->normalRenderMode = 13;
					button->highlightRenderMode = 13;
					m_controlButtons[2]->AddButton(button);
					button = new fmButton(6, 155, 25, 320, 226, -1);
					m_controlButtons[2]->AddButton(button);
					button = new fmButton(6, 475, 172, 0, 79, -1);
					m_controlButtons[2]->AddButton(button);
					this->m_swipeArea[0] = new fmSwipeArea(0, 0, this->screenRect[2], this->screenRect[3] - 64, 50, 50);
				}

				m_controlButtons++;
			}
		}

		// Setup Character Buttons
		{
			this->m_characterButtons = new fmButtonContainer();
			ButtonDef charDefs[] = {
				{.id = 0, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
				{.id = 1, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
				{.id = 2, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
				{.id = 3, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
				{.id = 4, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
			};
			createButtons(this->m_characterButtons, charDefs);
		}

		// Setup Dialog Buttons
		{
			this->m_dialogButtons = new fmButtonContainer();
			ButtonDef dialogBaseDefs[] = {
				{.id = 0, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
				{.id = 1, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
				{.id = 2, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
				{.id = 3, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
				{.id = 4, .x = 0, .y = 0, .w = 0, .h = 0, .soundResID = 1027},
			};
			createButtons(this->m_dialogButtons, dialogBaseDefs);
			button = new fmButton(5, 390, 20, 90, 90, 1027);
			button->SetImage(this->imgPageUP_Icon, true);
			button->SetHighlightImage(this->imgPageUP_Icon, true);
			button->normalRenderMode = 12;
			button->highlightRenderMode = 0;
			this->m_dialogButtons->AddButton(button);
			button = new fmButton(6, 390, 110, 90, 90, 1027);
			button->SetImage(this->imgPageDOWN_Icon, true);
			button->SetHighlightImage(this->imgPageDOWN_Icon, true);
			button->normalRenderMode = 12;
			button->highlightRenderMode = 0;
			this->m_dialogButtons->AddButton(button);
			button = new fmButton(7, 390, 110, 90, 90, 1027);
			button->SetImage(this->imgPageOK_Icon, true);
			button->SetHighlightImage(this->imgPageOK_Icon, true);
			button->normalRenderMode = 12;
			button->highlightRenderMode = 0;
			this->m_dialogButtons->AddButton(button);
			button = new fmButton(8, 0, 0, 0, 0, 1027);
			this->m_dialogButtons->AddButton(button);
		}

		// Setup SoftKey Buttons
		{
			this->m_softKeyButtons = new fmButtonContainer();
			static const ButtonDef softKeyDefs[] = {
				{.id = 19, .x = 0, .y = 250, .w = 100, .h = 70, .soundResID = 1027},
				{.id = 20, .x = 380, .y = 250, .w = 100, .h = 70, .soundResID = 1027},
			};
			createButtons(this->m_softKeyButtons, softKeyDefs);
		}

		// Setup Mixing Buttons
		{
			this->m_mixingButtons = new fmButtonContainer();
		}

		// Setup Story Buttons
		{
			this->m_storyButtons = new fmButtonContainer();
			static const ButtonDef storyDefs[] = {
				{0, 0,   280, 60,  40, 1027}, // Back
				{1, 380, 280, 100, 40, 1027}, // Next
				{2, 420, 0,   60,  40, 1027}, // Skip
			};
			createButtons(this->m_storyButtons, storyDefs);
		}

		// Setup TreadMill Buttons
		{
			this->imgBootL = app->loadImage("bootL.bmp", true);
			this->imgBootR = app->loadImage("bootR.bmp", true);
			this->m_treadmillButtons = new fmButtonContainer();
			int w = imgBootL->width;
			int h = imgBootL->height;
			int x = 240 - (2 * w);
			int y = 160 - (h / 2);
			button = new fmButton(0, x, y, w, h, 1027);
			button->SetImage(this->imgBootL, true);
			button->SetHighlightImage(this->imgBootL, true);
			button->normalRenderMode = 12;
			button->highlightRenderMode = 0;
			this->m_treadmillButtons->AddButton(button);
			button = new fmButton(1, x + 3 * w, y, w, h, 1027);
			button->SetImage(this->imgBootR, true);
			button->SetHighlightImage(this->imgBootR, true);
			button->normalRenderMode = 12;
			button->highlightRenderMode = 0;
			this->m_treadmillButtons->AddButton(button);
		}

		return true;
	}

	return false;
}

void Canvas::flushGraphics() {
	if (this->headless) { return; }
	this->graphics.resetScreenSpace();
	this->backPaint(&this->graphics);
}

void Canvas::backPaint(Graphics* graphics) {


	graphics->clearClipRect();

	if (this->repaintFlags & Canvas::REPAINT_CLEAR) {
		this->repaintFlags &= ~Canvas::REPAINT_CLEAR;
		graphics->eraseRgn(this->displayRect);
	}

	if (this->repaintFlags & Canvas::REPAINT_VIEW3D) {
		if (app->render->_gles->isInit) {
			this->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
			if (app->render->isFading()) {
				app->render->fadeScene(graphics);
			}
		}
		else {
			//this->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
			//app->render->Render3dScene();
			app->render->drawRGB(graphics);
		}
	}

	if (this->repaintFlags & Canvas::REPAINT_PARTICLES) {
		this->repaintFlags &= ~Canvas::REPAINT_PARTICLES;
		app->particleSystem->renderSystems(graphics);
	}

	if (this->state == Canvas::ST_COMBAT || this->state == Canvas::ST_PLAYING) {
		this->m_swipeArea[this->m_controlMode]->Render(graphics);
	}

	if (((this->repaintFlags & Canvas::REPAINT_HUD) != 0) && (this->state != Canvas::ST_MENU)) { // REPAINT_HUD
		//this->repaintFlags &= ~Canvas::REPAINT_HUD; java, brew only
		app->hud->draw(graphics);
	}

	if (app->player->inTargetPractice) {
		this->drawTargetPracticeScore(graphics);
	}

	// Dispatch render to registered state handler
	if (this->state >= 0 && this->state < MAX_STATE_ID && stateHandlers[this->state]) {
		stateHandlers[this->state]->render(this, graphics);
	}

	/*if ((this->repaintFlags & Canvas::REPAINT_SOFTKEYS) != 0x0) { // REPAINT_SOFTKEYS
		this->repaintFlags &= ~Canvas::REPAINT_SOFTKEYS;
		this->drawSoftKeys(graphics);
	}*/

	if (this->repaintFlags & Canvas::REPAINT_MENU) {
		this->repaintFlags &= ~Canvas::REPAINT_MENU;
		app->menuSystem->paint(graphics);
	}

	if (this->repaintFlags & Canvas::REPAINT_STARTUP_LOGO) {
		this->repaintFlags &= ~Canvas::REPAINT_STARTUP_LOGO;
		graphics->fillRect(0, 0, this->displayRect[2], this->displayRect[3], -0x1000000);
		graphics->drawImage(this->imgStartupLogo, this->displayRect[2] / 2, this->displayRect[3] / 2, 3, 0, 0);
	}

	//printf("this->repaintFlags %d\n", this->repaintFlags);
	if (this->repaintFlags & Canvas::REPAINT_LOADING_BAR) { // REPAINT_LOADING_BAR
		this->repaintFlags &= ~Canvas::REPAINT_LOADING_BAR;
		this->drawLoadingBar(graphics);
	}

	if (this->fadeFlags && (app->time < this->fadeTime + this->fadeDuration)) {
		int alpha = ((app->time - this->fadeTime) << 8) / this->fadeDuration;

		if ((this->fadeFlags & Canvas::FADE_FLAG_FADEOUT) != 0) {
			alpha = 256 - alpha;
		}

		graphics->fade(this->fadeRect, alpha, this->fadeColor);
	}
	else {
		this->fadeFlags = Canvas::FADE_FLAG_NONE;
	}

	if (this->state == Canvas::ST_BENCHMARK) {
		if (this->st_enabled) {
			int n = this->viewRect[1];
			this->debugTime = app->upTimeMs;
			Text* largeBuffer = app->localization->getLargeBuffer();
			largeBuffer->setLength(0);
			largeBuffer->append("Rndr ms: ");
			largeBuffer->append(this->st_fields[0] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[0] * 100 / this->st_count - this->st_fields[0] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			largeBuffer->setLength(0);
			largeBuffer->append("Bsp ms: ");
			largeBuffer->append(this->st_fields[1] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[1] * 100 / this->st_count - this->st_fields[1] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			largeBuffer->setLength(0);
			largeBuffer->append("Hud ms: ");
			largeBuffer->append(this->st_fields[2] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[2] * 100 / this->st_count - this->st_fields[2] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			int n2 = this->st_fields[4] + this->st_fields[5];
			largeBuffer->setLength(0);
			largeBuffer->append("Blit ms: ");
			largeBuffer->append(n2 / this->st_count)->append('.');
			largeBuffer->append(n2 * 100 / this->st_count - n2 / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			largeBuffer->setLength(0);
			largeBuffer->append("Paus ms: ");
			largeBuffer->append(this->st_fields[6] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[6] * 100 / this->st_count - this->st_fields[6] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			largeBuffer->setLength(0);
			largeBuffer->append("Dbg ms: ");
			largeBuffer->append(this->st_fields[9] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[9] * 100 / this->st_count - this->st_fields[9] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			largeBuffer->setLength(0);
			largeBuffer->append("Loop ms: ");
			largeBuffer->append(this->st_fields[7] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[7] * 100 / this->st_count - this->st_fields[7] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			largeBuffer->setLength(0);
			largeBuffer->append("Key ms: ");
			largeBuffer->append(this->st_fields[11] - this->st_fields[10]);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			largeBuffer->setLength(0);
			largeBuffer->append("State ms: ");
			largeBuffer->append(this->st_fields[12] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[12] * 100 / this->st_count - this->st_fields[12] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			largeBuffer->setLength(0);
			largeBuffer->append("Totl ms: ");
			largeBuffer->append(this->st_fields[8] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[8] * 100 / this->st_count - this->st_fields[8] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			largeBuffer->setLength(0);
			largeBuffer->append(this->st_count);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += 16;
			this->debugTime = app->upTimeMs - this->debugTime;
			largeBuffer->dispose();
		}
	}
	else if (this->state == Canvas::ST_CAMERA || this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_COMBAT || this->state == Canvas::ST_INTER_CAMERA) {
		int n3 = this->viewRect[1];
		if (this->showSpeeds) {
			int lastRenderTime = this->afterRender - this->beforeRender;
			if (this->lastFrameTime == app->time) {
				this->afterRender = (this->beforeRender = 0);
				this->lastRenderTime = lastRenderTime;
			}
			int n4 = app->time - this->totalFrameTime;
			this->totalFrameTime = app->time;
			Text* largeBuffer2 = app->localization->getLargeBuffer();
			largeBuffer2->setLength(0);
			largeBuffer2->append("ms: ");
			largeBuffer2->append(this->lastRenderTime)->append('/');
			largeBuffer2->append(app->render->clearColorBuffer)->append('/');
			largeBuffer2->append(app->render->bltTime)->append('/');
			largeBuffer2->append(n4);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			n3 += 16;
			largeBuffer2->setLength(0);
			largeBuffer2->append("li: ");
			largeBuffer2->append(app->render->lineRasterCount)->append('/');
			largeBuffer2->append(app->render->lineCount);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			n3 += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer2->setLength(0);
			largeBuffer2->append("sp: ");
			largeBuffer2->append(app->render->spriteRasterCount)->append('/');
			largeBuffer2->append(app->render->spriteCount)->append('/');
			largeBuffer2->append(app->render->numMapSprites);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			n3 += Applet::FONT_HEIGHT[app->fontType];
			if (app->render->renderMode == 63) {
				largeBuffer2->setLength(0);
				largeBuffer2->append("cnt: ");
				largeBuffer2->append(app->tinyGL->spanCalls)->append('/');
				largeBuffer2->append(app->tinyGL->spanPixels);
				graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
				n3 += Applet::FONT_HEIGHT[app->fontType];
				largeBuffer2->setLength(0);
				largeBuffer2->append("tris: ");
				largeBuffer2->append(app->tinyGL->countBackFace)->append('/');
				largeBuffer2->append(app->tinyGL->countDrawn);
				graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
				n3 += Applet::FONT_HEIGHT[app->fontType];
			}
			largeBuffer2->setLength(0);
			largeBuffer2->append("OSTime: ");
			int v30 = 0;
			for (int i = 0; i < 8; i++) {
				v30 += app->osTime[i];
			}
			largeBuffer2->append(v30 / 8);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			n3 += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer2->setLength(0);
			largeBuffer2->append("Code: ");
			int v31 = 0;
			for (int i = 0; i < 8; i++) {
				v31 += app->codeTime[i];
			}
			largeBuffer2->append(v31 / 8);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			largeBuffer2->dispose();
			n3 += Applet::FONT_HEIGHT[app->fontType];
		}

		if (this->showLocation) {
			Text* smallBuffer = app->localization->getSmallBuffer();
			smallBuffer->setLength(0);
			smallBuffer->append(this->viewX >> 6);
			smallBuffer->append(' ');
			smallBuffer->append(this->viewY >> 6);
			smallBuffer->append(' ');
			int angle = this->viewAngle & 0x3FF;
			if (angle == Enums::ANGLE_NORTH) {
				smallBuffer->append('N');
			}
			else if (angle == Enums::ANGLE_EAST) {
				smallBuffer->append('E');
			}
			else if (angle == Enums::ANGLE_SOUTH) {
				smallBuffer->append('S');
			}
			else if (angle == Enums::ANGLE_WEST) {
				smallBuffer->append('W');
			}
			else if (angle == Enums::ANGLE_NORTHEAST) {
				smallBuffer->append("NE");
			}
			else if (angle == Enums::ANGLE_NORTHWEST) {
				smallBuffer->append("NW");
			}
			else if (angle == Enums::ANGLE_SOUTHEAST) {
				smallBuffer->append("SE");
			}
			else if (angle == Enums::ANGLE_SOUTHWEST) {
				smallBuffer->append("SW");
			}
			graphics->drawString(smallBuffer, this->viewRect[0], n3, 0);
			smallBuffer->dispose();
		}
	}

	graphics->resetScreenSpace();
	if (this->showFreeHeap) {
		graphics->setColor(0xFF000000);
		graphics->fillRect(this->viewRect[0], this->viewRect[1] + this->viewRect[3] - 18, this->viewRect[2], 16);
		switch (this->updateChar) {
		case '*': {
			this->updateChar = '+';
			break;
		}
		case '+': {
			this->updateChar = '%';
			break;
		}
		case '%': {
			this->updateChar = '#';
			break;
		}
		case '#': {
			this->updateChar = '*';
			break;
		}
		}
		Text *largeBuffer3 = app->localization->getLargeBuffer();
		if (largeBuffer3 != nullptr) {
			largeBuffer3->append(this->updateChar);
			largeBuffer3->append(" ");
			largeBuffer3->append(1000000000/*App.getFreeMemory()*/);
			graphics->drawString(largeBuffer3, this->SCR_CX, this->viewRect[1] + this->viewRect[3] - 16, 17);
			largeBuffer3->setLength(0);
			for (int i = 0; i < 10; ++i) {
				largeBuffer3->append(app->game->numLevelLoads[i]);
				largeBuffer3->append('|');
			}
			graphics->drawString(largeBuffer3, this->viewRect[0], this->viewRect[1], 4);
			largeBuffer3->setLength(0);
			int t = app->game->totalPlayTime + (app->upTimeMs - app->game->lastSaveTime) / 1000; //[GEC] hace que el tiempo avance
			int n6 = t / 3600;
			int n7 = (t - n6 * 3600) / 60;
			int n8 = (t - n6 * 3600) - n7 * 60;
			largeBuffer3->append(n6);
			largeBuffer3->append(':');
			if (n7 < 10) {
				largeBuffer3->append(0);
			}
			largeBuffer3->append(n7);
			largeBuffer3->append(':');
			if (n8 < 10) {
				largeBuffer3->append(0);
			}
			largeBuffer3->append(n8);
			graphics->drawString(largeBuffer3, this->viewRect[0] + this->viewRect[2] - 3, this->viewRect[1], 8);
			largeBuffer3->dispose();
		}
	}
}

void Canvas::run() {

	app->CalcAccelerometerAngles();

	int upTimeMs = app->upTimeMs;
	app->lastTime = app->time;
	app->time = upTimeMs;

	if (this->st_enabled != false) {
		this->st_count = this->st_count + 1;
		this->st_fields[0] = this->st_fields[0] + app->render->frameTime;
		this->st_fields[1] = this->st_fields[1] + app->render->bspTime;
		this->st_fields[2] = this->st_fields[2] + app->hud->drawTime;
		this->st_fields[4] = this->st_fields[4] + app->render->bltTime;
		this->st_fields[5] = (this->pauseTime - this->flushTime) + this->st_fields[5];
		this->st_fields[6] = (this->loopEnd - this->pauseTime) + this->st_fields[6];
		this->st_fields[8] = (app->time - app->lastTime) + this->st_fields[8];
		this->st_fields[3] = this->st_fields[3] + app->combat->renderTime;
		this->st_fields[9] = this->st_fields[9] + this->debugTime;
		this->st_fields[7] = (this->loopEnd - this->loopStart) + this->st_fields[7];
		upTimeMs = app->upTimeMs;
	}
	this->loopStart = upTimeMs;

	if (!app->game->pauseGameTime && this->state != Canvas::ST_MENU) {
		app->gameTime += app->time - app->lastTime;
	}

	if (this->vibrateTime && this->vibrateTime < app->time) {
		this->vibrateTime = 0;
	}

	if (this->state != Canvas::ST_DIALOG && this->state != Canvas::ST_MENU && app->game->isInputBlockedByScript()) {
		this->clearEvents(1);
	}

	this->runInputEvents();

	// [GEC]
	if (!this->headless && (this->repaintFlags & Canvas::REPAINT_VIEW3D)) { // REPAINT_VIEW3D
		if (!app->render->_gles->isInit) {
			app->render->Render3dScene();
			app->tinyGL->applyClearColorBuffer();
		}
	}

	if ((this->state != Canvas::ST_MENU) || (app->menuSystem->menu != Menus::MENU_ENABLE_SOUNDS)) {
		app->game->numTraceEntities = 0;
		app->game->UpdatePlayerVars();
		app->game->gsprite_update(app->time);
		app->game->runScriptThreads(app->gameTime);
	}

	//printf("lastTime %d\n", app->lastTime);
	//printf("time %d\n", app->time);
	//printf("this->loopStart %d\n", this->loopStart);
	int time = app->upTimeMs;
	app->game->updateAutomap = false;
	//printf("this->state %d\n", this->state);

	// Dispatch to registered state handler
	if (this->state >= 0 && this->state < MAX_STATE_ID && stateHandlers[this->state]) {
		stateHandlers[this->state]->update(this);
	}
	// ST_SAVING — inline (needs early return from run)
	else if (this->state == Canvas::ST_SAVING) {
		if ((this->saveType & 0x2) != 0x0 || (this->saveType & 0x1) != 0x0) {
			if ((this->saveType & 0x8) != 0x0) {
				if (app->game->spawnParam != 0) {
					int n11 = 32 + ((app->game->spawnParam & 0x1F) << 6);
					int n12 = 32 + ((app->game->spawnParam >> 5 & 0x1F) << 6);
					app->game->saveState(this->lastMapID, this->loadMapID, n11, n12, (app->game->spawnParam >> 10 & 0xFF) << 7, 0, n11, n12, n11, n12, 36, 0, 0, this->saveType);
				}
				else {
					app->game->saveState(this->lastMapID, this->loadMapID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, this->saveType);
				}
			}
			else if ((this->saveType & 0x10) != 0x0) {
				int n13 = 32 + ((app->game->spawnParam & 0x1F) << 6);
				int n14 = 32 + ((app->game->spawnParam >> 5 & 0x1F) << 6);
				app->game->saveState(this->loadMapID, app->menuSystem->LEVEL_STATS_nextMap, n13, n14, (app->game->spawnParam >> 10 & 0xFF) << 7, 0, n13, n14, n13, n14, 36, 0, 0, this->saveType);
			}
			else {
				app->game->saveState(this->loadMapID, this->loadMapID, this->destX, this->destY, this->destAngle, this->viewPitch, this->prevX, this->prevY, this->saveX, this->saveY, this->saveZ, this->saveAngle, this->savePitch, this->saveType);
			}
			app->hud->addMessage((short)0, (short)38);
		}
		else {
			app->Error(48); // ERR_SAVESTATE
		}
		if ((this->saveType & 0x4) != 0x0) {
			this->backToMain(false);
		}
		else if ((this->saveType & 0x40) != 0x0) {
			app->shutdown();
		}
		else if ((this->saveType & 0x8) != 0x0) {
			this->setState(Canvas::ST_TRAVELMAP);
		}
		else if ((this->saveType & 0x10) != 0x0) {
			app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_LEVEL_END), 1u, 3, saveType & 8);
			app->menuSystem->setMenu(Menus::MENU_LEVEL_STATS);
		}
		else if ((this->saveType & 0x100) != 0x0) {
			app->menuSystem->setMenu(Menus::MENU_END_FINALQUIT);
		}
		else {
			if ((this->saveType & 0x80) != 0x0) {
				app->menuSystem->returnToGame();
			}
			this->setState(Canvas::ST_PLAYING);
		}
		this->saveType = 0;
		this->clearEvents(1);
	}
	// ST_LOADING — inline (needs early return from run for loadMedia)
	else if (this->state == Canvas::ST_LOADING) {
		if (this->loadType == 0) {
			if (!this->loadMedia()) {
				this->flushGraphics();
				return;
			}
		}
		else {
			app->game->loadState(this->loadType);
			app->hud->addMessage((short)0, (short)39);
			this->loadType = 0;
		}
	}

	if (this->state == Canvas::ST_SAVING || this->state == Canvas::ST_LOADING) {
		this->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
	}
	this->st_fields[12] = app->upTimeMs - time;

	this->flushTime = app->upTimeMs;
	if (!this->headless) {
		this->graphics.resetScreenSpace();
		this->backPaint(&this->graphics);
	}
	if (this->keyPressedTime != 0) {
		this->lastKeyPressedTime = app->upTimeMs - this->keyPressedTime;
		this->keyPressedTime = 0;
	}
	this->pauseTime = app->upTimeMs;
	this->loopEnd = app->upTimeMs;
	this->stateChanged = false;

	if (this->sysSoundDelayTime > 0 && this->sysSoundDelayTime < app->upTimeMs - this->sysSoundTime) {
		app->sound->playSound((app->nextByte() & 0xF) + 1000, 0, 3, 0);
		this->sysSoundTime = app->upTimeMs;
	}
}

void Canvas::clearEvents(int ignoreFrameInput) {
	this->numEvents = 0;
	this->keyDown = false;
	this->keyDownCausedMove = false;
	this->ignoreFrameInput = ignoreFrameInput;
}

void Canvas::loadRuntimeData() {


	app->loadRuntimeImages();
	//app->checkPeakMemory("after loadRuntimeData");
}

void Canvas::freeRuntimeData() {

	app->freeRuntimeImages();
}

void Canvas::startShake(int i, int i2, int i3) {

	SDLGL* sdlGL = this->sdlGL;

	if (app->game->skippingCinematic) {
		return;
	}

	if (i2 != 0) {
		this->shakeTime = app->time + i;
		this->shakeIntensity = 2 * i2;
		this->staleTime += 1;
	}

	if (i3 != 0 && this->vibrateEnabled) {
		controllerVibrate(i3); // [GEC]
		this->vibrateTime = i3 + app->upTimeMs;
	}
}

static const char* canvasStateName(int state) {
	switch (state) {
		case Canvas::ST_MENU: return "MENU";
		case Canvas::ST_INTRO_MOVIE: return "INTRO_MOVIE";
		case Canvas::ST_PLAYING: return "PLAYING";
		case Canvas::ST_INTER_CAMERA: return "INTER_CAMERA";
		case Canvas::ST_COMBAT: return "COMBAT";
		case Canvas::ST_AUTOMAP: return "AUTOMAP";
		case Canvas::ST_LOADING: return "LOADING";
		case Canvas::ST_DIALOG: return "DIALOG";
		case Canvas::ST_INTRO: return "INTRO";
		case Canvas::ST_MINI_GAME: return "MINI_GAME";
		case Canvas::ST_DYING: return "DYING";
		case Canvas::ST_EPILOGUE: return "EPILOGUE";
		case Canvas::ST_CREDITS: return "CREDITS";
		case Canvas::ST_SAVING: return "SAVING";
		case Canvas::ST_CAMERA: return "CAMERA";
		case Canvas::ST_ERROR: return "ERROR";
		case Canvas::ST_TRAVELMAP: return "TRAVELMAP";
		case Canvas::ST_CHARACTER_SELECTION: return "CHARACTER_SELECTION";
		case Canvas::ST_LOOTING: return "LOOTING";
		case Canvas::ST_TREADMILL: return "TREADMILL";
		case Canvas::ST_BOT_DYING: return "BOT_DYING";
		case Canvas::ST_LOGO: return "LOGO";
		default: return "UNKNOWN";
	}
}

void Canvas::setState(int state) {

	LOG_INFO("[canvas] setState: {} -> {}\n", canvasStateName(this->state), canvasStateName(state));
	this->stateChanged = true;
	for (int i = 0; i < 9; ++i) {
		this->stateVars[i] = 0;
	}
	this->m_controlButtonIsTouched = false;
	this->m_controlButton = 0;

	// Call onExit for registered state handler
	if (this->state >= 0 && this->state < MAX_STATE_ID && stateHandlers[this->state]) {
		stateHandlers[this->state]->onExit(this);
	}

	this->oldState = this->state;
	this->state = state;

	//printf("state %d\n", state);
	// Inline enter setup for states without handlers (ST_LOADING, ST_SAVING)
	if (state == Canvas::ST_LOADING || state == Canvas::ST_SAVING) {
		this->repaintFlags &= ~Canvas::REPAINT_HUD;
		this->pacifierX = this->SCR_CX - 66;
		this->updateLoadingBar(false);
	}

	// Call onEnter for registered state handler
	if (state >= 0 && state < MAX_STATE_ID && stateHandlers[state]) {
		stateHandlers[state]->onEnter(this);
	}
}

void Canvas::setAnimFrames(int animFrames) {
	this->animFrames = animFrames;
	this->animPos = (64 + this->animFrames - 1) / this->animFrames;
	this->animAngle = (256 + this->animFrames - 1) / this->animFrames;
}

void Canvas::checkFacingEntity() {

	if (!this->updateFacingEntity) {
		return;
	}
	int destX = this->destX;
	int destY = this->destY;
	int destZ = this->destZ;
	int n = 21741;
	int *view = app->tinyGL->view;
	app->game->trace(destX + (-view[2] * 28 >> 14), destY + (-view[6] * 28 >> 14), destZ + (-view[10] * 28 >> 14), destX + (6 * -view[2] >> 8), destY + (6 * -view[6] >> 8), destZ + (6 * -view[10] >> 8), nullptr, n, 2, this->isZoomedIn);
	Entity* traceEntity = app->game->traceEntity;
	if (traceEntity != nullptr && traceEntity->def == nullptr) {
		traceEntity = nullptr;
	}
	if (traceEntity != nullptr && (traceEntity->def->eType == Enums::ET_ITEM || traceEntity->def->eType == Enums::ET_MONSTERBLOCK_ITEM || traceEntity->def->eType == Enums::ET_SPRITEWALL || traceEntity->def->eType == Enums::ET_ATTACK_INTERACTIVE || traceEntity->def->eType == Enums::ET_DECOR_NOCLIP)) {
		int i = 0;
		while (i < app->game->numTraceEntities) {
			Entity* entity = app->game->traceEntities[i];
			short linkIndex = entity->linkIndex;
			if (entity->def->eType == Enums::ET_MONSTER) {
				if (traceEntity->def->eType != Enums::ET_SPRITEWALL) {
					traceEntity = entity;
					break;
				}
				break;
			}
			else {
				if (entity->def->eType == Enums::ET_DOOR || entity->def->eType == Enums::ET_PLAYERCLIP) {
					break;
				}
				if (entity->def->eType == Enums::ET_WORLD) {
					break;
				}
				if (entity->def->eType == Enums::ET_SPRITEWALL && (app->render->mapFlags[linkIndex] & 0x2) != 0x0) {
					break;
				}
				if (entity->def->eType == Enums::ET_DECOR) {
					if (traceEntity->def->eType == Enums::ET_SPRITEWALL) {
						traceEntity = entity;
						break;
					}
					break;
				}
				else {
					if (entity->def->eType == Enums::ET_DECOR_NOCLIP) {
						if (traceEntity->def->eSubType != Enums::DECOR_DYNAMITE) {
							traceEntity = entity;
							break;
						}
					}
					else if (entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && (entity->def->eSubType == Enums::INTERACT_BARRICADE || entity->def->eSubType == Enums::INTERACT_CRATE || entity->def->eSubType == Enums::INTERACT_PICKUP) && traceEntity != nullptr && traceEntity->def->eType != Enums::ET_ITEM) {
						traceEntity = entity;
						break;
					}
					++i;
				}
			}
		}
	}
	app->player->facingEntity = traceEntity;
	if (app->player->facingEntity != nullptr) {
		Entity* facingEntity = app->player->facingEntity;
		int dist = facingEntity->distFrom(this->viewX, this->viewY);
		if (facingEntity->def->eType != Enums::ET_MONSTER && dist > app->combat->tileDistances[2]) {
			app->player->facingEntity = nullptr;
		}
		else if (facingEntity->def->eType == Enums::ET_NPC && dist <= app->combat->tileDistances[0]) {
			app->player->showHelp((short)0, false);
		}
		else if (dist <= app->combat->tileDistances[0]) {
			if (facingEntity->def->eType == Enums::ET_ATTACK_INTERACTIVE) {
				if (facingEntity->def->eSubType == Enums::INTERACT_BARRICADE) {
					app->player->showHelp((short)2, false);
				}
				else if (facingEntity->def->eSubType == Enums::INTERACT_CRATE) {
					app->player->showHelp((short)3, false);
				}
				else if (facingEntity->def->eSubType == Enums::INTERACT_PICKUP) {
					app->player->showHelp((short)9, false);
				}
			}
			else if (facingEntity->def->eType == Enums::ET_DOOR) {
				if (facingEntity->def->eSubType == Enums::DOOR_LOCKED) {
					app->player->showHelp((short)1, false);
				}
				app->player->showHelp((short)7, false);
			}
			else if (facingEntity->def->eType == Enums::ET_ITEM && facingEntity->def->eSubType == Enums::ITEM_KEY) {
				app->player->showHelp((short)4, false);
			}
			else if (facingEntity->def->tileIndex == 158) {
				app->player->showHelp((short)18, false);
			}
		}
	}
	Entity* traceEntity2 = app->game->traceEntity;
	int n2 = 4141;
	if (traceEntity2 != nullptr && (1 << traceEntity2->def->eType & n2) == 0x0) {
		for (int j = 1; j < app->game->numTraceEntities; ++j) {
			Entity* entity2 = app->game->traceEntities[j];
			if ((1 << entity2->def->eType & n2) != 0x0) {
				traceEntity2 = entity2;
				break;
			}
		}
	}
	if (traceEntity2 != nullptr) {
		int dist2 = traceEntity2->distFrom(this->viewX, this->viewY);
		if (dist2 <= app->combat->tileDistances[0] && traceEntity2->def->eType == Enums::ET_WORLD && app->combat->weaponDown) {
			app->combat->shiftWeapon(true);
		}
		else if ((this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_DIALOG) && ((0x2 & 1 << app->player->ce->weapon) == 0x0 || dist2 <= app->combat->tileDistances[0])) {
			if (traceEntity2->def->eType == Enums::ET_NPC) {
				app->combat->shiftWeapon(true);
			}
			else if (app->combat->weaponDown) {
				app->combat->shiftWeapon(false);
			}
		}
		else if (this->state == Canvas::ST_DIALOG && this->oldState != Canvas::ST_INTER_CAMERA) {
			app->combat->shiftWeapon(true);
		}
		else {
			app->combat->shiftWeapon(false);
		}
	}
	else {
		app->combat->shiftWeapon(false);
	}
	this->updateFacingEntity = false;
}

void Canvas::finishMovement() {


	if (this->gotoThread != nullptr && this->viewAngle == this->destAngle) {
		this->gotoThread->run();
		this->gotoThread = nullptr;
	}
	app->game->executeTile(this->destX >> 6, this->destY >> 6, this->flagForFacingDir(8), true);
	app->game->executeTile(this->destX >> 6, this->destY >> 6, app->game->eventFlags[1], true);
	app->game->touchTile(this->destX, this->destY, true);
	if (this->knockbackDist > 0) {
		--this->knockbackDist;
		this->destZ += 12;
		if (this->knockbackDist == 0) {
			this->destZ = 36 + app->render->getHeight(this->destX, this->destY);
		}
	}
	else if (this->gotoThread == nullptr && this->state == Canvas::ST_PLAYING && app->game->monstersTurn == 0) {
		//if (this->state != Canvas::ST_AUTOMAP) {
			this->updateFacingEntity = true;
		//}
		this->uncoverAutomap();
		app->game->advanceTurn();
	}
	else if (this->state == Canvas::ST_AUTOMAP) {
		this->uncoverAutomap();
		app->game->advanceTurn();
		if (app->game->animatingEffects != 0) {
			this->setState(Canvas::ST_PLAYING);
		}
		else {
			app->game->snapMonsters(true);
			app->game->snapLerpSprites(-1);
		}
	}
}

int Canvas::flagForWeapon(int i) {

	bool weaponIsASentryBot = app->player->weaponIsASentryBot(i);
	i = 1 << i;
	if (weaponIsASentryBot && (!app->player->isFamiliar || (app->player->familiarType != 1 && app->player->familiarType != 3))) {
		return 0;
	}
	if ((i & 0x2) != 0x0) {
		return 4096;
	}
	if ((i & 0x800) != 0x0) {
		return 16384;
	}
	return 8192;
}

int Canvas::flagForFacingDir(int i) {
	int destAngle = this->destAngle;
	if (i == 4) {
		destAngle += 512;
	}
	if (i == 8 || i == 4) {
		return i | 1 << ((destAngle & 0x3FF) >> 7) + 4;
	}
	return 0;
}

void Canvas::startRotation(bool b) {


	int8_t b2 = Canvas::viewStepValues[((this->destAngle & 0x3FF) >> 7 << 1) + 0];
	int8_t b3 = Canvas::viewStepValues[((this->destAngle & 0x3FF) >> 7 << 1) + 1];
	int n = 384;
	app->game->trace(this->destX, this->destY, this->destX + (b2 * n >> 6), this->destY + (b3 * n >> 6), nullptr, 4133, 2);
	Entity* traceEntity = app->game->traceEntity;
	int n2 = app->game->traceFracs[0] * n >> 14;
	int n3;
	int n4;
	if (traceEntity != nullptr && (traceEntity->def->eType == Enums::ET_WORLD || traceEntity->def->eType == Enums::ET_SPRITEWALL) && n2 <= 36) {
		n3 = this->destZ;
		n4 = (b ? 0 : 1);
	}
	else {
		bool b4 = !this->pitchIsControlled(this->destX >> 6, this->destY >> 6, app->game->VecToDir(b2 * 32, b3 * 32, false));
		if (traceEntity != nullptr && traceEntity->def->eType == Enums::ET_MONSTER) {
			if (b4) {
				int* calcPosition = traceEntity->calcPosition();
				n3 = app->render->getHeight(calcPosition[0], calcPosition[1]) + 36;
			}
			else {
				n3 = this->destZ;
			}
			n4 = 1;
		}
		else {
			n2 = 64;
			if (b4) {
				n3 = app->render->getHeight(this->destX + b2, this->destY + b3) + 36;
			}
			else {
				n3 = this->destZ;
			}
			n4 = (b ? 0 : 1);
		}
		n4 = 1;
	}
	if (n4 == 0) {
		return;
	}
	if (n2 == 0) {
		this->destPitch = 0;
	}
	else {
		this->destPitch = ((n3 - this->destZ) << 7) / n2;
	}
	if (this->destPitch < -64) {
		this->destPitch = -64;
	}
	else if (this->destPitch > 64) {
		this->destPitch = 64;
	}
	this->pitchStep = std::abs((this->destPitch - this->viewPitch) / this->animFrames);
}

void Canvas::finishRotation(bool b) {


	this->viewSin = app->render->sinTable[this->destAngle & 0x3FF];
	this->viewCos = app->render->sinTable[this->destAngle + 256 & 0x3FF];
	this->viewStepX = Canvas::viewStepValues[(((this->destAngle & 0x3FF) >> 7) << 1) + 0];
	this->viewStepY = Canvas::viewStepValues[(((this->destAngle & 0x3FF) >> 7) << 1) + 1];
	int n = this->destAngle - 256 & 0x3FF;
	this->viewRightStepX = Canvas::viewStepValues[((n >> 7) << 1) + 0];
	this->viewRightStepY = Canvas::viewStepValues[((n >> 7) << 1) + 1];
	if (b && app->hud->msgCount > 0 && (app->hud->messageFlags[0] & 0x2) != 0x0) {
		app->hud->msgTime = 0;
	}
	if (this->gotoThread != nullptr && this->viewX == this->destX && this->viewY == this->destY) {
		ScriptThread* gotoThread = this->gotoThread;
		this->gotoThread = nullptr;
		gotoThread->run();
	}
	if (this->state == Canvas::ST_COMBAT) {
		this->updateFacingEntity = true;
	}
	else {
		app->game->executeTile(this->destX >> 6, this->destY >> 6, this->flagForFacingDir(8), true);
		this->updateFacingEntity = true;
	}
}

/*int KEY_ARROWUP = -1;
int KEY_ARROWDOWN = -2;
int KEY_ARROWLEFT = -3;
int KEY_ARROWRIGHT = -4;
int KEY_OK = -5;
int KEY_CLR = -8;
int KEY_LEFTSOFT = -6;
int KEY_RIGHTSOFT = -7;
int KEY_BACK = -8;*/

#include "Input.h"

#define MOVEFORWARD	1
#define MOVEBACK	2
#define TURNLEFT	3
#define TURNRIGHT	4
#define MENUOPEN	5
#define SELECT		6
#define AUTOMAP		7
//8
#define MOVELEFT		9
#define MOVERIGHT		10
#define PREVWEAPON		11
#define NEXTWEAPON		12
//13
#define PASSTURN		14
//15
#define MENU_UP			16
#define MENU_DOWN		17
#define MENU_PAGE_UP	18
#define MENU_PAGE_DOWN	19
#define MENU_SELECT		20
#define MENU_OPEN		21

#define NUM_CODES 36
int keys_codeActions[NUM_CODES] = {
	AVK_CLR,		Enums::ACTION_BACK,
	AVK_SOFT2,		Enums::ACTION_AUTOMAP,
	AVK_SOFT1,		Enums::ACTION_MENU,
	// New items Only Port
	AVK_STAR,		Enums::ACTION_PREVWEAPON,
	AVK_POUND,		Enums::ACTION_AUTOMAP,
	AVK_NEXTWEAPON, Enums::ACTION_NEXTWEAPON,
	AVK_PREVWEAPON, Enums::ACTION_PREVWEAPON,
	AVK_AUTOMAP,	Enums::ACTION_AUTOMAP,
	AVK_UP,			Enums::ACTION_UP,
	AVK_DOWN,		Enums::ACTION_DOWN,
	AVK_LEFT,		Enums::ACTION_LEFT,
	AVK_RIGHT,		Enums::ACTION_RIGHT,
	AVK_MOVELEFT,	Enums::ACTION_STRAFELEFT,
	AVK_MOVERIGHT,	Enums::ACTION_STRAFERIGHT,
	AVK_SELECT,		Enums::ACTION_FIRE,
	AVK_MENUOPEN,	Enums::ACTION_MENU,
	AVK_PASSTURN,	Enums::ACTION_PASSTURN,
	AVK_BOTDISCARD,	Enums::ACTION_BOT_DISCARD
};

int Canvas::getKeyAction(int i) {

	int iVar1;

	//printf("getKeyAction i %d\n", i);

	if (this->state == Canvas::ST_MENU) {
		if (i & AVK_MENU_UP) {
			return Enums::ACTION_UP;
		}
		if (i & AVK_MENU_DOWN) {
			return Enums::ACTION_DOWN;
		}
		if (i & AVK_MENU_PAGE_UP) {
			return Enums::ACTION_LEFT;
		}
		if (i & AVK_MENU_PAGE_DOWN) {
			return Enums::ACTION_RIGHT;
		}
		if (i & AVK_MENU_SELECT) {
			return Enums::ACTION_FIRE;
		}
		if (i & AVK_ITEMS_INFO) {
			return Enums::ACTION_MENU_ITEM_INFO;
		}
	}

	if (i & AVK_MENU_OPEN) {
		return Enums::ACTION_MENU;
	}

	if (!app->player->isFamiliar) {
		if (i & AVK_ITEMS_INFO) {
			return Enums::ACTION_ITEMS;
		}

		if (i & AVK_DRINKS) {
			return Enums::ACTION_ITEMS_DRINKS;
		}

		if (i & AVK_PDA) {
			return Enums::ACTION_QUESTLOG;
		}
	}

	i &= ~(AVK_MENU_UP | AVK_MENU_DOWN | AVK_MENU_PAGE_UP | AVK_MENU_PAGE_DOWN | AVK_MENU_SELECT | AVK_MENU_OPEN | AVK_ITEMS_INFO | AVK_DRINKS | AVK_PDA);

	for (int j = 0; j < (NUM_CODES / 2); j++)
	{
		if (keys_codeActions[(j * 2) + 0] == i) {
			//printf("rtn %d\n", keys_codeActions[(j * 2) + 1]);
			return keys_codeActions[(j * 2) + 1];
		}
	}

	if (i - 1U < 10) { // KEY_1 to KEY_9 ... KEY_0
		return this->keys_numeric[i - 1U];
	}

	if (i == 12) { // KEY_STAR
		return Enums::ACTION_PREVWEAPON;
	}
	if (i == 11) { // KEY_POUND
		return Enums::ACTION_AUTOMAP;
	}
	if (i == 14) { // KEY_ARROWLEFT
		return Enums::ACTION_LEFT;
	}
	if (i == 15) { // KEY_ARROWRIGHT
		return Enums::ACTION_RIGHT;
	}
	if (i == 16) { // KEY_ARROWUP
		return Enums::ACTION_UP;
	}
	if (i == 17) { // KEY_ARROWDOWN
		return Enums::ACTION_DOWN;
	}
	if (i == 13) { // KEY_OK
		return Enums::ACTION_FIRE;
	}

	iVar1 = (i ^ i >> 0x1f) - (i >> 0x1f);
	if (iVar1 == 19) { // KEY_LEFTSOFT
		return Enums::ACTION_MENU;
	}
	if (iVar1 == 20) { // KEY_RIGHTSOFT
		return Enums::ACTION_AUTOMAP; // ACTION_AUTOMAP
	}
	if (iVar1 == 18) { // KEY_CLR, KEY_BACK
		return Enums::ACTION_BACK;
	}

	return Enums::ACTION_NONE;
}

bool Canvas::attemptMove(int n, int n2) {


	if (this->renderOnly) {
		this->destX = n;
		this->destY = n2;
		return true;
	}

	if (app->player->isFamiliar && (app->render->mapFlags[(n2 >> 6) * 32 + (n >> 6)] & 0x10) != 0x0 && (n != this->saveX || n2 != this->saveY)) {
		app->hud->addMessage((short)0, (short)222, 3);
		return false;
	}
	int n3 = app->player->noclip ? 0 : 13501;
	app->game->eventFlagsForMovement(this->viewX, this->viewY, n, n2);
	this->abortMove = false;
	app->game->executeTile(this->viewX >> 6, this->viewY >> 6, app->game->eventFlags[0], true);
	bool b = false;
	if (!this->abortMove) {
		app->game->trace(this->viewX, this->viewY, n, n2, app->player->getPlayerEnt(), n3, 16);
		if (app->game->traceEntity == nullptr || (app->player->isFamiliar && n == this->saveX && n2 == this->saveY && app->game->traceEntity == &app->game->entities[app->player->playerEntityCopyIndex])) {
			if (app->player->isFamiliar && n == this->saveX && n2 == this->saveY && app->game->traceEntity == &app->game->entities[app->player->playerEntityCopyIndex]) {
				app->player->familiarReturnsToPlayer(true);
			}
			if (app->hud->msgCount > 0 && (app->hud->messageFlags[0] & 0x2) != 0x0) {
				app->hud->msgTime = 0;
			}
			this->automapDrawn = false;
			this->destX = n;
			this->destY = n2;
			this->destZ = 36 + app->render->getHeight(this->destX, this->destY);
			this->zStep = (std::abs(this->destZ - this->viewZ) + this->animFrames - 1) / this->animFrames;
			this->prevX = this->viewX;
			this->prevY = this->viewY;
			this->startRotation(false);
			app->player->relink();
			b = true;
		}
		else if (this->knockbackDist == 0 && this->state == Canvas::ST_AUTOMAP) {
			app->game->advanceTurn();
		}
	}
	else if (this->knockbackDist != 0) {
		this->knockbackDist = 0;
	}
	return b;
}

void Canvas::loadState(int loadType, short n, short n2) {

	this->loadType = loadType;
	app->game->saveConfig();
	this->setLoadingBarText(n, n2);
	this->setState(Canvas::ST_LOADING);
}

void Canvas::saveState(int saveType, short n, short n2) {
	this->saveType = saveType;
	this->setLoadingBarText(n, n2);
	this->setState(Canvas::ST_SAVING);
}

void Canvas::loadMap(int loadMapID, bool b, bool tm_NewGame) {

	LOG_INFO("[canvas] loadMap: id={} newGame={}\n", loadMapID, tm_NewGame);
	if (loadMapID > 0 && loadMapID < 11) {
		bool b2 = false;
		int n = loadMapID - 1;
		short n2 = app->game->numLevelLoads[n];
		app->game->numLevelLoads[n] = (short)(n2 + 1);
		if ((b2 ? 1 : 0) == n2) {
			app->player->currentLevelDeaths = 0;
		}
	}
	this->lastMapID = this->loadMapID;
	this->loadMapID = loadMapID;
	app->sound->soundStop();
	this->TM_NewGame = tm_NewGame;
	if (!b && app->game->activeLoadType == 0 && this->lastMapID >= 1 && this->lastMapID <= 10) {
		this->saveState(43, (short)3, (short)196);
	}
	else {
		this->setLoadingBarText((short)3, app->game->levelNames[this->loadMapID - 1]);
		this->setState(Canvas::ST_TRAVELMAP);
	}
}

void Canvas::drawScroll(Graphics* graphics, int n, int n2, int n3, int n4) { // J2ME
	int width = this->imgDialogScroll->width;
	int height = this->imgDialogScroll->height;
	graphics->drawRegion(this->imgDialogScroll, 0, 0, width, height, n, n2 + n4 - height, 0, 0, 0);
	graphics->drawRegion(this->imgDialogScroll, 0, 0, width, height, n + n3 - width, n2 + n4 - height, 0, 2, 0);
	graphics->drawRegion(this->imgDialogScroll, 0, 0, width, height, n, n2, 0, 1, 0);
	graphics->drawRegion(this->imgDialogScroll, 0, 0, width, height, n + n3 - width, n2, 0, 3, 0);
	graphics->fillRegion(this->imgDialogScroll, width - 14, 0, 14, height, n + width, n2, n3 - 2 * width, height, 3);
	graphics->fillRegion(this->imgDialogScroll, width - 14, 0, 14, height, n + width, n2 + (n4 - height), n3 - 2 * width, height, 0);
	graphics->fillRegion(this->imgDialogScroll, 0, 0, width, 6, n, n2 + height, width, n4 - 2 * height, 0);
	graphics->fillRegion(this->imgDialogScroll, 0, 0, width, 6, n + n3 - width, n2 + height, width, n4 - 2 * height, 3);
	graphics->fillRegion(this->imgDialogScroll, width - 6, 0, 6, 6, n + width, n2 + height, n3 - 2 * width, n4 - 2 * height, 0);
}

bool Canvas::handlePlayingEvents(int key, int action) {

	//printf("handlePlayingEvents %d, %d\n", key, action);

	bool b = false;
	if (!this->isZoomedIn && (this->viewX != this->destX || this->viewY != this->destY || this->viewAngle != this->destAngle)) {
		return false;
	}

	if (this->knockbackDist != 0 || this->changeMapStarted) {
		return false;
	}

	if (this->renderOnly) {
		this->viewX = this->destX;
		this->viewY = this->destY;
		this->viewZ = this->destZ;
		this->viewAngle = this->destAngle;
		this->viewAngle = this->destPitch;
		this->viewSin = app->render->sinTable[this->destAngle & 0x3FF];
		this->viewCos = app->render->sinTable[this->destAngle + 256 & 0x3FF];
		this->viewStepX = this->viewCos * 64 >> 16;
		this->viewStepY = -this->viewSin * 64 >> 16;
		this->invalidateRect();
	}
	else {
		if (!this->isZoomedIn) {
			if (this->viewX != this->destX || this->viewY != this->destY) {
				b = true;
				this->viewX = this->destX;
				this->viewY = this->destY;
				this->viewZ = this->destZ;
				this->finishMovement();
				this->invalidateRect();
			}
			else if (this->viewAngle != this->destAngle) {
				b = true;
				this->viewAngle = this->destAngle;
				this->viewAngle = this->destPitch;
				this->finishRotation(true);
				this->invalidateRect();
			}
		}
		if (this->blockInputTime != 0) {
			return true;
		}

		if (app->hud->isInWeaponSelect != 0) { // [GEC]
			return true;
		}

		bool b2 = this->state == Canvas::ST_AUTOMAP;
		if (action != Enums::ACTION_PREVWEAPON && action != Enums::ACTION_NEXTWEAPON && action != Enums::ACTION_LEFT && action != Enums::ACTION_RIGHT && (app->game->activePropogators != 0 || !app->game->snapMonsters(b2) || app->game->animatingEffects != 0)) {
			return true;
		}
	}

	if (action == Enums::ACTION_BOT_DISCARD) { // [GEC]
		if (app->player->isFamiliar && this->state != Canvas::ST_AUTOMAP) {
			app->player->familiarReturnsToPlayer(false);
		}
		else if (app->player->weaponIsASentryBot(app->player->ce->weapon) && this->state != Canvas::ST_AUTOMAP) {
			app->player->attemptToDiscardFamiliar(app->player->ce->weapon);
		}
	}

	if (action == Enums::ACTION_AUTOMAP) {
		if (!app->player->inTargetPractice) {
			// OLD -> Original
			/*if (app->player->isFamiliar && this->state != Canvas::ST_AUTOMAP) {
				app->player->familiarReturnsToPlayer(false);
			}
			else if (app->player->weaponIsASentryBot(app->player->ce->weapon) && this->state != Canvas::ST_AUTOMAP) {
				app->player->attemptToDiscardFamiliar(app->player->ce->weapon);
			}
			else */{
				this->setState((this->state != Canvas::ST_AUTOMAP) ? Canvas::ST_AUTOMAP : Canvas::ST_PLAYING);
			}
		}
	}
	else if (action == Enums::ACTION_UP) {
		this->attemptMove(this->viewX + this->viewStepX, this->viewY + this->viewStepY);
	}
	else if (action == Enums::ACTION_DOWN) {
		this->attemptMove(this->viewX - this->viewStepX, this->viewY - this->viewStepY);
	}
	else if (action == Enums::ACTION_STRAFELEFT) {
		this->attemptMove(this->viewX + this->viewStepY, this->viewY - this->viewStepX);
	}
	else if (action == Enums::ACTION_STRAFERIGHT) {
		this->attemptMove(this->viewX - this->viewStepY, this->viewY + this->viewStepX);
	}
	else if (action == Enums::ACTION_LEFT || action == Enums::ACTION_RIGHT) {
		int n3 = 256;
		app->hud->damageTime = 0;
		if (action == Enums::ACTION_RIGHT) {
			n3 = -256;
		}
		this->destAngle += n3;
		this->startRotation(false);
		this->automapDrawn = false;
	}
	else if (action == Enums::ACTION_PREVWEAPON || action == Enums::ACTION_NEXTWEAPON) {
		int weapon = app->player->ce->weapon;
		if (app->combat->getWeaponFlags(weapon).isThrowableItem) {
			return true;
		}
		if (action == Enums::ACTION_PREVWEAPON) {
			app->player->selectPrevWeapon();
		}
		else {
			app->player->selectNextWeapon();
		}
		if (weapon != app->player->ce->weapon) {
			app->hud->addMessage((short)1, (short)app->player->activeWeaponDef->longName, 1);
		}
		app->player->helpBitmask |= 0x100;
	}
	else if (action == Enums::ACTION_BACK) {
		if (this->state == Canvas::ST_AUTOMAP) {
			this->setState(Canvas::ST_PLAYING);
		}
		else if (app->player->inTargetPractice) {
			app->player->exitTargetPractice();
		}
		else {
			app->hud->msgCount = 0;
			app->menuSystem->setMenu(Menus::MENU_INGAME);
		}
	}
	else if (action == Enums::ACTION_MENU) {
		if (app->player->inTargetPractice) {
			app->player->exitTargetPractice();
		}
		else {
			app->hud->msgCount = 0;
			app->menuSystem->setMenu(Menus::MENU_INGAME);
		}
	}
	else if (action == Enums::ACTION_ITEMS) {
		if (app->player->inTargetPractice) {
			app->player->exitTargetPractice();
		}
		else {
			app->hud->msgCount = 0;
			app->menuSystem->setMenu(Menus::MENU_ITEMS);
			app->menuSystem->oldMenu = Menus::MENU_ITEMS; // [GEC]
		}
	}
	else if (action == Enums::ACTION_ITEMS_DRINKS) {
		if (app->player->inTargetPractice) {
			app->player->exitTargetPractice();
		}
		else {
			app->hud->msgCount = 0;
			app->menuSystem->setMenu(Menus::MENU_ITEMS_DRINKS);
			app->menuSystem->oldMenu = Menus::MENU_ITEMS_DRINKS; // [GEC]
		}
	}
	else if (action == Enums::ACTION_QUESTLOG) {
		if (app->player->inTargetPractice) {
			app->player->exitTargetPractice();
		}
		else {
			app->hud->msgCount = 0;
			app->menuSystem->setMenu(Menus::MENU_INGAME_QUESTLOG);
			app->menuSystem->oldMenu = Menus::MENU_INGAME_QUESTLOG; // [GEC]
		}
	}
	else if (action == Enums::ACTION_FIRE) {
		if (app->player->facingEntity != nullptr && app->player->facingEntity->def->eType == Enums::ET_ATTACK_INTERACTIVE) {
			this->lootSource = app->player->facingEntity->name;
		}
		else {
			this->lootSource = -1;
		}

		int weapon2 = app->player->ce->weapon;
		
		int n4 = 16384;
		int n5 = 13997;

		if (app->combat->getWeaponFlags(weapon2).isMelee) {
			//n5 |= 0x2000; // J2ME only?
		}
		if (app->combat->getWeaponFlags(weapon2).interactFlags != 0) {
			n5 |= app->combat->getWeaponFlags(weapon2).interactFlags;
		}
		int n6 = 0;
		int n7 = 6;
		if (app->combat->getWeaponFlags(weapon2).isMelee) {
			n7 = 1;
			n5 |= 0x10;
		}

		Entity* entity = nullptr;
		Entity* entity2 = nullptr;

		int n8 = this->viewX + (n7 * -app->tinyGL->view[2] >> 8);
		int n9 = this->viewY + (n7 * -app->tinyGL->view[6] >> 8);
		int n10 = this->viewZ + (n7 * -app->tinyGL->view[10] >> 8);
		app->game->trace(this->viewX, this->viewY, this->viewZ, n8, n9, n10, nullptr, n5, 2, this->isZoomedIn);

		int i = 0;
		while (i < app->game->numTraceEntities) {
			Entity* entity3 = app->game->traceEntities[i];
			int n11 = app->game->traceFracs[i];
			int dist = entity3->distFrom(this->viewX, this->viewY);
			uint8_t eType = entity3->def->eType;
			if (eType == Enums::ET_WORLD || eType == Enums::ET_SPRITEWALL || eType == Enums::ET_PLAYERCLIP) {
				if (entity == nullptr) {
					entity = entity3;
					n4 = n11;
					break;
				}
				break;
			}
			else {
				if (eType == Enums::ET_ATTACK_INTERACTIVE) {
					if ((1 << entity3->def->eSubType & 0x1) == 0x0 || app->combat->getWeaponFlags(app->player->ce->weapon).isMelee) {
						if (n6 == 0) {
							entity = entity3;
							n4 = n11;
							break;
						}
						break;
					}
				}
				else if (eType == Enums::ET_NONOBSTRUCTING_SPRITEWALL) {
					if (app->combat->getWeaponFlags(weapon2).isMelee) {
						entity2 = entity3;
					}
				}
				else if (eType == Enums::ET_NPC) {
					if (dist >= 8192) {
						if (n6 == 0) {
							entity = entity3;
							n4 = n11;
							break;
						}
						break;
					}
				}
				else {
					if (eType == Enums::ET_MONSTER) {
						entity = entity3;
						n4 = n11;
						n6 = 0;
						break;
					}
					if (eType == Enums::ET_DOOR) {
						if (entity == nullptr) {
							entity = entity3;
							n4 = n11;
							n6 = 0;
							break;
						}
						break;
					}
					else if (eType == Enums::ET_CORPSE) {
						if (dist == app->combat->tileDistances[0]) {
							if (app->combat->getWeaponFlags(weapon2).canLootCorpses) {
#if 0 // J2ME
								if (entity != nullptr && entity->def->eType == Enums::ET_CORPSE) {
									if (entity->linkIndex < entity3->linkIndex) {
										if (!entity3->isMonster() && entity3->def->eSubType != Enums::MONSTER_SENTRY_BOT) {
											if (entity3->param == 0 && entity3->loot != nullptr) {
												entity = entity3;
												n4 = n11;
												n6 = 1;
											}
											else {
												app->hud->addMessage((short)0, (short)250, 2);
											}
										}
										else {
											entity = entity3;
											n4 = n11;
										}
									}
								}
								else if (!entity3->isMonster() && entity3->def->eSubType != Enums::MONSTER_SENTRY_BOT) {
									if (entity3->param == 0 && entity3->loot != nullptr) {
										entity = entity3;
										n4 = n11;
										n6 = 1;
									}
									else {
										app->hud->addMessage((short)0, (short)250, 2);
									}
								}
								else {
									entity = entity3;
									n4 = n11;
								}
#else
								if (((entity == nullptr) || (entity->def->eType != Enums::ET_CORPSE))
									|| (entity->linkIndex < entity3->linkIndex)) {
									entity = entity3;
									n4 = n11;
								}
#endif
							}
							else if (!this->isZoomedIn) {
								if (!entity3->isMonster()) {
									if (entity3->param == 0 && entity3->loot != nullptr) {
										entity = entity3;
										n4 = n11;
										n6 = 1;
									}
								}
								else if ((entity3->monsterFlags & 0x800) == 0x0 && entity3->loot != nullptr) {
									entity = entity3;
									n4 = n11;
									n6 = 1;
								}
							}
						}
					}
					else if (eType == Enums::ET_ENV_DAMAGE) {
						if (entity3->def->eSubType == Enums::ENV_DAMAGE_FIRE && app->combat->getWeaponFlags(weapon2).fountainWeapon && app->player->ammo[app->combat->weapons[weapon2 * Combat::WEAPON_MAX_FIELDS + Combat::WEAPON_FIELD_AMMOTYPE]] >= app->combat->weapons[weapon2 * Combat::WEAPON_MAX_FIELDS + Combat::WEAPON_FIELD_AMMOUSAGE]) {
							entity = entity3;
							n4 = n11;
							n6 = 0;
							break;
						}
					}
					else if (eType == Enums::ET_DECOR) {
						if ((app->render->mapSpriteInfo[entity3->getSprite()] & 0xFF) == 0x95) {
							entity = entity3;
							n4 = n11;
							break;
						}
					}
					else if (eType == Enums::ET_DECOR_NOCLIP) {
						if (entity3->def->eSubType == Enums::DECOR_WATER_SPOUT && dist == app->combat->tileDistances[0]) {
							entity = entity3;
							n4 = n11;
							break;
						}
					}
					else if (eType != Enums::ET_DECOR && eType != Enums::ET_ITEM && entity == nullptr) {
						entity = entity3;
						n4 = n11;
					}
				}
				++i;
			}
		}

		int dist2 = app->combat->tileDistances[9];
		if (entity != nullptr) {
			dist2 = entity->distFrom(this->viewX, this->viewY);
		}
		if (n6 != 0) {
			this->setState(Canvas::ST_LOOTING);
			this->poolLoot(entity->calcPosition());
			return true;
		}
		int n12 = weapon2 * 9;
		if (entity != nullptr && entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && (1 << entity->def->eSubType & 0x1) == 0x0 && app->combat->WorldDistToTileDist(dist2) > app->combat->weapons[n12 + 3]) {
			entity = nullptr;
		}
		if (entity2 != nullptr && (entity == nullptr || (1 << weapon2 & 0x0) != 0x0 || (entity->def->eType != Enums::ET_MONSTER && entity->def->eType != Enums::ET_CORPSE))) {
			entity = entity2;
		}

		if (entity != nullptr && entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && entity->def->eSubType == Enums::INTERACT_CRATE) {
			if (dist2 <= app->combat->tileDistances[0]) {
				entity->param = app->upTimeMs + 200;
				app->game->unlinkEntity(entity);
			}
			else if (app->combat->getWeaponFlags(weapon2).splashDamage) {}
		}

		int flagForFacingDir = this->flagForFacingDir(4);
		int n13 = this->destX + this->viewStepX >> 6;
		int n14 = this->destY + this->viewStepY >> 6;
		if (app->game->executeTile(n13, n14, flagForFacingDir, true)) {
			if (!app->game->skipAdvanceTurn && this->state == Canvas::ST_PLAYING) {
				app->game->touchTile(this->destX, this->destY, false);
				app->game->snapMonsters(true);
				app->game->advanceTurn();
			}
		}
		else if (entity != nullptr && entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && entity->def->eSubType == Enums::INTERACT_PICKUP && dist2 <= app->combat->tileDistances[0] && (app->combat->throwableItemAmmoType < 0 || app->player->ammo[app->combat->throwableItemAmmoType] == 0)) {
			if (!app->player->isFamiliar) {
				int fountainAmmoType = app->combat->weapons[app->player->ce->weapon * Combat::WEAPON_MAX_FIELDS + Combat::WEAPON_FIELD_AMMOTYPE];
				int fountainAmmoMax = this->gameConfig->capAmmo;
				if (app->combat->getWeaponFlags(app->player->ce->weapon).fountainWeapon && app->player->ammo[fountainAmmoType] < fountainAmmoMax) {
					app->hud->addMessage((short)248);
					app->player->ammo[fountainAmmoType] = fountainAmmoMax;
					app->player->showHelp((short)14, false);
					app->sound->playSound(Sounds::getResIDByName(SoundName::HOLYWATERPISTOL_REFILL), 0, 3, 0);
				}
				else if (app->player->ce->getStat(4) < 11) {
					app->hud->addMessage((short)238, 3);
				}
				else {
					app->player->currentWeaponCopy = app->player->ce->weapon;
					app->player->setPickUpWeapon(entity->def->tileIndex);
					app->player->give(2, 8, 1, true);
					app->player->giveAmmoWeapon(14, true);
					this->turnEntityIntoWaterSpout(entity);
					int* calcPosition = entity->calcPosition();
					if (this->shouldFakeCombat(calcPosition[0] >> 6, calcPosition[1] >> 6, flagForFacingDir) && app->combat->explodeThread != nullptr) {
						app->combat->explodeThread->run();
						app->combat->explodeThread = nullptr;
					}
					app->sound->playSound(Sounds::getResIDByName(SoundName::WEAPON_SNIPER_SCOPE), 0, 3, 0);
				}
				return true;
			}
		}
		else {
			if (entity != nullptr && entity->def->eType == Enums::ET_DECOR_NOCLIP && entity->def->eSubType == Enums::DECOR_WATER_SPOUT && dist2 <= app->combat->tileDistances[0] && dist2 > 0 && app->combat->getWeaponFlags(app->player->ce->weapon).fountainWeapon) {
				int fAmmoType = app->combat->weapons[app->player->ce->weapon * Combat::WEAPON_MAX_FIELDS + Combat::WEAPON_FIELD_AMMOTYPE];
				int fAmmoMax = this->gameConfig->capAmmo;
				if (app->player->ammo[fAmmoType] < fAmmoMax) {
					app->hud->addMessage((short)248);
					app->player->showHelp((short)14, false);
					app->sound->playSound(Sounds::getResIDByName(SoundName::HOLYWATERPISTOL_REFILL), 0, 3, 0);
				}
				else {
					app->hud->addMessage((short)249);
				}
				app->player->ammo[fAmmoType] = fAmmoMax;
				return true;
			}
			if (entity != nullptr && entity->def->eType == Enums::ET_DOOR && dist2 <= app->combat->tileDistances[0] && !app->combat->getWeaponFlags(weapon2).isThrowableItem) {
				if (!app->player->isFamiliar) {
					if (entity->def->eSubType == Enums::DOOR_LOCKED) {
						app->hud->addMessage((short)44, 2);
					}
					else {
						app->game->performDoorEvent(0, entity, 1);
						app->game->advanceTurn();
					}
				}
				else {
					app->sound->playSound(Sounds::getResIDByName(SoundName::SENTRYBOT_PAIN), 0, 3, 0);
					app->hud->addMessage((short)193, 2);
				}
				return true;
			}
			if (entity != nullptr && entity->def->eType == Enums::ET_SPRITEWALL && dist2 <= app->combat->tileDistances[0] && (app->render->mapFlags[n14 * 32 + n13] & 0x4) != 0x0) {
				if (app->game->performDoorEvent(0, entity, 1, true)) {
					app->game->awardSecret(true);
				}
				return true;
			}
			if (entity != nullptr && (entity->def->eType == Enums::ET_WORLD || entity->def->eType == Enums::ET_SPRITEWALL) && dist2 <= app->combat->tileDistances[0]) {
				if (this->isZoomedIn) {
					return true;
				}

				if (app->player->isFamiliar) {
					app->sound->playSound(Sounds::getResIDByName(SoundName::SENTRYBOT_PAIN), 0, 3, 0);
				}
				else if (app->player->characterChoice == 1) {
					app->sound->playSound(Sounds::getResIDByName(SoundName::NO_USE2), 0, 3, 0);
				}
				else if (app->player->characterChoice >= 1 && app->player->characterChoice <= 3) {
					app->sound->playSound(Sounds::getResIDByName(SoundName::NO_USE), 0, 3, 0);
				}

				app->combat->shiftWeapon(true);
				this->pushedWall = true;
				this->pushedTime = app->gameTime + 500;
				app->render->rockView(1000, this->viewX + (this->viewStepX >> 6) * 2, this->viewY + (this->viewStepY >> 6) * 2, this->viewZ);
				return true;
			}
			else {
				const Combat::FamiliarDef* fd = app->combat->getFamiliarDefByType(app->player->familiarType);
			if (app->player->isFamiliar && fd && fd->selfDestructs) {
					app->player->startSelfDestructDialog();
					return true;
				}
				if (!app->player->isFamiliar && app->player->weaponIsASentryBot(weapon2)) {
					app->player->attemptToDeploySentryBot();
					return true;
				}
				if (entity != nullptr && entity->def->eType == Enums::ET_ENV_DAMAGE) {
					this->shouldFakeCombat(app->game->traceCollisionX >> 6, app->game->traceCollisionY >> 6, flagForFacingDir);
					app->player->fireWeapon(entity, app->game->traceCollisionX, app->game->traceCollisionY);
					app->game->removeEntity(entity);
					app->player->addXP(5);
				}
				else {
					if (!this->isZoomedIn && app->combat->getWeaponFlags(weapon2).isScoped && app->player->ammo[app->combat->weapons[n12 + 4]] > 0) {
						this->initZoom();
						return true;
					}
					if (entity != nullptr && entity->def->eType != Enums::ET_WORLD) {
						int* calcPosition2 = entity->calcPosition();
						bool shouldFakeCombat = this->shouldFakeCombat(calcPosition2[0] >> 6, calcPosition2[1] >> 6, flagForFacingDir);
						int n15 = weapon2 * 9;
						if (shouldFakeCombat || (app->combat->weapons[n15 + 3] == 1 && app->combat->weapons[n15 + 2] == 1 && dist2 > app->combat->tileDistances[0])) {
							app->game->traceCollisionX = calcPosition2[0];
							app->game->traceCollisionY = calcPosition2[1];
							app->player->fireWeapon(&app->game->entities[0], calcPosition2[0], calcPosition2[1]);
						}
						else {
							if (this->isZoomedIn) {
								this->zoomCollisionX = this->viewX + (n4 * (n8 - this->viewX) >> 14);
								this->zoomCollisionY = this->viewY + (n4 * (n9 - this->viewY) >> 14);
								this->zoomCollisionZ = this->viewZ + (n4 * (n10 - this->viewZ) >> 14);
							}
							app->player->fireWeapon(entity, calcPosition2[0], calcPosition2[1]);
							if (app->player->inTargetPractice) {
								if (app->player->ammo[this->gameConfig->tpAmmoType] == 0) {
									app->player->exitTargetPractice();
								}
								else {
									app->player->assessTargetPracticeShot(entity);
								}
							}
						}
					}
					else {
						this->shouldFakeCombat(app->game->traceCollisionX >> 6, app->game->traceCollisionY >> 6, flagForFacingDir);
						app->player->fireWeapon(&app->game->entities[0], app->game->traceCollisionX, app->game->traceCollisionY);
						if (app->player->inTargetPractice) {
							if (app->player->ammo[this->gameConfig->tpAmmoType] == 0) {
								app->player->exitTargetPractice();
							}
							else {
								app->hud->addMessage((short)68);
							}
						}
					}
				}
			}
		}
	}
	else if (action == Enums::ACTION_PASSTURN) {
		app->hud->addMessage((short)45);
		app->game->touchTile(this->destX, this->destY, false);
		app->game->advanceTurn();
		this->invalidateRect();
	}

	return this->endOfHandlePlayingEvent(action, b);
}

bool Canvas::handleCinematicInput(int action) { // J2ME

	if (action == Enums::ACTION_FIRE) {
		app->game->executeTile(this->destX >> 6, this->destY >> 6, app->game->eventFlags[1], true);
	}
	return false;
}

bool Canvas::shouldFakeCombat(int n, int n2, int n3) {

	bool b = false;
	int n4 = app->player->ce->weapon * 9;
	if (app->combat->weapons[n4 + 4] != 0) {
		short n5 = app->player->ammo[app->combat->weapons[n4 + 4]];
		if (app->combat->weapons[n4 + 5] > 0 && n5 - app->combat->weapons[n4 + 5] < 0) {
			return false;
		}
	}
	n3 |= this->flagForWeapon(app->player->ce->weapon);
	if (app->game->doesScriptExist(n, n2, n3)) {
		(app->combat->explodeThread = app->game->allocScriptThread())->queueTile(n, n2, n3);
		b = true;
	}
	return b;
}

bool Canvas::endOfHandlePlayingEvent(int action, bool b) {

	if ((action == Enums::ACTION_STRAFELEFT || action == Enums::ACTION_STRAFERIGHT || action == Enums::ACTION_LEFT || action == Enums::ACTION_RIGHT) && (this->viewX != this->destX || this->viewY != this->destY || this->viewAngle != this->destAngle)) {
		app->player->facingEntity = nullptr;
	}
	return true;
}

bool Canvas::handleEvent(int key) {


	int state = this->state;
	int keyAction = this->getKeyAction(key);
	int fadeFlags = app->render->getFadeFlags();

	
	//printf("handleEvent key: %d keyAction: %d\n", key, keyAction);
	//printf("this->state %d\n", state);

	if (key == 26)
		return true;

	// Dispatch input to registered state handler
	if (state >= 0 && state < MAX_STATE_ID && stateHandlers[state]) {
		return stateHandlers[state]->handleInput(this, key, keyAction);
	}

	// Only ST_LOADING/ST_SAVING reach here (no handlers registered)
	return false;
}

void Canvas::runInputEvents() {


	while (this->blockInputTime == 0) {
		if (this->ignoreFrameInput > 0) {
			this->numEvents = 0;
			this->ignoreFrameInput--;
			return;
		}
		if (this->numEvents == 0) {
			return;
		}

		
		int ev = this->events[0];
		//printf("this->events[0] %d\n", ev);
		this->st_fields[11] = app->upTimeMs;
		if (!this->handleEvent(ev)) {
			return;
		}

		if (this->numEvents > 0) {
			this->numEvents--;
		}
	}

	if (app->gameTime > this->blockInputTime) {
		if (this->state == Canvas::ST_PLAYING) {
			this->drawPlayingSoftKeys();
		}
		this->blockInputTime = 0;
	}
	this->clearEvents(1);
}

bool Canvas::loadMedia() {

	LOG_INFO("[canvas] loadMedia\n");

	//printf("Canvas::isLoaded %d\n", Canvas::isLoaded);
	if (Canvas::isLoaded == false) {
		this->updateLoadingBar(Canvas::isLoaded);
		this->drawLoadingBar(&this->graphics);
		Canvas::isLoaded = true;
		
		return false;
	}
	Canvas::isLoaded = false;

	bool allowSounds = app->sound->allowSounds;
	app->canvas->inInitMap = true;
	app->sound->allowSounds = false;
	this->mediaLoading = true;
	bool displaySoftKeys = this->displaySoftKeys;
	this->updateLoadingBar(false);
	this->unloadMedia();
	this->displaySoftKeys = false;
	this->isZoomedIn = false;
	app->StopAccelerometer();
	app->tinyGL->resetViewPort();

	for (int i = 0; i < 128; ++i) {
		if (i != 15) {
			app->game->scriptStateVars[i] = 0;
		}
	}

	if (!app->render->beginLoadMap(this->loadMapID)) {
		return false;
	}

	if (this->loadMapID <= 10 && this->loadMapID > app->player->highestMap) {
		app->player->highestMap = this->loadMapID;
	}

	if (app->game->isSaved) {
		this->setLoadingBarText((short)0, (short)38);
	}
	else if (app->game->isLoaded) {
		this->setLoadingBarText((short)0, (short)39);
	}
	else if (this->loadType != 3) {
		this->setLoadingBarText((short)3, (short)(app->render->mapNameField & 0x3FF));
	}

	this->updateLoadingBar(false);
	app->render->mapMemoryUsage -= 1000000000;
	//app->checkPeakMemory("after map loaded");
	app->game->loadMapEntities();
	app->hud->msgCount = 0;

	this->updateLoadingBar(false);
	app->player->playTime = app->gameTime;
	app->game->curLevelTime = app->gameTime;
	this->clearEvents(1);
	app->particleSystem->freeAllParticles();
	this->displaySoftKeys = displaySoftKeys;
	app->player->levelInit();

	this->updateLoadingBar(false);
	app->game->loadWorldState();

	this->updateLoadingBar(false);
	app->game->spawnPlayer();
	this->knockbackDist = 0;
	if (!app->game->isLoaded && this->loadType != 3) {
		app->game->saveLevelSnapshot();
	}

	this->updateLoadingBar(false);
	//printf("app->game->isLoaded %d\n", app->game->isLoaded);
	//printf("this->loadType %d\n", this->loadType);
	if (app->game->isLoaded || app->game->hasSavedState() || this->loadType == 3) {
		this->loadRuntimeData();
		//app->checkPeakMemory("after loadRuntimeData");
	}
	else {
		app->game->saveState(this->loadMapID, this->loadMapID, this->destX, this->destY, this->destAngle, this->destPitch, this->destX, this->destY, this->saveX, this->saveY, this->saveZ, this->saveAngle, this->savePitch, 3);
	}

	this->updateLoadingBar(false);
	app->player->selectWeapon(app->player->ce->weapon);
	app->game->scriptStateVars[12] = app->game->difficulty;
	app->game->executeStaticFunc(0);

	this->updateLoadingBar(false);
	if (app->player->gameCompleted) {
		app->game->executeStaticFunc(1);
	}

	if (!app->game->isLoaded) {
		this->prevX = this->destX;
		this->prevY = this->destY;
		app->game->executeTile(this->viewX >> 6, this->viewY >> 6, 4081, 1);
		this->finishRotation(false);
		this->dequeueHelpDialog(true);
	}
	this->finishRotation(false);

	app->game->endMonstersTurn();
	this->uncoverAutomap();
	this->updateLoadingBar(false);
	app->game->isSaved = (app->game->isLoaded = false);
	app->game->activeLoadType = 0;
	this->dequeueHelpDialog(true);
	if (this->state == 0) {
		this->setState(Canvas::ST_PLAYING);
	}
	app->game->pauseGameTime = false;
	app->lastTime = (app->time = app->upTimeMs);
	this->blockInputTime = app->gameTime + 200;
	app->sound->allowSounds = allowSounds;
	this->inInitMap = false;
	this->mediaLoading = false;
	this->renderScene(this->viewX, this->viewY, this->viewZ, this->viewAngle, this->viewPitch, 0, 290);

	if (this->state != Canvas::ST_CAMERA) {
		this->repaintFlags |= Canvas::REPAINT_HUD;
	}
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;

	if (this->state == Canvas::ST_PLAYING) {
		this->drawPlayingSoftKeys();
	}

	for (int i = 0; i < 18; i++) {
		app->render->monsterIdleTime[i] = ((app->nextByte() % 10) * 1000) + app->time + 12000;
	}

	if (this->state == Canvas::ST_LOADING && !this->loadType) {
		this->setState(Canvas::ST_PLAYING);
	}

	return true;
}

// combatState() moved to CombatState::update()

void Canvas::automapState() {

	app->game->updateLerpSprites();
	if (!this->automapDrawn && app->game->animatingEffects == 0) {
		this->updateView();
		this->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
		if (this->state != Canvas::ST_AUTOMAP) {
			this->updateView();
		}
	}
	if (this->state == Canvas::ST_AUTOMAP || this->state == Canvas::ST_PLAYING) {
		this->drawPlayingSoftKeys();
	}
}

void Canvas::renderOnlyState() {


	if (this->st_enabled) {
		this->viewAngle = (this->viewAngle + this->animAngle & 0x3FF);
		this->viewPitch = 0;
	}
	else {
		if (this->viewX == this->destX && this->viewY == this->destY && this->viewAngle == this->destAngle) {
			return;
		}
		if (this->viewX < this->destX) {
			this->viewX += this->animPos;
			if (this->viewX > this->destX) {
				this->viewX = this->destX;
			}
		}
		else if (this->viewX > this->destX) {
			this->viewX -= this->animPos;
			if (this->viewX < this->destX) {
				this->viewX = this->destX;
			}
		}
		if (this->viewY < this->destY) {
			this->viewY += this->animPos;
			if (this->viewY >this->destY) {
				this->viewY =this->destY;
			}
		}
		else if (this->viewY > this->destY) {
			this->viewY -= this->animPos;
			if (this->viewY < this->destY) {
				this->viewY = this->destY;
			}
		}
		if (this->viewZ < this->destZ) {
			++this->viewZ;
		}
		else if (this->viewZ > this->destZ) {
			--this->viewZ;
		}
		if (this->viewAngle < this->destAngle) {
			this->viewAngle += this->animAngle;
			if (this->viewAngle > this->destAngle) {
				this->viewAngle = this->destAngle;
			}
		}
		else if (this->viewAngle > this->destAngle) {
			this->viewAngle -= this->animAngle;
			if (this->viewAngle < this->destAngle) {
				this->viewAngle = this->destAngle;
			}
		}
		if (this->viewPitch < this->destPitch) {
			this->viewPitch += this->pitchStep;
			if (this->viewPitch > this->destPitch) {
				this->viewPitch = this->destPitch;
			}
		}
		else if (this->viewPitch > this->destPitch) {
			this->viewPitch -= this->pitchStep;
			if (this->viewPitch <this->destPitch) {
				this->viewPitch =this->destPitch;
			}
		}
	}
	this->lastFrameTime = app->time;
	app->render->render((this->viewX << 4) + 8, (this->viewY << 4) + 8, (this->viewZ << 4) + 8, this->viewAngle, 0, 0, 290);
	app->combat->drawWeapon(0, 0);
	this->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_VIEW3D);
}

// playingState() moved to PlayingState::update()
// menuState() moved to MenuState::update()
// dyingState() moved to DyingState::update()
// familiarDyingState() moved to BotDyingState::update()

void Canvas::logoState() {


	if (!app->sound->soundsLoaded) {
		app->sound->cacheSounds();
	}

	if (CAppContainer::getInstance()->customMapID || CAppContainer::getInstance()->customMapFile) {
		// Replicate startGame(true) + character selection init
		if (app->menuSystem->background != app->menuSystem->imgMainBG) {
			app->menuSystem->background->~Image();
		}
		app->menuSystem->imgMainBG->~Image();
		app->menuSystem->background = nullptr;
		app->menuSystem->imgMainBG = nullptr;
		app->sound->soundStop();

		this->setLoadingBarText(-1, -1);
		app->game->removeState(true);
		app->game->activeLoadType = 0;
		this->loadType = 0;
		this->loadMapID = 0;
		this->lastMapID = 0;
		app->player->reset();
		app->player->totalDeaths = 0;
		app->player->currentLevelDeaths = 0;
		app->player->helpBitmask = 0;
		app->player->invHelpBitmask = 0;
		app->player->ammoHelpBitmask = 0;
		app->player->weaponHelpBitmask = 0;
		app->player->armorHelpBitmask = 0;
		app->player->currentGrades = 0;
		app->player->ce->weapon = -1;
		this->clearEvents(1);

		app->game->difficulty = 1; // Normal
		app->player->setCharacterChoice(1); // Major (default)
		app->player->reset();

		if (CAppContainer::getInstance()->customMapID) {
			this->startupMap = CAppContainer::getInstance()->customMapID;
		}

		this->loadMap(this->startupMap, false, true);
		return;
	}

	if (CAppContainer::getInstance()->minigameName) {
		const char* mgName = CAppContainer::getInstance()->minigameName;
		CAppContainer::getInstance()->minigameName = nullptr;
		app->sound->soundStop();
		IMinigame* mg = CAppContainer::getInstance()->minigameRegistry.getByName(mgName);
		if (mg) {
			mg->playFromMainMenu();
		} else {
			LOG_WARN("[canvas] Unknown minigame: {}\n", mgName);
		}
		return;
	}

	if (this->pacLogoTime <= 120) {
		this->pacLogoTime++;
		if (this->imgStartupLogo == nullptr) {
			this->imgStartupLogo = app->loadImage("l2.bmp", true);
		}
		this->repaintFlags |= Canvas::REPAINT_STARTUP_LOGO;
	}
	else {
		this->imgStartupLogo->~Image();
		this->imgStartupLogo = nullptr;

		this->setState(Canvas::ST_INTRO_MOVIE);
		this->numEvents = 0;
		this->keyDown = false;
		this->keyDownCausedMove = false;
		this->ignoreFrameInput = 1;
	}
}

void Canvas::drawScrollBar(Graphics* graphics, int i, int i2, int i3, int i4, int i5, int i6, int i7)
{
	bool v9; // cc
	int v12; // r1
	Canvas* v14; // [sp+20h] [bp-24h]
	int v15; // [sp+24h] [bp-20h]
	int v16; // [sp+28h] [bp-1Ch]

	v14 = this;
	v9 = i7 < 0;
	if (i7)
		v9 = i7 < i6;
	if (v9)
	{
		v12 = i6 - i7;
		if (i6 - i7 < i4)
			v12 = i4;
		v15 = 3 * i3 / (4 * ((i7 + i6 - 1) / i7));
		v16 = ((i4 << 16) / (v12 << 8) * ((i3 - v15 - 14) << 8)) >> 16;
		if (i6 == i5)
			v16 = i3 - 3 * i3 / (4 * ((i7 + i6 - 1) / i7)) - 14;
		graphics->drawRegion(this->imgUIImages, 60, 0, 7, 7, i, i2, 24, 0, 0);
		graphics->drawRegion(v14->imgUIImages, 60, 7, 7, 7, i, i3 + i2, 40, 0, 0);
		graphics->setColor(-5002605);
		graphics->fillRect(i - 7, i2 + 7, 7, i3 - 14);
		graphics->setColor(-1585235);
		graphics->fillRect(i - 7, v16 + 7 + i2, 7, v15);
		graphics->setColor(-16777216);
		graphics->drawRect(i - 7, v16 + 7 + i2, 6, v15 - 1);
		graphics->drawRect(i - 7, i2, 6, i3 - 1);
	}
}

void Canvas::renderScene(int viewX, int viewY, int viewZ, int viewAngle, int viewPitch, int viewRoll, int viewFov) {


	{ // J2ME
		//this->staleView = true;
		//if (!this->staleView && (this->staleTime == 0 || app->time < this->staleTime)) {
			//return;
		//}
	}

	this->staleView = false;
	this->staleTime = 0;
	this->lastFrameTime = app->time;
	this->beforeRender = app->upTimeMs;
	app->render->render((viewX << 4) + 8, (viewY << 4) + 8, (viewZ << 4) + 8, viewAngle, viewPitch, viewRoll, viewFov);
	this->afterRender = app->upTimeMs;
	++this->renderSceneCount;
	app->render->renderPortal();
	if (!this->isZoomedIn) {
		app->combat->drawWeapon(this->shakeX, this->shakeY);
	}
	if (app->render->postProcessMode != 0) {
		app->render->postProcessView(&this->graphics);
	}

	this->repaintFlags |= Canvas::REPAINT_VIEW3D;
}

void Canvas::startSpeedTest(bool b) {
	this->renderOnly = true;
	this->st_enabled = true;
	this->st_count = 1;
	for (int i = 0; i < Canvas::SPD_NUM_FIELDS; ++i) {
		this->st_fields[i] = 0;
	}
	if (!b) {
		this->animAngle = 4;
		this->destAngle = this->viewAngle;
		this->setState(Canvas::ST_BENCHMARK);
	}
}

void Canvas::backToMain(bool b) {


	this->loadMapID = 0;
	app->freeRuntimeImages();
	app->player->reset();
	app->game->unloadMapData();
	app->render->unloadMap();
	app->render->endFade();

	app->menuSystem->imgMainBG->~Image();
	app->menuSystem->imgMainBG = app->loadImage("logo.bmp", true);

	if (b) {
		this->clearEvents(1);
		if (this->skipIntro) {
			app->player->reset();
			app->render->unloadMap();
			app->game->unloadMapData();
			this->loadMap(this->startupMap, true, false);
		}
		else {
			if (app->localization->selectLanguage) {
				app->menuSystem->clearStack();
				app->menuSystem->setMenu(Menus::MENU_SELECT_LANGUAGE);
			}
			else {
				app->menuSystem->setMenu(Menus::MENU_MAIN);
			}
		}
	}
	else {
		app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_TITLE), 1, 3, false);
		app->menuSystem->setMenu(Menus::MENU_MAIN);
	}
}

void Canvas::drawPlayingSoftKeys() {


	if (app->player->inTargetPractice) {
		this->setLeftSoftKey((short)0, (short)30);
		this->clearRightSoftKey();
	}
	else if (this->isZoomedIn) {
		this->setSoftKeys((short)0, (short)52, (short)0, (short)55);
	}
	else if (this->state == Canvas::ST_AUTOMAP) {
		this->setSoftKeys((short)0, (short)52, (short)0, (short)53);
	}
	else if (app->player->isFamiliar) {
		this->setSoftKeys((short)0, (short)52, (short)0, (short)216);
	}
	else if (app->player->weaponIsASentryBot(app->player->ce->weapon)) {
		this->setSoftKeys((short)0, (short)52, (short)0, (short)220);
	}
	else {
		this->setSoftKeys((short)0, (short)52, (short)0, (short)55);
	}
}

void Canvas::updateView() {


	if (app->time < this->shakeTime) {
		this->shakeX = app->nextByte() % (this->shakeIntensity * 2) - this->shakeIntensity;
		this->shakeY = app->nextByte() % (this->shakeIntensity * 2) - this->shakeIntensity;
		this->staleView = true;
	}
	else if (this->shakeX != 0 || this->shakeY != 0) {
		this->staleView = true;
		this->shakeX = (this->shakeY = 0);
	}

	if (app->game->isCameraActive() && this->state != Canvas::ST_INTER_CAMERA) {
		app->game->activeCamera->Render();
		this->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
		app->hud->repaintFlags &= 0x18;
		return;
	}

	if (this->knockbackDist > 0 && this->viewX == this->destX && this->viewY == this->destY) {
		this->attemptMove(this->viewX + this->knockbackX * 64, this->viewY + this->knockbackY * 64);
	}

	bool b = this->viewX == this->destX && this->viewY == this->destY;
	bool b2 = this->viewAngle == this->destAngle;
	int animPos = this->animPos;
	int animAngle = this->animAngle;

	if (app->player->statusEffects[2] > 0 || this->knockbackDist > 0) {
		animPos += animPos / 2;
		animAngle += animAngle / 2;
	}

	if (this->viewX != this->destX || this->viewY != this->destY || this->viewZ != this->destZ || this->viewAngle != this->destAngle) {
		this->invalidateRect();
	}

	if (this->viewX < this->destX) {
		this->viewX += animPos;
		if (this->viewX > this->destX) {
			this->viewX = this->destX;
		}
	}
	else if (this->viewX > this->destX) {
		this->viewX -= animPos;
		if (this->viewX < this->destX) {
			this->viewX = this->destX;
		}
	}

	if (this->viewY < this->destY) {
		this->viewY += animPos;
		if (this->viewY > this->destY) {
			this->viewY = this->destY;
		}
	}
	else if (this->viewY > this->destY) {
		this->viewY -= animPos;
		if (this->viewY < this->destY) {
			this->viewY = this->destY;
		}
	}

	if (this->viewZ < this->destZ) {
		this->viewZ += this->zStep;
		if (this->viewZ > this->destZ) {
			this->viewZ = this->destZ;
		}
	}
	else if (this->viewZ > this->destZ) {
		this->viewZ -= this->zStep;
		if (this->viewZ < this->destZ) {
			this->viewZ = this->destZ;
		}
	}

	if (this->viewAngle < this->destAngle) {
		this->viewAngle += animAngle;
		if (this->viewAngle > this->destAngle) {
			this->viewAngle = this->destAngle;
		}
	}
	else if (this->viewAngle > this->destAngle) {
		this->viewAngle -= animAngle;
		if (this->viewAngle < this->destAngle) {
			this->viewAngle = this->destAngle;
		}
	}

	if (this->viewPitch < this->destPitch) {
		this->viewPitch += this->pitchStep;
		if (this->viewPitch > this->destPitch) {
			this->updateFacingEntity = true;
			this->viewPitch = this->destPitch;
		}
	}
	else if (this->viewPitch > this->destPitch) {
		this->viewPitch -= this->pitchStep;
		if (this->viewPitch < this->destPitch) {
			this->updateFacingEntity = true;
			this->viewPitch = this->destPitch;
		}
	}

	int viewZ = this->viewZ;
	if (this->knockbackDist != 0) {
		int n = this->viewX;
		if (this->knockbackX == 0) {
			n = this->viewY;
		}
		viewZ = this->viewZ + (10 * app->render->sinTable[(std::abs(n - this->knockbackStart) << 9) / this->knockbackWorldDist & 0x3FF] >> 16);
	}

	if (this->state == Canvas::ST_AUTOMAP) {
		this->viewX = this->destX;
		this->viewY = this->destY;
		this->viewZ = this->destZ;
		this->viewAngle = this->destAngle;
		this->viewPitch = this->destPitch;
	}

	if (this->state == Canvas::ST_COMBAT) {
		app->game->gsprite_update(app->time);
	}
	if (app->game->gotoTriggered) {
		app->game->gotoTriggered = false;
		int flagForFacingDir = this->flagForFacingDir(8);
		app->game->eventFlagsForMovement(-1, -1, -1, -1);
		app->game->executeTile(this->destX >> 6, this->destY >> 6, app->game->eventFlags[1], true);
		app->game->executeTile(this->destX >> 6, this->destY >> 6, flagForFacingDir, true);
	}
	else if (!b && this->viewX == this->destX && this->viewY == this->destY) {
		this->finishMovement();
	}

	if (!b2 && this->viewAngle == this->destAngle) {
		this->finishRotation(true);
	}

	if (app->game->isCameraActive() && this->state != Canvas::ST_INTER_CAMERA) {
		app->game->activeCamera->Update(app->game->activeCameraKey, app->gameTime - app->game->activeCameraTime);
		app->game->activeCamera->Render();
		this->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
		return;
	}

	if (this->isZoomedIn) {
		int n2 = this->zoomAccuracy * app->render->sinTable[(app->time - this->zoomStateTime) / 2 & 0x3FF] >> 24;
		int n3 = this->zoomAccuracy * app->render->sinTable[(app->time - this->zoomStateTime) / 3 & 0x3FF] >> 24;
		int zoomFOV = this->zoomFOV;
		int n4;
		if (app->time < this->zoomTime) {
			n4 = this->zoomDestFOV + (this->zoomFOV - this->zoomDestFOV) * (this->zoomTime - app->time) / 360;
		}
		else {
			this->zoomTime = 0;
			n4 = (this->zoomFOV = this->zoomDestFOV);
		}
		int n5 = this->zoomPitch + this->viewPitch;
		if (app->combat->curAttacker == nullptr && this->state == Canvas::ST_COMBAT && app->combat->nextStage == 1) {
			n5 += app->render->sinTable[512 * ((app->gameTime - app->combat->animStartTime << 16) / app->combat->animTime) >> 16 & 0x3FF] * 28 >> 16;
		}
		this->renderScene(this->viewX, this->viewY, this->viewZ, this->viewAngle + this->zoomAngle + n2, n5 + n3, this->viewRoll, n4);
		this->updateFacingEntity = true;
	}
	else if (this->loadMapID != 0) {
		this->renderScene(this->viewX, this->viewY, viewZ, this->viewAngle, this->viewPitch, this->viewRoll, 290);
	}
}

void Canvas::clearSoftKeys() {
	this->softKeyLeftID = -1;
	this->softKeyRightID = -1;
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::clearLeftSoftKey() {
	this->softKeyLeftID = -1;
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::clearRightSoftKey() {
	this->softKeyRightID = -1;
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::setLeftSoftKey(short i, short i2) {
	if (!this->displaySoftKeys) {
		return;
	}
	this->softKeyLeftID = Localization::STRINGID(i, i2);
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::setRightSoftKey(short i, short i2) {
	if (!this->displaySoftKeys) {
		return;
	}
	this->softKeyRightID = Localization::STRINGID(i, i2);
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::setSoftKeys(short n, short n2, short n3, short n4) {
	if (!this->displaySoftKeys) {
		return;
	}
	this->softKeyLeftID = Localization::STRINGID(n, n2);
	this->softKeyRightID = Localization::STRINGID(n3, n4);
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
	this->checkHudEvents();
}

void Canvas::checkHudEvents() {

	//app->hud->hudEventsAvailable = (this->displaySoftKeys && (this->softKeyLeftID == 52 || this->softKeyRightID == 52)); // J2ME Only
}

void Canvas::drawSoftKeys(Graphics* graphics) {

	// J2ME Only
}

void Canvas::setLoadingBarText(short loadingStringID, short loadingStringType) {
	this->loadingStringID = loadingStringID;
	this->loadingStringType = loadingStringType;
	this->loadingFlags |= 0x3;
}

void Canvas::updateLoadingBar(bool b) {

	int uVar2;

	if (b == false) {
		if (app->upTimeMs - this->lastPacifierUpdate < 0x96) {
			return;
		}
		this->lastPacifierUpdate = app->upTimeMs;
	}
	uVar2 = this->loadingFlags;
	this->loadingFlags = uVar2 | 3;
	if ((this->loadingStringID == -1) || (this->loadingStringType == -1)) {
		this->setLoadingBarText((short)0, (short)57);
	}
	this->loadingFlags = uVar2 | 3;
	this->repaintFlags |= Canvas::REPAINT_LOADING_BAR;
	return;
}

void Canvas::drawLoadingBar(Graphics* graphics) {

	Text* text;
	int iVar1;
	int iVar2;
	int iVar3;
	int uVar4;
	int iVar5;
	int iVar6;

	if ((this->loadingFlags & 3) != 0) {
		iVar1 = this->SCR_CX;
		iVar6 = this->SCR_CY;
		if ((this->loadingFlags & 1) != 0) {
			text = app->localization->getSmallBuffer();
			this->loadingFlags &= 0xfffffffe;
			graphics->eraseRgn(this->displayRect);
			graphics->fillRegion(this->imgFabricBG, iVar1 + -0x4b, iVar6 + -0x1d, 0x96, 0x3a);
			graphics->setColor(0xffffffff);
			graphics->drawRect(iVar1 + -0x4b, iVar6 + -0x1d, 0x96, 0x3a);
			app->localization->composeText(this->loadingStringID, this->loadingStringType, text);
			text->dehyphenate();
			graphics->drawString(text, this->SCR_CX, this->SCR_CY + -0x16, 0x11);
			text->setLength(0);
			app->localization->composeText(0, 58, text);
			text->dehyphenate();
			graphics->drawString(text, this->SCR_CX, this->SCR_CY + -6, 0x11);
			text->dispose();
			iVar1 = this->SCR_CX;
			iVar6 = this->SCR_CY;
		}
		this->loadingFlags &= 0xfffffffd;
		iVar2 = iVar1 + 0x3e;
		iVar5 = this->pacifierX + 10;
		this->pacifierX = iVar5;
		iVar3 = iVar2;
		if (iVar2 <= iVar5) {
			iVar3 = iVar1 + -0x42;
		}
		if ((iVar2 <= iVar5) || (iVar3 = iVar1 + -0x42, iVar5 < iVar3)) {
			this->pacifierX = iVar3;
		}
		graphics->setColor(-0x1000000);
		graphics->fillRect(iVar1 + -0x43, iVar6 + 0xb, 0x86, 0xc);
		graphics->setColor(-1);
		graphics->drawRect(iVar1 + -0x43, iVar6 + 0xb, 0x86, 0xc);
		iVar3 = this->pacifierX;
		iVar1 = (iVar1 + 0x43) - iVar3;
		if (0x19 < iVar1) {
			iVar1 = 0x1a;
		}
		iVar2 = ((iVar3 / 10) / 6) * 8;
		uVar4 = (iVar3 / 10) % 6;
		if (uVar4 < 3) {
			iVar2 = 6;
		}
		if (2 < uVar4) {
			iVar2 = 0;
		}
		graphics->drawRegion(this->imgLoadingFire, 0, 0, iVar1, 9, iVar3, iVar6 + 0xd, 0, iVar2, 0);
	}
}

void Canvas::unloadMedia() {

	this->freeRuntimeData();
	app->game->unloadMapData();
	app->render->unloadMap();
}

void Canvas::invalidateRect() {
	this->staleView = true;
}

int Canvas::getRecentLoadType() {
	if (this->recentBriefSave) {
		return 2;
	}
	return 1;
}

void Canvas::initZoom() {

	this->zoomTime = 0;
	this->zoomCurFOVPercent = 0;
	this->zoomFOV = this->zoomDestFOV = 190;
	this->zoomAngle = 0;
	this->zoomPitch = 0;
	this->zoomTurn = 0;
	this->viewPitch = this->destPitch = 0;
	this->zoomStateTime = app->time;
	this->isZoomedIn = true;
	app->StartAccelerometer();
	this->m_sniperScopeDialScrollButton->Update(0, Applet::IOS_HEIGHT);
	this->zoomAccuracy = 2560 * std::max(0, std::min((256 - app->player->ce->getStatPercent(Enums::STAT_ACCURACY) << 8) / 26, 256)) >> 8;
	this->zoomMinFOVPercent = 256;
	this->zoomMaxAngle = 64 - (this->zoomAccuracy >> 8);
	app->render->startFade(500, 2);
	this->drawPlayingSoftKeys();

	this->sdlGL->centerMouse(0, -22); // [GEC]
}

void Canvas::zoomOut() {

	this->isZoomedIn = false;
	this->viewAngle += this->zoomAngle;
	int n = 255;
	this->destAngle = this->viewAngle = ((this->viewAngle + (n >> 1)) & ~n);
	app->render->startFade(500, 2);
	this->finishRotation(true);
	app->tinyGL->resetViewPort();
	this->drawPlayingSoftKeys();
}

bool Canvas::handleZoomEvents(int key, int action) {
	return this->handleZoomEvents(key, action, false);
}

bool Canvas::handleZoomEvents(int key, int action, bool b) {

	if (!b && ((this->zoomTime != 0) || app->game->activePropogators != 0 || app->game->animatingEffects != 0 || !app->game->snapMonsters(false))) {
		return true;
	}
	int n3 = 5 + ((20 * (256 - this->zoomCurFOVPercent)) >> 8);
	if (action == Enums::ACTION_MENU || action == Enums::ACTION_BACK) {
		this->zoomOut();
		return true;
	}
	if (action == Enums::ACTION_AUTOMAP) {
		if (!app->player->inTargetPractice) {
			app->hud->msgCount = 0;
			app->menuSystem->setMenu(Menus::MENU_INGAME);
			return true;
		}
	}
	else if (action == Enums::ACTION_RIGHT) {
		this->zoomAngle -= n3;
		this->updateFacingEntity = true;
		++this->zoomTurn;
		this->sdlGL->centerMouse(0, -22); // [GEC]
		this->app->StopAccelerometer(); // [GEC]
	}
	else if (action == Enums::ACTION_LEFT) {
		this->zoomAngle += n3;
		this->updateFacingEntity = true;
		++this->zoomTurn;
		this->sdlGL->centerMouse(0, -22); // [GEC]
		this->app->StopAccelerometer(); // [GEC]
	}
	else if (action == Enums::ACTION_DOWN) {
		this->zoomPitch -= n3;
		++this->zoomTurn;
		this->sdlGL->centerMouse(0, -22); // [GEC]
		this->app->StopAccelerometer(); // [GEC]
	}
	else if (action == Enums::ACTION_UP) {
		this->zoomPitch += n3;
		++this->zoomTurn;
		this->sdlGL->centerMouse(0, -22); // [GEC]
		this->app->StopAccelerometer(); // [GEC]
	}
	else if (action == Enums::ACTION_PASSTURN) {
		app->hud->addMessage((short)45);
		app->game->touchTile(this->destX, this->destY, false);
		app->game->advanceTurn();
		this->invalidateRect();
		this->zoomTurn = 0;
	}
	else if (action == Enums::ACTION_NEXTWEAPON) {
		if (this->zoomCurFOVPercent < this->zoomMinFOVPercent) {
			this->zoomCurFOVPercent += 64;
			this->zoomCurFOVPercent = std::min(this->zoomCurFOVPercent, this->zoomMinFOVPercent); // [GEC]
			++this->zoomTurn;
			this->zoomDestFOV = 190 + ((-55 * this->zoomCurFOVPercent) >> 7);
			this->zoomDestFOV = std::max(this->zoomDestFOV, 102);
			this->zoomTime = app->time + 360;

			// [GEC] update scroll bar
			{
				float maxScroll = (float)((this->m_sniperScopeDialScrollButton->barRect).h - this->m_sniperScopeDialScrollButton->field_0x4c_);
				float curFOV = (float)((float)this->zoomCurFOVPercent / (float)this->zoomMinFOVPercent);
				this->m_sniperScopeDialScrollButton->field_0x48_ = (int)(maxScroll - (maxScroll * curFOV));
				app->sound->playSound(Sounds::getResIDByName(SoundName::SLIDER), 0, 5, false);
			}

		}
	}
	else if (action == Enums::ACTION_PREVWEAPON) {
		if (this->zoomCurFOVPercent > 0) {
			this->zoomCurFOVPercent -= 64;
			this->zoomCurFOVPercent = std::max(this->zoomCurFOVPercent, 0); // [GEC]
			++this->zoomTurn;
			this->zoomDestFOV = 190 + ((-55 * this->zoomCurFOVPercent) >> 7);
			this->zoomTime = app->time + 360;

			// [GEC] update scroll bar
			{
				float maxScroll = (float)((this->m_sniperScopeDialScrollButton->barRect).h - this->m_sniperScopeDialScrollButton->field_0x4c_);
				float curFOV = (float)((float)this->zoomCurFOVPercent / (float)this->zoomMinFOVPercent);
				this->m_sniperScopeDialScrollButton->field_0x48_ = (int)(maxScroll - (maxScroll * curFOV));
				app->sound->playSound(Sounds::getResIDByName(SoundName::SLIDER), 0, 5, false);
			}
		}
		++this->zoomTurn;
	}
	else if (action == Enums::ACTION_FIRE) {
		this->zoomTurn = 0;
		return handlePlayingEvents(key, action);
	}

	if (this->zoomPitch < -this->zoomMaxAngle) {
		this->zoomPitch = -this->zoomMaxAngle;
	}
	else if (this->zoomPitch > this->zoomMaxAngle) {
		this->zoomPitch = this->zoomMaxAngle;
	}
	if ((this->zoomTurn & 0x7) == 0x7) {
		app->game->advanceTurn();
	}
	return true;
}

void Canvas::setMenuDimentions(int x, int y, int w, int h)
{
	this->menuRect[0] = x;
	this->menuRect[1] = y;
	this->menuRect[2] = w;
	this->menuRect[3] = h;
}

void Canvas::setBlendSpecialAlpha(float alpha) {

	if (alpha < 0.0f) {
		alpha = 0.0f;
	}
	else if (alpha > 1.0f) {
		alpha = 1.0f;
	}

	this->blendSpecialAlpha = alpha;
}

void Canvas::touchStart(int pressX, int pressY) {

	this->touched = false;

	if (this->state == Canvas::ST_MENU) {
		app->menuSystem->handleUserTouch(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_LOOTING) {
		this->handleEvent(6);
	}
	else if ((this->state == Canvas::ST_PLAYING) || (this->state == Canvas::ST_COMBAT)) {
		if (!this->isZoomedIn) {
			app->hud->handleUserTouch(pressX, pressY, true);

			if (!app->hud->isInWeaponSelect) {
				this->m_controlButtons[this->m_controlMode + 0]->HighlightButton(pressX, pressY, true);
				this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(pressX, pressY, true);
				this->m_controlButtons[this->m_controlMode + 4]->HighlightButton(pressX, pressY, true);

				this->m_controlButton = nullptr;
				fmButton* button;
				if (!this->isZoomedIn && (
					(button = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButton(pressX, pressY)) ||
					(button = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButton(pressX, pressY)) ||
					(button = this->m_controlButtons[this->m_controlMode + 4]->GetTouchedButton(pressX, pressY))) &&
					(button->buttonID != 6)) {
					this->m_controlButton = button;
					this->m_controlButtonIsTouched = true;
				}
				else if (this->m_swipeArea[this->m_controlMode]->rect.ContainsPoint(pressX, pressY))
				{
					this->m_swipeArea[this->m_controlMode]->touched = true;
					this->m_swipeArea[this->m_controlMode]->begX = pressX;
					this->m_swipeArea[this->m_controlMode]->begY = pressY;
					this->m_swipeArea[this->m_controlMode]->curX = -1;
					this->m_swipeArea[this->m_controlMode]->curY = -1;
					return;
				}
				this->m_swipeArea[this->m_controlMode]->touched = false;
			}
		}
		else {
			if (this->m_sniperScopeDialScrollButton->field_0x0_ &&
				this->m_sniperScopeDialScrollButton->barRect.ContainsPoint(pressX, pressY)) {
				this->m_sniperScopeDialScrollButton->SetTouchOffset(pressX, pressY);
				this->m_sniperScopeDialScrollButton->field_0x14_ = 1;
			}
			else {
				app->hud->handleUserTouch(pressX, pressY, true);
				if (!app->hud->isInWeaponSelect) {
					this->m_sniperScopeButtons->HighlightButton(pressX, pressY, true);
				}
			}
		}
	}
	else if (this->state == Canvas::ST_AUTOMAP) {
		//puts("touch in automap!");
		this->m_softKeyButtons->HighlightButton(pressX, pressY, true);
		this->m_controlButtons[this->m_controlMode + 0]->HighlightButton(pressX, pressY, true);
		this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(pressX, pressY, true);
		this->m_controlButton = nullptr;
		fmButton* button;
		if (((button = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButton(pressX, pressY)) ||
			(button = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButton(pressX, pressY))) &&
			(button->buttonID != 6)) {
			this->m_controlButton = button;
			this->m_controlButtonIsTouched = true;
		}
	}
	else if (this->state == Canvas::ST_MIXING) {
		this->m_mixingButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_INTRO) {
		this->m_storyButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_DIALOG) {
		this->m_dialogButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_TREADMILL) {
		this->m_treadmillButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_CHARACTER_SELECTION) {
		this->m_characterButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_MINI_GAME) {
		IMinigame* mg = CAppContainer::getInstance()->minigameRegistry.getById(app->canvas->stateVars[0]);
		if (mg) mg->touchStart(pressX, pressY);
	}
	else if (this->state == Canvas::ST_CAMERA) { // [GEC ]Port: New
		this->m_softKeyButtons->HighlightButton(pressX, pressY, true);
	}
}

void Canvas::touchMove(int pressX, int pressY) {

	fmSwipeArea::SwipeDir swDir;

	//this->touched = true; // Old

	// [GEC] Evita falsos toques en la pantalla
	const int begMouseX = (int)(gBegMouseX * Applet::IOS_WIDTH);
	const int begMouseY = (int)(gBegMouseY * Applet::IOS_HEIGHT);
	if (!pointInRectangle(pressX, pressY, begMouseX - 3, begMouseY - 3, 6, 6)) {
		this->touched = true;
	}

	if (this->state == Canvas::ST_MENU) {
		app->menuSystem->handleUserMoved(pressX, pressY);
	}
	else if ((this->state == Canvas::ST_PLAYING) || (this->state == Canvas::ST_COMBAT)) {

		if (this->isZoomedIn) {
			if (this->m_sniperScopeDialScrollButton->field_0x14_) {
				this->m_sniperScopeDialScrollButton->Update(pressX, pressY);
				int field_0x44 = this->m_sniperScopeDialScrollButton->field_0x44_;
				if (!this->m_sniperScopeDialScrollButton->field_0x0_ || (field_0x44 <= 2)) {
					field_0x44 = 3;
				}
				this->zoomDestFOV = 110 * field_0x44 / 15 + 80;

				// [GEC] update zoomCurFOVPercent
				{
					float maxScroll = (float)((this->m_sniperScopeDialScrollButton->barRect).h - this->m_sniperScopeDialScrollButton->field_0x4c_);
					float yScroll = (float)((float)this->m_sniperScopeDialScrollButton->field_0x48_ / maxScroll);
					this->zoomCurFOVPercent = this->zoomMinFOVPercent - (int)((float)this->zoomMinFOVPercent * yScroll);
				}
			}
			else {
				if (this->m_sniperScopeDialScrollButton->field_0x0_
					&& this->m_sniperScopeDialScrollButton->barRect.ContainsPoint(pressX, pressY))
				{
					this->m_sniperScopeDialScrollButton->SetTouchOffset(pressX, pressY);
					this->m_sniperScopeDialScrollButton->field_0x14_ = 1;
					return;
				}

				app->hud->handleUserMoved(pressX, pressY);
				if (!app->hud->isInWeaponSelect) {
					this->m_sniperScopeButtons->HighlightButton(pressX, pressY, true);
				}
			}
		}
		else {
			app->hud->handleUserMoved(pressX, pressY);
			if (!app->hud->isInWeaponSelect) {
				if (this->m_swipeArea[this->m_controlMode]->touched) {
					if (this->m_swipeArea[this->m_controlMode]->UpdateSwipe(pressX, pressY, &swDir)) {
						this->touched = true;
						this->touchSwipe(swDir);
						return;
					}
				}
				else {
					this->m_controlButtons[this->m_controlMode + 0]->HighlightButton(pressX, pressY, true);
					this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(pressX, pressY, true);
					this->m_controlButtons[this->m_controlMode + 4]->HighlightButton(pressX, pressY, true);

					if (this->m_controlButton && !this->m_controlButton->highlighted) {
						this->m_controlButton = nullptr;
					}
					this->m_controlButton = nullptr;

					fmButton* button;
					if (!this->isZoomedIn && (
						(button = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButton(pressX, pressY)) ||
						(button = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButton(pressX, pressY)) ||
						(button = this->m_controlButtons[this->m_controlMode + 4]->GetTouchedButton(pressX, pressY))) &&
						(button->buttonID != 6)) {
						this->m_controlButton = button;
						this->m_controlButtonIsTouched = true;
					}
				}
			}
		}
	}
	else if (this->state == Canvas::ST_AUTOMAP) {
		this->m_softKeyButtons->HighlightButton(pressX, pressY, 1);
		this->m_controlButtons[this->m_controlMode + 0]->HighlightButton(pressX, pressY, true);
		this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(pressX, pressY, true);

		if (this->m_controlButton && !this->m_controlButton->highlighted) {
			this->m_controlButton = nullptr;
		}
		this->m_controlButton = nullptr;

		fmButton* button;
		if (((button = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButton(pressX, pressY)) ||
			(button = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButton(pressX, pressY))) &&
			(button->buttonID != 6)) {
			this->m_controlButton = button;
			this->m_controlButtonIsTouched = true;
		}
	}
	else if (this->state == Canvas::ST_MIXING) {
		this->m_mixingButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_INTRO) {
		this->m_storyButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_DIALOG) {
		this->m_dialogButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_TREADMILL) {
		this->m_treadmillButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_CHARACTER_SELECTION) {
		this->m_characterButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_MINI_GAME) {
		IMinigame* mg = CAppContainer::getInstance()->minigameRegistry.getById(app->canvas->stateVars[0]);
		if (mg) mg->touchMove(pressX, pressY);
	}
	else if (this->state == Canvas::ST_CAMERA) { // [GEC]: New
		this->m_softKeyButtons->HighlightButton(pressX, pressY, true);
	}
}

void Canvas::touchEnd(int pressX, int pressY) {


	short sVar1;
	int iVar3;
	int state;
	int uVar5;

	state = this->state;
	//printf("state %d\n", state);
	if (this->state == Canvas::ST_MENU) {
		app->menuSystem->handleUserTouch(pressX, pressY, false);
		return;
	}

	if (this->state == Canvas::ST_INTRO) {
		state = this->m_storyButtons->GetTouchedButtonID(pressX, pressY);
		if (state != 1) {
			if (state == 2) {
				if (this->storyPage >= this->storyTotalPages - 1) {
					return;
				}
			}
			else if (state != 0) {
				return;
			}
		}
		this->stateVars[0] = state;
	LAB_00022b00:
		state = this->numEvents;
		if (state == 4) {
			return;
		}
		iVar3 = 6;
	}
	else {
		if (this->state != Canvas::ST_DIALOG) {
			if (this->state == Canvas::ST_AUTOMAP) {
				this->m_controlButton = nullptr;
				state = this->m_softKeyButtons->GetTouchedButtonID(pressX, pressY);
				this->m_controlButtonTime = app->gameTime + -1;
			}
			else {
				if (this->state == Canvas::ST_COMBAT || this->state == Canvas::ST_PLAYING) {
					state = this->touchToKey_Play(pressX, pressY);
				}
				else {
					if (this->state == Canvas::ST_CAMERA) {
						if (app->canvas->softKeyRightID != -1) { // [GEC]: New
							this->m_softKeyButtons->HighlightButton(pressX, pressY, true);
						}

						if ((app->canvas->softKeyRightID != -1) &&
							(state = this->m_softKeyButtons->GetTouchedButtonID(pressX, pressY),
								state == 20)) {
						LAB_00022c60:
							
							state = 6;
							goto LAB_00022c64;
						}
					}
					else {
						if (this->state == Canvas::ST_MINI_GAME) {
							IMinigame* mg = CAppContainer::getInstance()->minigameRegistry.getById(app->canvas->stateVars[0]);
							if (mg) mg->touchEnd(pressX, pressY);
						}
						else {
							if (this->state == Canvas::ST_CHARACTER_SELECTION) { // [GEC]
								state = AVK_PASSTURN;
								goto LAB_00022c64;
							}
							if (this->state != Canvas::ST_TREADMILL) goto LAB_00022c60;
							state = this->m_treadmillButtons->GetTouchedButtonID(pressX, pressY);
							if (state == 0) {
								state = this->numEvents;
								if (state != 4) {
									iVar3 = 2;
								LAB_00022c48:
									this->events[state] = iVar3;
									this->numEvents = state + 1;
									this->keyPressedTime = app->upTimeMs;
								}
							}
							else {
								if (state == 1) {
									state = this->numEvents;
									if (state != 4) {
										iVar3 = 4;
										goto LAB_00022c48;
									}
								}
							}
						}
					}
					state = -1;
				}
			}
		LAB_00022c64:
			this->m_controlButtonIsTouched = false;
			if (state == -1) {
				return;
			}
			iVar3 = this->numEvents;
			if (iVar3 == 4) {
				return;
			}
			this->events[iVar3] = state;
			this->numEvents = iVar3 + 1;
			state = app->upTimeMs;
			goto LAB_00022ca4;
		}
		state = this->m_dialogButtons->GetTouchedButtonID(pressX, pressY);
		if (state - 7U < 2) {
			state = 6;
		LAB_00022944:
			if (this->currentDialogLine < this->numDialogLines - this->dialogViewLines) goto LAB_00022980;
			uVar5 = this->dialogFlags;
			if ((uVar5 & 2) != 0) {
				return;
			}
			if ((uVar5 & 4) != 0) {
				return;
			}
			if ((uVar5 & 1) != 0) {
				return;
			}
		LAB_00022af8:
			//pCVar4 = *Applet;
			goto LAB_00022b00;
		}
		if (state == 6) goto LAB_00022944;
	LAB_00022980:
		if ((1 < state - 5U) || (this->numDialogLines <= this->dialogViewLines)) {
			uVar5 = this->dialogFlags;
			if ((uVar5 & 2) == 0) {
				if (uVar5 == 0) {
					return;
				}
				if (this->currentDialogLine < this->numDialogLines - this->dialogViewLines) {
					return;
				}
				if (((uVar5 & 4) == 0) && ((uVar5 & 1) == 0)) {
					return;
				}
				if (1 < state - 3U) {
					return;
				}
				if (state == 3) {
					app->game->scriptStateVars[4] = 0;
				}
				else {
					app->game->scriptStateVars[4] = 1;
				}
			}
			else {
				iVar3 = this->dialogStyle;
				sVar1 = (short)state;
				if (iVar3 == 11) {
					if (state == 0) {
						app->game->scriptStateVars[4] = sVar1;
					}
					else {
						if (state != 1) {
							return;
						}
						app->game->scriptStateVars[4] = sVar1;
					}
					iVar3 = this->numEvents;
					if (iVar3 != 4) {
						this->events[iVar3] = 6;
						this->numEvents = iVar3 + 1;
						this->keyPressedTime = app->upTimeMs;
					}
					iVar3 = this->dialogStyle;
				}
				if (iVar3 != 10) {
					return;
				}
				if (state == 0) {
					app->game->scriptStateVars[4] = sVar1;
				}
				else {
					if (state != 1) {
						return;
					}
					app->game->scriptStateVars[4] = sVar1;
				}
			}
			goto LAB_00022af8;
		}
		if (state != 5) goto LAB_00022af8;
		state = this->numEvents;
		if (state == 4) {
			return;
		}
		iVar3 = 19;
	}
	this->events[state] = iVar3;
	this->numEvents = state + 1;
	state = app->upTimeMs;
LAB_00022ca4:
	this->keyPressedTime = state;
	return;
}

void Canvas::touchEndUnhighlight() {

	if (this->state == Canvas::ST_MIXING) {
		this->m_mixingButtons->HighlightButton(0, 0, false);
	}
	else if (this->state == Canvas::ST_INTRO) {
		this->m_storyButtons->HighlightButton(0, 0, false);
	}

	if (this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_COMBAT || this->state == Canvas::ST_AUTOMAP || this->state == Canvas::ST_DIALOG || this->state == Canvas::ST_CAMERA) {
		this->m_controlButtons[this->m_controlMode]->HighlightButton(0, 0, false);
		this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(0, 0, false);
		this->m_controlButtons[this->m_controlMode + 4]->HighlightButton(0, 0, false);
		this->m_sniperScopeButtons->HighlightButton(0, 0, false);
		this->m_softKeyButtons->HighlightButton(0, 0, false);
		this->m_dialogButtons->HighlightButton(0, 0, false);
		this->m_characterButtons->HighlightButton(0, 0, false);
	}
	else if (this->state == Canvas::ST_TREADMILL) {
		this->m_treadmillButtons->HighlightButton(0, 0, false);
	}
}

bool Canvas::pitchIsControlled(int n, int n2, int n3) {


	bool b = false;
	for (int i = 0; i < Render::MAX_KEEP_PITCH_LEVEL_TILES; ++i) {
		if (app->render->mapKeepPitchLevelTiles[i] != -1) {
			short n4 = app->render->mapKeepPitchLevelTiles[i];
			if ((n2 << 5) + n == (n4 & 0x3FF) && n3 == (n4 & 0xFFFFFC00) >> 10) {
				b = true;
				break;
			}
		}
	}
	return b;
}

int Canvas::touchToKey_Play(int pressX, int pressY) {

	int result;

	this->m_controlButton = 0;
	if (this->m_sniperScopeDialScrollButton->field_0x14_)
	{
		this->m_sniperScopeDialScrollButton->field_0x14_ = 0;
		return -1;
	}
	this->m_swipeArea[this->m_controlMode]->touched = false;
	if (((pressY > 256)) || (app->hud->isInWeaponSelect))
	{
		app->hud->handleUserTouch(pressX, pressY, false);
		return -1;
	}
	if (this->touched) {
		return -1;
	}
	if (this->isZoomedIn) {
		return this->m_sniperScopeButtons->GetTouchedButtonID(pressX, pressY);
	}
	if (this->m_controlButtonIsTouched) {
		return -1;
	}

	result = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButtonID(pressX, pressY);
	if (result == -1)
	{
		result = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButtonID(pressX, pressY);
		if (result == -1) {
			result = this->m_controlButtons[this->m_controlMode + 4]->GetTouchedButtonID(pressX, pressY);
		}
	}

	if (result != 6) {
		return -1;
	}

	return result;
}

void Canvas::drawTouchSoftkeyBar(Graphics* graphics, bool highlighted_Left, bool highlighted_Right) {

	Text* SmallBuffer = app->localization->getSmallBuffer();

	graphics->drawImage(app->menuSystem->imgGameMenuPanelbottom,
		0, Applet::IOS_HEIGHT - app->menuSystem->imgGameMenuPanelbottom->height, 0, 0, 0);
	if (app->player->isFamiliar) {
		graphics->drawImage(app->menuSystem->imgGameMenuPanelBottomSentrybot,
			0, Applet::IOS_HEIGHT - app->menuSystem->imgGameMenuPanelBottomSentrybot->height, 0, 0, 0);
	}

	if (!highlighted_Left || (highlighted_Left && this->softKeyLeftID == -1)) {
		graphics->drawImage(app->hud->imgSwitchLeftNormal, 9, 268, 0, 0, 0);
	}
	else {
		graphics->drawImage(app->hud->imgSwitchLeftActive, 9, 268, 0, 0, 0);
	}

	if (this->softKeyLeftID != -1) {
		SmallBuffer->setLength(0);
		app->localization->composeText(this->softKeyLeftID, SmallBuffer);
		SmallBuffer->dehyphenate();
		graphics->drawString(SmallBuffer, 2, Applet::IOS_HEIGHT, 36);
	}

	if (!highlighted_Right || (highlighted_Right && this->softKeyRightID == -1)) {
		graphics->drawImage(app->hud->imgSwitchRightNormal, 438, 268, 0, 0, 0);
	}
	else {
		graphics->drawImage(app->hud->imgSwitchRightActive, 438, 268, 0, 0, 0);
	}

	if (this->softKeyRightID != -1) {
		SmallBuffer->setLength(0);
		app->localization->composeText(this->softKeyRightID, SmallBuffer);
		SmallBuffer->dehyphenate();
		graphics->drawString(SmallBuffer, Applet::IOS_WIDTH - 2, Applet::IOS_HEIGHT, 40);
	}
	SmallBuffer->dispose();
}

void Canvas::touchSwipe(int swDir) {

	int iVar1;
	int iVar2;

	if (this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_COMBAT) {
#if 0 // IOS
		switch (swDir) {
			case fmSwipeArea::SwipeDir::Left:
				iVar1 = 2;
				break;
			case fmSwipeArea::SwipeDir::Right:
				iVar1 = 4;
				break;
			case fmSwipeArea::SwipeDir::Down:
				iVar1 = AVK_RIGHT;
				break;
			case fmSwipeArea::SwipeDir::Up:
				iVar1 = AVK_LEFT;
				break;
			default:
				return;
		}

		if (this->isZoomedIn != false) {
			bool bVar4 = iVar1 == 2;
			if (bVar4) {
				iVar1 = 5;
			}
			if ((!bVar4) && (iVar1 == 4)) {
				iVar1 = 7;
			}
		}
#else
		switch (swDir) {
			case fmSwipeArea::SwipeDir::Left:
				iVar1 = AVK_MOVELEFT;
				break;
			case fmSwipeArea::SwipeDir::Right:
				iVar1 = AVK_MOVERIGHT;
				break;
			case fmSwipeArea::SwipeDir::Down:
				iVar1 = AVK_DOWN;
				break;
			case fmSwipeArea::SwipeDir::Up:
				iVar1 = AVK_UP;
				break;
			default:
				return;
		}
#endif

		if (this->numEvents != 4) {
			this->events[this->numEvents++] = iVar1;
			this->keyPressedTime = app->upTimeMs;
		}
	}
}

void Canvas::turnEntityIntoWaterSpout(Entity* entity) {

	int sprite = entity->getSprite();
	entity->def = app->entityDefManager->lookup(SpriteDefs::getIndex("water_spout"));
	entity->name = (short)(entity->def->name | 0x400);
	app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFFFF00) | SpriteDefs::getIndex("water_spout"));
	entity->info |= 0x400000;
}


void Canvas::flipControls() {
	int v1; // r10
	fmButtonContainer** v2; // r6
	int x; // r8
	fmButton* Button; // r5
	fmButton* v5; // r0
	bool v6; // zf
	fmButton* v7; // r4
	fmButton* v8; // r4
	fmButton* v9; // r0
	bool v10; // zf
	fmButton* v11; // r4
	fmButton* v12; // r0
	bool v13; // zf
	fmButton* v14; // r5
	int v15; // r8

	v1 = 0;
	v2 = &this->m_controlButtons[4];
	this->isFlipControls ^= 1u;
	do
	{
		v2[-4]->FlipButtons();
		v2[-2]->FlipButtons();
		v2[0]->FlipButtons();
		while (1)
		{
			Button = v2[-2]->GetButton(5);
			v5 = v2[-2]->GetButton(7);
			v6 = Button == 0;
			if (Button)
				v6 = v5 == 0;
			v7 = v5;
			if (v6)
				break;
			x = Button->touchArea.x;
			Button->SetTouchArea(v5->touchArea.x, Button->touchArea.y, Button->touchArea.w, Button->touchArea.h);
			v7->SetTouchArea(x, v7->touchArea.y, v7->touchArea.w, v7->touchArea.h);
			Button->buttonID = -2;
			v7->buttonID = -3;
		}
		while (1)
		{
			v8 = v2[-2]->GetButton(-2);
			v9 = v2[-2]->GetButton(-3);
			v10 = v8 == 0;
			if (v8)
				v10 = v9 == 0;
			if (v10)
				break;
			v8->buttonID = 5;
			v9->buttonID = 7;
		}
		v11 = v2[0]->GetButton(2);
		v12 = v2[0]->GetButton(4);
		v13 = v11 == 0;
		if (v11)
			v13 = v12 == 0;
		v14 = v12;
		if (!v13)
		{
			v15 = v11->touchArea.x;
			v11->SetTouchArea(v12->touchArea.x, v11->touchArea.y, v11->touchArea.w, v11->touchArea.h);
			v14->SetTouchArea(v15, v14->touchArea.y, v14->touchArea.w, v14->touchArea.h);
		}
		++v1;
		++v2;
	} while (v1 != 2);
}

void Canvas::setControlLayout() {
	if (this->m_controlLayout == 2) {
		this->m_controlGraphic = 0;
		this->m_controlMode = 1;
	}
	else {
		this->m_controlGraphic = this->m_controlLayout;
		this->m_controlMode = 0;
	}

	for (int i = 0; i < 2; i++) {
		this->m_controlButtons[i + 0]->SetGraphic(this->m_controlGraphic);
		this->m_controlButtons[i + 2]->SetGraphic(this->m_controlGraphic);
	}
}

void Canvas::addEvents(int event) { // [GEC]
	if (this->numEvents < Canvas::MAX_EVENTS) {
		//printf("numEvents %d Code %d\n", this->numEvents, event);
		this->events[0] = event;
		this->numEvents = 1;
	}
}
