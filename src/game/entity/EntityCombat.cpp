#include <stdexcept>
#include <algorithm>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "Entity.h"
#include "EntityDef.h"
#include "EntityMonster.h"
#include "Combat.h"
#include "CombatEntity.h"
#include "Render.h"
#include "Game.h"
#include "Canvas.h"
#include "Player.h"
#include "Hud.h"
#include "Text.h"
#include "ParticleSystem.h"
#include "BinaryStream.h"
#include "Enums.h"
#include "SpriteDefs.h"
#include "EventBus.h"
#include "GameEvents.h"
#include "Sound.h"
#include "SoundNames.h"
#include "Sounds.h"
#include "AIComponent.h"

bool Entity::touched() {


    uint8_t eType = this->def->eType;
    //printf("Entity::touched %d -------------------------->\n", eType);
    if (eType == Enums::ET_MONSTERBLOCK_ITEM || eType == Enums::ET_ITEM) {
        if (this->touchedItem()) {
            app->game->scriptStateVars[11] = this->def->tileIndex;
            app->game->executeStaticFunc(11);
            if (this->isDroppedEntity()) {
                app->render->setSpriteEnt(this->getSprite(), -1);
                this->def = nullptr;
            }
            return true;
        }
    }
    else if (eType == Enums::ET_ENV_DAMAGE) {
        if (!app->player->isFamiliar) {
            const auto& ed = this->def->envDamage;
            if (ed.damage > 0) {
                // Check resistance buff if configured
                bool resisted = (ed.resistBuffIdx >= 0 && app->player->buffs[ed.resistBuffIdx] > 0);
                if (!resisted) {
                    app->player->painEvent(nullptr, false);
                    app->player->pain(ed.damage, this, true);
                    if (ed.statusEffectId >= 0) {
                        app->player->addStatusEffect(ed.statusEffectId, ed.statusDuration, ed.statusPower);
                        app->player->translateStatusEffects();
                    }
                }
            }
        }
        return true;
    }
    return false;
}

bool Entity::touchedItem() {


    if (this->def->eSubType == Enums::IT_INVENTORY) {
        int param = 1;
        if (this->isDroppedEntity()) {
            param = this->param;
        }
        else if (this->def->parm == 24) {
            param = 2 + app->nextInt() % 3;
        }
        if (!app->player->give(this->def->eSubType, this->def->parm, param, false)) {
            Text* messageBuffer = app->hud->getMessageBuffer(2);
            app->localization->resetTextArgs();
            app->localization->addTextArg((short)1, this->def->name);
            app->localization->composeText((short)0, (short)83, messageBuffer);
            app->hud->finishMessageBuffer();
            return false;
        }
        app->localization->resetTextArgs();
        if (this->def->eSubType == Enums::IT_INVENTORY && (this->def->parm == 19 || this->def->parm == 20)) {
            app->localization->composeText((short)0, (short)84, app->hud->getMessageBuffer(3));
            app->hud->repaintFlags |= 0x4;
        }
        else if (this->def->parm == 24) {
            Text* messageBuffer2 = app->hud->getMessageBuffer(1);
            app->localization->resetTextArgs();
            app->localization->addTextArg(param);
            Text* smallBuffer = app->localization->getSmallBuffer();
            app->localization->composeText((short)1, this->def->longName, smallBuffer);
            app->localization->addTextArg(smallBuffer);
            app->localization->composeText((short)0, (short)86, messageBuffer2);
            smallBuffer->dispose();
        }
        else {
            Text* messageBuffer3 = app->hud->getMessageBuffer(1);
            app->localization->addTextArg((short)1, this->def->longName);
            app->localization->composeText((short)0, (short)85, messageBuffer3);
        }
        app->hud->finishMessageBuffer();
        if (!this->isDroppedEntity() && this->def->parm != 18) {
            app->game->foundLoot(this->getSprite(), 1);
        }
    }
    else if (this->def->eSubType == Enums::IT_FOOD) {
        int n;
        if (this->def->healAmount > 0) {
            n = this->def->healAmount;
        } else if (this->def->parm == 0 || this->def->parm == 2) {
            n = 40;  // legacy fallback: large food
        } else {
            n = 20;  // legacy fallback: small food
        }
        if (!app->player->addHealth(n)) {
            app->hud->addMessage((short)46, 2);
            return false;
        }
    }
    else if (this->def->eSubType == Enums::IT_AMMO) {
        int param2;
        if (this->isDroppedEntity()) {
            param2 = this->param;
        }
        else {
            param2 = 2 + app->nextInt() % 4;
            if (this->def->parm == 2) {
                param2 &= 0xFFFFFFFE;
            }
        }
        if (!app->player->give(2, this->def->parm, param2, false)) {
            app->hud->addMessage((short)87);
        }
        else {
            Text* messageBuffer4 = app->hud->getMessageBuffer(1);
            app->localization->resetTextArgs();
            app->localization->addTextArg(param2);
            Text* smallBuffer2 = app->localization->getSmallBuffer();
            app->localization->composeTextField(this->name, smallBuffer2);
            app->localization->addTextArg(smallBuffer2);
            app->localization->composeText((short)0, (short)86, messageBuffer4);
            app->hud->finishMessageBuffer();
            smallBuffer2->dispose();
            if (!this->isDroppedEntity()) {
                app->game->foundLoot(this->getSprite(), 1);
            }
        }
    }
    else if (this->def->eSubType == Enums::IT_WEAPON) {
        if (app->player->weaponIsASentryBot(this->def->parm)) {
            if (app->player->isFamiliar || app->player->hasASentryBot()) {
                return false;
            }
            int param3;
            if (this->isDroppedEntity()) {
                param3 = this->param;
            }
            else {
                param3 = 100;
            }
            app->player->give(1, this->def->parm, 1, false);
            app->player->ammo[app->combat->familiarAmmoType] = (short)param3;
            app->hud->addMessage((short)0, (short)223, 3);
        }
        else {
            app->player->give(1, this->def->parm, 1, false);
            int n2 = this->def->parm * 9;
            if (app->combat->weapons[n2 + 5] != 0) {
                if ((1 << this->def->parm & 0x200) != 0x0) {
                    app->player->give(2, app->combat->weapons[n2 + 4], 8, true);
                }
                else {
                    app->player->give(2, app->combat->weapons[n2 + 4], 10, true);
                }
            }
            Text* messageBuffer5 = app->hud->getMessageBuffer(1);
            app->localization->resetTextArgs();
            app->localization->addTextArg((short)1, this->def->longName);
            app->localization->composeText((short)0, (short)85, messageBuffer5);
            app->hud->finishMessageBuffer();
            app->player->showWeaponHelp(this->def->parm, false);
        }
        if (!this->isDroppedEntity()) {
            app->game->foundLoot(this->getSprite(), 1);
        }
    }
    app->eventBus->emit(ItemPickupEvent{this, (int)this->def->eSubType, (int)this->def->parm, 1, this->isDroppedEntity()});
    app->game->removeEntity(this);
    app->sound->playSound(Sounds::getResIDByName(SoundName::ITEM_PICKUP), 0, 4, false);
    return true;
}

