#include "LootingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Player.h"
#include "Combat.h"
#include "Hud.h"
#include "Render.h"
#include "Graphics.h"
#include "Text.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Enums.h"
#include "LootDistributor.h"

static void giveLootPool(Canvas* canvas) {
	Applet* app = canvas->app;
	LootDistributor* ld = app->lootDistributor.get();
	for (int i = 0; i < ld->numPoolItems; ++i) {
		int n = ld->lootPool[i];
		int n2 = n >> 12 & 0xF;
		if (n2 != 6) {
			int n3 = (n & 0xFC0) >> 6;
			app->player->give(n2, n3, n & 0x3F, false);
			if (n2 == 1) {
				int n4 = n3 * 9;
				uint8_t a = app->combat->weapons[n4 + 5];
				if (a != 0) {
					app->player->give(2, app->combat->weapons[n4 + 4], std::max((int)a, 10), false);
				}
			}
		}
	}
	if (ld->lootPoolCredits != 0) {
		app->player->give(0, 24, ld->lootPoolCredits, false);
		ld->lootPoolCredits = 0;
	}
	app->game->foundLoot(canvas->viewX + canvas->viewStepX, canvas->viewY + canvas->viewStepY, canvas->viewZ, ld->numLootItems);
	ld->numPoolItems = 0;
	ld->numLootItems = 0;
	ld->lootText->dispose();
}

void LootingState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	canvas->clearSoftKeys();
	canvas->lootingTime = app->time;
	canvas->crouchingForLoot = true;
	canvas->lootingCachedPitch = canvas->destPitch;
	canvas->lootSoundPlayed = false;
}

void LootingState::onExit(Canvas* canvas) {
}

void LootingState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	app->hud->repaintFlags |= 0x22;
	canvas->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_VIEW3D);
	int height = app->render->getHeight(canvas->destX, canvas->destY);
	int height2 = app->render->getHeight(canvas->destX + canvas->viewStepX, canvas->destY + canvas->viewStepY);
	if (app->time < canvas->lootingTime + 500) {
		int n = (500 - (app->time - canvas->lootingTime) << 16) / 500;
		int n2 = 65536 - n;
		if (canvas->crouchingForLoot) {
			int n3 = (height > height2) ? height : (height * n + height2 * n2 >> 16);
			canvas->viewX = canvas->destX + (48 + (-48 * n >> 16)) * (canvas->viewStepX >> 6);
			canvas->viewY = canvas->destY + (48 + (-48 * n >> 16)) * (canvas->viewStepY >> 6);
			canvas->viewZ = n3 + 26 + (10 * n >> 16);
			canvas->viewPitch = std::max(-(64 - (64 * n >> 16)) + canvas->lootingCachedPitch, -64);
		}
		else {
			int n4 = (height > height2) ? height : (height * n2 + height2 * n >> 16);
			canvas->viewX = canvas->destX + (48 * n >> 16) * (canvas->viewStepX >> 6);
			canvas->viewY = canvas->destY + (48 * n >> 16) * (canvas->viewStepY >> 6);
			canvas->viewZ = n4 + 36 + (-10 * n >> 16);
			canvas->viewPitch = std::max(-(64 * n >> 16) + canvas->lootingCachedPitch, -64);
		}
		canvas->updateView();
	}
	else {
		if (!canvas->lootSoundPlayed) {
			canvas->lootSoundPlayed = true;
			app->sound->playSound(Sounds::getResIDByName(SoundName::LOOT), 0, 3, 0);
		}
		if (canvas->crouchingForLoot) {
			canvas->viewX = canvas->destX + 48 * (canvas->viewStepX >> 6);
			canvas->viewY = canvas->destY + 48 * (canvas->viewStepY >> 6);
			canvas->viewZ = std::max(height, height2) + 26;
			canvas->viewPitch = std::max(-64 + canvas->lootingCachedPitch, -64);
			canvas->updateView();
		}
		else {
			canvas->viewX = canvas->destX;
			canvas->viewY = canvas->destY;
			canvas->viewZ = height + 36;
			canvas->viewPitch = canvas->lootingCachedPitch;
			canvas->updateView();
			canvas->setState(Canvas::ST_PLAYING);
			app->game->advanceTurn();
		}
	}
}

