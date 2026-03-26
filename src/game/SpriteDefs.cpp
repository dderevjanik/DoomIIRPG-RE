#include "SpriteDefs.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

// Static member definitions
std::unordered_map<std::string, int> SpriteDefs::tileNameToIndex;
std::unordered_map<int, std::string> SpriteDefs::tileIndexToName;
std::unordered_map<std::string, int> SpriteDefs::ranges;

bool SpriteDefs::loadFromYAML(const char* path) {
	try {
		YAML::Node config = YAML::LoadFile(path);

		// Load tiles section
		YAML::Node tiles = config["tiles"];
		if (!tiles || !tiles.IsMap()) {
			printf("[sprites] sprites.yaml: missing or invalid 'tiles' map\n");
			return false;
		}

		tileNameToIndex.clear();
		tileIndexToName.clear();

		for (auto it = tiles.begin(); it != tiles.end(); ++it) {
			std::string name = it->first.as<std::string>();
			int index = it->second.as<int>(0);
			tileNameToIndex[name] = index;
			// Only store first name for each index (skip aliases)
			if (tileIndexToName.find(index) == tileIndexToName.end()) {
				tileIndexToName[index] = name;
			}
		}

		// Load ranges section
		YAML::Node rangesNode = config["ranges"];
		ranges.clear();
		if (rangesNode && rangesNode.IsMap()) {
			for (auto it = rangesNode.begin(); it != rangesNode.end(); ++it) {
				std::string name = it->first.as<std::string>();
				int value = it->second.as<int>(0);
				ranges[name] = value;
			}
		}

		printf("[sprites] loaded %d tile names, %d ranges from %s\n",
			(int)tileNameToIndex.size(), (int)ranges.size(), path);
		return true;
	} catch (const YAML::Exception& e) {
		printf("[sprites] sprites.yaml: %s\n", e.what());
		return false;
	}
}

int SpriteDefs::getIndex(const std::string& name) {
	auto it = tileNameToIndex.find(name);
	if (it != tileNameToIndex.end()) {
		return it->second;
	}
	return 0;
}

int SpriteDefs::getRange(const std::string& name) {
	auto it = ranges.find(name);
	if (it != ranges.end()) {
		return it->second;
	}
	return 0;
}

bool SpriteDefs::isInRange(int index, const std::string& first, const std::string& last) {
	return index >= getRange(first) && index <= getRange(last);
}
