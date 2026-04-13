#include <stdexcept>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include "Log.h"

#include "CAppContainer.h"
#include "DataNode.h"
#include "App.h"
#include "Hud.h"
#include "Game.h"
#include "Sound.h"
#include "Canvas.h"
#include "Player.h"
#include "MenuSystem.h"
#include "MenuStrings.h"
#include "CombatEntity.h"
#include "HackingGame.h"
#include "SentryBotGame.h"
#include "VendingMachine.h"
#include "MenuItem.h"
#include "Graphics.h"
#include "JavaStream.h"
#include "Resource.h"
#include "Button.h"
#include "Image.h"
#include "Sounds.h"
#include "Render.h"
#include "Combat.h"
#include "ComicBook.h"
#include "Enums.h"
#include "Input.h"
#include "Menus.h"
#include "SDLGL.h"
#include "Utils.h"
#include "TinyGL.h"
#include "GLES.h"
#include "SoundNames.h"
#include "Sounds.h"

MenuSystem::MenuSystem() = default;

MenuSystem::~MenuSystem() = default;

bool MenuSystem::startup() {
	this->app = CAppContainer::getInstance()->app;
	this->sdlGL = this->sdlGL;
	Applet* app = this->app;
	LOG_INFO("[menu] startup\n");

	LOG_INFO("[menu] loading from menus.yaml\n");
	if (!this->loadMenusFromYAML("menus.yaml")) {
		app->Error("Failed to load menus.yaml\n");
		return false;
	}

	this->numItems = 0;
	this->background = nullptr;
	this->menu = Menus::MENU_NONE;
	this->selectedIndex = 0;
	this->scrollIndex = 0;
	this->type = 0;
	this->maxItems = 0;
	this->cheatCombo = 0;
	this->goBackToStation = false;
	this->activeSlider = SliderMode::None;

	this->imgVendingButtonLarge = app->loadImage("vending_button_large.bmp", true);
	this->imgInGameMenuOptionButton = app->loadImage("inGame_menu_option_button.bmp", true);
	this->imgMenuButtonBackground = app->loadImage("menu_button_background.bmp", true);
	this->imgMenuButtonBackgroundOn = app->loadImage("menu_button_background_on.bmp", true);
	this->imgMenuArrowDown = app->loadImage("menu_arrow_down.bmp", true);
	this->imgMenuArrowUp = app->loadImage("menu_arrow_up.bmp", true);
	this->imgVendingArrowUpGlow = app->loadImage("vending_arrow_up_glow.bmp", true);
	this->imgVendingArrowDownGlow = app->loadImage("vending_arrow_down_glow.bmp", true);
	this->imgMenuDial = app->loadImage("Menu_Dial.bmp", true);
	this->imgMenuDialKnob = app->loadImage("Menu_Dial_Knob.bmp", true);
	this->imgMenuMainBOX = app->loadImage("Menu_Main_BOX.bmp", true);
	this->imgMainMenuOverLay = app->loadImage("Main_Menu_OverLay.bmp", true);
	this->imgMainHelpOverLay = app->loadImage("Main_Help_OverLay.bmp", true);
	this->imgMainAboutOverLay = app->loadImage("Main_About_OverLay.bmp", true);
	this->imgMenuYesNoBOX = app->loadImage("Menu_YesNo_BOX.bmp", true);
	this->imgMenuChooseDIFFBOX = app->loadImage("Menu_ChooseDIFF_BOX.bmp", true);
	this->imgMenuLanguageBOX = app->loadImage("Menu_Language_BOX.bmp", true);
	this->imgSwitchLeftNormal = app->loadImage("Switch_Left_Normal.bmp", true);
	this->imgSwitchLeftActive = app->loadImage("Switch_Left_Active.bmp", true);
	this->imgMenuOptionBOX3 = app->loadImage("Menu_Option_BOX3.bmp", true);
	this->imgMenuOptionBOX4 = app->loadImage("Menu_Option_BOX4.bmp", true);
	this->imgMenuOptionSliderBar = app->loadImage("Menu_Option_SliderBar.bmp", true);
	this->imgMenuOptionSliderOFF = app->loadImage("Menu_Option_SliderOFF.bmp", true);
	this->imgMenuOptionSliderON = app->loadImage("Menu_Option_SliderON.bmp", true);
	this->imgHudNumbers = app->loadImage("Hud_Numbers.bmp", true);
	this->imgGameMenuPanelbottom = app->loadImage("gameMenu_Panel_bottom.bmp", true);
	this->imgGameMenuPanelBottomSentrybot = app->loadImage("gameMenu_Panel_bottom_sentrybot.bmp", true);
	this->imgGameMenuHealth = app->loadImage("gameMenu_Health.bmp", true);
	this->imgGameMenuShield = app->loadImage("gameMenu_Shield.bmp", true);
	this->imgGameMenuInfoButtonPressed = app->loadImage("gameMenu_infoButton_Pressed.bmp", true);
	this->imgGameMenuInfoButtonNormal = app->loadImage("gameMenu_infoButton_Normal.bmp", true);
	this->imgVendingButtonHelp = app->loadImage("vending_button_help.bmp", true);
	this->imgGameMenuTornPage = app->loadImage("gameMenu_TornPage.bmp", true);
	this->imgGameMenuBackground = app->loadImage("gameMenu_Background.bmp", true);
	this->imgMainMenuDialA_Anim = app->loadImage("Main_Menu_DialA_anim.bmp", true);
	this->imgMainMenuDialC_Anim = app->loadImage("Main_Menu_DialC_anim.bmp", true);
	this->imgMainMenuSlide_Anim = app->loadImage("Main_Menu_Slide_anim.bmp", true);
	this->imgGameMenuScrollBar = app->loadImage("gameMenu_ScrollBar.bmp", true);
	this->imgGameMenuTopSlider = app->loadImage("gameMenu_topSlider.bmp", true);
	this->imgGameMenuMidSlider = app->loadImage("gameMenu_midSlider.bmp", true);
	this->imgGameMenuBottomSlider = app->loadImage("gameMenu_bottomSlider.bmp", true);
	this->imgVendingScrollBar = app->loadImage("vending_scroll_bar.bmp", true);
	this->imgVendingScrollButtonTop = app->loadImage("vending_scroll_button_top.bmp", true);
	this->imgVendingScrollButtonMiddle = app->loadImage("vending_scroll_button_middle.bmp", true);
	this->imgVendingScrollButtonBottom = app->loadImage("vending_scroll_button_bottom.bmp", true);
	this->imgLogo = app->loadImage("logo2.bmp", true);

	this->imgMenuButtonBackgroundExt = app->loadImage("menu_button_background.bmp", true); // [GEC]
	this->imgMenuButtonBackgroundExtOn = app->loadImage("menu_button_background_on.bmp", true); // [GEC]

	// [GEC] Fix the image
	fixImage(this->imgMenuButtonBackground);
	fixImage(this->imgMenuButtonBackgroundExt);

	enlargeButtonImage(this->imgMenuButtonBackgroundExt); // [GEC]
	enlargeButtonImage(this->imgMenuButtonBackgroundExtOn); // [GEC]

	this->imgMenuBtnBackground = this->imgMenuButtonBackground; // [GEC] Default
	this->imgMenuBtnBackgroundOn = this->imgMenuButtonBackgroundOn;  // [GEC] Default

	this->isMainMenuScrollBar = false;
	this->isMainMenu = false;
	this->menuItem_height = 46;
	this->menuItem_width = 162;
	this->menuItem_fontPaddingBottom = 0;
	this->menuItem_paddingBottom = 0;
	this->drawHelpText = false;
	this->selectedHelpIndex = -1;
	this->dialA_Anim1 = 0;
	this->dialC_Anim1 = 0;
	this->dialA_Anim2 = 0;
	this->dialC_Anim2 = 0;
	this->slideAnim1 = 0;
	this->slideAnim2 = 0;
	this->animTime = 0;

	// Load UI definitions (button containers + themes) from ui.yaml
	// Must be called after images are loaded since it references them for button creation
	LOG_INFO("[menu] loading from ui.yaml\n");
	if (!this->loadUIFromYAML("ui.yaml")) {
		app->Error("Failed to load ui.yaml\n");
		return false;
	}

	this->sliderID = 0;
	this->sfxVolumeScroll = 0;
	this->musicVolumeScroll = 0;
	this->alphaScroll = 0;
	this->vibrationIntensityScroll = 0; // [GEC]
	this->deadzoneScroll = 0; // [GEC]
	this->updateSlider = false;

	return true;
}


