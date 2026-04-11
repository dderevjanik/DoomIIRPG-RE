#include "GameModuleRegistry.h"
#include <cstring>
#include <vector>

struct ModuleEntry {
	const char* id;
	GameModuleFactory factory;
};

static std::vector<ModuleEntry>& getEntries() {
	static std::vector<ModuleEntry> entries;
	return entries;
}

void GameModuleRegistry::registerModule(const char* id, GameModuleFactory factory) {
	getEntries().push_back({id, factory});
}

IGameModule* GameModuleRegistry::create(const char* id) {
	if (!id) return nullptr;
	for (const auto& entry : getEntries()) {
		if (strcmp(entry.id, id) == 0) {
			return entry.factory();
		}
	}
	return nullptr;
}

std::vector<std::string> GameModuleRegistry::listModules() {
	std::vector<std::string> ids;
	for (const auto& entry : getEntries()) {
		ids.push_back(entry.id);
	}
	return ids;
}
