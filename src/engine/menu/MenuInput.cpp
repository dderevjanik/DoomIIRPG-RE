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
#include "DialogManager.h"
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
#include "WidgetScreen.h"
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


bool MenuSystem::enterDigit(int i) {


	this->cheatCombo *= 10;
	this->cheatCombo += i;
	if (this->cheatCombo == 3666) {
		this->cheatCombo = 0;
		this->selectedIndex = 0;
		this->scrollIndex = 0;
		this->gotoMenu(Menus::MENU_DEBUG);
		return true;
	}
	if (this->cheatCombo == 1666) {
		app->canvas->loadMap(app->canvas->loadMapID, true, false);
		return true;
	}
	if (this->cheatCombo == 4332) {
		app->player->giveAll();
	}
	else if (this->cheatCombo == 3366) {
		this->cheatCombo = 0;
		if (this->menu >= Menus::MENU_INGAME) {
			app->canvas->startSpeedTest(false);
		}
	}
	return false;
}


void MenuSystem::doDetailsSelect()
{
	if (this->type == 7 || this->type == 5) {
		return;
	}
	if (this->menu == Menus::MENU_VENDING_MACHINE_CANT_BUY) {
		return;
	}
	this->cheatCombo = 0;

	if (this->items[this->selectedIndex].flags & 0x20) {
		this->showDetailsMenu();
	}
	else if (this->items[this->selectedIndex].action != 0) {
		this->select(this->selectedIndex);
	}
}


void MenuSystem::handleMenuEvents(int key, int keyAction) {


	key &= ~AVK_MENU_NUMBER; // [GEC]
	if (((key - AVK_0) < 10) && (this->enterDigit(key - AVK_0) != 0)) {
		return;
	}

	if (this->menu == Menus::MENU_COMIC_BOOK) { // [GEC]
		if (keyAction == Enums::ACTION_MENU) {
			this->app->comicBook->DeleteImages();
			this->back();
		} else {
			this->app->comicBook->handleComicBookEvents(key, keyAction);
		}
		return;
	}

	if (!this->isChangingValues()) { // Old changeSfxVolume
		// [GEC] Evita cualquer movimiento si esta activo
		if (this->drawHelpText) {
			if ((keyAction == Enums::ACTION_MENU) ||
				(keyAction == Enums::ACTION_FIRE) ||
				(keyAction == Enums::ACTION_MENU_ITEM_INFO)) {
				this->drawHelpText = false;
				this->selectedHelpIndex = -1;
			}
			return;
		}
		else if ((keyAction == Enums::ACTION_MENU_ITEM_INFO)) {
			if (this->menu != Menus::MENU_MAIN_OPTIONS) {
				if (this->items[this->selectedIndex].helpField != MenuSystem::EMPTY_TEXT && (this->selectedIndex < this->numItems)) {
					this->soundClick();
					this->drawHelpText = true;
					this->selectedHelpIndex = this->selectedIndex;
				}
			}
		}

		if (keyAction == Enums::ACTION_UP) {
			if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) {
				if (app->player->inventory[24] >= app->vendingMachine->currentItemPrice * (app->vendingMachine->currentItemQuantity + 1)) {
					++app->vendingMachine->currentItemQuantity;
					this->setMenu(this->menu);
				}
			}
			else if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
				this->scrollUp();
			}
		}
		else if (keyAction == Enums::ACTION_DOWN) {
			if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) {
				if (app->vendingMachine->currentItemQuantity > 1) {
					--app->vendingMachine->currentItemQuantity;
					this->setMenu(this->menu);
				}
			}
			else if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
				this->scrollDown();
			}
		}
		else if (keyAction == Enums::ACTION_LEFT) {
			if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
				this->scrollPageUp();
			}
		}
		else if (keyAction == Enums::ACTION_RIGHT) {
			if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
				this->scrollPageDown();
			}
		}
		else if (keyAction == Enums::ACTION_FIRE) {
			this->select(this->selectedIndex);
		}
		else if (keyAction != Enums::ACTION_PASSTURN) {
			if (keyAction == Enums::ACTION_AUTOMAP) {
				/*if (Canvas.softKeyRightID == Text.STRINGID((short)0, (short)29)) {
					returnToGame();
				}
				else {
					doDetailsSelect();
				}*/

				if ((this->menu == Menus::MENU_LEVEL_STATS || this->menu == Menus::MENU_END_RANKING)) {
					this->select(this->selectedIndex);
				}
				else if (this->menu == Menus::MENU_SHOWDETAILS) {
					if (this->goBackToStation) {
						this->goBackToStation = 0;
						this->returnToGame();
					}
					else {
						this->doDetailsSelect();
					}
				}
				else if (this->menu == Menus::MENU_MAIN_MORE_GAMES) {
					this->doDetailsSelect();
				}
				else if (this->moreGamesPage == 3) {
					this->back();
				}	
			}
			else if (keyAction == Enums::ACTION_MENU) {
				if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
					/*if (this->menu == Menus::MENU_MAIN) {
						this->gotoMenu(13);
					}
					else */if (this->menu == Menus::MENU_VENDING_MACHINE) {
						app->vendingMachine->returnFromBuying();
					}
					else {
						this->back();
					}
				}
				else if (this->moreGamesPage == 0) {
					this->back();
				}
			}
			else if (keyAction == Enums::ACTION_BACK) {
				if (this->menu != Menus::MENU_ENABLE_SOUNDS) {
					if (this->menu == Menus::MENU_MAIN) {
						this->gotoMenu(Menus::MENU_MAIN_EXIT);
					}
					else if (this->menu == Menus::MENU_LEVEL_STATS || this->menu == Menus::MENU_END_ || this->menu == Menus::MENU_END_RANKING || this->menu == Menus::MENU_END_FINALQUIT) {
						this->select(this->selectedIndex);
					}
					else if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
						if (this->menu == Menus::MENU_VENDING_MACHINE) {
							app->vendingMachine->returnFromBuying();
						}
						else {
							this->back();
						}
					}
					else if (this->moreGamesPage == 0) {
						this->back();
					}
				}
			}
		}
	}
	else {
		if (keyAction == Enums::ACTION_RIGHT || keyAction == Enums::ACTION_LEFT) {
			this->setMenu(this->menu);
		}
		else if ((keyAction <= Enums::ACTION_FIRE || keyAction >= Enums::ACTION_BACK)) {
			this->select(this->selectedIndex);
		}
	}
}

