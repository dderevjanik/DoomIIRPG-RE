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

void Entity::aiThink(bool forceNewGoal) {
    EntityMonster* monster = this->monster;
    if (this->monsterFlags & Enums::MFLAG_ATTACKING) {
        this->monsterFlags &= ~Enums::MFLAG_ATTACKING;
    }
    if (!(this->monsterFlags & Enums::MFLAG_NOTHINK)) {
        if (!this->aiIsValidGoal()) {
            this->aiChooseNewGoal(forceNewGoal);
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

bool Entity::calcPath(int curTileX, int curTileY, int goalTileX, int goalTileY, int traceMask, bool retreat) {


    int dx = goalTileX - curTileX;
    int dy = goalTileY - curTileY;
    int closestPathDist = dx * dx + dy * dy;
    uint8_t* visitOrder = app->game->visitOrder;
    int* visitDist = app->game->visitDist;
    app->game->visitedTiles[curTileY] |= 1 << curTileX;
    bool checkLineOfSight = this->checkLineOfSight(curTileX, curTileY, goalTileX, goalTileY, traceMask | 0x100);
    if ((app->game->lineOfSight == 0 && !checkLineOfSight) || (app->game->lineOfSight == 1 && checkLineOfSight)) {
        if (retreat) {
            closestPathDist -= 30;
        }
        else {
            closestPathDist += 30;
        }
    }
    if (checkLineOfSight) {
        closestPathDist += app->game->lineOfSightWeight;
    }
    if (app->game->pathDepth > 0 && ((!retreat && closestPathDist < app->game->closestPathDist) || (retreat && closestPathDist > app->game->closestPathDist))) {
        app->game->closestPath = app->game->curPath;
        app->game->closestPathDepth = app->game->pathDepth;
        app->game->closestPathDist = closestPathDist;
    }
    if (curTileX == goalTileX && curTileY == goalTileY) {
        app->game->closestPath = app->game->curPath;
        app->game->closestPathDepth = app->game->pathDepth;
        app->game->closestPathDist = closestPathDist;
        return true;
    }
    if (app->game->pathDepth == app->game->pathSearchDepth) {
        return false;
    }
    int numCandidates = 0;
    const int* viewStepValues = Canvas::viewStepValues;
    int dirsLeft = 4;
    uint8_t dirIdx = (uint8_t)(app->nextByte() & 0x3);
    while (--dirsLeft >= 0) {
        dirIdx = (uint8_t)(dirIdx + 1 & 0x3);
        int nextTileX = curTileX + (viewStepValues[(dirIdx << 2) + 0] >> 6);
        int nextTileY = curTileY + (viewStepValues[(dirIdx << 2) + 1] >> 6);
        if (nextTileY >= 0 && nextTileY < 32 && nextTileX >= 0) {
            if (nextTileX >= 32) {
                continue;
            }
            if (app->game->findMapEntity(nextTileX << 6, nextTileY << 6, 256) != nullptr) {
                continue;
            }
            if ((app->game->visitedTiles[nextTileY] & 1 << nextTileX) != 0) {
                continue;
            }
            visitOrder[numCandidates] = dirIdx;
            int candDx = goalTileX - nextTileX;
            int candDy = goalTileY - nextTileY;
            visitDist[numCandidates] = candDx * candDx + candDy * candDy;
            bool checkLineOfSight2 = this->checkLineOfSight(nextTileX, nextTileY, goalTileX, goalTileY, traceMask | 0x100);
            if ((app->game->lineOfSight == 0 && !checkLineOfSight2) || (app->game->lineOfSight == 1 && checkLineOfSight2)) {
                if (retreat) {
                    visitDist[numCandidates] -= 30;
                }
                else {
                    visitDist[numCandidates] += 30;
                }
            }
            if (checkLineOfSight2) {
                visitDist[numCandidates] += app->game->lineOfSightWeight;
            }
            ++numCandidates;
        }
    }
    for (int i = 0; i < numCandidates; ++i) {
        for (int j = 0; j < numCandidates - i - 1; ++j) {
            int distDiff = visitDist[j] - visitDist[j + 1];
            if ((!retreat && distDiff > 0) || (retreat && distDiff < 0)) {
                int tmpDist = visitDist[j + 1];
                visitDist[j + 1] = visitDist[j];
                visitDist[j] = tmpDist;
                uint8_t tmpDir = visitOrder[j + 1];
                visitOrder[j + 1] = visitOrder[j];
                visitOrder[j] = tmpDir;
            }
        }
    }
    int packedDirs = 0;
    int packedShift = 0;
    for (int k = 0; k < numCandidates; ++k) {
        packedDirs |= (visitOrder[k] & 0x3) << packedShift;
        packedShift += 2;
    }
    for (int l = 0; l < numCandidates; ++l) {
        int dir = packedDirs & 0x3;
        packedDirs >>= 2;
        int stepTileX = curTileX + (viewStepValues[(dir << 2) + 0] >> 6);
        int stepTileY = curTileY + (viewStepValues[(dir << 2) + 1] >> 6);
        app->game->trace((curTileX << 6) + 32, (curTileY << 6) + 32, (stepTileX << 6) + 32, (stepTileY << 6) + 32, app->game->skipEnt, traceMask, 16);
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
            app->game->curPath |= (int64_t)dir << 62;
            if (this->calcPath(stepTileX, stepTileY, goalTileX, goalTileY, traceMask, retreat)) {
                return true;
            }
            --app->game->pathDepth;
            app->game->curPath <<= 2;
        }
    }
    return false;
}

bool Entity::aiGoal_MOVE() {


    bool retreat = false;
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
        retreat = true;
        app->game->lineOfSight = 1;
        app->game->pathSearchDepth = this->ai->goalParam;
    }
    else if (this->ai->goalType == 4) {
        this->ai->goalX = app->game->destX >> 6;
        this->ai->goalY = app->game->destY >> 6;
        retreat = true;
        app->game->lineOfSight = 1;
    }
    else if (this->ai->goalType == 2) {
        app->game->findEnt = &app->game->entities[this->ai->goalParam];
        this->ai->goalX = app->game->findEnt->linkIndex % 32;
        this->ai->goalY = app->game->findEnt->linkIndex / 32;
    }
    if (retreat) {
        app->game->closestPathDist = 0;
    }
    int calcPath = this->calcPath(sX >> 6, sY >> 6, this->ai->goalX, this->ai->goalY, 15535, retreat) ? 1 : 0;
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

int Entity::distFrom(int x, int y) {
    int* calcPosition = this->calcPosition();
    return std::max((x - calcPosition[0]) * (x - calcPosition[0]), (y - calcPosition[1]) * (y - calcPosition[1]));
}
