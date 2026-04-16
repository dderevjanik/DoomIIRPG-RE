#include <algorithm>
#include <print>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
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


void MenuSystem::paint(Graphics* graphics) {

	Canvas* canvas; // r1
	int* screenRect; // r5
	int ScrollPos; // r0
	int yPos; // r10
	bool v17; // zf
	bool v22; // zf
	int v29; // r3
	bool v30; // zf
	bool v31; // zf
	MenuItem* items; // r5
	int v33; // r6
	int numItems; // r2
	int v35; // r10
	int* v36; // r12
	int v37; // r0
	int menuItem_width; // r3
	int flags; // r3
	int v40; // r0
	int action; // r1
	int v42; // r12
	int v43; // r2
	int textField2; // r3
	int v45; // r4
	int v46; // r3
	bool v47; // zf
	bool v49; // zf
	int v50; // r4
	int v55; // r6
	int v56; // r0
	Image* imgGameMenuTornPage; // r10
	VendingMachine* vendingMachine; // r1
	float v59; // r12
	int v60; // r3
	float v61; // r6
	Text* v62; // r0
	Canvas* v63; // r5
	Text* v64; // r4
	int* menuRect; // [sp+3Ch] [bp-44h]
	Text* textBuffer1; // [sp+40h] [bp-40h]
	Text* textBuffer2; // [sp+44h] [bp-3Ch]
	int v76; // [sp+48h] [bp-38h]
	int v78; // [sp+50h] [bp-30h]
	int v79; // [sp+54h] [bp-2Ch]
	int v80; // [sp+58h] [bp-28h]
	int v81; // [sp+5Ch] [bp-24h]
	int v82; // [sp+60h] [bp-20h]
	int menuHelpMaxChars; // [sp+64h] [bp-1Ch]

	if (this->menu == Menus::MENU_COMIC_BOOK) {
		app->comicBook->Draw(graphics);
		return;
	}
	canvas = app->canvas.get();
	menuRect = canvas->menuRect;
	screenRect = canvas->screenRect;
	textBuffer1 = app->localization->getLargeBuffer();
	textBuffer2 = app->localization->getLargeBuffer();
	graphics->clipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
	ScrollPos = this->getScrollPos();
	v76 = ScrollPos;

	if (this->menu >= Menus::MENU_INGAME) {
		graphics->drawImage(app->menuSystem->imgGameMenuPanelbottom, 0, Applet::IOS_HEIGHT - this->imgGameMenuPanelbottom->height, 0, 0, 0);
		if (this->menu >= Menus::MENU_VENDING_MACHINE && this->menu <= Menus::MENU_VENDING_MACHINE_LAST) {
			graphics->drawImage(app->vendingMachine->imgVendingBG, 0, 0, 0, 0, 0);
		}
		else {
			graphics->drawImage(this->imgGameMenuBackground, 0, 0, 0, 0, 0);
			yPos = 20 + (Applet::IOS_HEIGHT - this->imgGameMenuPanelbottom->height);
			// Health
			textBuffer1->setLength(0);
			textBuffer1->append(app->player->ce->getStat(Enums::STAT_HEALTH))->append("/")->append(app->player->ce->getStat(Enums::STAT_MAX_HEALTH));
			while (textBuffer1->length() <= 6) {
				textBuffer1->append(' ');
			}
			textBuffer1->append("  ");

			// Shield
			textBuffer2->setLength(0);
			textBuffer2->append(app->player->ce->getStat(Enums::STAT_ARMOR))->append("/")->append(200);
			while (textBuffer2->length() <= 6) {
				textBuffer2->insert(' ', 0);
			}
			textBuffer1->append(textBuffer2);
			graphics->drawString(textBuffer1, app->canvas->SCR_CX, yPos + 3, 3);
			graphics->drawImage(this->imgGameMenuHealth, ((Applet::IOS_WIDTH - textBuffer1->getStringWidth()) >> 1) - 5, yPos + 3, 10, 0, 0);
			graphics->drawImage(this->imgGameMenuShield, ((textBuffer1->getStringWidth() + Applet::IOS_WIDTH) >> 1) + 5, yPos + 3, 6, 0, 0);
		}
	}
	else if ((this->menu == Menus::MENU_END_RANKING) || (this->menu == Menus::MENU_LEVEL_STATS)) {
		graphics->drawImage(app->canvas->imgEndOfLevelStatsBG, 0, 0, 0, 0, 0);
	}
	else {
		// Render image items (type: background / type: image) from YAML menu def
		const YAMLMenuDef* imgDef = getMenuDef(this->menu);
		if (imgDef) {
			for (const auto& mi : imgDef->items) {
				if (!(mi.flags & Menus::ITEM_IMAGE)) continue;
				Image* img = resolveMenuImage(mi.imageName);
				if (!img) continue;
				int x = (mi.imageX == -1) ? ((Applet::IOS_WIDTH - img->width) >> 1) : mi.imageX;
				int y = mi.imageY;
				graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
				graphics->drawImage(img, x, y, mi.imageAnchor, 0, mi.imageRenderMode);
			}
		}
	}

	this->drawSoftkeyButtons(graphics);

	graphics->setClipRect(screenRect[0], menuRect[1], screenRect[2], menuRect[3]);

	if (this->menu == Menus::MENU_INGAME_CONTROLS) {
		app->hud->drawArrowControls(graphics);
	}

	if (this->menu == Menus::MENU_MAIN_CONTROLS) {
		graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
		app->hud->drawArrowControls(graphics);
	}

	graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);

	if (this->menu == Menus::MENU_MAIN_OPTIONS || 
		this->menu == Menus::MENU_INGAME_OPTIONS ||
		this->menu == Menus::MENU_INGAME_CONTROLS) {
		//this->drawOptionsScreen(graphics); // Old
	}
	else if (this->menu < Menus::MENU_INGAME || this->menu != Menus::MENU_MAIN_MORE_GAMES) {
		this->drawButtonFrame(graphics); // Usado en Wolfenstein RPG
	}

	// Clip rect: menuRect is authoritative (set by setMenuSettings from YAML layout)
	// Ingame/vending menus use tight clip; main menus use loose clip for visual overflow
	if (this->menu >= Menus::MENU_INGAME) {
		graphics->setClipRect(screenRect[0], menuRect[1], screenRect[2], menuRect[3]);
	} else {
		graphics->setClipRect(screenRect[0], menuRect[1] - 15, screenRect[2], menuRect[3] + 30);
	}
	this->drawTouchButtons(graphics, false);

	if (this->menu != Menus::MENU_MAIN_OPTIONS) {
		goto LABEL_66;
	}
	if (this->HasVibration()) {
		this->drawTouchButtons(graphics, true);
	}
	if (this->menu != Menus::MENU_MAIN_OPTIONS || !HasVibration()) {
	LABEL_66:
		this->drawTouchButtons(graphics, true);
	}

	if ((this->menu != Menus::MENU_MAIN_CONFIRMNEW) &&
		(this->menu != Menus::MENU_MAIN_OPTIONS) &&
		(this->menu != Menus::MENU_MAIN_DIFFICULTY) &&
		(this->menu != Menus::MENU_MAIN_MINIGAME) &&
		(this->menu != Menus::MENU_MAIN_MORE_GAMES) &&
		(this->menu != Menus::MENU_MAIN_EXIT) &&
		(this->menu != Menus::MENU_END_) &&
		(this->menu != Menus::MENU_END_FINALQUIT)
		) {
		this->drawScrollbar(graphics);
	}

	/*v29 = this->menu;
	v30 = v29 == Menus::MENU_MAIN_CONFIRMNEW;
	if (v29 != Menus::MENU_MAIN_CONFIRMNEW)
		v30 = v29 == Menus::MENU_MAIN_OPTIONS;
	if (!v30)
	{
		v31 = v29 == Menus::MENU_MAIN_DIFFICULTY;
		if (v29 != Menus::MENU_MAIN_DIFFICULTY)
			v31 = v29 == Menus::MENU_MAIN_MINIGAME;
		if (!v31)
		{
			v31 = v29 == Menus::MENU_MAIN_MORE_GAMES;
			if (v29 != Menus::MENU_MAIN_MORE_GAMES)
			{
				v31 = v29 == Menus::MENU_MAIN_EXIT;
				if (v29 != Menus::MENU_MAIN_EXIT)
					v31 = v29 == Menus::MENU_END_;
			}
		}
		if (!v31 && v29 != Menus::MENU_END_FINALQUIT) {
			this->drawScrollbar(graphics);
		}
	}*/

	items = this->items;
	v33 = -v76;
	numItems = this->numItems;
	v79 = 0;
	v78 = menuRect[2] / Applet::CHAR_SPACING[app->fontType];

	while (v79 < numItems && v33 < app->canvas->menuRect[3])
	{
		bool isScrollBar = (items->flags & Menus::ITEM_SCROLLBAR); // [GEC]
		bool isScrollBarTwo = (items->flags & Menus::ITEM_SCROLLBARTWO); // [GEC]
		action = (items->action && !isScrollBar && !isScrollBarTwo) ? items->action : 0; // [GEC]

		if ((items->flags & 0x8000) != 0)
		{
			v35 = v33;
			goto LABEL_155;
		}
		v35 = v33 + this->getMenuItemHeight(v79);

		if ((items->flags & Menus::ITEM_DISABLEDTWO) != 0) { // [GEC]
			app->setFontRenderMode(2);
		}

		if (v35 > 0)
		{
			v80 = v33 + menuRect[1];
			if (items->textField != 3072 || (items->flags & 0x40) != 0)
			{
				textBuffer1->setLength(0);
				if ((items->flags & 0x400) != 0)
				{
					textBuffer1->append('\x87');
					textBuffer1->append(" ");
				}
				app->localization->composeTextField(items->textField, textBuffer1);
				if ((items->flags & 2) == 0) {
					textBuffer1->dehyphenate();
				}
				if (action)
				{
					v37 = textBuffer1->getStringWidth();
					menuItem_width = this->menuItem_width;
					if (v37 + 10 > menuItem_width)
					{
						textBuffer1->setLength((menuItem_width - 10) / Applet::FONT_WIDTH[app->fontType]);
						textBuffer1->append('\x85');
					}
				}
				flags = items->flags;
				if ((flags & 0x40) != 0)
				{
					this->buildDivider(textBuffer1, v78);
					flags = items->flags;
				}
				if ((flags & 8) != 0)
				{
					v40 = textBuffer1->getStringWidth(false);
					v42 = v40 >> 1;
					if (action)
					{
						v81 = menuRect[0] + (this->menuItem_width >> 1) - v42;
						goto LABEL_130;
					}
					v81 = menuRect[0] + (menuRect[2] >> 1) - v42;
				LABEL_121:
					v46 = this->menu;
					v47 = v46 == Menus::MENU_MAIN_EXIT;
					if (v46 != Menus::MENU_MAIN_EXIT)
						v47 = v46 == Menus::MENU_END_RANKING;
					if (v47 || v46 == Menus::MENU_MAIN_DIFFICULTY || v46 == Menus::MENU_MAIN_MINIGAME || v46 == Menus::MENU_END_ || v46 == Menus::MENU_END_FINALQUIT || v46 == Menus::MENU_SELECT_LANGUAGE)
					{
						v80 += 8;
						goto LABEL_132;
					}
				}
				else
				{
					v43 = menuRect[0];

					if (action)
						flags = v43 + 8;
					else
						v81 = menuRect[0];

					if (action)
						v81 = flags;

					textField2 = items->textField2;
					if (textField2 != 3072)
					{
						if (action)
							textField2 = this->menuItem_width;
						else
							v36 = menuRect + 2;

						if (action)
							v43 += textField2;
						else
							textField2 = *v36;

						if (action)
							v45 = v43 - 8;
						else
							v45 = v43 + textField2;

						textBuffer2->setLength(0);
						app->localization->composeTextField(items->textField2, textBuffer2);
						if (!(items->flags & Menus::ITEM_NODEHYPHENATE)) {
							textBuffer2->dehyphenate();
						}

						if ((items->flags & Menus::ITEM_BINDING) && ((this->menu == Menus::MENU_MAIN_BINDINGS)  || (this->menu == Menus::MENU_INGAME_BINDINGS))) { // New Flag
							if (app->time > this->nextMsgTime) {
								this->nextMsgTime = app->time + 1000;
								this->nextMsg++;
							}

							int j;
							for (j = 0; j < KEYBINDS_MAX; j++) {
								if (keyMappingTemp[items->param].keyBinds[j] == -1) {
									break;
								}
							}

							if (j != 0) {
								textBuffer2->setLength(0);
								if (keyMappingTemp[items->param].keyBinds[this->nextMsg % j] & IS_CONTROLLER_BUTTON) {
									textBuffer2->append(buttonNames[keyMappingTemp[items->param].keyBinds[this->nextMsg % j] & ~(IS_CONTROLLER_BUTTON | IS_MOUSE_BUTTON)]);
								}
								else {
									textBuffer2->append((char*)SDL_GetScancodeName((SDL_Scancode)keyMappingTemp[items->param].keyBinds[this->nextMsg % j]));
								}
							} else if (textBuffer2->length() == 0) {
								textBuffer2->append("Unbound");
							}
						}
						
						if (action) {
							graphics->drawString(textBuffer2, v45, v80 + (this->menuItem_height >> 1), 10);
						}
						else {
							graphics->drawString(textBuffer2, v45, v80, 24);
						}
						
					}
					if (!action)
						goto LABEL_121;
				LABEL_130:
					v46 = this->menu;
				}
				v49 = v46 == Menus::MENU_MAIN_CONFIRMNEW;
				if (v46 == Menus::MENU_MAIN_CONFIRMNEW)
					v49 = v79 == 3;
				if (v49)
					v80 += 7;


				if (isScrollBarTwo) { // [GEC]
					v81 -= (v81 - ((Applet::IOS_WIDTH - this->imgMenuOptionBOX3->width) >> 1)) - 15;
				}

				if (this->type != 5 && this->type != 7 && this->menu != Menus::MENU_VENDING_MACHINE_CANT_BUY && this->menu != Menus::MENU_VENDING_MACHINE_DETAILS && this->menu != Menus::MENU_VENDING_MACHINE_CONFIRM && v79 == this->selectedIndex) {
					int n11 = v81 + canvas->OSC_CYCLE[app->time / 100 % 4];
					if (action) {
						graphics->drawCursor(n11 + 3, v80 + (this->menuItem_height >> 1) - 8, 8);
					}
					else {
						graphics->drawCursor(n11 + 3, v80, 8);
					}
					v81 += 8;
				}

				if (action)
					graphics->drawString(textBuffer1, v81, v80 + (this->menuItem_height >> 1), 2);
				else
					LABEL_132:
				graphics->drawString(textBuffer1, v81, v80, 0);


				if (isScrollBar || isScrollBarTwo) { //[GEC] New system
					this->drawCustomScrollbar(graphics, items, textBuffer1, (action) ? v80 + (this->menuItem_height >> 1) : v80);
				}
				
				if (items->flags & Menus::ITEM_DISABLED) {
					if (action) {
						if (this->menu == Menus::MENU_MAIN_OPTIONS) {
							graphics->FMGL_fillRect(menuRect[0] + 4, v33 + menuRect[1] + 6,
								this->menuItem_width - 7, this->menuItem_height - 15,
								0.2, 0.2, 0.2, 0.5);
						}
						else {
							graphics->FMGL_fillRect(menuRect[0],v33 + menuRect[1],
								this->menuItem_width, this->menuItem_height,
								this->m_menuButtons->GetButton(0)->normalRed,
								this->m_menuButtons->GetButton(0)->normalGreen,
								this->m_menuButtons->GetButton(0)->normalBlue,
								this->m_menuButtons->GetButton(0)->normalAlpha);
						}
					}
					else
					{
						v55 = textBuffer1->length();
						textBuffer1->setLength(0);
						v50 = 0;
						while (v50 < v55)
						{
							textBuffer1->append('\x89');
							++v50;
						}
						graphics->drawString(textBuffer1, v81, v80, 0);
					}
				}
				if (items->flags & Menus::ITEM_LEFT_ARROW) {
					graphics->drawRegion(app->hud->imgAttArrow, 0, 0, 12, 12, v81 - 17, v80, 0, 1, 0);
				}
				if (items->flags & Menus::ITEM_RIGHT_ARROW) {
					v56 = textBuffer1->getStringWidth(false);
					graphics->drawRegion(app->hud->imgAttArrow, 0, 0, 12, 12, v56 + v81 + 5, v80, 0, 3, 0);
				}
			}
		}
		app->setFontRenderMode(0); // [GEC]
		numItems = this->numItems;
	LABEL_155:
		++items;
		v33 = v35;
		++v79;
	}
	if (this->drawLogo) {
		graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
		graphics->drawImage(this->imgLogo, (Applet::IOS_WIDTH - this->imgLogo->width) >> 1, 0, 0, 0, 0);
	}
	if (this->drawHelpText)
	{
		imgGameMenuTornPage = this->imgGameMenuTornPage;
		graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
		graphics->FMGL_fillRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, 0.0, 0.0, 0.5);
		v60 = this->menu;
		v61 = 230 - (imgGameMenuTornPage->width >> 1);
		if (v60 <= Menus::MENU_ITEMS_HOLY_WATER_MAX)
		{
			v82 = 10;
		}
		else
		{
			v59 = 0.0;
			vendingMachine = app->vendingMachine.get();
		}
		if (v60 >= Menus::MENU_VENDING_MACHINE)
		{
			imgGameMenuTornPage = vendingMachine->imgVendingBG;;
			v61 = v59;
			v82 = (v59);
		}
		graphics->drawImage(imgGameMenuTornPage, v61, v82, 0, 0, 0);
		v62 = app->localization->getLargeBuffer();
		v63 = app->canvas.get();
		v64 = v62;
		menuHelpMaxChars = v63->menuHelpMaxChars;
		v63->menuHelpMaxChars = this->imgGameMenuTornPage->width / Applet::FONT_WIDTH[app->fontType];
		this->items[this->selectedHelpIndex].WrapHelpText(app, v62);
		app->canvas->menuHelpMaxChars = menuHelpMaxChars;
		graphics->drawString(
			v64,
			v61 + (imgGameMenuTornPage->width >> 1),
			v82 + (this->imgGameMenuTornPage->height >> 1),
			3);
		v64->dispose();
	}



	if (this->activeSlider == SliderMode::Binding) {
		graphics->FMGL_fillRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0, 0, 0, 0.75f);
		textBuffer1->setLength(0);
		textBuffer1->append("Press New Key For");
		graphics->drawString(textBuffer1, canvas->SCR_CX, canvas->SCR_CY, Graphics::ANCHORS_CENTER);
		textBuffer1->setLength(0);
		app->localization->composeTextField(this->items[this->selectedIndex].textField, textBuffer1);
		textBuffer1->deleteAt(textBuffer1->length()-1, 1);
		graphics->drawString(textBuffer1, canvas->SCR_CX, canvas->SCR_CY + Applet::FONT_HEIGHT[app->fontType], Graphics::ANCHORS_CENTER);
	}


	textBuffer1->dispose();
	textBuffer2->dispose();
}

