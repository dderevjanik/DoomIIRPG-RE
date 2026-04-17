#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include "Log.h"

#include "App.h"
#include "Image.h"
#include "CAppContainer.h"
#include "Canvas.h"
#include "Graphics.h"
#include "Game.h"
#include "Hud.h"
#include "Render.h"
#include "Combat.h"
#include "Player.h"
#include "Entity.h"
#include "EntityDef.h"
#include "MinigameUI.h"
#include "Text.h"
#include "Button.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Enums.h"
#include "ScriptThread.h"
#include "DialogManager.h"

void DialogManager::render(Canvas* canvas, Graphics* graphics) {
	if (canvas->dialogBuffer != nullptr && canvas->dialogBuffer->length() == 0) {
		return;
	}

	this->m_dialogButtons->GetButton(1)->drawButton = false;
	this->m_dialogButtons->GetButton(2)->drawButton = false;
	this->m_dialogButtons->GetButton(3)->drawButton = false;
	this->m_dialogButtons->GetButton(4)->drawButton = false;
	this->m_dialogButtons->GetButton(5)->drawButton = false;
	this->m_dialogButtons->GetButton(6)->drawButton = false;
	this->m_dialogButtons->GetButton(7)->drawButton = false;

	int* dialogRect = canvas->dialogRect;
	dialogRect[0] = -canvas->screenRect[0];
	dialogRect[2] = canvas->hudRect[2];
	dialogRect[3] = this->dialogViewLines * 16 + 8;
	dialogRect[1] = Applet::IOS_HEIGHT - dialogRect[3] - 1;
	this->dialogTypeLineIdx = this->numDialogLines;
	int n = dialogRect[0] + 1;
	int n2 = 0xFF000000;
	int color = 0xFFFFFFFF;
	int color2 = 0xFF666666;
	if (this->dialogStyle == 3) {
		// Scroll style: custom drawing
		n = -canvas->screenRect[0] + 1;
		dialogRect[1] = Applet::IOS_HEIGHT - dialogRect[3] - 10;
		graphics->fillRect(dialogRect[0], dialogRect[1] - 10, canvas->hudRect[2], canvas->dialogRect[3] + 20, 12800);
		graphics->setColor(color);
		graphics->drawRect(dialogRect[0], dialogRect[1] - 10, dialogRect[2] - 1, dialogRect[3] + 19);
	} else if (this->dialogStyle >= 0 && this->dialogStyle < canvas->dialogStyleDefCount) {
		const DialogStyleDef& style = canvas->dialogStyleDefs[this->dialogStyle];
		n2 = style.bgColor;
		if (style.altBgColor != -1 && (this->dialogFlags & 0x1) != 0) {
			n2 = style.altBgColor;
		}
		if (style.headerColor != -1) {
			color2 = style.headerColor;
		}
		if (style.yAdjust != 0) {
			dialogRect[1] += style.yAdjust;
		}
		if (style.positionTop) {
			dialogRect[1] = canvas->hudRect[1] + 20;
		}
		if (style.posTopOnFlag != 0 && (this->dialogFlags & style.posTopOnFlag) != 0) {
			dialogRect[1] = canvas->hudRect[1] + 20;
		}
	}

	if ((this->dialogFlags & 4) != 0 || (this->dialogFlags & 1) != 0) {
		this->m_dialogButtons->GetButton(3)->drawButton = true;
		this->m_dialogButtons->GetButton(4)->drawButton = true;
	}
	else
	{
		this->m_dialogButtons->GetButton(3)->drawButton = false;
		assert(m_dialogButtons->GetButton(4));
		this->m_dialogButtons->GetButton(4)->drawButton = false;
	}

	const DialogStyleDef& style = (this->dialogStyle >= 0 && this->dialogStyle < canvas->dialogStyleDefCount)
		? canvas->dialogStyleDefs[this->dialogStyle] : canvas->dialogStyleDefs[0];

	int currentDialogLine = 0;
	if (style.hasHeader) {
		// Header bar above dialog (help, terminal, special)
		currentDialogLine = 1;
		graphics->setColor(n2);
		graphics->fillRect(dialogRect[0], dialogRect[1], dialogRect[2], dialogRect[3]);
		graphics->setColor(color2);
		graphics->fillRect(dialogRect[0], dialogRect[1] - 18, dialogRect[2], 18);
		graphics->setColor(color);
		graphics->drawRect(dialogRect[0], dialogRect[1] - 18, dialogRect[2] - 1, 18);
		graphics->drawRect(dialogRect[0], dialogRect[1], dialogRect[2] - 1, dialogRect[3]);

		this->m_dialogButtons->GetButton(8)->drawButton = true;
		fmButton* Button = this->m_dialogButtons->GetButton(8);
		int fontHeight = Applet::FONT_HEIGHT[app->fontType];
		Button->SetTouchArea(*dialogRect, dialogRect[1] - fontHeight - 2, dialogRect[2], fontHeight + dialogRect[3] + 2);

		if (this->specialLootIcon != -1) {
			graphics->drawRegion(app->hud->imgActions, 0, 18 * this->specialLootIcon, 18, 18, dialogRect[0], dialogRect[1] - 17, 0, 0, 0);
			graphics->drawRegion(app->hud->imgActions, 0, 18 * this->specialLootIcon, 18, 18, dialogRect[2] - 18, dialogRect[1] - 17, 0, 0, 0);
		}
		if (style.greenTerminal) {
			canvas->graphics.currentCharColor = 2;
		}
		graphics->drawString(canvas->dialogBuffer, canvas->SCR_CX, dialogRect[1] - 16, 1, this->dialogIndexes[0], this->dialogIndexes[1]);
	}
	else if (style.hasChestHeader) {
		// Chest dialog with optional item name header
		graphics->setColor(n2);
		graphics->fillRect(dialogRect[0], dialogRect[1], dialogRect[2], dialogRect[3]);
		graphics->setColor(color);
		graphics->drawRect(dialogRect[0], dialogRect[1], dialogRect[2] - 1, dialogRect[3]);
		if (this->dialogItem != nullptr) {
			currentDialogLine = 1;
			graphics->setColor(n2);
			graphics->fillRect(dialogRect[0], dialogRect[1] - 12, dialogRect[2], 12);
			graphics->setColor(color);
			graphics->drawRect(dialogRect[0], dialogRect[1] - 12, dialogRect[2] - 1, 12);
			graphics->drawString(canvas->dialogBuffer, dialogRect[0] + (dialogRect[2] + 10 - 2 >> 1), dialogRect[1] - 5, 3, this->dialogIndexes[0], this->dialogIndexes[1]);
			this->m_dialogButtons->GetButton(8)->drawButton = true;
			fmButton* Button = this->m_dialogButtons->GetButton(8);
			Button->SetTouchArea(*dialogRect, dialogRect[1] - 12, dialogRect[2], dialogRect[3] + 12);
		}
		else {
			this->m_dialogButtons->GetButton(8)->drawButton = true;
			fmButton* Button = this->m_dialogButtons->GetButton(8);
			Button->SetTouchArea(*dialogRect, dialogRect[1], dialogRect[2], dialogRect[3]);
		}
	}
	else if (!style.customDraw) {
		// Standard dialog box
		graphics->setColor(n2);
		graphics->fillRect(dialogRect[0], dialogRect[1], dialogRect[2], dialogRect[3]);
		graphics->setColor(color);
		graphics->drawRect(dialogRect[0], dialogRect[1], dialogRect[2] - 1, dialogRect[3]);

		// Gradient fill
		if (style.hasGradient) {
			int n6;
			int n5 = n6 = dialogRect[1] + 1;
			int n7 = n6 + (dialogRect[3] - 1);
			while (++n5 < n7) {
				int n8 = 96 + ((256 - (n5 - n6 << 8) / (n7 - n6)) * 160 >> 8);
				graphics->setColor((((n2 & 0xFF00FF00) >> 8) * n8 & 0xFF00FF00) | ((n2 & 0xFF00FF) * n8 >> 8 & 0xFF00FF) & 0xDE);
				graphics->drawLine(dialogRect[0] + 1, n5, dialogRect[0] + (dialogRect[2] - 2), n5);
			}
		}

		// Player portrait
		if (style.hasPortrait) {
			int width = app->hud->imgPortraitsSM->width;
			int n9 = app->hud->imgPortraitsSM->height / 3;
			int n10 = (app->player->characterChoice == 2) ? 1 : (app->player->characterChoice == 3) ? 2 : 0;
			graphics->drawRegion(app->hud->imgPortraitsSM, 0, n9 * n10, width, n9, dialogRect[0] + 2, dialogRect[1] + 3, 0, 0, 0);
			n += app->hud->imgPortraitsSM->width + 2;
		}

		// Icon from imgUIImages
		if (style.iconSrcX >= 0) {
			int iconX, iconY, iconAnchor;
			if (style.iconBottom) {
				iconX = canvas->SCR_CX + 10;
				iconY = dialogRect[1] + dialogRect[3];
				iconAnchor = 0;
			} else if (this->dialogStyle == 5 && (this->dialogFlags & 0x2) != 0x0) {
				// Monster icon: positioned at bottom when flag set
				iconX = canvas->SCR_CX - 64;
				iconY = dialogRect[1] + dialogRect[3] + 6;
				iconAnchor = 36;
			} else {
				iconX = canvas->SCR_CX - 64;
				iconY = dialogRect[1] + 1;
				iconAnchor = 36;
			}
			graphics->drawRegion(canvas->imgUIImages, style.iconSrcX, style.iconSrcY, style.iconW, style.iconH, iconX, iconY, iconAnchor, 0, 0);
		}
	}
	if (this->currentDialogLine < currentDialogLine) {
		this->currentDialogLine = currentDialogLine;
	}
	int n11 = dialogRect[1] + 2;
	for (int n12 = 0; n12 < this->dialogViewLines && this->currentDialogLine + n12 < this->numDialogLines; ++n12) {
		short n13 = this->dialogIndexes[(this->currentDialogLine + n12) * 2];
		short n14 = this->dialogIndexes[(this->currentDialogLine + n12) * 2 + 1];
		int n15 = 0;
		if (n12 == this->dialogTypeLineIdx) {
			n15 = (app->time - this->dialogLineStartTime) / 25;
			if (n15 >= n14) {
				n15 = n14;
				++this->dialogTypeLineIdx;
				this->dialogLineStartTime = app->time;
			}
		}
		else if (n12 < this->dialogTypeLineIdx) {
			n15 = n14;
		}
		if (this->dialogStyle == 9) {
			canvas->graphics.currentCharColor = 2;
		}
		graphics->drawString(canvas->dialogBuffer, n, n11, 0, n13, n15);
		n11 += 16;
	}
	int8_t b = canvas->OSC_CYCLE[app->time / 200 % 4];
	short n16 = app->game->scriptStateVars[4];
	if ((this->dialogFlags & 0x2) != 0x0) {
		int y = canvas->screenRect[3] - 214;
		int x = canvas->screenRect[0] + 3;

		Text* smallBuffer = app->localization->getSmallBuffer();
		Text* smallBuffer2 = app->localization->getSmallBuffer();
		if ((this->dialogFlags & 0x8) != 0x0) {
			app->localization->composeText((short)0, (short)214, smallBuffer);
			app->localization->composeText((short)0, (short)215, smallBuffer2);
		}
		else {
			app->localization->composeText((short)0, (short)141, smallBuffer);
			app->localization->composeText((short)0, (short)140, smallBuffer2);
		}
		smallBuffer->dehyphenate();
		smallBuffer2->dehyphenate();

		int strX = x + 4;
		int strY = y + 9;
		int strH = 32;
		int strW = std::max(smallBuffer->getStringWidth(), smallBuffer2->getStringWidth());
		strW += 20;

		this->m_dialogButtons->GetButton(0)->drawButton = true;
		int hColor = (this->m_dialogButtons->GetButton(0)->highlighted) ? 0xFF8A8A8A : 0xFF4A4A4A;
		graphics->fillRect(x, y, strW, strH, hColor);
		graphics->drawRect(x, y, strW, strH, -1);
		graphics->drawString(smallBuffer, strX, strY, 4);
		this->m_dialogButtons->GetButton(0)->SetTouchArea(x, y, strW, (strH - 5));

		if (n16 == 0) {
			graphics->drawCursor(strX + strW + b - 8, strY, 0x18, false);
		}

		strY = y + 73;
		this->m_dialogButtons->GetButton(1)->drawButton = true;
		hColor = (this->m_dialogButtons->GetButton(1)->highlighted) ? 0xFF8A8A8A : 0xFF4A4A4A;
		graphics->fillRect(x, y + 64, strW, strH, hColor);
		graphics->drawRect(x, y + 64, strW, strH, -1);
		graphics->drawString(smallBuffer2, strX, strY, 4);
		this->m_dialogButtons->GetButton(1)->SetTouchArea(x, y + 64, strW, (strH - 5));

		if (n16 == 1) {
			graphics->drawCursor(strX + strW + b - 8, strY, 0x18, false);
		}
		smallBuffer->dispose();
		smallBuffer2->dispose();
	}
	else if (this->dialogFlags != 0 && this->currentDialogLine >= this->numDialogLines - this->dialogViewLines) {
		short n19 = 30;
		short n20 = 31;
		Text* smallBuffer3 = app->localization->getSmallBuffer();
		if ((this->dialogFlags & 0x1) != 0x0) {
			n19 = 140;
			n20 = 141;
		}
		if ((this->dialogFlags & 0x4) != 0x0 || (this->dialogFlags & 0x1) != 0x0) {
			int n21 = canvas->dialogRect[1] + (16 * this->dialogViewLines) - 14;

			int rowHeight = Applet::FONT_HEIGHT[app->fontType] + 2;
			int fontHeight = Applet::FONT_HEIGHT[app->fontType];

			smallBuffer3->setLength(0);
			app->localization->composeText((short)0, n19, smallBuffer3);
			smallBuffer3->dehyphenate();
			this->m_dialogButtons->GetButton(3)->drawButton = true;
			int hColor = (this->m_dialogButtons->GetButton(3)->highlighted) ? ((n2 + 0x333333) | 0xFF000000) : n2;
			graphics->fillRect(96, n21, 96, rowHeight, hColor);
			graphics->drawRect(96, n21, 96, rowHeight, -1);
			graphics->drawString(smallBuffer3, 144, n21 + (rowHeight >> 1) + 2, 3);
			this->m_dialogButtons->GetButton(3)->SetTouchArea(96, n21 - 40, 96, fontHeight + 42);

			if (n16 == 0) {
				graphics->drawCursor((128 - (smallBuffer3->getStringWidth() >> 1)) + b, n21 + (fontHeight >> 1) - 5, 0);
			}

			smallBuffer3->setLength(0);
			app->localization->composeText((short)0, n20, smallBuffer3);
			smallBuffer3->dehyphenate();
			this->m_dialogButtons->GetButton(4)->drawButton = true;
			hColor = (this->m_dialogButtons->GetButton(4)->highlighted) ? ((n2 + 0x333333) | 0xFF000000) : n2;
			graphics->fillRect(288, n21, 96, rowHeight, hColor);
			graphics->drawRect(288, n21, 96, rowHeight, -1);
			graphics->drawString(smallBuffer3, 336, n21 + (rowHeight >> 1) + 2, 3);
			this->m_dialogButtons->GetButton(4)->SetTouchArea(288, n21 - 40, 96, fontHeight + 42);

			if (n16 == 1) {
				graphics->drawCursor((320 - (smallBuffer3->getStringWidth() >> 1)) + b, n21 + (fontHeight >> 1) - 5, 4);
			}
		}
		smallBuffer3->dispose();
	}

	if (this->numDialogLines <= this->dialogViewLines) {
		if (!(this->dialogFlags & 2) && !(this->dialogFlags & 4) && !(this->dialogFlags & 1)) {
			this->m_dialogButtons->GetButton(7)->drawButton = true;
			this->m_dialogButtons->GetButton(7)->Render(graphics);
		}
	}
	else {
		int numDialogLines;
		if (this->currentDialogLine + this->dialogViewLines > this->numDialogLines) {
			numDialogLines = this->numDialogLines;
		}
		else {
			numDialogLines = this->currentDialogLine + this->dialogViewLines;
		}

		canvas->drawScrollBar(graphics, dialogRect[0] + dialogRect[2] - 1, dialogRect[1] + 2, dialogRect[3] - 4, this->currentDialogLine - currentDialogLine, numDialogLines - currentDialogLine, this->numDialogLines - currentDialogLine, this->dialogViewLines);

		if (this->numDialogLines - currentDialogLine > this->dialogViewLines) {
			if (this->currentDialogLine > 1) {
				this->m_dialogButtons->GetButton(5)->drawButton = true;
				this->m_dialogButtons->GetButton(5)->Render(graphics);
			}
			if (this->currentDialogLine < this->numDialogLines - this->dialogViewLines) {
				this->m_dialogButtons->GetButton(6)->drawButton = true;
				this->m_dialogButtons->GetButton(6)->Render(graphics);
			}
			else {
				this->m_dialogButtons->GetButton(7)->drawButton = true;
				this->m_dialogButtons->GetButton(7)->Render(graphics);
			}
		}
		else {
			this->m_dialogButtons->GetButton(7)->drawButton = true;
			this->m_dialogButtons->GetButton(7)->Render(graphics);
		}
	}

	this->m_dialogButtons->GetButton(8)->drawButton = true;
	this->m_dialogButtons->GetButton(8)->Render(graphics);
	canvas->clearLeftSoftKey();
}