void MenuSystem::select(int i) {


	//printf("select %d\n", i);

	this->cheatCombo = 0;
	this->menuMode = 0;
	if ((i < app->menuSystem->numItems) &&
		((this->items[i].flags & (Menus::ITEM_DISABLED | Menus::ITEM_DISABLEDTWO)) != 0)) {
		return;
	}

	if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::VENDING_SALE), 0, 3, false);
	}

	int action = this->items[i].action;
	//printf("action %d\n", action);

	if(this->menu == Menus::MENU_END_RANKING) {
		app->sound->soundStop();
		app->canvas->setState(Canvas::ST_CREDITS);
		return;
	}

	if (this->menu == Menus::MENU_LEVEL_STATS) {
		if (!app->canvas->endingGame) {
			app->canvas->loadMap(this->LEVEL_STATS_nextMap, true, false);
			return;
		}
		this->gotoMenu(Menus::MENU_END_RANKING);
	}
	else if(this->menu != Menus::MENU_MAIN_MORE_GAMES) {
		if (this->type == 5 || this->type == 7) {
			if (this->menu == Menus::MENU_SHOWDETAILS && this->goBackToStation) {
				this->returnToGame();
				return;
			}
			this->back();
			return;
		}

		app->sound->playSound(Sounds::getResIDByName(SoundName::PISTOL), 0, 5, false); // [GEC] Pistol Sound
	}
	else {
		//SysIPhoneOpenURL("http://www.idsoftware.com/iphone-games/index.htm");
		return;
	}

	switch (action) {
		case 0: { // ACTION_NONE
			break;
		}
		case 1: { // ACTION_GOTO
			if (this->items[i].param == 75 || this->items[i].param == 73) {
				this->saveIndexes(4);
			}
			this->gotoMenu(this->items[i].param);
			break;
		}
		case 2: { // ACTION_BACK
			this->back();
			break;
		}
		case 3: { // ACTION_LOAD
			app->canvas->loadState(app->canvas->getRecentLoadType(), (short)3, (short)194);
			break;
		}
		case 4: { // ACTION_SAVE
			int n2 = 3;
			int n3;
			if (this->items[i].param == 1) {
				n3 = (n2 | 0x100);
			}
			else {
				n3 = (n2 | 0x80);
			}
			app->canvas->saveState(n3, (short)3, (short)196);
			break;
		}
		case 5:{ // ACTION_BACKTOMAIN
			app->canvas->backToMain(false);
			break;
		}
		case 6: { // ACTION_TOGSOUND
			app->sound->allowSounds = true;
			if (this->menu == Menus::MENU_ENABLE_SOUNDS) {
				app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_GENERAL), 1u, 3, 0);
				app->canvas->setState(Canvas::ST_INTRO_MOVIE);
				break;
			}

			app->canvas->vibrateEnabled ^= true;
			if (app->canvas->vibrateEnabled) {
				app->canvas->startShake(0, 0, 500);
			}
			this->items[0].textField2 = this->onOffValue(app->canvas->vibrateEnabled);

			this->setMenu(this->menu);
			break;
		}
		case 7: { // ACTION_NEWGAME
			app->canvas->m_controlAlpha = 0x32;
			if (app->game->hasSavedState()) {
				this->gotoMenu(Menus::MENU_MAIN_CONFIRMNEW);
			}
			else {
				this->gotoMenu(Menus::MENU_MAIN_DIFFICULTY);
			}
			break;
		}
		case 8: { // ACTION_EXIT
			app->shutdown();
			break;
		}
		case 9: { // ACTION_CHANGESTATE
			if (this->items[i].param == Canvas::ST_CREDITS) {
				if (app->canvas->loadMapID != 0) {
					app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_TITLE), 1, 3, false);
				}
			}
			app->canvas->setState(this->items[i].param);
			break;
		}
		case 10: { // ACTION_DIFFICULTY
			app->game->difficulty = (uint8_t)this->items[i].param;
			app->canvas->clearSoftKeys();
			this->startGame(true);
			break;
		}
		case 11: { // ACTION_RETURNTOGAME
			this->returnToGame();
			break;
		}
		case 12: { // ACTION_RESTARTLEVEL
			app->canvas->loadState(2, (short)3, (short)194);
			if (this->menu != Menus::MENU_INGAME_DEAD) {
				app->hud->addMessage((short)3, (short)195);
				break;
			}
			break;
		}
		case 13: { // ACTION_SAVEQUIT
			app->canvas->saveState(67, (short)3, (short)196);
			break;
		}
		case 14: { // ACTION_OFFERSUCCESS
			break;
		}
		case Menus::ACTION_CHANGESFXVOLUME: { // ACTION_CHANGESFXVOLUME
			this->activeSlider = (this->activeSlider == SliderMode::SfxVolume) ? SliderMode::None : SliderMode::SfxVolume;
			this->setMenu(this->menu);
			break;
		}
		case 16: { // ACTION_SHOWDETAILS
			this->showDetailsMenu();
			break;
		}
		case 17: { // ACTION_CHANGEMAP
			app->game->spawnParam = 0;
			app->canvas->loadMap(1 + this->items[i].param, false, false);
			break;
		}
		case 18: // ACTION_USEITEMWEAPON
		case 20: // ACTION_USEITEMSYRING
		case 21: // ACTION_USEITEMOTHER
		case 24: { // ACTION_CONFIRMUSE
			this->selectItemAction(i);
			break;
		}
		case 19: { // ACTION_SELECT_LANGUAGE
			app->localization->setLanguage(this->items[i].param);
			this->back();
			break;
		}
		case 22: { // ACTION_CONTINUE
			this->startGame(false);
			break;
		}
		case 23: { // ACTION_MAIN_SPECIAL
			app->canvas->backToMain(false);
			break;
		}
		case 25: { // ACTION_SAVEEXIT
			app->canvas->saveState(7, (short)3, (short)196);
			break;
		}
		case 26: { // ACTION_BACKTWO
			this->popMenu(this->poppedIdx, &this->m_scrollBar->scrollOffset, &this->m_scrollBar->thumbPosition, &this->scrollIndex);
			this->back();
			break;
		}
		case 27: { // ACTION_MINIGAME
			if (this->items[i].param == 2) {
				app->sound->soundStop();
				app->hackingGame->playFromMainMenu();
			}
			else if (this->items[i].param == 0) {
				app->sound->soundStop();
				app->sentryBotGame->playFromMainMenu();
			}
			else if (this->items[i].param == 4) {
				app->sound->soundStop();
				app->vendingMachine->playFromMainMenu();
			}
			break;
		}
		case 28: // ACTION_CONFIRMBUY
		case 29: // ACTION_BUYDRINK
		case 30: { // ACTION_BUYSNACK
			this->selectVendingAction(i);
			break;
		}
		case 33: { // ACTION_RETURN_TO_PLAYER
			app->player->familiarReturnsToPlayer(false);
			this->returnToGame();
			break;
		}
		case 35: // ACTION_FLIP_CONTROLS
		case 36: // ACTION_CONTROL_LAYOUT
		case Menus::ACTION_CHANGEMUSICVOLUME:
		case Menus::ACTION_CHANGEALPHA:
		case Menus::ACTION_SET_BINDING:
		case Menus::ACTION_DEFAULT_BINDINGS:
		case Menus::ACTION_TOG_VIBRATION:
		case Menus::ACTION_CHANGE_VIBRATION_INTENSITY:
		case Menus::ACTION_CHANGE_DEADZONE: {
			this->selectControlSettings(i);
			break;
		}

		case Menus::ACTION_CHANGE_VID_MODE:
		case Menus::ACTION_TOG_VSYNC:
		case Menus::ACTION_CHANGE_RESOLUTION:
		case Menus::ACTION_APPLY_CHANGES:
		case Menus::ACTION_TOG_TINYGL: {
			this->selectVideoSettings(i);
			break;
		}

		case 102: case 103: case 104: case 105: case 106: case 107: case 108:
		case 109: case 110: case 111: case 112: case 114: case 115: case 116:
		case 117: case 118: case 119: case 120: case 121: case 122: case 123:
		case 124: case 125: case 126: case 127: case 128: {
			this->selectDebugAction(i);
			break;
		}
		case 129: { // ACTION_WIDGET_OPTIONS
			auto* ws = static_cast<WidgetScreen*>(app->canvas->stateHandlers[Canvas::ST_WIDGET_SCREEN]);
			if (ws) {
				ws->loadFromYAML(app, "screens/options.yaml");
				this->returnToGame();
				app->canvas->setState(Canvas::ST_WIDGET_SCREEN);
			}
			break;
		}
		default:
			app->Error("Unhandled Menu Action: %i", this->items[i].action);
			break;
	}
}

