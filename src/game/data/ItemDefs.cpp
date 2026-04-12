#include "ItemDefs.h"
#include "DataNode.h"
#include "Log.h"

// Static member definitions
std::unordered_map<std::string, int, StringHash, std::equal_to<>> ItemDefs::inventoryNameToIndex;
std::unordered_map<std::string, int, StringHash, std::equal_to<>> ItemDefs::ammoNameToIndex;

bool ItemDefs::parse(const DataNode& config) {
	// Load inventory section
	DataNode inventory = config["inventory"];
	ItemDefs::inventoryNameToIndex.clear();
	if (inventory && inventory.isMap()) {
		for (auto it = inventory.begin(); it != inventory.end(); ++it) {
			ItemDefs::inventoryNameToIndex[it.key().asString()] = it.value().asInt(0);
		}
	}

	// Load ammo section
	DataNode ammo = config["ammo"];
	ItemDefs::ammoNameToIndex.clear();
	if (ammo && ammo.isMap()) {
		for (auto it = ammo.begin(); it != ammo.end(); ++it) {
			ItemDefs::ammoNameToIndex[it.key().asString()] = it.value().asInt(0);
		}
	}

	LOG_INFO("[items] loaded %d inventory names, %d ammo names\n",
		(int)ItemDefs::inventoryNameToIndex.size(), (int)ItemDefs::ammoNameToIndex.size());
	return true;
}

int ItemDefs::getInventoryIndex(std::string_view name) {
	if (auto it = inventoryNameToIndex.find(name); it != inventoryNameToIndex.end()) {
		return it->second;
	}
	// Fall back to numeric string
	try { return std::stoi(std::string(name)); } catch (...) { return -1; }
}

int ItemDefs::getAmmoIndex(std::string_view name) {
	if (auto it = ammoNameToIndex.find(name); it != ammoNameToIndex.end()) {
		return it->second;
	}
	// Fall back to numeric string
	try { return std::stoi(std::string(name)); } catch (...) { return -1; }
}