void MenuSystem::drawScrollbar(Graphics* graphics) {
	fmScrollButton* scrollBar;
	int knobOffset;
	int x, y;

	if(this->isMainMenuScrollBar) {
		x = this->m_scrollBar->barRect.x;
		y = this->m_scrollBar->barRect.y;
		graphics->drawImage(this->imgMenuDial, x, y, 0, 0, 0);
		scrollBar = this->m_scrollBar;
		knobOffset = scrollBar->enabled;
		if (knobOffset != 0) {
			knobOffset = scrollBar->thumbPosition + (scrollBar->thumbSize >> 1);
		}
		graphics->drawImage(this->imgMenuDialKnob, x + 12, y + ((knobOffset * 4) / 5 - (this->imgMenuDialKnob->height >> 1)) + 16, 0, 0, 0);
	}
	else {
		if (this->menu >= Menus::MENU_INGAME || this->type != 5 || this->menu == Menus::MENU_END_RANKING || this->menu == Menus::MENU_LEVEL_STATS) {
			this->m_scrollBar->Render(graphics);
		}
	}
}

void MenuSystem::drawButtonFrame(Graphics* graphics) {


	if ((this->menu != Menus::MENU_END_) &&
		((this->menu != Menus::MENU_MAIN_CONFIRMNEW && this->menu != Menus::MENU_MAIN_CONFIRMNEW2) && this->menu != Menus::MENU_END_FINALQUIT) &&
		((this->menu != Menus::MENU_MAIN_EXIT && this->menu != Menus::MENU_ENABLE_SOUNDS) && this->menu != Menus::MENU_END_) &&
		(this->menu != Menus::MENU_MAIN_MINIGAME && this->menu != Menus::MENU_MAIN_DIFFICULTY) &&
		(this->menu != Menus::MENU_MAIN_MORE_GAMES && this->menu == Menus::MENU_SELECT_LANGUAGE))
	{
		int posY = 0;
		for (int i = 0; i < this->numItems; i = i + 1) {
			if (!(this->items[i].flags & 0x8000)) {
				posY += this->getMenuItemHeight(i);
			}
		}
		graphics->drawImage(this->imgMenuLanguageBOX, app->canvas->menuRect[0], posY + app->canvas->menuRect[1] + 1, 32, 0, 0);
	}
}