void MenuSystem::selectItemAction(int i) {
	Applet* app = this->app;
	int action = this->items[i].action;
	if (action == 18) { // ACTION_USEITEMWEAPON
		this->saveIndexes(1);
		if (i > 0) {
			app->player->selectWeapon((short)this->items[i].param);
			this->returnToGame();
		}
	}
	else if (action == 20 || action == 21) { // ACTION_USEITEMSYRING / ACTION_USEITEMOTHER
		bool useItem = app->player->useItem((short)this->menuParam);
		if (app->game->animatingEffects != 0) {
			this->returnToGame();
			return;
		}
		app->game->snapMonsters(true);
		app->game->snapLerpSprites(-1);
		if (app->canvas->state != Canvas::ST_MENU) {
			return;
		}
		if (useItem) {
			this->back();
			return;
		}
		if (this->menuParam == 22) {
			this->gotoMenu(Menus::MENU_ITEMS_HOLY_WATER_MAX);
		}
		else if (this->menuParam >= 16 && this->menuParam < 18) {
			this->gotoMenu(Menus::MENU_ITEMS_HEALTHMSG);
		}
		else if (this->menuParam >= 11 && this->menuParam < 13) {
			this->gotoMenu(Menus::MENU_ITEMS_ARMORMSG);
		}
		else {
			this->gotoMenu(Menus::MENU_ITEMS_SYRINGEMSG);
		}
	}
	else if (action == 24) { // ACTION_CONFIRMUSE
		if (this->menu == Menus::MENU_ITEMS_DRINKS) {
			this->saveIndexes(2);
		}
		else if (this->menu == Menus::MENU_ITEMS) {
			this->saveIndexes(4);
		}
		this->menuParam = this->items[i].param;
		this->gotoMenu(Menus::MENU_ITEMS_CONFIRM);
	}
}