void MenuSystem::buildDivider(Text* text, int i) {
	int cnt = (i - (text->length() + 2)) / 2;
	if (text->length() > 0) {
		text->insert(' ', 0);
		text->append(' ');
	}
	else {
		text->append('\x80');
		text->append('\x80');
		text->append('\x80');
	}
	for (int j = 0; j < cnt; j++) {
		text->insert('\x80', 0);
		text->append('\x80');
	}
}

void MenuSystem::scrollDown() { // J2ME
	if ((this->items[this->selectedIndex].flags & 0x100) != 0x0) {
		this->shiftBlockText(this->selectedIndex, 1, this->maxItems);
	}
	else {
		this->soundClick(); // [GEC]
		this->moveDir(1);
	}
}

void MenuSystem::scrollUp() { // J2ME
	if ((this->items[this->selectedIndex].flags & 0x100) != 0x0) {
		this->shiftBlockText(this->selectedIndex, -1, this->maxItems);
	}
	else {
		this->soundClick(); // [GEC]
		this->moveDir(-1);
	}
}

bool MenuSystem::scrollPageDown() { // J2ME
	if ((this->items[this->selectedIndex].flags & 0x100) != 0x0) {
		return this->shiftBlockText(this->selectedIndex, this->maxItems, this->maxItems);
	}
	this->soundClick(); // [GEC]
	int selectedIndex = this->selectedIndex;
	for (int n = 0; n < this->maxItems && this->selectedIndex != this->numItems - 1; ++n) {
		this->moveDir(1);
		if (this->selectedIndex < selectedIndex) {
			this->moveDir(-1);
			break;
		}
	}
	return selectedIndex != this->selectedIndex;
}

void MenuSystem::scrollPageUp() { // J2ME
	if ((this->items[this->selectedIndex].flags & 0x100) != 0x0) {
		this->shiftBlockText(this->selectedIndex, -this->maxItems, this->maxItems);
	}
	else {
		this->soundClick(); // [GEC]
		int selectedIndex = this->selectedIndex;
		for (int n = 0; n < this->maxItems && this->selectedIndex != 0; ++n) {
			this->moveDir(-1);
			if (this->selectedIndex >= selectedIndex) {
				this->moveDir(1);
				break;
			}
		}
	}
}

bool MenuSystem::shiftBlockText(int n, int i, int j) { // J2ME

	if (n > this->numItems || (this->items[n].flags & 0x100) == 0x0) {
		return false;
	}
	Text* largeBuffer = app->localization->getLargeBuffer();
	app->localization->composeText(this->items[n].textField, largeBuffer);
	int n2 = this->items[n].param >> 26 & 0x3F;
	int b = this->items[n].param >> 20 & 0x3F;
	int n4;
	int n3 = n4 = (this->items[n].param & 0x3FF);
	if (i > 0) {
		for (i = std::min(i, n2 - (b + j)); i > 0; --i) {
			n3 = largeBuffer->findFirstOf('|', n3) + 1;
			++b;
		}
	}
	else {
		for (i = std::min(std::abs(i), b); i > 0; --i) {
			n3 = largeBuffer->findLastOf('|', n3 - 1) + 1;
			--b;
		}
	}
	int n5;
	for (n5 = n3; j > 0; --j, ++n5) {
		n5 = largeBuffer->findFirstOf('|', n5);
		if (n5 == -1) {
			n5 = largeBuffer->length();
			break;
		}
	}
	this->items[n].param = ((n2 & 0x3F) << 26 | (b & 0x3F) << 20 | (n5 - n3 & 0x3FF) << 10 | (n3 & 0x3FF));
	largeBuffer->dispose();
	return n3 != n4;
}