bool Entity::pain(int n, Entity* entity) {


    bool b = false;
    int sprite = this->getSprite();
    if ((this->info & 0x20000) == 0x0) {
        return b;
    }
    if (this->def->eType == Enums::ET_MONSTER) {
        int stat = this->combat->getStat(0);
        int stat2 = this->combat->getStat(1);
        int n2 = stat - n;
        if (this->isBoss()) {
            int n3 = stat2 >> 1;
            int n4 = n3 >> 1;
            int n5 = stat2 - n4;
            if (n2 <= n5 && n2 + n > n5) {
                if (!app->canvas->combatDone) {
                    b = (app->game->executeStaticFunc(2) != 0);
                    if (n2 < n3) {
                        n2 = n3 + 1;
                        this->monsterEffects &= ~1u;
                    }
                }
                else {
                    n2 = this->combat->getStat(0);
                    this->monsterEffects &= ~1u;
                }
            }
            else if (n2 <= n3 && n2 + n > n3) {
                if (!app->canvas->combatDone) {
                    b = (app->game->executeStaticFunc(3) != 0);
                    if (n2 < n4) {
                        n2 = n4 + 1;
                        this->monsterEffects &= ~1u;
                    }
                }
                else {
                    n2 = this->combat->getStat(0);
                    this->monsterEffects &= ~1u;
                }
            }
            else if (n2 <= n4 && n2 + n > n4) {
                if (!app->canvas->combatDone) {
                    b = (app->game->executeStaticFunc(4) != 0);
                    if (n2 < 0) {
                        n2 = 1;
                        this->monsterEffects &= ~1u;
                    }
                }
                else {
                    n2 = this->combat->getStat(0);
                    this->monsterEffects &= ~1u;
                }
            }
            if (b && (this->monsterFlags & Enums::MFLAG_NOTHINK) != 0x0) {
                app->combat->animLoopCount = 1;
            }
        }

        if ((this->monsterFlags & Enums::MFLAG_NOKILL) != 0x0 && n2 <= 0) {
            n2 = 1;
        }
        this->combat->setStat(0, n2);
        app->eventBus->emit(MonsterPainEvent{this, entity, n, n2});
        if (n2 > 0) {

            int monsterSound = app->game->getMonsterSound(this->def->monsterIdx, Enums::MSOUND_PAIN);
            app->sound->playSound(monsterSound, 0, 3, 0);

            if (app->combat->punchingMonster == 0 && 0x0 == (this->monsterEffects & 0x2)) {
                app->render->setSpriteFrame(sprite, 0x60);
                this->ai->frameTime = app->time + 250;
            }
            if (app->combat->attackerWeaponId != 2) {
                this->ai->resetGoal();
            }
        }
        else if ((this->monsterEffects & 0x2) == 0x0) {
            app->render->setSpriteFrame(sprite, 0x60);
            if (app->combat->animLoopCount > 0) {
                this->ai->frameTime = app->time + 250;
            }
            if (app->combat->punchingMonster != 0) {
                this->ai->frameTime = app->combat->animEndTime + 200;
            }
            else {
                this->ai->frameTime = app->time + 250 + 200;
            }
        }
    }
    else if (this->def->eType == Enums::ET_ATTACK_INTERACTIVE) {
        const auto& dd = this->def->destroy;
        // Spawn destruction particles if configured
        if (dd.particleId >= 0) {
            app->particleSystem->spawnParticles(dd.particleId, dd.particleColor, sprite);
        }
        // Set destruction sprite frame if configured
        if (dd.destroyFrame >= 0) {
            app->render->setSpriteFrame(sprite, dd.destroyFrame);
        }
        // Convert to water spout (pickup-like behavior)
        if (dd.convertToWaterSpout) {
            app->canvas->turnEntityIntoWaterSpout(this);
            return b;
        }
        // Unlink-only (barricade-like) vs full removal
        if (dd.unlinkOnly) {
            app->sound->playSound(Sounds::getResIDByName(SoundName::GLASS), 0, 3, false);
            app->game->unlinkEntity(this);
        } else {
            app->game->removeEntity(this);
            this->info |= 0x400000;
            app->render->setSpriteInfoFlag(sprite, 0x10000);
        }
    }
    return b;
}