void MenuSystem::selectVendingAction(int i) {
	Applet* app = this->app;
	int action = this->items[i].action;
	if (action == 28) { // ACTION_CONFIRMBUY
		this->menuParam = this->items[i].param;
		app->vendingMachine->currentItemQuantity = 1;
		if (this->menu == Menus::MENU_VENDING_MACHINE_LAST) {
			this->setMenu(Menus::MENU_VENDING_MACHINE_CONFIRM);
		}
		else {
			this->gotoMenu(Menus::MENU_VENDING_MACHINE_CONFIRM);
		}
	}
	else if (action == 29) { // ACTION_BUYDRINK
		if (app->vendingMachine->buyDrink(this->menuParam)) {
			this->back();
		}
		else {
			this->setMenu(Menus::MENU_VENDING_MACHINE_CANT_BUY);
		}
	}
	else if (action == 30) { // ACTION_BUYSNACK
		if (app->player->inventory[24] < app->vendingMachine->currentItemPrice * app->vendingMachine->currentItemQuantity) {
			this->setMenu(Menus::MENU_VENDING_MACHINE_CANT_BUY);
		}
		else {
			for (int j = 0; j < app->vendingMachine->currentItemQuantity; ++j) {
				app->player->inventory[24] -= (short)app->vendingMachine->getSnackPrice();
				app->player->give(0, 16, 1, false);
			}
			this->back();
		}
	}
}

