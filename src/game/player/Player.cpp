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
#include "DialogManager.h"
#include "Render.h"
#include "Game.h"
#include "Text.h"
#include "Hud.h"
#include "Enums.h"
#include "BinaryStream.h"
#include "Sound.h"
#include "DataNode.h"
#include "SoundNames.h"
#include "Sounds.h"
#include "ItemDefs.h"
#include "EventBus.h"
#include "GameEvents.h"

Player::Player() {
	this->gameConfig = &CAppContainer::getInstance()->gameConfig;
}

Player::~Player() {
	delete this->itemDefs;
}

bool Player::startup() {
	LOG_INFO("[player] startup\n");
	this->app = CAppContainer::getInstance()->app;
	this->gameConfig = &CAppContainer::getInstance()->gameConfig;
	Applet* app = this->app;
	this->ammo.resize(Enums::MAX_AMMO, 0);
	this->ammoCopy.resize(Enums::MAX_AMMO, 0);
	this->isFamiliar = false;
	this->noclip = false;
	this->god = false;
	this->helpBitmask = 0;
	this->invHelpBitmask = 0;
	this->weaponHelpBitmask = 0;
	this->armorHelpBitmask = 0;
	this->currentLevelDeaths = 0;
	this->totalDeaths = 0;
	this->currentGrades = 0;
	this->bestGrades = 0;
	this->enableHelp = true;
	this->baseCe = new CombatEntity();
	this->ce = new CombatEntity();
	this->reset();

	return true;
}

bool Player::modifyCollision(Entity* entity) {

	return nullptr != entity && entity->def->eType == Enums::ET_SPRITEWALL &&
	       app->render->getSpriteTileNum((entity->info & 0xFFFF) - 1) == 131;
}

void Player::advanceTurn() {


	this->moves++;
	this->totalMoves++;
	this->updateStatusEffects();
	bool tookDamage = false;

	if (this->buffs[this->regenBuffIdx] > 0) {
		this->addHealth(this->buffs[15 + this->regenBuffIdx], false);
	}

	if (this->statusEffects[53] > 0) {
		this->addHealth(-this->statusEffects[35]);
		tookDamage = true;
	}

	if (this->statusEffects[this->fireBuffIdx] > 0) {
		int fireDmg = this->buffPerTurnDamage[this->fireBuffIdx];
		this->addHealth(-fireDmg);
		Text* text = app->hud->getMessageBuffer(0);
		app->localization->composeText(0, 82, text);
		text->append(' ');
		app->localization->resetTextArgs();
		app->localization->addTextArg(fireDmg);
		app->localization->composeText(0, 71, text);
		app->hud->finishMessageBuffer();
		tookDamage = true;
	}

	if (this->inCombat && this->totalMoves - this->lastCombatTurn >= this->gameConfig->outOfCombatTurns) {
		this->inCombat = false;
	}

	if (this->statusEffects[15] > 0) {
		++this->counters[3];
	}

	this->turnTime = app->time;
	if (tookDamage && app->canvas->state == Canvas::ST_AUTOMAP) {
		app->canvas->setState(Canvas::ST_PLAYING);
	}
}

void Player::levelInit() {

	this->moves = 0;
	this->numNotebookIndexes = 0;
	this->questComplete = 0;
	this->questFailed = 0;
	this->turnTime = app->time;
	this->inCombat = false;
	if (this->ce->getStat(Enums::STAT_HEALTH) == 0) {
		this->ce->setStat(Enums::STAT_HEALTH, 1);
	}
}

void Player::fillMonsterStats() {


	int totalCount = 0;
	int aliveCount = 0;
	for (int i = 0; i < app->game->numEntities; ++i) {
		Entity* entity = &app->game->entities[i];
		if (entity->isMonster()) {
			if ((entity->monsterFlags & Enums::MFLAG_NOTRACK) == 0x0) {
				++totalCount;
				if ((app->game->entities[i].info & 0x1010000) != 0x0) {
					++aliveCount;
				}
			}
		}
	}

	this->monsterStats[0] = aliveCount;
	this->monsterStats[1] = totalCount;
}

void Player::readyWeapon() {
	this->app->canvas->readyWeaponSound = 2;
}

void Player::selectWeapon(int weaponIdx) {

	LOG_INFO("[player] selectWeapon: {} -> {}\n", this->ce->weapon, weaponIdx);
	if (this->isFamiliar) {
		return;
	}
	if (!app->combat->getWeaponFlags(weaponIdx).isThrowableItem) {
		int tiIdx = app->combat->throwableItemWeaponIdx;
		if (tiIdx >= 0) {
			this->weapons &= ~(1 << tiIdx);
			this->ammo[app->combat->throwableItemAmmoType] = 0;
		}
	}
	if (app->canvas->isZoomedIn) {
		app->canvas->zoomOut();
	}
	if ((this->weapons & ~this->disabledWeapons & 1 << weaponIdx) == 0x0) {
		this->ce->weapon = weaponIdx;
		this->selectNextWeapon();
	}
	if (this->ce->weapon != weaponIdx) {
		app->canvas->invalidateRect();
		this->prevWeapon = this->ce->weapon;
	}
	this->ce->weapon = weaponIdx;
	if (app->canvas->state != Canvas::ST_DIALOG && app->canvas->state != Canvas::ST_CAMERA) {
		app->canvas->drawPlayingSoftKeys();
	}
	if (weaponIdx != this->prevWeapon) {
		this->readyWeapon();
	}
	this->activeWeaponDef = app->entityDefManager->find(6, 1, weaponIdx);
	app->canvas->updateFacingEntity = true;
	app->hud->repaintFlags |= 0x4;
	if (weaponIdx != this->prevWeapon) {
		app->eventBus->emit(WeaponSwitchEvent{this->prevWeapon, weaponIdx});
	}
}

