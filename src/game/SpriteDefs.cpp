#include "SpriteDefs.h"
#include <yaml-cpp/yaml.h>
#include <cstdio>

// Static member definitions
std::unordered_map<std::string, int> SpriteDefs::tileNameToIndex;
std::unordered_map<int, std::string> SpriteDefs::tileIndexToName;
std::unordered_map<std::string, SpriteSource> SpriteDefs::tileNameToSource;
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
		tileNameToSource.clear();

		for (auto it = tiles.begin(); it != tiles.end(); ++it) {
			std::string name = it->first.as<std::string>();
			SpriteSource src;

			if (it->second.IsScalar()) {
				// Simple format: zombie: 20
				src.type = SpriteSourceType::Bin;
				src.id = it->second.as<int>(0);
				tileNameToIndex[name] = src.id;
				if (tileIndexToName.find(src.id) == tileIndexToName.end()) {
					tileIndexToName[src.id] = name;
				}
			} else if (it->second.IsMap()) {
				// Extended format: chainsaw: { type: bin, file: tables.bin, id: 2 }
				YAML::Node entry = it->second;
				std::string typeStr = entry["type"].as<std::string>("bin");
				src.file = entry["file"].as<std::string>("");

				if (typeStr == "png") {
					src.type = SpriteSourceType::Png;
					tileNameToIndex[name] = SPRITE_INDEX_EXTERNAL;
				} else {
					src.type = SpriteSourceType::Bin;
					src.id = entry["id"].as<int>(0);
					tileNameToIndex[name] = src.id;
					if (tileIndexToName.find(src.id) == tileIndexToName.end()) {
						tileIndexToName[src.id] = name;
					}
				}
			}

			tileNameToSource[name] = src;
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

		int pngCount = 0;
		for (const auto& [k, v] : tileNameToSource) {
			if (v.type == SpriteSourceType::Png) pngCount++;
		}
		printf("[sprites] loaded %d tile names (%d bin, %d png), %d ranges from %s\n",
			(int)tileNameToIndex.size(),
			(int)tileNameToIndex.size() - pngCount, pngCount,
			(int)ranges.size(), path);
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

const SpriteSource* SpriteDefs::getSource(const std::string& name) {
	auto it = tileNameToSource.find(name);
	if (it != tileNameToSource.end()) {
		return &it->second;
	}
	return nullptr;
}

bool SpriteDefs::isPng(const std::string& name) {
	const SpriteSource* src = getSource(name);
	return src && src->type == SpriteSourceType::Png;
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