void MenuSystem::moveDir(int n) { // J2ME


	//this->menuMode = this->MODE_FULLREFRESH;
	if (this->type == 5 || this->type == 7) {
		if (n < 0 && this->scrollIndex > 0) {
			this->scrollIndex += n;
		}
		else if (n > 0 && this->scrollIndex < this->numItems - this->maxItems) {
			this->scrollIndex += n;
		}
		this->selectedIndex = this->scrollIndex;
	}
	else {
		do {
			this->selectedIndex += n;
			if (this->selectedIndex >= this->numItems || this->selectedIndex < 0) {
				if (this->selectedIndex < 0) {
					this->selectedIndex = this->numItems - 1;
				}
				else {
					this->selectedIndex = 0;
					if (this->type == 9) {
						this->scrollIndex = 0;
					}
				}
				while (this->items[this->selectedIndex].textField == this->EMPTY_TEXT || (this->items[this->selectedIndex].flags & 0x8001) != 0x0) {
					this->selectedIndex += n;
				}
				break;
			}
		} while (this->items[this->selectedIndex].textField == this->EMPTY_TEXT || (this->items[this->selectedIndex].flags & 0x8001) != 0x0);
		
		if (this->maxItems != 0 && n < 0) {
			if (this->selectedIndex - this->maxItems + 1 > this->scrollIndex) {
				this->scrollIndex = this->selectedIndex - this->maxItems + 1;
			}
			else if (this->selectedIndex < this->scrollIndex) {
				this->scrollIndex = this->selectedIndex;
			}
			else if (this->type == 9 && this->selectedIndex - 3 < this->scrollIndex && this->scrollIndex > 0) {
				--this->scrollIndex;
			}
		}
		else if (this->maxItems != 0) {
			if (this->selectedIndex > this->scrollIndex + this->maxItems - 1) {
				this->scrollIndex = this->selectedIndex - this->maxItems + 1;
			}
			else if (this->scrollIndex > this->selectedIndex) {
				this->scrollIndex = this->selectedIndex;
			}
			else if (this->type == 9 && this->selectedIndex + 3 > this->scrollIndex + this->maxItems - 1 && this->scrollIndex + this->maxItems < this->numItems) {
				++this->scrollIndex;
			}
		}
	}

	//printf("this->type %d\n", this->type);
	//printf("this->numItems %d\n", this->numItems);
	//printf("this->scrollIndex %d\n", this->scrollIndex);
	//printf("this->selectedIndex %d\n", this->selectedIndex);

	// [GEC] Actualiza la posicion del scroll touch
	if (this->m_scrollBar->field_0x0_ && this->menu != Menus::MENU_COMIC_BOOK) {
		int numItems = this->numItems;
		int maxScroll = (this->m_scrollBar->field_0x40_ - this->m_scrollBar->field_0x3c_);
		int maxScroll2 = ((this->m_scrollBar->barRect).h - this->m_scrollBar->field_0x4c_);
		int iVar4 = (maxScroll / numItems) * 2;
		int iVar2 = (maxScroll2 / numItems) * 2;

		int begItem = 0;
		int endItem = 0;
		int begY1 = 0;
		int begY2 = 0;

		for (int i = 0; i < numItems; i++) { // Ajusta la posici�n si es necesario
			if (!(this->items[i].flags & 0x8001)) {
				endItem = i;
			}
		}

		//printf("endItem %d\n", endItem);
		for (int i = 0; i < numItems; i++) { // Ajusta la posici�n si es necesario
			if (!(this->items[i].flags & 0x8001)) {
				begItem = i;
				break;
			}
			else {
				if (!(this->items[i].flags & 0x8000)) {
					begY1 += this->getMenuItemHeight2(i);
					begY2 += iVar2;

					//if ((this->items[i].flags & Menus::ITEM_SCROLLBAR) != 0) { // [GEC]
						//begY1 += 10 + Applet::FONT_HEIGHT[app->fontType];
					//}
				}
			}
		}

		this->m_scrollBar->field_0x44_ = 0;
		this->m_scrollBar->field_0x48_ = 0;

		if (this->type == 5 || this->type == 7) {
			this->m_scrollBar->field_0x44_ = this->selectedIndex * iVar4;
			this->m_scrollBar->field_0x48_ = this->selectedIndex * iVar2;
			this->m_scrollBar->field_0x44_ = std::min(this->m_scrollBar->field_0x44_, maxScroll);
			this->m_scrollBar->field_0x48_ = std::min(this->m_scrollBar->field_0x48_, maxScroll2);
		}
		else {
			int y1 = 0;
			int y2 = 0;

			if (this->selectedIndex == this->scrollIndex) {
				for (int i = 0; i < this->selectedIndex; i++) { // Ajusta la posici�n si es necesario
					if (!(this->items[i].flags & 0x8000)) {
						y1 += this->getMenuItemHeight2(i);
						y2 += iVar2;
					}
				}
			}

			for (int j = 0; j < this->scrollIndex; j++) {
				if (y1 == 0) { this->m_scrollBar->field_0x44_ += this->getMenuItemHeight2(this->selectedIndex) - y1; }
				else { this->m_scrollBar->field_0x44_ = y1; }
				if (y2 == 0) { this->m_scrollBar->field_0x48_ += iVar2; }
				else { this->m_scrollBar->field_0x48_ = y2; }
				this->m_scrollBar->field_0x44_ = std::min(this->m_scrollBar->field_0x44_, maxScroll);
				this->m_scrollBar->field_0x48_ = std::min(this->m_scrollBar->field_0x48_, maxScroll2);
			}

			if (this->selectedIndex == begItem) {  // Ajusta la posici�n si es necesario
				this->scrollIndex = 0;
				this->m_scrollBar->field_0x44_ -= begY1;
				this->m_scrollBar->field_0x48_ -= begY2;
				this->m_scrollBar->field_0x44_ = std::max(this->m_scrollBar->field_0x44_, 0);
				this->m_scrollBar->field_0x48_ = std::max(this->m_scrollBar->field_0x48_, 0);
			}

			if (this->old_0x48 != this->m_scrollBar->field_0x48_) {
				app->sound->playSound(Sounds::getResIDByName(SoundName::MENU_SCROLL), 0, 5, false); // [GEC]
			}

			this->old_0x44 = this->m_scrollBar->field_0x44_;
			this->old_0x48 = this->m_scrollBar->field_0x48_;
		}
	}
}


void MenuSystem::back() {

	//printf("back:: this->menu %d\n", this->menu);

	if ((this->menu == Menus::MENU_MAIN_BINDINGS) || (this->menu == Menus::MENU_INGAME_BINDINGS)) {
		// Apply changes
		std::memcpy(keyMapping, keyMappingTemp, sizeof(keyMapping));
	}

	if (this->menu != Menus::MENU_MAIN_MORE_GAMES && this->menu != Menus::MENU_MAIN) {
		if (!app->sound->isSoundPlaying(1122)) {
			app->sound->playSound(Sounds::getResIDByName(SoundName::SWITCH_EXIT), 0, 5, false); // [GEC]
		}
	}

	if (this->menu == Menus::MENU_MAIN_OPTIONS || this->menu == Menus::MENU_INGAME_OPTIONS) {
		this->leaveOptionsMenu();
	}

	if ((this->menu == Menus::MENU_MAIN_OPTIONS_VIDEO) || (this->menu == Menus::MENU_INGAME_OPTIONS_VIDEO)) { // [GEC]
		this->sdlGL->restore();
	}

	if (this->stackCount != 0) {
		if (this->stackCount - 1 < 0){
			this->app->Error("Menu stack is empty");
		}
		int y1;
		int y2;
		int index;
		this->setMenu(this->popMenu(this->poppedIdx, &y1, &y2, &index));
		this->selectedIndex = this->poppedIdx[0];
		this->m_scrollBar->field_0x44_ = y1; // [GEC]
		this->m_scrollBar->field_0x48_ = y2; // [GEC]
		this->scrollIndex = index; // [GEC]
	}
	else if (this->menu == Menus::MENU_INGAME || this->menu == Menus::MENU_ITEMS || this->menu == Menus::MENU_ITEMS_DRINKS ||
		this->menu == Menus::MENU_INGAME_QUESTLOG || this->menu == Menus::MENU_INGAME_SNIPER) {
		this->returnToGame();
	}
	else if (this->menu == Menus::MENU_MAIN_MINIGAME || this->menu == Menus::MENU_COMIC_BOOK) {
		this->setMenu(Menus::MENU_MAIN);
	}
}

