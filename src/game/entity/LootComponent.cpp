#include "LootComponent.h"

void LootComponent::reset() {
	for (int i = 0; i < MAX_SLOTS; ++i) {
		this->lootSet[i] = 0;
	}
}
