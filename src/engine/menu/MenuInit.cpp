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


void MenuSystem::initMenu(int menu) {

	Text* textbuff;
	int flags = 0;

	if (this->oldMenu != menu) {
		this->scrollIndex = 0;
		this->selectedIndex = 0;
	}
	this->maxItems = 4;  // [GEC] 4 por defecto
	this->drawLogo = false; // [GEC]

	this->numItems = 0;
	for (int i = 0; i < 50; i++) {
		this->items[i].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
	}

	textbuff = app->localization->getLargeBuffer();
	app->localization->resetTextArgs();
	//printf("initMenu %d\n", menu);
	this->setMenuSettings();

	// Apply YAML-driven init properties (type, selectedIndex, etc.)
	const YAMLMenuDef* menuDef = getMenuDef(menu);
	if (menuDef) {
		if (menuDef->type > 0) {
			this->type = menuDef->type;
		}
		if (menuDef->maxItems > 0) {
			this->maxItems = menuDef->maxItems;
		}
		if (menuDef->selectedIndex > 0) {
			this->selectedIndex = menuDef->selectedIndex;
		}
		if (menuDef->scrollIndex >= 0) {
			this->scrollIndex = menuDef->scrollIndex;
		}
		// Help menus: load resource and return early
		if (menuDef->helpResource >= 0) {
			this->LoadHelpResource((short)menuDef->helpResource);
			if (menu == Menus::MENU_MAIN_EFFECTHELP) {
				this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
			}
			return;
		}
		// SetYESNO dialogs: apply from YAML and return early
		if (menuDef->yesnoStringId >= 0) {
			if (menuDef->yesnoClearStack) {
				this->clearStack();
			}
			this->scrollIndex = 0;
			this->SetYESNO((short)menuDef->yesnoStringId, menuDef->yesnoSelectYes,
						   menuDef->yesnoYesAction, menuDef->yesnoYesParam,
						   menuDef->yesnoNoAction, menuDef->yesnoNoParam);
			return;
		}
	}

	switch (menu) {
		case Menus::MENU_LEVEL_STATS: {
			this->loadMenuItems(menu, 0, -1);
			this->items[3].textField = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1);
			this->items[5].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2);
			this->items[6].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3);
			this->items[7].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4);
			this->items[8].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5);
			this->items[9].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6);
			this->items[10].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7);
			this->fillStatus(true, false, false);
			break;
		}
		case Menus::MENU_MAIN: {
			int index = 2;
			if (app->game->hasSavedState()) {
				index++;
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::CONTINUE_ITEM), MenuSystem::EMPTY_TEXT, 8, 22, 0, MenuSystem::EMPTY_TEXT);
			}
			// [GEC] Don't load "other games" item
			{
				this->loadMenuItems(menu, 0, 2);
				this->loadMenuItems(menu, 3, -1);
			}

			break;
		}
		// MENU_MAIN_DIFFICULTY, MENU_MAIN_MINIGAME, MENU_MAIN_HELP
		case Menus::MENU_INGAME_OPTIONS: // [GEC]
		case Menus::MENU_MAIN_OPTIONS: {
			if (!this->loadYAMLMenuItems(menu)) {
				// Fallback: build items in C++ (GEC menus without YAML items)
				if (menu == Menus::MENU_MAIN_OPTIONS) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_OPTIONS_SOUND, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_OPTIONS_VIDEO, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_OPTIONS_INPUT, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_OPTIONS_SOUND, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_OPTIONS_VIDEO, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_OPTIONS_INPUT, MenuSystem::EMPTY_TEXT);
				}
				// Text args for item labels
				textbuff->setLength(0);
				textbuff->append("Options");
				app->localization->addTextArg(textbuff);
				textbuff->setLength(0);
				textbuff->append("Sound");
				app->localization->addTextArg(textbuff);
				textbuff->setLength(0);
				textbuff->append("Video");
				app->localization->addTextArg(textbuff);
				textbuff->setLength(0);
				textbuff->append("Input");
				app->localization->addTextArg(textbuff);
			}
			break;
		}
		case Menus::MENU_SELECT_LANGUAGE: {
			flags = 8;
		}
		case Menus::MENU_INGAME_LANGUAGE: {
			this->oldLanguageSetting = app->localization->defaultLanguage;
			this->type = ((menu == Menus::MENU_INGAME_LANGUAGE) ? 1 : 4);
			this->scrollIndex = 0;
			if (this->peekMenu() != 25) {
				textbuff->setLength(0);
				app->localization->composeText((short)3, (short)80, textbuff);
				if (this->type == 4) {
					while (textbuff->length() < 8) {
						textbuff->append(' ');
					}
				}
				app->localization->addTextArg(textbuff);
				this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, flags, 2, 0, MenuSystem::EMPTY_TEXT);
			}

			//for (int l = 0; l < 5; ++l) {
			for (int l = 0; l < 1; ++l) {
				app->localization->loadGroupFromYAML(l, 14);
				textbuff->setLength(0);
				app->localization->composeText((short)14, (short)0, textbuff);
				if (this->type == 1) {
					textbuff->trim();
				}
				textbuff->dehyphenate();
				app->localization->addTextArg(textbuff);
				this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, flags, 19, l, MenuSystem::EMPTY_TEXT);
			}
			app->localization->loadGroupFromYAML(app->localization->defaultLanguage, 14);
			break;
		}
		case Menus::MENU_END_RANKING: {
			this->clearStack();
			this->scrollIndex = 0;
			this->selectedIndex = 0;
			this->FillRanking();
			break;
		}
		// MENU_ENABLE_SOUNDS now handled by YAML-driven yesno above
		case Menus::MENU_INGAME: {
			if (app->player->isFamiliar) {
				this->loadMenuItems(menu, 0, 1);
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)355), MenuSystem::EMPTY_TEXT, 0, 33, 0, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)356));
				this->loadMenuItems(menu, 1, -1);
				this->items[0].flags = 4;
				this->items[6].flags = 4;
			}
			else {
				this->loadMenuItems(menu, app->player->isFamiliar, -1);
				if (app->player->inventory[18] == 0) {
					this->items[2].flags = app->player->inventory[18] + 4;
				}
				if (app->game->isCameraActive()) {
					this->items[3].flags = 4;
					this->items[4].flags = 4;
					this->items[5].flags = 4;
					this->items[10].flags = 4;
					this->items[11].action = 1;
					this->items[11].param = 58;
				}
			}
			break;
		}

		case Menus::MENU_INGAME_PLAYER: {
			this->loadMenuItems(menu, 0, -1);
			this->items[2].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1);
			this->items[3].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2);
			this->items[4].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3);
			this->items[5].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4);
			this->items[6].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5);
			this->items[7].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6);
			this->items[8].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7);
			this->items[9].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT8);
			this->items[10].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT9);
			this->items[11].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT10);
			this->items[12].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT11);
			this->items[13].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT12);
			this->items[14].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT13);
			this->items[15].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT14);
			this->items[16].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT15);
			this->items[17].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT16);
			this->items[18].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT17);
			this->fillStatus(false, true, true);
			break;
		}

		case Menus::MENU_INGAME_LEVEL: {
			this->loadMenuItems(menu, 0, -1);
			this->items[1].textField = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1);
			this->items[2].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2);
			this->items[3].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3);
			this->items[4].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4);
			this->items[5].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5);
			this->items[6].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6);
			this->items[7].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7);
			this->fillStatus(true, false, false);
			break;
		}

		case Menus::MENU_INGAME_GRADES: {
			this->loadMenuItems(menu, 0, -1);
			this->buildLevelGrades(textbuff);
			break;
		}

