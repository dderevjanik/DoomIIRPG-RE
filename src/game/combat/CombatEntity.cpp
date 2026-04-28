#include <stdexcept>
#include <algorithm>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "CombatEntity.h"
#include "JavaStream.h"
#include "EntityMonster.h"
#include "Entity.h"
#include "EntityDef.h"
#include "Combat.h"
#include "Player.h"
#include "Render.h"
#include "Canvas.h"
#include "Game.h"
#include "Enums.h"


CombatEntity::CombatEntity() = default;

CombatEntity::CombatEntity(int health, int armor, int defense, int strength, int accuracy, int agility) {
	//printf("CombatEntity::init2\n");
	this->app = nullptr;
	this->setStat(1, health);
	this->setStat(0, health);
	this->setStat(2, armor);
	this->setStat(3, defense);
	this->setStat(4, strength);
	this->setStat(5, accuracy);
	this->setStat(6, agility);
}

CombatEntity::~CombatEntity() {
}

void CombatEntity::clone(CombatEntity* ce) {
    ce->stats[1] = this->stats[1];
    ce->stats[2] = this->stats[2];
    ce->stats[3] = this->stats[3];
    ce->stats[4] = this->stats[4];
    ce->stats[5] = this->stats[5];
    ce->stats[6] = this->stats[6];
    ce->setStat(0, this->stats[0]);
    ce->weapon = this->weapon;
}

int CombatEntity::getStat(int i) {
    return this->stats[i];
}

int CombatEntity::getStatPercent(int i) {
    return (this->stats[i] << 8) / 100;
}

int CombatEntity::getIQPercent() {
    return std::max(0, std::min((this->stats[7] - 100) * 100 / 100, 100));
}

int CombatEntity::addStat(int i, int i2) {
    i2 += this->stats[i];
    this->setStat(i, i2);
    return this->stats[i];
}

int CombatEntity::setStat(int i, int i2) {
    if (i2 < 0) {
        i2 = 0;
    }
    switch (i) {
    case 1: {
        break;
    }
    case 0: {
        i2 = std::min(i2, this->getStat(1));
        break;
    }
    default: {
        i2 = std::min(i2, 255);
        break;
    }
    }
    return this->stats[i] = i2;
}

void CombatEntity::loadState(InputStream* inputStream, bool b) {
    if (b) {
        this->stats[0] = (int)inputStream->readShort();
        this->stats[1] = (int)inputStream->readShort();
        this->stats[2] = (int)inputStream->readByte();
        this->stats[3] = (int)inputStream->readByte();
        this->stats[4] = (int)inputStream->readByte();
        this->stats[5] = (int)inputStream->readByte();
        this->stats[6] = (int)inputStream->readByte();
        this->stats[7] = (int)inputStream->readByte();
        this->weapon = (int)inputStream->readByte();
    }
    else {
        this->stats[0] = (int)inputStream->readShort();
    }
}

void CombatEntity::saveState(OutputStream* outputStream, bool b) {
    if (b) {
        outputStream->writeShort((int16_t)this->stats[0]);
        outputStream->writeShort((int16_t)this->stats[1]);
        outputStream->writeByte((uint8_t)this->stats[2]);
        outputStream->writeByte((uint8_t)this->stats[3]);
        outputStream->writeByte((uint8_t)this->stats[4]);
        outputStream->writeByte((uint8_t)this->stats[5]);
        outputStream->writeByte((uint8_t)this->stats[6]);
        outputStream->writeByte((uint8_t)this->stats[7]);
        outputStream->writeByte((uint8_t)this->weapon);
    }
    else {
        outputStream->writeShort((int16_t)this->stats[0]);
    }
}

void CombatEntity::calcCombat(CombatEntity* combatEntity, Entity* entity, bool b, int n, int n2) {
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;

    app->combat->crDamage = 0;
    app->combat->crArmorDamage = 0;
    CombatEntity* combatEntity2;
    if (!b) {
        combatEntity2 = entity->combat;
    }
    else {
        combatEntity2 = app->player->ce;
    }
    int calcHit = this->calcHit(combatEntity, combatEntity2, b, n, false);
    if ((calcHit & 0x1007) == 0x0) {
        return;
    }
    if (!b && app->combat->getWeaponFlags(combatEntity->weapon).chainsawHitEvent) {
        app->player->onWeaponKill(combatEntity->weapon, true);
    }
    int calcDamage = this->calcDamage(combatEntity, entity, combatEntity2, b, n2);
    if (calcDamage == 0 && app->combat->crArmorDamage == 0) {
        calcHit |= 0x100;
    }
    app->combat->crFlags = calcHit;
    app->combat->crDamage = calcDamage;
}

