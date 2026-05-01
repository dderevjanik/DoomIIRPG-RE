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
#include "LootComponent.h"
#include "AIComponent.h"
#include "Sound.h"
#include "SoundNames.h"
#include "Sounds.h"

bool Entity::isBinaryEntity(int* array) {

    bool b = false;
    if (this->def == nullptr) {
        return false;
    }
    if (this->isDroppedEntity()) {
        return false;
    }
    switch (this->def->eType) {
        case Enums::ET_ITEM:
        case Enums::ET_ATTACK_INTERACTIVE:
        case Enums::ET_MONSTERBLOCK_ITEM: {
            if (this->def->eSubType == Enums::INTERACT_PICKUP) {
                b = false;
                break;
            }
            if ((this->info & 0x100000) != 0x0) {
                b = true;
                break;
            }
            b = false;
            break;
        }
        case Enums::ET_DOOR: {
            if (app->render->getSpriteScaleFactor(this->getSprite()) != 64) {
                b = true;
            }
            else {
                b = false;
            }
            if (this->def->eSubType == Enums::DOOR_LOCKED && nullptr != array) {
                array[1] |= 0x200000;
            }
            break;
        }
        case Enums::ET_NONOBSTRUCTING_SPRITEWALL: {
           int sprite = this->getSprite();
            if ((app->render->getSpriteInfoRaw(sprite) & 0xFF) == SpriteDefs::getIndex("fence")) {
                if ((app->render->getSpriteInfoRaw(sprite) & 0x10000) != 0x0) {
                    b = true;
                }
                else {
                    b = false;
                }
            }
            break;
        }
        case Enums::ET_CORPSE: {
            if (this->def->eSubType == Enums::CORPSE_SKELETON) {
                b = (this->param != 0);
                break;
            }
            return false;
        }
        case Enums::ET_DECOR_NOCLIP: {
            if (this->def->eSubType == Enums::DECOR_WATER_SPOUT) {
                b = ((this->info & 0x400000) != 0x0);
                break;
            }
            return false;
        }
        default: {
            return false;
        }
    }
    if (nullptr != array) {
        array[0] = (b ? 1 : 0);
    }
    return true;
}

bool Entity::isNamedEntity(int* array) {

    if (this->def == nullptr || this->name == Localization::STRINGID((short)1, this->def->name) || this->def->eType == Enums::ET_CORPSE) {
        return false;
    }
    array[0] = this->name;
    if (array[0] != -1) {
        return true;
    }
    app->Error(25); // ERR_ISNAMEDENTITY
    return false;
}

void Entity::saveState(OutputStream* OS, int n) {


    short* mapSprites = app->render->mapSprites;
    int* mapSpriteInfo = app->render->mapSpriteInfo;
    if ((n & 0x20000) != 0x0) {
        this->isNamedEntity(this->tempSaveBuf);
        OS->writeShort((int16_t)this->tempSaveBuf[0]);
    }
    if ((n & 0x80000) != 0x0) {
        return;
    }
    if (this->def->eType == Enums::ET_ATTACK_INTERACTIVE && this->def->eSubType == Enums::INTERACT_BARRICADE) {
        OS->writeShort(mapSprites[app->render->S_SCALEFACTOR + this->getSprite()]);
        return;
    }
    int sprite = this->getSprite();
    if (this->def->eType == Enums::ET_MONSTER && (n & 0x200000) != 0x0) {
        OS->writeByte(mapSprites[app->render->S_X + sprite] >> 3);
        OS->writeByte(mapSprites[app->render->S_Y + sprite] >> 3);
        OS->writeByte((mapSpriteInfo[sprite] & 0xFF00) >> 8);
        OS->writeShort(this->monsterFlags);
        return;
    }
    OS->writeByte(this->info >> 16 & 0xFF);
    if ((this->info & 0x10000) == 0x0) {
        OS->writeByte((mapSpriteInfo[sprite] & 0xFF0000) >> 16);
        OS->writeByte((mapSpriteInfo[sprite] & 0xFF00) >> 8);
        if (this->isDroppedEntity() || (app->render->getSpriteInfoRaw(sprite) & 0xF000000) == 0x0) {
            OS->writeByte(mapSprites[app->render->S_X + sprite] >> 3);
            OS->writeByte(mapSprites[app->render->S_Y + sprite] >> 3);
        }
        if (this->isDroppedEntity()) {
            OS->writeInt(this->param);
        }
        if (!this->isDroppedEntity() && (app->render->getSpriteInfoRaw(sprite) & 0xF000000) != 0x0) {
            OS->writeShort(this->linkIndex);
        }
    }
    if (this->isMonster()) {
        OS->writeShort(this->monsterFlags);
        if ((this->info & 0x10000) == 0x0) {
            if ((this->monsterFlags & Enums::MFLAG_SCALED) != 0x0) {
                OS->writeByte(mapSprites[app->render->S_SCALEFACTOR + sprite]);
            }
            if ((n & 0x100000) == 0x0) {
                OS->writeShort(this->monsterEffects);
                this->combat->saveState(OS, false);
                this->ai->saveGoalState(OS);
            }
        }
    }
    else if (this->isDroppedEntity()) {
        OS->writeByte((uint8_t)(this->def->eType | this->def->eSubType << 4));
        OS->writeByte(this->def->parm);
        if ((n & 0x100000) != 0x0) {
            int lootSet2;
            int lootSet0;
            int lootSet1 = lootSet0 = (lootSet2 = 0);
            if (this->loot != nullptr) {
                lootSet0 = this->loot->lootSet[0];
                lootSet1 = this->loot->lootSet[1];
                lootSet2 = this->loot->lootSet[2];
            }
            OS->writeInt(lootSet0);
            OS->writeInt(lootSet1);
            OS->writeInt(lootSet2);
        }
        if (this->def->eType == Enums::ET_DECOR_NOCLIP && this->def->eSubType == Enums::DECOR_DYNAMITE) {
            OS->writeByte((mapSpriteInfo[sprite] & 0xFF000000) >> 24);
        }
    }
    else {
        OS->writeShort(mapSprites[app->render->S_Z + sprite]);
    }
}

