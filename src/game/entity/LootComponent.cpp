#include "LootComponent.h"
#include <cstring>

LootComponent::LootComponent() {
	std::memset(this, 0, sizeof(LootComponent));
}

void LootComponent::reset() {
	this->lootSet[0] = 0;
	this->lootSet[1] = 0;
	this->lootSet[2] = 0;
}