int Entity::checkMonsterDeath(bool b, bool b2) {

    int xpAwarded = 0;
    if (b && (app->player->weapons & 0x2000) != 0x0 && app->combat->attackerWeaponId != 13) {
        app->player->give(2, 6, 1);
    }
    if (this->isMonster()) {
        if (b2) {
            int resourceID = app->game->getMonsterSound(this->def->monsterIdx, Enums::MSOUND_DEATH);
            app->sound->playSound(resourceID, 0, 5, false);
        }
        if (b) {
            xpAwarded = app->combat->monsterBehaviors[this->def->monsterIdx].xp;
            app->player->addXP(xpAwarded);
        }
        if ((this->monsterFlags & Enums::MFLAG_NOTRACK) == 0x0) {
            app->player->fillMonsterStats();
            if (app->player->monsterStats[0] == app->player->monsterStats[1] && (app->player->killedMonstersLevels & 1 << (app->canvas->loadMapID - 1)) == 0x0) {
                app->player->showAchievementMessage(2);
                app->player->killedMonstersLevels |= 1 << (app->canvas->loadMapID - 1);
            }
        }
    }
    return xpAwarded;
}

void Entity::died(bool b, Entity* entity) {

    LOG_INFO("[entity] died: type={} subType={} sprite={}\n", this->def->eType, this->def->eSubType, this->getSprite());
    int sprite = this->getSprite();
    short n = app->render->getSpriteX(sprite);
    short n2 = app->render->getSpriteY(sprite);
    int n3 = app->render->getSpriteInfoRaw(sprite);
    if ((this->info & 0x20000) == 0x0 || (this->isMonster() && (this->monsterFlags & Enums::MFLAG_NOKILL) != 0x0)) {
        return;
    }
    this->info &= 0xFFFDFFFF;
    uint8_t  eType = this->def->eType;
    uint8_t  eSubType = this->def->eSubType;
    int16_t  monsterIdx = this->def->monsterIdx;
    if (eType == Enums::ET_ATTACK_INTERACTIVE) {
        app->localization->resetTextArgs();
        Text* smallBuffer = app->localization->getSmallBuffer();
        app->localization->composeTextField(this->name, smallBuffer);
        app->localization->addTextArg(smallBuffer);
        app->hud->addMessage((short)89);
        smallBuffer->dispose();
        app->player->addXP(5);
        if (this->def->eSubType != Enums::INTERACT_PICKUP && this->def->eSubType != Enums::INTERACT_CRATE) {
            app->game->destroyedObject(sprite);
        }
    }
    else if (eType == Enums::ET_CORPSE) {
        n3 |= 0x10000;
        this->info |= 0x410000;
        this->info &= 0xFFF7FFFF;
        if (this->isMonster()) {
            this->monsterEffects = 0;
        }
        app->player->counters[4]++;
        app->game->unlinkEntity(this);
    }
    else if (eType == Enums::ET_MONSTER) {
        this->info |= 0x400000;
        this->ai->resetGoal();
        app->game->snapLerpSprites(this->getSprite());
        n3 = ((n3 & 0xFFFF00FF) | 0x7000);
        this->ai->frameTime = app->time;
        if ((this->info & 0x10000) != 0x0) {
            n3 |= 0x17000;
        }
        else {
            this->info |= 0x1020000;
            this->trimCorpsePile(n, n2);
        }
        if ((this->monsterEffects & 0x2) || (this->monsterEffects & 0x1)) {
            this->clearMonsterEffects();
        }
        else {
            this->monsterEffects &= 0xFFFF801F;
            this->monsterEffects |= 0x220220;
        }
        if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE && (this->monsterFlags & Enums::MFLAG_NORAISE) == 0x0) {
            int n5 = 2 + app->nextInt() % 3;
            this->monsterEffects &= 0xFFFE1FFB;
            this->monsterEffects |= n5 << 13;
            this->monsterEffects |= 0x4;
        }
        app->game->deactivate(this);
        this->undoAttack();
        if (this->isBoss()) {
            app->player->inCombat = false;
            app->game->executeStaticFunc(5);
        }
        else if (app->combat->monsterBehaviors[monsterIdx].floats) {
            app->game->gsprite_allocAnim(241, n, n2, app->render->getSpriteZ(sprite));
            n3 |= 0x10000;
            app->game->spawnDropItem(this);
        }
        int xpAwarded = this->checkMonsterDeath(b, true);
        app->eventBus->emit(MonsterDeathEvent{this, entity, xpAwarded, this->deathByExplosion(entity), (int)n, (int)n2});
        if ((this->info & 0x10000) != 0x0 || app->combat->monsterBehaviors[monsterIdx].floats) {
            this->info = ((this->info & 0xFEFDFFFF) | 0x10000);
            app->game->unlinkEntity(this);
        }
        this->def = this->def->corpseDef;
        this->name = (short)(this->def->name | 0x400);
        app->canvas->invalidateRect();
        if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE && (this->monsterFlags & Enums::MFLAG_NORAISE) == 0x0 && (this->info & 0x80000) == 0x0) {
            int n6 = 2 + app->nextInt() % 3;
            this->monsterEffects |= n6 << 13;
            this->monsterEffects |= 0x4;
        }
        else if ((this->info & 0x80000) != 0x0) {
            n3 |= 0x10000;
            this->info |= 0x410000;
            this->info &= 0xFFF7FFFF;
            app->player->counters[4]++;
            app->game->unlinkEntity(this);
            GameSprite* gsprite_allocAnim = app->game->gsprite_allocAnim(241, app->render->getSpriteX(sprite), app->render->getSpriteY(sprite), app->render->getSpriteZ(sprite) - 20);
            gsprite_allocAnim->flags |= 0x400;
            gsprite_allocAnim->startScale = 96;
            gsprite_allocAnim->destScale = 127;
            gsprite_allocAnim->scaleStep = 38;
        }
    }
    if ((this->info & 0x2000000) != 0x0) {
        app->game->executeEntityFunc(this, this->deathByExplosion(entity));
        this->info &= 0xFDFFFFFF;
    }
    app->render->setSpriteInfoRaw(sprite, n3);
    app->canvas->updateFacingEntity = true;
}