void Player::selectPrevWeapon() {


	int weapon = this->ce->weapon;
	int availableWeapons = this->weapons & ~this->disabledWeapons;
	for (int i = weapon - 1; i >= 0; --i) {
		if ((availableWeapons & 1 << i) != 0x0) {
			int weaponFieldOffset = i * 9;
			if (app->combat->weapons[weaponFieldOffset + 5] == 0 || this->ammo[app->combat->weapons[weaponFieldOffset + 4]] != 0 || i == 2) {
				this->selectWeapon(i);
				return;
			}
		}
	}
	if (this->ce->weapon == weapon && !app->combat->getWeaponFlags(weapon).isThrowableItem) {
		for (int j = 14; j > weapon; --j) {
			if ((availableWeapons & 1 << j) != 0x0) {
				int weaponFieldOffset = j * 9;
				if (app->combat->weapons[weaponFieldOffset + 5] == 0 || this->ammo[app->combat->weapons[weaponFieldOffset + 4]] != 0 || j == 2) {
					this->selectWeapon(j);
					return;
				}
			}
		}
	}
}

void Player::selectNextWeapon() {


	int weapon = this->ce->weapon;
	int availableWeapons = this->weapons & ~this->disabledWeapons;
	for (int i = weapon + 1; i < 15; ++i) {
		if ((availableWeapons & 1 << i) != 0x0) {
			int weaponFieldOffset = i * 9;
			if (app->combat->weapons[weaponFieldOffset + 5] == 0 || this->ammo[app->combat->weapons[weaponFieldOffset + 4]] != 0 || i == 2) {
				this->selectWeapon(i);
				return;
			}
		}
	}
	if (this->ce->weapon == weapon && weapon != 0) {
		for (int j = 0; j < weapon; ++j) {
			if ((availableWeapons & 1 << j) != 0x0) {
				int weaponFieldOffset = j * 9;
				if (app->combat->weapons[weaponFieldOffset + 5] == 0 || this->ammo[app->combat->weapons[weaponFieldOffset + 4]] != 0 || j == 2) {
					this->selectWeapon(j);
					return;
				}
			}
		}
	}
}

int Player::getHealth() {
	return this->isFamiliar ? this->ammo[app->combat->familiarAmmoType] : this->ce->getStat(0);
}

int Player::modifyStat(int statId, int delta) {
	int before;
	int after;
	if (statId == 0) {
		before = this->getHealth();
		this->addHealth(delta);
		if (delta < 0) {
			this->painEvent(nullptr, false);
		}
		after = this->getHealth();
	} else {
		before = this->baseCe->getStat(statId);
		int cap = (statId == 7) ? 200 : 99;
		if (before + delta > cap) {
			delta = cap - before;
		}
		if (delta != 0) {
			this->baseCe->setStat(statId, before + delta);
		}
		this->updateStats();
		after = this->baseCe->getStat(statId);
	}
	return after - before;
}

bool Player::requireStat(int statId, int minValue) {
	return this->ce->getStat(statId) >= minValue;
}

bool Player::requireItem(int category, int index, int minCount, int maxCount) {
	int bit = 1 << index;
	if (category != 1) {
		return category == 0 && this->inventory[index - 0] >= minCount && this->inventory[index - 0] <= maxCount;
	}
	if (maxCount != 0) {
		return (this->weapons & bit) != 0x0;
	}
	return (this->weapons & bit) == 0x0;
}

void Player::addXP(int xp) {

	LOG_INFO("[player] addXP: {} (total={}, nextLevel={})\n", xp, this->currentXP + xp, this->nextLevelXP);
	app->localization->resetTextArgs();
	app->localization->addTextArg(xp);
	if (xp < 0) {
		app->hud->addMessage((short)102);
	} else {
		app->hud->addMessage((short)103);
	}
	this->currentXP += xp;
	this->xpGained += xp;
	while (this->currentXP >= this->nextLevelXP) {
		this->addLevel();
	}
	this->counters[5] += xp;
}

