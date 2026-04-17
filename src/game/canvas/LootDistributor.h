#pragma once

class Applet;
class Text;

// Owns loot-pool state used by the looting dialog.
// Populated by poolLoot() when the player loots corpses on a tile;
// consumed by LootingState to render the loot dialog and apply credits.
class LootDistributor
{
public:
	Applet* app = nullptr;

	short lootPoolIndices[18] = {};
	int lootPool[9] = {};
	int lootPoolCredits = 0;
	int numPoolItems = 0;
	int numLootItems = 0;
	Text* lootText = nullptr;
	int lootLineNum = 0;

	explicit LootDistributor(Applet* app) : app(app) {}

	// Scans the tile identified by array[0],array[1] for corpses and aggregates
	// their loot into lootPool / lootText for display. Called from
	// Canvas::handlePlayingEvents when the player triggers a loot action.
	void poolLoot(int* array);
};