bool Entity::deathByExplosion(Entity* entity) {

    return entity == app->player->getPlayerEnt() && app->combat->getWeaponFlags(app->player->ce->weapon).splashDamage;
}

void Entity::aiCalcSimpleGoal(bool b) {


    if (app->combat->monsterBehaviors[this->def->monsterIdx].canResurrect && this->findRaiseTarget(app->combat->monsterBehaviors[this->def->monsterIdx].resurrectSearchRadius, 0, 0) != -1) {
        return;
    }
    if (app->player->buffs[app->player->fearBuffIdx] > 0 && !app->combat->monsterBehaviors[this->def->monsterIdx].fearImmune) {
        this->ai->goalType = 4;
        this->ai->goalParam = 1;
        return;
    }
    int aiWeaponForTarget = this->aiWeaponForTarget(&app->game->entities[1]);
    bool b2 = false;
    if (aiWeaponForTarget != -1) {
        this->combat->weapon = aiWeaponForTarget;
        b2 = true;
    }
    if (b2) {
        this->ai->goalType = 3;
        this->ai->goalParam = 1;
        if (app->combat->monsterBehaviors[this->def->monsterIdx].evading) {
            this->monsterFlags |= Enums::MFLAG_ABILITY;
        }
    }
    else {
        this->ai->goalType = 2;
        this->ai->goalParam = 1;
        if (app->combat->monsterBehaviors[this->def->monsterIdx].evading) {
            this->monsterFlags &= ~Enums::MFLAG_ABILITY;
        }
    }
}

void Entity::aiChooseNewGoal(bool b) {
    uint8_t eSubType = this->def->eSubType;
    this->ai->resetGoal();
    this->aiCalcSimpleGoal(b);
    if (app->combat->monsterBehaviors[this->def->monsterIdx].evading && this->ai->goalType == 3) {
        this->ai->goalFlags |= 0x8;
    }
}

