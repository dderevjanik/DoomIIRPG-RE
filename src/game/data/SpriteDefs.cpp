#include <string_view>
#include "SpriteDefs.h"
#include "CAppContainer.h"
#include "DataNode.h"
#include "Log.h"

// Static member definitions
std::unordered_map<std::string, int, StringHash, std::equal_to<>> SpriteDefs::tileNameToIndex;
std::unordered_map<int, std::string> SpriteDefs::tileIndexToName;
std::unordered_map<std::string, SpriteSource, StringHash, std::equal_to<>> SpriteDefs::tileNameToSource;
std::unordered_map<int, std::string> SpriteDefs::tileIndexToPng;
std::unordered_map<std::string, int, StringHash, std::equal_to<>> SpriteDefs::ranges;
std::unordered_map<int, SpriteRenderProps> SpriteDefs::renderProps;

static const std::string EMPTY_STRING;

std::expected<void, std::string> SpriteDefs::parse(const DataNode& config) {
	// Load sprites section (fall back to "tiles" for backwards compat)
	DataNode tiles = config["sprites"];
	if (!tiles || !tiles.isMap()) {
		tiles = config["tiles"];
	}
	if (!tiles || !tiles.isMap()) {
		return std::unexpected("missing or invalid 'sprites' map");
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
					LOG_INFO("[sprites] PNG override: tile {} ({}) -> {}\n", src.id, name.c_str(), src.file.c_str());
				}
				// Explicit png: field overrides the binary texture with a PNG file
				DataNode pngNode = entry["png"];
				if (pngNode) {
					std::string pngPath = pngNode.asString("");
					if (!pngPath.empty()) {
						SpriteDefs::tileIndexToPng[src.id] = pngPath;
						LOG_INFO("[sprites] PNG override (png: field): tile {} ({}) -> {}\n", src.id, name.c_str(), pngPath.c_str());
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

	// Parse render: blocks into renderProps (keyed by tile index)
	SpriteDefs::renderProps.clear();
	for (auto it = tiles.begin(); it != tiles.end(); ++it) {
		if (!it.value().isMap()) continue;
		DataNode renderNode = it.value()["render"];
		if (!renderNode || !renderNode.isMap()) continue;

		std::string name = it.key().asString();
		int tileIdx = SpriteDefs::getIndex(name);
		if (tileIdx <= 0) continue;

		SpriteRenderProps props;
		props.zOffsetFloor = renderNode["z_offset_floor"].asInt(0);
		props.zOffsetWall = renderNode["z_offset_wall"].asInt(0);
		props.skip = renderNode["skip"].asBool(false);
		props.multiplyShift = renderNode["multiply_shift"].asBool(false);
		props.renderPath = renderNode["path"].asString("");

		DataNode texAnim = renderNode["tex_anim"];
		if (texAnim) {
			props.texAnim.sDivisor = texAnim["s_div"].asInt(0);
			props.texAnim.tDivisor = texAnim["t_div"].asInt(0);
			props.texAnim.mask = texAnim["mask"].asInt(0x3FF);
		}

		DataNode glow = renderNode["glow"];
		if (glow) {
			props.glow.sprite = SpriteDefs::getIndex(glow["sprite"].asString(""));
			props.glow.zMult = glow["z_mult"].asInt(0);
			// Parse render mode name
			std::string modeStr = glow["mode"].asString("");
			auto& renderModes = CAppContainer::getInstance()->gameConfig.renderModes;
			if (auto mit = renderModes.find(modeStr); mit != renderModes.end()) {
				props.glow.renderMode = mit->second;
			}
		}

		DataNode composite = renderNode["composite"];
		if (composite && composite.isSequence()) {
			for (auto cit = composite.begin(); cit != composite.end(); ++cit) {
				DataNode layer = cit.value();
				SpriteCompositeLayer cl;
				cl.sprite = SpriteDefs::getIndex(layer["sprite"].asString(""));
				cl.zMult = layer["z_mult"].asInt(0);
				props.composite.push_back(cl);
			}
		}

		DataNode autoAnim = renderNode["auto_anim"];
		if (autoAnim) {
			props.autoAnimPeriod = autoAnim["period"].asInt(0);
			props.autoAnimFrames = autoAnim["frames"].asInt(2);
		}

		DataNode viewNode = renderNode["position_at_view"];
		if (viewNode) {
			props.positionAtView = true;
			props.viewOffsetMult = viewNode["offset_mult"].asInt(0);
			props.viewZOffset = viewNode["z_offset"].asInt(0);
		}

		DataNode lootNode = renderNode["loot_aura"];
		if (lootNode) {
			props.lootAura = true;
			props.lootAuraScale = lootNode["scale"].asInt(18);
			props.lootAuraFlags = lootNode["flags"].asInt(512);
		}

		SpriteDefs::renderProps[tileIdx] = std::move(props);
	}
	LOG_INFO("[sprites] loaded {} sprite render props\n", (int)SpriteDefs::renderProps.size());

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
	LOG_INFO("[sprites] loaded {} sprite names ({} bin, {} image, {} sheet), {} ranges\n",
		(int)SpriteDefs::tileNameToIndex.size(),
		binCount, imageCount, sheetCount,
		(int)SpriteDefs::ranges.size());
	return {};
}

int SpriteDefs::getIndex(std::string_view name) {
	if (auto it = tileNameToIndex.find(name); it != tileNameToIndex.end()) {
		return it->second;
	}
	return 0;
}

const SpriteSource* SpriteDefs::getSource(std::string_view name) {
	if (auto it = tileNameToSource.find(name); it != tileNameToSource.end()) {
		return &it->second;
	}
	return nullptr;
}

bool SpriteDefs::isImage(std::string_view name) {
	const SpriteSource* src = getSource(name);
	return src && src->type == SpriteSourceType::Image;
}

bool SpriteDefs::isSheet(std::string_view name) {
	const SpriteSource* src = getSource(name);
	return src && src->type == SpriteSourceType::Sheet;
}

bool SpriteDefs::isExternal(std::string_view name) {
	const SpriteSource* src = getSource(name);
	return src && (src->type == SpriteSourceType::Image || src->type == SpriteSourceType::Sheet);
}

int SpriteDefs::getRange(std::string_view name) {
	if (auto it = ranges.find(name); it != ranges.end()) {
		return it->second;
	}
	return 0;
}

bool SpriteDefs::isInRange(int index, std::string_view first, std::string_view last) {
	return index >= getRange(first) && index <= getRange(last);
}

const std::string& SpriteDefs::getPngOverride(int tileIndex) {
	if (auto it = tileIndexToPng.find(tileIndex); it != tileIndexToPng.end()) {
		return it->second;
	}
	return EMPTY_STRING;
}

const SpriteRenderProps* SpriteDefs::getRenderProps(int tileIndex) {
	if (auto it = renderProps.find(tileIndex); it != renderProps.end()) {
		return &it->second;
	}
	return nullptr;
}