void Entity::loadState(InputStream* IS, int n) {


    if ((n & 0x20000) != 0x0) {
        this->name = IS->readShort();
    }
    int sprite = this->getSprite();
    int n2 = app->render->getSpriteInfoRaw(sprite) & 0xFF;
    if ((app->render->getSpriteInfoRaw(sprite) & 0x400000) != 0x0) {
        n2 += 257;
    }
    if ((n & 0x40000) != 0x0) {
        app->render->setSpriteInfoRaw(sprite, app->render->getSpriteInfoRaw(sprite) & 0xFFFEFFFF);
        if ((n & 0x80000) != 0x0 && (this->info & 0x100000) == 0x0) {
            app->game->linkEntity(this, app->render->getSpriteX(sprite) >> 6, app->render->getSpriteY(sprite) >> 6);
        }
    }
    else {
        app->render->setSpriteInfoFlag(sprite, 0x10000);
        if ((this->info & 0x100000) != 0x0 && this->def->eType != Enums::ET_DOOR && (n2 < SpriteDefs::getRange("dummy_start") || n2 > SpriteDefs::getRange("dummy_end"))) {
            app->game->unlinkEntity(this);
        }
        else if (this->def->eType == Enums::ET_ATTACK_INTERACTIVE && this->def->eSubType != Enums::INTERACT_PICKUP) {
            ++app->game->destroyedObj;
        }
    }
    if ((n & 0x1000000) != 0x0) {
        this->info |= 0x2000000;
    }
    if (this->isBinaryEntity(nullptr)) {
        this->restoreBinaryState(n);
        if ((n & 0x80000) != 0x0) {
            return;
        }
    }
    if (this->def != nullptr && this->def->eType == Enums::ET_ATTACK_INTERACTIVE && this->def->eSubType == Enums::INTERACT_BARRICADE) {
        short short1 = IS->readShort();
        if (short1 != app->render->getSpriteScaleFactor(sprite)) {
            app->render->setSpriteScaleFactor(sprite, short1);
            this->info |= 0x400000;
        }
        return;
    }
    if ((n & 0x100000) != 0x0) {
        if ((n & 0x40000) == 0x0) {
            this->info |= 0x10000;
            if ((this->info & 0x100000) != 0x0) {
                app->game->unlinkEntity(this);
            }
        }
        else if ((n & 0x80000) != 0x0) {
            app->Error(Enums::ERR_CLEANCORPSE);
        }
        app->render->setSpriteInfoRaw(sprite, ((app->render->getSpriteInfoRaw(sprite) & 0xFFFF00FF) | 0x7000));
        if (!this->isDroppedEntity()) {
            this->def = this->def->corpseDef;
        }
    }
    if (this->def != nullptr && this->def->eType == Enums::ET_NPC) {
        this->param = (((n & 0x200000) != 0x0) ? 1 : 0);
        this->param += (((n & 0x4000000) != 0x0) ? 1 : 0);
    }
    if (this->isMonster()) {
        if ((n & 0x800000) != 0x0) {
            this->monsterFlags |= Enums::MFLAG_NORESPAWN;
            this->info |= 0x400000;
        }
        if ((n & 0x400000) != 0x0) {
            this->monsterFlags |= Enums::MFLAG_NOTRACK;
            this->info |= 0x400000;
        }
        if ((n & 0x8000000) != 0x0) {
            this->monsterFlags |= Enums::MFLAG_LOOTED;
            this->info |= 0x400000;
        }
    }
    if ((n & 0x80000) != 0x0) {
        return;
    }
    if ((this->info & 0x100000) != 0x0) {
        app->game->unlinkEntity(this);
    }
    int sprite2 = this->getSprite();
    if (this->def != nullptr && this->def->eType == Enums::ET_MONSTER && (n & 0x200000) != 0x0) {
        short n5 = (short)((IS->readByte() & 0xFF) << 3);
        short n6 = (short)((IS->readByte() & 0xFF) << 3);
        int n7 = (IS->readByte() & 0xFF) << 8;
        app->render->setSpriteX(sprite2, n5);
        app->render->setSpriteY(sprite2, n6);
        app->render->setSpriteZ(sprite2, (short)(app->render->getHeight(n5, n6) + 32));
        app->render->setSpriteInfoRaw(sprite2, ((app->render->getSpriteInfoRaw(sprite2) & 0xFFFF00FF) | n7));
        app->render->relinkSprite(sprite2);
        if ((this->info & 0x100000) != 0x0) {
            app->game->unlinkEntity(this);
        }
        if ((n & 0x40000) == 0x0) {
            app->game->deactivate(this);
        }
        else {
            app->game->linkEntity(this, n5 >> 6, n6 >> 6);
        }
        this->monsterFlags = IS->readShort();
        if (this->monsterFlags != 0 || n7 != 0) {
            this->info |= 0x400000;
        }
        return;
    }
    this->info = ((this->info & 0xFF00FFFF) | (IS->readByte() & 0xFF) << 16);
    if ((this->info & 0x10000) == 0x0) {
        int n8 = IS->readByte() & 0xFF;
        int n9 = IS->readByte() & 0xFF;
        app->render->setSpriteInfoRaw(sprite2, ((app->render->getSpriteInfoRaw(sprite2) & 0xFF0000FF) | n9 << 8 | n8 << 16));
        if (this->isDroppedEntity() || (app->render->getSpriteInfoRaw(sprite2) & 0xF000000) == 0x0) {
            app->render->setSpriteX(sprite2, (short)(IS->readUnsignedByte() << 3));
            app->render->setSpriteY(sprite2, (short)(IS->readUnsignedByte() << 3));
            if (this->isMonster() || this->isDroppedEntity()) {
                app->render->setSpriteZ(sprite2, (short)(app->render->getHeight(app->render->getSpriteX(sprite2), app->render->getSpriteY(sprite2)) + 32));
            }
            app->render->relinkSprite(sprite2);
        }
        if (this->isDroppedEntity()) {
            this->param = IS->readInt();
        }
        if (!this->isDroppedEntity() && (app->render->getSpriteInfoRaw(sprite2) & 0xF000000) != 0x0) {
            this->linkIndex = IS->readShort();
        }
        else {
            this->linkIndex = (short)((app->render->getSpriteX(sprite2) >> 6) + (app->render->getSpriteY(sprite2) >> 6) * 32);
        }
        if (((app->render->getSpriteInfoRaw(sprite2) & 0xF000000) == 0xC000000 || (app->render->getSpriteInfoRaw(sprite2) & 0xF000000) == 0x3000000) && (this->def->eType == Enums::ET_NONOBSTRUCTING_SPRITEWALL || this->def->eType == Enums::ET_SPRITEWALL)) {
            int n10 = this->linkIndex % 32;
            int n11 = this->linkIndex / 32;
            app->render->setSpriteX(sprite2, (short)((n10 << 6) + 32));
            app->render->setSpriteY(sprite2, (short)((n11 << 6) + 32));
            app->render->relinkSprite(sprite2);
        }
        if (n9 != 0) {
            this->info |= 0x400000;
        }
    }
    if ((n & 0x2000000) != 0x0) {
        this->info |= 0x80000;
    }
    if (this->isMonster()) {
        this->monsterFlags = IS->readShort();
        if ((this->info & 0x10000) == 0x0) {
            if ((this->monsterFlags & Enums::MFLAG_SCALED) != 0x0) {
                app->render->setSpriteScaleFactor(sprite2, (short)IS->readUnsignedByte());
            }
            if ((n & 0x100000) != 0x0) {
                this->info |= 0x1000000;
            }
            else {
                this->monsterEffects = IS->readShort();
                this->combat->loadState(IS, false);
                this->ai->loadGoalState(IS);
            }
        }
        if ((this->info & 0x40000) != 0x0) {
            this->info &= 0xFFFBFFFF;
            app->game->activate(this, false, false, false, true);
        }
        if ((this->info & 0x1000000) != 0x0) {
            this->def = this->def->corpseDef;
        }
    }
    else if (this->isDroppedEntity()) {
        uint8_t byte1 = IS->readByte();
        uint8_t b = (uint8_t)(byte1 & 0xF);
        uint8_t b2 = (uint8_t)(byte1 >> 4 & 0xF);
        uint8_t byte2 = IS->readByte();
        this->def = app->entityDefManager->find(b, b2, byte2);
        if (this->name == -1) {
            this->name = (short)(this->def->name | 0x400);
        }
        short n12 = this->def->tileIndex;
        if ((n & 0x100000) != 0x0) {
            n12 = this->def->monsterDef->tileIndex;
            int int1 = IS->readInt();
            int int2 = IS->readInt();
            int int3 = IS->readInt();
            if (this->param == 0) {
                this->populateDefaultLootSet();
                this->loot->lootSet[0] = int1;
                this->loot->lootSet[1] = int2;
                this->loot->lootSet[2] = int3;
            }
            this->info |= 0x1000000;
        }
        app->render->setSpriteInfoRaw(sprite2, ((app->render->getSpriteInfoRaw(sprite2) & 0xFFFFFF00) | n12));
        app->render->setSpriteEnt(sprite2, this->getIndex());
        if (this->def->eType == Enums::ET_DECOR_NOCLIP && this->def->eSubType == Enums::DECOR_DYNAMITE) {
            int n13 = (IS->readByte() & 0xFF) << 24;
            app->render->setSpriteInfoFlag(sprite2, n13);
            app->render->setSpriteZ(sprite2, (short)(app->render->getHeight(app->render->getSpriteX(sprite2), app->render->getSpriteY(sprite2)) + (((app->render->getSpriteInfoRaw(sprite2) & 0xF000000) != 0x0) ? 32 : 31)));
            app->render->setSpriteScaleFactor(sprite2, 32);
            app->render->relinkSprite(sprite2);
        }
    }
    else {
        app->render->setSpriteZ(sprite2, IS->readShort());
        app->render->relinkSprite(sprite2);
    }
    if ((this->info & 0x100000) != 0x0) {
        if ((app->render->getSpriteInfoRaw(sprite2) & 0xF000000) != 0x0) {
            app->game->linkEntity(this, this->linkIndex % 32, this->linkIndex / 32);
        }
        else {
            app->game->linkEntity(this, app->render->getSpriteX(sprite2) >> 6, app->render->getSpriteY(sprite2) >> 6);
        }
    }
}