bool Entity::aiIsValidGoal() {


    uint8_t goalType = this->ai->goalType;
    if (this->ai->goalTurns >= app->combat->monsterBehaviors[this->def->monsterIdx].goalMaxTurns || goalType == 3 || goalType == 0) {
        return false;
    }
    if (goalType == 2) {
        Entity* entity = &app->game->entities[this->ai->goalParam];
        if (this->ai->goalParam != 1) {
            int* calcPosition = entity->calcPosition();
            Entity* mapEntity = app->game->findMapEntity(calcPosition[0], calcPosition[1], 15535);
            if (entity->linkIndex != this->linkIndex && (mapEntity == nullptr || mapEntity == entity)) {
                return true;
            }
        }
    }
    else if (goalType == 1) {
        if (this->linkIndex != this->ai->goalX + this->ai->goalY * 32) {
            return true;
        }
    }
    else {
        if (goalType == 4 || goalType == 5) {
            return this->ai->goalTurns < this->ai->goalParam;
        }
        if (goalType == 6) {
            if (this->ai->goalTurns < this->ai->goalParam) {
                return true;
            }
            if (this->ai->goalTurns == this->ai->goalParam) {
                this->ai->resetGoal();
                return false;
            }
        }
    }
    return false;
}

bool Entity::aiIsAttackValid() {


    EntityMonster* monster = this->monster;
    int weapon = this->combat->weapon;
    if (weapon <= -1) { //[GEC]
        return false;
    }
    int* calcPosition = this->calcPosition();
    app->game->trace(calcPosition[0], calcPosition[1], app->game->destX, app->game->destY, this, 5295, 2);
    Entity* traceEntity = app->game->traceEntity;
    if (traceEntity != nullptr) {
        bool b = app->combat->weapons[weapon * 9 + 3] >= app->combat->WorldDistToTileDist(this->distFrom(app->game->destX, app->game->destY));
        bool b2 = false;
        if (b) {
            int n = calcPosition[0] - app->game->destX;
            int n2 = calcPosition[1] - app->game->destY;
            b2 = ((n != 0 || n2 != 0) && (n == 0 || n2 == 0));
        }
        if (this->ai->target == nullptr && traceEntity->def->eType == Enums::ET_PLAYER && b && b2) {
            return true;
        }
        if (this->ai->target == traceEntity && b && b2) {
            return true;
        }
    }
    return false;
}