void MenuSystem::drawTouchButtons(Graphics* graphics, bool b) {


	Text* textBuff;
	fmButton* button;
	bool v12;
	int buttonID;
	int height;
	int posY;

	textBuff = app->localization->getLargeBuffer();
	buttonID = 0;
	posY = 0 - this->getScrollPos();

	int maxItemsMain = 0;
	int maxItemsGame = 0;
	for (int i = 0; (i < this->numItems) && (posY < app->canvas->menuRect[3]); i++)
	{
		if (!(this->items[i].flags & 0x8000)) {
			height = this->getMenuItemHeight(i);

			if ((posY + height) > 0) { // [GEC]
				maxItemsMain++;
			}

			if ((posY + height) > 0 && this->items[i].action && !(this->items[i].flags & (Menus::ITEM_SCROLLBAR | Menus::ITEM_SCROLLBARTWO))) // [GEC]
			{
				button = this->m_menuButtons->GetButton(buttonID);
				button->selectedIndex = i;
				button->SetTouchArea(app->canvas->menuRect[0], posY + app->canvas->menuRect[1], this->menuItem_width, this->menuItem_height);

				if (this->menu >= Menus::MENU_INGAME)
				{
					v12 = buttonID == 15;
					if (buttonID != 15) {
						v12 = buttonID == 11;
					}
					if (!v12 && posY > 210) {// Old -> posY > 230
						maxItemsGame++;
						button->SetTouchArea(app->canvas->menuRect[0], 350, this->menuItem_width, this->menuItem_height, false); // Port: add param "false"
					}
				}

				if (!(this->items[i].flags & (Menus::ITEM_DISABLED | Menus::ITEM_DISABLEDTWO)))
				{
					if (b)
					{
						if (button->highlighted)
						{
							button->Render(graphics);
						}
					}
					else if (!button->highlighted)
					{
						button->Render(graphics);
					}
				}
				else {
					if (!b)
					{
						if (this->menu == Menus::MENU_MAIN_OPTIONS)
						{
							if (button->highlighted) {
								button->highlighted = false;
							}
							button->Render(graphics);
						}
					}
				}

				if (this->menu <= Menus::MENU_INGAME_STATUS ||
					this->menu == Menus::MENU_INGAME_HELP || 
					this->menu == Menus::MENU_ITEMS || 
					this->menu == Menus::MENU_ITEMS_WEAPONS || 
					this->menu == Menus::MENU_ITEMS_DRINKS ||
					this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM || 
					this->menu == Menus::MENU_VENDING_MACHINE_DRINKS || 
					this->menu == Menus::MENU_VENDING_MACHINE_SNACKS)
				{
					button = this->m_infoButtons->GetButton(buttonID);
					button->selectedIndex = i;
					button->SetTouchArea(app->canvas->menuRect[0] + this->menuItem_width + this->menuItem_paddingBottom,
						posY + app->canvas->menuRect[1], this->imgGameMenuInfoButtonPressed->width, this->imgGameMenuInfoButtonPressed->height);

					if (this->menu >= Menus::MENU_INGAME && posY > 210) { // Old -> posY > 230
						button->SetTouchArea(app->canvas->menuRect[0], 350, this->menuItem_width, this->menuItem_height, false); // Port: add param "false"
					}

					if (b)
					{
						if (button->highlighted)
						{
							button->Render(graphics);
						}
					}
					else if (!button->highlighted)
					{
						button->Render(graphics);
					}
				}
				buttonID++;
			}
			posY += height;
		}
	}


	//this->maxItems = maxItemsMain - maxItemsGame;

	//printf("maxItemsMain %d\n", maxItemsMain);
	//printf("maxItemsGame %d\n", maxItemsGame);

	if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) {
		this->m_vendingButtons->Render(graphics);
	}

	textBuff->dispose();
}