int Entity::getSaveHandle(bool b) {

    int* tempSaveBuf = this->tempSaveBuf;
    tempSaveBuf[tempSaveBuf[0] = 1] = this->getIndex();
    bool droppedEntity = this->isDroppedEntity();
    bool binaryEntity = this->isBinaryEntity(tempSaveBuf);
    if (((this->info & 0xFFFF) == 0x0 || this->def == nullptr) && !binaryEntity) {
        return -1;
    }
    if (droppedEntity && (this->info & 0x100000) == 0x0) {
        return -1;
    }
    if (droppedEntity && b && this->def->eType == Enums::ET_DECOR_NOCLIP && this->def->eSubType == Enums::DECOR_DYNAMITE) {
        return -1;
    }
    bool b2 = tempSaveBuf[0] != 0;
    int n = tempSaveBuf[1];
    if (binaryEntity && b2) {
        n |= 0x10000;
    }
    if (this->isNamedEntity(this->tempSaveBuf)) {
        n |= 0x20000;
    }
    if ((app->render->getSpriteInfoRaw(this->getSprite()) & 0x10000) == 0x0) {
        n |= 0x40000;
    }
    if ((this->info & 0x2000000) != 0x0) {
        n |= 0x1000000;
    }
    if ((this->info & 0x400000) == 0x0) {
        n |= 0x80000;
        if ((n & 0x20000) == 0x0 && this->def->eType == Enums::ET_DECOR && this->def->eSubType != Enums::DECOR_STATUE) {
            return -1;
        }
    }
    if ((this->info & 0x1010000) != 0x0) {
        n |= 0x100000;
    }
    if ((this->info & 0x80000) != 0x0) {
        n |= 0x2000000;
    }
    if ((this->info & 0x10000) != 0x0) {
        n = ((n & 0xFFFBFFFF) | 0x100000);
    }
    if (this->def->eType == Enums::ET_NPC && this->param != 0) {
        n |= 0x200000;
        if (this->param == 2) {
            n |= 0x4000000;
        }
    }
    if (this->isMonster()) {
        if ((this->monsterFlags & Enums::MFLAG_NORESPAWN) != 0x0) {
            n |= 0x800000;
        }
        if ((this->monsterFlags & Enums::MFLAG_NOTRACK) != 0x0) {
            n |= 0x400000;
        }
        if ((this->monsterFlags & Enums::MFLAG_LOOTED) != 0x0) {
            n |= 0x8000000;
        }
    }
    if (b) {
        if (this->def->eType == Enums::ET_CORPSE && this->def->eSubType != Enums::CORPSE_SKELETON && !droppedEntity && this->isMonster() && 0x0 == (this->monsterFlags & Enums::MFLAG_NOTRACK)) {
            n = ((n & 0xFFFBFFFF) | 0x80000);
        }
        else if (this->def->eType == Enums::ET_MONSTER) {
            n |= 0x200000;
        }
    }
    return n;
}

