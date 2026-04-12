#pragma once
#include <expected>
#include <string>
#include <string_view>
#include <unordered_map>
#include "StringHash.h"

class ItemDefs {
  public:
	// Inventory name-to-index map (populated from items.yaml "inventory" section)
	static std::unordered_map<std::string, int, StringHash, std::equal_to<>> inventoryNameToIndex;

	// Ammo name-to-index map (populated from items.yaml "ammo" section)
	static std::unordered_map<std::string, int, StringHash, std::equal_to<>> ammoNameToIndex;

	// Parse item definitions from a DataNode (called by ResourceManager)
	[[nodiscard]] static std::expected<void, std::string> parse(const class DataNode& config);

	// Get inventory item index by name, returns -1 if not found
	[[nodiscard]] static int getInventoryIndex(std::string_view name);

	// Get ammo type index by name, returns -1 if not found
	[[nodiscard]] static int getAmmoIndex(std::string_view name);
};