void DialogManager::close(Canvas* canvas, bool skipDialog) {
	this->dialogClosing = true;
	this->specialLootIcon = -1;
	this->showingLoot = false;
	app->player->unpause(app->time - this->dialogStartTime);
	canvas->dialogBuffer->dispose();
	canvas->dialogBuffer = nullptr;
	if (this->numHelpMessages == 0 && (this->dialogStyle == 3 || this->dialogStyle == 4 || (this->dialogStyle == 2 && this->dialogType == 1) || (this->dialogStyle == 12 && app->game->scriptStateVars[4] == 1 && !skipDialog))) {
		app->game->queueAdvanceTurn = true;
	}
	if (this->dialogStyle == 11 && (this->dialogFlags & 0x2) != 0x0) {
		if (app->game->scriptStateVars[4] == 0) {
			++app->game->numMallocsForVIOS;
		}
		else {
			app->game->angryVIOS = true;
		}
	}
	if (canvas->oldState == Canvas::ST_INTER_CAMERA) {
		app->game->activeCameraTime = app->gameTime - app->game->activeCameraTime;
		canvas->setState(Canvas::ST_INTER_CAMERA);
	}
	else if (app->game->isCameraActive()) {
		app->game->activeCameraTime = app->gameTime - app->game->activeCameraTime;
		canvas->setState(Canvas::ST_CAMERA);
	}
	else if (canvas->oldState == Canvas::ST_COMBAT && !canvas->combatDone) {
		canvas->setState(Canvas::ST_COMBAT);
	}
	else {
		canvas->setState(Canvas::ST_PLAYING);
	}
	if (this->dialogResumeMenu) {
		canvas->setState(Canvas::ST_MENU);
	}
	this->dialogClosing = false;
	if (this->dialogResumeScriptAfterClosed) {
		app->game->skipDialog = skipDialog;
		this->dialogThread->run();
		app->game->skipDialog = false;
	}
	if (this->dialogStyle == 12 && app->player->attemptingToSelfDestructFamiliar) {
		app->player->attemptingToSelfDestructFamiliar = false;
		if (app->game->scriptStateVars[4] == 1 && !skipDialog) {
			app->player->familiarDying(true);
		}
	}
	else if (this->dialogStyle == 13 && app->minigameUI->repairingArmor) {
		if (app->game->scriptStateVars[4] == 1 && !skipDialog) {
			if (app->player->inventory[12] >= 5) {
				app->player->give(0, 12, -5);
				app->player->give(0, 11, 1);
				app->sound->playSound(Sounds::getResIDByName(SoundName::ITEM_PICKUP), 0, 3, 0);
				int modifyStat = app->player->modifyStat(3, 1);
				if (modifyStat > 0) {
					app->localization->resetTextArgs();
					app->localization->addTextArg(modifyStat);
					app->hud->addMessage((short)0, (short)212, 3);
				}
				else {
					app->hud->addMessage((short)0, (short)213, 3);
				}
			}
			else {
				if (app->player->characterChoice == 1) {
					app->sound->playSound(Sounds::getResIDByName(SoundName::NO_USE2), 0, 3, 0);
				}
				else if (app->player->characterChoice >= 1 && app->player->characterChoice <= 3) {
					app->sound->playSound(Sounds::getResIDByName(SoundName::NO_USE), 0, 3, 0);
				}
				app->hud->addMessage((short)0, (short)210, 3);
			}
		}
		app->minigameUI->endArmorRepair(canvas);
	}
	canvas->repaintFlags |= Canvas::REPAINT_VIEW3D;
}