void MenuSystem::setMenu(int menu) {


	LOG_INFO("[menu] menu {}\n", menu);
	this->cheatCombo = 0;
	this->menuMode = 0;
	if ((menu == Menus::MENU_MAIN_BEGIN || menu == Menus::MENU_INGAME) || (menu == Menus::MENU_INGAME_KICKING)) {
		this->clearStack();
	}
	if (menu == Menus::MENU_ENABLE_SOUNDS) {
		this->menuMode = 0;
	}

	// Restaura
	if (this->menu == Menus::MENU_DEBUG || this->menu == Menus::MENU_MAIN_BINDINGS || this->menu == Menus::MENU_INGAME_BINDINGS || this->menu == Menus::MENU_INGAME_OPTIONS_VIDEO) { // [GEC]
		this->old_0x44 = this->m_scrollBar->field_0x44_;
		this->old_0x48 = this->m_scrollBar->field_0x48_;
	}
	else {
		this->old_0x44 = 0; // [GEC]
		this->old_0x48 = 0; // [GEC]
	}
	/*this->old_0x44 = this->btnScroll->field_0x44_;
	this->old_0x48 = this->btnScroll->field_0x48_;
	if (this->oldMenu != menu) {
		this->old_0x44 = 0; // [GEC]
		this->old_0x48 = 0; // [GEC]
	}*/

	this->oldMenu = this->menu;
	this->menu = menu;

	//if (this->menu == Menus::MENU_DEBUG) { // [GEC]
		//this->oldMenu = menu;
	//}

	if (this->menu != Menus::MENU_NONE) {
		this->initMenu(this->menu);
		if (app->canvas->state != Canvas::ST_MENU) {
			app->canvas->setState(Canvas::ST_MENU);
		}
	}
	else {
		this->returnToGame();
		this->menu = Menus::MENU_NONE;
	}

	if (this->oldMenu == Menus::MENU_DEBUG || this->oldMenu == Menus::MENU_MAIN_BINDINGS || this->oldMenu == Menus::MENU_INGAME_BINDINGS || this->oldMenu == Menus::MENU_INGAME_OPTIONS_VIDEO) { // [GEC]
		this->m_scrollBar->field_0x44_ = this->old_0x44;
		this->m_scrollBar->field_0x48_ = this->old_0x48;
	}
}



void MenuSystem::setItemsFromText(int i, Text* text, int i2, int i3, int i4) {


	this->numItems = i;
	text->wrapText(i2);
	int n5;
	int first;
	for (n5 = 0; (first = text->findFirstOf('|', n5)) >= 0; n5 = first + 1) {
		app->localization->addTextArg(text, n5, first);
		this->items[i++].Set(getLastArgString(), MenuSystem::EMPTY_TEXT, 0x2 | i3, 0, i4, MenuSystem::EMPTY_TEXT);
	}
	app->localization->addTextArg(n5, text->length());
	this->items[i++].Set(getLastArgString(), MenuSystem::EMPTY_TEXT, 0x2 | i3, 0, i4, MenuSystem::EMPTY_TEXT);
	this->numItems = i;
	text->dispose();
}


void MenuSystem::returnToGame() {


	this->numItems = 0;
	app->time = app->lastTime = app->upTimeMs;
	if (app->game->isCameraActive()) {
		app->canvas->setState(Canvas::ST_INTER_CAMERA);
	}
	else {
		app->canvas->setState(Canvas::ST_PLAYING);
	}
}

void MenuSystem::gotoMenu(int menu) {
	if (menu != this->menu) {
		this->pushMenu(this->menu, this->selectedIndex, this->m_scrollBar->field_0x44_, this->m_scrollBar->field_0x48_, this->scrollIndex); // [GEC]
	}

	this->setMenu(menu);
}



void MenuSystem::infiniteLoop() {
	for (int i = 1024; ; i += 4) {
		i = 0;
	}
}

int MenuSystem::infiniteRecursion(int* array) {
	int n = 0;
	++array[n];
	if (array[0] > 0) {
		array[0] = 0;
		return infiniteRecursion(array);
	}
	return array[0];
}

void MenuSystem::systemTest(int sysType) {

	int i = 0;
	int j = 0;

	switch (sysType)
	{
		case 332: { // SYS_LOOP
			//this->infiniteLoop();
			break;
		}
		case 333: { // SYS_RECURSION
			/*if (i + 1 > 0) {
				i = 0;
				this->infiniteRecursion(&i);
			}*/
			break;
		}
		case 334: { // SYS_ERR
			/*for (j = 0; j != 1024; ++j) {
				//app->setTimerTest();
			}
			do
			{
				//app->clearSetTimerTest();
				--j;
			} while (j);*/
			break;
		}
		case 338: { // SYS_SOUND
			app->canvas->sysSoundDelayTime = app->canvas->sysSoundDelayTime == 0;
			break;
		}
		case 339: { // SYS_SOUND_DELAY
			app->canvas->sysSoundDelayTime = 100 * ((app->canvas->sysSoundDelayTime + 100) / 100) % 2000;
			break;
		}
		case 341: { // SYS_ADV_TIME
			app->sysAdvTime = (app->sysAdvTime + 5) % 35;
			break;
		}
	}
}

void MenuSystem::startGame(bool b) {


	if (this->background != this->imgMainBG) {
		this->background->~Image();
	}

	this->imgMainBG->~Image();

	this->background = nullptr;
	this->imgMainBG = nullptr;

	app->sound->soundStop();

	if (b) {
		app->canvas->setLoadingBarText(-1, -1);
		app->game->removeState(true);

		app->game->activeLoadType = 0;
		app->canvas->loadType = 0;
		app->canvas->loadMapID = 0;
		app->canvas->lastMapID = 0;
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
		app->canvas->clearEvents(1);
		app->canvas->setState(Canvas::ST_CHARACTER_SELECTION);
	}
	else {
		app->canvas->loadState(app->canvas->getRecentLoadType(), 3, 194);
	}
}

void MenuSystem::SetYESNO(short i, int i2, int i3, int i4) {

	Text* text;

	text = app->localization->getLargeBuffer();
	app->localization->composeText(3, i, text);
	this->SetYESNO(text, i2, i3, i4, 2, 0);
	text->dispose();
}

void MenuSystem::SetYESNO(short i, int i2, int i3, int i4, int i5, int i6) {

	Text* text;

	text = app->localization->getLargeBuffer();
	app->localization->composeText(3, i, text);
	this->SetYESNO(text, i2, i3, i4, i5, i6);
	text->dispose();
}

void MenuSystem::SetYESNO(Text* text, int i, int i2, int i3, int i4, int i5) {


	app->localization->resetTextArgs();
	if (text->findFirstOf('\n', 0) >= 0) {
		int n6;
		int first;
		for (n6 = 0; (first = text->findFirstOf('\n', n6)) >= 0; n6 = first + 1) {
			app->localization->addTextArg(text, n6, first);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
		}
		app->localization->addTextArg(text, n6, text->length());
		this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
	}
	else {
		app->localization->addTextArg(text);
		this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
	}

	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::YES_LABEL), MenuSystem::EMPTY_TEXT, 8, i2, i3, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::NO_LABEL), MenuSystem::EMPTY_TEXT, 8, i4, i5, MenuSystem::EMPTY_TEXT);

	if (i == 1) {
		this->selectedIndex = this->numItems - 2;
	}
	else {
		this->selectedIndex = this->numItems - 1;
	}
}

void MenuSystem::LoadHelpResource(short i) {

	Text* text;

	this->scrollIndex = 0;
	this->selectedIndex = 0;
	text = app->localization->getLargeBuffer();

	app->localization->loadText(2);
	app->localization->composeText(2, i, text);
	app->localization->unloadText(2);

	text->wrapText((this->menu >= Menus::MENU_INGAME) ? app->canvas->ingameScrollWithBarMaxChars : 35);

	this->LoadHelpItems(text, 0);

	int h = 0;
	for(int j = 0; j < this->numItems; j++) {
		if (!(this->items[0].flags & 0x8000)) {
			h += this->getMenuItemHeight(j);
		}
	}

	if ((app->canvas->menuRect[3] < h) && (this->menu >= Menus::MENU_INGAME)) {
		this->numItems = 0;
		app->localization->resetTextArgs();
		text->dispose();

		text = app->localization->getLargeBuffer();
		app->localization->loadText(2);
		app->localization->composeText(2, i, text);
		app->localization->unloadText(2);

		app->canvas->menuRect[2] -= 27;
		text->wrapText(app->canvas->menuRect[2] / Applet::CHAR_SPACING[app->fontType]);
		this->LoadHelpItems(text, 0);
	}

	app->localization->unloadText(2);
	text->dispose();
}