void Player::addLevel() {

	Text* textBuff;
	int stat;

	LOG_INFO("[player] levelUp: {} -> {}\n", this->level, this->level + 1);
	this->level++;
	this->nextLevelXP = this->calcLevelXP(this->level);

	textBuff = app->localization->getLargeBuffer();
	textBuff->setLength(0);
	app->localization->resetTextArgs();
	app->localization->addTextArg(this->level);
	app->localization->composeText((short)0, (short)104, textBuff);

	const GameConfig& gc = *this->gameConfig;
	int healthBonus = gc.levelUpHealth;
	stat = this->baseCe->getStat(1);
	if (stat + healthBonus > 999) {
		healthBonus = 999 - stat;
	}
	if (healthBonus != 0) {
		this->baseCe->setStat(Enums::STAT_MAX_HEALTH, stat + healthBonus);
		app->localization->resetTextArgs();
		app->localization->addTextArg(healthBonus);
		app->localization->composeText((short)0, (short)105, textBuff);
	}

	this->baseCe->getStat(Enums::STAT_DEFENSE);
	stat = this->modifyStat(Enums::STAT_DEFENSE, gc.levelUpDefense);
	if (stat != 0) {
		app->localization->resetTextArgs();
		app->localization->addTextArg(stat);
		app->localization->composeText((short)0, (short)106, textBuff);
	}

	this->baseCe->getStat(Enums::STAT_STRENGTH);
	stat = this->modifyStat(Enums::STAT_STRENGTH, gc.levelUpStrength);
	if (stat != 0) {
		app->localization->resetTextArgs();
		app->localization->addTextArg(stat);
		app->localization->composeText((short)0, (short)107, textBuff);
	}

	this->baseCe->getStat(Enums::STAT_ACCURACY);
	stat = this->modifyStat(Enums::STAT_ACCURACY, gc.levelUpAccuracy);
	if (stat != 0) {
		app->localization->resetTextArgs();
		app->localization->addTextArg(stat);
		app->localization->composeText((short)0, (short)108, textBuff);
	}

	this->baseCe->getStat(Enums::STAT_AGILITY);
	stat = this->modifyStat(Enums::STAT_AGILITY, gc.levelUpAgility);
	if (stat != 0) {
		app->localization->resetTextArgs();
		app->localization->addTextArg(stat);
		app->localization->composeText((short)0, (short)109, textBuff);
	}
	app->localization->composeText((short)0, (short)110, textBuff);

	this->ce->setStat(Enums::STAT_HEALTH, this->ce->getStat(Enums::STAT_MAX_HEALTH));
	/*if ((this->weapons & 0x78) != 0x0) {
	    this->ammo[app->combat->familiarAmmoType] = 100;
	}*/
	app->hud->repaintFlags |= 0x4;

	bool dispose = true;
	if (app->canvas->state != Canvas::ST_MENU) {
		app->sound->soundStop();
		app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_LEVELUP), 0, 5, false);
		dispose = (app->dialogManager->enqueueHelpDialog(app->canvas.get(), textBuff, 0) ? false : true);
	}

	if (dispose) {
		textBuff->dispose();
	}
}

int Player::calcLevelXP(int level) {
	const GameConfig& gc = *this->gameConfig;
	return gc.xpLinear * level + gc.xpCubic * ((level - 1) * (level - 1) * (level - 1) + (level - 1));
}

int Player::calcScore() {
	const GameConfig& gc = *this->gameConfig;

	int score = 0;
	bool allLevelsBeaten = true;
	int i;
	for (i = 0; i <= 8; ++i) {
		if ((this->killedMonstersLevels & 1 << i) != 0x0) {
			score += gc.scorePerLevel;
		} else {
			allLevelsBeaten = false;
		}
	}
	if (allLevelsBeaten) {
		score += gc.scoreAllLevelsBonus;
	}
	if (this->totalDeaths == 0) {
		score += gc.scoreNoDeathsBonus;
	} else if (this->totalDeaths < gc.scoreDeathThreshold) {
		score += (gc.scoreDeathPenaltyBase - this->totalDeaths) * gc.scoreDeathPenaltyMult;
	} else {
		score -= gc.scoreManyDeathsPenalty;
	}
	int minutesPlayed = (this->totalTime + (app->gameTime - this->playTime)) / 60000;
	if (minutesPlayed < gc.scoreTimeBonusMinutes) {
		score += (gc.scoreTimeBonusMinutes - minutesPlayed) * gc.scoreTimeBonusMult;
	}
	if (this->totalMoves < gc.scoreMoveThreshold) {
		score += (gc.scoreMoveThreshold - this->totalMoves) / gc.scoreMoveDivisor;
	}
	bool allSecretsFound = true;
	while (i <= 8) {
		if ((this->foundSecretsLevels & 1 << i) != 0x0) {
			score += gc.scorePerSecret;
		} else {
			allSecretsFound = false;
		}
		++i;
	}
	if (allSecretsFound) {
		score += gc.scoreAllSecretsBonus;
	}
	return score;
}


void Player::setStatsAccordingToCharacterChoice() {
	if (this->characterChoice == 0) {
		return;
	}

	DataNode config = DataNode::loadFile("characters.yaml");
	if (config) {
		DataNode chars = config["characters"];
		if (!chars || !chars.isMap()) {
			app->Error("characters.yaml: missing or invalid 'characters' map");
			return;
		}

		int idx = this->characterChoice - 1; // characterChoice is 1-based
		int ci = 0;
		for (auto it = chars.begin(); it != chars.end(); ++it, ++ci) {
			if (ci == idx) {
				DataNode c = it.value();
				this->baseCe->setStat(Enums::STAT_DEFENSE, c["defense"].asInt());
				this->baseCe->setStat(Enums::STAT_STRENGTH, c["strength"].asInt());
				this->baseCe->setStat(Enums::STAT_ACCURACY, c["accuracy"].asInt());
				this->baseCe->setStat(Enums::STAT_AGILITY, c["agility"].asInt());
				this->baseCe->setStat(Enums::STAT_IQ, c["iq"].asInt());
				int credits = c["credits"].asInt();
				this->give(0, 24, credits, true);
				return;
			}
		}
		app->Error("characters.yaml: character index %d not found", idx);
	}
}



