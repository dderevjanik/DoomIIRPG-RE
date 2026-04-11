#include <stdexcept>
#include <algorithm>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "GLES.h"
#include "Combat.h"
#include "CombatEntity.h"
#include "Entity.h"
#include "EntityMonster.h"
#include "EntityDef.h"
#include "Player.h"
#include "Game.h"
#include "Canvas.h"
#include "Render.h"
#include "Hud.h"
#include "Text.h"
#include "Enums.h"
#include "ParticleSystem.h"
#include "Sound.h"
#include "SoundNames.h"
#include "Sounds.h"

Combat::Combat() {
	// Zero all POD members. The map is default-constructed (empty).
	// We skip the map by zeroing in two ranges around it.
	char* thisPtr = reinterpret_cast<char*>(this);
	char* mapStart = reinterpret_cast<char*>(&this->monsterNameToIndex);
	char* mapEnd = mapStart + sizeof(this->monsterNameToIndex);
	char* objEnd = thisPtr + sizeof(Combat);
	std::memset(thisPtr, 0, mapStart - thisPtr);
	if (mapEnd < objEnd) {
		std::memset(mapEnd, 0, objEnd - mapEnd);
	}
}

Combat::~Combat() {
}

short Combat::getWeaponWeakness(int n, int n2, int n3) {
    return (short)((this->monsterWeakness[(n2 * this->tiersPerMonster + n3) * 8 + n / 2] >> ((n & 0x1) << 2) & 0xF) + 1 << 5);
}

bool Combat::startup() {
	this->app = CAppContainer::getInstance()->app;
	Applet* app = this->app;
	LOG_INFO("[combat] startup\n");

	this->monsters = new CombatEntity*[this->monsterSlotCount];
	for (int n = 0, i = 0; i < this->monsterSlotCount; ++i, n += 6) {
		this->monsters[i] = new CombatEntity(5 * (this->monsterStats[n + 0] & 0xFF), this->monsterStats[n + 1], this->monsterStats[n + 2], this->monsterStats[n + 3], this->monsterStats[n + 4], this->monsterStats[n + 5]);
	}

	for (int j = 0; j < 16; j++) {
		this->tileDistances[j] = 64 * (j + 1) * (64 * (j + 1));
		//printf("this->tileDistances[%d] %d\n", j, this->tileDistances[j]);
	}

	this->gotCrit = false;
	this->gotHit = false;
	this->settingDynamite = false;
	this->oneShotCheat = false;

	return true;
}

void Combat::performAttack(Entity* curAttacker, Entity* curTarget, int attackX, int attackY, bool b) {

    LOG_INFO("[combat] performAttack: attacker=%s target=%s at (%d,%d)\n",
        curAttacker ? "monster" : "player",
        curTarget ? "monster" : "player",
        attackX, attackY);
    this->attackX = attackX;
    this->attackY = attackY;
    this->curAttacker = curAttacker;
    this->curTarget = curTarget;
    this->accumRoundDamage = 0;
    this->field_0x110_ = -1;
    if (curAttacker == nullptr || curTarget == nullptr) {
        app->player->updateStats();
    }
    if (curAttacker != nullptr) {
        curAttacker->info |= 0x200000;
    }
    if (curTarget != nullptr) {
        curTarget->info |= 0x200000;
    }
    if (curTarget == nullptr || (curAttacker == nullptr && curTarget->def->eType == Enums::ET_MONSTER)) {
        app->player->lastCombatTurn = app->player->totalMoves;
        app->player->inCombat = true;
    }
    if (curTarget != nullptr) {
        this->targetType = curTarget->def->eType;
        this->targetSubType = curTarget->def->eSubType;
        this->targetMonster = curTarget->monster;
    }
    else {
        this->targetType = (this->targetSubType = 0);
        this->targetMonster = nullptr;
    }
    if (this->curAttacker == nullptr) {
        this->attackerWeaponId = app->player->ce->weapon;
        this->attackerMonster = nullptr;
    }
    else {
        this->attackerMonster = this->curAttacker->monster;
        this->attackerWeaponId = this->attackerMonster->ce.weapon;
        if (this->getWeaponFlags(this->attackerWeaponId).chargeAttack) {
            this->attackFrame = 64;
        }
        else if (this->attackerWeaponId == this->getMonsterField(this->curAttacker->def, 1)) {
            this->attackFrame = 80;
        }
        else {
            this->attackFrame = 64;
        }
    }
    this->attackerWeapon = this->attackerWeaponId * 9;
    if (this->punchingMonster > 0 && !this->punchMissed) {
        this->stage = -1;
        this->nextStage = 0;
        this->nextStageTime = app->gameTime + 300;
    }
    else {
        this->stage = 0;
        this->nextStageTime = 0;
    }
    this->animEndTime = 0;
    this->animLoopCount = this->weapons[this->attackerWeapon + 7];
    if (this->curAttacker == nullptr) {
        short n = app->player->ammo[this->weapons[this->attackerWeapon + 4]];
        int8_t b2 = this->weapons[this->attackerWeapon + 5];
        if (b2 > 0) {
            this->animLoopCount = std::min(n / b2, this->animLoopCount);
        }
    }
    this->attackerWeaponProj = this->weapons[this->attackerWeapon + 6];
    if (this->curAttacker != nullptr) {
        this->worldDist = this->curAttacker->distFrom(app->canvas->viewX, app->canvas->viewY);
        this->tileDist = this->WorldDistToTileDist(this->worldDist);
    }
    else {
        this->worldDist = this->curTarget->distFrom(app->canvas->viewX, app->canvas->viewY);
        this->tileDist = this->WorldDistToTileDist(this->worldDist);
    }
    short* mapSprites = app->render->mapSprites;
    if (this->getWeaponFlags(this->attackerWeaponId).chargeAttack) {
        int sprite = this->curAttacker->getSprite();
        int n2 = mapSprites[app->render->S_X + sprite] - app->canvas->viewX;
        int n3 = mapSprites[app->render->S_Y + sprite] - app->canvas->viewY;
        int a = n2;
        int a2 = n3;
        if (a != 0) {
            a /= std::abs(a);
        }
        if (a2 != 0) {
            a2 /= std::abs(a2);
        }
        int n4 = std::abs(n2 + n3) >> 6;
        int n5 = app->canvas->viewX + a * 64;
        int n6 = app->canvas->viewY + a2 * 64;
        LerpSprite* allocLerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
        allocLerpSprite->startTime = app->gameTime;
        allocLerpSprite->travelTime = n4 * 200;
        allocLerpSprite->flags |= Enums::LS_FLAG_ENT_NORELINK;
        allocLerpSprite->srcScale = allocLerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
        allocLerpSprite->srcX = mapSprites[app->render->S_X + sprite];
        allocLerpSprite->srcY = mapSprites[app->render->S_Y + sprite];
        allocLerpSprite->srcZ = mapSprites[app->render->S_Z + sprite];
        allocLerpSprite->dstX = n5 - a * 28;
        allocLerpSprite->dstY = n6 - a2 * 28;
        allocLerpSprite->dstZ = app->render->getHeight(allocLerpSprite->dstX, allocLerpSprite->dstY) + 32;
        app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | this->attackFrame << 8);
        this->curAttacker->monster->frameTime = 0x7FFFFFFF;//Integer.MAX_VALUE;
        app->game->unlinkEntity(this->curAttacker);
        app->game->linkEntity(this->curAttacker, n5 >> 6, n6 >> 6);
        this->stage = -1;
        this->nextStage = 0;
        this->nextStageTime = app->gameTime + allocLerpSprite->travelTime;
    }
    app->canvas->setState(Canvas::ST_COMBAT);
}

void Combat::checkMonsterFX() {

    EntityMonster* monster = this->curTarget->monster;
    int monsterEffects = monster->monsterEffects;
    if (app->player->statusEffects[12] > 0) {
        monsterEffects |= (app->player->statusEffects[30] - 1 << 5 | 0x1);
    }
    monster->monsterEffects = monsterEffects;
}