void LootingState::render(Canvas* canvas, Graphics* graphics) {
	Applet* app = canvas->app;
	if (canvas->crouchingForLoot && app->time > canvas->lootingTime + 500) {
		int* dialogRect = canvas->dialogRect;
		dialogRect[0] = canvas->viewRect[0] + 0;
		dialogRect[1] = canvas->viewRect[1] + 0 + 16;
		dialogRect[2] = canvas->viewRect[2] - dialogRect[0] - 0 - 1;
		dialogRect[3] = 48;
		graphics->setColor(0xFF660000);
		graphics->fillRect(dialogRect[0], dialogRect[1], dialogRect[2], dialogRect[3]);
		graphics->setColor(0xFF000000);
		graphics->fillRect(dialogRect[0], dialogRect[1] - 18, dialogRect[2], 18);
		graphics->setColor(0xFFFFFFFF);
		graphics->drawRect(dialogRect[0], dialogRect[1] - 18, dialogRect[2], 18);
		graphics->drawRect(dialogRect[0], dialogRect[1], dialogRect[2], dialogRect[3]);
		Text* smallBuffer = app->localization->getSmallBuffer();
		app->localization->composeText((short)0, (short)227, smallBuffer);
		smallBuffer->dehyphenate();
		graphics->drawString(smallBuffer, canvas->SCR_CX, dialogRect[1] - 16, 1);
		smallBuffer->dispose();
		LootDistributor* ld = app->lootDistributor.get();
		for (int i = 0; i < 3; ++i) {
			graphics->drawString(ld->lootText, dialogRect[0] + 5, dialogRect[1] + 1 + i * 16, 20, ld->lootPoolIndices[2 * (i + ld->lootLineNum)], ld->lootPoolIndices[2 * (i + ld->lootLineNum) + 1]);
		}
		int n = ld->numPoolItems + ((ld->lootPoolCredits != 0) ? 1 : 0);
		canvas->drawScrollBar(graphics, dialogRect[0] + dialogRect[2], dialogRect[1] + 1, dialogRect[3] - 1, ld->lootLineNum, (ld->lootLineNum + 3 > n) ? n : (ld->lootLineNum + 3), n, 3);
	}
}

bool LootingState::handleInput(Canvas* canvas, int key, int action) {
	Applet* app = canvas->app;
	if (canvas->crouchingForLoot && app->time > canvas->lootingTime + 500) {
		LootDistributor* ld = app->lootDistributor.get();
		int max = std::max(ld->numPoolItems + ((ld->lootPoolCredits != 0) ? 1 : 0) - 3, 0);
		if (action == Enums::ACTION_FIRE) {
			if (ld->lootLineNum >= max) {
				canvas->lootingTime = app->time;
				canvas->crouchingForLoot = false;
				giveLootPool(canvas);
			}
			else {
				ld->lootLineNum = std::min(ld->lootLineNum + 3, max);
			}
		}
		else if (action == Enums::ACTION_PASSTURN || action == Enums::ACTION_BACK) {
			canvas->lootingTime = app->time;
			canvas->crouchingForLoot = false;
			giveLootPool(canvas);
		}
		else if (action == Enums::ACTION_DOWN) {
			ld->lootLineNum = std::min(ld->lootLineNum + 1, max);
		}
		else if (action == Enums::ACTION_UP) {
			ld->lootLineNum = std::max(ld->lootLineNum - 1, 0);
		}
		else if (action == Enums::ACTION_LEFT) {
			ld->lootLineNum = 0;
		}
		else if (action == Enums::ACTION_RIGHT) {
			ld->lootLineNum = max;
		}
	}
	return true;
}