void MenuSystem::FillRanking() {

	int max = std::max(app->player->finalCurrentGrade(), 1);
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
	if (app->game->difficulty == Enums::DIFFICULTY_EASY) {
		this->addItem(Localization::STRINGID((short)3, (short)200), MenuSystem::EMPTY_TEXT, 72, 0, 0, MenuSystem::EMPTY_TEXT);
	}
	else if (app->game->difficulty == Enums::DIFFICULTY_NORMAL) {
		this->addItem(Localization::STRINGID((short)3, (short)201), MenuSystem::EMPTY_TEXT, 72, 0, 0, MenuSystem::EMPTY_TEXT);
	}
	else {
		this->addItem(Localization::STRINGID((short)3, (short)202), MenuSystem::EMPTY_TEXT, 72, 0, 0, MenuSystem::EMPTY_TEXT);
	}
	short n = 0;
	short n2 = 0;
	switch (max) {
		case 6: {
			n = 28;
			n2 = 29;
			break;
		}
		case 5: {
			n = 26;
			n2 = 27;
			break;
		}
		case 4: {
			n = 24;
			n2 = 25;
			break;
		}
		case 3: {
			if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE) {
				n = 24;
				n2 = 25;
				break;
			}
			n = 22;
			n2 = 23;
			break;
		}
		default: {
			if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE) {
				n = 24;
				n2 = 25;
				break;
			}
			if (app->game->difficulty == Enums::DIFFICULTY_NORMAL) {
				n = 22;
				n2 = 23;
				break;
			}
			n = 20;
			n2 = 21;
			break;
		}
	}
	app->localization->resetTextArgs();
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID((short)3, (short)203), MenuSystem::EMPTY_TEXT, 8, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID((short)3, app->player->gradeToString(max)), MenuSystem::EMPTY_TEXT, 8, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID((short)3, (short)204), MenuSystem::EMPTY_TEXT, 8, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID((short)0, n), MenuSystem::EMPTY_TEXT, 8, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
	Text* largeBuffer = app->localization->getLargeBuffer();
	app->localization->composeText((short)0, n2, largeBuffer);
	largeBuffer->wrapText(app->canvas->menuRect[2] / Applet::CHAR_SPACING[app->fontType]);
	this->LoadHelpItems(largeBuffer, 8);
	largeBuffer->dispose();
}

void MenuSystem::LoadNotebook() {

	this->detailsHelpText = app->localization->getLargeBuffer();
	this->type = 7;
	this->items[this->numItems++].Set(app->render->mapNameField, MenuSystem::EMPTY_TEXT, 9);
	this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 73);
	for (int i = 0; i < app->player->numNotebookIndexes; ++i) {
		this->detailsHelpText->setLength(0);
		int n = 0;
		app->localization->composeText(app->canvas->loadMapStringID, app->player->notebookIndexes[i], this->detailsHelpText);
		if (app->player->isQuestFailed(i)) {
			this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)227), MenuSystem::EMPTY_TEXT, 0);
			n = 4;
		}
		else if (app->player->isQuestDone(i)) {
			this->detailsHelpText->insert('\x87', 0);
			this->detailsHelpText->insert(" ", 1);
		}
		this->detailsHelpText->wrapText(app->canvas->ingameScrollWithBarMaxChars);
		this->LoadHelpItems(this->detailsHelpText, n);
		this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0);
	}
	int n2 = (app->canvas->displayRect[3] - 26) / 16;
	int index = this->numItems - n2;
	this->scrollIndex = index;
	this->selectedIndex = index;
	if (index < 0) {
		this->selectedIndex = 0;
		this->scrollIndex = 0;
	}

	this->detailsHelpText->dispose();
}

void MenuSystem::LoadHelpItems(Text* text, int i) {


	int n2 = 2;
	int n3;
	int first;
	for (n3 = 0; (first = text->findFirstOf('|', n3)) >= 0; n3 = first + 1) {
		if (first > n3 && text->charAt(n3) == '#') {
			app->localization->addTextArg(text, n3 + 1, first);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, n2 | 0x8 | 0x40 | i, 0, 0, MenuSystem::EMPTY_TEXT);
		}
		else {
			app->localization->addTextArg(text, n3, first);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, n2 | i, 0, 0, MenuSystem::EMPTY_TEXT);
		}
	}
	int length = text->length();
	if (n3 < length) {
		if (text->charAt(n3) == '#') {
			app->localization->addTextArg(text, n3, length);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, n2 | 0x8 | 0x40 | 0x40 | i, 0, 0, MenuSystem::EMPTY_TEXT);
		}
		else {
			app->localization->addTextArg(text, n3, text->length());
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, n2 | i, 0, 0, MenuSystem::EMPTY_TEXT);
		}
	}
}

void MenuSystem::buildFraction(int i, int i2, Text* text) {
	text->setLength(0);
	if (i < 0) {
		text->append('-');
	}
	text->append(i);
	text->append("/");
	if (i2 < 0) {
		text->append('-');
	}
	text->append(i2);
}

void MenuSystem::buildModStat(int i, int i2, Text* text) {
	text->setLength(0);
	text->append(i);
	if (i2 == 0) {
		return;
	}
	text->append((i2 > 0) ? "(+" : "(-");
	text->append(i2);
	text->append(')');
}

void MenuSystem::buildLevelGrades(Text* text) {

	int n = 30;
	app->localization->resetTextArgs();
	text->setLength(0);
	Text* smallBuffer = app->localization->getSmallBuffer();
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, (short)47, smallBuffer);
	smallBuffer->dehyphenate();
	for (int i = text->length() + smallBuffer->length(); i < n; ++i) {
		text->append('\x80');
	}
	text->append(smallBuffer);
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, (short)48, smallBuffer);
	smallBuffer->dehyphenate();
	for (int j = text->length() + smallBuffer->length(); j < (n + 8); ++j) {
		text->append('\x80');
	}
	text->append(smallBuffer);
	smallBuffer->dispose();
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
	app->localization->addTextArg(text);
	this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
	for (int k = 0; k < app->game->levelNamesCount - 1; ++k) {
		int n2 = k + 1;
		if (app->player->getCurrentGrade(n2) != 0 || app->player->getBestGrade(n2) != 0 || n2 <= app->canvas->loadMapID) {
			this->buildLevelGrade(n2, text, n, n + 8);
			app->localization->addTextArg(text);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
		}
	}
}