int Combat::playerSeq() {

    int animLoopCount = 0;
    bool b = false;
    if (this->nextStageTime != 0 && app->gameTime > this->nextStageTime && this->numActiveMissiles == 0 && app->game->animatingEffects == 0) {
        this->stage = this->nextStage;
        this->nextStageTime = 0;
        this->nextStage = -1;
        this->field_0x110_ = -1;
    }
    if (this->stage == 0) {
        this->isGibbed = false;
        this->totalDamage = 0;
        this->totalArmorDamage = 0;
        this->hitType = 0;
        this->deathAmt = 0;
        this->gotCrit = false;
        this->gotHit = false;
        this->crFlags = 0;
        this->damage = 0;
        this->targetKilled = false;
        this->flashTime = 1; // Old -> 0
        ++app->player->counters[6];
        if (this->punchingMonster == 1) {
            this->punchingMonster = 2;
        }
        if (this->getWeaponFlags(this->attackerWeaponId).autoHit || this->attackerWeaponId >= this->numWeaponFlags) {
            this->crFlags |= 0x40;
        }
        if (this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOTYPE] != 0) {
            b = true;
            if (this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOUSAGE] != 0) {
                animLoopCount = app->player->ammo[this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOTYPE]] / this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOUSAGE];
            }
        }
        if (b && this->targetType != 2 && !this->getWeaponFlags(this->attackerWeaponId).isThrowableItem && !app->player->weaponIsASentryBot(this->attackerWeaponId)) {
            if (animLoopCount == 2) {
                app->hud->addMessage((short)62);
            }
            else if (animLoopCount < 5 && animLoopCount > 1) {
                app->localization->resetTextArgs();
                app->localization->addTextArg(animLoopCount - 1);
                app->hud->addMessage((short)63);
            }
        }
        if (this->targetType == 2) {
            if (!this->getWeaponFlags(this->attackerWeaponId).isMelee) {
                this->crFlags |= 0x20;
            }
            app->player->ce->calcCombat(app->player->ce, this->curTarget, false, this->worldDist, this->targetSubType);
            if ((this->crFlags & 0x1007) != 0x0) {
                this->curTarget->info |= 0x4000000;
                if ((this->crFlags & 0x2) != 0x0) {
                    this->gotCrit = true;
                    this->hitType = 2;
                }
                else if ((this->crFlags & 0x1) != 0x0 || (this->crFlags & 0x4) != 0x0) {
                    this->hitType = 1;
                }
                this->gotHit = true;
                this->damage = this->crDamage;
                this->deathAmt = this->targetMonster->ce.getStat(0) - this->damage;
                if (this->damage > 0 && this->gotHit) {
                    app->player->counters[2] += this->damage;
                    if (this->damage > app->player->counters[1]) {
                        app->player->counters[1] = this->damage;
                    }
                }
            }
        }
        else {
            this->hitType = calcHit(this->curTarget);
            int damage = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_STRMIN];
            int damageMax = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_STRMAX];
            if (damage != damageMax) {
                damage += app->nextByte() % (damageMax - damage);
            }
            if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE) {
                damage -= damage >> 2;
            }
            this->damage = damage;
        }
        int n3 = -1;
        if (this->wpAttackSoundAlt != nullptr && this->wpAttackSoundAlt[this->attackerWeaponId] != -1 &&
            app->player->characterChoice == 1) {
            n3 = this->wpAttackSoundAlt[this->attackerWeaponId];
        } else if (this->wpAttackSound != nullptr) {
            n3 = this->wpAttackSound[this->attackerWeaponId];
        }
        if (n3 != -1) {
            app->sound->playCombatSound(n3, 0, 4);
        }
        this->totalDamage += this->damage;
        this->totalArmorDamage += this->crArmorDamage;
        this->animTime = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_SHOTHOLD];
        if (app->player->statusEffects[2] > 0) {
            this->animTime *= 5;
        }
        else {
            this->animTime *= 10;
        }
        this->animStartTime = app->gameTime;
        this->animEndTime = this->animStartTime + this->animTime;
        this->flashDone = false;
        this->flashDoneTime = this->animStartTime + this->flashTime;
        if (b && animLoopCount < this->animLoopCount) {
            this->animLoopCount = animLoopCount;
        }
        this->launchProjectile();
        if (!this->getWeaponFlags(this->attackerWeaponId).noRecoil) {
            int n4 = 2;
            if (this->getWeaponFlags(this->attackerWeaponId).autoHit) {
                n4 = 6;
            }
            app->render->rockView(this->animTime, app->canvas->viewX - n4 * (app->canvas->viewStepX >> 6), app->canvas->viewY - n4 * (app->canvas->viewStepY >> 6), app->canvas->viewZ);
        }
        if (this->totalDamage == 0) {
            if (this->targetType == 2) {
                if ((this->crFlags & 0x100) != 0x0) {
                    app->hud->addMessage((short)59, 1);
                }
                else if ((this->crFlags & 0x400) != 0x0) {
                    app->hud->addMessage((short)64);
                }
                else if ((this->crFlags & 0x1) == 0x0) {
                    app->hud->addMessage((short)68);
                }
            }
        }
        else if (this->targetType == 2) {
            this->accumRoundDamage += this->totalDamage;
        }
        else if (this->hitType != 0) {
            this->targetKilled = true;
        }
        this->stage = -1;
        app->hud->repaintFlags |= 0x4;
        //app->hud->repaintFlags |= 0x40; //J2ME
        int ammoUsage = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOUSAGE];
        int ammoType = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOTYPE];
        app->player->ammo[ammoType] -= ammoUsage;
        app->player->ammo[ammoType] = (short)std::max((int)app->player->ammo[ammoType], 0);
        this->nextStage = 1;
        this->nextStageTime = this->animEndTime;
        this->updateProjectile();
        app->canvas->invalidateRect();
        if (this->totalDamage == 0 || this->hitType == 0) {
            ++app->player->counters[7];
        }
    }
    else {
        if (this->stage == 1) {
            this->settingDynamite = false;
            this->dynamitePlaced = false;
            app->canvas->invalidateRect();
            if (this->targetType == 2) {
                this->curTarget->info &= 0xFBFFFFFF;
            }
            if (this->targetType == 9 && this->targetSubType == 17) {
                int sprite = this->curTarget->getSprite();
                app->game->executeTile(app->render->mapSprites[app->render->S_X + sprite] >> 6, app->render->mapSprites[app->render->S_Y + sprite] >> 6, 0xFF4 | app->canvas->flagForWeapon(this->attackerWeaponId), true);
            }
            if (this->attackerWeaponProj == 5) {
                this->checkForBFGDeaths(this->attackX >> 6, this->attackY >> 6);
            }
            int n6 = 4385;
            if (this->targetKilled || (this->targetType == 2 && this->targetMonster->ce.getStat(0) <= 0)) {
                this->curTarget->died(true, app->player->getPlayerEnt());
                this->targetKilled = true;
            }
            else if (--this->animLoopCount > 0 && (1 << this->targetType & n6) == 0x0 && !this->punchMissed && this->targetType != 10) {
                this->stage = 0;
                this->animEndTime = (this->animTime = 0);
                this->nextStageTime = 0;
                return 1;
            }
            if (this->punchingMonster > 0 && !this->targetKilled && !this->punchMissed) {
                int sprite = this->curTarget->getSprite();
                if (sprite != -1 && this->curTarget->monster != nullptr) {
                    app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x0);
                }
            }
            this->punchingMonster = 0;
            this->punchMissed = false;
            if (this->targetType == 2 && this->accumRoundDamage != 0 && (this->curTarget->info & 0x20000) != 0x0) {
                Text* messageBuffer = app->hud->getMessageBuffer();
                app->localization->resetTextArgs();
                if (this->gotCrit) {
                    app->localization->composeText((short)0, (short)70, messageBuffer);
                }
                app->localization->addTextArg(this->accumRoundDamage);
                app->localization->composeText((short)0, (short)71, messageBuffer);
                app->hud->finishMessageBuffer();
            }
            if (this->getWeaponFlags(this->attackerWeaponId).consumeOnUse) {
                app->player->weapons &= ~(1 << this->attackerWeaponId);
                app->player->selectWeapon(app->player->currentWeaponCopy);
            }
            return 0;
        }
        if (this->stage == -1) {
            app->canvas->staleView = true;
            if (this->numActiveMissiles != 0) {
                this->updateProjectile();
            }
        }
    }
    return 1;
}