bool Player::useItem(int itemIdx) {

	// Find item definition for this inventory index
	const ItemDef* def = nullptr;
	if (this->itemDefs) {
		for (const auto& d : *this->itemDefs) {
			if (d.index == itemIdx) {
				def = &d;
				break;
			}
		}
	}

	if (!def) {
		return false;
	}

	if (this->inventory[itemIdx] == 0 && !def->skipEmptyCheck) {
		return false;
	}

	// Check requires_no_buff
	if (def->requiresNoBuff >= 0 && this->statusEffects[def->requiresNoBuff] != 0) {
		return false;
	}

	// No effects = immediate success (e.g. pack)
	if (def->effects.empty() && def->bonusEffects.empty()) {
		return true;
	}

	// Apply main effects
	bool anyApplied = false;
	bool allApplied = true;
	bool hadStatusEffect = false;

	for (const auto& e : def->effects) {
		bool ok = false;
		switch (e.type) {
			case ItemEffect::HEALTH:
				ok = this->addHealth(e.amount);
				break;
			case ItemEffect::ARMOR:
				ok = this->addArmor(e.amount);
				break;
			case ItemEffect::STATUS_EFFECT:
				ok = this->addStatusEffect(e.buffIndex, e.amount, e.duration);
				if (ok) hadStatusEffect = true;
				break;
		}
		if (ok) anyApplied = true;
		else allApplied = false;
	}

	bool success = def->requireAll ? allApplied : anyApplied;
	if (!success) {
		return false;
	}

	if (hadStatusEffect) {
		this->translateStatusEffects();
	}

	// Remove buffs
	for (int buffIdx : def->removeBuffs) {
		this->removeStatusEffect(buffIdx);
	}

	// Apply bonus effects (always applied, don't affect success)
	for (const auto& e : def->bonusEffects) {
		switch (e.type) {
			case ItemEffect::HEALTH:
				this->addHealth(e.amount);
				break;
			case ItemEffect::ARMOR:
				this->addArmor(e.amount);
				break;
			case ItemEffect::STATUS_EFFECT:
				this->addStatusEffect(e.buffIndex, e.amount, e.duration);
				this->translateStatusEffects();
				break;
		}
	}

	// Ammo cost
	if (def->ammoCostType >= 0) {
		this->ammo[def->ammoCostType] -= def->ammoCostAmount;
	}

	app->sound->playSound(Sounds::getResIDByName(SoundName::USE_ITEM), 0, 3, false);
	if (def->consume) {
		--this->inventory[itemIdx];
	}
	if (def->advanceTurn) {
		app->game->advanceTurn();
	}
	return true;
}

bool Player::give(int category, int index, int count) {
	return this->give(category, index, count, false);
}

bool Player::give(int category, int index, int count, bool silent) {
	return this->give(category, index, count, silent, false);
}

bool Player::give(int category, int index, int count, bool silent, bool selfPickup) {

	if (count == 0) {
		return false;
	}
	int bit = 1 << (index & 0xffU);
	switch (category) {
		case 1: {
			if (this->weaponIsASentryBot((index & 0xffU)) && count > 0) {
				if (this->isFamiliar) {
					return false;
				}
				this->give(2, 7, 100, true);
				this->weapons &= ~(8 | 16 | 32 | 64);
			}
			bool wasNew = (this->weapons & bit) == 0x0;
			if (count < 0) {
				if (!this->isFamiliar || selfPickup) {
					this->weapons &= ~bit;
					if (index == this->ce->weapon) {
						this->selectNextWeapon();
					}
				}
				return true;
			}
			if (this->isFamiliar && !selfPickup) {
				this->weaponsCopy |= bit;
			} else {
				this->weapons |= bit;
			}
			if (!silent) {
				this->showWeaponHelp(index, false);
			}
			if (wasNew && !this->isFamiliar) {
				this->selectWeapon(index);
			}
			break;
		}
		case 0: {
			short* array = (this->isFamiliar && !selfPickup) ? this->inventoryCopy : this->inventory;
			const GameConfig& gc = *this->gameConfig;
			int newCount = count + array[index - 0];
			if (index == 24) {
				if (newCount > gc.capCredits) {
					newCount = gc.capCredits;
				}
			} else if (newCount > gc.capInventory) {
				newCount = gc.capInventory;
			}
			if (newCount < 0) {
				return false;
			}
			if (index == 13 && (!this->isFamiliar || selfPickup)) {
				this->give(2, 3, newCount * 20, true, true);
			} else {
				array[index - 0] = (short)newCount;
			}
			if (!silent) {
				this->showInvHelp(index - 0, false);
				break;
			}
			break;
		}
		case 2: {
			short* array2 = (this->isFamiliar && !selfPickup && index != 6) ? this->ammoCopy.data() : this->ammo.data();
			const GameConfig& gc2 = *this->gameConfig;
			int newCount = count + array2[index];
			if (newCount > gc2.capAmmo) {
				newCount = gc2.capAmmo;
			}
			if (index == 6 && newCount > gc2.capBotFuel) {
				newCount = gc2.capBotFuel;
			}
			if (newCount < 0) {
				return false;
			}
			array2[index] = (short)newCount;
			if (!silent) {
				this->showAmmoHelp(index, false);
			}
			app->hud->repaintFlags |= 0x4;
			break;
		}
		case 3: {
			this->addHealth(count);
			break;
		}
		default: {
			return false;
		}
	}
	return true;
}

void Player::giveAmmoWeapon(int weaponIdx, bool silent) {
	this->weapons |= 1 << (weaponIdx & 0xffU);
	this->selectWeapon(weaponIdx);
	if (!silent) {
		this->showWeaponHelp(weaponIdx, false);
	}
}

