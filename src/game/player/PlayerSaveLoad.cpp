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
#include "JavaStream.h"
#include "Sound.h"
#include "DataNode.h"
#include "SoundNames.h"
#include "Sounds.h"
#include "ItemDefs.h"

bool Player::loadState(InputStream* IS) {

	this->baseCe->loadState(IS, true);
	this->ce->loadState(IS, true);
	this->setCharacterChoice(IS->readShort());
	this->weapons = IS->readInt();
	this->weaponsCopy = IS->readInt();
	this->level = (IS->readByte() & 0xFF);
	this->currentXP = IS->readInt();
	this->nextLevelXP = this->calcLevelXP(this->level);
	this->totalTime = IS->readInt();
	this->totalMoves = IS->readInt();
	this->completedLevels = IS->readInt();
	this->killedMonstersLevels = IS->readInt();
	this->foundSecretsLevels = IS->readInt();
	this->disabledWeapons = IS->readInt();
	this->prevWeapon = IS->readByte();
	this->gamePlayedMask = IS->readInt();
	this->lastCombatTurn = IS->readInt();
	this->inCombat = IS->readBoolean();
	this->highestMap = IS->readShort();
	this->isFamiliar = IS->readBoolean();
	this->setFamiliarType(IS->readShort());
	this->playerEntityCopyIndex = IS->readInt();
	this->hackedVendingMachines = IS->readInt();
	this->vendingMachineHackTriesLeft1 = IS->readInt();
	this->vendingMachineHackTriesLeft2 = IS->readInt();
	{
		int legacyCount = IS->readInt();
		this->killGrantCounts.clear();
		if (legacyCount > 0) {
			this->killGrantCounts[Enums::WP_CHAINSAW] = legacyCount;
		}
	}
	this->lastSkipCode = IS->readByte();

	for (int i = 0; i < 9; ++i) {
		this->ammo[i] = IS->readShort();
	}
	for (int j = 0; j < 9; ++j) {
		this->ammoCopy[j] = IS->readShort();
	}
	for (int k = 0; k < 26; ++k) {
		this->inventory[k] = IS->readShort();
	}
	for (int l = 0; l < 26; ++l) {
		this->inventoryCopy[l] = IS->readShort();
	}
	this->numStatusEffects = IS->readByte();
	if (this->numStatusEffects == 0) {
		for (int n3 = 0; n3 < 18; ++n3) {
			this->statusEffects[36 + n3] = 0;
			this->statusEffects[0 + n3] = 0;
			this->statusEffects[18 + n3] = 0;
		}
	} else {
		for (int n4 = 0; n4 < 18; ++n4) {
			this->statusEffects[36 + n4] = IS->readShort();
			this->statusEffects[0 + n4] = IS->readShort();
			this->statusEffects[18 + n4] = IS->readShort();
		}
	}
	this->numStatusEffectsCopy = (this->isFamiliar ? IS->readByte() : 0);
	if (this->numStatusEffectsCopy == 0) {
		for (int n5 = 0; n5 < 18; ++n5) {
			this->statusEffectsCopy[36 + n5] = 0;
			this->statusEffectsCopy[0 + n5] = 0;
			this->statusEffectsCopy[18 + n5] = 0;
		}
	} else {
		for (int n6 = 0; n6 < 18; ++n6) {
			this->statusEffectsCopy[36 + n6] = IS->readShort();
			this->statusEffectsCopy[0 + n6] = IS->readShort();
			this->statusEffectsCopy[18 + n6] = IS->readShort();
		}
	}
	for (int n7 = 0; n7 < 8; ++n7) {
		this->counters[n7] = IS->readInt();
	}
	this->gameCompleted = IS->readBoolean();
	this->translateStatusEffects();
	this->updateStats();
	return true;
}