int Combat::monsterSeq() {

    int sprite = this->curAttacker->getSprite();
    int n = app->render->mapSpriteInfo[sprite];
    int n2 = (n & 0xFF00) >> 8;
    int n3 = n2 & 0xF0;
    int n4 = n2 & 0xF;
    if ((n3 == 64 || n3 == 80) && n4 == 0) {
        if (app->time >= this->curAttacker->monster->frameTime) {
            this->curAttacker->monster->frameTime = app->time + this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_SHOTHOLD] * 10;
            this->nextStageTime = app->gameTime + this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_SHOTHOLD] * 10;
            app->render->mapSpriteInfo[sprite] = ((n & 0xFFFF00FF) | (n3 | n4 + 1) << 8);
            this->launchProjectile();
        }
    }
    else if (this->nextStageTime != 0 && app->gameTime > this->nextStageTime && this->numActiveMissiles == 0 && app->game->animatingEffects == 0) {
        if (this->reflectionDmg > 0 && this->curAttacker != nullptr && this->curAttacker->monster != nullptr) {
            app->particleSystem->spawnMonsterBlood(this->curAttacker, false);
            this->curAttacker->pain(this->reflectionDmg / 2, app->player->getPlayerEnt());
            this->reflectionDmg = 0;
            if (this->curAttacker->monster->ce.getStat(Enums::STAT_HEALTH) <= 0) {
                this->curAttacker->died(true, app->player->getPlayerEnt());
            }
            else {
                this->curAttacker->monster->frameTime = app->time + 250;
                this->nextStageTime = app->gameTime + 250;
            }
        }
        else {
            this->stage = this->nextStage;
            this->nextStageTime = 0;
            this->nextStage = -1;
        }
        this->field_0x110_ = -1;
    }
    if (this->stage == 0) {
        this->totalDamage = 0;
        this->totalArmorDamage = 0;
        this->hitType = 0;
        this->crFlags = 0;
        this->gotCrit = false;
        this->gotHit = false;
        this->targetKilled = false;
        this->reflectionDmg = 0;
#if 0 // J2ME
        if (this->curAttacker->def->eSubType == Enums::MONSTER_SENTRY_BOT || (this->curAttacker->def->eSubType == Enums::BOSS_MASTERMIND && this->curAttacker->def->parm != 0)) {
            Sound.playSound(15);
        }
        else {
            Sound.playSound(5);
        }
#endif
        int soundType;
        if (this->attackerWeaponId == this->getMonsterField(this->curAttacker->def, 1)) {
            soundType = Enums::MSOUND_ATTACK2;
        }
        else {
            soundType = Enums::MSOUND_ATTACK1;
        }

        if (this->weapons[this->attackerWeapon + 1] > 0) {
            if (this->curTarget == nullptr) {
                app->player->ce->calcCombat(&this->attackerMonster->ce, app->player->getPlayerEnt(), true, app->player->distFrom(this->curAttacker), -1);
                if ((this->crFlags & 0x1007) != 0x0) {
                    if ((this->crFlags & 0x2) != 0x0) {
                        this->gotCrit = true;
                        this->hitType = 2;
                    }
                    else {
                        this->hitType = 1;
                    }
                    this->gotHit = true;
                    this->damage = this->crDamage;
                }
                else {
                    this->damage = 0;
                }
            }
            else {
                this->hitType = 1;
                this->damage = 1;
            }
            this->totalDamage += this->damage;
            this->totalArmorDamage += this->crArmorDamage;
        }
        int monsterSound;
        const auto& atkWpFlags = this->getWeaponFlags(this->attackerWeaponId);
        if (atkWpFlags.missSoundOverride != -1 && !(this->crFlags & Enums::CR_HITMASK)) {
            monsterSound = atkWpFlags.missSoundOverride;
        }
        else if (atkWpFlags.attackSoundOverride != -1) {
            monsterSound = atkWpFlags.attackSoundOverride;
        }
        else {
            monsterSound = app->game->getMonsterSound(this->curAttacker->def->eSubType, this->curAttacker->def->parm, soundType);
        }
        app->sound->playCombatSound(monsterSound, 0, 4);

        --this->animLoopCount;
        this->animTime = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_SHOTHOLD] * 10;
        this->animStartTime = app->gameTime;
        this->animEndTime = this->animStartTime + this->animTime;
        if (!this->getWeaponFlags(this->attackerWeaponId).chargeAttack) {
            app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | this->attackFrame << 8);
            this->curAttacker->monster->frameTime = app->time + this->animTime;
        }
        if (this->curTarget != nullptr) {
            this->targetKilled = true;
        }
        if (this->getWeaponFlags(this->attackerWeaponId).chargeAttack) {
            LerpSprite* lerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
            lerpSprite->startTime = app->gameTime;
            lerpSprite->travelTime = 500;
            lerpSprite->flags |= Enums::LS_FLAG_ENT_NORELINK;
            lerpSprite->srcScale = lerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
            lerpSprite->srcX = app->render->mapSprites[app->render->S_X + sprite];
            lerpSprite->srcY = app->render->mapSprites[app->render->S_Y + sprite];
            lerpSprite->srcZ = app->render->mapSprites[app->render->S_Z + sprite];
            lerpSprite->dstX = (this->curAttacker->linkIndex % 32 << 6) + 32;
            lerpSprite->dstY = (this->curAttacker->linkIndex / 32 << 6) + 32;
            lerpSprite->dstZ = app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + 32;
            lerpSprite->calcDist();
            app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x0);
            this->curAttacker->monster->frameTime = app->time + lerpSprite->travelTime;
            this->launchProjectile();
        }
        MonsterBehaviors& atkBeh = app->combat->monsterBehaviors[this->curAttacker->def->eSubType];
        if (this->gotHit && (this->getWeaponFlags(this->attackerWeaponId).poisonOnHit || atkBeh.onHitPoison)) {
            app->player->addStatusEffect(atkBeh.onHitPoison ? atkBeh.onHitPoisonId : 13, atkBeh.onHitPoison ? atkBeh.onHitPoisonDuration : 5, atkBeh.onHitPoison ? atkBeh.onHitPoisonPower : 3);
            app->player->translateStatusEffects();
        }
        this->stage = -1;
        if (this->animLoopCount <= 0) {
            this->nextStage = 1;
        }
        else {
            this->nextStage = 0;
        }
        this->nextStageTime = this->animEndTime;
        if (this->getWeaponFlags(this->attackerWeaponId).flipSpriteOnEnd && this->animLoopCount == 0) {
            app->render->mapSpriteInfo[sprite] ^= 0x20000;
        }
        this->updateProjectile();
        if ((this->crFlags & 0x80) != 0x0) {
            app->hud->addMessage((short)67);
        }
        app->canvas->invalidateRect();
    }
    else {
        if (this->stage == 1) {
            if (this->curAttacker->def->eType == Enums::ET_MONSTER) {
                app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x0);
            }
            if (this->targetKilled) {
                this->curTarget->died(false, nullptr);
            }
            app->localization->resetTextArgs();
            if (this->accumRoundDamage > 0 && app->player->buffs[0] > 0 && !this->curAttacker->isBoss()) {
                app->localization->addTextArg(this->accumRoundDamage);
                app->hud->addMessage((short)0, (short)131);
            }
            else if (this->accumRoundDamage > 0) {
                app->localization->addTextArg((short)1, (short)(this->curAttacker->def->name & 0x3FF));
                app->localization->addTextArg(this->accumRoundDamage);
                app->hud->addMessage((short)0, (short)112);
            }
            app->canvas->shakeTime = 0;
            app->hud->damageDir = 0;
            app->hud->damageTime = 0;
            this->attackerMonster->flags |= 0x400;
            app->game->gsprite_clear(64);
            app->canvas->invalidateRect();
            return 0;
        }
        if (this->stage == -1) {
            app->canvas->staleView = true;
            if (this->numActiveMissiles != 0 || this->exploded) {
                this->updateProjectile();
            }
        }
    }
    if (app->canvas->state == Canvas::ST_DYING || app->canvas->state == Canvas::ST_BOT_DYING) {
        return 0;
    }
    return 1;
}

void Combat::drawEffects() {

    if (app->player->statusEffects[13] > 0) {
        int n = 131072;
        int n2 = ((176 * n) / 65536);
        app->render->draw2DSprite(234, app->time / 256 & 0x3, (app->canvas->viewRect[2] / 2) - (n2 / 2), app->canvas->viewRect[3] - ((n2 / 4) + 132), 0, 3, 0, n);
    }
}