void Player::updateQuests(short questId, int status) {


	if (status == 0) {
		if (this->numNotebookIndexes == 8) {
			// app->Error(Enums::ERR_MAX_NOTEBOOKINDEXES); // J2ME
			return;
		}
		this->questComplete &= (uint8_t)~(1 << this->numNotebookIndexes);
		this->questFailed &= (uint8_t)~(1 << this->numNotebookIndexes);
		this->notebookIndexes[this->numNotebookIndexes++] = questId;
	} else {
		for (int i = 0; i < this->numNotebookIndexes; ++i) {
			if (questId == this->notebookIndexes[i]) {
				if (status == 1) {
					this->questComplete |= (uint8_t)(1 << i);
				} else if (status == 2) {
					this->questFailed |= (uint8_t)(1 << i);
				}
				return;
			}
		}
		if (status == 1) {
			this->questComplete |= (uint8_t)(1 << this->numNotebookIndexes);
			this->questFailed &= (uint8_t)~(1 << this->numNotebookIndexes);
		} else if (status == 2) {
			this->questComplete &= (uint8_t)~(1 << this->numNotebookIndexes);
			this->questFailed |= (uint8_t)(1 << this->numNotebookIndexes);
		}
		this->notebookPositions[this->numNotebookIndexes] = 0;
		this->notebookIndexes[this->numNotebookIndexes++] = questId;
	}
}

void Player::setQuestTile(int questId, int tileX, int tileY) {
	for (int i = 0; i < this->numNotebookIndexes; ++i) {
		if (questId == this->notebookIndexes[i]) {
			this->notebookPositions[i] = (short)(tileX << 5 | tileY);
			return;
		}
	}
}

bool Player::isQuestDone(int questIdx) {
	return (this->questComplete & 1 << questIdx) != 0x0;
}

bool Player::isQuestFailed(int questIdx) {
	return (this->questFailed & 1 << questIdx) != 0x0;
}

void Player::formatTime(int millis, Text* text) {
	text->setLength(0);
	int totalSeconds = millis / 1000;
	int totalMinutes = totalSeconds / 60;
	int hours = totalMinutes / 60;
	int mins = totalMinutes % 60;
	text->append(hours);
	text->append(":");
	if (mins < 10) {
		text->append("0");
	}
	text->append(mins);
	text->append(":");
	int secs = totalSeconds - totalMinutes * 60;
	if (secs < 10) {
		text->append("0");
	}
	text->append(secs);
}

void Player::showInvHelp(int itemIdx, bool force) {


	if (!this->enableHelp && !force) {
		return;
	}
	int idx = itemIdx - 0;
	if ((this->invHelpBitmask & 1 << idx) != 0x0) {
		return;
	}
	this->invHelpBitmask |= 1 << idx;
	app->dialogManager->enqueueHelpDialog(app->canvas.get(), app->entityDefManager->find(6, 0, itemIdx));
}

void Player::showAmmoHelp(int ammoType, bool force) {


	if (!this->enableHelp && !force) {
		return;
	}
	int bit = 1 << ammoType;
	if ((this->ammoHelpBitmask & bit) != 0x0) {
		return;
	}
	this->ammoHelpBitmask |= bit;
	app->dialogManager->enqueueHelpDialog(app->canvas.get(), app->entityDefManager->find(6, 2, ammoType));
}

bool Player::showHelp(short helpId, bool force) {


	if (app->game->isCameraActive()) {
		return false;
	}

	if (!this->enableHelp && !force) {
		return false;
	}
	if ((this->helpBitmask & 1 << helpId) != 0x0 && !force) {
		return false;
	}
	this->helpBitmask |= 1 << helpId;
	app->dialogManager->enqueueHelpDialog(app->canvas.get(), helpId);
	if (helpId != 5 && (app->canvas->state == Canvas::ST_AUTOMAP || app->canvas->state == Canvas::ST_PLAYING)) {
		app->dialogManager->dequeueHelpDialog(app->canvas.get());
	}
	return true;
}

void Player::showWeaponHelp(int weaponIdx, bool force) {


	if (!this->enableHelp && !force) {
		return;
	}
	if ((this->weaponHelpBitmask & 1 << weaponIdx) != 0x0) {
		return;
	}
	this->weaponHelpBitmask |= 1 << weaponIdx;
	app->dialogManager->enqueueHelpDialog(app->canvas.get(), app->entityDefManager->find(6, 1, weaponIdx));
}


void Player::setCharacterChoice(short choice) {

	LOG_INFO("[player] setCharacterChoice: {}\n", choice);
	this->characterChoice = choice;
	app->game->scriptStateVars[14] = choice;
}


void Player::unpause(int delay) {
	if (delay <= 0) {
		return;
	}
}


void Player::resetCounters() {
	for (int i = 0; i < 8; ++i) {
		this->counters[i] = 0;
	}
}

Entity* Player::getPlayerEnt() {
	return &this->app->game->entities[1];
}

void Player::setPickUpWeapon(int tileIdx) {


	EntityDef* lookup = nullptr;
	EntityDef* find = app->entityDefManager->find(6, 1, 14);
	if (tileIdx != 15) {
		lookup = app->entityDefManager->lookup(tileIdx);
	}
	if (lookup != nullptr) {
		find->tileIndex = lookup->tileIndex;
		find->name = lookup->name;
		find->longName = lookup->longName;
		find->description = lookup->description;
	} else {
		find->tileIndex = 15;
		find->name = 159;
		find->longName = 159;
		find->description = 159;
	}
}