void MenuSystem::buildLevelGrade(int i, Text* text, int i2, int i3) {

	int n4 = (i == app->canvas->loadMapID) ? app->player->levelGrade(false) : app->player->getCurrentGrade(i);
	int bestGrade = app->player->getBestGrade(i);
	text->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->game->levelNames[i - 1], text);
	text->dehyphenate();
	Text* smallBuffer = app->localization->getSmallBuffer();
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(n4), smallBuffer);
	smallBuffer->dehyphenate();
	int length = smallBuffer->length();
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(n4), smallBuffer);
	for (int i = text->length() + length; i < i2; ++i) {
		text->append(" ");
	}
	text->append(smallBuffer);
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(bestGrade), smallBuffer);
	smallBuffer->dehyphenate();
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(bestGrade), smallBuffer);
	for (int j = i2 + 1; j < i3; ++j) {
		text->append(" ");
	}
	text->append(smallBuffer);
	smallBuffer->dispose();
}

void MenuSystem::fillStatus(bool b, bool b2, bool b3) {

	app->localization->resetTextArgs();
	Text* largeBuffer = app->localization->getLargeBuffer();
	if (b2) {
		this->buildFraction(app->player->ce->getStat(0), app->player->ce->getStat(1), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildFraction(app->player->ce->getStat(2), 200, largeBuffer);
		app->localization->addTextArg(largeBuffer);
		app->localization->addTextArg(app->player->level);
		app->localization->addTextArg(app->player->currentXP);
		app->localization->addTextArg(app->player->nextLevelXP);
		this->buildModStat(app->player->baseCe->getStat(3), app->player->ce->getStat(3) - app->player->baseCe->getStat(3), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildModStat(app->player->baseCe->getStat(4), app->player->ce->getStat(4) - app->player->baseCe->getStat(4), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildModStat(app->player->baseCe->getStat(5), app->player->ce->getStat(5) - app->player->baseCe->getStat(5), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildModStat(app->player->baseCe->getStat(6), app->player->ce->getStat(6) - app->player->baseCe->getStat(6), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildModStat(app->player->baseCe->getStat(7), app->player->ce->getStat(7) - app->player->baseCe->getStat(7), largeBuffer);
		app->localization->addTextArg(largeBuffer);
	}
	if (b) {
		if (b && !b2 && !b3) {
			app->localization->addTextArg(Strings::FILE_MENUSTRINGS, (short)(app->render->mapNameField & 0x3FF));
		}
		app->player->formatTime(app->game->totalLevelTime + (app->gameTime - app->game->curLevelTime), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		app->player->fillMonsterStats();
		this->buildFraction(app->game->mapSecretsFound, app->game->totalSecrets, largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildFraction(app->player->monsterStats[0], app->player->monsterStats[1], largeBuffer);
		app->localization->addTextArg(largeBuffer);
		app->localization->addTextArg(app->player->moves);
		app->localization->addTextArg(app->player->currentLevelDeaths);
		Text* smallBuffer = app->localization->getSmallBuffer();
		app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(app->player->levelGrade(false)), smallBuffer);
		app->localization->addTextArg(smallBuffer);
		smallBuffer->dispose();
	}
	if (b3) {
		largeBuffer->setLength(0);
		app->player->formatTime(app->player->totalTime + (app->gameTime - app->player->playTime), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		app->localization->addTextArg(app->player->totalMoves);
		app->localization->addTextArg(app->player->totalDeaths);
		app->localization->addTextArg(app->player->counters[6]);
		app->localization->addTextArg(app->player->counters[7]);
		app->localization->addTextArg(app->player->counters[1]);
		app->localization->addTextArg(app->player->counters[2]);
	}
	largeBuffer->dispose();
}

void MenuSystem::saveIndexes(int i) {
	this->indexes[i * 2] = this->selectedIndex;
	this->indexes[i * 2 + 1] = this->scrollIndex;
}

void MenuSystem::loadIndexes(int i) {
	this->selectedIndex = this->indexes[i * 2];
	this->scrollIndex = this->indexes[i * 2 + 1];
}

void MenuSystem::showDetailsMenu() {
	// Pendiente
}

int MenuSystem::onOffValue(bool b) {
	return Localization::STRINGID(Strings::FILE_MENUSTRINGS, (b ? MenuStrings::ON_LABEL : MenuStrings::OFF_LABEL));
}

void MenuSystem::leaveOptionsMenu(void) {


	//app->canvas->pacifierX = app->canvas->SCR_CX - 66;
	//app->canvas->repaintFlags |= Canvas::REPAINT_CLEAR;
	//app->canvas->setLoadingBarText(3, 0xe4);
	//app->canvas->updateLoadingBar(true);
	app->game->saveConfig();
}

int MenuSystem::getStackCount() {
	return this->stackCount;
}

void MenuSystem::clearStack() {
	this->stackCount = 0;
}

void MenuSystem::pushMenu(int i, int i2, int Y1, int Y2, int index2) {

	if (this->stackCount + 1 >= 10) {
		app->Error("Menu stack is full.");
	}
	this->menuIdxStack[this->stackCount] = i2;
	this->scrollY1Stack[this->stackCount] = Y1; // [GEC]
	this->scrollY2Stack[this->stackCount] = Y2; // [GEC]
	this->scrollI2Stack[this->stackCount] = index2; // [GEC]
	this->menuStack[this->stackCount++] = i;
}

int MenuSystem::popMenu(int* array, int* Y1, int* Y2, int *index2) {

	if (this->stackCount - 1 < 0) {
		app->Error("Menu stack is empty");
	}
	array[0] = this->menuIdxStack[this->stackCount - 1];
	*Y1 = this->scrollY1Stack[this->stackCount - 1]; // [GEC]
	*Y2 = this->scrollY2Stack[this->stackCount - 1]; // [GEC]
	*index2 = this->scrollI2Stack[this->stackCount - 1]; // [GEC]
	return this->menuStack[--this->stackCount];
}

int MenuSystem::peekMenu() {
	if (this->stackCount - 1 < 0) {
		return -1;
	}
	return this->menuStack[this->stackCount - 1];
}

int MenuSystem::getLastArgString() {


	if (app->localization->numTextArgs > 0) {
		return Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)(MenuStrings::ARGUMENT1 + (app->localization->numTextArgs - 1)));
	}

	return MenuSystem::EMPTY_TEXT;
}

void MenuSystem::fillVendingMachineSnacks(int i, Text* text) {


	int n2 = 1;
	if (i >= 1 && i <= 3) {
		n2 = 0;
	}
	else if (i >= 7 && i <= 9) {
		n2 = 2;
	}
	if (n2 == 0) {
		text->setLength(0);
		text->append(app->vendingMachine->getSnackPrice());
		EntityDef* find = app->entityDefManager->find(6, 0, 16);
		app->localization->addTextArg(text);
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)358), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)358), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)359), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)359), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)360), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)360), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)361), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)361), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)362), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)362), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)363), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)363), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)364), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)364), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)365), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)365), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)366), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)366), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)367), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)367), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)368), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)368), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
	}
	else if (n2 == 1) {
		text->setLength(0);
		text->append(app->vendingMachine->getSnackPrice());
		EntityDef* find = app->entityDefManager->find(6, 0, 16);
		app->localization->addTextArg(text);
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)370), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)370), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)371), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)371), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)372), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)372), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)373), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)373), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)374), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)374), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)375), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)375), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)376), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)376), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)377), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)377), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)378), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)378), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)379), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)379), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)368), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)368), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
	}
	else if (n2 == 2) {
		text->setLength(0);
		text->append(app->vendingMachine->getSnackPrice());
		EntityDef* find = app->entityDefManager->find(6, 0, 16);
		app->localization->addTextArg(text);
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)380), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)380), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)381), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)381), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)382), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)382), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)383), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)383), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)384), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)384), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)385), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)385), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)386), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)386), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)387), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)387), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)388), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)388), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)389), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)389), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)390), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)390), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
	}
}