bool Player::saveState(OutputStream* OS) {

	this->baseCe->saveState(OS, true);
	this->ce->saveState(OS, true);
	OS->writeShort(this->characterChoice);
	OS->writeInt(this->weapons);
	OS->writeInt(this->weaponsCopy);
	OS->writeByte(this->level);
	OS->writeInt(this->currentXP);
	int gameTime = app->gameTime;
	this->totalTime += gameTime - this->playTime;
	this->playTime = gameTime;
	OS->writeInt(this->totalTime);
	OS->writeInt(this->totalMoves);
	OS->writeInt(this->completedLevels);
	OS->writeInt(this->killedMonstersLevels);
	OS->writeInt(this->foundSecretsLevels);
	OS->writeInt(this->disabledWeapons);
	OS->writeByte(this->prevWeapon);
	OS->writeInt(this->gamePlayedMask);
	OS->writeInt(this->lastCombatTurn);
	OS->writeBoolean(this->inCombat);
	OS->writeShort(this->highestMap);
	OS->writeBoolean(this->isFamiliar);
	OS->writeShort(this->familiarType);
	OS->writeInt(this->playerEntityCopyIndex);
	OS->writeInt(this->hackedVendingMachines);
	OS->writeInt(this->vendingMachineHackTriesLeft1);
	OS->writeInt(this->vendingMachineHackTriesLeft2);
	{
		auto it = this->killGrantCounts.find(Enums::WP_CHAINSAW);
		OS->writeInt(it != this->killGrantCounts.end() ? it->second : 0);
	}
	OS->writeByte(this->lastSkipCode);
	for (int i = 0; i < 9; ++i) {
		OS->writeShort(this->ammo[i]);
	}
	for (int j = 0; j < 9; ++j) {
		OS->writeShort(this->ammoCopy[j]);
	}
	for (int k = 0; k < 26; ++k) {
		OS->writeShort(this->inventory[k]);
	}
	for (int l = 0; l < 26; ++l) {
		OS->writeShort(this->inventoryCopy[l]);
	}
	OS->writeByte(this->numStatusEffects);
	if (this->numStatusEffects != 0) {
		for (int n3 = 0; n3 < 18; ++n3) {
			OS->writeShort(this->statusEffects[36 + n3]);
			OS->writeShort(this->statusEffects[0 + n3]);
			OS->writeShort(this->statusEffects[18 + n3]);
		}
	}
	if (this->isFamiliar) {
		OS->writeByte(this->numStatusEffectsCopy);
		if (this->numStatusEffectsCopy != 0) {
			for (int n4 = 0; n4 < 18; ++n4) {
				OS->writeShort(this->statusEffectsCopy[36 + n4]);
				OS->writeShort(this->statusEffectsCopy[0 + n4]);
				OS->writeShort(this->statusEffectsCopy[18 + n4]);
			}
		}
	}
	for (int n5 = 0; n5 < 8; ++n5) {
		OS->writeInt(this->counters[n5]);
	}
	OS->writeBoolean(this->gameCompleted);
	return true;
}

void Player::relink() {
	this->unlink();
	this->link();
}

void Player::unlink() {


	Entity* playerEnt = getPlayerEnt();
	if ((playerEnt->info & 0x100000) != 0x0) {
		app->game->unlinkEntity(playerEnt);
	}
}

void Player::link() {


	Entity* playerEnt = getPlayerEnt();
	if (app->canvas->destX >= 0 && app->canvas->destX <= 2047 && app->canvas->destY >= 0 &&
	    app->canvas->destY <= 2047) {
		app->game->linkEntity(playerEnt, app->canvas->destX >> 6, app->canvas->destY >> 6);
	}
}