void Player::giveAll() {


	if (!this->isFamiliar) {
		if (this->hasASentryBot()) {
			for (int i = 0; i <= 3; ++i) {
				int botWeaponIdx = 3 + i;
				if ((this->weapons & 1 << botWeaponIdx) != 0x0) {
					this->weapons &= ~(1 << botWeaponIdx);
					this->weapons |= 1 << 3 + (i + 1) % 4;
					break;
				}
			}
		} else {
			this->weapons |= 0x8;
		}
		this->weapons |= (0x3FFF & 0xFFFFFFF7 & 0xFFFFFFEF & 0xFFFFFFDF & 0xFFFFFFBF);
		this->selectPrevWeapon();
		this->selectNextWeapon();
	}
	const GameConfig& gc = *this->gameConfig;
	for (int j = 0; j < Enums::MAX_AMMO; ++j) {
		if (j != Enums::AMMO_ITEM) {
			this->give(2, j, gc.capAmmo, true);
		}
	}
	for (int k = 0; k < 26; ++k) {
		if (k != 24) {
			this->give(0, (uint8_t)k, gc.capInventory, true);
			app->dialogManager->numHelpMessages = 0;
		}
	}
	this->give(0, 24, gc.capCredits, true);
	this->ce->setStat(Enums::STAT_HEALTH, this->ce->getStat(Enums::STAT_MAX_HEALTH));
}

void Player::equipForLevel(int highestMap) {

	if (this->isFamiliar) {
		this->familiarReturnsToPlayer(false);
	}
	int viewX = app->canvas->viewX;
	int viewY = app->canvas->viewY;
	int viewAngle = app->canvas->viewAngle;
	this->reset();
	app->canvas->viewX = (app->canvas->destX = (app->canvas->saveX = (app->canvas->prevX = viewX)));
	app->canvas->viewY = (app->canvas->destY = (app->canvas->saveY = (app->canvas->prevY = viewY)));
	app->canvas->viewZ = (app->canvas->destZ = app->render->getHeight(app->canvas->viewX, app->canvas->viewY) + 36);
	app->canvas->viewAngle = (app->canvas->destAngle = (app->canvas->saveAngle = viewAngle));
	app->canvas->viewPitch = (app->canvas->destPitch = 0);
	this->highestMap = highestMap;
	bool enableHelp = this->enableHelp;
	this->enableHelp = false;
	this->weapons = 0;
	app->game->numMallocsForVIOS = 0;

	// Load map starting loadout from per-level level.yaml
	{
		// Build weapon name→index lookup from weapons.yaml
		std::map<std::string, int> weaponNameToIndex;
		DataNode wpConfig = DataNode::loadFile("weapons.yaml");
		if (wpConfig) {
			DataNode wpNode = wpConfig["weapons"];
			if (wpNode) {
				for (auto it = wpNode.begin(); it != wpNode.end(); ++it)
					weaponNameToIndex[it.key().asString()] = it.value()["index"].asInt(-1);
			}
		}

		// Resolve weapon name or index
		auto resolveWeapon = [&](const DataNode& node) -> int {
			if (node.isScalar()) {
				std::string s = node.asString();
				if (auto it = weaponNameToIndex.find(s); it != weaponNameToIndex.end()) return it->second;
				try { return std::stoi(s); } catch (...) { return -1; }
			}
			return node.asInt(-1);
		};

		// Map 10 uses map 9 loadout
		int lookupMap = (highestMap == 10) ? 9 : highestMap;
		const GameConfig& gc = *this->gameConfig;
		DataNode config2;
		if (auto lit = gc.levelInfos.find(lookupMap); lit != gc.levelInfos.end()) {
			config2 = DataNode::loadFile(lit->second.configFile.c_str());
		}
		DataNode r = config2 ? config2["starting_loadout"] : DataNode();
		if (r) {
				// Weapons
				DataNode weapons = r["weapons"];
				if (weapons) {
					for (int w = 0; w < weapons.size(); w++) {
						int idx = resolveWeapon(weapons[w]);
						if (idx >= 0) this->give(1, idx, 1);
					}
				}
				// Weapons given with specific ammo amounts
				DataNode wa = r["weapon_ammo"];
				if (wa) {
					for (auto it = wa.begin(); it != wa.end(); ++it) {
						int idx = resolveWeapon(it.key());
						if (idx >= 0) this->give(1, idx, it.value().asInt());
					}
				}
				// Armor
				if (r["armor"])
					this->addArmor(r["armor"].asInt());
				// Inventory items
				DataNode inv = r["inventory"];
				if (inv) {
					for (auto it = inv.begin(); it != inv.end(); ++it) {
						int idx = ItemDefs::getInventoryIndex(it.key().asString());
						if (idx >= 0) this->give(0, idx, it.value().asInt());
					}
				}
				// Ammo
				DataNode ammo = r["ammo"];
				if (ammo) {
					for (auto it = ammo.begin(); it != ammo.end(); ++it) {
						int idx = ItemDefs::getAmmoIndex(it.key().asString());
						if (idx >= 0) this->give(2, idx, it.value().asInt());
					}
				}
				// XP
				if (r["xp"])
					this->addXP(r["xp"].asInt());
				// Stats
				DataNode stats = r["stats"];
				if (stats) {
					if (stats["defense"])  this->modifyStat(Enums::STAT_DEFENSE, stats["defense"].asInt());
					if (stats["strength"]) this->modifyStat(Enums::STAT_STRENGTH, stats["strength"].asInt());
					if (stats["accuracy"]) this->modifyStat(Enums::STAT_ACCURACY, stats["accuracy"].asInt());
					if (stats["agility"])  this->modifyStat(Enums::STAT_AGILITY, stats["agility"].asInt());
					if (stats["iq"])       this->modifyStat(Enums::STAT_IQ, stats["iq"].asInt());
				}
				// VIOS mallocs
				app->game->numMallocsForVIOS = r["vios_mallocs"].asInt(0);
			}
	}
	this->give(0, 18, 1);
	this->enableHelp = enableHelp;
	this->selectNextWeapon();
	app->canvas->updateFacingEntity = true;
}

