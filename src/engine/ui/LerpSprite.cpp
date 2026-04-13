#include <stdexcept>

#include "LerpSprite.h"
#include "CAppContainer.h"
#include "App.h"
#include "Game.h"
#include "Render.h"
#include "JavaStream.h"
#include "Enums.h"

LerpSprite::LerpSprite() {
    this->app = nullptr;
}

LerpSprite::~LerpSprite() {
}

void LerpSprite::saveState(OutputStream* OS) {
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    if (this->hSprite == 0) {
        return;
    }
    OS->writeInt(this->travelTime);
    OS->writeInt(app->gameTime - this->startTime);
    OS->writeShort((int16_t)this->hSprite);
    OS->writeShort((int16_t)app->render->getSpriteX(this->hSprite - 1));
    OS->writeShort((int16_t)app->render->getSpriteY(this->hSprite - 1));
    OS->writeShort((int16_t)app->render->getSpriteZ(this->hSprite - 1));
    OS->writeShort((int16_t)this->srcX);
    OS->writeShort((int16_t)this->srcY);
    OS->writeShort((int16_t)this->srcZ);
    OS->writeShort((int16_t)this->dstX);
    OS->writeShort((int16_t)this->dstY);
    OS->writeShort((int16_t)this->dstZ);
    OS->writeShort((int16_t)this->height);
    OS->writeByte((uint8_t)this->srcScale);
    OS->writeByte((uint8_t)this->dstScale);
    OS->writeShort((int16_t)this->flags);
    if (this->thread != nullptr) {
        OS->writeByte((uint8_t)this->thread->getIndex());
    }
    else {
        OS->writeByte((uint8_t)-1);
    }
}

void LerpSprite::calcDist() {
	if (!this->app) this->app = CAppContainer::getInstance()->app;
	Applet* app = this->app;
	this->dist = (int)(app->game->FixedSqrt((this->dstX - this->srcX) * (this->dstX - this->srcX) + (this->dstY - this->srcY) * (this->dstY - this->srcY) << 8) >> 8);
}

void LerpSprite::loadState(InputStream* IS) {
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    this->travelTime = IS->readInt();
    this->startTime = app->gameTime - IS->readInt();
    this->hSprite = IS->readShort();
    int n = this->hSprite - 1;
    app->render->setSpriteX(n, IS->readShort());
    app->render->setSpriteY(n, IS->readShort());
    app->render->setSpriteZ(n, IS->readShort());
    app->render->relinkSprite(n);
    this->srcX = IS->readShort();
    this->srcY = IS->readShort();
    this->srcZ = IS->readShort();
    this->dstX = IS->readShort();
    this->dstY = IS->readShort();
    this->dstZ = IS->readShort();
    this->height = IS->readShort();
    this->srcScale = (int)IS->readByte();
    this->dstScale = (int)IS->readByte();
    this->flags = IS->readShort();
    int index = IS->readSignedByte();
    if (index == -1) {
        this->thread = nullptr;
    }
    else {
        this->thread = &app->game->scriptThreads[index];
    }
    if (this->flags & Enums::LS_FLAG_ANIMATING_EFFECT) {
        ++app->game->animatingEffects;
    }
    this->calcDist();
}