void Combat::drawWeapon(int sx, int sy) {

    bool b = false;
    int frame = 0;
    int renderFlags = 0;
    int flags = 0;
    int scrX = (Applet::IOS_WIDTH / 2) - 44;
    int scrY = (Applet::IOS_HEIGHT / 2) - 29;
    int loweredWeaponY= Combat::LOWEREDWEAPON_Y;

    if (!app->render->_gles->isInit) {  // [GEC] Adjusted like this to match the XY position on the GL version
        scrX = (app->render->screenWidth / 2) - 62;
        scrY = (app->render->screenHeight / 2) - 18;
        loweredWeaponY += 13;
        renderFlags |= Render::RENDER_FLAG_SCALE_WEAPON; // [GEC]
        scrX += this->wpSwOffsetX[app->player->ce->weapon];
        scrY += this->wpSwOffsetY[app->player->ce->weapon];
    }

    this->renderTime = app->upTimeMs;
    if (this->punchingMonster == 0 && !this->lerpingWeapon && this->weaponDown) {
        scrY += loweredWeaponY;
    }

    bool b2 = app->canvas->state == Canvas::ST_CAMERA && app->game->cinematicWeapon != -1;
    int weapon = app->player->ce->weapon;

    scrY += this->wpDisplayOffsetY[weapon];

    if (app->player->isFamiliar) {
        const Combat::FamiliarDef* fd = app->combat->getFamiliarDefByType(app->player->familiarType);
        if (!fd || !fd->canShoot) {
            return;
        }
        weapon = 0;
    }
    if (b2) {
        weapon = app->game->cinematicWeapon;
        scrY -= app->canvas->CAMERAVIEW_BAR_HEIGHT;
    }
    int wpnField = weapon * Combat::FIELD_COUNT;
    if (app->canvas->state == Canvas::ST_DYING || app->player->weapons == 0 || weapon == -1) {
        return;
    }
    sy = -std::abs(sy);
    if (!b2 && app->time < app->hud->damageTime && app->hud->damageCount >= 0 && this->totalDamage > 0) {
        renderFlags |= Render::RENDER_FLAG_BRIGHTREDSHIFT;
    }

    int wpIdleX = this->wpinfo[wpnField + Combat::FLD_WPIDLEX];
    int wpIdleY = this->wpinfo[wpnField + Combat::FLD_WPIDLEY];
    int wpAtkX = this->wpinfo[wpnField + Combat::FLD_WPATKX];
    int wpAtkY = this->wpinfo[wpnField + Combat::FLD_WPATKY];

    const Combat::FamiliarDef* fd2 = app->combat->getFamiliarDefByType(app->player->familiarType);
    if (app->player->isFamiliar && fd2 && fd2->canShoot) {
        wpAtkX = 1 + (wpAtkX - wpIdleX);
        wpAtkY = 50 + (wpAtkY - wpIdleY);
        wpIdleX = 1;
        wpIdleY = 50;
    }
    int wpFlashX = this->wpinfo[wpnField + Combat::FLD_WPFLASHX];
    int wpFlashY = this->wpinfo[wpnField + Combat::FLD_WPFLASHY];

    int wpX = wpIdleX;
    int wpY = wpIdleY;
    int gameTime = app->gameTime;
    bool b5 = b2 || (app->canvas->state == Canvas::ST_COMBAT && this->curAttacker == nullptr && this->nextStage == 1);
    bool b6 = false;
    if (b5) {
        wpX = wpAtkX;
        wpY = wpAtkY;
        if (!this->flashDone) {
            b = !this->getWeaponFlags(weapon).drawDoubleSprite;
            if (gameTime >= this->flashDoneTime) {
                this->flashDone = true;
            }
        }
        else {
            int n15 = (gameTime - this->animStartTime << 16) / this->animTime;
            if (n15 >= 65536) {
                wpX = wpIdleX;
                wpY = wpIdleY;
            }
            else if (this->getWeaponFlags(weapon).vibrateAnim) {
                if (n15 < 43690) {
                    wpX = wpAtkX + ((n15 & 0x8) ?  2 : 0);
                    wpY = wpAtkY + ((n15 & 0x8) ? -1 : 0);
                }
                else {
                    b6 = true;
                    wpX = 3 * ((65536 - n15) * wpAtkX + (n15 - 43690) * wpIdleX) >> 16;
                    wpY = 3 * ((65536 - n15) * wpAtkY + (n15 - 43690) * wpIdleY) >> 16;
                }
            }
            else {
                wpX = (65536 - n15) * wpAtkX + n15 * wpIdleX >> 16;
                wpY = (65536 - n15) * wpAtkY + n15 * wpIdleY >> 16;
            }
        }
    }
    else if (this->punchingMonster == 0 && this->lerpingWeapon) {
        if (this->lerpWpDown && this->lerpWpStartTime + this->lerpWpDur > gameTime) {
            scrY += (gameTime - this->lerpWpStartTime << 16) / this->lerpWpDur * loweredWeaponY >> 16;
        }
        else if (!this->lerpWpDown && this->lerpWpStartTime + this->lerpWpDur > gameTime) {
            scrY += (65536 - (gameTime - this->lerpWpStartTime << 16) / this->lerpWpDur) * loweredWeaponY >> 16;
        }
        else {
            this->lerpWpStartTime = 0;
            this->lerpingWeapon = false;
            this->weaponDown = this->lerpWpDown;
            if (this->lerpWpDown) {
                scrY += loweredWeaponY;
            }
        }
    }
    int renderMode = Render::RENDER_NORMAL;
    int x = scrX + (wpX + sx);
    int y = scrY - (wpY + sy);

    app->render->_gles->SetGLState();
    if (app->player->isFamiliar) {
        y -= 35;
        if (!app->render->_gles->isInit) {  // [GEC] Adjusted like this to match the XY position on the GL version
            x += 8;
            y -= 7;
        }
    }
    if (app->player->weaponIsASentryBot(weapon)) {
        int n21 = this->getWeaponTileNum(weapon);
        int n22 = flags;
        int n23 = (app->time + n21 * 1337) / 1024 & 0xF;
        if (n23 == 0 || (n23 >= 4 && n23 <= 7) || (n23 >= 10 && n23 <= 12)) {
            n22 ^= 0x20000;
        }
        int n24 = (n23 & 0x2) ? 1 : 2;
        app->render->draw2DSprite(n21, 2, x, y, flags, renderMode, renderFlags, 0x10000);
        app->render->draw2DSprite(n21, 3, x, y + n24, n22, renderMode, renderFlags, 0x10000);
    }
    else if (this->getWeaponFlags(weapon).isThrowableItem) {
        if (app->player->ammo[this->weapons[weapon * Combat::WEAPON_MAX_FIELDS + Combat::WEAPON_FIELD_AMMOTYPE]] > 0) {
            app->render->draw2DSprite(app->player->activeWeaponDef->tileIndex, 1, x + wpFlashX, y + wpFlashY, flags, renderMode, renderFlags, 0x10000);
        }
        else {
            app->render->draw2DSprite(this->getWeaponTileNum(weapon), 0, x, y, flags, renderMode, renderFlags, 0x10000);
        }
    }
    else {
        if (b6 | (this->getWeaponFlags(weapon).showFlashFrame && b5)) {
            frame = 1;
        }
        if (b && this->getWeaponFlags(weapon).hasFlashSprite) {
            int xf = 40;
            int yf = 40;
            if (!app->render->_gles->isInit) {  // [GEC] Adjusted like this to match the XY position on the GL version
                xf += 15;
                yf += 20;
            }
            app->render->draw2DSprite(this->getWeaponTileNum(0), 3, x + wpFlashX + xf, y + wpFlashY + yf, flags, 5, (!app->render->_gles->isInit) ? 0x400 : 0, 0x8000);
        }
        if (this->getWeaponFlags(weapon).drawDoubleSprite) {
            app->render->draw2DSprite(this->getWeaponTileNum(0), frame, x, y, flags, renderMode, renderFlags, 0x10000);
        }

        app->render->draw2DSprite(this->getWeaponTileNum(weapon), frame, x, y, flags, renderMode, renderFlags, 0x10000);
    }
    app->render->_gles->ResetGLState();
    this->drawEffects();
    this->renderTime = app->upTimeMs - this->renderTime;
}

void Combat::shiftWeapon(bool lerpWpDown) {

    if (lerpWpDown == this->lerpWpDown || lerpWpDown == this->weaponDown) {
        return;
    }
    this->lerpingWeapon = true;
    this->lerpWpDown = lerpWpDown;
    this->lerpWpStartTime = app->gameTime;
    this->lerpWpDur = Combat::LOWERWEAPON_TIME;
}

int Combat::runFrame() {
    if (this->curAttacker == nullptr) {
        return this->playerSeq();
    }
    return this->monsterSeq();
}

int Combat::calcHit(Entity* entity) {

    int n = app->player->ce->weapon * 9;
    int worldDistToTileDist = this->WorldDistToTileDist(entity->distFrom(app->canvas->destX, app->canvas->destY));
    int8_t b = this->weapons[n + 3];
    if (worldDistToTileDist < this->weapons[n + 2] || worldDistToTileDist > b) {
        this->crFlags |= 0x400;
        return 0;
    }
    if ((entity->info & 0x20000) == 0x0) {
        return 0;
    }
    if (entity->def->eType != Enums::ET_ATTACK_INTERACTIVE) {
        return 1;
    }
    if ((this->tableCombatMasks[entity->def->parm] & 1 << app->player->ce->weapon) != 0x0) {
        return 1;
    }
    return 0;
}