int Entity::aiWeaponForTarget(Entity* entity) {


    int sprite = this->getSprite();
    int viewX;
    int viewY;
    if (entity->def->eType == Enums::ET_PLAYER) {
        viewX = app->canvas->viewX;
        viewY = app->canvas->viewY;
    }
    else {
        viewX = app->render->getSpriteX(sprite);
        viewY = app->render->getSpriteY(sprite);
    }
    int a = viewX - app->render->getSpriteX(sprite);
    int a2 = viewY - app->render->getSpriteY(sprite);
    int8_t* weapons = app->combat->weapons;
    if (app->combat->monsterBehaviors[this->def->monsterIdx].orthogonalAttackOnly) {
        if (a != 0 && a2 != 0) {
            return -1;
        }
        int* calcPosition = entity->calcPosition();
        app->game->trace(app->render->getSpriteX(sprite), app->render->getSpriteY(sprite), calcPosition[0], calcPosition[1], this, 4131, 2);
        if (app->game->traceEntity == entity) {
            return app->combat->getMonsterField(this->def, 0);
        }
        return -1;
    }
    else {
        const MonsterDef& mb = app->combat->monsterBehaviors[this->def->monsterIdx];
        if (mb.teleport.enabled) {
            if (this->param <= 0) {
                const auto& tp = mb.teleport;
                int cdRange = tp.cooldownMax - tp.cooldownMin;
                this->param = (cdRange > 0) ? ((int)app->nextInt() % (cdRange + 1) + tp.cooldownMin) : tp.cooldownMin;
                int tpDiam = tp.range * 2 + 1;
                int n = tp.maxAttempts;
                while (n-- > 0) {
                    short n2 = app->render->getSpriteX(sprite);
                    short n3 = app->render->getSpriteY(sprite);
                    int n4 = 0;
                    int n5 = 0;

                    while (n-- > 0 && n4 == 0 && n5 == 0){
                        n4 = (int)app->nextInt() % tpDiam - tp.range;
                        n5 = (int)app->nextInt() % tpDiam - tp.range;
                    }

                    int n6 = n2 + (n4 << 6);
                    int n7 = n3 + (n5 << 6);
                    app->game->trace(n2, n3, n6, n7, this, 15535, 2);
                    if (app->game->numTraceEntities == 0) {
                        if (tp.particleId >= 0)
                            app->particleSystem->spawnParticles(tp.particleId, -1, sprite);
                        app->sound->playSound(Sounds::getResIDByName(SoundName::TELEPORT), 0, 1, 0);
                        app->game->unlinkEntity(this);
                        app->render->setSpriteX(sprite, (short)n6);
                        app->render->setSpriteY(sprite, (short)n7);
                        app->render->setSpriteZ(sprite, (short)(app->render->getHeight(n6, n7) + 32));
                        app->game->linkEntity(this, n6 >> 6, n7 >> 6);
                        app->render->relinkSprite(sprite);
                        if (tp.particleId >= 0)
                            app->particleSystem->spawnParticles(tp.particleId, -1, sprite);
                        break;
                    }
                }
            }
            else {
                --this->param;
            }
        }
        if (a != 0 && a2 != 0) {
            if (app->combat->monsterBehaviors[this->def->monsterIdx].diagonalAttack) {
                int monsterField = app->combat->getMonsterField(this->def, app->combat->monsterBehaviors[this->def->monsterIdx].diagonalAttackField);
                int n8 = monsterField * 9;
                int worldDistToTileDist = app->combat->WorldDistToTileDist(entity->distFrom(app->render->getSpriteX(sprite), app->render->getSpriteY(sprite)));
                if (weapons[n8 + 2] <= worldDistToTileDist && weapons[n8 + 3] >= worldDistToTileDist) {
                    int* calcPosition2 = entity->calcPosition();
                    app->game->trace(app->render->getSpriteX(sprite), app->render->getSpriteY(sprite), calcPosition2[0], calcPosition2[1], this, 4131, 2);
                    if (app->game->traceEntity == entity) {
                        return monsterField;
                    }
                }
                return -1;
            }
            return -1;
        }
        else {
            app->game->trace(app->render->getSpriteX(sprite), app->render->getSpriteY(sprite), viewX, viewY, this, 5295, 2);
            if (app->game->traceEntity != entity) {
                return -1;
            }
            int n9 = a;
            int n10 = a2;
            if (n9 != 0) {
                n9 /= std::abs(a);
            }
            if (n10 != 0) {
                n10 /= std::abs(a2);
            }
            app->game->trace(app->render->getSpriteX(sprite) + n9 * 18, app->render->getSpriteY(sprite) + n10 * 18, viewX, viewY, this, 15791, 2);
            bool b = app->game->traceEntity == entity;
            int monsterField2 = app->combat->getMonsterField(this->def, 0);
            int monsterField3 = app->combat->getMonsterField(this->def, 1);
            if (!b) {
                if (app->combat->getWeaponFlags(monsterField2).requiresLineOfSight) {
                    monsterField2 = 0;
                }
                if (app->combat->getWeaponFlags(monsterField3).requiresLineOfSight) {
                    monsterField3 = 0;
                }
            }
            int n12;
            int n11 = n12 = -1;
            int worldDistToTileDist2 = app->combat->WorldDistToTileDist(entity->distFrom(app->render->getSpriteX(sprite), app->render->getSpriteY(sprite)));
            if (monsterField2 == monsterField3) {
                monsterField3 = 0;
            }
            if (monsterField2 != 0) {
                int n13 = monsterField2 * 9;
                if (weapons[n13 + 2] <= worldDistToTileDist2 && weapons[n13 + 3] >= worldDistToTileDist2) {
                    n12 = n13;
                }
            }
            if (monsterField3 != 0) {
                int n14 = monsterField3 * 9;
                if (weapons[n14 + 2] <= worldDistToTileDist2 && weapons[n14 + 3] >= worldDistToTileDist2) {
                    n11 = n14;
                }
            }

            bool atk1NeedsLos = (monsterField2 >= 0 && app->combat->getWeaponFlags(monsterField2).requiresLosPath);
            bool atk2NeedsLos = (monsterField3 >= 0 && app->combat->getWeaponFlags(monsterField3).requiresLosPath);
            if (atk1NeedsLos || atk2NeedsLos) {
                bool b2 = true;
                int n15 = app->render->getSpriteX(sprite) >> 6;
                int n16 = app->render->getSpriteY(sprite) >> 6;

                do {
                    if (app->game->baseVisitedTiles[n16] & 1 << n15){
                        b2 = false;
                        break;
                    }
                    n15 += n9;
                    n16 += n10;
                } while (--worldDistToTileDist2 > 0);

                if (atk1NeedsLos && !b2) {
                    n12 = -1;
                }
                if (atk2NeedsLos && !b2) {
                    n11 = -1;
                }
            }

            int n17;
            if (n11 != -1 && n12 != -1) {
                // attack1_chance is 0-100 (percent). Pick attack1 with that probability when both attacks are in range.
                n17 = (((int)(app->nextByte() % 100) < app->combat->getMonsterField(this->def, 2)) ? monsterField2 : monsterField3);
            }
            else if (n11 != -1) {
                n17 = monsterField3;
            }
            else {
                if (n12 == -1) {
                    return -1;
                }
                n17 = monsterField2;
            }
            return n17;
        }
    }
}