int CombatEntity::calcHit(CombatEntity* ce, CombatEntity* ce2, bool b, int i, bool b2) {
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    int attackerWeapon = app->combat->attackerWeapon;
    int attackerWeaponId = app->combat->attackerWeaponId;
    Entity* curTarget = app->combat->curTarget;
    int eType;
    if (curTarget == nullptr) {
        eType = 1;
    }
    else {
        eType = curTarget->def->eType;
    }
    if (app->combat->oneShotCheat && !b) {
        return app->combat->crFlags |= 0x1;
    }
    
    if (app->combat->getWeaponFlags(attackerWeaponId).isScoped) {
        if (eType == Enums::ET_MONSTER) {
            int sprite = curTarget->getSprite();
            int n2 = app->canvas->zoomCollisionX - app->render->getSpriteX(sprite);
            int n3 = app->canvas->zoomCollisionY - app->render->getSpriteY(sprite);
            int n4 = app->canvas->zoomCollisionZ - app->render->getSpriteZ(sprite);
            int n5 = app->render->getSpriteInfoRaw(sprite) >> 8 & 0xF0;
            int(*imageFrameBounds)[4] = app->render->getImageFrameBounds(curTarget->def->tileIndex, 3, 2, 0);
            int v57[3][2], v56[3][2];

            std::memset(v57, 0, sizeof(v57));
            std::memset(v56, 0, sizeof(v56));

            if (curTarget->def->hasRenderFlag(EntityDef::RFLAG_TALL_HITBOX)){
                v56[0][0] -= 1;
                v56[0][1] += 60;
            }

            int n6 = -1;
            for (int i = 0; i < 3; ++i) {
                if (n2 > (imageFrameBounds[i][0] + v57[i][0]) &&
                    n2 < (imageFrameBounds[i][1] + v57[i][1]) &&
                    n3 > (imageFrameBounds[i][0] + v57[i][0]) &&
                    n3 < (imageFrameBounds[i][1] + v57[i][1]) &&
                    n4 > (imageFrameBounds[i][2] + v56[i][0]) &&
                    n4 < (imageFrameBounds[i][3] + v56[i][1])) {
                    n6 = i;
                    break;
                }
            }
            if (n6 != -1) {
                if (n6 == 0 || n5 == 16) {
                    app->combat->crFlags |= 0x2;
                }
                else if (n6 == 1) {
                    app->combat->crFlags |= 0x1;
                }
                else {
                    app->combat->crFlags |= 0x5;
                }
            }
        }
        return app->combat->crFlags;
    }

    bool b3 = false;
    int worldDistToTileDist = app->combat->WorldDistToTileDist(i);
    int n7;
    if (worldDistToTileDist < app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMIN]) {
        n7 = app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMIN] - worldDistToTileDist;
    }
    else if (worldDistToTileDist > app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMAX]) {
        if (app->combat->getWeaponFlags(attackerWeaponId).outOfRangeStillFires) {
            b3 = true;
        }
        n7 = worldDistToTileDist - app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMAX];
    }
    else {
        n7 = 0;
    }
    if ((app->combat->crFlags & 0x10) != 0x0) {
        n7 = 0;
    }
    else if ((app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMIN] == app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMAX] || (app->combat->crFlags & 0x40) != 0x0) && n7 > 0) {
        return app->combat->crFlags |= 0x400;
    }
    if (b2 || app->combat->getWeaponFlags(ce->weapon).alwaysHits) {
        return app->combat->crFlags |= 0x1;
    }
    int stat = ce->getStat(Enums::STAT_ACCURACY);
    int stat2 = ce2->getStat(Enums::STAT_AGILITY);
    if (!app->combat->getWeaponFlags(attackerWeaponId).fullAgility) {
        stat2 = stat2 * 96 >> 8;
    }
    app->combat->crHitChance = (stat - stat2 << 8) / 100;
    app->combat->crHitChance -= 16 * n7;
    if (app->combat->crHitChance < 1) {
        app->combat->crHitChance = 1;
    }
    int nextByte = app->nextByte();
    if (app->combat->punchingMonster != 0) {
        if (app->combat->animLoopCount == 1 || !app->combat->punchMissed) {
            nextByte = 0;
        }
        else if (app->combat->punchMissed) {
            app->combat->playerMissRepetition = 0;
            nextByte = 255;
        }
    }
    int n8 = 1;
    if (((!b && app->combat->playerMissRepetition < n8) || (b && app->combat->monsterMissRepetition < 2)) && nextByte > app->combat->crHitChance && (b || app->canvas->loadMapID < 8 || app->combat->tileDist > 1)) {
        if (b) {
            ++app->combat->monsterMissRepetition;
        }
        else {
            ++app->combat->playerMissRepetition;
        }
        return app->combat->crFlags;
    }
    if (b && app->player->statusEffects[18 + app->player->reflectBuffIdx] > 0) {
        if (--app->player->statusEffects[18 + app->player->reflectBuffIdx] == 0) {
            app->player->removeStatusEffect(app->player->reflectBuffIdx);
        }
        app->player->translateStatusEffects();
        app->combat->crFlags |= 0x100;
    }
    if (b) {
        app->combat->monsterMissRepetition = 0;
    }
    else {
        app->combat->playerMissRepetition = 0;
    }
    if (b3) {
        return app->combat->crFlags |= 0x4;
    }
    if (!b || app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE) {
        app->combat->crCritChance = app->combat->crHitChance / 20;
    }
    else {
        app->combat->crCritChance = 0;
    }
    if (app->nextByte() < app->combat->crCritChance) {
        return app->combat->crFlags |= 0x2;
    }
    return app->combat->crFlags |= 0x1;
}

