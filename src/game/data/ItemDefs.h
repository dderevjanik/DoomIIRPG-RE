#pragma once
#include <string>
#include <unordered_map>

class ItemDefs {
  public:
	// Inventory name-to-index map (populated from items.yaml "inventory" section)
	static std::unordered_map<std::string, int> inventoryNameToIndex;

	// Ammo name-to-index map (populated from items.yaml "ammo" section)
	static std::unordered_map<std::string, int> ammoNameToIndex;

	// Parse item definitions from a DataNode (called by ResourceManager)
	[[nodiscard]] static bool parse(const class DataNode& config);

	// Get inventory item index by name, returns -1 if not found
	[[nodiscard]] static int getInventoryIndex(const std::string& name);

	// Get ammo type index by name, returns -1 if not found
	[[nodiscard]] static int getAmmoIndex(const std::string& name);
};