bool Entity::checkLineOfSight(int n, int n2, int n3, int n4, int n5) {


    int a = n3 - n;
    int a2 = n4 - n2;
    if (a != 0 && a2 != 0) {
        return false;
    }
    if (a != 0) {
        a /= std::abs(a);
    }
    if (a2 != 0) {
        a2 /= std::abs(a2);
    }
    while (n != n3 && n2 != n4) {
        n += a;
        n2 += a2;
        if (app->game->findMapEntity(n << 6, n2 << 6, n5)) {
            return false;
        }
    }
    return true;
}

void Entity::attack() {

    if ((this->monsterFlags & Enums::MFLAG_ATTACKING) == 0x0) {
        this->monsterFlags |= Enums::MFLAG_ATTACKING;
        this->nextAttacker = app->game->combatMonsters;
        app->game->combatMonsters = this;
    }
}

void Entity::undoAttack() {

    if ((this->monsterFlags & Enums::MFLAG_ATTACKING) == 0x0) {
        return;
    }
    this->monsterFlags &= ~Enums::MFLAG_ATTACKING;
    int sprite = this->getSprite();
    if (app->combat->getWeaponFlags(this->combat->weapon).chargeAttack) {
        app->render->setSpriteFrame(sprite, 0);
    }
    this->ai->resetGoal();
    Entity* entity = app->game->combatMonsters;
    Entity* entity2 = nullptr;
    while (entity != nullptr && entity != this) {
        entity2 = entity;
        entity = entity->nextAttacker;
    }
    if (entity2 != nullptr) {
        entity2->nextAttacker = this->nextAttacker;
    }
    else if (app->game->combatMonsters != nullptr) {
        app->game->combatMonsters = this->nextAttacker;
    }
}

void Entity::trimCorpsePile(int n, int n2) {

    Entity* entity = app->game->inactiveMonsters;
    if (entity != nullptr) {
        int n3 = 0;
        do {
            int sprite = entity->getSprite();
            if (app->render->getSpriteX(sprite) == n && app->render->getSpriteY(sprite) == n2 && (entity->info & 0x1010000) != 0x0 && (app->render->getSpriteInfoRaw(sprite) & 0x10000) == 0x0 && ++n3 >= 3) {
                app->render->setSpriteInfoFlag(sprite, 0x10000);
                entity->info = ((entity->info & 0xFEFFFFFF) | 0x10000);
                app->game->unlinkEntity(entity);
            }
            entity = entity->nextOnList;
        } while (entity != app->game->inactiveMonsters);
    }
}

void Entity::knockback(int n, int n2, int n3) {

    int32_t* knockbackDelta = this->knockbackDelta;
    if (n3 == 0) {
        return;
    }
    int destX;
    int destY;
    int n4;
    if (this->def->eType == Enums::ET_PLAYER) {
        destX = app->game->destX;
        destY = app->game->destY;
        n4 = 13501;
    }
    else {
        int sprite = this->getSprite();
        destX = app->render->getSpriteX(sprite);
        destY = app->render->getSpriteY(sprite);
        n4 = 15535;
    }
    knockbackDelta[0] = destX - n;
    knockbackDelta[1] = destY - n2;
    if (knockbackDelta[0] != 0) {
        knockbackDelta[0] /= std::abs(knockbackDelta[0]);
        app->canvas->knockbackStart = destX;
        app->canvas->knockbackWorldDist = std::abs(64 * knockbackDelta[0] * n3);
    }
    if (knockbackDelta[1] != 0) {
        knockbackDelta[1] /= std::abs(knockbackDelta[1]);
        app->canvas->knockbackStart = destY;
        app->canvas->knockbackWorldDist = std::abs(64 * knockbackDelta[1] * n3);
    }

    int farthestKnockbackDist = this->getFarthestKnockbackDist(destX, destY, destX + 64 * knockbackDelta[0] * n3, destY + 64 * knockbackDelta[1] * n3, this, n4, 16, n3);
    if (farthestKnockbackDist == 0 || (knockbackDelta[0] == 0 && knockbackDelta[1] == 0)) {
        return;
    }
    int goalX = destX + knockbackDelta[0] * farthestKnockbackDist * 64 >> 6;
    int goalY = destY + knockbackDelta[1] * farthestKnockbackDist * 64 >> 6;
    if (this->def->eType == Enums::ET_PLAYER) {
        if (this->def->eSubType != Enums::PLAYER_FAMILIAR) {
            app->canvas->knockbackX = knockbackDelta[0];
            app->canvas->knockbackY = knockbackDelta[1];
            app->canvas->knockbackDist = farthestKnockbackDist;
        }
    }
    else {
        this->ai->goalType = 1;
        this->ai->goalX = goalX;
        this->ai->goalY = goalY;
        this->monsterFlags |= Enums::MFLAG_KNOCKBACK;
        LerpSprite* aiInitLerp = this->aiInitLerp(400);
        app->game->unlinkEntity(this);
        app->game->linkEntity(this, goalX, goalY);
        app->game->interpolatingMonsters = true;
        app->game->updateLerpSprite(aiInitLerp);
    }
}

