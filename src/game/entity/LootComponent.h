#pragma once
struct LootComponent
{
	static constexpr int MAX_SLOTS = 6;
	int lootSet[MAX_SLOTS] = {};

	LootComponent() = default;
	void reset();
};