//--------------------------------------------------------------------------


void MenuSystem::setMenuSettings() {

	fmButton* button;
	int x, y, w, h;

	this->menuItem_height = 46;
	this->menuItem_width = 162;
	this->isMainMenuScrollBar = false;
	this->isMainMenu = false;
	this->menuItem_fontPaddingBottom = 0;
	this->menuItem_paddingBottom = 0;

	this->imgMenuBtnBackground = this->imgMenuButtonBackground; // [GEC] Default
	this->imgMenuBtnBackgroundOn = this->imgMenuButtonBackgroundOn;  // [GEC] Default

	// Resolve theme for current menu (overrides defaults if available)
	const MenuTheme* theme = getThemeForMenu(this->menu);
	if (theme && theme->btnImage) {
		this->imgMenuBtnBackground = theme->btnImage;
		this->imgMenuBtnBackgroundOn = theme->btnHighlightImage ? theme->btnHighlightImage : theme->btnImage;
	}

	app->canvas->setMenuDimentions(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);

	for (int i = 0; i < 9; i++) {
		button = this->m_menuButtons->GetButton(i);
		button->normalRenderMode = 0;
		button->highlightRenderMode = 0;
	}

	if (this->menu >= Menus::MENU_INGAME) {
		app->canvas->setMenuDimentions(70, 0, 340, Applet::IOS_HEIGHT - this->imgGameMenuPanelbottom->height);
	}

	// Apply theme item dimensions (or fallback to original logic)
	if (theme) {
		if (theme->itemWidth > 0) {
			this->menuItem_width = theme->itemWidth;
		} else if (theme->btnImage) {
			this->menuItem_width = theme->btnImage->width;
		}
		if (theme->itemHeight > 0) this->menuItem_height = theme->itemHeight;
		this->menuItem_paddingBottom = theme->itemPaddingBottom;
	} else if (this->menu >= Menus::MENU_INGAME) {
		this->menuItem_width = this->imgInGameMenuOptionButton->width;
		this->menuItem_paddingBottom = 10;
	}

	// Apply info button theme (or fallback to original logic)
	if (theme && theme->infoBtnImage) {
		for (int i = 0; i != 9; i++) {
			button = this->m_infoButtons->GetButton(i);
			button->SetImage(theme->infoBtnImage, false);
			button->SetHighlightImage(theme->infoBtnHighlightImage, false);
			button->normalRenderMode = theme->infoBtnRenderMode;
			button->highlightRenderMode = theme->infoBtnHighlightRenderMode;
		}
	} else {
		if (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) {
			for (int i = 0; i != 9; i++) {
				button = this->m_infoButtons->GetButton(i);
				button->SetImage(this->imgGameMenuInfoButtonNormal, false);
				button->SetHighlightImage(this->imgGameMenuInfoButtonPressed, false);
				button->normalRenderMode = 0;
				button->highlightRenderMode = 0;
			}
		} else {
			this->menuItem_width = this->imgVendingButtonLarge->width;
			this->menuItem_paddingBottom = 3;
			for (int i = 0; i != 9; i++) {
				button = this->m_infoButtons->GetButton(i);
				button->SetImage(this->imgVendingButtonHelp, false);
				button->SetHighlightImage(nullptr, false);
				button->normalRenderMode = 2;
				button->highlightRenderMode = 0;
			}
		}
	}

	// Try data-driven layout from YAML first
	const YAMLMenuDef* def = getMenuDef(this->menu);
	if (def && def->layout.isSet) {
		if (def->itemWidth > 0) {
			this->menuItem_width = def->itemWidth;
		}
		w = resolveLayoutValue(def->layout.wMode, def->layout.wVal);
		h = resolveLayoutValue(def->layout.hMode, def->layout.hVal);
		x = resolveLayoutValue(def->layout.xMode, def->layout.xVal, w);
		y = resolveLayoutValue(def->layout.yMode, def->layout.yVal);
		if (def->vibrationY >= 0 && this->HasVibration()) {
			y = def->vibrationY;
		}
		app->canvas->setMenuDimentions(x, y, w, h);
		return;
	}

	// No fallback layout needed — all menus should have YAML layout
}

void MenuSystem::updateTouchButtonState() {
	fmButton* button01;
	fmButton* button02;
	//printf("MenuSystem::updateTouchButtonState\n");

	for (int i = 0; i < 9; i++) {
		this->m_menuButtons->GetButton(i)->SetTouchArea(Applet::IOS_WIDTH, 0, 0, 0);
		this->m_infoButtons->GetButton(i)->SetTouchArea(Applet::IOS_WIDTH, 0, 0, 0);
	}

	for (int i = 0; i < 9; i++) {
		button01 = this->m_menuButtons->GetButton(i);
		button02 = this->m_infoButtons->GetButton(i);

		button01->drawButton = false;
		button02->drawButton = false;

		button02->SetHighlighted(false);
		button02->normalRenderMode = 2;
		button02->highlightRenderMode = 0;

		// Apply theme-driven button images, with original fallback
		const MenuTheme* theme = getThemeForMenu(this->menu);
		if (theme && theme->btnImage) {
			button01->SetImage(theme->btnImage, false);
			button01->SetHighlightImage(theme->btnHighlightImage, false);
			button01->normalRenderMode = theme->btnRenderMode;
			button01->highlightRenderMode = theme->btnHighlightRenderMode;
		} else {
			// Fallback: original button image setup by menu ID range
			if (this->menu <= Menus::MENU_INHERIT_BACKMENU) {
				button01->SetImage(this->imgMenuBtnBackground, false);
				button01->SetHighlightImage(this->imgMenuBtnBackgroundOn, false);
				button01->normalRenderMode = 1;
				button01->highlightRenderMode = 0;
			}
			else {
				if (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) {
					button01->SetImage(this->imgInGameMenuOptionButton, false);
				}
				else {
					button01->SetImage(this->imgVendingButtonLarge, false);
				}
				button01->normalRenderMode = 1;
				button01->highlightRenderMode = 0;
				button01->SetHighlightImage(nullptr, false);
			}
		}

		button01->drawTouchArea = false;
	}

	int buttonID = 0;
	for (int i = 0; (i < this->numItems) && (buttonID <= 8); i++) {

		if (this->items[i].action != 0) {
			button01 = this->m_menuButtons->GetButton(buttonID);
			button01->drawButton = true;

			// Data-driven info button visibility, with original fallback
			const YAMLMenuDef* infoDef = getMenuDef(this->menu);
			if (infoDef && infoDef->showInfoButtons) {
				button02 = this->m_infoButtons->GetButton(buttonID);
				button02->drawButton = true;
			} else if (!infoDef) {
				// Fallback: original hardcoded info button check
				uint32_t uVar2 = this->menu;
				bool bVar5 = Menus::MENU_INGAME_LANGUAGE < uVar2;
				if (uVar2 != Menus::MENU_INGAME_HELP) {
					bVar5 = uVar2 != Menus::MENU_INGAME;
				}
				if (((((!bVar5 || (uVar2 == Menus::MENU_INGAME_HELP || uVar2 == Menus::MENU_INGAME_STATUS)) || (uVar2 == Menus::MENU_INGAME_HELP)) || (uVar2 == Menus::MENU_ITEMS)) ||
					((uVar2 == Menus::MENU_ITEMS_WEAPONS || (uVar2 == Menus::MENU_ITEMS_DRINKS)))) ||
					((uVar2 == Menus::MENU_VENDING_MACHINE_CONFIRM || ((uVar2 == Menus::MENU_VENDING_MACHINE_DRINKS || (uVar2 == Menus::MENU_VENDING_MACHINE_SNACKS)))))) {
					button02 = this->m_infoButtons->GetButton(buttonID);
					button02->drawButton = true;
				}
			}

			buttonID++;
		}
	}

	for (int i = 0; i <= 17 /*15*/; i++) {
		this->m_menuButtons->GetButton(i)->SetHighlighted(false);
	}

	// Reset button visibility for buttons 11-17
	for (int i = 11; i <= 17; i++) {
		this->m_menuButtons->GetButton(i)->drawButton = false;
	}

	// Data-driven button visibility from YAML, with original fallback
	const YAMLMenuDef* btnDef = getMenuDef(this->menu);
	if (btnDef && !btnDef->visibleButtons.empty()) {
		for (int btnId : btnDef->visibleButtons) {
			this->m_menuButtons->GetButton(btnId)->drawButton = true;
		}
		// Conditional buttons (e.g., music slider visible only when music is on)
		for (int btnId : btnDef->visibleButtonsConditional) {
			if (isUserMusicOn()) {
				this->m_menuButtons->GetButton(btnId)->drawButton = true;
			}
		}
	}
	// No fallback needed — all menus should have visible_buttons in YAML
}