int CombatEntity::calcDamage(CombatEntity* ce, Entity* entity, CombatEntity* ce2, bool b, int n) {
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    int weapon = ce->weapon * 9;
    int dmgStrMin = app->combat->weapons[weapon + Combat::WEAPON_FIELD_STRMIN] & 0xFF;
    int dmgStrMax = app->combat->weapons[weapon + Combat::WEAPON_FIELD_STRMAX] & 0xFF;

    if (app->combat->getWeaponFlags(ce->weapon).doubleDamage) {
        dmgStrMin *= 2;
        dmgStrMax *= 2;
    }

    if (b && app->player->buffs[app->player->reflectBuffIdx] > 0) {
        return app->combat->crDamage = dmgStrMax;
    }
    if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE) {
        dmgStrMin -= dmgStrMin >> 2;
    }
    if ((app->combat->crFlags & 0x2) != 0x0 || (app->combat->crFlags & 0x2000) != 0x0) {
        dmgStrMin = dmgStrMax * 2;
    }
    else if ((app->combat->crFlags & 0x4) != 0x0) {
        dmgStrMin = dmgStrMax / 2;
    }
    else if (dmgStrMax != dmgStrMin) {
        dmgStrMin += app->nextByte() % (dmgStrMax - dmgStrMin);
    }
    if ((app->combat->crFlags & 0x20) == 0x0) {
        if (!b) {
            dmgStrMin += 3 * (ce->getStatPercent(Enums::STAT_STRENGTH) * dmgStrMin >> 8);
        }
        else {
            dmgStrMin += ce->getStatPercent(Enums::STAT_STRENGTH) * dmgStrMin >> 8;
        }
    }
    int armorDamage = 0;
    int damage;
    if (!b) {
        if (app->player->buffs[app->player->angerBuffIdx] > 0) {
            dmgStrMin += (app->player->buffs[15 + app->player->angerBuffIdx] << 8) / 100 * dmgStrMin >> 8;
        }
        int weaponWeakness = app->combat->getWeaponWeakness(ce->weapon, entity->def->monsterIdx);
        if (ce->weapon >= 0 && ce->weapon < MonsterBehaviors::MAX_WEAKNESS_MODS) {
            int8_t mod = app->combat->monsterBehaviors[entity->def->monsterIdx].weaknessMods[ce->weapon];
            if (mod == -1) {
                weaponWeakness = 0;
            } else if (mod > 0) {
                int shift = (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE) ? std::max(1, (int)mod - 1) : mod;
                weaponWeakness <<= shift;
            }
        }
        damage = weaponWeakness * dmgStrMin >> 8;
    }
    else {
        if (!app->player->isFamiliar) {
            armorDamage = std::min(((171 * dmgStrMin >> 8) + 1) / 2, ce2->getStat(Enums::STAT_ARMOR));
            damage = dmgStrMin - 2 * armorDamage;
        }
        else if (app->player->familiarType == 1 || app->player->familiarType == 2) {
            damage = dmgStrMin * 7 / 8;
        }
        else {
            damage = dmgStrMin * 5 / 8;
        }
        if (app->combat->getWeaponFlags(ce->weapon).blockableByShield && app->player->buffs[app->player->antifireBuffIdx] > 0) {
            armorDamage = 0;
            damage = 0;
        }
    }
    int crDamage = damage - (ce2->getStatPercent(Enums::STAT_DEFENSE) * damage >> 8);
    if (app->combat->oneShotCheat && !b) {
        crDamage = 999;
    }
    app->combat->crArmorDamage = armorDamage;
    return app->combat->crDamage = crDamage;
}
