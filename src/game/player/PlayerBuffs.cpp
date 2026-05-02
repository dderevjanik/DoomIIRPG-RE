#include <stdexcept>
#include <algorithm>
#include <map>
#include <string>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "Player.h"
#include "Combat.h"
#include "Canvas.h"
#include "Render.h"
#include "Game.h"
#include "Text.h"
#include "Hud.h"
#include "Enums.h"
#include "TinyGL.h"
#include "BinaryStream.h"
#include "Sound.h"
#include "DataNode.h"
#include "SoundNames.h"
#include "Sounds.h"
#include "ItemDefs.h"

void Player::updateStatusEffects() {
	if (this->numStatusEffects == 0) {
		return;
	}
	for (int i = 0; i < 18; ++i) {
		if (this->statusEffects[0 + i] != 0) {
			if (this->statusEffects[0 + i] <= 5 && this->statusEffects[0 + i] == 1) {
				this->removeStatusEffect(i);
			} else if (this->statusEffects[0 + i] != 0) {
				--this->statusEffects[0 + i];
			}
		}
	}
	this->translateStatusEffects();
}

void Player::translateStatusEffects() {
	for (int i = 0; i < 15; i++) {
		this->buffs[0 + i] = (this->buffs[15 + i] = 0);
	}

	this->numbuffs = 0;
	for (int j = 0; j < 18; j++) {
		int duration = this->statusEffects[0 + j];
		int amount = this->statusEffects[18 + j];
		if (duration != 0) {
			if (duration > 0) {
				switch (j) {
					case 14: {
						this->buffs[20] -= (short)amount;
						this->buffs[5] = (short)duration;
						this->buffs[19] -= (short)amount;
						this->buffs[4] = (short)duration;
						this->buffs[29] -= (short)(this->statusEffects[36 + j] * 4);
						this->buffs[14] = (short)duration;
						break;
					}
					case 15:
					case 17: {
						this->buffs[20] += (short)amount;
						this->buffs[5] = (short)duration;
						this->buffs[19] += (short)amount;
						this->buffs[4] = (short)duration;
						this->buffs[22] -= (short)(amount + (amount / 2));
						this->buffs[7] = (short)duration;
						break;
					}
					case 16: {
						this->buffs[22] -= (short)amount;
						this->buffs[7] = (short)duration;
						break;
					}
					default: {
						this->buffs[15 + j] += (short)amount;
						this->buffs[0 + j] = (short)duration;
						break;
					}
				}
			}
		}
	}

	for (int k = 0; k < 15; k++) {
		if (this->buffs[0 + k] > 0) {
			if ((1 << k & this->buffNoAmountMask) == 0x0 && this->buffs[15 + k] == 0) {
				this->buffs[0 + k] = 0;
			} else {
				this->numbuffs++;
			}
		}
	}
	this->updateStats();
}

void Player::removeStatusEffect(int effectIdx) {


	if (effectIdx == 18) {
		this->numStatusEffects = 0;
		for (int i = 0; i < 18; i++) {
			this->statusEffects[0 + i] = 0;
			this->statusEffects[18 + i] = 0;
			this->statusEffects[36 + i] = 0;
		}
	} else {
		if (this->statusEffects[36 + effectIdx] == 0) {
			return;
		}

		if (effectIdx == 17) {
			app->render->startFogLerp(1, 0, 2000);
		}

		this->statusEffects[18 + effectIdx] = 0;
		this->statusEffects[36 + effectIdx] = 0;
		this->statusEffects[0 + effectIdx] = 0;
		this->numStatusEffects--;
	}
	this->translateStatusEffects();
}

bool Player::addStatusEffect(int effectIdx, int amount, int duration) {


	if (this->isFamiliar) {
		return false;
	}
	if (effectIdx >= 0 && effectIdx < 15 && this->buffBlockedBy[effectIdx] >= 0) {
		if (this->buffs[this->buffBlockedBy[effectIdx]] > 0) {
			return false;
		}
	}
	if (effectIdx >= 0 && effectIdx < 15 && this->buffApplySound[effectIdx] >= 0) {
		app->sound->playSound(this->buffApplySound[effectIdx], 0, 3, false);
	}
	int newStacks = this->statusEffects[36 + effectIdx] + 1;
	int maxStacks = (effectIdx >= 0 && effectIdx < 15) ? this->buffMaxStacks[effectIdx] : 3;
	if (newStacks > maxStacks) {
		if (effectIdx == 14) {
			this->statusEffects[0 + effectIdx] = duration;
		}
		return false;
	}
	if (newStacks == 1) {
		this->numStatusEffects++;
		if (effectIdx == 17) {
			app->render->fogMin = 0;
			if (app->render->fogRange > 0) {
				app->render->fogRange = -1;
			}
			app->render->startFogLerp(1024, 0, 2000);
		}
	}
	this->statusEffects[18 + effectIdx] += amount;
	this->statusEffects[0 + effectIdx] = duration;
	this->statusEffects[36 + effectIdx] = newStacks;
	return true;
}

