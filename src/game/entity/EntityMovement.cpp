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
#include "JavaStream.h"
#include "Enums.h"
#include "SpriteDefs.h"
#include "Sound.h"
#include "SoundNames.h"
#include "Sounds.h"
#include "AIComponent.h"

void Entity::aiMoveToGoal() {


    uint8_t goalType = this->ai->goalType;
    AIComponent* ai = this->ai;
    if (goalType == 2 || goalType == 1 || goalType == 4 || goalType == 5) {
        this->aiGoal_MOVE();
    }
    else if (goalType == 3) {
        if (ai->goalParam == 1) {
            ai->target = nullptr;
        }
        else {
            ai->target = &app->game->entities[ai->goalParam];
        }
        this->attack();
    }
    if (this->def->eSubType == Enums::BOSS_PINKY) {
        int sprite = this->getSprite();
        app->game->scriptStateVars[9] = app->render->getSpriteX(sprite);
        app->game->scriptStateVars[10] = app->render->getSpriteY(sprite);
    }
}

void Entity::aiThink(bool b) {
    EntityMonster* monster = this->monster;
    if (this->monsterFlags & Enums::MFLAG_ATTACKING) {
        this->monsterFlags &= ~Enums::MFLAG_ATTACKING;
    }
    if (!(this->monsterFlags & Enums::MFLAG_NOTHINK)) {
        if (!this->aiIsValidGoal()) {
            this->aiChooseNewGoal(b);
        }
        ai->goalTurns++;
        this->aiMoveToGoal();
    }
}

LerpSprite* Entity::aiInitLerp(int travelTime) {


    int sprite = this->getSprite();
    LerpSprite* allocLerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
    allocLerpSprite->srcX = app->render->getSpriteX(sprite);
    allocLerpSprite->srcY = app->render->getSpriteY(sprite);
    allocLerpSprite->srcZ = app->render->getSpriteZ(sprite);
    allocLerpSprite->dstX = 32 + (this->ai->goalX << 6);
    allocLerpSprite->dstY = 32 + (this->ai->goalY << 6);
    allocLerpSprite->dstZ = 32 + app->render->getHeight(allocLerpSprite->dstX, allocLerpSprite->dstY);
    allocLerpSprite->srcScale = allocLerpSprite->dstScale = app->render->getSpriteScaleFactor(sprite);
    allocLerpSprite->startTime = app->gameTime;
    allocLerpSprite->travelTime = travelTime;
    allocLerpSprite->flags |= (Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_ASYNC);
    this->ai->frameTime = app->time + travelTime;
    allocLerpSprite->calcDist();
    this->ai->goalFlags |= 0x1;
    return allocLerpSprite;
}

void Entity::aiFinishLerp() {
    this->ai->goalFlags &= ~1;
    if ((this->monsterFlags & Enums::MFLAG_KNOCKBACK) != 0x0) {
        this->monsterFlags &= ~Enums::MFLAG_KNOCKBACK;
        this->info &= ~0x10000000;
    }
    else {
        this->aiReachedGoal_MOVE();
    }
}

