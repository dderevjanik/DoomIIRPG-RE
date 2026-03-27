#include "EntityNames.h"
#include "DataNode.h"
#include "Enums.h"
#include "CAppContainer.h"
#include "App.h"
#include "Combat.h"
#include <cstdio>

// Static member definitions
std::unordered_map<std::string, int> EntityNames::entityTypes;
std::unordered_map<std::string, int> EntityNames::monsterSubtypes;
std::unordered_map<std::string, int> EntityNames::itemSubtypes;
std::unordered_map<std::string, int> EntityNames::doorSubtypes;
std::unordered_map<std::string, int> EntityNames::decorSubtypes;
std::unordered_map<std::string, int> EntityNames::weaponNames;
std::unordered_map<std::string, int> EntityNames::ammoParms;
std::vector<std::string> EntityNames::entityTypesByIndex;
std::vector<std::string> EntityNames::weaponNamesByIndex;

static void loadSection(const DataNode& config, const char* key,
						std::unordered_map<std::string, int>& map) {
	map.clear();
	DataNode section = config[key];
	if (section && section.isMap()) {
		for (auto it = section.begin(); it != section.end(); ++it) {
			map[it.key().asString()] = it.value().asInt(0);
		}
	}
}

static void buildIndexVector(const std::unordered_map<std::string, int>& map,
							 std::vector<std::string>& vec) {
	int maxIdx = 0;
	for (auto& kv : map) {
		if (kv.second > maxIdx) maxIdx = kv.second;
	}
	vec.assign(maxIdx + 1, "");
	for (auto& kv : map) {
		if (kv.second >= 0 && kv.second < (int)vec.size()) {
			vec[kv.second] = kv.first;
		}
	}
}

bool EntityNames::parseTypes(const DataNode& config) {
	loadSection(config, "entity_types", EntityNames::entityTypes);
	loadSection(config, "monster_subtypes", EntityNames::monsterSubtypes);
	loadSection(config, "item_subtypes", EntityNames::itemSubtypes);
	loadSection(config, "door_subtypes", EntityNames::doorSubtypes);
	loadSection(config, "decor_subtypes", EntityNames::decorSubtypes);

	buildIndexVector(EntityNames::entityTypes, EntityNames::entityTypesByIndex);

	printf("[entity_names] loaded %d entity types, %d monster subtypes\n",
		(int)EntityNames::entityTypes.size(), (int)EntityNames::monsterSubtypes.size());
	return true;
}

bool EntityNames::parseWeapons(const DataNode& config) {
	// Build weapon name map from weapons: section (each weapon has name -> index)
	EntityNames::weaponNames.clear();
	DataNode weapons = config["weapons"];
	if (weapons) {
		for (auto it = weapons.begin(); it != weapons.end(); ++it) {
			std::string name = it.key().asString();
			int index = it.value()["index"].asInt(-1);
			if (index >= 0) EntityNames::weaponNames[name] = index;
		}
	}
	loadSection(config, "ammo_parms", EntityNames::ammoParms);
	buildIndexVector(EntityNames::weaponNames, EntityNames::weaponNamesByIndex);

	printf("[entity_names] loaded %d weapon names, %d ammo parms\n",
		(int)EntityNames::weaponNames.size(), (int)EntityNames::ammoParms.size());
	return true;
}

int EntityNames::lookupInMap(const std::unordered_map<std::string, int>& map,
							 const std::string& name, int fallback) {
	auto it = map.find(name);
	if (it != map.end()) return it->second;
	try { return std::stoi(name); } catch (...) { return fallback; }
}

int EntityNames::entityTypeFromString(const std::string& name) {
	return lookupInMap(entityTypes, name, 0);
}

const char* EntityNames::entityTypeToString(int index) {
	if (index >= 0 && index < (int)entityTypesByIndex.size() && !entityTypesByIndex[index].empty()) {
		return entityTypesByIndex[index].c_str();
	}
	return "unknown";
}

int EntityNames::lookupSubtype(int eType, const std::string& name) {
	switch (eType) {
		case Enums::ET_MONSTER:
		case Enums::ET_CORPSE: {
			// Try dynamic map first (loaded from monsters.yaml)
			Applet* app = CAppContainer::getInstance()->app;
			if (app && app->combat) {
				auto it = app->combat->monsterNameToIndex.find(name);
				if (it != app->combat->monsterNameToIndex.end())
					return it->second;
			}
			return lookupInMap(monsterSubtypes, name, 0);
		}
		case Enums::ET_ITEM:
		case Enums::ET_MONSTERBLOCK_ITEM:
			return lookupInMap(itemSubtypes, name, 0);
		case Enums::ET_DOOR:
			return lookupInMap(doorSubtypes, name, 0);
		case Enums::ET_DECOR:
		case Enums::ET_DECOR_NOCLIP:
			return lookupInMap(decorSubtypes, name, 0);
		default:
			try { return std::stoi(name); } catch (...) { return 0; }
	}
}

int EntityNames::lookupParm(int eType, int eSubType, const std::string& name) {
	if (eType == Enums::ET_ITEM || eType == Enums::ET_MONSTERBLOCK_ITEM) {
		if (eSubType == Enums::ITEM_WEAPON)
			return lookupInMap(weaponNames, name, 0);
		if (eSubType == Enums::ITEM_AMMO)
			return lookupInMap(ammoParms, name, 0);
	}
	try { return std::stoi(name); } catch (...) { return 0; }
}

int EntityNames::weaponToIndex(const std::string& name) {
	return lookupInMap(weaponNames, name, 0);
}

std::string EntityNames::weaponFromIndex(int index) {
	if (index >= 0 && index < (int)weaponNamesByIndex.size() && !weaponNamesByIndex[index].empty()) {
		return weaponNamesByIndex[index];
	}
	return std::to_string(index);
}
