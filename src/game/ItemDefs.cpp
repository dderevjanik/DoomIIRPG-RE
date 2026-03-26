#include "ItemDefs.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

// Static member definitions
std::unordered_map<std::string, int> ItemDefs::inventoryNameToIndex;
std::unordered_map<std::string, int> ItemDefs::ammoNameToIndex;

bool ItemDefs::loadFromYAML(const char* path) {
	try {
		YAML::Node config = YAML::LoadFile(path);

		// Load inventory section
		YAML::Node inventory = config["inventory"];
		inventoryNameToIndex.clear();
		if (inventory && inventory.IsMap()) {
			for (auto it = inventory.begin(); it != inventory.end(); ++it) {
				inventoryNameToIndex[it->first.as<std::string>()] = it->second.as<int>(0);
			}
		}

		// Load ammo section
		YAML::Node ammo = config["ammo"];
		ammoNameToIndex.clear();
		if (ammo && ammo.IsMap()) {
			for (auto it = ammo.begin(); it != ammo.end(); ++it) {
				ammoNameToIndex[it->first.as<std::string>()] = it->second.as<int>(0);
			}
		}

		printf("[items] loaded %d inventory names, %d ammo names from %s\n",
			(int)inventoryNameToIndex.size(), (int)ammoNameToIndex.size(), path);
		return true;
	} catch (const YAML::Exception& e) {
		printf("[items] items.yaml: %s\n", e.what());
		return false;
	}
}

int ItemDefs::getInventoryIndex(const std::string& name) {
	auto it = inventoryNameToIndex.find(name);
	if (it != inventoryNameToIndex.end()) {
		return it->second;
	}
	// Fall back to numeric string
	try { return std::stoi(name); } catch (...) { return -1; }
}

int ItemDefs::getAmmoIndex(const std::string& name) {
	auto it = ammoNameToIndex.find(name);
	if (it != ammoNameToIndex.end()) {
		return it->second;
	}
	// Fall back to numeric string
	try { return std::stoi(name); } catch (...) { return -1; }
}