void Combat::explodeOnMonster() {

    LOG_INFO("[combat] explodeOnMonster: weaponId=%d dmg=%d hit=%d\n", this->attackerWeaponId, this->totalDamage, this->hitType);
    if (this->explodeThread != nullptr) {
        this->explodeThread->run();
        this->explodeThread = nullptr;
    }
    app->render->shotsFired = true;
    if (this->getWeaponFlags(this->attackerWeaponId).isMelee && this->hitType == 0) {
        app->render->shotsFired = false;
    }
    else if (this->curTarget->monster != nullptr && this->curTarget->def->eType == Enums::ET_MONSTER && (this->curTarget->info & 0x40000) == 0x0) {
        app->game->activate(this->curTarget, true, false, true, true);
    }
    if (this->hitType == 0) {
        if (this->targetType != 2 && this->getWeaponFlags(this->attackerWeaponId).splashDamage) {
            this->radiusHurtEntities(this->attackX >> 6, this->attackY >> 6, 1, this->damage + this->crArmorDamage >> 2, app->player->getPlayerEnt(), nullptr);
        }
        return;
    }
    if (this->targetType == 2) {
        if (this->totalDamage > 0) {
            this->checkMonsterFX();
            bool pain = this->curTarget->pain(this->totalDamage, app->player->getPlayerEnt());
            if (!this->getWeaponFlags(this->attackerWeaponId).noBloodOnHit) {
                app->particleSystem->spawnMonsterBlood(this->curTarget, false);
            }
            if (this->targetMonster->ce.getStat(Enums::STAT_HEALTH) > 0) {
                int n = 0;
                if (0x0 != (this->targetMonster->monsterEffects & 0x2) || (this->targetMonster->flags & 0x1000) != 0x0 || pain) {
                    n = 0;
                }
                if (n > 0) {
                    app->player->unlink();
                    this->curTarget->knockback(app->canvas->viewX, app->canvas->viewY, n);
                    app->player->link();
                }
            }
            {
                const auto& wf = this->getWeaponFlags(this->attackerWeaponId);
                if (wf.splashDamage && wf.splashDivisor > 0) {
                    this->radiusHurtEntities(this->attackX >> 6, this->attackY >> 6, 1, this->crDamage / wf.splashDivisor, app->player->getPlayerEnt(), this->curTarget);
                }
            }
        }
        else if (this->totalDamage < 0) {
            if (this->targetMonster->ce.getStat(Enums::STAT_HEALTH) - this->totalDamage > this->targetMonster->ce.getStat(Enums::STAT_MAX_HEALTH)) {
                this->totalDamage = -(this->targetMonster->ce.getStat(Enums::STAT_MAX_HEALTH) - this->targetMonster->ce.getStat(Enums::STAT_HEALTH));
            }
            this->targetMonster->ce.setStat(Enums::STAT_HEALTH, this->targetMonster->ce.getStat(Enums::STAT_HEALTH) - this->totalDamage);
        }
    }
    else if (this->targetType == 10) {
        if (this->totalDamage > 0) {
            this->curTarget->pain(this->totalDamage, app->player->getPlayerEnt());
        }
    }
    else if (this->targetType == 9) {
        int sprite = this->curTarget->getSprite();
        app->render->mapSpriteInfo[sprite] |= 0x10000;
        this->isGibbed = true;
        app->particleSystem->spawnMonsterBlood(this->curTarget, this->isGibbed);
        app->game->spawnDropItem(this->curTarget);
        app->sound->playSound(Sounds::getResIDByName(SoundName::GIB), 0, 3, 0);
    }
}

void Combat::explodeOnPlayer() {

    LOG_INFO("[combat] explodeOnPlayer: dmg=%d armorDmg=%d hit=%d\n", this->totalDamage, this->totalArmorDamage, this->gotHit);
    if (this->curTarget != nullptr) {
        return;
    }
    int sprite = this->curAttacker->getSprite();
    int viewAngle = app->canvas->viewAngle;
    if (app->canvas->isZoomedIn) {
        viewAngle += app->canvas->zoomAngle;
    }
    app->hud->damageDir = app->player->calcDamageDir(app->canvas->viewX, app->canvas->viewY, viewAngle, app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite]);
    if (this->animLoopCount > 0) {
        app->hud->damageTime = app->time + 150;
    }
    else {
        app->hud->damageTime = app->time + 1000;
    }
    if (app->hud->damageDir != 3) {
        app->canvas->staleTime = app->hud->damageTime + 1;
    }
    bool b = false;
    if (app->player->buffs[0] > 0 && !this->curAttacker->isBoss()) {
        b = true;
    }
    if (this->gotHit && (this->totalDamage > 0 || this->totalArmorDamage > 0)) {
        app->player->painEvent(this->curAttacker, true);
        if (this->curAttacker != nullptr) {
            Entity* entity = &app->game->entities[1];
            app->game->unlinkEntity(this->curAttacker);
            if (this->attackerWeaponProj == 7) {
                app->player->addStatusEffect(13, 5, 3);
                app->player->translateStatusEffects();
            }
            app->player->addArmor(-this->totalArmorDamage);
            if (this->totalDamage > 0) {
                if (!b) {
                    if (app->game->difficulty != Enums::DIFFICULTY_EASY) {
                        int loadMapID = app->canvas->loadMapID;
                        if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE) {
                            loadMapID *= 2;
                        }
                        this->totalDamage += (this->totalDamage >> 1) + loadMapID / this->weapons[this->attackerWeapon + 7];
                    }
                    EntityDef* def = this->curAttacker->def;
                    if (def->eType == Enums::ET_MONSTER && this->monsterBehaviors[def->eSubType].isVios) {
                        if (def->parm == 4) {
                            this->totalDamage += app->game->numMallocsForVIOS * 8;
                        }
                        else if (app->game->angryVIOS) {
                            this->totalDamage += 8;
                        }
                    }
                    this->accumRoundDamage += this->totalDamage;
                    app->player->pain(this->totalDamage, this->curAttacker, false);
                    if (app->player->ce->getStat(Enums::STAT_HEALTH) == 0) {
                        return;
                    }
                    if (app->canvas->knockbackDist == 0) {
                        int* calcPosition = this->curAttacker->calcPosition();
                        int a = app->canvas->viewX - calcPosition[0];
                        int a2 = app->canvas->viewY - calcPosition[1];
                        if (a != 0) {
                            a /= std::abs(a);
                        }
                        if (a2 != 0) {
                            a2 /= std::abs(a2);
                        }
                        app->render->rockView(200, app->canvas->viewX + a * 6, app->canvas->viewY + a2 * 6, app->canvas->viewZ);
                    }
                    if (app->player->ce->getStat(Enums::STAT_HEALTH) > 0 && app->combat->monsterBehaviors[this->curAttacker->def->eSubType].knockbackWeaponId >= 0 && app->combat->monsterBehaviors[this->curAttacker->def->eSubType].knockbackWeaponId == this->attackerWeaponId) {
                        entity->knockback(app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite], 1);
                    }
                }
                else if (this->curAttacker->monster != nullptr) {
                    this->accumRoundDamage = (this->reflectionDmg = this->totalDamage);
                    this->animLoopCount = 0;
                    this->nextStage = 1;
                }
            }
            else {
                app->hud->damageCount = 0;
                this->totalDamage = 0;
            }
            if (this->totalDamage <= 0) {
                app->hud->addMessage((short)72);
            }
            app->game->linkEntity(this->curAttacker, this->curAttacker->linkIndex % 32, this->curAttacker->linkIndex / 32);
            if (app->player->isFamiliar && app->player->ammo[this->familiarAmmoType] <= 0) {
                app->player->familiarDying(false);
            }
        }
    }
    else {
        app->hud->damageCount = 0;
        if ((this->crFlags & 0x100) != 0x0) {
            app->localization->resetTextArgs();
            app->hud->addMessage((short)73);
        }
        else if ((this->crFlags & 0x80) == 0x0) {
            app->localization->resetTextArgs();
            Text* smallBuffer = app->localization->getSmallBuffer();
            app->localization->composeTextField(this->curAttacker->name, smallBuffer);
            app->localization->addTextArg(smallBuffer);
            app->hud->addMessage((short)69);
            smallBuffer->dispose();
        }
        app->hud->damageCount = -1;
    }
}

int Combat::getMonsterField(EntityDef* entityDef, int n) {
    return this->monsterAttacks[(entityDef->eSubType * 9) + (entityDef->parm * 3) + n];
}

void Combat::checkForBFGDeaths(int x, int y) {

    int n3 = (x << 6) + 32;
    int n4 = (y << 6) + 32;
    int n5 = app->render->getHeight(x << 6, y << 6) + 32;
    for (int i = y - 1; i <= y + 1; ++i) {
        for (int j = x - 1; j <= x + 1; ++j) {
            if (j != x || i != y) {
                app->game->trace(n3, n4, n5, (j << 6) + 32, (i << 6) + 32, app->render->getHeight(j << 6, i << 6) + 32, nullptr, 4129, 1, false);
                if (app->game->traceEntity == nullptr) {
                    Entity* nextOnTile;
                    for (Entity* mapEntity = app->game->findMapEntity(j << 6, i << 6, 30381); mapEntity != nullptr; mapEntity = nextOnTile) {
                        nextOnTile = mapEntity->nextOnTile;
                        if (mapEntity->monster != nullptr && mapEntity->monster->ce.getStat(0) <= 0 && mapEntity->def->eType != Enums::ET_CORPSE) {
                            mapEntity->died(true, app->player->getPlayerEnt());
                        }
                    }
                }
            }
        }
    }
}

void Combat::radiusHurtEntities(int n, int n2, int n3, int n4, Entity* entity, Entity* entity2) {

    int n5 = (n << 6) + 32;
    int n6 = (n2 << 6) + 32;
    int n7 = app->render->getHeight(n << 6, n2 << 6) + 32;
    for (int i = n2 - 1; i <= n2 + 1; ++i) {
        for (int j = n - 1; j <= n + 1; ++j) {
            app->game->trace(n5, n6, n7, (j << 6) + 32, (i << 6) + 32, app->render->getHeight(j << 6, i << 6) + 32, nullptr, 4129, 1, true);
            if (app->game->traceEntity == nullptr) {
                this->hurtEntityAt(j, i, n, n2, (j == n && i == n2) ? 0 : n3, n4, entity, entity2);
            }
        }
    }
    for (int k = n2 - 1; k <= n2 + 1; ++k) {
        for (int l = n - 1; l <= n + 1; ++l) {
            int n8 = 0x4004 | app->game->eventFlagForDirection(n - l, n2 - k);
            if (app->game->doesScriptExist(l, k, n8)) {
                ScriptThread* allocScriptThread = app->game->allocScriptThread();
                allocScriptThread->queueTile(l, k, n8);
                allocScriptThread->run();
            }
        }
    }
}