#if 0 // Old
		case Menus::MENU_INGAME_OPTIONS: {
			this->loadMenuItems(menu, 0, 1); // Controls item
			// [GEC] New
			{
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGESFXVOLUME, 1, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGEMUSICVOLUME, 2, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
				textbuff->setLength(0);
				textbuff->append(" SoundFx Volume");
				app->localization->addTextArg(textbuff);
				textbuff->setLength(0);
				textbuff->append(" Music Volume");
				app->localization->addTextArg(textbuff);
				if (!isUserMusicOn()) {
					this->items[3].flags |= Menus::ITEM_DISABLEDTWO;
				}
			}
			//this->items[1].textField = Localization::STRINGID(MenuSystem::INDEX_OTHER, MenuStrings::SOUND_FX_VOLUME);


#if 0 // BREW Only
			this->loadMenuItems(menu, 0, -1);
			int v22;
			if (app->sound->allowSounds)
			{
				this->items[1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::SOUND_LABEL);
				this->items[2].flags = (this->activeSlider == SliderMode::SfxVolume) ? 0x10000 : 0 | this->items[2].flags & 0xFFFFFFFB;
				this->items[2].textField2 = this->getLastArgString();
			}
			else {
				this->items[1].textField2 =  Localization::STRINGID(Strings::FILE_MENUSTRINGS, (app->canvas->vibrateEnabled) ? 147 : 69);
				this->items[2].flags |= 0x8000u;
				this->items[2].textField2 = MenuSystem::EMPTY_TEXT;
			}

			this->items[3].textField2 = Localization::STRINGID(Strings::FILE_TRANSLATIONS, (short)0);
			this->items[3].param = 36;
			--this->numItems;
#endif
			break;
		}
#endif

		case Menus::MENU_INGAME_QUESTLOG: {
			this->LoadNotebook();
			break;
		}

		// Menus handled by YAML early return or default:
		// help menus (help_resource), SetYESNO dialogs (yesno),
		// ingame_recipes, ingame_loadnosave, comic_book

#if 0 // Old
		case Menus::MENU_INGAME_CONTROLS: {
			this->loadMenuItems(menu, 0, -1);
			textbuff->setLength(0);
			if (app->canvas->field_0xd10_ == 0) {
				this->items[0].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)392);
			}
			else if (app->canvas->field_0xd10_ == 1) {
				this->items[0].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)393);
			}
			else if (app->canvas->field_0xd10_ == 2) {
				this->items[0].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)394);
			}
			if (app->canvas->isFlipControls) {
				this->items[1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)197);
			}
			else {
				this->items[1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)198);
			}

			// [GEC]
			{
				this->items[this->numItems++].Set(Localization::STRINGID(MenuSystem::INDEX_OTHER, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGEALPHA, 3, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);

				textbuff->setLength(0);
				textbuff->append(" Alpha");
				app->localization->addTextArg(textbuff);
			}

			break;
		}
