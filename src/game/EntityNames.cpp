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

bool EntityNames::parseTypes(const DataNode&) {
	// Entity types — hardcoded engine constants
	entityTypes = {
		{"world", Enums::ET_WORLD},
		{"player", Enums::ET_PLAYER},
		{"monster", Enums::ET_MONSTER},
		{"npc", Enums::ET_NPC},
		{"playerclip", Enums::ET_PLAYERCLIP},
		{"door", Enums::ET_DOOR},
		{"item", Enums::ET_ITEM},
		{"decor", Enums::ET_DECOR},
		{"env_damage", Enums::ET_ENV_DAMAGE},
		{"corpse", Enums::ET_CORPSE},
		{"attack_interactive", Enums::ET_ATTACK_INTERACTIVE},
		{"monsterblock_item", Enums::ET_MONSTERBLOCK_ITEM},
		{"spritewall", Enums::ET_SPRITEWALL},
		{"nonobstructing_spritewall", Enums::ET_NONOBSTRUCTING_SPRITEWALL},
		{"decor_noclip", Enums::ET_DECOR_NOCLIP},
	};

	// Item subtypes
	itemSubtypes = {
		{"inventory", 0},
		{"weapon", Enums::ITEM_WEAPON},
		{"ammo", Enums::ITEM_AMMO},
		{"food", 3},
		{"sack", 4},
		{"c_note", 5},
		{"c_string", 6},
	};

	// Door subtypes
	doorSubtypes = {
		{"locked", Enums::DOOR_LOCKED},
		{"unlocked", Enums::DOOR_UNLOCKED},
	};

	// Decor subtypes
	decorSubtypes = {
		{"misc", 0},
		{"exithall", 1},
		{"mixing", 2},
		{"statue", 3},
		{"tombstone", 5},
		{"dynamite", 6},
		{"water_spout", 7},
		{"treadmill", 8},
	};

	// Monster subtypes — hardcoded fallback, but lookupSubtype()
	// prefers combat->monsterNameToIndex (populated from monsters.yaml)
	monsterSubtypes = {
		{"zombie", 0}, {"zombie_commando", 1}, {"lost_soul", 2},
		{"imp", 3}, {"sawcubus", 4}, {"pinky", 5},
		{"cacodemon", 6}, {"sentinel", 7}, {"mancubus", 8},
		{"revenant", 9}, {"arch_vile", 10}, {"sentry_bot", 11},
		{"cyberdemon", 12}, {"mastermind", 13}, {"phantom", 14},
		{"boss_vios", 15}, {"boss_vios2", 16},
	};

	buildIndexVector(entityTypes, entityTypesByIndex);

	printf("[entity_names] initialized %d entity types, %d monster subtypes\n",
		(int)entityTypes.size(), (int)monsterSubtypes.size());
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

	// ammo_parms still comes from weapons.yaml (data-driven)
	ammoParms.clear();
	DataNode ammoNode = config["ammo_parms"];
	if (ammoNode && ammoNode.isMap()) {
		for (auto it = ammoNode.begin(); it != ammoNode.end(); ++it) {
			ammoParms[it.key().asString()] = it.value().asInt(0);
		}
	}

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
			// Try monsters.yaml index first (dynamic, data-driven)
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