int Player::distFrom(Entity* entity) {

	return entity->distFrom(app->canvas->destX, app->canvas->destY);
}

void Player::showAchievementMessage(int achievementIdx) {


	Text* smallBuffer = app->localization->getSmallBuffer();
	Text* smallBuffer2 = app->localization->getSmallBuffer();
	app->localization->resetTextArgs();
	switch (achievementIdx) {
		case 0: {
			app->localization->composeText(136, smallBuffer);
			break;
		}
		case 1: {
			app->localization->composeText(137, smallBuffer);
			break;
		}
		case 2: {
			app->localization->composeText(138, smallBuffer);
			break;
		}
		case 3: {
			app->localization->composeText(139, smallBuffer);
			break;
		}
	}
	app->localization->addTextArg(smallBuffer);
	smallBuffer->dispose();
	app->localization->composeText((short)0, (short)135, smallBuffer2);
	if (!app->dialogManager->enqueueHelpDialog(app->canvas.get(), smallBuffer2, 3)) {
		smallBuffer2->dispose();
	}
	int xpReward = 10;
	if (achievementIdx == 2) {
		xpReward = this->calcLevelXP(this->level) - calcLevelXP(this->level - 1) >> 4;
	}
	this->addXP(xpReward);
	app->sound->playSound(Sounds::getResIDByName(SoundName::CHIME), 0, 3, false);
}

short Player::gradeToString(int grade) {
	short stringId = 44;
	switch (grade) {
		case 6: {
			stringId = 38;
			break;
		}
		case 5: {
			stringId = 39;
			break;
		}
		case 4: {
			stringId = 40;
			break;
		}
		case 3: {
			stringId = 41;
			break;
		}
		case 2: {
			stringId = 42;
			break;
		}
		case 1: {
			stringId = 43;
			break;
		}
	}
	return stringId;
}

int Player::levelGrade(bool record) {


	int currentGrade = this->getCurrentGrade(app->canvas->loadMapID);
	if (currentGrade != 0) {
		return currentGrade;
	}
	this->fillMonsterStats();
	if (this->monsterStats[1] == 0 || app->game->totalSecrets == 0) {
		return 0;
	}
	int max = std::max(((6 * ((this->monsterStats[0] << 8) / this->monsterStats[1]) >> 8) +
	                        (6 * ((app->game->mapSecretsFound << 8) / app->game->totalSecrets) >> 8) >>
	                    1) -
	                       this->currentLevelDeaths,
	                   1);
	if (record) {
		if (max > this->getBestGrade(app->canvas->loadMapID)) {
			this->setBestGrade(app->canvas->loadMapID, max);
		}
		this->setCurrentGrade(app->canvas->loadMapID, max);
	}
	return max;
}

int Player::finalCurrentGrade() {
	int total = 0;
	for (int i = 1; i <= 9; ++i) {
		total += this->getCurrentGrade(i);
	}
	return total / 9;
}

int Player::finalBestGrade() {
	int total = 0;
	for (int i = 1; i <= 9; ++i) {
		total += this->getBestGrade(i);
	}
	return total / 9;
}

int Player::getCurrentGrade(int mapId) {
	return (this->currentGrades >> (3 * (mapId - 1))) & 0x7;
}

void Player::setCurrentGrade(int mapId, int grade) {
	this->currentGrades |= grade << (3 * (mapId - 1));
}

int Player::getBestGrade(int mapId) {
	return (this->bestGrades >> (3 * (mapId - 1))) & 0x7;
}

void Player::setBestGrade(int mapId, int grade) {
	this->bestGrades &= ~(7 << (3 * (mapId - 1)));
	this->bestGrades |= grade << (3 * (mapId - 1));
}

bool Player::vendingMachineIsHacked(int vmId) {
	return (this->hackedVendingMachines & 1 << vmId) == 1 << vmId;
}

void Player::setVendingMachineHack(int vmId) {
	this->hackedVendingMachines |= 1 << vmId;
}

int Player::getVendingMachineTriesLeft(int max) {
	max = std::max(std::min(max, 18), 1);
	int packed;
	if (max > 9) {
		packed = this->vendingMachineHackTriesLeft2;
		max -= 9;
	} else {
		packed = this->vendingMachineHackTriesLeft1;
	}
	return (packed >> (3 * (max - 1))) & 0x7;
}

void Player::removeOneVendingMachineTry(int max) {
	max = std::max(std::min(max, 18), 1);
	if (this->getVendingMachineTriesLeft(max) > 0) {
		if (max > 9) {
			max -= 9;
			this->vendingMachineHackTriesLeft2 -= 1 << (3 * (max - 1));
		} else {
			this->vendingMachineHackTriesLeft1 -= 1 << (3 * (max - 1));
		}
	}
}

