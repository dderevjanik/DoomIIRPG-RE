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

void Canvas::dialogState(Graphics* graphics) {


	if (this->dialogBuffer != nullptr && this->dialogBuffer->length() == 0) {
		return;
	}

	this->m_dialogButtons->GetButton(1)->drawButton = false;
	this->m_dialogButtons->GetButton(2)->drawButton = false;
	this->m_dialogButtons->GetButton(3)->drawButton = false;
	this->m_dialogButtons->GetButton(4)->drawButton = false;
	this->m_dialogButtons->GetButton(5)->drawButton = false;
	this->m_dialogButtons->GetButton(6)->drawButton = false;
	this->m_dialogButtons->GetButton(7)->drawButton = false;

	int* dialogRect = this->dialogRect;
	dialogRect[0] = -this->screenRect[0];
	dialogRect[2] = this->hudRect[2];
	dialogRect[3] = this->dialogViewLines * 16 + 8;
	dialogRect[1] = Applet::IOS_HEIGHT - dialogRect[3] - 1;//Canvas.screenRect[3] - dialogRect[3] - 1;
	this->dialogTypeLineIdx = this->numDialogLines;
	int n = dialogRect[0] + 1;
	int n2 = 0xFF000000;
	int color = 0xFFFFFFFF;
	int color2 = 0xFF666666;
	if (this->dialogStyle == 3) {
		// Scroll style: custom drawing
		n = -this->screenRect[0] + 1;
		dialogRect[1] = Applet::IOS_HEIGHT - dialogRect[3] - 10;
		graphics->fillRect(dialogRect[0], dialogRect[1] - 10, this->hudRect[2], this->dialogRect[3] + 20, 12800);
		graphics->setColor(color);
		graphics->drawRect(dialogRect[0], dialogRect[1] - 10, dialogRect[2] - 1, dialogRect[3] + 19);
	} else if (this->dialogStyle >= 0 && this->dialogStyle < this->dialogStyleDefCount) {
		const DialogStyleDef& style = this->dialogStyleDefs[this->dialogStyle];
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
			dialogRect[1] = this->hudRect[1] + 20;
		}
		if (style.posTopOnFlag != 0 && (this->dialogFlags & style.posTopOnFlag) != 0) {
			dialogRect[1] = this->hudRect[1] + 20;
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
		//__symbol_stub4::___assert_rtn("dialogState","/Users/greghodges/doom2rpg/trunk/Doom2rpg_iphone/xcode/Classes/Canvas.cpp", 0x1499,"m_dialogButtons->GetButton(4)");
		
		this->m_dialogButtons->GetButton(4)->drawButton = false;
	}

	const DialogStyleDef& style = (this->dialogStyle >= 0 && this->dialogStyle < this->dialogStyleDefCount)
		? this->dialogStyleDefs[this->dialogStyle] : this->dialogStyleDefs[0];

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
			this->graphics.currentCharColor = 2;
		}
		graphics->drawString(this->dialogBuffer, this->SCR_CX, dialogRect[1] - 16, 1, this->dialogIndexes[0], this->dialogIndexes[1]);
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
			graphics->drawString(this->dialogBuffer, dialogRect[0] + (dialogRect[2] + 10 - 2 >> 1), dialogRect[1] - 5, 3, this->dialogIndexes[0], this->dialogIndexes[1]);
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
				iconX = this->SCR_CX + 10;
				iconY = dialogRect[1] + dialogRect[3];
				iconAnchor = 0;
			} else if (this->dialogStyle == 5 && (this->dialogFlags & 0x2) != 0x0) {
				// Monster icon: positioned at bottom when flag set
				iconX = this->SCR_CX - 64;
				iconY = dialogRect[1] + dialogRect[3] + 6;
				iconAnchor = 36;
			} else {
				iconX = this->SCR_CX - 64;
				iconY = dialogRect[1] + 1;
				iconAnchor = 36;
			}
			graphics->drawRegion(this->imgUIImages, style.iconSrcX, style.iconSrcY, style.iconW, style.iconH, iconX, iconY, iconAnchor, 0, 0);
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
			this->graphics.currentCharColor = 2;
		}
		graphics->drawString(this->dialogBuffer, n, n11, 0, n13, n15);
		n11 += 16;
	}
	int8_t b = this->OSC_CYCLE[app->time / 200 % 4];
	short n16 = app->game->scriptStateVars[4];
	if ((this->dialogFlags & 0x2) != 0x0) {
		int y = this->screenRect[3] - 214;
		int x = this->screenRect[0] + 3;

		Text* smallBuffer = app->localization->getSmallBuffer();
		Text* smallBuffer2 = app->localization->getSmallBuffer();
		if ((this->dialogFlags & 0x8) != 0x0) {
			app->localization->composeText((short)0, (short)214, smallBuffer); // VIOS_MALLOC
			app->localization->composeText((short)0, (short)215, smallBuffer2); // VIOS_DELETE
		}
		else {
			app->localization->composeText((short)0, (short)141, smallBuffer); // YES_LABEL
			app->localization->composeText((short)0, (short)140, smallBuffer2); // NO_LABEL
		}
		smallBuffer->dehyphenate();
		smallBuffer2->dehyphenate();

		int strX = x + 4; // Old x + 8
		int strY = y + 9; // Old y + 1
		int strH = 32;
		int strW = std::max(smallBuffer->getStringWidth(), smallBuffer2->getStringWidth());
		strW += 20;// strW += 18

		//------------------------------------------------------------------------
		this->m_dialogButtons->GetButton(0)->drawButton = true;
		int hColor = (this->m_dialogButtons->GetButton(0)->highlighted) ? 0xFF8A8A8A : 0xFF4A4A4A;
		graphics->fillRect(x, y, strW, strH, hColor);
		graphics->drawRect(x, y, strW, strH, -1);
		graphics->drawString(smallBuffer, strX, strY, 4);
		this->m_dialogButtons->GetButton(0)->SetTouchArea(x, y, strW, (strH - 5));

		if (n16 == 0) { // J2ME/BREW
			graphics->drawCursor(strX + strW + b - 8, strY, 0x18, false);
		}

		//------------------------------------------------------------------------
		strY = y + 73; // Old y + 71
		this->m_dialogButtons->GetButton(1)->drawButton = true;
		hColor = (this->m_dialogButtons->GetButton(1)->highlighted) ? 0xFF8A8A8A : 0xFF4A4A4A;
		graphics->fillRect(x, y + 64, strW, strH, hColor);
		graphics->drawRect(x, y + 64, strW, strH, -1);
		graphics->drawString(smallBuffer2, strX, strY, 4);
		this->m_dialogButtons->GetButton(1)->SetTouchArea(x, y + 64, strW, (strH - 5));

		if (n16 == 1) {// J2ME/BREW
			graphics->drawCursor(strX + strW + b - 8, strY, 0x18, false);
		}
		//------------------------------------------------------------------------
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
			int n21 = this->dialogRect[1] + (16 * this->dialogViewLines) - 14;

			int v75 = Applet::FONT_HEIGHT[app->fontType] + 2;
			int v113 = Applet::FONT_HEIGHT[app->fontType];

			//------------------------------------------------------------------------
			smallBuffer3->setLength(0);
			app->localization->composeText((short)0, n19, smallBuffer3);
			smallBuffer3->dehyphenate();
			this->m_dialogButtons->GetButton(3)->drawButton = true;
			int hColor = (this->m_dialogButtons->GetButton(3)->highlighted) ? ((n2 + 0x333333) | 0xFF000000) : n2;
			graphics->fillRect(96, n21, 96, v75, hColor);
			graphics->drawRect(96, n21, 96, v75, -1);
			graphics->drawString(smallBuffer3, 144, n21 + (v75 >> 1) + 2, 3);
			this->m_dialogButtons->GetButton(3)->SetTouchArea(96, n21 - 40, 96, v113 + 42);

			if (n16 == 0) { // J2ME/BREW
				graphics->drawCursor((128 - (smallBuffer3->getStringWidth() >> 1)) + b, n21 + (v113 >> 1) - 5, 0);
			}
			//------------------------------------------------------------------------
			smallBuffer3->setLength(0);
			app->localization->composeText((short)0, n20, smallBuffer3);
			smallBuffer3->dehyphenate();
			this->m_dialogButtons->GetButton(4)->drawButton = true;
			hColor = (this->m_dialogButtons->GetButton(4)->highlighted) ? ((n2 + 0x333333) | 0xFF000000) : n2;
			graphics->fillRect(288, n21, 96, v75, hColor);
			graphics->drawRect(288, n21, 96, v75, -1);
			graphics->drawString(smallBuffer3, 336, n21 + (v75 >> 1) + 2, 3);
			this->m_dialogButtons->GetButton(4)->SetTouchArea(288, n21 - 40, 96, v113 + 42);

			if (n16 == 1) { // J2ME/BREW
				graphics->drawCursor((320 - (smallBuffer3->getStringWidth() >> 1)) + b, n21 + (v113 >> 1) - 5, 4);
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

		this->drawScrollBar(graphics, dialogRect[0] + dialogRect[2] - 1, dialogRect[1] + 2, dialogRect[3] - 4, this->currentDialogLine - currentDialogLine, numDialogLines - currentDialogLine, this->numDialogLines - currentDialogLine, this->dialogViewLines);

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
	this->clearLeftSoftKey();

#if 0
	if (this->dialogFlags > this->dialogViewLines) {
		int numDialogLines;
		if (this->currentDialogLine + this->dialogViewLines > this->numDialogLines) {
			numDialogLines = this->numDialogLines;
		}
		else {
			numDialogLines = this->currentDialogLine + this->dialogViewLines;
		}
		if (this->dialogStyle == 3) {
			this->drawScrollBar(graphics, dialogRect[0] + dialogRect[2] - 1, dialogRect[1] - 8, dialogRect[3] + 16, this->currentDialogLine - currentDialogLine, numDialogLines - currentDialogLine, this->numDialogLines - currentDialogLine, this->dialogViewLines);
		}
		else {
			this->drawScrollBar(graphics, dialogRect[0] + dialogRect[2] - 1, dialogRect[1] + 2, dialogRect[3] - 4, this->currentDialogLine - currentDialogLine, numDialogLines - currentDialogLine, this->numDialogLines - currentDialogLine, this->dialogViewLines);
		}
	}
	if (this->currentDialogLine > currentDialogLine) {
		this->setLeftSoftKey((short)0, (short)125);
	}
	else {
		//this->clearLeftSoftKey();
	}
#endif
}


void Canvas::closeDialog(bool skipDialog) {


	this->dialogClosing = true;
	this->specialLootIcon = -1;
	this->showingLoot = false;
	app->player->unpause(app->time - this->dialogStartTime);
	this->dialogBuffer->dispose();
	this->dialogBuffer = nullptr;
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
	if (this->oldState == Canvas::ST_INTER_CAMERA) {
		app->game->activeCameraTime = app->gameTime - app->game->activeCameraTime;
		this->setState(Canvas::ST_INTER_CAMERA);
	}
	else if (app->game->isCameraActive()) {
		app->game->activeCameraTime = app->gameTime - app->game->activeCameraTime;
		this->setState(Canvas::ST_CAMERA);
	}
	else if (this->oldState == Canvas::ST_COMBAT && !this->combatDone) {
		this->setState(Canvas::ST_COMBAT);
	}
	else {
		this->setState(Canvas::ST_PLAYING);
	}
	if (this->dialogResumeMenu) {
		this->setState(Canvas::ST_MENU);
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
	else if (this->dialogStyle == 13 && this->repairingArmor) {
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
		this->endArmorRepair();
	}
	this->repaintFlags |= Canvas::REPAINT_VIEW3D;
}


void Canvas::prepareDialog(Text* text, int dialogStyle, int dialogFlags) {

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
		this->updateFacingEntity = true;
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
	if (this->dialogBuffer == nullptr) {
		this->dialogBuffer = app->localization->getLargeBuffer();
	}
	else {
		this->dialogBuffer->setLength(0);
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
		int n2 = this->dialogMaxChars;
		//int n3 = Hud.imgPortraitsSM.getWidth() / 9 + 1;
		smallBuffer->setLength(0);
		smallBuffer->append("   ");
		int n3 = smallBuffer->length();
		for (int j = 0; j < 2; ++j) {
			Text* dialogBuffer = this->dialogBuffer;
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
				n2 = this->dialogWithBarMaxChars;
			}
		}
	}
	else if (dialogStyle == 3) {
		this->dialogBuffer->append(text);
		this->dialogBuffer->wrapText(this->scrollMaxChars);
		if (this->dialogBuffer->getNumLines() > this->dialogViewLines) {
			this->dialogBuffer->setLength(0);
			this->dialogBuffer->append(text);
			this->dialogBuffer->wrapText(this->scrollWithBarMaxChars);
		}
	}
	else {
		this->dialogBuffer->append(text);
		this->dialogBuffer->wrapText(this->dialogMaxChars);
		int numLines = this->dialogBuffer->getNumLines();
		if (dialogStyle == 2 || dialogStyle == 16 || dialogStyle == 9) {
			--numLines;
		}
		if (numLines > this->dialogViewLines) {
			this->dialogBuffer->setLength(0);
			this->dialogBuffer->append(text);
			this->dialogBuffer->wrapText(this->dialogWithBarMaxChars);
		}
	}
	int length2 = this->dialogBuffer->length();
	this->numDialogLines = 0;
	while (i < length2) {
		if (this->dialogBuffer->charAt(i) == '|') {
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


void Canvas::startDialog(ScriptThread* scriptThread, short n, int n2, int n3) {
	this->startDialog(scriptThread, (short)0, n, n2, n3, false);
}


void Canvas::startDialog(ScriptThread* scriptThread, short n, short n2, int n3, int n4, bool b) {


	Text* largeBuffer = app->localization->getLargeBuffer();
	app->localization->composeText(n, n2, largeBuffer);
	this->startDialog(scriptThread, largeBuffer, n3, n4, b);
	largeBuffer->dispose();
}


void Canvas::startDialog(ScriptThread* scriptThread, Text* text, int n, int n2) {
	this->startDialog(scriptThread, text, n, n2, false);
}


void Canvas::startDialog(ScriptThread* dialogThread, Text* text, int n, int n2, bool dialogResumeScriptAfterClosed) {
	this->dialogResumeScriptAfterClosed = dialogResumeScriptAfterClosed;
	this->dialogResumeMenu = false;
	this->dialogThread = dialogThread;
	this->readyWeaponSound = 0;
	this->prepareDialog(text, n, n2);
	this->setState(Canvas::ST_DIALOG);
}


void Canvas::dequeueHelpDialog() {
	this->dequeueHelpDialog(false);
}


void Canvas::dequeueHelpDialog(bool b) {


	if (this->numHelpMessages == 0) {
		return;
	}
	if (this->state == Canvas::ST_DIALOG || this->dialogClosing) {
		return;
	}
	if (!b && this->state != Canvas::ST_PLAYING && this->state != Canvas::ST_INTER_CAMERA && app->game->monstersTurn == 0) {
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
			app->Error(0); // ERR_DEQUEUEHELP
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
			this->startDialog(nullptr, largeBuffer, n, n2, false);
		}
		else {
			this->startDialog(&app->game->scriptThreads[n3], largeBuffer, n, n2, true);
		}
	}
	largeBuffer->dispose();
}


void Canvas::enqueueHelpDialog(short n) {
	this->enqueueHelpDialog((short)0, n, (uint8_t)(-1));
}


bool Canvas::enqueueHelpDialog(short n, short n2, uint8_t b) {


	if (!app->player->enableHelp || this->state == Canvas::ST_DYING) {
		return false;
	}
	if (this->numHelpMessages == 16) {
		app->Error(41); // ERR_MAXHELP
		return false;
	}
	this->helpMessageTypes[this->numHelpMessages] = 2;
	this->helpMessageInts[this->numHelpMessages] = (n << 16 | n2);
	this->helpMessageObjs[this->numHelpMessages] = nullptr;
	this->helpMessageThreads[this->numHelpMessages] = b;
	this->numHelpMessages++;
	if (this->state == Canvas::ST_PLAYING) {
		this->dequeueHelpDialog();
	}
	return true;
}


bool Canvas::enqueueHelpDialog(Text* text) {
	return this->enqueueHelpDialog(text, 0);
}


bool Canvas::enqueueHelpDialog(Text* text, int n) {


	if (!app->player->enableHelp || this->state == Canvas::ST_DYING) {
		return false;
	}
	if (this->numHelpMessages == 16) {
		app->Error(41); // ERR_MAXHELP
		return false;
	}
	this->helpMessageTypes[this->numHelpMessages] = n;
	this->helpMessageObjs[this->numHelpMessages] = text;
	this->helpMessageThreads[this->numHelpMessages] = -1;
	this->numHelpMessages++;
	if (this->state == Canvas::ST_PLAYING) {
		this->dequeueHelpDialog();
	}
	return true;
}


void Canvas::enqueueHelpDialog(EntityDef* entityDef) {


	if (!app->player->enableHelp || this->state == Canvas::ST_DYING) {
		return;
	}
	if (this->numHelpMessages == 16) {
		app->Error(41); // ERR_MAXHELP
		return;
	}
	this->helpMessageTypes[this->numHelpMessages] = 1;
	this->helpMessageObjs[this->numHelpMessages] = entityDef;
	this->helpMessageThreads[this->numHelpMessages] = -1;
	this->numHelpMessages++;
	if (this->state == Canvas::ST_PLAYING) {
		this->dequeueHelpDialog();
	}
}


