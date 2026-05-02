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
#include "BinaryStream.h"
#include "Sound.h"
#include "DataNode.h"
#include "SoundNames.h"
#include "Sounds.h"
#include "ItemDefs.h"
#include "EventBus.h"
#include "GameEvents.h"

bool Player::addHealth(int amount) {
	return this->addHealth(amount, true);
}

bool Player::addHealth(int amount, bool announce) {


	app->hud->repaintFlags |= 0x4;
	int stat;
	int stat2;
	if (this->isFamiliar) {
		stat = this->ammo[app->combat->familiarAmmoType];
		stat2 = this->gameConfig->familiarMaxHealth;
	} else {
		stat = this->ce->getStat(Enums::STAT_HEALTH);
		stat2 = this->ce->getStat(Enums::STAT_MAX_HEALTH);
	}
	if (amount > 0) {
		if (stat == stat2) {
			return false;
		}
	} else if (this->god) {
		return false;
	}
	if (this->isFamiliar) {
		this->ammo[app->combat->familiarAmmoType] = (short)std::max(0, this->ammo[app->combat->familiarAmmoType] + amount);
	} else {
		app->hud->playerStartHealth = stat;
		this->ce->addStat(Enums::STAT_HEALTH, amount);
	}
	int newHealth = this->isFamiliar ? this->ammo[app->combat->familiarAmmoType] : this->ce->getStat(Enums::STAT_HEALTH);
	if (announce && newHealth > stat) {
		app->localization->resetTextArgs();
		app->localization->addTextArg(newHealth - stat);
		app->hud->addMessage((short)111);
	}
	if (!this->isFamiliar) {
		if (amount > 0) {
			app->eventBus->emit(PlayerHealEvent{amount, stat, newHealth});
		} else if (amount < 0) {
			app->eventBus->emit(PlayerDamageEvent{-amount, nullptr, stat, newHealth});
		}
	}
	return true;
}

bool Player::addArmor(int amount) {


	int stat = this->ce->getStat(2);
	if (stat >= this->gameConfig->capArmor && amount > 0) {
		return false;
	}
	if (amount < 0 && this->god) {
		return false;
	}
	app->hud->repaintFlags |= 0x4;
	this->ce->setStat(Enums::STAT_ARMOR, std::max(0, std::min(this->gameConfig->capArmor, stat + amount)));
	return true;
}

int Player::calcDamageDir(int x1, int y1, int angle, int x2, int y2) {
	int dx = x2 - x1;
	int dy = y2 - y1;
	angle &= Enums::ANGLE_FULL;
	if (dx == 0 && dy == 0) {
		return Enums::DIR_NORTHWEST;
	}
	int ang;
	if (dx > 0) {
		if (dy < 0) {
			ang = Enums::ANGLE_NORTHEAST;
		} else if (dy > 0) {
			ang = Enums::ANGLE_SOUTHEAST;
		} else {
			ang = Enums::ANGLE_EAST;
		}
	} else if (dx < 0) {
		if (dy < 0) {
			ang = Enums::ANGLE_NORTHWEST;
		} else if (dy > 0) {
			ang = Enums::ANGLE_SOUTHWEST;
		} else {
			ang = Enums::ANGLE_WEST;
		}
	} else if (dy > 0) {
		ang = Enums::ANGLE_SOUTH;
	} else {
		ang = Enums::ANGLE_NORTH;
	}

	int dAng = (ang - angle) & Enums::ANGLE_FULL;
	if (dAng > Enums::ANGLE_PI) {
		dAng = -(Enums::ANGLE_2PI - dAng);
	}
	int dir = (dAng / Enums::ANGLE_PI_FOURTH) + Enums::DIR_NORTHWEST;
	if (dir < 0) {
		dir = Enums::DIR_SOUTHEAST;
	}
	return dir;
}

void Player::painEvent(Entity* entity, bool fromMonster) {


	if (entity == nullptr) {
		app->hud->damageDir = 3;
	}

	if (this->isFamiliar) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::SENTRYBOT_PAIN), 0, 3, false);
	} else {
		if (this->characterChoice == 1) {
			app->sound->playSound(Sounds::getResIDByName(SoundName::PLAYER_PAIN2), 0, 3, false);
		} else {
			app->sound->playSound(Sounds::getResIDByName(SoundName::PLAYER_PAIN1), 0, 3, false);
		}
	}

	app->hud->damageCount = 1;
	if (!fromMonster) {
		app->canvas->startShake(500, 2, 150);
	} else { // [GEC] Damage From Monster
		app->canvas->startShake(0, 0, 150);
	}
}