void Player::enterTargetPractice(int tileX, int tileY, int direction, ScriptThread* targetPracticeThread) {


	this->inTargetPractice = true;
	app->canvas->targetPracticeThread = targetPracticeThread;
	app->canvas->saveX = app->canvas->viewX;
	app->canvas->saveY = app->canvas->viewY;
	app->canvas->saveZ = app->canvas->viewZ;
	app->canvas->saveAngle = app->canvas->viewAngle;
	int viewAngle;
	if (direction == 4) {
		viewAngle = Enums::ANGLE_WEST;
	} else if (direction == 0) {
		viewAngle = Enums::ANGLE_EAST;
	} else if (direction == 2) {
		viewAngle = Enums::ANGLE_NORTH;
	} else {
		viewAngle = Enums::ANGLE_SOUTH;
	}
	app->canvas->destAngle = (app->canvas->viewAngle = viewAngle);
	app->canvas->destX = (app->canvas->viewX = (tileX << 6) + 32);
	app->canvas->destY = (app->canvas->viewY = (tileY << 6) + 32);
	app->canvas->destZ = (app->canvas->viewZ = app->render->getHeight(app->canvas->viewX, app->canvas->viewY) + 36);
	this->showHelp((short)17, false);
	this->stripInventoryForTargetPractice();
	this->targetPracticeScore = 0;
	app->render->startFade(750, 2);
}

void Player::assessTargetPracticeShot(Entity* entity) {


	int sprite = entity->getSprite();
	int dx = app->canvas->zoomCollisionX - app->render->getSpriteX(sprite);
	int dy = app->canvas->zoomCollisionY - app->render->getSpriteY(sprite);
	int dz = app->canvas->zoomCollisionZ - app->render->getSpriteZ(sprite);
	int frameBaseFlags = app->render->getSpriteInfoRaw(sprite) >> 8 & 0xF0;
	int (*imageFrameBounds)[4] = app->render->getImageFrameBounds(entity->def->tileIndex, 3, 2, 0);
	int hitZone = -1;
	for (int i = 0; i < 3; ++i) {
		if (dx > imageFrameBounds[i][0] && dx < imageFrameBounds[i][1] && dy > imageFrameBounds[i][0] &&
		    dy < imageFrameBounds[i][1] && dz > imageFrameBounds[i][2] && dz < imageFrameBounds[i][3]) {
			hitZone = i;
			break;
		}
	}
	if (hitZone != -1) {
		const GameConfig& gc = *this->gameConfig;
		if (hitZone == 0 || frameBaseFlags == 16) {
			app->hud->addMessage((short)230, 4);
			this->targetPracticeScore += gc.tpHeadPoints;
			app->canvas->headShotTime = app->time + gc.tpHitDisplayMs;
			app->canvas->bodyShotTime = 0;
			app->canvas->legShotTime = 0;
		} else if (hitZone == 1) {
			app->hud->addMessage((short)231, 4);
			this->targetPracticeScore += gc.tpBodyPoints;
			app->canvas->headShotTime = 0;
			app->canvas->bodyShotTime = app->time + gc.tpHitDisplayMs;
			app->canvas->legShotTime = 0;
		} else {
			app->hud->addMessage((short)232, 4);
			this->targetPracticeScore += gc.tpLegPoints;
			app->canvas->headShotTime = 0;
			app->canvas->bodyShotTime = 0;
			app->canvas->legShotTime = app->time + gc.tpHitDisplayMs;
		}
	} else {
		app->hud->addMessage((short)68, 4);
		app->canvas->headShotTime = 0;
		app->canvas->bodyShotTime = 0;
		app->canvas->legShotTime = 0;
	}
}

void Player::exitTargetPractice() {


	this->inTargetPractice = false;
	app->canvas->destAngle = (app->canvas->viewAngle = app->canvas->saveAngle);
	app->canvas->destX = (app->canvas->viewX = app->canvas->saveX);
	app->canvas->destY = (app->canvas->viewY = app->canvas->saveY);
	app->canvas->destZ = (app->canvas->viewZ = app->canvas->saveZ);
	app->canvas->finishRotation(false);
	this->restoreInventory();
	app->render->startFade(750, 2);
	int passingScore = 240;
	app->localization->resetTextArgs();
	app->localization->addTextArg(this->targetPracticeScore);

	if (this->targetPracticeScore > passingScore / 2) {
		int modifyStat = this->modifyStat(5, 1);
		if (modifyStat > 0) {
			app->localization->addTextArg(modifyStat);
			app->hud->addMessage((short)233, 3);
		} else {
			app->hud->addMessage((short)235, 3);
		}
	} else {
		app->hud->addMessage((short)234, 3);
	}

	if (app->canvas->targetPracticeThread != nullptr) {
		app->canvas->setState(Canvas::ST_PLAYING);
		app->canvas->targetPracticeThread->run();
		app->canvas->targetPracticeThread = nullptr;
	}
}

bool Player::hasANanoDrink() {
	for (int i = 0; i < 11; ++i) {
		if (this->inventory[i] > 0) {
			return true;
		}
	}
	return false;
}


void Player::forceRemoveFromScopeZoom() {


	if (app->canvas->isZoomedIn) {
		app->canvas->zoomTurn = 0;
		app->canvas->handleZoomEvents(-6, 15, true);
	}
}
