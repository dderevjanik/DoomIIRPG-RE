#include "MenuState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Player.h"
#include "MenuSystem.h"
#include "Sound.h"
#include "Hud.h"
#include "Enums.h"
#include "Menus.h"

extern int gVibrationIntensity;
extern int gDeadZone;

void MenuState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	app->menuSystem->startTime = app->time;
	if (canvas->oldState != Canvas::ST_MENU) {
		canvas->clearEvents(1);
	}
	app->beginImageLoading();
	app->endImageLoading();
}

void MenuState::onExit(Canvas* canvas) {
	Applet* app = canvas->app;
	app->player->unpause(app->time - app->menuSystem->startTime);
	app->menuSystem->clearStack();
}

void MenuState::update(Canvas* canvas) {
	Applet* app = canvas->app;

	// menuState() inlined
	short n = -1;
	int menu = app->menuSystem->menu;
	if ((app->menuSystem->items[app->menuSystem->selectedIndex].flags & 0x20)) {
		n = 49;
	}
	else {
		if (menu == Menus::MENU_END_RANKING || menu == Menus::MENU_LEVEL_STATS) {
			n = 43;
		}
		else if (app->menuSystem->type != 5) {
			if (menu != Menus::MENU_SHOWDETAILS) {
				if (menu == Menus::MENU_VENDING_MACHINE_DETAILS || menu == Menus::MENU_VENDING_MACHINE_CONFIRM) {
					n = 202;
				}
				else if (menu != Menus::MENU_VENDING_MACHINE_CANT_BUY) {
					if (app->menuSystem->items[app->menuSystem->selectedIndex].action != 0) {
						n = 121;
					}
				}
			}
		}
	}

	if (menu != Menus::MENU_MAIN_MORE_GAMES) {
		canvas->clearSoftKeys();
		if (app->menuSystem->getStackCount() != 0 || menu == Menus::MENU_MAIN_MINIGAME) {
			if (app->menuSystem->peekMenu() != 25) {
				canvas->setLeftSoftKey((short)3, (short)80);
			}
		}
		else if (menu == Menus::MENU_INGAME || menu == Menus::MENU_INGAME_KICKING || menu == Menus::MENU_INGAME_SNIPER) {
			canvas->setLeftSoftKey((short)0, (short)30);
		}
		else if (menu == Menus::MENU_VENDING_MACHINE) {
			canvas->setLeftSoftKey((short)3, (short)80);
		}
		if (n != -1) {
			if (!app->menuSystem->isChangingValues()) {
				canvas->setRightSoftKey((short)0, n);
			}
		}
		else if (app->menuSystem->menu == Menus::MENU_SHOWDETAILS) {
			canvas->setRightSoftKey((short)0, (short)40);
		}
	}

	canvas->repaintFlags |= Canvas::REPAINT_MENU;
}

void MenuState::render(Canvas* canvas, Graphics* graphics) {
	// Menu rendering handled by REPAINT_MENU flag (runs for all states)
}

bool MenuState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = canvas->app;

	// Back button: shutdown if on enable sounds screen
	if (key == 18) {
		if (app->menuSystem->menu == Menus::MENU_ENABLE_SOUNDS) {
			app->shutdown();
		}
		return true;
	}

	// Volume/settings slider adjustment
	if (app->menuSystem->isChangingValues()) {
		if (app->menuSystem->activeSlider == MenuSystem::SliderMode::SfxVolume) {
			if (action == Enums::ACTION_RIGHT) {
				app->sound->volumeUp(10);
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (action == Enums::ACTION_LEFT) {
				app->sound->volumeDown(10);
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
		else if (app->menuSystem->activeSlider == MenuSystem::SliderMode::MusicVolume) {
			if (action == Enums::ACTION_RIGHT) {
				app->sound->musicVolumeUp(10);
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (action == Enums::ACTION_LEFT) {
				app->sound->musicVolumeDown(10);
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
		else if (app->menuSystem->activeSlider == MenuSystem::SliderMode::ButtonsAlpha) {
			if (action == Enums::ACTION_RIGHT) {
				canvas->m_controlAlpha += 10;
				if (canvas->m_controlAlpha > 100) canvas->m_controlAlpha = 100;
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (action == Enums::ACTION_LEFT) {
				canvas->m_controlAlpha -= 10;
				if (canvas->m_controlAlpha < 0) canvas->m_controlAlpha = 0;
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
		else if (app->menuSystem->activeSlider == MenuSystem::SliderMode::VibrationIntensity) {
			if (action == Enums::ACTION_RIGHT) {
				gVibrationIntensity += 10;
				if (gVibrationIntensity > 100) gVibrationIntensity = 100;
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (action == Enums::ACTION_LEFT) {
				gVibrationIntensity -= 10;
				if (gVibrationIntensity < 0) gVibrationIntensity = 0;
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
		else if (app->menuSystem->activeSlider == MenuSystem::SliderMode::Deadzone) {
			if (action == Enums::ACTION_RIGHT) {
				gDeadZone += 5;
				if (gDeadZone > 100) gDeadZone = 100;
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (action == Enums::ACTION_LEFT) {
				gDeadZone -= 5;
				if (gDeadZone < 0) gDeadZone = 0;
				app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
	}

	// Normal menu event handling
	app->menuSystem->handleMenuEvents(key, action);
	return true;
}