#endif

		case Menus::MENU_DEBUG: {
			this->loadMenuItems(menu, 0, -1);
			if (this->peekMenu() == 3) {
				this->items[0].action = 1;
				this->items[0].param = 3;
			}
			this->items[5].textField2 = this->onOffValue(app->player->noclip);
			this->items[6].textField2 = this->onOffValue(app->combat->oneShotCheat);
			this->items[7].textField2 = this->onOffValue(app->game->disableAI);
			this->items[8].textField2 = this->onOffValue(app->game->skipMinigames);
			this->items[9].textField2 = this->onOffValue(app->player->enableHelp);
			this->items[10].textField2 = this->onOffValue(app->player->god);
			this->items[11].textField2 = this->onOffValue(app->canvas->showLocation);
			app->localization->addTextArg(app->canvas->animFrames);
			this->items[12].textField2 = Localization::STRINGID((short)3, (short)241);
			this->items[13].textField2 = this->onOffValue(app->canvas->showSpeeds);
			this->items[14].textField2 = this->onOffValue(app->render->skipFlats);
			this->items[15].textField2 = this->onOffValue(app->render->skipCull);
			this->items[16].textField2 = this->onOffValue(app->render->skipBSP);
			this->items[17].textField2 = this->onOffValue(app->render->skipLines);
			this->items[18].textField2 = this->onOffValue(app->render->skipSprites);
			this->items[19].textField2 = this->onOffValue(app->canvas->renderOnly);
			this->items[20].textField2 = this->onOffValue(app->render->skipDecals);
			this->items[21].textField2 = this->onOffValue(app->render->skip2DStretch);
			if (app->render->renderMode == 0) {
				this->items[22].textField2 = Localization::STRINGID((short)3, (short)298);
			}
			else if (app->render->renderMode == 63) {
				this->items[22].textField2 = Localization::STRINGID((short)3, (short)299);
			}
			else if (app->render->renderMode == 31) {
				this->items[22].textField2 = Localization::STRINGID((short)3, (short)300);
			}
			else {
				Text* smallBuffer = app->localization->getSmallBuffer();
				smallBuffer->setLength(0);
				if ((app->render->renderMode & 0x1) != 0x0) {
					smallBuffer->append('T');
				}
				if ((app->render->renderMode & 0x2) != 0x0) {
					smallBuffer->append('C');
				}
				if ((app->render->renderMode & 0x4) != 0x0) {
					smallBuffer->append('P');
				}
				if ((app->render->renderMode & 0x8) != 0x0) {
					smallBuffer->append('S');
				}
				if ((app->render->renderMode & 0x10) != 0x0) {
					smallBuffer->append('R');
				}
				app->localization->addTextArg(smallBuffer);
				smallBuffer->dispose();
				this->items[22].textField2 = this->getLastArgString();
			}
			this->items[26].textField2 = this->onOffValue(app->canvas->showFreeHeap);
			break;
		}

		case Menus::MENU_DEBUG_MAPS: { // Menus::MENU_DEBUG_MAPS
			this->type = 1;
			this->addItem(Localization::STRINGID((short)3, (short)80), MenuSystem::EMPTY_TEXT, 0, 2, 0, MenuSystem::EMPTY_TEXT);
			if (this->peekMenu() == 77) {
				items[0].action = 26;
			}
			for (int k = 0; k < 10; ++k) {
				this->addItem(Localization::STRINGID((short)3, app->game->levelNames[k]), MenuSystem::EMPTY_TEXT, 0, 17, k, MenuSystem::EMPTY_TEXT);
			}
			break;
		}

		case Menus::MENU_DEBUG_STATS: {
			app->localization->resetTextArgs();
			/*app->localization->addTextArg((app->getTotalMemory() + 1023) / 1024);
			app->localization->addTextArg((int)(App.initialMemory + 1023L) / 1024);
			app->localization->addTextArg((App.getMemFootprint() + 1023) / 1024);
			app->localization->addTextArg((App.getFreeMemory() + 1023) / 1024);
			app->localization->addTextArg((App.findLargestMemoryBlock() + 1023) / 1024);
			app->localization->addTextArg(((int)App.peakMemoryUsage + 1023) / 1024);
			app->localization->addTextArg((Render.mapMemoryUsage + 1023) / 1024);
			app->localization->addTextArg((Render.spriteMem + 1023) / 1024);
			app->localization->addTextArg((Render.lineMem + 1023) / 1024);
			app->localization->addTextArg((Render.nodeMem + 1023) / 1024);
			largeBuffer.setLength(0);
			largeBuffer.append((Render.texelMemoryUsage + 1023) / 1024);
			largeBuffer.append('/');
			largeBuffer.append((Render.paletteMemoryUsage + 1023) / 1024);
			Text.addTextArg(largeBuffer);
			Text.addTextArg(App.imageMemory);
			loadMenuItems(n);
			items[1].textField2 = Text.STRINGID((short)3, (short)235);
			items[2].textField2 = Text.STRINGID((short)3, (short)236);
			items[3].textField2 = Text.STRINGID((short)3, (short)237);
			items[4].textField2 = Text.STRINGID((short)3, (short)238);
			items[5].textField2 = Text.STRINGID((short)3, (short)239);
			items[6].textField2 = Text.STRINGID((short)3, (short)240);
			items[7].textField2 = Text.STRINGID((short)3, (short)241);
			items[8].textField2 = Text.STRINGID((short)3, (short)242);
			items[9].textField2 = Text.STRINGID((short)3, (short)243);
			items[10].textField2 = Text.STRINGID((short)3, (short)244);
			items[11].textField2 = Text.STRINGID((short)3, (short)245);
			items[12].textField2 = Text.STRINGID((short)3, (short)246);
			*/
			break;
		}

		case Menus::MENU_SHOWDETAILS: {
			Text* weaponStatStr = nullptr;
			if (this->detailsDef != nullptr && this->detailsDef->eType == Enums::ET_ITEM && this->detailsDef->eSubType == Enums::ITEM_WEAPON) {
				weaponStatStr = app->combat->getWeaponStatStr(this->detailsDef->parm);
			}
			app->localization->resetTextArgs();
			this->type = 7;
			this->setItemsFromText(0, this->detailsTitle, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
			for (int n15 = 0; n15 < this->numItems; ++n15) {
				this->items[n15].flags |= 0x8;
			}
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 73, 0, 0, MenuSystem::EMPTY_TEXT);
			this->setItemsFromText(this->numItems, this->detailsHelpText, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
			if (weaponStatStr != nullptr) {
				this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 73, 0, 0, MenuSystem::EMPTY_TEXT);
				this->setItemsFromText(this->numItems, weaponStatStr, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
				weaponStatStr->dispose();
			}
			this->detailsTitle->dispose();
			this->detailsHelpText->dispose();
			break;
		}

		case Menus::MENU_ITEMS: {
			//this->loadMenuItems(menu, 0, 3); // BREW "resume game" item
			int n4 = 1;
			for (int n5 = 16; n5 < 18; ++n5) {
				short n6 = app->player->inventory[n5 - 0];
				if (n6 > 0) {
					if (n4 != 0) {
						n4 = 0;
						this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)112), MenuSystem::EMPTY_TEXT, 73, 0, 0, MenuSystem::EMPTY_TEXT);
					}
					textbuff->setLength(0);
					textbuff->append(n6);
					EntityDef* find = app->entityDefManager->find(6, 0, n5);
					app->localization->addTextArg(textbuff);
					this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->name), this->getLastArgString(), 32, 24, n5, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
				}
			}
			int fwIdx = app->combat->fountainWeaponIdx;
			int fwAmmoType = app->combat->fountainAmmoType;
			if (fwIdx >= 0 && fwAmmoType >= 0 && app->player->ammo[fwAmmoType] >= 25 && (app->player->weapons & (1 << fwIdx)) != 0x0) {
				textbuff->setLength(0);
				textbuff->append(app->player->ammo[fwAmmoType]);
				EntityDef* find2 = app->entityDefManager->find(6, 2, fwAmmoType);
				app->localization->addTextArg(textbuff);
				this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find2->name), this->getLastArgString(), 32, 24, 22, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find2->description));
			}
			int n7 = 1;
			for (int n8 = 11; n8 < 13; ++n8) {
				short n9 = app->player->inventory[n8 - 0];
				if (n9 > 0) {
					if (n7 != 0) {
						n7 = 0;
						this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)113), MenuSystem::EMPTY_TEXT, 73, 0, 0, MenuSystem::EMPTY_TEXT);
					}
					textbuff->setLength(0);
					textbuff->append(n9);
					EntityDef* find3 = app->entityDefManager->find(6, 0, n8);
					app->localization->addTextArg(textbuff);
					addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find3->name), this->getLastArgString(), 32, 24, n8, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find3->description));
				}
			}
			//this->loadMenuItems(menu, 3, 1);
			this->loadMenuItems(menu, 1, 1); // Load "inventory label" item
			if (app->player->hasANanoDrink()) {
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)178), Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)0), 0, 1, 75, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)179));
			}
			this->loadMenuItems(menu, 2, -1); // Load the remaining items

			//this->loadMenuItems(menu, 4, -1);
			textbuff->setLength(0);
			textbuff->append(app->player->inventory[24]);
			app->localization->addTextArg(textbuff);
			EntityDef* find4 = app->entityDefManager->find(6, 0, 24);
			this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find4->name), this->getLastArgString(), 32/*33*/, 16, 24, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find4->description));
			this->loadIndexes(4);

			this->selectedIndex = 0; // [GEC]
			break;
		}

		case Menus::MENU_ITEMS_WEAPONS: {
			this->loadMenuItems(menu, 0, -1);
			this->loadIndexes(1);

			this->selectedIndex = 0; // [GEC]

			for (int n10 = 0; n10 < 15; ++n10) {
				if ((app->player->weapons & 1 << n10) != 0x0) {
					int n11 = (app->combat->getWeaponFlags(app->player->ce->weapon).isThrowableItem && n10 != app->player->ce->weapon) ? 4 : 0;
					EntityDef* find5 = app->entityDefManager->find(6, 1, (uint8_t)n10);
					if (app->player->ce->weapon == n10) {
						n11 |= 0x400;
					}
					int n12 = n10 * 9;
					textbuff->setLength(0);
					if (app->combat->weapons[n12 + 4] != 0) {
						int ammoType = app->combat->weapons[n12 + 4];
						if (app->combat->getWeaponFlags(n10).soulAmmoDisplay) {
							textbuff->append(app->player->ammo[ammoType]);
							textbuff->append('/');
							textbuff->append(app->combat->weapons[n12 + Combat::WEAPON_FIELD_AMMOUSAGE]);
						}
						else {
							textbuff->append(app->player->ammo[ammoType]);
						}
					}
					else {
						textbuff->append('\x80');
					}
					app->localization->addTextArg(textbuff);
					this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find5->name), this->getLastArgString(), n11 | 0x20, 18, n10, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find5->description));
				}
			}
			break;
		}

		case Menus::MENU_ITEMS_DRINKS: {
			this->loadMenuItems(menu, 0, -1);
			this->loadIndexes(2);
			this->selectedIndex = 0; // [GEC]

			bool b = false;
			for (int n13 = 0; n13 < 11; ++n13) {
				short n14 = app->player->inventory[n13 - 0];
				if (n14 > 0) {
					b = true;
					textbuff->setLength(0);
					textbuff->append(n14);
					EntityDef* find6 = app->entityDefManager->find(6, 0, n13);
					app->localization->addTextArg(textbuff);
					this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find6->name), this->getLastArgString(), 32, 24, n13, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find6->description));
				}
			}
			if (!b) {
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)1), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT); // [None]
			}
			break;
		}

		case Menus::MENU_ITEMS_CONFIRM: {
			this->type = 6;
			this->scrollIndex = 0;
			EntityDef* find7 = app->entityDefManager->find(6, 0, this->menuParam);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_ENTITYSTRINGS, find7->name, textbuff);
			app->localization->addTextArg(textbuff);
			if (this->peekMenu() == 75) {
				this->SetYESNO((short)184, 1, 20, this->menuParam);
				break;
			}
			this->SetYESNO((short)184, 1, 21, this->menuParam);
			break;
		}

		case Menus::MENU_VENDING_MACHINE: {
			//this->scrollIndex = 0; // [GEC]
			//this->selectedIndex = 3; // [GEC]

			this->loadMenuItems(menu, 0, 1);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(app->player->inventory[24]);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, (short)197, textbuff);
			app->localization->resetTextArgs();
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);
			break;
		}

		case Menus::MENU_VENDING_MACHINE_DRINKS: {
			//this->scrollIndex = 0; // [GEC]
			//this->selectedIndex = 3; // [GEC]

			this->loadMenuItems(menu, 0, 1);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(app->player->inventory[24]);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, (short)197, textbuff);
			app->localization->resetTextArgs();
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);

			for (int n16 = 0; n16 < 11; ++n16) {
				if (app->vendingMachine->drinkInThisVendingMachine(n16)) {
					textbuff->setLength(0);
					textbuff->append(app->vendingMachine->getDrinkPrice(n16));
					EntityDef* entDef = app->entityDefManager->find(6, 0, n16);
					app->localization->addTextArg(textbuff);
					this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entDef->name), this->getLastArgString(), 32, 28, n16, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entDef->description));
				}
			}
			break;
		}

		case Menus::MENU_VENDING_MACHINE_SNACKS: {
			//this->scrollIndex = 0; // [GEC]
			//this->selectedIndex = 3; // [GEC]

			this->loadMenuItems(menu, 0, 1);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(app->player->inventory[24]);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, (short)197, textbuff);
			app->localization->resetTextArgs();
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);
			this->fillVendingMachineSnacks(app->canvas->loadMapID, textbuff);
			break;
		}

		case Menus::MENU_VENDING_MACHINE_CONFIRM: {
			bool b2 = false;
			if (this->menuParam >= 0 && this->menuParam < 11) {
				b2 = true;
			}
			EntityDef* entityDef;
			int currentItemPrice;
			int n17;
			int n18;
			if (b2) {
				entityDef = app->entityDefManager->find(6, 0, this->menuParam);
				currentItemPrice = app->vendingMachine->getDrinkPrice(this->menuParam);
				n17 = Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entityDef->name);
				n18 = 29;
			}
			else {
				entityDef = app->entityDefManager->find(6, 0, 16);
				currentItemPrice = app->vendingMachine->getSnackPrice();
				n17 = this->menuParam;
				n18 = 30;
			}

			app->vendingMachine->currentItemPrice = currentItemPrice;
			this->loadMenuItems(menu, 0, 1);
			Text* smallBuffer5 = app->localization->Localization::getSmallBuffer();
			app->localization->resetTextArgs();
			smallBuffer5->setLength(0);
			smallBuffer5->append(app->player->inventory[24]);
			app->localization->addTextArg(smallBuffer5);
			smallBuffer5->setLength(0);
			app->localization->composeText(0, 197, smallBuffer5);
			Text* smallBuffer6 = app->localization->getSmallBuffer();
			app->localization->resetTextArgs();
			smallBuffer6->setLength(0);
			app->localization->composeText(n17, smallBuffer6);
			app->localization->addTextArg(smallBuffer6);
			smallBuffer6->setLength(0);
			smallBuffer6->append(app->vendingMachine->currentItemQuantity);
			app->localization->addTextArg(smallBuffer6);
			smallBuffer6->setLength(0);
			app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::COST_ITEM, smallBuffer6);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(currentItemPrice * app->vendingMachine->currentItemQuantity);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::COST_ITEM_EMPTY, textbuff);
			app->localization->resetTextArgs();

			app->localization->addTextArg(smallBuffer5);
			smallBuffer5->dispose();
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);

			app->localization->addTextArg(smallBuffer6);
			smallBuffer6->dispose();
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 0, n18, this->menuParam, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entityDef->description));
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);

			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem((Localization::STRINGID(Strings::FILE_MENUSTRINGS, 346)), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem((Localization::STRINGID(Strings::FILE_MENUSTRINGS, 347)), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem((Localization::STRINGID(Strings::FILE_MENUSTRINGS, 348)), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem((Localization::STRINGID(Strings::FILE_MENUSTRINGS, 349)), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			break;
		}

		case Menus::MENU_VENDING_MACHINE_CANT_BUY: {
			bool b3 = false;
			if (this->menuParam >= 0 && this->menuParam < 11) {
				b3 = true;
			}
			EntityDef* entityDef2;
			int n21;
			int n22;
			if (b3) {
				entityDef2 = app->entityDefManager->find(6, 0, this->menuParam);
				n21 = app->vendingMachine->getDrinkPrice(this->menuParam);
				n22 = Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entityDef2->name);
			}
			else {
				entityDef2 = app->entityDefManager->find(6, 0, 16);
				n21 = app->vendingMachine->getSnackPrice();
				n22 = this->menuParam;
			}
			this->loadMenuItems(menu, 0, 1);
			Text* smallBuffer7 = app->localization->getSmallBuffer();
			app->localization->resetTextArgs();
			smallBuffer7->setLength(0);
			smallBuffer7->append(app->player->inventory[24]);
			app->localization->addTextArg(smallBuffer7);
			smallBuffer7->setLength(0);
			app->localization->composeText((short)0, (short)197, smallBuffer7);
			Text* smallBuffer8 = app->localization->getSmallBuffer();
			app->localization->resetTextArgs();
			smallBuffer8->setLength(0);
			app->localization->composeText(n22, smallBuffer8);
			app->localization->addTextArg(smallBuffer8);
			smallBuffer8->setLength(0);
			smallBuffer8->append(app->vendingMachine->currentItemQuantity);
			app->localization->addTextArg(smallBuffer8);
			smallBuffer8->setLength(0);
			app->localization->composeText(Strings::FILE_MENUSTRINGS, (short)350, smallBuffer8);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(n21 * app->vendingMachine->currentItemQuantity);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_MENUSTRINGS, (short)351, textbuff);
			app->localization->resetTextArgs();
			app->localization->addTextArg(smallBuffer7);
			smallBuffer7->dispose();
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);
			app->localization->addTextArg(smallBuffer8);
			smallBuffer8->dispose();
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)354), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(Localization::STRINGID(Strings::FILE_CODESTRINGS, (short)96), MenuSystem::EMPTY_TEXT, 8, 2, 0, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entityDef2->description));
			break;
		}

		case Menus::MENU_VENDING_MACHINE_DETAILS: {
			app->localization->resetTextArgs();
			this->type = 9;
			this->setItemsFromText(0, this->detailsTitle, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
			for (int n19 = 0; n19 < this->numItems; ++n19) {
				this->items[n19].flags |= 0x8;
			}
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 72, 0, 0, MenuSystem::EMPTY_TEXT);
			this->setItemsFromText(this->numItems, this->detailsHelpText, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
			items[0].action = 28;
			items[0].param = this->menuParam;
			for (int n20 = 1; n20 < this->numItems; ++n20) {
				this->items[n20].flags |= 0x1;
			}
			this->detailsTitle->dispose();
			this->detailsHelpText->dispose();
			break;
		}

		case Menus::MENU_INGAME_OPTIONS_SOUND:
		case Menus::MENU_MAIN_OPTIONS_SOUND: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_OPTIONS_SOUND) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBARTWO, Menus::ACTION_CHANGESFXVOLUME, 1, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBARTWO, Menus::ACTION_CHANGEMUSICVOLUME, 2, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGESFXVOLUME, 1, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGEMUSICVOLUME, 2, MenuSystem::EMPTY_TEXT);
				}
				textbuff->setLength(0); textbuff->append("Sound Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("SoundFx Volume:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Music Volume:"); app->localization->addTextArg(textbuff);
			}
			if (!isUserMusicOn()) {
				this->items[3].flags |= Menus::ITEM_DISABLEDTWO;
			}
			break;
		}

		case Menus::MENU_INGAME_OPTIONS_VIDEO:
		case Menus::MENU_MAIN_OPTIONS_VIDEO: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_OPTIONS_VIDEO) {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
				} else {
					this->maxItems = 5;
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				}
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), 0, Menus::ACTION_CHANGE_VID_MODE, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_TOG_VSYNC, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6), 0, Menus::ACTION_CHANGE_RESOLUTION, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_TOG_TINYGL, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT8), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_APPLY_CHANGES, 0, MenuSystem::EMPTY_TEXT);
				textbuff->setLength(0); textbuff->append("Video Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Window Mode:"); app->localization->addTextArg(textbuff);
			}
			// Dynamic runtime value updates for text2 fields
			{
				int argIdx = app->localization->numTextArgs;
				int windowMode = this->sdlGL->windowMode;
				textbuff->setLength(0);
				if (windowMode == 0) textbuff->append("Windowed");
				else if (windowMode == 1) textbuff->append("Borderless");
				else if (windowMode == 2) textbuff->append("FullScreen");
				app->localization->addTextArg(textbuff);
				this->items[1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1 + argIdx);
				argIdx++;

				this->items[2].textField2 = this->onOffValue(this->sdlGL->vSync);

				int resolutionIndex = this->sdlGL->resolutionIndex;
				textbuff->setLength(0);
				textbuff->append("(")->append(sdlResVideoModes[resolutionIndex].width)->append("x")->append(sdlResVideoModes[resolutionIndex].height)->append(")");
				app->localization->addTextArg(textbuff);
				this->items[3].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1 + argIdx);
				argIdx++;

				if (!_glesObj->isInit) {
					this->items[4].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)197);
				} else {
					this->items[4].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)198);
				}

				textbuff->setLength(0); textbuff->append("VSync:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Resolution:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("TinyGL:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Apply Changes"); app->localization->addTextArg(textbuff);
			}
			break;
		}

		case Menus::MENU_INGAME_OPTIONS_INPUT:
		case Menus::MENU_MAIN_OPTIONS_INPUT: {  // [GEC]
			this->nextMsgTime = 0;
			this->nextMsg = 0;
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_OPTIONS_INPUT) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_CONTROLS, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_BINDINGS, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_CONTROLLER, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_CONTROLS, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_BINDINGS, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_CONTROLLER, MenuSystem::EMPTY_TEXT);
				}
				textbuff->setLength(0); textbuff->append("Input Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Touch Controls"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Bindings      "); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Controller    "); app->localization->addTextArg(textbuff);
			}
			break;
		}

		case Menus::MENU_INGAME_CONTROLS:  // [GEC]
		case Menus::MENU_MAIN_CONTROLS: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				int itemIndx = 2;
				if (menu == Menus::MENU_MAIN_CONTROLS) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
				} else {
					itemIndx = 1;
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				}
				textbuff->setLength(0); textbuff->append("Touch Controls Options"); app->localization->addTextArg(textbuff);
				this->loadMenuItems(Menus::MENU_INGAME_CONTROLS, 0, -1);
				if (menu == Menus::MENU_MAIN_CONTROLS) {
					this->items[this->numItems++].Set(Localization::STRINGID(MenuSystem::INDEX_OTHER, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBARTWO, Menus::ACTION_CHANGEALPHA, 3, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(MenuSystem::INDEX_OTHER, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGEALPHA, 3, MenuSystem::EMPTY_TEXT);
				}
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
				textbuff->setLength(0); textbuff->append("Alpha:"); app->localization->addTextArg(textbuff);
			}
			// Dynamic runtime value updates for control layout and flip controls
			{
				int itemIndx = (menu == Menus::MENU_MAIN_CONTROLS) ? 2 : 1;
				if (app->canvas->m_controlLayout == 0) {
					this->items[itemIndx].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)392);
				}
				else if (app->canvas->m_controlLayout == 1) {
					this->items[itemIndx].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)393);
				}
				else if (app->canvas->m_controlLayout == 2) {
					this->items[itemIndx].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)394);
				}

				if (app->canvas->isFlipControls) {
					this->items[itemIndx + 1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)197);
				}
				else {
					this->items[itemIndx + 1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)198);
				}
			}
			break;
		}

		case Menus::MENU_INGAME_BINDINGS:
		case Menus::MENU_MAIN_BINDINGS: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_BINDINGS) {
					this->maxItems = 8;
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
				} else {
					this->maxItems = 5;
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				}
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 1, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 4, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 5, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT8), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 2, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT9), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 3, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT10), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT11), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 8, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT12), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 6, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT13), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 7, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT14), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 9, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT15), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 10, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT16), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 11, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT17), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 12, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT18), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 13, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT19), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 14, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT20), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 15, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT21), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_DEFAULT_BINDINGS, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT, 0, 0, MenuSystem::EMPTY_TEXT);
				textbuff->setLength(0); textbuff->append("Bindings Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("MOVEMENT"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Unbound"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Move Forward:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Move Back:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Strafe Left:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Strafe Right:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Turn Left:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Turn Right:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("ACTIONS"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Attack/Use:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Next Weapon:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Prev Weapon:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Pass Turn:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Automap:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Items:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Drinks:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("PDA:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Sniper Mode:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Reset Defaults"); app->localization->addTextArg(textbuff);
			}
			break;
		}

		case Menus::MENU_INGAME_CONTROLLER:
		case Menus::MENU_MAIN_CONTROLLER: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_CONTROLLER) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
				}
				textbuff->setLength(0); textbuff->append("Controller Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Vibration:"); app->localization->addTextArg(textbuff);
			}
			// Dynamic runtime value: vibration on/off
			{
				int itemIndx = (menu == Menus::MENU_MAIN_CONTROLLER) ? 2 : 1;
				this->items[itemIndx].textField2 = this->onOffValue(app->canvas->vibrateEnabled);
			}
			break;
		}

		default: {
			// Missing menus (injected from YAML, not in menuData) have no binary items
			if (menuDef && menuDef->isMissing) break;
			int startItem = (menuDef) ? menuDef->loadStartItem : 0;
			this->loadMenuItems(menu, startItem, -1);
			break;
		}
	}

	this->updateTouchButtonState();
	textbuff->dispose();

	// [GEC] Restored From J2ME/BREW
	if (this->selectedIndex >= this->numItems) {
		this->moveDir(-1);
	}
	else if (this->items[this->selectedIndex].textField == MenuSystem::EMPTY_TEXT || (this->items[selectedIndex].flags & 0x8001) != 0) {
		this->moveDir(1);
	}

	int y, h;
	int iVar16, iVar14;
	int iVar18 = 0;
	int oldMenu = this->numItems;
	MenuSystem* pMVar15 = this;
	for (iVar16 = 0; iVar16 < this->numItems; iVar16++) {
		if ((pMVar15->items[iVar16].flags & 0x8000U) == 0) {
			iVar14 = this->getMenuItemHeight(iVar16);
			iVar18 = iVar18 + iVar14;
		}
	}

	//printf("menuRect[3] %d\n", app->canvas->menuRect[3]);
	//printf("iVar18 %d\n", iVar18);
	if (app->canvas->menuRect[3] < iVar18) {
		//printf("true\n");
		this->m_scrollBar->SetScrollBarImages(nullptr, nullptr, nullptr, nullptr);

		// Data-driven scrollbar configuration from theme
		const MenuTheme* sbTheme = getThemeForMenu(menu);
		const YAMLMenuDef* sbDef = getMenuDef(menu);
		ScrollbarStyle sbStyle = (sbTheme && sbTheme->scrollbar.style != SB_NONE) ? sbTheme->scrollbar.style : SB_DIAL;

		if (sbStyle == SB_BAR) {
			const ScrollbarConfig& sbc = sbTheme->scrollbar;
			h = sbc.barImg ? sbc.barImg->height : 0;
			y = app->canvas->menuRect[1] + (app->canvas->menuRect[3] - h >> 1);
			this->m_scrollBar->SetScrollBarImages(sbc.barImg, sbc.topImg, sbc.midImg, sbc.bottomImg);
			iVar16 = sbc.x;
			iVar14 = sbc.width;
			oldMenu = 0;
		} else {
			// Dial scrollbar (main menu style)
			if (sbDef && sbDef->scrollbarXOffset >= 0) {
				iVar16 = app->canvas->menuRect[0] + sbDef->scrollbarXOffset;
				y = app->canvas->menuRect[1] + sbDef->scrollbarYOffset;
			} else {
				int defX = (sbTheme) ? sbTheme->scrollbar.defaultX : 408;
				int defY = (sbTheme) ? sbTheme->scrollbar.defaultY : 81;
				iVar16 = defX;
				y = defY;
			}
			iVar14 = this->imgMenuDial->width;
			h = this->imgMenuDial->height;
			oldMenu = h + 0xf;
			if (-1 < h) {
				oldMenu = h;
			}
			oldMenu = oldMenu >> 4;
			this->isMainMenuScrollBar = true;
		}

		this->m_scrollBar->enabled = 1;
		this->m_scrollBar->barRect.Set(iVar16, y, iVar14, h);
		if (oldMenu == 0) {
			this->m_scrollBar->SetScrollBox
			(app->canvas->menuRect[0], app->canvas->menuRect[1], app->canvas->menuRect[2],
				app->canvas->menuRect[3], iVar18);
		}
		else {
			this->m_scrollBar->SetScrollBox
			(app->canvas->menuRect[0], app->canvas->menuRect[1], app->canvas->menuRect[2],
				app->canvas->menuRect[3], iVar18 - 50, oldMenu);
		}

		// [GEC] salva posiciones actuales
		//this->btnScroll->field_0x44_ = this->old_0x44;
		//this->btnScroll->field_0x48_ = this->old_0x48;

	}
	else {
		this->m_scrollBar->enabled = 0;
	}
	if (((menu < Menus::MENU_INGAME) && (this->type != 5 && menu != Menus::MENU_LEVEL_STATS)) && (menu != Menus::MENU_END_RANKING)) {
		this->isMainMenu = true;
	}
}