int MenuSystem::getScrollPos() {
	int height;
	int posY;
	int pos;

	pos = this->m_scrollBar->field_0x0_;
	if (pos != 0) {
		pos = this->m_scrollBar->field_0x44_;
	}

	if (this->isMainMenu != false) {
		posY = 0;
		for (int i = 0; i < this->numItems; i++) {
			if (!(this->items[i].flags & 0x8000)) {
				height = this->getMenuItemHeight(i);
				if (pos < (posY + (height >> 1))) {
					return posY;
				}
				posY += height;
			}
		}
	}
	return pos;
}

int MenuSystem::getMenuItemHeight(int i) {


	int padding;
	int height = 29; // default
	int sheight = 0; //[GEC]

	//if ((this->menu != Menus::MENU_MAIN_OPTIONS)  || !HasVibration()) // Old
	{
		if (this->items[i].flags & Menus::ITEM_PADDING) { // [GEC]
			sheight += this->items[i].param;
		}

		if (this->items[i].flags & Menus::ITEM_SCROLLBAR) { // [GEC]
			sheight += Applet::FONT_HEIGHT[app->fontType];
		}
		else if (this->items[i].flags & Menus::ITEM_SCROLLBARTWO) { // [GEC]
			sheight += (Applet::FONT_HEIGHT[app->fontType] << 1);
		}

		if (this->items[i].action && !(this->items[i].flags & (Menus::ITEM_SCROLLBAR | Menus::ITEM_SCROLLBARTWO))) { // [GEC]
			height = this->menuItem_height;
			if (i == (this->numItems - 1)) {
				return height;
			}
			padding = this->menuItem_paddingBottom;
		}
		else {
			height = Applet::FONT_HEIGHT[app->fontType];
			if (i == (this->numItems - 1)) {
				return height;
			}
			padding = this->menuItem_fontPaddingBottom;
		}
		height += padding;
	}
	return height + sheight;
}

int MenuSystem::getMenuItemHeight2(int i) { //[GEC]


	int padding;
	int height = 29; // default
	int sheight = 0; //[GEC]

	//if ((this->menu != Menus::MENU_MAIN_OPTIONS) || !HasVibration()) // Old
	{
		if (this->items[i].flags & Menus::ITEM_PADDING) { // [GEC]
			sheight += this->items[i].param;
		}

		if (this->items[i].flags & Menus::ITEM_SCROLLBAR) { // [GEC]
			sheight += Applet::FONT_HEIGHT[app->fontType];
		}
		else if (this->items[i].flags & Menus::ITEM_SCROLLBARTWO) { // [GEC]
			sheight += (Applet::FONT_HEIGHT[app->fontType] << 1);
		}

		if (this->items[i].action && !(this->items[i].flags & (Menus::ITEM_SCROLLBAR | Menus::ITEM_SCROLLBARTWO))) { // [GEC]
			height = this->menuItem_height;
			padding = this->menuItem_paddingBottom;
		}
		else {

			height = Applet::FONT_HEIGHT[app->fontType];
			padding = this->menuItem_fontPaddingBottom;
		}
		height += padding;
	}
	return height + sheight;
}

bool MenuSystem::HasVibration() {
	return true;
}
bool MenuSystem::isUserMusicOn() {
	return true;
}

bool MenuSystem::updateVolumeSlider(int buttonId, int x)
{

	int* value;
	int v9;
	int v10;
	//printf("buttonId %d\n", buttonId);

	if ((buttonId >= 12 && buttonId <= 14) || (buttonId >= 16 && buttonId <= 17)) {
		if (buttonId == 12) {
			this->sfxVolumeScroll = x;
			value = &app->sound->soundFxVolume;
		}
		else if (buttonId == 13) {
			this->musicVolumeScroll = x;
			value = &app->sound->musicVolume;
		}
		else if (buttonId == 14) {
			this->alphaScroll = x;
			value = &app->canvas->m_controlAlpha;
		}
		else if (buttonId == 16) { // [GEC]
			this->vibrationIntensityScroll = x;
			value = &gVibrationIntensity;
		}
		else if (buttonId == 17) { // [GEC]
			this->deadzoneScroll = x;
			value = &gDeadZone;
		}
		if (this->menu == Menus::MENU_MAIN_OPTIONS || this->menu == Menus::MENU_MAIN_OPTIONS_SOUND || this->menu == Menus::MENU_MAIN_CONTROLS || this->menu == Menus::MENU_MAIN_CONTROLLER) {
			v9 = 245 - this->imgMenuOptionSliderON->width;
			v10 = 100 * (x - ((this->imgMenuOptionSliderON->width >> 1) + ((Applet::IOS_WIDTH - this->imgMenuOptionBOX3->width) >> 1) + 4));
		}
		else
		{
			v9 = this->menuItem_width - 24;
			v10 = 100 * (x - (app->canvas->menuRect[0] + 12));
		}
		*value = v10 / v9;
		if (*value < 0) {
			*value = 0;
		}
		if (*value > 100) {
			*value = 100;
		}
		app->sound->updateVolume();
		return true;
	}

	return false;
}

void MenuSystem::refresh() {
	this->setMenu(this->menu);
}

void MenuSystem::soundClick() {
	this->app->sound->playSound(Sounds::getResIDByName(SoundName::DIALOG_HELP), 0, 5, false);
}