void DialogManager::prepare(Canvas* canvas, Text* text, int dialogStyle, int dialogFlags) {
	int i = 0;
	int n = 0;
	Text* smallBuffer = app->localization->getSmallBuffer();
	if (dialogStyle == 3) {
		this->dialogViewLines = 4;
	}
	else if (dialogStyle == 8) {
		this->dialogViewLines = 3;
	}
	else if (dialogStyle == 2) {
		this->dialogViewLines = 3;
	}
	else {
		this->dialogViewLines = 4;
	}
	if (dialogStyle == 1 || (dialogStyle == 5 && ((dialogFlags & 0x2) != 0x0 || (dialogFlags & 0x4) != 0x0))) {
		canvas->updateFacingEntity = true;
		Entity* facingEntity = app->player->facingEntity;
		if (facingEntity != nullptr && facingEntity->def != nullptr && (facingEntity->def->eType == Enums::ET_MONSTER || facingEntity->def->eType == Enums::ET_NPC)) {
			app->combat->curTarget = facingEntity;
			int sprite = facingEntity->getSprite();
			if (facingEntity->def->eType == Enums::ET_MONSTER) {
				app->render->setSpriteFrame(sprite, 96);
			}
			app->game->scriptStateVars[4] = 0;
		}
	}
	if (canvas->dialogBuffer == nullptr) {
		canvas->dialogBuffer = app->localization->getLargeBuffer();
	}
	else {
		canvas->dialogBuffer->setLength(0);
	}
	if ((dialogFlags & 0x4) != 0x0 || (dialogFlags & 0x1) != 0x0) {
		if (dialogStyle == 12) {
			app->game->scriptStateVars[4] = 0;
		}
		else {
			app->game->scriptStateVars[4] = 1;
		}
		smallBuffer->setLength(0);
		app->localization->composeText((short)0, (short)50, smallBuffer);
		text->append(smallBuffer);
	}
	if (dialogStyle == 8) {
		int n2 = canvas->dialogMaxChars;
		smallBuffer->setLength(0);
		smallBuffer->append("   ");
		int n3 = smallBuffer->length();
		for (int j = 0; j < 2; ++j) {
			Text* dialogBuffer = canvas->dialogBuffer;
			int length;
			for (int k = 0; k < text->length(); k += dialogBuffer->wrapText(length, n2 - n3, 1, '|')) {
				length = dialogBuffer->length();
				dialogBuffer->append(text, k);
			}
			if (j == 0) {
				if (dialogBuffer->getNumLines() <= 3) {
					break;
				}
				dialogBuffer->setLength(0);
				n2 = canvas->dialogWithBarMaxChars;
			}
		}
	}
	else if (dialogStyle == 3) {
		canvas->dialogBuffer->append(text);
		canvas->dialogBuffer->wrapText(canvas->scrollMaxChars);
		if (canvas->dialogBuffer->getNumLines() > this->dialogViewLines) {
			canvas->dialogBuffer->setLength(0);
			canvas->dialogBuffer->append(text);
			canvas->dialogBuffer->wrapText(canvas->scrollWithBarMaxChars);
		}
	}
	else {
		canvas->dialogBuffer->append(text);
		canvas->dialogBuffer->wrapText(canvas->dialogMaxChars);
		int numLines = canvas->dialogBuffer->getNumLines();
		if (dialogStyle == 2 || dialogStyle == 16 || dialogStyle == 9) {
			--numLines;
		}
		if (numLines > this->dialogViewLines) {
			canvas->dialogBuffer->setLength(0);
			canvas->dialogBuffer->append(text);
			canvas->dialogBuffer->wrapText(canvas->dialogWithBarMaxChars);
		}
	}
	int length2 = canvas->dialogBuffer->length();
	this->numDialogLines = 0;
	while (i < length2) {
		if (canvas->dialogBuffer->charAt(i) == '|') {
			this->dialogIndexes[this->numDialogLines * 2] = (short)n;
			this->dialogIndexes[this->numDialogLines * 2 + 1] = (short)(i - n);
			this->numDialogLines++;
			n = i + 1;
		}
		++i;
	}
	this->dialogIndexes[this->numDialogLines * 2] = (short)n;
	this->dialogIndexes[this->numDialogLines * 2 + 1] = (short)(length2 - n);
	this->numDialogLines++;
	this->currentDialogLine = 0;
	this->dialogLineStartTime = app->time;
	this->dialogTypeLineIdx = 0;
	this->dialogStartTime = app->time;
	this->dialogItem = nullptr;
	this->dialogFlags = dialogFlags;
	this->dialogStyle = dialogStyle;
	smallBuffer->dispose();

	if (dialogStyle == 2) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::DIALOG_HELP), 0, 3, 0);
	}
}