void Player::pain(int damage, Entity* entity, bool announce) {

	LOG_INFO("[player] pain: dmg={} hp={}/{}\n", damage, this->ce->getStat(Enums::STAT_HEALTH), this->ce->getStat(Enums::STAT_MAX_HEALTH));
	if (this->god) {
		return;
	}
	if (announce) {
		app->localization->resetTextArgs();
		if (entity != nullptr) {
			Text* smallBuffer = app->localization->getSmallBuffer();
			app->localization->composeText((short)1, (short)(entity->def->name & 0x3FF), smallBuffer);
			app->localization->addTextArg(smallBuffer);
			app->localization->addTextArg(damage);
			app->hud->addMessage((short)0, (short)112);
			smallBuffer->dispose();
		} else {
			app->localization->addTextArg(damage);
			app->hud->addMessage((short)0, (short)71);
		}
	}
	if (damage == 0) {
		return;
	}
	if (this->isFamiliar) {
		this->addHealth(-damage);
	} else {
		int stat = this->ce->getStat(Enums::STAT_HEALTH);
		if (damage >= stat && this->noDeathFlag) {
			damage = stat - 1;
		}
		int oldHealthFrac = (stat << 16) / (this->ce->getStat(Enums::STAT_MAX_HEALTH) << 8);
		int newHealthFrac = (stat - damage << 16) / (this->ce->getStat(Enums::STAT_MAX_HEALTH) << 8);
		if (newHealthFrac > 0) {
			if (oldHealthFrac > 26 && newHealthFrac <= 26) {
				app->hud->addMessage((short)113, 3);
			} else if (oldHealthFrac > 78 && newHealthFrac <= 78) {
				app->hud->addMessage((short)114, 3);
			} else if (oldHealthFrac > 128 && newHealthFrac <= 128 && (this->helpBitmask & 0xC00) == 0x0) {
				if (this->inventory[17] != 0 || this->inventory[16] != 0) {
					this->showHelp((short)10, true);
				} else {
					this->showHelp((short)11, true);
				}
			}
		}
		this->addHealth(-damage);
	}
	if (app->canvas->state == Canvas::ST_AUTOMAP) {
		app->canvas->setState(Canvas::ST_PLAYING);
	}
	if (this->ce->getStat(Enums::STAT_HEALTH) <= 0) {
		this->died();
	}
}

void Player::died() {

	LOG_INFO("[player] died: totalDeaths={} mapId={}\n", this->totalDeaths + 1, app->canvas->loadMapID);
	if (app->canvas->state == Canvas::ST_DYING) {
		return;
	}
	this->currentLevelDeaths++;
	this->totalDeaths++;

	if (this->characterChoice == 1) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::PLAYER_DEATH3), 0, 3, false);
	} else if (this->characterChoice == 2) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::PLAYER_DEATH2), 0, 3, false);
	} else if (this->characterChoice == 3) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::PLAYER_DEATH1), 0, 3, false);
	}

	app->canvas->startShake(350, 5, 500);
	this->ce->setStat(Enums::STAT_HEALTH, 0);
	app->canvas->setState(Canvas::ST_DYING);
	app->game->combatMonsters = nullptr;
}

bool Player::fireWeapon(Entity* entity, int attackX, int attackY) {


	if (this->ce->weapon == Enums::WP_SOUL_CUBE && !entity->isMonster()) {
		return false;
	}

	if (app->combat->weaponDown || (this->disabledWeapons != 0 && (this->weapons & 1 << this->ce->weapon) == 0x0)) {
		return false;
	}

	if (app->combat->lerpingWeapon) {
		if (app->combat->lerpWpDown) {
			return false;
		}
		app->combat->lerpingWeapon = false;
		app->combat->weaponDown = false;
	}

	if (entity->isMonster()) {
		entity->monsterFlags &= ~Enums::MFLAG_NOACTIVATE;
	}

	if (app->combat->getWeaponFlags(this->ce->weapon).chainsawHitEvent &&
	    (entity->def->eType == Enums::ET_CORPSE || (entity->def->eType == Enums::ET_ATTACK_INTERACTIVE &&
	                                                (entity->def->eSubType == Enums::INTERACT_BARRICADE ||
	                                                 entity->def->eSubType == Enums::INTERACT_FURNITURE)))) {
		this->onWeaponKill(this->ce->weapon, false);
	}

	int weaponFieldOffset = this->ce->weapon * Combat::WEAPON_MAX_FIELDS;
	if (app->combat->weapons[weaponFieldOffset + Combat::WEAPON_FIELD_AMMOTYPE] != 0) {
		short currentAmmo = this->ammo[app->combat->weapons[weaponFieldOffset + Combat::WEAPON_FIELD_AMMOTYPE]];
		if (app->combat->weapons[weaponFieldOffset + Combat::WEAPON_FIELD_AMMOUSAGE] > 0 &&
		    (currentAmmo - app->combat->weapons[weaponFieldOffset + Combat::WEAPON_FIELD_AMMOUSAGE]) < 0) {
			if (this->ce->weapon == Enums::WP_SOUL_CUBE) {
				app->hud->addMessage((short)117, 3);
			} else if (currentAmmo == 0) {
				app->hud->addMessage((short)115, 3);
			} else {
				app->hud->addMessage((short)116, 3);
			}
			return false;
		}
	}
	app->combat->performAttack(nullptr, entity, attackX, attackY, false);
	return true;
}

void Player::onWeaponKill(int weaponIdx, bool bonusHit) {
	const auto& flags = app->combat->getWeaponFlags(weaponIdx);
	if (flags.onKillGrantKills <= 0) {
		app->canvas->startShake(666, 1, 0);
		return;
	}

	this->killGrantCounts[weaponIdx]++;
	if (bonusHit && (app->combat->crFlags & 0x2) != 0x0) {
		this->killGrantCounts[weaponIdx]++;
	}
	if (this->killGrantCounts[weaponIdx] >= flags.onKillGrantKills) {
		this->killGrantCounts[weaponIdx] %= flags.onKillGrantKills;
		int modifyStat = this->modifyStat(4, flags.onKillGrantStrength);
		if (modifyStat != 0) {
			app->localization->resetTextArgs();
			app->localization->addTextArg(modifyStat);
			app->hud->addMessage((short)241, 3);
		}
	}
	app->canvas->startShake(666, 1, 0);
}
