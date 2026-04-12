#include "LootingState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"

void LootingState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	canvas->clearSoftKeys();
	canvas->lootingTime = app->time;
	canvas->crouchingForLoot = true;
	canvas->lootingCachedPitch = canvas->destPitch;
	canvas->field_0xac5_ = false;
}

void LootingState::onExit(Canvas* canvas) {
}

void LootingState::update(Canvas* canvas) {
	canvas->lootingState();
}

void LootingState::render(Canvas* canvas, Graphics* graphics) {
	canvas->drawLootingMenu(graphics);
}

bool LootingState::handleInput(Canvas* canvas, int key, int action) {
	canvas->handleLootingEvents(action);
	return true;
}