void Combat::hurtEntityAt(int n, int n2, int n3, int n4, int n5, int n6, Entity* entity, Entity* entity2) {
    this->hurtEntityAt(n, n2, n3, n4, n5, n6, entity, false, entity2);
}

void Combat::hurtEntityAt(int n, int n2, int n3, int n4, int n5, int n6, Entity* entity, bool b, Entity* entity2) {

    this->crFlags = 16;
    app->render->shotsFired = true;
    Entity* playerEnt = app->player->getPlayerEnt();
    Entity* nextOnTile;
    for (Entity* mapEntity = app->game->findMapEntity(n << 6, n2 << 6, 30383); mapEntity != nullptr; mapEntity = nextOnTile) {
        nextOnTile = mapEntity->nextOnTile;
        if (mapEntity != entity2 && (mapEntity->info & 0x20000) != 0x0) {
            if (mapEntity == playerEnt) {
                if (this->attackerWeaponProj != 5) {
                    n6 >>= 1;
                    int min = std::min(n6 * 10 / 100, app->player->ce->getStat(Enums::STAT_ARMOR));
                    n6 -= min;
                    n6 -= n6 * app->player->ce->getStat(Enums::STAT_DEFENSE) / 100;
                    app->player->painEvent(nullptr, false);
                    playerEnt->knockback((n3 << 6) + 32, (n4 << 6) + 32, n5);
                    if (n6 > 0) {
                        this->crArmorDamage = min;
                        app->player->pain(n6, nullptr, true);
                        app->player->addArmor(-this->crArmorDamage);
                        app->player->addStatusEffect(13, 5, 3);
                        app->player->translateStatusEffects();
                    }
                }
            }
            else if (mapEntity->def->eType == Enums::ET_CORPSE) {
                if (!b) {
                    return;
                }
                mapEntity->died(false, entity);
                mapEntity->info |= 0x10000;
                app->particleSystem->spawnMonsterBlood(mapEntity, true);
            }
            else if (mapEntity->monster != nullptr) {
                mapEntity->info |= 0x200000;
                if ((mapEntity->info & 0x40000) == 0x0) {
                    app->game->activate(mapEntity, true, false, true, true);
                }
                int n7 = this->getWeaponWeakness(this->attackerWeaponId, mapEntity->def->eSubType, mapEntity->def->parm) * n6 >> 8;
                int n8 = n7 - (mapEntity->monster->ce.getStatPercent(Enums::STAT_DEFENSE) * n7 >> 8);
                if (n8 > 0) {
                    if (this->attackerWeaponProj == 5) {
                        int sprite = mapEntity->getSprite();
                        int sX = app->render->mapSprites[app->render->S_X + sprite];
                        int sY = app->render->mapSprites[app->render->S_Y + sprite];
                        int sZ = app->render->mapSprites[app->render->S_Z + sprite];
                        GameSprite* gSprite = app->game->gsprite_allocAnim(244, sX, sY, sZ + (sZ >> 1));
                        gSprite->flags |= 0x800;
                        this->nextStageTime = app->gameTime + gSprite->duration;
                        app->render->mapSprites[app->render->S_RENDERMODE + gSprite->sprite] = 4;
                        app->render->mapSprites[app->render->S_SCALEFACTOR + gSprite->sprite] = 32;
                    }
                    bool pain = mapEntity->pain(n8, nullptr);
                    if ((mapEntity->monster->flags & 0x1000) == 0x0 && !pain) {
                        mapEntity->knockback((n3 << 6) + 32, (n4 << 6) + 32, n5);
                    }
                    app->localization->resetTextArgs();
                    app->localization->addTextIDArg(mapEntity->name);
                    app->localization->addTextArg(n8);
                    if (mapEntity->monster->ce.getStat(Enums::STAT_HEALTH) <= 0) {
                        app->hud->addMessage((short)74);
                        if (this->attackerWeaponProj != 5) {
                            if (b && entity != nullptr && entity->def->eType == Enums::ET_DECOR_NOCLIP && entity->def->eSubType == Enums::DECOR_DYNAMITE) {
                                mapEntity->info |= 0x10000;
                                app->particleSystem->spawnMonsterBlood(mapEntity, true);
                            }
                            mapEntity->died(true, entity);
                        }
                    }
                    else {
                        app->hud->addMessage((short)75);
                    }
                }
            }
            else if (mapEntity->def->eType != Enums::ET_ENV_DAMAGE) {
                if (mapEntity->def->eType == Enums::ET_ATTACK_INTERACTIVE) {}
                mapEntity->pain(n6, entity);
                mapEntity->died(true, entity);
            }
        }
    }
}

Text* Combat::getWeaponStatStr(int n) {

    int n2 = n * Combat::WEAPON_MAX_FIELDS;
    Text* largeBuffer = app->localization->getLargeBuffer();
     app->localization->resetTextArgs();
     app->localization->addTextArg(this->weapons[n2 + Combat::WEAPON_FIELD_STRMIN]);
     app->localization->composeText((short)0, (short)77, largeBuffer);
     app->localization->resetTextArgs();
    if (this->weapons[n2 + Combat::WEAPON_FIELD_RANGEMAX] != 1) {
         app->localization->addTextArg(this->weapons[n2 + Combat::WEAPON_FIELD_RANGEMIN]);
         app->localization->addTextArg(this->weapons[n2 + Combat::WEAPON_FIELD_RANGEMAX]);
         app->localization->composeText((short)0, (short)78, largeBuffer);
    }
    else {
         app->localization->addTextArg(this->weapons[n2 + Combat::WEAPON_FIELD_RANGEMAX]);
         app->localization->composeText((short)0, (short)79, largeBuffer);
    }
    EntityDef* find = app->entityDefManager->find(6, 2, this->weapons[n2 + Combat::WEAPON_FIELD_AMMOTYPE]);
    if (find != nullptr) {
         app->localization->composeText((short)0, (short)76, largeBuffer);
         app->localization->composeText((short)1, find->name, largeBuffer);
    }
    return largeBuffer;
}

Text* Combat::getArmorStatStr(int n) {

    Text* largeBuffer = app->localization->getLargeBuffer();
    if (n != -1) {
        app->localization->resetTextArgs();
        app->localization->addTextArg(app->player->ce->getStat(Enums::STAT_ARMOR));
        app->localization->composeText((short)0, (short)80, largeBuffer);
    }
    app->localization->composeText((short)0, (short)81, largeBuffer);
    return largeBuffer;
}

int Combat::WorldDistToTileDist(int n) {
	for (int i = 0; i < (Combat::MAX_TILEDISTANCES - 1); ++i) {
		if (n < this->tileDistances[i]) {
			return i;
		}
	}
	return (Combat::MAX_TILEDISTANCES - 1);
}

void Combat::cleanUpAttack() {
    this->stage = 1;
    this->animLoopCount = 0;
}