void Entity::restoreBinaryState(int n) {

    bool b = (n & 0x10000) != 0x0;
    switch (this->def->eType) {
        case Enums::ET_ITEM:
        case Enums::ET_ATTACK_INTERACTIVE:
        case Enums::ET_MONSTERBLOCK_ITEM: {
            if (this->def->eSubType == Enums::INTERACT_PICKUP) {
                if (b) {
                    app->canvas->turnEntityIntoWaterSpout(this);
                    break;
                }
                break;
            }
            else {
                if (b) {
                    app->game->unlinkEntity(this);
                    app->game->linkEntity(this, this->linkIndex % 32, this->linkIndex / 32);
                }
                else {
                    app->game->unlinkEntity(this);
                }
                if (this->def->eSubType == Enums::INTERACT_BARRICADE || this->def->eSubType == Enums::INTERACT_CRATE) {
                    int sprite = this->getSprite();
                    if (b) {
                        app->game->unlinkEntity(this);
                        app->game->linkEntity(this, this->linkIndex % 32, this->linkIndex / 32);
                        app->render->relinkSprite(sprite);
                    }
                    else {
                        app->render->setSpriteInfoRaw(sprite, ((app->render->getSpriteInfoRaw(sprite) & 0xFFFF00FF) | ((this->def->eSubType == Enums::INTERACT_CRATE) ? 3 : 1) << 8));
                        app->render->relinkSprite(sprite);
                    }
                    break;
                }
                break;
            }
            break;
        }
        case Enums::ET_DOOR: {
            bool b2 = this->def->eSubType == Enums::DOOR_LOCKED;
            app->game->setLineLocked(this, false);
            if (b) {
                app->game->performDoorEvent(0, this, 0);
            }
            else {
                app->game->performDoorEvent(1, this, 0);
            }
            if ((n & 0x200000) != 0x0 || b2) {
                app->game->setLineLocked(this, (n & 0x200000) != 0x0);
            }
            break;
        }
        case Enums::ET_NONOBSTRUCTING_SPRITEWALL: {
            if (b) {
                app->game->performDoorEvent(0, this, 0);
                break;
            }
            app->game->performDoorEvent(1, this, 0);
            break;
        }
        case Enums::ET_CORPSE: {
            if (b) {
                ++this->param;
                this->loot = nullptr;
                break;
            }
            break;
        }
        default: {
            break;
        }
    }
}
