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

Entity::Entity() {
    std::memset(this, 0, sizeof(Entity));
}

Entity::~Entity() {
}

bool Entity::startup() {
	//printf("Entity::startup\n");
	this->app = CAppContainer::getInstance()->app;

	return false;
}

void Entity::reset() {
	this->def = nullptr;
	this->prevOnTile = nullptr;
	this->nextOnTile = nullptr;
	this->linkIndex = 0;
	this->info = 0;
	this->param = 0;
	this->monster = nullptr;
	this->name = -1;
	if (this->lootSet) {
		delete this->lootSet;
	}
	this->lootSet = nullptr;
}

void Entity::initspawn() {
    if (!this->app) this->app = CAppContainer::getInstance()->app;

    uint8_t eType = this->def->eType;
    uint8_t eSubType = this->def->eSubType;
    this->name = (short)(this->def->name | 0x400);
    int sprite = this->getSprite();

    int tileNum = app->render->mapSpriteInfo[sprite] & 0xFF;
    if (eType == Enums::ET_MONSTER) {
        app->combat->monsters[this->def->monsterIdx]->clone(&this->monster->ce);

        if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE || (app->game->difficulty == Enums::DIFFICULTY_NORMAL && !this->isBoss())) {
            int stat = this->monster->ce.getStat(1);
            int n2 = stat + (stat >> 2);
            this->monster->ce.setStat(1, n2);
            this->monster->ce.setStat(0, n2);
        }
        short n3 = 0;
        short n4 = 64;
        app->render->mapSprites[app->render->S_Z + sprite] = (short)(32 + app->render->getHeight(app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite]));
        app->render->relinkSprite(sprite);
        if (app->combat->monsterBehaviors[this->def->monsterIdx].smallParm0Scale >= 0 && this->def->parm == 0) {
            n4 = (short)app->combat->monsterBehaviors[this->def->monsterIdx].smallParm0Scale;
        }
        app->render->mapSprites[app->render->S_SCALEFACTOR + sprite] = n4;
        app->render->mapSprites[app->render->S_RENDERMODE + sprite] = n3;
        this->info |= 0x20000;
        if (app->combat->monsterBehaviors[this->def->monsterIdx].isVios) {
            this->param = app->nextInt() % 3 + 3;
        }
    }
    else if (eType == Enums::ET_DECOR && eSubType != Enums::DECOR_STATUE) {
        app->render->mapSpriteInfo[sprite] &= ~0x10000;
        if (tileNum == SpriteDefs::getIndex("switch")) {
            app->render->mapSprites[app->render->S_SCALEFACTOR + sprite] = 32;
        }
    }
    else if (eType == Enums::ET_ATTACK_INTERACTIVE) {
        this->info |= 0x20000;
#if 0 // J2ME
        if (eSubType == SpriteDefs::getIndex("obj_crate")) {
            app->render->mapSprites[app->render->S_Z + sprite] -= 224;
        }
#endif
    }
    else if (this->def->eType == Enums::ET_CORPSE && this->def->eSubType == Enums::CORPSE_SKELETON) {
        this->info |= 0x420000;
    }
    else if (eType == Enums::ET_NPC) {
        this->param = 1;
    }

    if (this->def->eType != Enums::ET_MONSTER && this->def->eType != Enums::ET_CORPSE) {
        if (this->lootSet) {
            delete this->lootSet;
        }
        this->lootSet = nullptr;
    }
    else {
        this->populateDefaultLootSet();
    }
}

int Entity::getSprite() {
	return (this->info & 0xFFFF) - 1;
}

int* Entity::calcPosition() {

    int x;
    int y;
    if (this->def->eType == Enums::ET_WORLD) {
        x = app->game->traceCollisionX;
        y = app->game->traceCollisionY;
    }
    else if (this->def->eType == Enums::ET_PLAYER) {
        x = app->canvas->destX;
        y = app->canvas->destY;
    }
    else if (this->def->eType == Enums::ET_MONSTER) {
        int sprite = this->getSprite();
        x = app->render->mapSprites[app->render->S_X + sprite];
        y = app->render->mapSprites[app->render->S_Y + sprite];
    }
    else {
        int sprite = this->getSprite();
        x = app->render->mapSprites[app->render->S_X + sprite];
        y = app->render->mapSprites[app->render->S_Y + sprite];
    }
    this->pos[0] = x;
    this->pos[1] = y;
    return this->pos;
}