void MenuSystem::selectControlSettings(int i) {
	Applet* app = this->app;
	int action = this->items[i].action;
	switch (action) {
		case 35: // ACTION_FLIP_CONTROLS
			app->canvas->flipControls();
			break;
		case 36: { // ACTION_CONTROL_LAYOUT
			++app->canvas->m_controlLayout;
			if (app->canvas->m_controlLayout > 2) {
				app->canvas->m_controlLayout = 0;
			}
			app->canvas->setControlLayout();
			break;
		}
		case Menus::ACTION_CHANGEMUSICVOLUME:
			this->activeSlider = (this->activeSlider == SliderMode::MusicVolume) ? SliderMode::None : SliderMode::MusicVolume;
			break;
		case Menus::ACTION_CHANGEALPHA:
			this->activeSlider = (this->activeSlider == SliderMode::ButtonsAlpha) ? SliderMode::None : SliderMode::ButtonsAlpha;
			break;
		case Menus::ACTION_SET_BINDING:
			this->activeSlider = (this->activeSlider == SliderMode::Binding) ? SliderMode::None : SliderMode::Binding;
			break;
		case Menus::ACTION_DEFAULT_BINDINGS:
			std::memcpy(keyMappingTemp, keyMappingDefault, sizeof(keyMapping));
			break;
		case Menus::ACTION_TOG_VIBRATION:
			app->canvas->vibrateEnabled ^= true;
			if (app->canvas->vibrateEnabled) {
				app->canvas->startShake(0, 0, 500);
			}
			this->items[(this->menu == Menus::MENU_MAIN_CONTROLLER) ? 2 : 1].textField2 = this->onOffValue(app->canvas->vibrateEnabled);
			break;
		case Menus::ACTION_CHANGE_VIBRATION_INTENSITY:
			this->activeSlider = (this->activeSlider == SliderMode::VibrationIntensity) ? SliderMode::None : SliderMode::VibrationIntensity;
			break;
		case Menus::ACTION_CHANGE_DEADZONE:
			this->activeSlider = (this->activeSlider == SliderMode::Deadzone) ? SliderMode::None : SliderMode::Deadzone;
			break;
	}
	this->setMenu(this->menu);
}