void MenuSystem::addItem(int textField, int textField2, int flags, int action, int param, int helpField) {

	if (this->numItems == 50) {
		app->Error(100); // ERR_MAXMENUITEMS
	}
	this->items[this->numItems++].Set(textField, textField2, flags, action, param, helpField);
}

void MenuSystem::loadMenuItems(int menu, int begItem, int numItems) {


	int length = this->menuDataCount;
	for (int j = 0; j < length; ++j) {
		if ((this->menuData[j] & 0xFF) == menu) {
			int n4 = this->menuData[j];
			this->type = (n4 & 0xFF000000) >> 24;
			int n5 = 0;
			if (j != 0) {
				n5 = (this->menuData[j - 1] & 0xFFFF00) >> 8;
			}
			if (numItems == -1) {
				numItems = (((n4 & 0xFFFF00) >> 8) - n5) / 2;
				numItems -= begItem;
			}
			int n6 = n5 + begItem * 2;
			for (int k = 0; k < numItems; ++k) {
				int n7 = this->menuItems[n6 + 0];
				int n8 = this->menuItems[n6 + 1];
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (n7 >> 16)),
					Localization::STRINGID(Strings::FILE_MENUSTRINGS, 0), (n7 & 0xFFFF), ((n8 & 0xFF00) >> 8), (n8 & 0xFF), 
					Localization::STRINGID(Strings::FILE_MENUSTRINGS, ((n8 & 0xFFFF0000) >> 16)));
				n6 += 2;
			}
			return;
		}
	}

	app->Error(29); // ERR_LOADMENUITEMS
}