void Combat::updateProjectile() {

    if (this->numActiveMissiles > 0) {
        int renderMode = 0;
        for (int i = 0; i < this->numActiveMissiles; ++i) {
            GameSprite* actMissile = this->activeMissiles[i];
            if ((actMissile->flags & 0x2) != 0x0) {
                if (this->missileAnim == 248) {
                    app->render->mapSpriteInfo[actMissile->sprite] ^= 0x20000;
                }
                else if (this->missileAnim == 171) {
                    int sprite = this->curAttacker->getSprite();
                    int x1 = app->render->mapSprites[app->render->S_X + sprite];
                    int y1 = app->render->mapSprites[app->render->S_Y + sprite];
                    int x2 = app->game->viewX;
                    int y2 = app->game->viewY;
                    int dx = 0;
                    int dy = 0;
                    if (std::abs(y2 - y1) > std::abs(x2 - x1)) {
                        dy = (((y2 - y1) >> 31) << 1) + 1;
                    }
                    else {
                        dx = (((x2 - x1) >> 31) << 1) + 1;
                    }
                    int x = x1 + this->numThornParticleSystems * dx * 64 + dx * 32;
                    int y = y1 + this->numThornParticleSystems * dy * 64 + dy * 32;
                    if (std::abs(app->render->mapSprites[app->render->S_X + actMissile->sprite] - x) <= 4 && std::abs(app->render->mapSprites[app->render->S_Y + actMissile->sprite] - y) <= 4) {
                        int height = app->render->getHeight(x, y);
                        app->particleSystem->spawnParticles(1, 0xFF81603D, x, y, height);
                        GameSprite* gSprite = app->game->gsprite_allocAnim(170, x, y, height + 32);
                        app->render->mapSprites[app->render->S_SCALEFACTOR + gSprite->sprite] = 48;
                        gSprite->duration += actMissile->duration - (app->gameTime - actMissile->time);
                        ++this->numThornParticleSystems;
                    }
                    app->canvas->repaintFlags |= Canvas::REPAINT_PARTICLES;
                }
            }
            else {
                bool b = true;
                int scaleFactor = 64;
                int x = actMissile->pos[3];
                int y = actMissile->pos[4];
                int z = actMissile->pos[5];
                // Load impact data from projectile visual table
                if (this->projVisuals != nullptr && this->attackerWeaponProj >= 0 &&
                    this->attackerWeaponProj < this->numProjTypes) {
                    const auto& pv = this->projVisuals[this->attackerWeaponProj];
                    this->missileAnim = pv.impactAnim;
                    renderMode = pv.impactRenderMode;
                    if (pv.impactScreenShake) {
                        app->canvas->startShake(pv.shakeDuration, pv.shakeIntensity, pv.shakeFade);
                    }
                    if (pv.impactSound != -1) {
                        app->sound->playSound(pv.impactSound, 0, 4, 0);
                    }
                } else {
                    this->missileAnim = 0;
                }

                // Data-driven projectile impact behaviors
                if (this->projVisuals != nullptr && this->attackerWeaponProj >= 0 &&
                    this->attackerWeaponProj < this->numProjTypes) {
                    const auto& pv = this->projVisuals[this->attackerWeaponProj];
                    if (pv.causesFear && this->curTarget != nullptr && (this->crFlags & 0x1007) != 0x0) {
                        EntityMonster* monster = this->curTarget->monster;
                        if (monster != nullptr && !app->combat->monsterBehaviors[this->curTarget->def->eSubType].fearImmune) {
                            monster->resetGoal();
                            monster->goalType = 4;
                            monster->goalParam = 3;
                        }
                    }
                    if (pv.reflectsWithBuff && this->curTarget == nullptr && this->hitType != 0 &&
                        pv.reflectBuffId >= 0 && app->player->buffs[pv.reflectBuffId] > 0) {
                        this->missileAnim = pv.reflectAnim;
                        x += app->canvas->viewStepX >> 1;
                        y += app->canvas->viewStepY >> 1;
                        renderMode = pv.reflectRenderMode;
                    }
                    if (pv.impactParticles) {
                        app->particleSystem->spawnParticles(1, pv.particleColor, x, y, z + pv.particleZOffset);
                    }
                    z += pv.impactZOffset;
                    if (pv.soulCubeReturn) {
                        app->game->gsprite_destroy(actMissile);
                        for (int j = i + 1; j < this->numActiveMissiles; ++j) {
                            this->activeMissiles[j - 1] = this->activeMissiles[j];
                        }
                        --this->numActiveMissiles;
                        if (this->soulCubeIsAttacking) {
                            this->soulCubeIsAttacking = false;
                            this->exploded = true;
                            this->explodeOnMonster();
                            this->launchSoulCube();
                        }
                        return;
                    }
                    if (pv.impactParticlesOnSprite) {
                        app->particleSystem->spawnParticles(1, -1, actMissile->sprite);
                    }
                }

                if (this->curAttacker != nullptr && (1 << app->hud->damageDir & 0x1C) == 0x0 && !b) {
                    this->missileAnim = 0;
                }

                app->game->gsprite_destroy(actMissile);
                if (this->missileAnim != 0 && (this->curAttacker == nullptr || (this->curTarget == nullptr && this->hitType != 0))) {
                    GameSprite* gSprite = app->game->gsprite_allocAnim(this->missileAnim, x, y, z);
                    gSprite->flags |= 0x800;
                    this->nextStageTime = app->gameTime + gSprite->duration;
                    app->render->mapSprites[app->render->S_RENDERMODE + gSprite->sprite] = (short)renderMode;
                    app->render->mapSprites[app->render->S_SCALEFACTOR + gSprite->sprite] = (short)scaleFactor;
                    if (this->missileAnim == 243) {
                        app->render->mapSprites[app->render->S_SCALEFACTOR + gSprite->sprite] -= 16;
                    }
                    if (this->curAttacker != nullptr && (app->render->mapSpriteInfo[(this->curAttacker->info & 0xFFFF) - 1] & 0x20000) != 0x0) {
                        app->render->mapSpriteInfo[gSprite->sprite] |= 0x20000;
                    }
                }
                for (int k = i + 1; k < this->numActiveMissiles; ++k) {
                    this->activeMissiles[k - 1] = this->activeMissiles[k];
                }
                --this->numActiveMissiles;
                this->exploded = true;
            }
        }
    }
    else if (this->gotHit && (this->totalDamage > 0 || this->totalArmorDamage > 0) && this->exploded && this->getWeaponFlags(this->attackerWeaponId).meleeImpactAnim != 0) {
        this->missileAnim = this->getWeaponFlags(this->attackerWeaponId).meleeImpactAnim;
        int* calcPosition = this->curAttacker->calcPosition();
        if (app->game->isInFront(calcPosition[0] >> 6, calcPosition[1] >> 6)) {
            GameSprite* gSprite = app->game->gsprite_allocAnim(this->missileAnim, app->canvas->destZ, app->canvas->destY, app->canvas->destZ);
            gSprite->flags |= 0x800;
            app->render->mapSprites[app->render->S_RENDERMODE + gSprite->sprite] = 5;
            if ((app->render->mapSpriteInfo[(this->curAttacker->info & 0xFFFF) - 1] & 0x20000) != 0x0 && this->missileAnim == 245) {
                app->render->mapSpriteInfo[gSprite->sprite] |= 0x20000;
            }
        }
    }
    if (this->exploded) {
        if (this->curTarget == nullptr) {
            this->explodeOnPlayer();
            this->exploded = false;
        }
        else {
            this->explodeOnMonster();
            this->exploded = false;
        }
    }
}