void DialogManager::startDialog(Canvas* canvas, ScriptThread* scriptThread, short n, int n2, int n3) {
	this->startDialog(canvas, scriptThread, (short)0, n, n2, n3, false);
}


void DialogManager::startDialog(Canvas* canvas, ScriptThread* scriptThread, short n, short n2, int n3, int n4, bool b) {
	Text* largeBuffer = app->localization->getLargeBuffer();
	app->localization->composeText(n, n2, largeBuffer);
	this->startDialog(canvas, scriptThread, largeBuffer, n3, n4, b);
	largeBuffer->dispose();
}


void DialogManager::startDialog(Canvas* canvas, ScriptThread* scriptThread, Text* text, int n, int n2) {
	this->startDialog(canvas, scriptThread, text, n, n2, false);
}


void DialogManager::startDialog(Canvas* canvas, ScriptThread* dialogThread, Text* text, int n, int n2, bool dialogResumeScriptAfterClosed) {
	this->dialogResumeScriptAfterClosed = dialogResumeScriptAfterClosed;
	this->dialogResumeMenu = false;
	this->dialogThread = dialogThread;
	canvas->readyWeaponSound = 0;
	this->prepare(canvas, text, n, n2);
	canvas->setState(Canvas::ST_DIALOG);
}


void DialogManager::dequeueHelpDialog(Canvas* canvas) {
	this->dequeueHelpDialog(canvas, false);
}