void MenuSystem::drawSoftkeyButtons(Graphics* graphics)
{

	Text* textBuff;
	fmButton* button;
	int curFontType;
	int strX, strY;

	textBuff = app->localization->getSmallBuffer();

	button = this->m_menuButtons->GetButton(11);
	if (button->drawButton)
	{
		curFontType = app->fontType;
		app->setFont(0);

		if ((this->menu >= Menus::MENU_INGAME) && (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX)) {
			button->SetTouchArea(9, 268, app->hud->imgInGameMenuSoftkey->width, app->hud->imgInGameMenuSoftkey->height);

			if (button->highlighted) {
				graphics->drawImage(app->hud->imgInGameMenuSoftkey, 9, 268, 0, 0, 0);
			}
			else {
				graphics->drawRegion(app->hud->imgInGameMenuSoftkey, 0, 0,
					app->hud->imgInGameMenuSoftkey->width, app->hud->imgInGameMenuSoftkey->height,
					9, 268, 0, 0, 2);
			}

			strX = 42;
			strY = 295;
		}
		else if (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) {
			button->SetTouchArea(10, 262, this->imgSwitchLeftActive->width, this->imgSwitchLeftActive->height);
			graphics->drawImage(button->highlighted ? this->imgSwitchLeftActive : this->imgSwitchLeftNormal, 10, 262, 0, 0, 0);

			strX = 8; // old = -2;
			strY = 314; // old = 320;
		}
		else {
			button->SetTouchArea(56, 277, app->hud->imgVendingSoftkeyNormal->width, app->hud->imgVendingSoftkeyNormal->height);

			if (button->highlighted) {
				graphics->drawImage(app->hud->imgVendingSoftkeyPressed, 56, 277, 0, 0, 0);
			}
			else {
				graphics->drawRegion(app->hud->imgVendingSoftkeyNormal, 0, 0,
					app->hud->imgVendingSoftkeyNormal->width, app->hud->imgVendingSoftkeyNormal->height,
					56, 277, 0, 0, 0);
			}

			strX = 67;
			strY = 307;
		}
		
		textBuff->setLength(0);
		app->localization->composeText(3, 80, textBuff);
		textBuff->dehyphenate();
		graphics->drawString(textBuff, strX, strY, 36);
		app->setFont(curFontType);
	}

	button = this->m_menuButtons->GetButton(15);
	if (button->drawButton)
	{
		curFontType = app->fontType;
		app->setFont(0);

		
		if ((this->menu >= Menus::MENU_INGAME) && (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX)) {
			button->SetTouchArea(372, 268, app->hud->imgInGameMenuSoftkey->width, app->hud->imgInGameMenuSoftkey->height);

			if (button->highlighted) {
				graphics->drawImage(app->hud->imgInGameMenuSoftkey, 372, 268, 0, 0, 0);
			}
			else {
				graphics->drawRegion(app->hud->imgInGameMenuSoftkey, 0, 0,
					app->hud->imgInGameMenuSoftkey->width, app->hud->imgInGameMenuSoftkey->height,
					372, 268, 0, 0, 2);
			}

			strX = 448;
			strY = 295;
		}
		else if (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) {
			button->SetTouchArea(438, 268, app->hud->imgSwitchRightActive->width, app->hud->imgSwitchRightActive->height);
			graphics->drawImage(button->highlighted ? app->hud->imgSwitchRightActive : app->hud->imgSwitchRightNormal, 438, 268, 0, 0, 0);
			strX = 478;
			strY = Applet::IOS_HEIGHT;
		}
		else {
			button->SetTouchArea(362, 277, app->hud->imgVendingSoftkeyNormal->width, app->hud->imgVendingSoftkeyNormal->height);

			if (button->highlighted) {
				graphics->drawImage(app->hud->imgVendingSoftkeyPressed, 362, 277, 0, 0, 0);
			}
			else {
				graphics->drawRegion(app->hud->imgVendingSoftkeyNormal, 0, 0,
					app->hud->imgVendingSoftkeyNormal->width, app->hud->imgVendingSoftkeyNormal->height,
					362, 277, 0, 0, 0);
			}

			strX = 411;
			strY = 307;
		}

		textBuff->setLength(0);
		textBuff->append((this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) ? "Resume" : "Exit");
		textBuff->dehyphenate();
		graphics->drawString(textBuff, strX, strY, 40);
		app->setFont(curFontType);
	}

	textBuff->dispose();
}