bool Entity::calcPath(int n, int n2, int n3, int n4, int n5, bool b) {


    int n6 = n3 - n;
    int n7 = n4 - n2;
    int closestPathDist = n6 * n6 + n7 * n7;
    uint8_t* visitOrder = app->game->visitOrder;
    int* visitDist = app->game->visitDist;
    app->game->visitedTiles[n2] |= 1 << n;
    bool checkLineOfSight = this->checkLineOfSight(n, n2, n3, n4, n5 | 0x100);
    if ((app->game->lineOfSight == 0 && !checkLineOfSight) || (app->game->lineOfSight == 1 && checkLineOfSight)) {
        if (b) {
            closestPathDist -= 30;
        }
        else {
            closestPathDist += 30;
        }
    }
    if (checkLineOfSight) {
        closestPathDist += app->game->lineOfSightWeight;
    }
    if (app->game->pathDepth > 0 && ((!b && closestPathDist < app->game->closestPathDist) || (b && closestPathDist > app->game->closestPathDist))) {
        app->game->closestPath = app->game->curPath;
        app->game->closestPathDepth = app->game->pathDepth;
        app->game->closestPathDist = closestPathDist;
    }
    if (n == n3 && n2 == n4) {
        app->game->closestPath = app->game->curPath;
        app->game->closestPathDepth = app->game->pathDepth;
        app->game->closestPathDist = closestPathDist;
        return true;
    }
    if (app->game->pathDepth == app->game->pathSearchDepth) {
        return false;
    }
    int n8 = 0;
    const int* viewStepValues = Canvas::viewStepValues;
    int n9 = 4;
    uint8_t b2 = (uint8_t)(app->nextByte() & 0x3);
    while (--n9 >= 0) {
        b2 = (uint8_t)(b2 + 1 & 0x3);
        int n10 = n + (viewStepValues[(b2 << 2) + 0] >> 6);
        int n11 = n2 + (viewStepValues[(b2 << 2) + 1] >> 6);
        if (n11 >= 0 && n11 < 32 && n10 >= 0) {
            if (n10 >= 32) {
                continue;
            }
            if (app->game->findMapEntity(n10 << 6, n11 << 6, 256) != nullptr) {
                continue;
            }
            if ((app->game->visitedTiles[n11] & 1 << n10) != 0) {
                continue;
            }
            visitOrder[n8] = b2;
            int n12 = n3 - n10;
            int n13 = n4 - n11;
            visitDist[n8] = n12 * n12 + n13 * n13;
            bool checkLineOfSight2 = this->checkLineOfSight(n10, n11, n3, n4, n5 | 0x100);
            if ((app->game->lineOfSight == 0 && !checkLineOfSight2) || (app->game->lineOfSight == 1 && checkLineOfSight2)) {
                if (b) {
                    visitDist[n8] -= 30;
                }
                else {
                    visitDist[n8] += 30;
                }
            }
            if (checkLineOfSight2) {
                visitDist[n8] += app->game->lineOfSightWeight;
            }
            ++n8;
        }
    }
    for (int i = 0; i < n8; ++i) {
        for (int j = 0; j < n8 - i - 1; ++j) {
            int n17 = visitDist[j] - visitDist[j + 1];
            if ((!b && n17 > 0) || (b && n17 < 0)) {
                int n18 = visitDist[j + 1];
                visitDist[j + 1] = visitDist[j];
                visitDist[j] = n18;
                uint8_t b3 = visitOrder[j + 1];
                visitOrder[j + 1] = visitOrder[j];
                visitOrder[j] = b3;
            }
        }
    }
    int n19 = 0;
    int n20 = 0;
    for (int k = 0; k < n8; ++k) {
        n19 |= (visitOrder[k] & 0x3) << n20;
        n20 += 2;
    }
    for (int l = 0; l < n8; ++l) {
        int n21 = n19 & 0x3;
        n19 >>= 2;
        int n22 = n + (viewStepValues[(n21 << 2) + 0] >> 6);
        int n23 = n2 + (viewStepValues[(n21 << 2) + 1] >> 6);
        app->game->trace((n << 6) + 32, (n2 << 6) + 32, (n22 << 6) + 32, (n23 << 6) + 32, app->game->skipEnt, n5, 16);
        if (app->game->findEnt != nullptr && app->game->traceEntity == app->game->findEnt) {
            app->game->closestPath = app->game->curPath;
            app->game->closestPathDepth = app->game->pathDepth;
            app->game->closestPathDist = closestPathDist;
            return true;
        }
        int interactClipMask = app->game->interactClipMask;
        if (app->game->traceEntity != nullptr) {
            interactClipMask = 1 << app->game->traceEntity->def->eType;
        }
        if (interactClipMask == 0 || (interactClipMask & app->game->interactClipMask) != 0x0) {
            ++app->game->pathDepth;
            app->game->curPath >>= 2;
            app->game->curPath &= 0x3FFFFFFFFFFFFFFFLL;
            app->game->curPath |= (int64_t)n21 << 62;
            if (this->calcPath(n22, n23, n3, n4, n5, b)) {
                return true;
            }
            --app->game->pathDepth;
            app->game->curPath <<= 2;
        }
    }
    return false;
}