void DialogManager::dequeueHelpDialog(Canvas* canvas, bool b) {
	if (this->numHelpMessages == 0) {
		return;
	}
	if (canvas->state == Canvas::ST_DIALOG || this->dialogClosing) {
		return;
	}
	if (!b && canvas->state != Canvas::ST_PLAYING && canvas->state != Canvas::ST_INTER_CAMERA && app->game->monstersTurn == 0) {
		return;
	}
	if (app->game->secretActive) {
		return;
	}

	int n = 2;
	int n2 = 0;
	Text* largeBuffer = app->localization->getLargeBuffer();
	this->dialogType = this->helpMessageTypes[0];
	void* object = this->helpMessageObjs[0];
	short n3 = this->helpMessageThreads[0];
	if (this->dialogType == 1) {
		EntityDef* entityDef = (EntityDef*)object;
		uint8_t eSubType = entityDef->eSubType;
		short n4 = -1;
		if (eSubType == Enums::ITEM_CONSUMABLE) {
			if (entityDef->parm >= 0 && entityDef->parm < 11) {
				n4 = 32;
			}
			else if ((entityDef->parm >= 16 && entityDef->parm < 18) || (entityDef->parm >= 11 && entityDef->parm < 13)) {
				n4 = 35;
			}
			else if (entityDef->parm == 18) {
				n4 = 34;
			}
		}
		else if (eSubType == Enums::ITEM_WEAPON) {
			n4 = 36;
		}
		else if (eSubType != Enums::ITEM_AMMO) {
			app->Error(0);
			return;
		}
		if (entityDef != nullptr) {
			app->localization->composeText((short)1, entityDef->longName, largeBuffer);
			largeBuffer->append("|");
			app->localization->composeText((short)1, entityDef->description, largeBuffer);
			if (n4 != -1) {
				largeBuffer->append(" ");
				app->localization->composeText((short)0, n4, largeBuffer);
			}
		}
	}
	else if (this->dialogType == 2) {
		int n5 = this->helpMessageInts[0];
		app->localization->composeText((short)(n5 >> 16), (short)(n5 & 0xFFFF), largeBuffer);
	}
	else if (this->dialogType == 3) {
		largeBuffer->dispose();
		largeBuffer = (Text*)object;
	}
	else {
		largeBuffer->dispose();
		largeBuffer = (Text*)object;
	}
	for (int i = 0; i < 15; ++i) {
		this->helpMessageTypes[i] = this->helpMessageTypes[i + 1];
		this->helpMessageInts[i] = this->helpMessageInts[i + 1];
		this->helpMessageObjs[i] = this->helpMessageObjs[i + 1];
		this->helpMessageThreads[i] = this->helpMessageThreads[i + 1];
	}
	this->helpMessageObjs[15] = object;
	this->numHelpMessages--;
	if (app->player->enableHelp) {
		if (n3 == -1) {
			this->startDialog(canvas, nullptr, largeBuffer, n, n2, false);
		}
		else {
			this->startDialog(canvas, &app->game->scriptThreads[n3], largeBuffer, n, n2, true);
		}
	}
	largeBuffer->dispose();
}


