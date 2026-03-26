#ifndef __ENTITYNAMES_H__
#define __ENTITYNAMES_H__

#include <string>
#include <unordered_map>
#include <vector>

class EntityNames {
  public:
	// Bidirectional maps for each category
	static std::unordered_map<std::string, int> entityTypes;
	static std::unordered_map<std::string, int> monsterSubtypes;
	static std::unordered_map<std::string, int> itemSubtypes;
	static std::unordered_map<std::string, int> doorSubtypes;
	static std::unordered_map<std::string, int> decorSubtypes;
	static std::unordered_map<std::string, int> weaponNames;
	static std::unordered_map<std::string, int> ammoParms;

	// Index-to-name for reverse lookups (entity types, weapon names)
	static std::vector<std::string> entityTypesByIndex;
	static std::vector<std::string> weaponNamesByIndex;

	// Load entity type/subtype names from entities.yaml
	static bool loadEntityTypes(const char* entitiesPath);
	// Load weapon names and ammo parms from weapons.yaml
	static bool loadWeaponNames(const char* weaponsPath);

	// Entity type lookups
	static int entityTypeFromString(const std::string& name);
	static const char* entityTypeToString(int index);

	// Subtype/parm resolution (replaces resolveSubtype/resolveParm)
	static int lookupSubtype(int eType, const std::string& name);
	static int lookupParm(int eType, int eSubType, const std::string& name);

	// Weapon name lookups (replaces WeaponNames namespace)
	static int weaponToIndex(const std::string& name);
	static std::string weaponFromIndex(int index);

  private:
	static int lookupInMap(const std::unordered_map<std::string, int>& map, const std::string& name, int fallback = 0);
};

#endif