int MenuSystem::drawCustomScrollbar(Graphics* graphics, MenuItem* item, Text* text, int yPos) { // [GEC]


	int menuItem_width = this->menuItem_width;
	int v70 = this->menuItem_height >> 1;
	int v69 = v70 - 2;
	int v23 = app->canvas->menuRect[0];

	int value = 0;
	int valueScroll = 0;
	fmButton* button =  nullptr;
	bool disable = false;
	bool change = false;

	text->setLength(0);
	if (item->param == 1) { // SfxVolume
		value = (!app->sound->allowSounds) ? 0 : app->sound->soundFxVolume;
		valueScroll = this->sfxVolumeScroll;
		button = this->m_menuButtons->GetButton(12);
		disable = false;
		change = (app->menuSystem->activeSlider == MenuSystem::SliderMode::SfxVolume);
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}
	else if (item->param == 2) { // MusicVolume
		value = (!app->sound->allowMusics) ? 0 : app->sound->musicVolume;
		valueScroll = this->musicVolumeScroll;
		button = this->m_menuButtons->GetButton(13);
		disable = !isUserMusicOn();
		change = (app->menuSystem->activeSlider == MenuSystem::SliderMode::MusicVolume);
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}
	else if (item->param == 3) { // Alpha
		value = app->canvas->m_controlAlpha;
		valueScroll = this->alphaScroll;
		button = this->m_menuButtons->GetButton(14);
		disable = false;
		change = (app->menuSystem->activeSlider == MenuSystem::SliderMode::ButtonsAlpha);
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}
	else if (item->param == 4) { // Vibration Intensity
		value = gVibrationIntensity;
		valueScroll = this->vibrationIntensityScroll;
		button = this->m_menuButtons->GetButton(16);
		disable = false;
		change = (app->menuSystem->activeSlider == MenuSystem::SliderMode::VibrationIntensity);
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}
	else if (item->param == 5) { // Deadzone
		value = gDeadZone;
		valueScroll = this->deadzoneScroll;
		button = this->m_menuButtons->GetButton(17);
		disable = false;
		change = (app->menuSystem->activeSlider == MenuSystem::SliderMode::Deadzone);
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}

	if (item->flags & Menus::ITEM_SCROLLBAR) {
		if (disable) {
			app->setFontRenderMode(2);
		}

		graphics->drawString(text, menuItem_width + v23 - 4, yPos, 24);
		app->setFontRenderMode(0);
		int v27 = yPos + Applet::FONT_HEIGHT[app->fontType];
		if (button) {
			button->SetTouchArea(v23, v27 + 1, menuItem_width, v69);
		}
		graphics->setColor(0xFF323232);
		graphics->drawRect(v23, v27 + 1, menuItem_width - 1, v70 - 3);
		graphics->FMGL_fillRect(v23 + 1, v27 + 2, menuItem_width - 2, v70 - 4, 0.42, 0.35, 0.31, 0.7);

		int v66 = menuItem_width + v23;
		int v29 = v27 + 1;
		int v68 = v70 - 3;
		int v30 = v27 + 2;
		int v67 = v70 - 4;
		graphics->setColor(0xFF323232);
		if (button && button->highlighted) {
			int v32 = valueScroll - 12;
			if (v23 <= valueScroll - 12) {
				if (v66 < valueScroll + 12) {
					v32 = v66 - 24;
				}
			}
			else {
				v32 = v23;
			}
			graphics->drawRect(v32, v29, 23, v68);
			graphics->FMGL_fillRect(v32 + 1, v30, 22, v67, 0.9, 0.9, 0.65, 1.0);
		}
		else
		{
			int v33 = v23 + ((menuItem_width - 24) * value) / 100;
			graphics->drawRect(v33, v29, 23, v68);
			graphics->FMGL_fillRect(v33 + 1, v30, 22, v67, 0.75, 0.69, 0.65, 1.0);
		}

		if (disable) {
			graphics->FMGL_fillRect(v23, v30, menuItem_width, v69, 0.5, 0.5, 0.5, 0.3);
		}

		return 0;
	}
	else if (item->flags & Menus::ITEM_SCROLLBARTWO) {
		int v6 = ((Applet::IOS_WIDTH - this->imgMenuOptionBOX3->width) >> 1) + 1;
		int v59 = (Applet::IOS_WIDTH - this->imgMenuOptionBOX3->width) >> 1;
		int v27 = yPos + Applet::FONT_HEIGHT[app->fontType];

		int width = this->imgMenuOptionSliderON->width;
		if (button) {
			button->SetTouchArea(v6, v27 - 20, this->imgMenuOptionSliderBar->width, this->imgMenuOptionSliderBar->height);
		}
		graphics->drawImage(this->imgMenuOptionSliderBar, v6, v27 - 20, 0, 0, 0);
		int v9 = valueScroll - (width >> 1);
		int v10;
		if (v6 <= v9) {
			v10 = v59 + 246;
			if (width + v9 > v59 + 246)
				v9 = v10 - width;
		}
		else {
			v9 = v6;
			v10 = v59 + 246;
		}
		if (button && button->highlighted) {
			graphics->drawImage(this->imgMenuOptionSliderON, v9, v27, 0, 0, 0);
		}
		else {
			graphics->drawImage(this->imgMenuOptionSliderOFF, (v6 + ((245 - width) * value) / 100), v27, 0, 0, 0);
		}

		if (change) {
			v10 += 5;
			text->setLength(0); 
			text->append("<");
			graphics->drawString(text, v10, v27+5, 24);
			v10 += 5;
		}
		this->drawNumbers(graphics, v10, v27+3, 0, value);
		if (change) {
			v10 += 40;
			text->setLength(0);
			text->append(">");
			graphics->drawString(text, v10, v27+5, 24);
		}

		if (disable) {
			graphics->FMGL_fillRect(v6, v27 - 20, 244, 20, 0.2, 0.2, 0.2, 0.5);
		}
		return 0;
	}

	return 0;
}