void MenuSystem::selectVideoSettings(int i) {
	Applet* app = this->app;
	int action = this->items[i].action;
	switch (action) {
		case Menus::ACTION_CHANGE_VID_MODE:
			if (++this->sdlGL->windowMode > 2) {
				this->sdlGL->windowMode = 0;
			}
			break;
		case Menus::ACTION_TOG_VSYNC:
			this->sdlGL->vSync = !this->sdlGL->vSync;
			break;
		case Menus::ACTION_CHANGE_RESOLUTION:
			if (++this->sdlGL->resolutionIndex >= 18) {
				this->sdlGL->resolutionIndex = 0;
			}
			break;
		case Menus::ACTION_APPLY_CHANGES:
			this->sdlGL->updateVideo();
			app->game->saveConfig();
			break;
		case Menus::ACTION_TOG_TINYGL: {
			Canvas* canvas = app->canvas.get();
			TinyGL* tinyGL = app->tinyGL.get();
			_glesObj->isInit = !_glesObj->isInit;
			if (canvas->state == Canvas::ST_CAMERA) {
				tinyGL->setViewport(canvas->cinRect[0], canvas->cinRect[1], canvas->cinRect[2], canvas->cinRect[3]);
			}
			else {
				tinyGL->resetViewPort();
			}
			break;
		}
	}
	this->setMenu(this->menu);
}

void MenuSystem::selectDebugAction(int i) {
	Applet* app = this->app;
	int action = this->items[i].action;
	switch (action) {
		case 102: app->player->giveAll(); return;
		case 103: app->game->givemap(0, 0, 32, 32); return;
		case 104: app->player->noclip = !app->player->noclip; break;
		case 105: app->game->disableAI = !app->game->disableAI; break;
		case 106:
			app->player->enableHelp = !app->player->enableHelp;
			app->game->saveConfig();
			break;
		case 107: app->player->god = !app->player->god; break;
		case 108: app->canvas->showLocation = !app->canvas->showLocation; break;
		case 109: {
			int animFrames = app->canvas->animFrames + 1;
			if (animFrames > 15) animFrames = 2;
			app->canvas->setAnimFrames(animFrames);
			app->game->saveConfig();
			break;
		}
		case 110: app->canvas->showSpeeds = !app->canvas->showSpeeds; break;
		case 111: app->render->skipFlats = !app->render->skipFlats; break;
		case 112: app->render->skipCull = !app->render->skipCull; break;
		case 114: app->render->skipBSP = !app->render->skipBSP; break;
		case 115: app->render->skipLines = !app->render->skipLines; break;
		case 116: app->render->skipSprites = !app->render->skipSprites; break;
		case 117: app->canvas->renderOnly = !app->canvas->renderOnly; break;
		case 118: app->render->skipDecals = !app->render->skipDecals; break;
		case 119: app->render->skip2DStretch = !app->render->skip2DStretch; break;
		case 120: break; // ACTION_DRIVING_MODE (no-op)
		case 121: // ACTION_RENDER_MODE
			if (app->render->renderMode == 0) app->render->renderMode = 63;
			else app->render->renderMode >>= 1;
			break;
		case 122: app->player->equipForLevel(app->canvas->loadMapID); return;
		case 123: app->combat->oneShotCheat = !app->combat->oneShotCheat; break;
		case 124:
			app->dialogManager->enqueueHelpDialog(app->canvas.get(), (short)3, (short)342, (uint8_t)(-1));
			this->returnToGame();
			return;
		case 125: this->systemTest(this->items[i].param + 332); return;
		case 126: app->game->skipMinigames = !app->game->skipMinigames; break;
		case 127: app->canvas->showFreeHeap = !app->canvas->showFreeHeap; break;
		case 128:
			this->returnToGame();
			app->canvas->setState(Canvas::ST_WIDGET_SCREEN);
			return;
	}
	this->setMenu(Menus::MENU_DEBUG);
}

