#include "SpriteDefs.h"
#include "DataNode.h"
#include "Log.h"

// Static member definitions
std::unordered_map<std::string, int> SpriteDefs::tileNameToIndex;
std::unordered_map<int, std::string> SpriteDefs::tileIndexToName;
std::unordered_map<std::string, SpriteSource> SpriteDefs::tileNameToSource;
std::unordered_map<int, std::string> SpriteDefs::tileIndexToPng;
std::unordered_map<std::string, int> SpriteDefs::ranges;

static const std::string EMPTY_STRING;

bool SpriteDefs::parse(const DataNode& config) {
	// Load sprites section (fall back to "tiles" for backwards compat)
	DataNode tiles = config["sprites"];
	if (!tiles || !tiles.isMap()) {
		tiles = config["tiles"];
	}
	if (!tiles || !tiles.isMap()) {
		LOG_ERROR("[sprites] missing or invalid 'sprites' map\n");
		return false;
	}

	SpriteDefs::tileNameToIndex.clear();
	SpriteDefs::tileIndexToName.clear();
	SpriteDefs::tileNameToSource.clear();
	SpriteDefs::tileIndexToPng.clear();

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
			DataNode entry = it.value();
			src.file = entry["file"].asString("");

			// Infer type from fields:
			//   has "id:" → Bin (legacy binary sprite from tables.bin)
			//   has "frame_size:" → Sheet (sprite sheet with frame metadata)
			//   otherwise → Image (single image file)
			// If file: is an image (.png/.bmp) and id: is present, it's a
			// binary sprite with an image override — the engine loads the image
			// instead of the binary palette+texel data.
			DataNode idNode = entry["id"];
			DataNode frameSizeNode = entry["frame_size"];
			std::string typeStr = entry["type"].asString("");

			// Check if file is an image (not tables.bin)
			bool fileIsImage = false;
			if (!src.file.empty()) {
				auto ext = src.file.substr(src.file.find_last_of('.') + 1);
				fileIsImage = (ext == "png" || ext == "bmp" || ext == "jpg" || ext == "tga");
			}

			if (idNode) {
				// Binary sprite with tile index
				src.type = SpriteSourceType::Bin;
				src.id = idNode.asInt(0);
				SpriteDefs::tileNameToIndex[name] = src.id;
				if (SpriteDefs::tileIndexToName.find(src.id) == SpriteDefs::tileIndexToName.end()) {
					SpriteDefs::tileIndexToName[src.id] = name;
				}
				// If file is an image, register as PNG override for this tile
				if (fileIsImage) {
					SpriteDefs::tileIndexToPng[src.id] = src.file;
					LOG_INFO("[sprites] PNG override: tile %d (%s) -> %s\n", src.id, name.c_str(), src.file.c_str());
				}
				// Explicit png: field overrides the binary texture with a PNG file
				DataNode pngNode = entry["png"];
				if (pngNode) {
					std::string pngPath = pngNode.asString("");
					if (!pngPath.empty()) {
						SpriteDefs::tileIndexToPng[src.id] = pngPath;
						LOG_INFO("[sprites] PNG override (png: field): tile %d (%s) -> %s\n", src.id, name.c_str(), pngPath.c_str());
					}
				}
			} else if (frameSizeNode) {
				// Sprite sheet with frame metadata
				src.type = SpriteSourceType::Sheet;
				if (frameSizeNode.isSequence() && frameSizeNode.size() >= 2) {
					src.frameWidth = frameSizeNode[0].asInt(0);
					src.frameHeight = frameSizeNode[1].asInt(0);
				} else if (frameSizeNode.isMap()) {
					src.frameWidth = frameSizeNode["w"].asInt(0);
					src.frameHeight = frameSizeNode["h"].asInt(0);
				}
				src.frameCount = entry["frames"].asInt(1);
				SpriteDefs::tileNameToIndex[name] = SpriteDefs::SPRITE_INDEX_EXTERNAL;
			} else if (typeStr == "bin") {
				// Legacy explicit type: bin without id (treat as bin with id 0)
				src.type = SpriteSourceType::Bin;
				src.id = 0;
				SpriteDefs::tileNameToIndex[name] = 0;
			} else {
				// Single image file (png, bmp, etc.)
				src.type = SpriteSourceType::Image;
				SpriteDefs::tileNameToIndex[name] = SpriteDefs::SPRITE_INDEX_EXTERNAL;
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

	int imageCount = 0, sheetCount = 0;
	for (const auto& [k, v] : SpriteDefs::tileNameToSource) {
		if (v.type == SpriteSourceType::Image) imageCount++;
		if (v.type == SpriteSourceType::Sheet) sheetCount++;
	}
	int binCount = (int)SpriteDefs::tileNameToIndex.size() - imageCount - sheetCount;
	LOG_INFO("[sprites] loaded %d sprite names (%d bin, %d image, %d sheet), %d ranges\n",
		(int)SpriteDefs::tileNameToIndex.size(),
		binCount, imageCount, sheetCount,
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

bool SpriteDefs::isImage(const std::string& name) {
	const SpriteSource* src = getSource(name);
	return src && src->type == SpriteSourceType::Image;
}

bool SpriteDefs::isSheet(const std::string& name) {
	const SpriteSource* src = getSource(name);
	return src && src->type == SpriteSourceType::Sheet;
}

bool SpriteDefs::isExternal(const std::string& name) {
	const SpriteSource* src = getSource(name);
	return src && (src->type == SpriteSourceType::Image || src->type == SpriteSourceType::Sheet);
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

const std::string& SpriteDefs::getPngOverride(int tileIndex) {
	auto it = tileIndexToPng.find(tileIndex);
	if (it != tileIndexToPng.end()) {
		return it->second;
	}
	return EMPTY_STRING;
}