void MenuSystem::drawOptionsScreen(Graphics* graphics)
{


	Text* LargeBuffer; // r0
	int menu; // r3
	int v6; // r5
	int width; // r6
	fmButton* Button; // r0
	int v9; // r4
	int v10; // r10
	fmButton* v11; // r0
	Sound* sound; // r2
	int v13; // r3
	fmButton* v14; // r0
	int v15; // r4
	fmButton* v16; // r0
	Sound* v17; // r0
	int v18; // r3
	int v19; // r6
	int numItems; // r2
	int v22; // r5
	int v23; // r10
	int MenuItemHeight; // r0
	Sound* v25; // r3
	int v26; // r4
	int v27; // r4
	fmButton* v28; // r0
	int v29; // r5
	int v30; // r6
	int sfxScrollPos; // r3
	int v32; // r4
	int v33; // r4
	int v34; // r5
	Sound* v35; // r3
	int v36; // r4
	int v37; // r5
	fmButton* v38; // r0
	int musicScrollPos; // r0
	int v40; // r4
	int v41; // r4
	int v42; // r6
	int v43; // r2
	int v44; // r10
	int i; // r5
	int v47; // r0
	int buttonsAlpha; // r4
	int v49; // r4
	fmButton* v50; // r0
	int v51; // r6
	int v52; // r5
	int alphaScrollPos; // r0
	int v54; // r4
	float green; // r3
	int v57; // [sp+0h] [bp-84h]
	float red; // [sp+4h] [bp-80h]
	int v59; // [sp+14h] [bp-70h]
	int v60; // [sp+1Ch] [bp-68h]
	int v61; // [sp+24h] [bp-60h]
	int v62; // [sp+28h] [bp-5Ch]
	Text* v64; // [sp+34h] [bp-50h]
	int v65; // [sp+38h] [bp-4Ch]
	int v66; // [sp+40h] [bp-44h]
	int v67; // [sp+44h] [bp-40h]
	int v68; // [sp+50h] [bp-34h]
	int v69; // [sp+60h] [bp-24h]
	int v70; // [sp+64h] [bp-20h]
	int menuItem_width; // [sp+68h] [bp-1Ch]

	LargeBuffer = app->localization->getLargeBuffer();
	menu = this->menu;
	v64 = LargeBuffer;
	if (menu == Menus::MENU_MAIN_OPTIONS)
	{
		v6 = ((Applet::IOS_WIDTH - this->imgMenuOptionBOX3->width) >> 1) + 1;
		v59 = (Applet::IOS_WIDTH - this->imgMenuOptionBOX3->width) >> 1;

		LargeBuffer->setLength(0);
		app->localization->composeText(MenuSystem::INDEX_OTHER, MenuStrings::OPTIONS_ITEM, v64);
		v64->dehyphenate();
		graphics->drawString(v64, v6 + (this->imgMenuOptionBOX3->width >> 1), 157, 3);
		width = this->imgMenuOptionSliderON->width;
		Button = this->m_menuButtons->GetButton(12);
		Button->SetTouchArea(v6, 180, this->imgMenuOptionSliderBar->width, this->imgMenuOptionSliderBar->height);
		graphics->drawImage(this->imgMenuOptionSliderBar, v6, 180, 0, 0, 0);
		v9 = this->sfxVolumeScroll - (width >> 1);
		if (v6 <= v9)
		{
			v10 = v59 + 246;
			if (width + v9 > v59 + 246)
				v9 = v10 - width;
		}
		else
		{
			v9 = v6;
			v10 = v59 + 246;
		}
		v11 = this->m_menuButtons->GetButton(12);
		if (v11->highlighted)
			graphics->drawImage(this->imgMenuOptionSliderON, v9, 200, 0, 0, 0);
		else
			graphics->drawImage(this->imgMenuOptionSliderOFF,
				(v6 + ((245 - width) * app->sound->soundFxVolume) / 100),
				200,
				0,
				0,
				0);

		v64->setLength(0);
		app->localization->composeText(MenuSystem::INDEX_OTHER, MenuStrings::SOUND_FX_VOLUME, v64);
		v64->dehyphenate();
		graphics->drawString(v64, v59 + 15, 200, 36);
		sound = app->sound.get();
		v13 = sound->soundFxVolume;
		if (!sound->allowSounds)
			v13 = 0;

		this->drawNumbers(graphics, v10, 200, 0, v13);
		v14 = m_menuButtons->GetButton(13);
		v14->SetTouchArea(v6, 247, this->imgMenuOptionSliderBar->width, this->imgMenuOptionSliderBar->height);
		graphics->drawImage(this->imgMenuOptionSliderBar, v6, 247, 0, 0, 0);
		v15 = this->musicVolumeScroll - (width >> 1);
		if (v6 <= v15)
		{
			if (v10 < width + v15)
				v15 = v10 - width;
		}
		else
		{
			v15 = v6;
		}
		v16 = this->m_menuButtons->GetButton(13);
		if (v16->highlighted)
			graphics->drawImage(this->imgMenuOptionSliderON, v15, 267, 0, 0, 0);
		else
			graphics->drawImage(this->imgMenuOptionSliderOFF,
				(v6 + ((245 - width) * app->sound->musicVolume) / 100),
				267,
				0,
				0,
				0);

		v64->setLength(0);
		app->localization->composeText(MenuSystem::INDEX_OTHER, MenuStrings::SOUND_MUSIC_VOLUME, v64);
		v64->dehyphenate();
		graphics->drawString(v64, v59 + 15, 267, 36);
		v17 = app->sound.get();
		v18 = app->sound->musicVolume;
		if (!app->sound->allowMusics) {
			v18 = 0;
		}
		this->drawNumbers(graphics, v10, 267, 0, v18);
		if (!this->isUserMusicOn()) {
			graphics->FMGL_fillRect(v6, 247, 244, 20, 0.2, 0.2, 0.2, 0.5);
		}
	}
	else if (menu == Menus::MENU_INGAME_OPTIONS)
	{
		menuItem_width = this->menuItem_width;
		v19 = 0;
		numItems = this->numItems;
		v70 = this->menuItem_height >> 1;
		v69 = v70 - 2;
		v23 = app->canvas->menuRect[0];

		for (v22 = 0; v22 < numItems; ++v22) {
			if (!(this->items[v22].flags & 0x8000)) {
				MenuItemHeight = MenuSystem::getMenuItemHeight(v22);
				numItems = this->numItems;
				v19 += MenuItemHeight;
			}
		}
		v64->setLength(0);
		v64->append("SoundFx Volume");
		v64->dehyphenate();
		graphics->drawString(v64, v23 + 4, v19 + 10, 20);
		v25 = app->sound.get();
		v26 = app->sound->soundFxVolume;
		if (!app->sound->allowSounds)
			v26 = 0;
		v64->setLength(0);
		v64->append(v26);
		v66 = menuItem_width + v23;
		graphics->drawString(v64, menuItem_width + v23 - 4, v19 + 10, 24);
		v27 = v19 + 10 + Applet::FONT_HEIGHT[app->fontType];
		v28 = this->m_menuButtons->GetButton(12);
		v29 = v27 + 1;
		v28->SetTouchArea(v23, v27 + 1, menuItem_width, v69);
		graphics->setColor(-13487566);
		v68 = v70 - 3;
		graphics->drawRect(v23, v27 + 1, menuItem_width - 1, v70 - 3);
		v30 = v27 + 2;
		v67 = v70 - 4;
		graphics->FMGL_fillRect(v23 + 1, v27 + 2, menuItem_width - 2, v70 - 4, 0.42, 0.35, 0.31, 0.7);
		graphics->setColor(-13487566);
		if (this->m_menuButtons->GetButton(12)->highlighted)
		{
			sfxScrollPos = this->sfxVolumeScroll;
			v32 = sfxScrollPos - 12;
			if (v23 <= sfxScrollPos - 12)
			{
				if (v66 < sfxScrollPos + 12)
					v32 = v66 - 24;
			}
			else
			{
				v32 = v23;
			}
			graphics->drawRect(v32, v29, 23, v68);
			graphics->FMGL_fillRect(v32 + 1, v30, 22, v67, 0.9, 0.9, 0.65, 1.0);
		}
		else
		{
			v33 = v23 + ((menuItem_width - 24) * app->sound->soundFxVolume) / 100;
			graphics->drawRect(v33, v29, 23, v68);
			graphics->FMGL_fillRect(v33 + 1, v30, 22, v67, 0.75, 0.69, 0.65, 1.0);
		}
		v34 = v29 + v70 + 8;
		v64->setLength(0);
		v64->append("Music Volume");
		v64->dehyphenate();

		if (!this->isUserMusicOn()) {
			app->setFontRenderMode(2);
		}

		graphics->drawString(v64, v23 + 4, v34, 20);
		v35 = app->sound.get();
		v36 = app->sound->musicVolume;
		if (!app->sound->allowMusics)
			v36 = 0;
		v64->setLength(0);
		v64->append(v36);
		graphics->drawString(v64, menuItem_width + v23 - 4, v34, 24);
		app->setFontRenderMode(0);

		v37 = v34 + Applet::FONT_HEIGHT[app->fontType];
		v38 = this->m_menuButtons->GetButton(13);
		v38->SetTouchArea(v23, v37, menuItem_width, v69);
		graphics->setColor(-13487566);
		graphics->drawRect(v23, v37, menuItem_width - 1, v68);
		graphics->FMGL_fillRect(v23 + 1, v37 + 1, menuItem_width - 2, v67, 0.42, 0.35, 0.31, 0.7);
		graphics->setColor(-13487566);
		if (this->m_menuButtons->GetButton(13)->highlighted)
		{
			musicScrollPos = this->musicVolumeScroll;
			v40 = musicScrollPos - 12;
			if (v23 <= musicScrollPos - 12)
			{
				if (v66 < musicScrollPos + 12)
					v40 = v66 - 24;
			}
			else
			{
				v40 = v23;
			}
			graphics->drawRect(v40, v37, 23, v68);
			graphics->FMGL_fillRect(v40 + 1, v37 + 1, 22, v67, 0.9, 0.9, 0.65, 1.0);
		}
		else
		{
			v41 = v23 + ((menuItem_width - 24) * app->sound->musicVolume) / 100;
			graphics->drawRect(v41, v37, 23, v68);
			graphics->FMGL_fillRect(v41 + 1, v37 + 1, 22, v67, 0.75, 0.69, 0.65, 1.0);
		}
		if (!isUserMusicOn()) {
			graphics->FMGL_fillRect(v23, v37, menuItem_width, v69, 0.5, 0.5, 0.5, 0.3);
		}
	}
	else
	{
		v65 = this->menuItem_width;
		v61 = this->menuItem_height >> 1;
		v42 = 0;
		v43 = this->numItems;
		v44 = app->canvas->menuRect[0];
		for (i = 0; i < v43; ++i) {
			if (!(this->items[i].flags & 0x8000)) {
				v47 = this->getMenuItemHeight(i);
				v43 = this->numItems;
				v42 += v47;
			}
		}
		v64->setLength( 0);
		v64->append("Alpha");
		v64->dehyphenate();
		graphics->drawString(v64, v44 + 4, v42 + 10, 20);
		buttonsAlpha = app->canvas->m_controlAlpha;
		v64->setLength(0);
		v64->append(buttonsAlpha);
		v60 = v65 + v44;
		graphics->drawString(v64, v65 + v44 - 4, v42 + 10, 24);
		v49 = v42 + 10 + Applet::FONT_HEIGHT[app->fontType];
		v50 = this->m_menuButtons->GetButton(14);
		v51 = v49 + 1;
		v50->SetTouchArea(v44, v49 + 1, v65, v61 - 2);
		graphics->setColor(-13487566);
		v52 = v61 - 3;
		graphics->drawRect(v44, v49 + 1, v65 - 1, v61 - 3);
		v62 = v49 + 2;
		graphics->FMGL_fillRect(v44 + 1, v49 + 2, v65 - 2, v61 - 4, 0.42, 0.35, 0.31, 0.7);
		graphics->setColor(-13487566);
		if (this->m_menuButtons->GetButton(14)->highlighted)
		{
			alphaScrollPos = this->alphaScroll;
			v54 = alphaScrollPos - 12;
			if (v44 <= alphaScrollPos - 12)
			{
				if (v60 < alphaScrollPos + 12)
					v54 = v60 - 24;
			}
			else
			{
				v54 = v44;
			}
			graphics->drawRect(v54, v51, 23, v52);
			green = 0.9f;
			v57 = v61 - 4;
			red = 0.9f;
		}
		else
		{
			v54 = v44 + (v65 - 24) * app->canvas->m_controlAlpha / 100;
			graphics->drawRect(v54, v51, 23, v52);
			red = 0.75f;
			green = 0.69f;
			v57 = v61 - 4;
		}
		graphics->FMGL_fillRect(v54 + 1, v62, 22, v57, red, green, 0.65, 1.0);
	}
	v64->dispose();
}

void MenuSystem::drawNumbers(Graphics* graphics, int x, int y, int space, int number)
{
	int num;

	if (number < 1000) {
		num = (number % 100) / 10;
		graphics->drawRegion(this->imgHudNumbers, 0, (9 - number / 100) * 20, 10, 20, x, y, 20, 0, 0);
		x += space + 10;
		graphics->drawRegion(this->imgHudNumbers, 0, (9 - num) * 20, 10, 20, x, y, 20, 0, 0);
		x += space + 10;
		graphics->drawRegion(this->imgHudNumbers, 0, ((num * 10 - number % 100) + 9) * 20, 10, 20, x, y, 20, 0, 0);
	}
	else {
		std::println(stderr, "ERROR: drawnumbers() does not currently support values over 999");
	}
}