void MenuSystem::handleUserTouch(int x, int y, bool b) {

	bool isPressed;
	int touchX;
	int touchY;
	fmButton* Button;
	fmScrollButton* unusedScroll;
	fmScrollButton* btnScroll;
	int i;
	fmButton* unusedBtn1;
	int j;
	fmButton* unusedBtn2;
	int menuButtonID;
	int infoButtonID;
	fmButton* unusedBtn3;
	fmButtonContainer* btnContainer02;
	fmButton* unusedBtn4;
	int selectedIndex;
	int highlightX;
	int highlightY;
	fmButtonContainer* btnContainer03;
	bool highlightState;
	int vendingButtonID;
	VendingMachine* vendingRef;
	int quantity;
	VendingMachine* vendingMachine;
	int currentItemQuantity;

	isPressed = b;
	touchX = x;
	touchY = y;
	if (this->menu == Menus::MENU_COMIC_BOOK) {
		app->comicBook->Touch(x, y, b);
		return;
	}

	if (this->updateSlider)
	{
		if (!b)
		{
			this->m_menuButtons->GetButton(this->sliderID)->SetHighlighted(false);
			this->updateSlider = 0;
			return;
		}
	}
	else if (!b)
	{
		btnScroll = this->m_scrollBar;
		if (btnScroll->barTouched)
		{
			btnScroll->barTouched = 0;
			return;
		}
		if (btnScroll->contentDragging)
		{
			btnScroll->contentDragging = 0;
			return;
		}
		goto clear_button_highlights;
	}
	if (this->m_scrollBar->enabled && this->m_scrollBar->barRect.ContainsPoint(x, y))
	{
		if (this->isMainMenuScrollBar){
			this->m_scrollBar->SetTouchOffset(touchX, touchY);
		}
		else {
			this->m_scrollBar->thumbDragOffset = 0;
			this->m_scrollBar->Update(touchX, touchY);
		}
		this->m_scrollBar->barTouched = 1;
		return;
	}
clear_button_highlights:
	for (i = 0; i < 16; i++) {
		this->m_menuButtons->GetButton(i)->SetHighlighted(false);
	}
	for (j = 0; j < 9; j++) {
		this->m_infoButtons->GetButton(j)->SetHighlighted(false);
	}
	if (!this->drawHelpText)
	{
		if (app->canvas->touched) {
			if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) { // Port: new
				this->m_vendingButtons->HighlightButton(x, y, false);
				return;
			}
			return;
		}

		menuButtonID = this->m_menuButtons->GetTouchedButtonID(touchX, touchY);
		infoButtonID = this->m_infoButtons->GetTouchedButtonID(touchX, touchY);

		if (infoButtonID < 0)
		{
			if (menuButtonID < 0)
			{
				if (!isPressed)
				{
					if (this->menu != Menus::MENU_END_RANKING && this->menu != Menus::MENU_LEVEL_STATS)
					{
					handle_vending_confirm:
						if (this->menu != Menus::MENU_VENDING_MACHINE_CONFIRM)
							return;
						if (isPressed)
						{
							highlightX = touchX;
							highlightY = touchY;
							btnContainer03 = this->m_vendingButtons;
							highlightState = 1;
						highlight_vending_button:
							btnContainer03->HighlightButton(highlightX, highlightY, highlightState);
							return;
						}
						vendingButtonID = this->m_vendingButtons->GetTouchedButtonID(touchX, touchY);
						if (vendingButtonID)
						{
							if (vendingButtonID != 1)
								goto reset_vending_highlight;
							vendingMachine = app->vendingMachine.get();
							currentItemQuantity = vendingMachine->currentItemQuantity;
							if (currentItemQuantity <= 1)
								goto reset_vending_highlight;
							vendingMachine->currentItemQuantity = currentItemQuantity - 1;
						}
						else
						{
							vendingRef = app->vendingMachine.get();
							quantity = vendingRef->currentItemQuantity;
							if (app->player->inventory[24] < vendingRef->currentItemPrice + quantity * vendingRef->currentItemPrice)
								goto reset_vending_highlight;
							vendingRef->currentItemQuantity = quantity + 1;
						}
						this->setMenu(Menus::MENU_VENDING_MACHINE_CONFIRM);
					reset_vending_highlight:
						highlightX = 0;
						btnContainer03 = this->m_vendingButtons;
						highlightY = 0;
						highlightState = 0;
						goto highlight_vending_button;
					}
					app->canvas->handleEvent(13);
				}
			}
			else if (this->m_menuButtons->GetButton(menuButtonID)->drawButton)
			{
				if (isPressed)
				{
					this->m_menuButtons->GetButton(menuButtonID)->SetHighlighted(true);
					if (this->updateVolumeSlider(menuButtonID, touchX))
					{
						this->sliderID = menuButtonID;
						this->updateSlider = 1;
					}
				}
				else if (menuButtonID == 11) {
					this->back();
				}
				else if (menuButtonID == 15) {
					this->returnToGame();
				}
				else {
					this->selectedIndex = this->m_menuButtons->GetButton(menuButtonID)->selectedIndex;
					this->select(this->selectedIndex);
				}
			}
		}
		else if (this->m_infoButtons->GetButton(infoButtonID)->drawButton)
		{
			if (isPressed) {
				this->m_infoButtons->GetButton(infoButtonID)->SetHighlighted(true);
			}
			else {
				this->drawHelpText = true;
				this->selectedHelpIndex = this->m_infoButtons->GetButton(infoButtonID)->selectedIndex;
			}
		}
		goto handle_vending_confirm;
	}
	if (!isPressed)
	{
		this->drawHelpText = false;
		this->selectedHelpIndex = -1;
	}
}

