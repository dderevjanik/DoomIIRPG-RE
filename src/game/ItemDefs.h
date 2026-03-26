#ifndef __ITEMDEFS_H__
#define __ITEMDEFS_H__

#include <string>
#include <unordered_map>

class ItemDefs {
  public:
	// Inventory name-to-index map (populated from items.yaml "inventory" section)
	static std::unordered_map<std::string, int> inventoryNameToIndex;

	// Ammo name-to-index map (populated from items.yaml "ammo" section)
	static std::unordered_map<std::string, int> ammoNameToIndex;

	// Load item definitions from items.yaml
	// Returns true if loaded successfully, false on error
	static bool loadFromYAML(const char* path);

	// Get inventory item index by name, returns -1 if not found
	static int getInventoryIndex(const std::string& name);

	// Get ammo type index by name, returns -1 if not found
	static int getAmmoIndex(const std::string& name);
};

#endif
