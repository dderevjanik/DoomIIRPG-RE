#include "SpriteDefs.h"
#include "DataNode.h"
#include <cstdio>

// Static member definitions
std::unordered_map<std::string, int> SpriteDefs::tileNameToIndex;
std::unordered_map<int, std::string> SpriteDefs::tileIndexToName;
std::unordered_map<std::string, SpriteSource> SpriteDefs::tileNameToSource;
std::unordered_map<std::string, int> SpriteDefs::ranges;

bool SpriteDefs::parse(const DataNode& config) {
	// Load sprites section (fall back to "tiles" for backwards compat)
	DataNode tiles = config["sprites"];
	if (!tiles || !tiles.isMap()) {
		tiles = config["tiles"];
	}
	if (!tiles || !tiles.isMap()) {
		printf("[sprites] missing or invalid 'sprites' map\n");
		return false;
	}

	SpriteDefs::tileNameToIndex.clear();
	SpriteDefs::tileIndexToName.clear();
	SpriteDefs::tileNameToSource.clear();

	for (auto it = tiles.begin(); it != tiles.end(); ++it) {
		std::string name = it.key().asString();
		SpriteSource src;

		if (it.value().isScalar()) {
			// Simple format: zombie: 20
			src.type = SpriteSourceType::Bin;
			src.id = it.value().asInt(0);
			SpriteDefs::tileNameToIndex[name] = src.id;
			if (SpriteDefs::tileIndexToName.find(src.id) == SpriteDefs::tileIndexToName.end()) {
				SpriteDefs::tileIndexToName[src.id] = name;
			}
		} else if (it.value().isMap()) {
			// Extended format: chainsaw: { type: bin, file: tables.bin, id: 2 }
			DataNode entry = it.value();
			std::string typeStr = entry["type"].asString("bin");
			src.file = entry["file"].asString("");

			if (typeStr == "png" || typeStr == "bmp") {
				src.type = (typeStr == "bmp") ? SpriteSourceType::Bmp : SpriteSourceType::Png;
				SpriteDefs::tileNameToIndex[name] = SpriteDefs::SPRITE_INDEX_EXTERNAL;
			} else {
				src.type = SpriteSourceType::Bin;
				src.id = entry["id"].asInt(0);
				SpriteDefs::tileNameToIndex[name] = src.id;
				if (SpriteDefs::tileIndexToName.find(src.id) == SpriteDefs::tileIndexToName.end()) {
					SpriteDefs::tileIndexToName[src.id] = name;
				}
			}
		}

		SpriteDefs::tileNameToSource[name] = src;
	}

	// Load ranges section
	DataNode rangesNode = config["ranges"];
	SpriteDefs::ranges.clear();
	if (rangesNode && rangesNode.isMap()) {
		for (auto it = rangesNode.begin(); it != rangesNode.end(); ++it) {
			std::string name = it.key().asString();
			int value = it.value().asInt(0);
			SpriteDefs::ranges[name] = value;
		}
	}

	int pngCount = 0, bmpCount = 0;
	for (const auto& [k, v] : SpriteDefs::tileNameToSource) {
		if (v.type == SpriteSourceType::Png) pngCount++;
		if (v.type == SpriteSourceType::Bmp) bmpCount++;
	}
	int binCount = (int)SpriteDefs::tileNameToIndex.size() - pngCount - bmpCount;
	printf("[sprites] loaded %d sprite names (%d bin, %d png, %d bmp), %d ranges\n",
		(int)SpriteDefs::tileNameToIndex.size(),
		binCount, pngCount, bmpCount,
		(int)SpriteDefs::ranges.size());
	return true;
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

bool SpriteDefs::isBmp(const std::string& name) {
	const SpriteSource* src = getSource(name);
	return src && src->type == SpriteSourceType::Bmp;
}

bool SpriteDefs::isExternal(const std::string& name) {
	const SpriteSource* src = getSource(name);
	return src && (src->type == SpriteSourceType::Png || src->type == SpriteSourceType::Bmp);
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