void Combat::launchProjectile() {

    int n = 256;
    int n2;
    int n3 = 16;
    int renderMode = 0;
    if (this->attackerWeaponProj == 12) {
        this->soulCubeIsAttacking = true;
        this->launchSoulCube();
        return;
    }
    this->dodgeDir = (((int)app->nextInt() > 0x3FFFFFF) ? 1 : 0);
    int n5 = ((this->dodgeDir << 1) - 1) * 16;
    int x1;
    int y1;
    int z1;
    if (this->curAttacker == nullptr) {
        x1 = app->game->viewX;
        y1 = app->game->viewY;
        z1 = app->render->getHeight(x1, y1) + 32;
        n2 = 0;
    }
    else {
        int sprite = (this->curAttacker->info & 0xFFFF) - 1;
        x1 = app->render->mapSprites[app->render->S_X + sprite];
        y1 = app->render->mapSprites[app->render->S_Y + sprite];
        z1 = app->render->mapSprites[app->render->S_Z + sprite];
        n2 = 16;
    }
    int x2;
    int y2;
    int z2;
    if (this->curTarget == nullptr) {
        x2 = app->game->viewX;
        y2 = app->game->viewY;
        z2 = app->render->getHeight(x2, y2) + 32;
    }
    else if (this->targetType == 0) {
        x2 = app->game->traceCollisionX;
        y2 = app->game->traceCollisionY;
        z2 = app->game->traceCollisionZ;
    }
    else {
        int n10 = (this->curTarget->info & 0xFFFF) - 1;
        x2 = app->render->mapSprites[app->render->S_X + n10];
        y2 = app->render->mapSprites[app->render->S_Y + n10];
        z2 = app->render->mapSprites[app->render->S_Z + n10];
    }
    bool b = false;

    // Load projectile visual data from YAML-driven table
    if (this->projVisuals != nullptr && this->attackerWeaponProj >= 0 &&
        this->attackerWeaponProj < this->numProjTypes) {
        const auto& pv = this->projVisuals[this->attackerWeaponProj];
        if (pv.launchRenderMode < 0) {
            // No missile (bullet/melee)
            this->missileAnim = 0;
            this->exploded = true;
            return;
        }
        renderMode = pv.launchRenderMode;
        if (pv.launchAnimFromWeapon) {
            this->missileAnim = app->player->activeWeaponDef->tileIndex;
        } else if (pv.launchAnimMonster != 0 && this->curAttacker != nullptr) {
            this->missileAnim = pv.launchAnimMonster;
        } else {
            this->missileAnim = pv.launchAnim;
        }
        if (pv.launchSpeed > 0) n = pv.launchSpeed;
        n += pv.launchSpeedAdd;
        if (pv.launchOffsetXR != 0) {
            x1 += app->game->viewRightStepX >> pv.launchOffsetXR;
            y1 += app->game->viewRightStepY >> pv.launchOffsetXR;
        }
        z1 += pv.launchOffsetZ;
        z1 += pv.launchZOffset;
        z2 += pv.launchZOffset;
        b = false;

        // Data-driven projectile launch behaviors
        if (pv.closeRangeZAdjust &&
            app->render->mapSprites[app->render->S_SCALEFACTOR + this->curTarget->getSprite()] >= 64) {
            z2 += pv.closeRangeZAmount;
        }
        if (pv.monsterDamageBoost && this->curAttacker != nullptr) {
            n += n >> 1;
        }
        if (pv.resetThornParticles) {
            this->numThornParticleSystems = 0;
        }
    } else {
        this->missileAnim = 0;
        this->exploded = true;
        return;
    }
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    if ((this->crFlags & 0x400) != 0x0) {
        int n11 = this->weapons[this->attackerWeapon + 3] * 64;
        n3 = 0;
        if (dx != 0 && dx > dy) {
            x2 = x1 + (x2 - x1) * n11 / dx;
        }
        else if (dy != 0) {
            y2 = y1 + (y2 - y1) * n11 / dy;
        }
        dx = std::abs(x2 - x1);
        dy = std::abs(y2 - y1);
        n5 = 0;
    }
    else if (this->hitType == 0) {
        if (this->targetType == 2 || this->curTarget == nullptr) {
            n3 = 0;
        }
        else {
            n5 = 0;
        }
    }
    else {
        n5 = 0;
    }

    if (dy > dx) {
        int n12 = (((y2 - y1) >> 31) << 1) + 1;
        y1 +=  (n2 * n12);
        y2 += -(n3 * n12);
        x2 += n5;
    }
    else {
        int n15 = (((x2 - x1) >> 31) << 1) + 1;
        x1 +=  (n2 * n15);
        x2 += -(n3 * n15);
        y2 += n5;
    }
    if (this->attackerWeaponProj == 4) {
        if (this->curAttacker == nullptr) {
            x1 += 4 * app->canvas->viewRightStepX >> 6;
            y1 += 4 * app->canvas->viewRightStepY >> 6;
            z1 -= 5;
        }
        else {
            int n16 = 16;
            if ((app->render->mapSpriteInfo[this->curAttacker->getSprite()] & 0x20000) != 0x0) {
                n16 = -16;
            }
            x1 += n16 * app->canvas->viewRightStepX >> 6;
            y1 += n16 * app->canvas->viewRightStepY >> 6;
        }
    }
    GameSprite* allocMissile = this->allocMissile(x1, y1, z1, x2, y2, z2, 1000 * std::max(dx, dy) / n, renderMode);
    if ((this->missileAnim == 4 && this->curTarget != nullptr) || this->missileAnim == 243) {
        app->render->mapSpriteInfo[allocMissile->sprite] = ((app->render->mapSpriteInfo[allocMissile->sprite] & 0xFFFF00FF) | 0x1000);
        if (this->missileAnim == 243) {
            app->render->mapSprites[app->render->S_SCALEFACTOR + allocMissile->sprite] -= 16;
        }
    }
    else if (this->missileAnim >= 241 && this->missileAnim < 244) {
        app->render->mapSpriteInfo[allocMissile->sprite] = ((app->render->mapSpriteInfo[allocMissile->sprite] & 0xFFFF00FF) | 0x2000);
    }
    else if (this->missileAnim == 244) {
        app->render->mapSpriteInfo[allocMissile->sprite] = ((app->render->mapSpriteInfo[allocMissile->sprite] & 0xFFFF00FF) | 0x3000);
        if (this->curAttacker != nullptr) {
            app->render->mapSprites[app->render->S_SCALEFACTOR + allocMissile->sprite] -= 16;
        }
    }
    if (this->attackerWeaponProj == 2) {
        if (this->curAttacker == nullptr) {
            app->render->mapSprites[app->render->S_ENT + allocMissile->sprite] = app->player->getPlayerEnt()->getIndex();
            app->render->relinkSprite(allocMissile->sprite, allocMissile->pos[0] << 4, allocMissile->pos[1] << 4, allocMissile->pos[2] << 4);
        }
        else {
            app->render->mapSprites[app->render->S_ENT + allocMissile->sprite] = this->curAttacker->getIndex();
            app->render->relinkSprite(allocMissile->sprite, allocMissile->pos[3] << 4, allocMissile->pos[4] << 4, allocMissile->pos[5] << 4);
        }
        allocMissile->flags |= 0x4;
        allocMissile->pos[0] = allocMissile->pos[3];
        allocMissile->pos[1] = allocMissile->pos[4];
        allocMissile->pos[2] = allocMissile->pos[5];
        allocMissile->vel[0] = allocMissile->vel[1] = allocMissile->vel[2] = 0;
    }
    if (!b) {
        int sprite = allocMissile->sprite;
        app->render->mapSpriteInfo[allocMissile->sprite] &= 0xFFF700FF;
    }
    this->exploded = false;
}

GameSprite* Combat::allocMissile(int x1, int y1, int z1, int x2, int y2, int z2, int duration, int renderMode) {

    if (this->numActiveMissiles == 8) {
        app->Error("MAX_ACTIVE_MISSILES", Enums::ERR_MAX_MISSILES);
        return nullptr;
    }
    int n8 = app->render->mediaMappings[this->missileAnim + 1] - app->render->mediaMappings[this->missileAnim];
    int numActiveMissiles = this->numActiveMissiles;
    GameSprite* gameSprite = app->game->gsprite_alloc(this->missileAnim, n8, 2562);
    this->activeMissiles[this->numActiveMissiles++] = gameSprite;

    short* mapSprites = app->render->mapSprites;
    mapSprites[app->render->S_ENT + gameSprite->sprite] = -1;
    mapSprites[app->render->S_RENDERMODE + gameSprite->sprite] = (short)renderMode;
    mapSprites[app->render->S_X + gameSprite->sprite] = (gameSprite->pos[0] = (short)x1);
    mapSprites[app->render->S_Y + gameSprite->sprite] = (gameSprite->pos[1] = (short)y1);
    mapSprites[app->render->S_Z + gameSprite->sprite] = (gameSprite->pos[2] = (short)z1);
    app->render->mapSpriteInfo[gameSprite->sprite] |= 0x80000;
    gameSprite->pos[3] = (short)x2;
    gameSprite->pos[4] = (short)y2;
    gameSprite->pos[5] = (short)z2;
    gameSprite->duration = duration;
    if (this->attackerWeaponProj == 13) {
        gameSprite->flags |= 0x4000;
        int n9 = 8;
        if (gameSprite->pos[5] > gameSprite->pos[2]) {
            n9 /= 4;
        }
        gameSprite->pos[5] += (short)(std::min(this->tileDist, 5) * n9);
        if (gameSprite->pos[5] < gameSprite->pos[2]) {
            gameSprite->pos[5] = gameSprite->pos[2];
        }
    }
    if (duration != 0) {
        gameSprite->vel[0] = 1000 * (gameSprite->pos[3] - gameSprite->pos[0]) / duration;
        gameSprite->vel[1] = 1000 * (gameSprite->pos[4] - gameSprite->pos[1]) / duration;
        gameSprite->vel[2] = 1000 * (gameSprite->pos[5] - gameSprite->pos[2]) / duration;
    }
    else {
        gameSprite->vel[0] = gameSprite->vel[1] = gameSprite->vel[2] = 0;
    }
    if (this->missileAnim == 0) {
        app->render->mapSpriteInfo[gameSprite->sprite] |= 0x10000;
        return gameSprite;
    }
    app->render->relinkSprite(gameSprite->sprite);
    return gameSprite;
}

void Combat::launchSoulCube() {

    int sprite = this->curTarget->getSprite();
    int x1, y1, z1;
    int x2, y2, z2;

    if (this->soulCubeIsAttacking) {
        x1 = app->game->viewX;
        y1 = app->game->viewY;
        z1 = app->render->getHeight(x1, y1) + 32;
        x2 = app->render->mapSprites[app->render->S_X + sprite];
        y2 = app->render->mapSprites[app->render->S_Y + sprite];
        z2 = app->render->mapSprites[app->render->S_Z + sprite];
    }
    else {
        x1 = app->render->mapSprites[app->render->S_X + sprite];
        y1 = app->render->mapSprites[app->render->S_Y + sprite];
        z1 = app->render->mapSprites[app->render->S_Z + sprite];
        x2 = app->game->viewX;
        y2 = app->game->viewY;
        z2 = app->render->getHeight(x1, y1) + 32;
    }
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    if (dy > dx) {
        int n4 = (((y2 - y1) >> 31) << 1) + 1;
        if (this->soulCubeIsAttacking) {
            y1 += (64 * n4);
        }
        else {
            y2 += -(64 * n4);
        }
    }
    else {
        int n5 = (((x2 - x1) >> 31) << 1) + 1;
        if (this->soulCubeIsAttacking) {
            x1 += (64 * n5);
        }
        else {
            x2 += -(64 * n5);
        }
    }
    int duration = 1000 * std::max(dx, dy) / 256;
    this->missileAnim = 239;
    this->allocMissile(x1, y1, z1, x2, y2, z2, duration, 0);
}

int Combat::getWeaponTileNum(int n) {
    if (this->wpViewTile != nullptr && n >= 0 && n < this->numWeaponViewTiles && this->wpViewTile[n] != 0) {
        return this->wpViewTile[n];
    }
    // Fallback: sprite should be set in weapons.yaml for all weapons
    return 1 + n;
}

const Combat::FamiliarDef* Combat::getFamiliarDefByWeapon(int weaponIndex) const {
	for (int i = 0; i < this->familiarDefCount; i++) {
		if (this->familiarDefs[i].weaponIndex == weaponIndex) {
			return &this->familiarDefs[i];
		}
	}
	return nullptr;
}

