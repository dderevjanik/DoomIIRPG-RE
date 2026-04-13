#pragma once
struct LootComponent
{
	int lootSet[3] = {};

	LootComponent() = default;
	void reset();
};