void Player::drawStatusEffectIcon(Graphics* graphics, int effectIdx, int amount, int duration, int x, int y) {


	Text* smallBuffer = app->localization->getSmallBuffer();
	smallBuffer->setLength(0);
	if (effectIdx == 8) {
		smallBuffer->append('%');
		smallBuffer->append(amount);
	} else if ((1 << effectIdx & this->buffAmtNotDrawnMask) == 0x0) {
		if (amount >= 0) {
			smallBuffer->append('+');
			smallBuffer->append(amount);
		} else {
			smallBuffer->append(amount);
		}
	}
	graphics->drawString(smallBuffer, x, y + 2, 24);
	graphics->drawBuffIcon(effectIdx, x + 3, y + 1, 0);
	if (app->time - this->turnTime < 600) {
		smallBuffer->setLength(0);
		smallBuffer->append(duration);
		graphics->drawString(smallBuffer, x + 17, y + 2, 17);
		app->canvas->forcePump = true;
	}
	smallBuffer->dispose();
}

void Player::drawBuffs(Graphics* graphics) {

	if (this->numbuffs == 0 || app->canvas->state == Canvas::ST_DIALOG) {
		return;
	}
	int viewRight = app->canvas->viewRect[0] + app->canvas->viewRect[2];
	int boxX = viewRight - 32;
	int rowY = app->canvas->viewRect[1] + 2;
	int drawnCount = 0;
	bool hasOverflow = false;
	int numbuffs = this->numbuffs;
	if (numbuffs > 6) {
		numbuffs = 6;
		hasOverflow = true;
	}
	int boxHeight = numbuffs * 31 + 6;
	for (int i = 0; i < 15 && drawnCount < 6; ++i) {
		if (this->buffs[0 + i] > 0 && (1 << i & this->buffAmtNotDrawnMask) == 0x0) {
			if (this->buffs[15 + i] > 99 || this->buffs[15 + i] < -99) {
				boxX = viewRight - 73;
				break;
			}
			if (this->buffs[15 + i] > 9 || this->buffs[15 + i] < -9) {
				boxX = viewRight - 65;
			} else if (boxX == viewRight - 32) {
				boxX = viewRight - 57;
			}
		}
	}
	int boxWidth = viewRight - boxX + 4;
	if (hasOverflow) {
		boxHeight += 5;
	}
	graphics->setColor(0);
	graphics->fillRect(boxX - 5, rowY - 2, boxWidth, boxHeight);
	graphics->setColor(0xFFAAAAAA);
	graphics->drawRect(boxX - 5, rowY - 2, boxWidth, boxHeight);
	int iconX = viewRight - 36;
	for (int j = 0; j < 15 && drawnCount < 6; ++j) {
		if (this->buffs[0 + j] != 0) {
			++drawnCount;
			this->drawStatusEffectIcon(graphics, j, this->buffs[15 + j], this->buffs[0 + j], iconX, rowY);
			rowY += 31;
		}
	}
	if (hasOverflow) {
		Text* smallBuffer = app->localization->getSmallBuffer();
		smallBuffer->setLength(0);
		smallBuffer->append('\x85');
		graphics->drawString(smallBuffer, boxX + boxWidth / 2 - 4, rowY - 4, 1);
		smallBuffer->dispose();
	}
}

bool Player::hasPurifyEffect() {
	return this->statusEffects[this->purifyBuffIdx] != 0;
}

void Player::clearOutFamiliarsStatusEffects() {
	this->numbuffsCopy = 0;
	for (int i = 0; i < 15; ++i) {
		this->buffsCopy[15 + i] = (this->buffsCopy[0 + i] = 0);
	}
	this->numStatusEffectsCopy = 0;
	for (int j = 0; j < 18; ++j) {
		this->statusEffectsCopy[0 + j] = 0;
		this->statusEffectsCopy[36 + j] = 0;
		this->statusEffectsCopy[18 + j] = 0;
	}
}

void Player::swapStatusEffects() {
	int tempStatusEffects[54];
	short tempBuffs[30];

	int numbuffsCopy = this->numbuffsCopy;
	this->numbuffsCopy = this->numbuffs;
	this->numbuffs = numbuffsCopy;
	std::memcpy(tempBuffs, this->buffs, sizeof(tempBuffs));
	std::memcpy(this->buffs, this->buffsCopy, sizeof(tempBuffs));
	std::memcpy(this->buffsCopy, tempBuffs, sizeof(tempBuffs));

	int numStatusEffectsCopy = this->numStatusEffectsCopy;
	this->numStatusEffectsCopy = this->numStatusEffects;
	this->numStatusEffects = numStatusEffectsCopy;
	std::memcpy(tempStatusEffects, this->statusEffects, sizeof(tempStatusEffects));
	std::memcpy(this->statusEffects, this->statusEffectsCopy, sizeof(tempStatusEffects));
	std::memcpy(this->statusEffectsCopy, tempStatusEffects, sizeof(tempStatusEffects));
}