void DialogManager::enqueueHelpDialog(Canvas* canvas, short n) {
	this->enqueueHelpDialog(canvas, (short)0, n, (uint8_t)(-1));
}


bool DialogManager::enqueueHelpDialog(Canvas* canvas, short n, short n2, uint8_t b) {
	if (!app->player->enableHelp || canvas->state == Canvas::ST_DYING) {
		return false;
	}
	if (this->numHelpMessages == 16) {
		app->Error(41);
		return false;
	}
	this->helpMessageTypes[this->numHelpMessages] = 2;
	this->helpMessageInts[this->numHelpMessages] = (n << 16 | n2);
	this->helpMessageObjs[this->numHelpMessages] = nullptr;
	this->helpMessageThreads[this->numHelpMessages] = b;
	this->numHelpMessages++;
	if (canvas->state == Canvas::ST_PLAYING) {
		this->dequeueHelpDialog(canvas);
	}
	return true;
}


bool DialogManager::enqueueHelpDialog(Canvas* canvas, Text* text) {
	return this->enqueueHelpDialog(canvas, text, 0);
}


bool DialogManager::enqueueHelpDialog(Canvas* canvas, Text* text, int n) {
	if (!app->player->enableHelp || canvas->state == Canvas::ST_DYING) {
		return false;
	}
	if (this->numHelpMessages == 16) {
		app->Error(41);
		return false;
	}
	this->helpMessageTypes[this->numHelpMessages] = n;
	this->helpMessageObjs[this->numHelpMessages] = text;
	this->helpMessageThreads[this->numHelpMessages] = -1;
	this->numHelpMessages++;
	if (canvas->state == Canvas::ST_PLAYING) {
		this->dequeueHelpDialog(canvas);
	}
	return true;
}


void DialogManager::enqueueHelpDialog(Canvas* canvas, EntityDef* entityDef) {
	if (!app->player->enableHelp || canvas->state == Canvas::ST_DYING) {
		return;
	}
	if (this->numHelpMessages == 16) {
		app->Error(41);
		return;
	}
	this->helpMessageTypes[this->numHelpMessages] = 1;
	this->helpMessageObjs[this->numHelpMessages] = entityDef;
	this->helpMessageThreads[this->numHelpMessages] = -1;
	this->numHelpMessages++;
	if (canvas->state == Canvas::ST_PLAYING) {
		this->dequeueHelpDialog(canvas);
	}
}
