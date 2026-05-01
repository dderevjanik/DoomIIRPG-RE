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
		int n = this->statusEffects[0 + j];
		int n2 = this->statusEffects[18 + j];
		if (n != 0) {
			if (n > 0) {
				switch (j) {
					case 14: {
						this->buffs[20] -= (short)n2;
						this->buffs[5] = (short)n;
						this->buffs[19] -= (short)n2;
						this->buffs[4] = (short)n;
						this->buffs[29] -= (short)(this->statusEffects[36 + j] * 4);
						this->buffs[14] = (short)n;
						break;
					}
					case 15:
					case 17: {
						this->buffs[20] += (short)n2;
						this->buffs[5] = (short)n;
						this->buffs[19] += (short)n2;
						this->buffs[4] = (short)n;
						this->buffs[22] -= (short)(n2 + (n2 / 2));
						this->buffs[7] = (short)n;
						break;
					}
					case 16: {
						this->buffs[22] -= (short)n2;
						this->buffs[7] = (short)n;
						break;
					}
					default: {
						this->buffs[15 + j] += (short)n2;
						this->buffs[0 + j] = (short)n;
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

void Player::removeStatusEffect(int n) {


	if (n == 18) {
		this->numStatusEffects = 0;
		for (int i = 0; i < 18; i++) {
			this->statusEffects[0 + i] = 0;
			this->statusEffects[18 + i] = 0;
			this->statusEffects[36 + i] = 0;
		}
	} else {
		if (this->statusEffects[36 + n] == 0) {
			return;
		}

		if (n == 17) {
			app->render->startFogLerp(1, 0, 2000);
		}

		this->statusEffects[18 + n] = 0;
		this->statusEffects[36 + n] = 0;
		this->statusEffects[0 + n] = 0;
		this->numStatusEffects--;
	}
	this->translateStatusEffects();
}

bool Player::addStatusEffect(int n, int n2, int n3) {


	if (this->isFamiliar) {
		return false;
	}
	if (n >= 0 && n < 15 && this->buffBlockedBy[n] >= 0) {
		if (this->buffs[this->buffBlockedBy[n]] > 0) {
			return false;
		}
	}
	if (n >= 0 && n < 15 && this->buffApplySound[n] >= 0) {
		app->sound->playSound(this->buffApplySound[n], 0, 3, false);
	}
	int n4 = this->statusEffects[36 + n] + 1;
	int maxStacks = (n >= 0 && n < 15) ? this->buffMaxStacks[n] : 3;
	if (n4 > maxStacks) {
		if (n == 14) {
			this->statusEffects[0 + n] = n3;
		}
		return false;
	}
	if (n4 == 1) {
		this->numStatusEffects++;
		if (n == 17) {
			app->tinyGL->fogMin = 0;
			if (app->tinyGL->fogRange > 0) {
				app->tinyGL->fogRange = -1;
			}
			app->render->startFogLerp(1024, 0, 2000);
		}
	}
	this->statusEffects[18 + n] += n2;
	this->statusEffects[0 + n] = n3;
	this->statusEffects[36 + n] = n4;
	return true;
}

void Player::drawStatusEffectIcon(Graphics* graphics, int n, int n2, int n3, int n4, int n5) {


	Text* smallBuffer = app->localization->getSmallBuffer();
	smallBuffer->setLength(0);
	if (n == 8) {
		smallBuffer->append('%');
		smallBuffer->append(n2);
	} else if ((1 << n & this->buffAmtNotDrawnMask) == 0x0) {
		if (n2 >= 0) {
			smallBuffer->append('+');
			smallBuffer->append(n2);
		} else {
			smallBuffer->append(n2);
		}
	}
	graphics->drawString(smallBuffer, n4, n5 + 2, 24);
	graphics->drawBuffIcon(n, n4 + 3, n5 + 1, 0);
	if (app->time - this->turnTime < 600) {
		smallBuffer->setLength(0);
		smallBuffer->append(n3);
		graphics->drawString(smallBuffer, n4 + 17, n5 + 2, 17);
		app->canvas->forcePump = true;
	}
	smallBuffer->dispose();
}

void Player::drawBuffs(Graphics* graphics) {

	if (this->numbuffs == 0 || app->canvas->state == Canvas::ST_DIALOG) {
		return;
	}
	int n = app->canvas->viewRect[0] + app->canvas->viewRect[2];
	int n2 = n - 32;
	int n3 = app->canvas->viewRect[1] + 2;
	int n4 = 0;
	bool b = false;
	int numbuffs = this->numbuffs;
	if (numbuffs > 6) {
		numbuffs = 6;
		b = true;
	}
	int n5 = numbuffs * 31 + 6;
	for (int n6 = 0; n6 < 15 && n4 < 6; ++n6) {
		if (this->buffs[0 + n6] > 0 && (1 << n6 & this->buffAmtNotDrawnMask) == 0x0) {
			if (this->buffs[15 + n6] > 99 || this->buffs[15 + n6] < -99) {
				n2 = n - 73;
				break;
			}
			if (this->buffs[15 + n6] > 9 || this->buffs[15 + n6] < -9) {
				n2 = n - 65;
			} else if (n2 == n - 32) {
				n2 = n - 57;
			}
		}
	}
	int n7 = n - n2 + 4;
	if (b) {
		n5 += 5;
	}
	graphics->setColor(0);
	graphics->fillRect(n2 - 5, n3 - 2, n7, n5);
	graphics->setColor(0xFFAAAAAA);
	graphics->drawRect(n2 - 5, n3 - 2, n7, n5);
	int n8 = n - 36;
	for (int n9 = 0; n9 < 15 && n4 < 6; ++n9) {
		if (this->buffs[0 + n9] != 0) {
			++n4;
			this->drawStatusEffectIcon(graphics, n9, this->buffs[15 + n9], this->buffs[0 + n9], n8, n3);
			n3 += 31;
		}
	}
	if (b) {
		Text* smallBuffer = app->localization->getSmallBuffer();
		smallBuffer->setLength(0);
		smallBuffer->append('\x85');
		graphics->drawString(smallBuffer, n2 + n7 / 2 - 4, n3 - 4, 1);
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