void MenuSystem::handleUserMoved(int x, int y) {
	fmScrollButton* pfVar1;
	int iVar2;
	fmButton* pfVar3;
	int iVar4;
	int _x;
	int _y;

	_x = x;
	_y = y;

	if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) { // [GEC]: new line
		this->m_vendingButtons->HighlightButton(x, y, true);
		return;
	}

	if (this->menu == Menus::MENU_COMIC_BOOK) {
		this->app->comicBook->TouchMove(_x, _y);
	}
	else {
		//printf("field_0x674_ %d\n", field_0x674_);
		if (this->updateSlider) {
			this->updateVolumeSlider(this->sliderID, _x);
			return;
		}

		// [GEC] Hasta que este fuera del limite del rectangulo, permitir� el desplasamiento de los items del menu
		const int begMouseX = (int)(gBegMouseX * Applet::IOS_WIDTH);
		const int begMouseY = (int)(gBegMouseY * Applet::IOS_HEIGHT);
		if (pointInRectangle(x, y, begMouseX - 3, begMouseY - 3, 6, 6)) {
			return;
		}

		pfVar1 = this->m_scrollBar;
		if (pfVar1->barTouched == 0) {
			if ((pfVar1->enabled == 0) ||
				(iVar2 = pfVar1->barRect.ContainsPoint(_x, _y), iVar2 == 0)) {
				if (!this->drawHelpText) {
					iVar2 = 0;
					do {
						iVar4 = iVar2;
						pfVar3 = this->m_menuButtons->GetButton(iVar4);
						pfVar3->SetHighlighted(false);
						iVar2 = iVar4 + 1;
					} while (iVar4 + 1 != 0x10);

					iVar4 = iVar4 + -0xf;
					do {
						pfVar3 = this->m_infoButtons->GetButton(iVar4);
						iVar4 = iVar4 + 1;
						pfVar3->SetHighlighted(false);
					} while (iVar4 != 9);

					pfVar1 = this->m_scrollBar;
					if (pfVar1->contentDragging == 0) {
						if ((pfVar1->enabled == 0) ||
							(iVar2 = pfVar1->boxRect.ContainsPoint(_x, _y), iVar2 == 0)) {
							iVar2 = this->m_menuButtons->GetTouchedButtonID(_x, _y);
							_y = this->m_infoButtons->GetTouchedButtonID(_x, _y);
							if (_y < 0) {
								if ((-1 < iVar2) &&
									(pfVar3 = this->m_menuButtons->GetButton(iVar2),
										pfVar3->drawButton)) {
									pfVar3 = this->m_menuButtons->GetButton(iVar2);
									pfVar3->SetHighlighted(true);
									if (this->updateVolumeSlider(iVar2, _x)) {
										this->sliderID = iVar2;
										this->updateSlider = true;
									}
								}
							}
							else {
								pfVar3 = this->m_infoButtons->GetButton(_y);
								if (pfVar3->drawButton) {
									pfVar3 = this->m_infoButtons->GetButton(_y);
									pfVar3->SetHighlighted(true);
								}
							}
						}
						else {
							this->m_scrollBar->SetContentTouchOffset(_x, _y);
							this->m_scrollBar->contentDragging = 1;
						}
					}
					else {
						pfVar1->UpdateContent(_x, _y);
					}
				}
			}
			else {
				if (!this->isMainMenuScrollBar) {
					this->m_scrollBar->thumbDragOffset = 0;
					this->m_scrollBar->Update(_x, _y);
				}
				else {
					this->m_scrollBar->SetTouchOffset(_x, _y);
				}
				this->m_scrollBar->barTouched = 1;
			}
		}
		else {
			pfVar1->Update(_x, _y);
		}
	}
}