void Player::reset() {


	app->hud->msgCount = 0;
	this->numNotebookIndexes = 0;
	this->resetCounters();
	this->level = 1;
	this->currentXP = 0;
	this->nextLevelXP = this->calcLevelXP(this->level);
	this->facingEntity = nullptr;
	this->noclip = false;
	this->questComplete = 0;
	this->questFailed = 0;
	this->isFamiliar = false;
	this->setFamiliarType((short)0);
	this->attemptingToSelfDestructFamiliar = false;
	this->inTargetPractice = false;
	this->targetPracticeScore = 0;
	this->hackedVendingMachines = 0;
	this->botReturnedDueToMonster = false;
	this->unsetFamiliarOnceOutOfCinematic = false;
	this->vendingMachineHackTriesLeft1 = 0;
	this->vendingMachineHackTriesLeft2 = 0;
	for (int i = 0; i < 9; ++i) {
		this->vendingMachineHackTriesLeft1 += 4;
		this->vendingMachineHackTriesLeft2 += 4;
		if (i < 8) {
			this->vendingMachineHackTriesLeft1 <<= 3;
			this->vendingMachineHackTriesLeft2 <<= 3;
		}
	}
	this->killGrantCounts.clear();
	this->lastSkipCode = 0;
	this->playerEntityCopyIndex = -1;
	app->canvas->viewX = (app->canvas->destX = (app->canvas->saveX = (app->canvas->prevX = 0)));
	app->canvas->viewY = (app->canvas->destY = (app->canvas->saveY = (app->canvas->prevY = 0)));
	app->canvas->viewZ = (app->canvas->destZ = 36);
	app->canvas->viewAngle = (app->canvas->destAngle = (app->canvas->saveAngle = 0));
	app->canvas->viewPitch = (app->canvas->destPitch = (app->canvas->savePitch = 0));
	this->inCombat = false;
	for (int j = 0; j < 9; ++j) {
		this->ammo[j] = 0;
	}
	for (int k = 0; k < 26; ++k) {
		this->inventory[k] = 0;
	}
	this->give(0, 18, 1, true);
	this->numbuffs = 0;
	for (int l = 0; l < 15; ++l) {
		this->buffs[15 + l] = (this->buffs[0 + l] = 0);
	}
	this->numStatusEffects = 0;
	for (int n = 0; n < 18; ++n) {
		this->statusEffects[0 + n] = 0;
		this->statusEffects[18 + n] = 0;
		this->statusEffects[36 + n] = 0;
	}
	this->numbuffsCopy = 0;
	for (int n6 = 0; n6 < 15; ++n6) {
		this->buffsCopy[0 + n6] = 0;
		this->buffsCopy[15 + n6] = 0;
	}
	this->numStatusEffectsCopy = 0;
	for (int n7 = 0; n7 < 18; ++n7) {
		this->statusEffectsCopy[0 + n7] = 0;
		this->statusEffectsCopy[18 + n7] = 0;
		this->statusEffectsCopy[36 + n7] = 0;
	}
	this->weapons = 0;
	this->foundSecretsLevels = 0;
	this->killedMonstersLevels = 0;
	{
		const GameConfig& gc = CAppContainer::getInstance()->gameConfig;
		this->baseCe->setStat(Enums::STAT_MAX_HEALTH, gc.startingMaxHealth);
		this->setStatsAccordingToCharacterChoice();
		if (app->game->difficulty == Enums::DIFFICULTY_NORMAL) {
			this->baseCe->setStat(Enums::STAT_DEFENSE, 0);
		}
		this->updateStats();
		this->ce->setStat(Enums::STAT_HEALTH, gc.startingMaxHealth);
	}
	this->baseCe->setStat(Enums::STAT_ARMOR, 0);
	this->ce->setStat(Enums::STAT_ARMOR, 0);
	this->totalTime = 0;
	this->totalMoves = 0;
	this->completedLevels = 0;
	this->highestMap = 1;
	this->gameCompleted = false;
	this->gamePlayedMask = 0;
}

void Player::updateStats() {
	this->ce->setStat(Enums::STAT_MAX_HEALTH, this->baseCe->getStat(Enums::STAT_MAX_HEALTH) + this->buffs[25]);
	this->ce->setStat(Enums::STAT_HEALTH, this->ce->getStat(Enums::STAT_HEALTH));
	this->ce->setStat(Enums::STAT_STRENGTH, this->baseCe->getStat(Enums::STAT_STRENGTH) + this->buffs[20]);
	this->ce->setStat(Enums::STAT_ACCURACY,
	                  this->baseCe->getStat(Enums::STAT_ACCURACY) + this->buffs[22] - this->buffs[28]);
	this->ce->setStat(Enums::STAT_DEFENSE, this->baseCe->getStat(Enums::STAT_DEFENSE) + this->buffs[19]);
	this->ce->setStat(Enums::STAT_AGILITY, this->baseCe->getStat(Enums::STAT_AGILITY) + this->buffs[21]);
	this->ce->setStat(Enums::STAT_IQ, this->baseCe->getStat(Enums::STAT_IQ));
}