int Entity::getFarthestKnockbackDist(int n, int n2, int n3, int n4, Entity* entity, int n5, int n6, int n7) {

    int n8 = n7;
    app->game->trace(n, n2, n3, n4, entity, n5, n6);
    if (app->game->traceEntity != nullptr) {
        n8 = n8 * app->game->traceFracs[0] >> 14;
    }
    return n8;
}

int Entity::findRaiseTarget(int n, int n2, int n3) {

    Entity* inactiveMonsters = app->game->inactiveMonsters;
    int* calcPosition = this->calcPosition();
    int n4 = calcPosition[0];
    int n5 = calcPosition[1];
    if (this->param != 0) {
        --this->param;
        return -1;
    }
    int n6 = 0;
    if (inactiveMonsters != nullptr) {
        do {
            Entity* nextOnList = inactiveMonsters->nextOnList;
            int sprite = inactiveMonsters->getSprite();
            if ((inactiveMonsters->info & 0x10000) == 0x0 &&
                (inactiveMonsters->info & 0x8000000) == 0x0 &&
                (inactiveMonsters->info & 0x1000000) != 0x0 &&
                !inactiveMonsters->isBoss() &&
                (n2 == 0 || (inactiveMonsters->monsterFlags & n2) != 0x0) &&
                (inactiveMonsters->monsterFlags & n3) == 0x0 &&
                inactiveMonsters->distFrom(n4, n5) <= n &&
                app->game->findMapEntity(app->render->getSpriteX(sprite), app->render->getSpriteY(sprite), 15535) == nullptr &&
                (app->game->difficulty != Enums::DIFFICULTY_NIGHTMARE || 0x0 == (inactiveMonsters->monsterEffects & 0x4)))
            {
                this->raiseTargets[n6++] = inactiveMonsters;
            }
            inactiveMonsters = nextOnList;
        } while (inactiveMonsters != app->game->inactiveMonsters && n6 != 4);
        if (n6 != 0) {
            Entity* entity = this->raiseTargets[app->nextInt() % n6];
            entity->info |= 0x8000000;
            this->raiseTarget(entity->getIndex());
            this->param = 4;
            return entity->getIndex();
        }
    }
    return -1;
}

void Entity::raiseTarget(int n) {

    Entity* entity = &app->game->entities[n];
    int sprite = entity->getSprite();
    app->localization->resetTextArgs();
    Text* smallBuffer = app->localization->getSmallBuffer();
    app->localization->composeTextField(this->name, smallBuffer);
    app->localization->addTextArg(smallBuffer);
    smallBuffer->dispose();
    if (app->game->findMapEntity(app->render->getSpriteX(sprite), app->render->getSpriteY(sprite), 15535) != nullptr) {
        app->hud->addMessage((short)92);
        entity->info &= ~0x8000000;
        return;
    }
    entity->resurrect(app->render->getSpriteX(sprite), app->render->getSpriteY(sprite), app->render->getSpriteZ(sprite));
    app->game->activate(entity, false, false, true, true);
    Text* smallBuffer2 = app->localization->getSmallBuffer();
    app->localization->composeTextField(entity->name, smallBuffer2);
    app->localization->addTextArg(smallBuffer2);
    if (this->def->eType != Enums::ET_PLAYER) {
        app->hud->addMessage((short)93);
    }
    smallBuffer2->dispose();
}

void Entity::resurrect(int n, int n2, int n3) {

    LOG_INFO("[entity] resurrect: type={} subType={} at ({},{})\n", this->def->eType, this->def->eSubType, n, n2);
    int sprite = this->getSprite();
    this->def = this->def->monsterDef;
    this->name = (short)(this->def->name | 0x400);
    this->clearMonsterEffects();
    app->render->setSpriteX(sprite, (short)n);
    app->render->setSpriteY(sprite, (short)n2);
    app->render->setSpriteZ(sprite, (short)n3);
    app->render->setSpriteInfoRaw(sprite, app->render->getSpriteInfoRaw(sprite) & 0xFFFC00FF);
    if ((app->nextInt() & 0x1) != 0x0) {
        app->render->setSpriteInfoFlag(sprite, 0x20000);
    }
    app->render->relinkSprite(sprite);
    this->info &= 0xF6FEFFFF;
    this->info |= 0x20000;
    CombatEntity* ce = this->combat;
    this->initspawn();
    ce->setStat(0, ce->getStat(1));
    this->monsterFlags &= ~Enums::MFLAG_KNOCKBACK;
    app->game->unlinkEntity(this);
    app->game->linkEntity(this, n >> 6, n2 >> 6);
    app->canvas->updateFacingEntity = true;
    app->particleSystem->spawnParticles(1, -161512, sprite);
}