bool Entity::isBoss() {
	const MonsterBehaviors& mb = app->combat->monsterBehaviors[this->def->monsterIdx];
	return mb.isBoss && this->def->parm >= mb.bossMinTier;
}

bool Entity::isHasteResistant() {
    return this->isBoss();
}

bool Entity::isDroppedEntity() {

    short index = this->getIndex();
    return index >= app->game->firstDropIndex && index < app->game->firstDropIndex + 16;
}

short Entity::getIndex() {

    for (short n = 0; n < app->game->numEntities; ++n) {
        if (this == &app->game->entities[n]) {
            return n;
        }
    }
    return -1;
}

void Entity::updateMonsterFX() {

    if (nullptr != this->monster) {
        for (int i = 0; i < 5; ++i) {
            int n = 1 << i;
            int n2 = this->monster->monsterEffects;
            if ((n2 & n) != 0x0) {
                int n3 = 5 + i * 4;
                int n4 = n2 >> n3 & 0xF;
                if (this->def->eType != Enums::ET_CORPSE && (this->info & 0x20000) != 0x0) {
                    int n5 = 0;
                    if (n == 8) {
                        n5 = 4;
                        app->localization->resetTextArgs();
                        app->localization->addTextArg(n5);
                        app->hud->addMessage((short)0, (short)94);
                    }

                    if (n == 1) {
                        n5 = 10;
                        app->localization->resetTextArgs();
                        app->localization->addTextArg(n5);
                        app->hud->addMessage((short)0, (short)95);
                    }
                    else if (n5 > 0) {
                        app->localization->resetTextArgs();
                        app->localization->addTextArg(n5);
                        app->hud->addMessage((short)0, (short)71);
                    }
                    if (n5 > 0) {
                        this->pain(n5, nullptr);
                        n2 = this->monster->monsterEffects;
                        if (this->monster->ce.getStat(0) <= 0) {
                            this->died(true, nullptr);
                            n2 = ((this->monster->monsterEffects & 0xFFFF801F) | 0x220220);
                            n4 = 1;
                        }
                    }
                }
                int monsterEffects;
                if (n4 == 0) {
                    monsterEffects = (n2 & ~n);
                }
                else {
                    --n4;
                    monsterEffects = ((n2 & ~(15 << n3)) | n4 << n3);
                }
                this->monster->monsterEffects = monsterEffects;
            }
        }
    }
}

void Entity::populateDefaultLootSet() {
    if (this->lootSet) {
        delete this->lootSet;
        this->lootSet = nullptr;
    }

    this->lootSet = new int[3];
    this->lootSet[0] = 0;
    this->lootSet[1] = 0;
    this->lootSet[2] = 0;

    if (this->def->eType == Enums::ET_CORPSE) {
        const MonsterBehaviors& mb = app->combat->monsterBehaviors[this->def->monsterIdx];
        if (!mb.lootConfig.noCorpseLoot) {
            this->lootSet[0] = 1089;
        }
    }
    else {
        const MonsterBehaviors& mb = app->combat->monsterBehaviors[this->def->monsterIdx];
        const auto& lc = (mb.hasLootTiers && this->def->parm < MonsterBehaviors::MAX_LOOT_TIERS)
            ? mb.lootTiers[this->def->parm] : mb.lootConfig;
        if (lc.modulus > 0) {
            this->lootSet[0] = lc.base | (this->getSprite() % lc.modulus + lc.offset);
        } else {
            this->lootSet[0] = (0x6000 | this->findRandomJokeItem());
        }
    }
}

int Entity::findRandomJokeItem() {
    int sprite = this->getSprite();
    const auto& jokeItems = CAppContainer::getInstance()->gameConfig.jokeItems;
    auto it = jokeItems.find(app->canvas->loadMapID);
    if (it != jokeItems.end() && !it->second.empty()) {
        return it->second[sprite % it->second.size()];
    }
    app->Error(117); // ERR_ENT_LOOTSET
    return 0;
}

void Entity::addToLootSet(int n, int n2, int n3) {
    if (this->lootSet != nullptr && n3 > 0) {
        for (int i = 0; i < 3; ++i) {
            if (this->lootSet[i] == 0) {
                this->lootSet[i] = (n << 12 | n2 << 6 | n3);
                return;
            }
        }
    }
}

bool Entity::hasEmptyLootSet() {
    return this->lootSet == nullptr || this->lootSet[0] == 0;
}