void Player::stripInventoryForViosBattle() {
	this->weaponsCopy = 0;
	// Strip fountain weapon (holy water pistol) and save its ammo
	int fwIdx = app->combat->fountainWeaponIdx;
	int fwAmmo = app->combat->fountainAmmoType;
	if (fwIdx >= 0 && fwAmmo >= 0) {
		this->ammoCopy[fwAmmo] = this->ammo[fwAmmo];
		this->ammo[fwAmmo] = 0;
		int fwBit = 1 << fwIdx;
		if ((this->weapons & fwBit) != 0x0) {
			this->give(1, fwIdx, -1);
			this->weaponsCopy |= fwBit;
		}
	}
	// Strip soul cube weapon
	int swIdx = app->combat->soulWeaponIdx;
	if (swIdx >= 0) {
		int swBit = 1 << swIdx;
		if ((this->weapons & swBit) != 0x0) {
			this->give(1, swIdx, -1);
			this->weaponsCopy |= swBit;
		}
	}
	// Strip first found familiar weapon (sentry bot)
	for (int f = 0; f < app->combat->familiarDefCount; f++) {
		int famWpn = app->combat->familiarDefs[f].weaponIndex;
		int famBit = 1 << famWpn;
		if ((this->weapons & famBit) != 0x0) {
			this->give(1, famWpn, -1);
			this->weaponsCopy |= famBit;
			break;
		}
	}
}

void Player::stripInventoryForTargetPractice() {
	this->currentWeaponCopy = this->ce->weapon;
	std::memcpy(this->ammoCopy, this->ammo, sizeof(this->ammo));

	this->weaponsCopy = (this->weapons & -1);
	for (int i = 0; i < 9; ++i) {
		this->ammo[i] = 0;
	}
	this->weapons = 0;
	const auto& gc = CAppContainer::getInstance()->gameConfig;
	this->give(1, gc.tpWeaponIdx, 1, true);
	this->give(2, gc.tpAmmoType, gc.tpAmmoCount, true);
}

void Player::restoreInventory() {
	int fwIdx = app->combat->fountainWeaponIdx;
	int fwAmmo = app->combat->fountainAmmoType;
	int swIdx = app->combat->soulWeaponIdx;

	if ((this->weapons & 0x1) != 0x0) {
		this->currentWeaponCopy = this->ce->weapon;
		// Restore fountain weapon ammo
		if (fwIdx >= 0 && fwAmmo >= 0) {
			this->ammo[fwAmmo] = this->ammoCopy[fwAmmo];
			if ((this->weaponsCopy & (1 << fwIdx)) != 0x0) {
				this->give(1, fwIdx, 1, true);
			}
		}
		// Restore soul cube weapon
		if (swIdx >= 0 && (this->weaponsCopy & (1 << swIdx)) != 0x0) {
			this->give(1, swIdx, 1, true);
		}
		// Restore familiar weapon (preserve its ammo type to avoid give() adding extra)
		for (int f = 0; f < app->combat->familiarDefCount; f++) {
			int famWpn = app->combat->familiarDefs[f].weaponIndex;
			if ((this->weaponsCopy & (1 << famWpn)) != 0x0) {
				int famAmmoType = app->combat->weapons[famWpn * Combat::WEAPON_MAX_FIELDS + Combat::WEAPON_FIELD_AMMOTYPE];
				short savedAmmo = this->ammo[famAmmoType];
				this->give(1, famWpn, 1, true);
				this->ammo[famAmmoType] = savedAmmo;
				break;
			}
		}
		this->forceRemoveFromScopeZoom();
		this->selectWeapon(this->currentWeaponCopy);
		app->game->angryVIOS = false;
	} else {
		int tpWpn = CAppContainer::getInstance()->gameConfig.tpWeaponIdx;
		bool b = (this->weaponsCopy & (1 << tpWpn)) != 0x0;
		for (int i = 0; i < 9; ++i) {
			this->ammo[i] = 0;
			this->give(2, i, this->ammoCopy[i], true);
		}
		for (int j = 0; j < app->combat->numWeaponFlags && j < 32; ++j) {
			if ((1 << j & this->weaponsCopy) != 0x0) {
				this->give(1, j, 1, true);
			}
		}
		this->forceRemoveFromScopeZoom();
		if (!b) {
			this->give(1, tpWpn, -1);
		}
		this->selectWeapon(this->currentWeaponCopy);
	}
}