bool Entity::aiGoal_MOVE() {


    bool b = false;
    int sprite = this->getSprite();
    app->game->snapLerpSprites(sprite);
    int sX = (int)app->render->getSpriteX(sprite);
    int sY = (int)app->render->getSpriteY(sprite);
    app->game->closestPath = 0LL;
    app->game->closestPathDepth = 0;
    app->game->closestPathDist = 999999999;
    app->game->curPath = 0LL;
    app->game->pathDepth = 0;
    app->game->pathSearchDepth = app->combat->monsterBehaviors[this->def->monsterIdx].pathSearchDepth;
    app->game->findEnt = nullptr;
    app->game->skipEnt = this;
    app->game->lineOfSight = 2;
    app->game->lineOfSightWeight = app->combat->monsterBehaviors[this->def->monsterIdx].retreatLosWeight;
    app->game->interactClipMask = 32;
    std::memcpy(app->game->visitedTiles, app->game->baseVisitedTiles, sizeof(app->game->visitedTiles));
    if (this->ai->goalType == 2 && this->ai->goalParam == 1) {
        app->game->findEnt = &app->game->entities[1];
        this->ai->goalX = app->game->destX >> 6;
        this->ai->goalY = app->game->destY >> 6;
        app->game->lineOfSightWeight = app->combat->monsterBehaviors[this->def->monsterIdx].chaseLosWeight;
    }
    else if (this->ai->goalType == 5) {
        this->ai->goalX = app->game->destX >> 6;
        this->ai->goalY = app->game->destY >> 6;
        app->game->interactClipMask = 0;
        b = true;
        app->game->lineOfSight = 1;
        app->game->pathSearchDepth = this->ai->goalParam;
    }
    else if (this->ai->goalType == 4) {
        this->ai->goalX = app->game->destX >> 6;
        this->ai->goalY = app->game->destY >> 6;
        b = true;
        app->game->lineOfSight = 1;
    }
    else if (this->ai->goalType == 2) {
        app->game->findEnt = &app->game->entities[this->ai->goalParam];
        this->ai->goalX = app->game->findEnt->linkIndex % 32;
        this->ai->goalY = app->game->findEnt->linkIndex / 32;
    }
    if (b) {
        app->game->closestPathDist = 0;
    }
    int calcPath = this->calcPath(sX >> 6, sY >> 6, this->ai->goalX, this->ai->goalY, 15535, b) ? 1 : 0;
    if (calcPath == 0 && app->game->closestPathDist < 999999999) {
        calcPath = 1;
        app->game->curPath = app->game->closestPath;
        app->game->pathDepth = app->game->closestPathDepth;
    }
    if (calcPath != 0 && app->game->pathDepth > 0) {
        app->game->curPath >>= 64 - app->game->pathDepth * 2;
        this->info &= 0xEFFFFFFF;
        int dX = sX + Canvas::viewStepValues[(int)((app->game->curPath & 0x3LL) << 2) + 0];
        int dY = sY + Canvas::viewStepValues[(int)((app->game->curPath & 0x3LL) << 2) + 1];
        this->ai->goalX = dX >> 6;
        this->ai->goalY = dY >> 6;
        app->game->trace(sX, sY, dX, dY, this, app->game->interactClipMask, 25);
        if (app->game->numTraceEntities == 0) {
            app->game->unlinkEntity(this);
            app->game->linkEntity(this, dX >> 6, dY >> 6);
            if (!app->render->cullBoundingBox(std::min(sX, dX) - 16 << 4, std::min(sY, dY) - 16 << 4, std::max(sX, dX) + 16 << 4, std::max(sY, dY) + 16 << 4, true)) {
                this->info |= 0x10000000;
            }
            app->game->interpolatingMonsters = true;
            this->aiInitLerp(app->combat->monsterBehaviors[this->def->monsterIdx].movementTimeMs);
        }
        else {
            this->ai->goalX = sX >> 6;
            this->ai->goalY = sY >> 6;
            if (app->game->traceEntity->def->eType == Enums::ET_DOOR) {
                app->game->performDoorEvent(0, app->game->traceEntity, 2);
            }
        }
        return true;
    }
    return false;
}

void Entity::aiReachedGoal_MOVE() {

    EntityMonster* monster = this->monster;
    AIComponent* ai = this->ai;
    EntityDef* def = this->def;
    this->info &= ~0x10000000;
    if (ai->goalType != 4 && ai->goalType != 5 && (app->combat->monsterBehaviors[def->monsterIdx].moveToAttack || (app->combat->monsterBehaviors[def->monsterIdx].evading && !(this->monsterFlags & Enums::MFLAG_ABILITY)))) {
        Entity* target = &app->game->entities[1];
        int aiWeaponForTarget = this->aiWeaponForTarget(target);
        if (aiWeaponForTarget != -1) {
            if (target == &app->game->entities[1]) {
                ai->target = nullptr;
            }
            else {
                ai->target = target;
            }
            this->combat->weapon = aiWeaponForTarget;
            this->attack();
            return;
        }
    }
    if (app->combat->monsterBehaviors[def->monsterIdx].evading) {
        this->monsterFlags |= Enums::MFLAG_ABILITY;
    }
    if ((ai->goalFlags & 0x10) != 0x0) {
        ai->goalFlags &= ~0x10;
        this->aiCalcSimpleGoal(false);
        if (ai->goalType == 1 || ai->goalType == 2) {
            if (!app->game->tileObstructsAttack(ai->goalX, ai->goalY)) {
                this->aiGoal_MOVE();
            }
            else {
                ai->resetGoal();
            }
        }
    }
}

int Entity::distFrom(int n, int n2) {
    int* calcPosition = this->calcPosition();
    return std::max((n - calcPosition[0]) * (n - calcPosition[0]), (n2 - calcPosition[1]) * (n2 - calcPosition[1]));
}
