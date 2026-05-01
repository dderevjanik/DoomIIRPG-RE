#include <span>
#include <stdexcept>
#include <unordered_map>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "DataNode.h"
#include "JavaStream.h"
#include "Resource.h"
#include "MayaCamera.h"
#include "Canvas.h"
#include "Image.h"
#include "Render.h"
#include "Game.h"
#include "Text.h"
#include "GLES.h"
#include "TGLVert.h"
#include "TinyGL.h"
#include "Player.h"
#include "Game.h"
#include "MenuSystem.h"
#include "Menus.h"
#include "Combat.h"
#include "Enums.h"
#include "Hud.h"
#include "Utils.h"
#include "Sound.h"
#include "EntityDef.h"
#include "SpriteDefs.h"
#include "stb_image.h"

void Render::initSpriteDefs() {
	TILE_SKY_BOX = SpriteDefs::getIndex("sky_box");
	TILE_BOSS_VIOS = SpriteDefs::getIndex("boss_vios");
	TILE_BOSS_VIOS5 = SpriteDefs::getIndex("boss_vios5");
	TILE_OBJ_CRATE = SpriteDefs::getIndex("obj_crate");
	TILE_TERMINAL_TARGET = SpriteDefs::getIndex("terminal_target");
	TILE_TERMINAL_HACKING = SpriteDefs::getIndex("terminal_hacking");
	TILE_CLOSED_PORTAL_EYE = SpriteDefs::getIndex("closed_portal_eye");
	TILE_EYE_PORTAL = SpriteDefs::getIndex("eye_portal");
	TILE_PORTAL_SOCKET = SpriteDefs::getIndex("portal_socket");
	TILE_RED_DOOR_LOCKED = SpriteDefs::getIndex("red_door_locked");
	TILE_BLUE_DOOR_UNLOCKED = SpriteDefs::getIndex("blue_door_unlocked");
	TILE_FLAT_LAVA = SpriteDefs::getIndex("flat_lava");
	TILE_FLAT_LAVA2 = SpriteDefs::getIndex("flat_lava2");
	TILE_HELL_HANDS = SpriteDefs::getIndex("hell_hands");
	TILE_FADE = SpriteDefs::getIndex("fade");
	TILE_SCORCH_MARK = SpriteDefs::getIndex("scorch_mark");
	TILE_WATER_STREAM = SpriteDefs::getIndex("water_stream");
	TILE_OBJ_FIRE = SpriteDefs::getIndex("obj_fire");
	TILE_OBJ_TORCHIERE = SpriteDefs::getIndex("obj_torchiere");
	TILE_SFX_LIGHTGLOW1 = SpriteDefs::getIndex("sfx_lightglow1");
	TILE_WATER_SPOUT = SpriteDefs::getIndex("water_spout");
	TILE_TREE_TOP = SpriteDefs::getIndex("tree_top");
	TILE_TREE_TRUNK = SpriteDefs::getIndex("tree_trunk");
	TILE_PRACTICE_TARGET = SpriteDefs::getIndex("practice_target");
	TILE_OBJ_CORPSE = SpriteDefs::getIndex("obj_corpse");
	TILE_OBJ_OTHER_CORPSE = SpriteDefs::getIndex("obj_other_corpse");
	TILE_OBJ_SCIENTIST_CORPSE = SpriteDefs::getIndex("obj_scientist_corpse");
	TILE_MONSTER_SENTRY_BOT = SpriteDefs::getIndex("monster_sentry_bot");
	TILE_MONSTER_RED_SENTRY_BOT = SpriteDefs::getIndex("monster_red_sentry_bot");
	TILE_TREADMILL_SIDE = SpriteDefs::getIndex("treadmill_side");
	TILE_SENTINEL_SPIKES_DUMMY = SpriteDefs::getIndex("sentinel_spikes_dummy");
	TILE_SHADOW = SpriteDefs::getIndex("shadow");
	TILE_GLASS = SpriteDefs::getIndex("glass");
}

Render::Render() = default;

Render::~Render() {}

bool Render::startup() {
	this->app = CAppContainer::getInstance()->app;
	this->gameConfig = &CAppContainer::getInstance()->gameConfig;
	this->sdlGL = CAppContainer::getInstance()->sdlGL;
	this->headless = CAppContainer::getInstance()->headless;

	LOG_INFO("Render::startup\n");

	this->nodeIdxs = new short[Render::MAX_VISIBLE_NODES];
	this->mapFlags = new uint8_t[1024];
	this->staticFuncs = new int[12];
	this->customSprites = new int[Render::MAX_CUSTOM_SPRITES];
	this->dropSprites = new int[Render::MAX_DROP_SPRITES];
	this->splitSprites = new int[Render::MAX_SPLIT_SPRITES];
	this->temp = new int[3];
	this->renderMode = Render::RENDER_DEFAULT;

	this->skipCull = false;
	this->skipBSP = false;
	this->skipLines = false;
	this->skipSprites = false;
	this->skipViewNudge = false;

	//-------------------------------
	this->numMapSprites = 0;
	if (this->mapSprites) {
		delete this->mapSprites;
	}
	this->mapSprites = nullptr;

	if (this->mapSpriteInfo) {
		delete this->mapSpriteInfo;
	}
	this->mapSpriteInfo = nullptr;

	//-------------------------------
	this->lastTileEvent = -1;
	this->numTileEvents = 0;
	if (this->tileEvents) {
		delete this->tileEvents;
	}
	this->tileEvents = nullptr;

	//-------------------------------
	this->mapByteCodeSize = 0;
	if (this->mapByteCode) {
		delete this->mapByteCode;
	}
	this->mapByteCode = nullptr;

	//-------------------------------
	this->currentFrameTime = 0;
	this->postProcessMode = 0;
	this->brightenPostProcess = false;
	this->brightenPostProcessBeginTime = 0;
	this->screenVScrollOffset = 0;
	this->useMastermindHack = false;
	this->useCaldexHack = false;
	this->delayedSpriteBuffer[0] = -1;
	this->delayedSpriteBuffer[1] = -1;

	this->screenWidth = app->canvas->viewRect[2];
	this->screenHeight = app->canvas->viewRect[3];
	if ((this->screenHeight & 0x1) != 0x0) {
		--this->screenHeight;
	}

	this->_gles = new gles;
	this->_gles->WindowInit();
	this->_gles->GLInit(this);
	this->imgPortal = nullptr;

	this->imgPortal = app->loadImage("portal_image2.bmp", true);

	// [GEC] New
	this->fixWaterAnim1 = false;
	this->fixWaterAnim2 = false;
	this->fixWaterAnim3 = false;
	this->fixWaterAnim4 = false;

	return true;
}

void Render::shutdown() {
	this->unloadMap();
}

int Render::getNextEventIndex() {
	if (this->lastTileEvent == -1) {
		return -1;
	}

	int n = (this->lastTileEvent & 0xFFFF0000) >> 16;
	int n2 = (this->lastTileEvent & 0xFFFF) + 1;
	if (n2 < this->numTileEvents) {
		int n3 = n2 * 2;
		if ((this->tileEvents[n3] & 0x3FF) == n) {
			this->lastTileEvent = n2 | (n << 16);
			return n3;
		}
	}

	return this->lastTileEvent = -1;
}

int Render::findEventIndex(int n) {
	for (int i = 0; i < this->numTileEvents; ++i) {
		int n2 = i * 2;
		if ((this->tileEvents[n2] & 0x3FF) == n) {
			this->lastTileEvent = i | (n << 16);
			return n2;
		}
	}
	return this->lastTileEvent = -1;
}

void Render::unloadMap() {


	this->startFogLerp(32752, 32752, 0);
	this->_gles->UnloadSkyMap();

	if (this->mediaBounds) {
		delete this->mediaBounds;
	}
	this->mediaBounds = nullptr;

	if (this->mediaMappings) {
		delete this->mediaMappings;
	}
	this->mediaMappings = nullptr;

	if (this->mediaDimensions) {
		delete this->mediaDimensions;
	}
	this->mediaDimensions = nullptr;

	if (this->mediaPalColors) {
		delete this->mediaPalColors;
	}
	this->mediaPalColors = nullptr;

	if (this->mediaTexelSizes) {
		delete this->mediaTexelSizes;
	}
	this->mediaTexelSizes = nullptr;

	if (this->mediaPalettesSizes) {
		delete this->mediaPalettesSizes;
	}
	this->mediaPalettesSizes = nullptr;

	if (this->mediaTexelSizes2) {
		delete this->mediaTexelSizes2;
	}
	this->mediaTexelSizes2 = nullptr;

	if (this->mediaTexels) {
		for (int i = 0; i < 1024; i++) {
			if (this->mediaTexels[i]) {
				delete this->mediaTexels[i];
			}
			this->mediaTexels[i] = nullptr;
		}
		delete this->mediaTexels;
	}
	this->mediaTexels = nullptr;

	if (this->mediaPalettes) {
		for (int i = 0; i < 1024; i++) {
			delete[] this->mediaPalettes[i];
		}
		delete[] this->mediaPalettes;
	}
	this->mediaPalettes = nullptr;

	if (this->normals) {
		delete this->normals;
	}
	this->normals = nullptr;

	if (this->nodeNormalIdxs) {
		delete this->nodeNormalIdxs;
	}
	this->nodeNormalIdxs = nullptr;

	if (this->nodeOffsets) {
		delete this->nodeOffsets;
	}
	this->nodeOffsets = nullptr;

	if (this->nodeChildOffset1) {
		delete this->nodeChildOffset1;
	}
	this->nodeChildOffset1 = nullptr;

	if (this->nodeChildOffset2) {
		delete this->nodeChildOffset2;
	}
	this->nodeChildOffset2 = nullptr;

	if (this->nodeSprites) {
		delete this->nodeSprites;
	}
	this->nodeSprites = nullptr;

	if (this->nodeBounds) {
		delete this->nodeBounds;
	}
	this->nodeBounds = nullptr;

	if (this->nodePolys) {
		delete this->nodePolys;
	}
	this->nodePolys = nullptr;

	if (this->lineFlags) {
		delete this->lineFlags;
	}
	this->lineFlags = nullptr;

	if (this->lineXs) {
		delete this->lineXs;
	}
	this->lineXs = nullptr;

	if (this->lineYs) {
		delete this->lineYs;
	}
	this->lineYs = nullptr;

	if (this->heightMap) {
		delete this->heightMap;
	}
	this->heightMap = nullptr;

	this->numMapSprites = 0;
	if (this->mapSprites) {
		delete this->mapSprites;
	}
	this->mapSprites = nullptr;

	if (this->mapSpriteInfo) {
		delete this->mapSpriteInfo;
	}
	this->mapSpriteInfo = nullptr;

	this->numTileEvents = 0;
	this->mapByteCodeSize = 0;
	app->game->mapSecretsFound = 0;

	if (this->tileEvents) {
		delete this->tileEvents;
	}
	this->tileEvents = nullptr;

	if (this->mapByteCode) {
		delete this->mapByteCode;
	}
	this->mapByteCode = nullptr;

	if (this->skyMapTexels) {
		delete this->skyMapTexels;
	}
	this->skyMapTexels = nullptr;

	if (this->skyMapPalette) {
		delete[] this->skyMapPalette;
	}
	this->skyMapPalette = nullptr;

	for (int i = 0; i < 12; ++i) {
		this->staticFuncs[i] = -1;
	}

	if (app->canvas->loadMapStringID != -1) {
		app->localization->unloadText(app->canvas->loadMapStringID);
	}
	app->canvas->loadMapStringID = -1;

	for (int j = 0; j < Render::MAX_CUSTOM_SPRITES; ++j) {
		this->customSprites[j] = -1;
	}
	for (int k = 0; k < Render::MAX_DROP_SPRITES; ++k) {
		this->dropSprites[k] = -1;
	}
}

// Geometry-only teardown used by Render::reloadGeometryOnly. Mirrors the
// geometry portion of unloadMap() but preserves mediaMappings / mediaPalettes
// / mediaTexels / GPU textures and the sky. 95% of the teardown cost comes
// from media; keeping it intact is the whole point of the fast reload path.
void Render::unloadGeometryOnly() {
	if (this->normals)          { delete this->normals;           this->normals = nullptr; }
	if (this->nodeNormalIdxs)   { delete this->nodeNormalIdxs;    this->nodeNormalIdxs = nullptr; }
	if (this->nodeOffsets)      { delete this->nodeOffsets;       this->nodeOffsets = nullptr; }
	if (this->nodeChildOffset1) { delete this->nodeChildOffset1;  this->nodeChildOffset1 = nullptr; }
	if (this->nodeChildOffset2) { delete this->nodeChildOffset2;  this->nodeChildOffset2 = nullptr; }
	if (this->nodeSprites)      { delete this->nodeSprites;       this->nodeSprites = nullptr; }
	if (this->nodeBounds)       { delete this->nodeBounds;        this->nodeBounds = nullptr; }
	if (this->nodePolys)        { delete this->nodePolys;         this->nodePolys = nullptr; }
	if (this->lineFlags)        { delete this->lineFlags;         this->lineFlags = nullptr; }
	if (this->lineXs)           { delete this->lineXs;            this->lineXs = nullptr; }
	if (this->lineYs)           { delete this->lineYs;            this->lineYs = nullptr; }
	if (this->heightMap)        { delete this->heightMap;         this->heightMap = nullptr; }
	this->numMapSprites = 0;
	if (this->mapSprites)       { delete this->mapSprites;        this->mapSprites = nullptr; }
	if (this->mapSpriteInfo)    { delete this->mapSpriteInfo;     this->mapSpriteInfo = nullptr; }
	this->numTileEvents = 0;
	this->mapByteCodeSize = 0;
	app->game->mapSecretsFound = 0;
	if (this->tileEvents)       { delete this->tileEvents;        this->tileEvents = nullptr; }
	if (this->mapByteCode)      { delete this->mapByteCode;       this->mapByteCode = nullptr; }
	for (int i = 0; i < 12; ++i) this->staticFuncs[i] = -1;
	for (int j = 0; j < Render::MAX_CUSTOM_SPRITES; ++j) this->customSprites[j] = -1;
	for (int k = 0; k < Render::MAX_DROP_SPRITES;  ++k) this->dropSprites[k]  = -1;
}

bool Render::reloadGeometryOnly(const std::vector<uint8_t>& binBytes, int mapNameID) {
	// Hot-reload path used by the in-engine editor. Media + sky are preserved
	// from a prior full load; only geometry / sprites / scripts / map flags
	// change. Body lives in Render::loadMap — this wrapper exists so callers
	// don't need to know about LoadOptions.
	return this->loadMap(binBytes, mapNameID, LoadOptions{/*keepMedia=*/true, /*keepSky=*/true});
}

void Render::RegisterMedia(int n) {
	short mappingsBeg = this->mediaMappings[n];
	short mappingsEnd = this->mediaMappings[n + 1];
	for (; mappingsBeg < mappingsEnd; mappingsBeg++) {
		int palIndex = mappingsBeg;
		if ((this->mediaPalColors[palIndex] & Render::MEDIA_FLAG_REFERENCE) != 0x0) {
			palIndex = (this->mediaPalColors[palIndex] & 0x3FF);
		}
		this->mediaPalColors[palIndex] |= Render::MEDIA_PALETTE_REGISTERED;

		int texelIndex = mappingsBeg;
		if ((this->mediaTexelSizes[texelIndex] & Render::MEDIA_FLAG_REFERENCE) != 0x0) {
			texelIndex = (this->mediaTexelSizes[texelIndex] & 0x3FF);
		}
		this->mediaTexelSizes[texelIndex] |= Render::MEDIA_TEXELS_REGISTERED;
	}
}

void Render::FinalizeMedia() {

	app->canvas->updateLoadingBar(false);
	this->texelMemoryUsage = 0;
	this->paletteMemoryUsage = 0;

	// Load palettes and texels from converter output
	DataNode palYaml = DataNode::loadFile("levels/textures/media_palettes.yaml");
	DataNode texYaml = DataNode::loadFile("levels/textures/media_texels.yaml");

	if (palYaml && texYaml) {
		this->FinalizeMediaFromYaml(palYaml, texYaml);
	} else {
		app->Error("media_palettes.yaml or media_texels.yaml not found\n");
	}

	this->_gles->CreateAllActiveTextures();
	app->canvas->updateLoadingBar(false);
}

void Render::FinalizeMediaFromYaml(DataNode& palYaml, DataNode& texYaml) {

	// Phase 1: Allocate arrays for registered entries (same as legacy)
	int palIdx = 0;
	int texIdx = 0;
	for (int i = 0; i < 1024; ++i) {
		bool texReg = (this->mediaTexelSizes[i] & Render::MEDIA_TEXELS_REGISTERED) != 0;
		bool palReg = (this->mediaPalColors[i] & Render::MEDIA_PALETTE_REGISTERED) != 0;

		if (texReg) {
			int size = (this->mediaTexelSizes[i] & 0x3FFFFFFF) + 1;
			this->texelMemoryUsage += size;
			this->mediaTexelSizes2[texIdx] = size;
			this->mediaTexels[texIdx] = new uint8_t[size]();
			texIdx++;
		}
		if (palReg) {
			int numColors = this->mediaPalColors[i] & 0x3FFFFFFF;
			this->paletteMemoryUsage += 4 * numColors;
			this->mediaPalettesSizes[palIdx] = numColors;
			this->mediaPalettes[palIdx] = new uint16_t[numColors]();
			palIdx++;
		}
	}
	app->canvas->updateLoadingBar(false);

	// Phase 2: Load palettes from YAML
	// Build a map from media ID → resolved palette data (handling references)
	std::unordered_map<int, DataNode> palDataMap;
	for (auto it = palYaml.begin(); it != palYaml.end(); ++it) {
		int mid = it.key().asInt(-1);
		if (mid >= 0) palDataMap[mid] = it.value();
	}

	int n5 = 0;
	for (int j = 0; j < 1024; ++j) {
		bool registered = (this->mediaPalColors[j] & Render::MEDIA_PALETTE_REGISTERED) != 0;
		bool isRef = (this->mediaPalColors[j] & Render::MEDIA_FLAG_REFERENCE) != 0;

		if (registered && !isRef) {
			// Load palette colors from YAML
			int numColors = this->mediaPalColors[j] & 0x3FFFFFFF;
			auto pit = palDataMap.find(j);
			if (pit != palDataMap.end()) {
				DataNode colors = pit->second["colors"];
				if (colors && colors.isSequence()) {
					for (int k = 0; k < numColors && k < colors.size(); k++) {
						this->mediaPalettes[n5][k] = (uint16_t)colors[k].asInt(0);
					}
				}
			} else {
				LOG_WARN("[render] No palette data for registered media {} ({} colors)\n", j, numColors);
			}

			// Resolve references (same logic as legacy)
			int refMarker = (j | Render::MEDIA_FLAG_REFERENCE);
			for (int l = j + 1; l < 1024; ++l) {
				if (this->mediaPalColors[l] == refMarker) {
					this->mediaPalColors[l] = (0xC0000000 | n5);
				}
			}
			this->mediaPalColors[j] = (0x40000000 | n5);
			++n5;
		}
		if ((j & 0x1E) == 0x1E) {
			app->canvas->updateLoadingBar(false);
		}
	}

	// Phase 3: Load texels from .dat files
	std::unordered_map<int, DataNode> texDataMap;
	for (auto it = texYaml.begin(); it != texYaml.end(); ++it) {
		int mid = it.key().asInt(-1);
		if (mid >= 0) texDataMap[mid] = it.value();
	}

	int n9 = 0;
	for (int j = 0; j < 1024; ++j) {
		bool registered = (this->mediaTexelSizes[j] & Render::MEDIA_TEXELS_REGISTERED) != 0;
		bool isRef = (this->mediaTexelSizes[j] & Render::MEDIA_FLAG_REFERENCE) != 0;
		int texelSize = (this->mediaTexelSizes[j] & 0x3FFFFFFF) + 1;

		if (registered && !isRef) {
			auto tit = texDataMap.find(j);
			if (tit != texDataMap.end()) {
				DataNode fileNode = tit->second["file"];
				if (fileNode) {
					std::string filePath = "levels/textures/media_texels/" + fileNode.asString("");
					InputStream texIs;
					if (texIs.loadFile(filePath.c_str(), InputStream::LOADTYPE_RESOURCE)) {
						int readSize = std::min(texelSize, texIs.getFileSize());
						memcpy(this->mediaTexels[n9], texIs.getData(), readSize);
						texIs.close();
					} else {
						LOG_WARN("[render] Failed to load texel file: {} (media {})\n", filePath.c_str(), j);
					}
				}
			} else {
				LOG_WARN("[render] No texel data for registered media {}\n", j);
			}

			// Water animation fixups (same MD5 checks as legacy)
			if (j == 814) {
				if (checkFileMD5Hash(this->mediaTexels[n9], texelSize, 0x2AFCC8EDC9EA8610, 0xEA5458CBEBF345EC))
					this->fixWaterAnim1 = true;
			} else if (j == 815) {
				if (checkFileMD5Hash(this->mediaTexels[n9], texelSize, 0x14F8BC466131E9AB, 0xFBEC94B21422E569))
					this->fixWaterAnim2 = true;
			} else if (j == 816) {
				if (checkFileMD5Hash(this->mediaTexels[n9], texelSize, 0xB784065E177D596E, 0x23729441F971FEE7))
					this->fixWaterAnim3 = true;
			} else if (j == 817) {
				if (checkFileMD5Hash(this->mediaTexels[n9], texelSize, 0x8A53E5E7CD062F73, 0xFAA5B42239006DC8))
					this->fixWaterAnim4 = true;
			}

			// Resolve references (same logic as legacy)
			int refMarker = (j | Render::MEDIA_FLAG_REFERENCE);
			for (int l = j + 1; l < 1024; ++l) {
				if (this->mediaTexelSizes[l] == refMarker) {
					this->mediaTexelSizes[l] = (0xC0000000 | n9);
				}
			}
			this->mediaTexelSizes[j] = (0x40000000 | n9);
			++n9;
		}
		if ((j & 0xF) == 0xF) {
			app->canvas->updateLoadingBar(false);
		}
	}

	LOG_INFO("[render] Loaded media from YAML: {} palettes, {} texels\n", n5, n9);
	this->nextPalIdx = n5;
	this->nextTexIdx = n9;
}

bool Render::registerAndFinalizeMedia(int id) {
	if (id <= 0 || id >= Render::MEDIA_MAX_MAPPINGS) return false;

	// media_palettes.yaml / media_texels.yaml are multi-MB; parsing them on
	// every call made a single weapon placement (which drags in ~10 projectile
	// sprites) spend seconds re-loading the same data. Cache once per session.
	static DataNode palYaml;
	static DataNode texYaml;
	static bool yamlsLoaded = false;
	if (!yamlsLoaded) {
		palYaml = DataNode::loadFile("levels/textures/media_palettes.yaml");
		texYaml = DataNode::loadFile("levels/textures/media_texels.yaml");
		yamlsLoaded = true;
	}
	if (!palYaml || !texYaml) {
		LOG_WARN("[render] registerAndFinalizeMedia({}): media YAMLs missing\n", id);
		return false;
	}

	short mappingsBeg = this->mediaMappings[id];
	short mappingsEnd = this->mediaMappings[id + 1];

	for (short m = mappingsBeg; m < mappingsEnd; ++m) {
		// --- Palette slot ---
		int palBase = m;
		if ((this->mediaPalColors[palBase] & Render::MEDIA_FLAG_REFERENCE) != 0) {
			// Already-resolved ref points at (0xC0000000 | compactedIdx) so
			// it's finalized. Raw refs (before finalize) have low bits as the
			// underlying id; follow those to the target slot.
			if ((this->mediaPalColors[palBase] & 0x40000000) == 0) {
				palBase = this->mediaPalColors[palBase] & 0x3FF;
			}
		}
		if ((this->mediaPalColors[palBase] & 0x40000000) == 0) {
			int numColors = this->mediaPalColors[palBase] & 0x3FFFFFFF;
			int idx = this->nextPalIdx++;
			this->paletteMemoryUsage += 4 * numColors;
			this->mediaPalettesSizes[idx] = numColors;
			this->mediaPalettes[idx] = new uint16_t[numColors]();

			// Load palette colours from media_palettes.yaml
			DataNode entry = palYaml[palBase];
			if (entry) {
				DataNode colors = entry["colors"];
				if (colors && colors.isSequence()) {
					int n = std::min(numColors, (int)colors.size());
					for (int k = 0; k < n; ++k) {
						this->mediaPalettes[idx][k] = (uint16_t)colors[k].asInt(0);
					}
				}
			} else {
				LOG_WARN("[render] registerAndFinalizeMedia: no palette data for {}\n", palBase);
			}

			// Resolve any forward-range refs that pointed at this slot — their
			// entries still hold the raw ref marker from media_mappings.yaml.
			int refMarker = (palBase | Render::MEDIA_FLAG_REFERENCE);
			for (int l = palBase + 1; l < 1024; ++l) {
				if (this->mediaPalColors[l] == refMarker) {
					this->mediaPalColors[l] = (0xC0000000 | idx);
				}
			}
			this->mediaPalColors[palBase] = (0x40000000 | idx);
		}

		// --- Texel slot ---
		int texBase = m;
		if ((this->mediaTexelSizes[texBase] & Render::MEDIA_FLAG_REFERENCE) != 0) {
			if ((this->mediaTexelSizes[texBase] & 0x40000000) == 0) {
				texBase = this->mediaTexelSizes[texBase] & 0x3FF;
			}
		}
		if ((this->mediaTexelSizes[texBase] & 0x40000000) == 0) {
			int texelSize = (this->mediaTexelSizes[texBase] & 0x3FFFFFFF) + 1;
			int idx = this->nextTexIdx++;
			this->texelMemoryUsage += texelSize;
			this->mediaTexelSizes2[idx] = texelSize;
			this->mediaTexels[idx] = new uint8_t[texelSize]();

			// Copy texel bytes from the .dat file referenced by media_texels.yaml.
			DataNode entry = texYaml[texBase];
			if (entry) {
				DataNode fileNode = entry["file"];
				if (fileNode) {
					std::string filePath = "levels/textures/media_texels/" + fileNode.asString("");
					InputStream texIs;
					if (texIs.loadFile(filePath.c_str(), InputStream::LOADTYPE_RESOURCE)) {
						int readSize = std::min(texelSize, texIs.getFileSize());
						memcpy(this->mediaTexels[idx], texIs.getData(), readSize);
						texIs.close();
					} else {
						LOG_WARN("[render] registerAndFinalizeMedia: texel load failed for {}: {}\n", texBase, filePath);
					}
				}
			} else {
				LOG_WARN("[render] registerAndFinalizeMedia: no texel data for {}\n", texBase);
			}

			int refMarker = (texBase | Render::MEDIA_FLAG_REFERENCE);
			for (int l = texBase + 1; l < 1024; ++l) {
				if (this->mediaTexelSizes[l] == refMarker) {
					this->mediaTexelSizes[l] = (0xC0000000 | idx);
				}
			}
			this->mediaTexelSizes[texBase] = (0x40000000 | idx);
		}
	}
	return true;
}

bool Render::loadSkyFromPng(const std::string& path) {
	InputStream pngIs;
	if (!pngIs.loadFile(path.c_str(), InputStream::LOADTYPE_RESOURCE)) {
		LOG_WARN("[render] Sky texture not found: {}\n", path.c_str());
		return false;
	}

	int w = 0, h = 0, channels = 0;
	uint8_t* rgba = stbi_load_from_memory(pngIs.getData(), pngIs.getFileSize(), &w, &h, &channels, 4);
	pngIs.close();

	if (!rgba) {
		LOG_WARN("[render] Failed to decode sky PNG: {}\n", path.c_str());
		return false;
	}

	// Convert RGBA pixels to indexed texels + RGB565 palette
	// Build palette from unique RGB565 colors
	std::vector<uint16_t> palette;
	std::unordered_map<uint16_t, uint8_t> colorToIndex;

	int totalPixels = w * h;
	this->skyMapTexels = new uint8_t[totalPixels];

	for (int i = 0; i < totalPixels; i++) {
		uint8_t r = rgba[i * 4 + 0];
		uint8_t g = rgba[i * 4 + 1];
		uint8_t b = rgba[i * 4 + 2];
		// Convert to RGB565
		uint16_t c565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

		auto it = colorToIndex.find(c565);
		if (it != colorToIndex.end()) {
			this->skyMapTexels[i] = it->second;
		} else {
			uint8_t idx = (uint8_t)palette.size();
			if (palette.size() >= 256) {
				// More than 256 unique colors - clamp to last
				idx = 255;
			} else {
				colorToIndex[c565] = idx;
				palette.push_back(c565);
			}
			this->skyMapTexels[i] = idx;
		}
	}

	stbi_image_free(rgba);

	// Allocate palette with 16 fog levels
	int palSize = (int)palette.size();
	this->skyMapPalette = new uint16_t[palSize];
	memcpy(this->skyMapPalette, palette.data(), palSize * sizeof(uint16_t));

	LOG_INFO("[render] Loaded sky from PNG: {} ({}x{}, {} colors)\n", path.c_str(), w, h, palSize);
	return true;
}

bool Render::beginLoadMap(int mapNameID) {
	// Full reload from disk. Reads the compiled .bin into a buffer, then
	// delegates to the shared loadMap body. All per-map init (level strings,
	// mapFlags clear, media arrays alloc, FinalizeMedia, sky, palette banks)
	// runs inside loadMap under `LoadOptions{}` (all-defaults = full reload).
	std::string mapFilePath = this->gameConfig->getMapFile(mapNameID);
	const char* mapFile = CAppContainer::getInstance()->customMapFile
	    ? CAppContainer::getInstance()->customMapFile
	    : mapFilePath.c_str();

	InputStream IS;
	IS.loadResource(mapFile);
	const int sz = IS.getFileSize();
	if (sz <= 0) return false;
	std::vector<uint8_t> bytes(IS.getData(), IS.getData() + sz);
	IS.close();

	return this->loadMap(bytes, mapNameID, LoadOptions{});
}

bool Render::loadMap(const std::vector<uint8_t>& bin, int mapNameID, LoadOptions opts) {
	// Canonical map loader. See Render.h for LoadOptions. The full path
	// (opts = {}) is what beginLoadMap calls; the hot path
	// (keepMedia=keepSky=true) is what reloadGeometryOnly calls. No other
	// entry points should exist — every engine step (postProcessSprites,
	// height_map override, loadMayaCameras, tile-event indexing, etc.) lives
	// here once so the two callers can't drift out of sync.

	InputStream IS;
	if (!IS.loadFromBuffer(bin.data(), int(bin.size()))) return false;

	this->mapNameID = mapNameID;

	// --- Per-session init (full reload only; hot path keeps prior state) ---
	if (!opts.keepMedia) {
		app->canvas->loadMapStringID = (short)(4 + (this->mapNameID - 1));
		app->localization->loadText(app->canvas->loadMapStringID);

		for (int i = 0; i < 1024; ++i) this->mapFlags[i] = 0;

		this->mapEntranceAutomap = -1;
		this->mapExitAutomap = -1;
		for (int i = 0; i < Render::MAX_LADDERS_PER_MAP; ++i) this->mapLadders[i] = -1;
		for (int i = 0; i < Render::MAX_KEEP_PITCH_LEVEL_TILES; ++i) this->mapKeepPitchLevelTiles[i] = -1;

		this->portalState = Render::PORTAL_DNE;
		this->previousPortalState = 0;
		this->portalScripted = false;
		this->mapNameField = (0xC00 | app->game->levelNames[this->mapNameID - 1]);

		// Media mapping / palette / texel index arrays. Retained across hot
		// reloads — allocated here once per full reload.
		this->mediaMappings = new short[Render::MEDIA_MAX_MAPPINGS];
		this->mediaDimensions = new uint8_t[Render::MEDIA_MAX_IMAGES];
		this->mediaBounds = new short[(Render::MEDIA_MAX_IMAGES * 4)];
		this->mediaPalColors = new int[Render::MEDIA_MAX_IMAGES];
		this->mediaPalettesSizes = new int[Render::MEDIA_MAX_IMAGES];
		this->mediaTexelSizes = new int[Render::MEDIA_MAX_IMAGES];
		this->mediaTexelSizes2 = new int[Render::MEDIA_MAX_IMAGES];
		this->mediaTexels = new uint8_t*[1024]();
		this->mediaPalettes = new uint16_t*[1024]();

		app->canvas->updateLoadingBar(false);

		DataNode mediaMappingsYaml = DataNode::loadFile("levels/textures/media_mappings.yaml");
		if (mediaMappingsYaml) {
			DataNode mNode = mediaMappingsYaml["mappings"];
			DataNode dNode = mediaMappingsYaml["dimensions"];
			DataNode bNode = mediaMappingsYaml["bounds"];
			DataNode pNode = mediaMappingsYaml["pal_colors"];
			DataNode tNode = mediaMappingsYaml["texel_sizes"];
			for (int i = 0; i < Render::MEDIA_MAX_MAPPINGS && i < mNode.size(); i++)
				this->mediaMappings[i] = (short)mNode[i].asInt(0);
			for (int i = 0; i < Render::MEDIA_MAX_IMAGES && i < dNode.size(); i++)
				this->mediaDimensions[i] = (uint8_t)dNode[i].asInt(0);
			for (int i = 0; i < Render::MEDIA_MAX_IMAGES && i < bNode.size(); i++) {
				DataNode row = bNode[i];
				int base = i * 4;
				this->mediaBounds[base + 0] = (short)row[0].asInt(0);
				this->mediaBounds[base + 1] = (short)row[1].asInt(0);
				this->mediaBounds[base + 2] = (short)row[2].asInt(0);
				this->mediaBounds[base + 3] = (short)row[3].asInt(0);
			}
			for (int i = 0; i < Render::MEDIA_MAX_IMAGES && i < pNode.size(); i++)
				this->mediaPalColors[i] = pNode[i].asInt(0);
			for (int i = 0; i < Render::MEDIA_MAX_IMAGES && i < tNode.size(); i++)
				this->mediaTexelSizes[i] = tNode[i].asInt(0);
		} else {
			app->Error("media_mappings.yaml not found\n");
		}
		// Readjust the sky box index so it doesn't interfere with the fade texture.
		this->mediaMappings[TILE_SKY_BOX] = (gles::MAX_MEDIA - 1);

		app->canvas->updateLoadingBar(false);
	}

	// --- Pass 1: header + counts ---
	app->resource->read(&IS, 42);
	if (app->resource->shiftUByte() != 3) {
		app->Error(68); // ERR_BADMAPVERSION
		return false;
	}
	this->mapCompileDate = app->resource->shiftInt();
	this->mapSpawnIndex = app->resource->shiftUShort();
	this->mapSpawnDir = app->resource->shiftUByte();
	this->mapFlagsBitmask = app->resource->shiftByte();
	app->game->totalSecrets = app->resource->shiftByte();
	app->game->totalLoot = app->resource->shiftUByte();
	this->numNodes = app->resource->shiftUShort();
	int dataSizePolys = app->resource->shiftUShort();
	this->numLines = app->resource->shiftUShort();
	this->numNormals = app->resource->shiftUShort();
	this->numNormalSprites = app->resource->shiftUShort();
	this->numZSprites = app->resource->shiftShort();

	// Bin-side sprite counts (numNormalSprites/numZSprites may be overridden
	// by the YAML sprites block below, but binNumMapSprites drives the
	// pass-2 byte read from the file).
	const int binNumNormalSprites = this->numNormalSprites;
	const int binNumZSprites = this->numZSprites;
	const int binNumMapSprites = binNumNormalSprites + binNumZSprites;

	// level.yaml overrides may swap in YAML-driven sprites (monsters, doors,
	// NPCs) whose count replaces numNormalSprites/numZSprites.
	bool spritesFromYaml = false;
	DataNode yamlSpritesNode;
	DataNode levelYaml;
	this->loadMapLevelOverrides(mapNameID, levelYaml, spritesFromYaml, yamlSpritesNode);

	this->numMapSprites = this->numNormalSprites + this->numZSprites;
	this->numSprites = this->numMapSprites + Render::MAX_CUSTOM_SPRITES + Render::MAX_DROP_SPRITES;
	this->numTileEvents = (int)app->resource->shiftShort();
	this->mapByteCodeSize = (int)app->resource->shiftShort();
	app->game->totalMayaCameras = app->resource->shiftByte();
	app->game->totalMayaCameraKeys = app->resource->shiftShort();
	app->canvas->updateLoadingBar(false);

	short totalMayaTweens = 0;
	for (int l = 0; l < 6; ++l) {
		app->game->ofsMayaTween[l] = totalMayaTweens;
		short s = app->resource->shiftShort();
		if (s != -1) totalMayaTweens += s;
	}
	app->game->totalMayaTweens = totalMayaTweens;

	// --- Media section header (always read; register only on full path) ---
	app->resource->readMarker(&IS, 0xDEADBEEF);
	app->resource->read(&IS, 2);
	int mediaCount = app->resource->shiftUShort();
	app->resource->read(&IS, mediaCount * 2);
	if (opts.keepMedia) {
		// Advance past the media IDs without registering — existing media
		// state is preserved from the prior full load (and the editor
		// registers any new IDs incrementally via registerAndFinalizeMedia
		// before calling this).
		for (int i = 0; i < mediaCount; ++i) app->resource->shiftUShort();
	} else {
		for (int i = 0; i < mediaCount; ++i) this->RegisterMedia(app->resource->shiftUShort());
	}
	app->resource->readMarker(&IS, 0xDEADBEEF);
	IS.close();

	if (!opts.keepMedia) {
		DataNode yamlMedia = levelYaml ? levelYaml["textures"] : DataNode();
		if (yamlMedia && yamlMedia.isSequence() && yamlMedia.size() > 0) {
			// Clear prior registration bits and re-register from YAML. Only
			// valid on full reload; hot reload would discard already-loaded
			// palettes/texels here.
			for (int i = 0; i < Render::MEDIA_MAX_IMAGES; i++) {
				this->mediaPalColors[i] &= ~0x40000000;
				this->mediaTexelSizes[i] &= ~0x40000000;
			}
			for (int i = 0; i < (int)yamlMedia.size(); i++) {
				std::string val = yamlMedia[i].asString("");
				int tileId = 0;
				if (!val.empty() && (val[0] < '0' || val[0] > '9')) {
					tileId = SpriteDefs::getIndex(val);
				} else {
					tileId = yamlMedia[i].asInt(0);
				}
				if (tileId > 0) this->RegisterMedia(tileId);
			}
		}
		this->FinalizeMedia();
	}

	// --- Allocate geometry ---
	// Hot path already has arrays from the prior load; free them without
	// touching media. The "free" sets numMapSprites/numTileEvents/
	// mapByteCodeSize back to 0 as part of its bookkeeping — stash the
	// values parsed from this bin's header so the allocations below use
	// them.
	const int numMapSpritesLocal  = this->numMapSprites;
	const int numTileEventsLocal  = this->numTileEvents;
	const int mapByteCodeSizeLocal = this->mapByteCodeSize;
	if (opts.keepMedia) {
		unloadGeometryOnly();
		this->numMapSprites   = numMapSpritesLocal;
		this->numTileEvents   = numTileEventsLocal;
		this->mapByteCodeSize = mapByteCodeSizeLocal;
		this->numSprites = this->numMapSprites + Render::MAX_CUSTOM_SPRITES + Render::MAX_DROP_SPRITES;
	}

	this->nodeNormalIdxs = new uint8_t[this->numNodes];
	this->nodeOffsets = new short[this->numNodes];
	this->nodeChildOffset1 = new short[this->numNodes];
	this->nodeChildOffset2 = new short[this->numNodes];
	this->nodeSprites = new short[this->numNodes];
	this->nodeBounds = new uint8_t[this->numNodes * 4];
	this->nodePolys = new uint8_t[dataSizePolys];
	this->lineFlags = new uint8_t[(this->numLines + 1) / 2];
	this->lineXs = new uint8_t[this->numLines * 2];
	this->lineYs = new uint8_t[this->numLines * 2];
	this->normals = new short[this->numNormals * 3];
	this->heightMap = new uint8_t[1024];
	for (int i = 0; i < this->numNodes; ++i) this->nodeSprites[i] = -1;

	this->mapSprites = new short[this->numSprites * 10];
	for (int i = 0; i < this->numSprites * 10; ++i) this->mapSprites[i] = 0;
	this->mapSpriteInfo = new int[this->numSprites * 2];
	// NOTE: the second init loop writes mapSprites (not mapSpriteInfo) — this
	// matches the original code exactly. It's redundant on top of the loop
	// above and does NOT zero mapSpriteInfo, but since every slot that ends
	// up being rendered gets mapSpriteInfo written later (bin read or YAML
	// sprites), this has never produced a visible bug. Left alone to avoid
	// changing behaviour during the unification refactor.
	for (int i = 0; i < this->numSprites * 2; ++i) this->mapSprites[i] = 0;

	this->S_X = this->numSprites * 0;
	this->S_Y = this->numSprites * 1;
	this->S_Z = this->numSprites * 2;
	this->S_RENDERMODE = this->numSprites * 3;
	this->S_NODE = this->numSprites * 4;
	this->S_NODENEXT = this->numSprites * 5;
	this->S_VIEWNEXT = this->numSprites * 6;
	this->S_ENT = this->numSprites * 7;
	this->S_SCALEFACTOR = this->numSprites * 8;
	this->SINFO_SORTZ = this->numSprites;

	this->tileEvents = new int[this->numTileEvents * 2];
	this->mapByteCode = new uint8_t[this->mapByteCodeSize];
	app->game->mayaCameras = new MayaCamera[app->game->totalMayaCameras];
	app->game->mayaCameraKeys = new short[app->game->totalMayaCameraKeys * 7];
	app->game->mayaCameraTweens = new int8_t[app->game->totalMayaTweens];
	app->game->mayaTweenIndices = new short[app->game->totalMayaCameraKeys * 6];
	app->game->setKeyOffsets();

	// --- Pass 2: re-read from the top, fill geometry arrays ---
	IS.loadFromBuffer(bin.data(), int(bin.size()));
	app->canvas->updateLoadingBar(false);

	app->resource->read(&IS, 42);
	app->resource->readMarker(&IS, 0xDEADBEEF);
	app->resource->read(&IS, mediaCount * 2 + 2);
	app->resource->readMarker(&IS, 0xDEADBEEF);

	app->resource->readShortArray(&IS, std::span(this->normals, this->numNormals * 3));
	app->resource->readMarker(&IS);
	app->resource->readShortArray(&IS, std::span(this->nodeOffsets, this->numNodes));
	app->resource->readMarker(&IS);
	app->resource->readByteArray(&IS, std::span(this->nodeNormalIdxs, this->numNodes));
	app->resource->readMarker(&IS);
	app->resource->readShortArray(&IS, std::span(this->nodeChildOffset1, this->numNodes));
	app->resource->readShortArray(&IS, std::span(this->nodeChildOffset2, this->numNodes));
	app->resource->readMarker(&IS);
	app->resource->readByteArray(&IS, std::span(this->nodeBounds, this->numNodes * 4));
	app->resource->readMarker(&IS);
	app->canvas->updateLoadingBar(false);
	app->resource->readByteArray(&IS, std::span(this->nodePolys, dataSizePolys));
	app->resource->readMarker(&IS);
	app->resource->readByteArray(&IS, std::span(this->lineFlags, (this->numLines + 1) / 2));
	app->resource->readByteArray(&IS, std::span(this->lineXs, this->numLines * 2));
	app->resource->readByteArray(&IS, std::span(this->lineYs, this->numLines * 2));
	app->resource->readMarker(&IS);
	app->resource->readByteArray(&IS, std::span(this->heightMap, 1024));
	app->resource->readMarker(&IS);
	app->canvas->updateLoadingBar(false);

	// Sprites — bin counts drive the byte read even when YAML overrides the
	// sprite list later; we have to consume those bytes either way.
	app->resource->readCoordArray(&IS, std::span(this->mapSprites + this->S_X, binNumMapSprites));
	app->resource->readCoordArray(&IS, std::span(this->mapSprites + this->S_Y, binNumMapSprites));
	app->canvas->updateLoadingBar(false);

	for (int i = 0; i < this->numMapSprites; i++) {
		this->mapSprites[i + this->S_NODE] = -1;
		this->mapSprites[i + this->S_NODENEXT] = -1;
		this->mapSprites[i + this->S_VIEWNEXT] = -1;
		this->mapSprites[i + this->S_ENT] = -1;
		this->mapSprites[i + this->S_SCALEFACTOR] = 64;
		this->mapSprites[i + this->S_Z] = Render::SPRITE_Z_DEFAULT;
	}

	{
		int n5 = 0;
		int rem = binNumMapSprites;
		while (rem > 0) {
			int n6 = (Resource::IO_SIZE > rem) ? rem : Resource::IO_SIZE;
			rem -= n6;
			app->resource->read(&IS, n6);
			while (--n6 >= 0) this->mapSpriteInfo[n5++] = app->resource->shiftUByte();
		}
		app->resource->readMarker(&IS);
		app->canvas->updateLoadingBar(false);

		int n7 = 0;
		int rem2 = binNumMapSprites;
		while (rem2 > 0) {
			int n8 = ((Resource::IO_SIZE / 2) > rem2) ? rem2 : (Resource::IO_SIZE / 2);
			rem2 -= n8;
			app->resource->read(&IS, n8 * 2);
			while (--n8 >= 0) this->mapSpriteInfo[n7++] |= (app->resource->shiftUShort() & 0xFFFF) << 16;
		}
		app->resource->readMarker(&IS);
		app->resource->readUByteArray(&IS, std::span(this->mapSprites + this->S_Z + binNumNormalSprites, binNumZSprites));
		app->resource->readMarker(&IS);

		int binNormIdx = binNumNormalSprites;
		int binZRem = binNumZSprites;
		while (binZRem > 0) {
			int n10 = (Resource::IO_SIZE > binZRem) ? binZRem : Resource::IO_SIZE;
			binZRem -= n10;
			app->resource->read(&IS, n10);
			while (--n10 >= 0) this->mapSpriteInfo[binNormIdx++] |= app->resource->shiftUByte() << 8;
		}
		app->canvas->updateLoadingBar(false);
		app->resource->readMarker(&IS);
	}

	// YAML sprites replace the bin sprite data for level.yaml-driven maps.
	if (spritesFromYaml) this->loadSpritesFromYaml(yamlSpritesNode);

	app->resource->readUShortArray(&IS, std::span(this->staticFuncs, 12));
	app->resource->readMarker(&IS);

	// Tile events + mark event tiles in mapFlags. readIntArray takes N ints.
	app->resource->readIntArray(&IS, std::span(this->tileEvents, this->numTileEvents * 2));
	for (int i = 0; i < this->numTileEvents; i++) {
		int index = this->tileEvents[i << 1] & 0x3FF;
		this->mapFlags[index] |= 0x40;
	}
	app->resource->readMarker(&IS);

	if (this->mapByteCodeSize > 0) {
		app->resource->readByteArray(&IS, std::span(this->mapByteCode, this->mapByteCodeSize));
	}
	app->resource->readMarker(&IS);
	app->canvas->updateLoadingBar(false);

	// Maya cameras. For editor bins with 0 cameras, the loop body in
	// loadMayaCameras doesn't execute — so both shipped and editor bin
	// formats are handled uniformly here.
	app->game->loadMayaCameras(&IS);
	app->resource->readMarker(&IS);

	// Camera YAML override (full path only — hot reload preserves the camera
	// state it loaded initially).
	if (!opts.keepMedia) {
		DataNode yamlCameras = levelYaml ? levelYaml["cameras"] : DataNode();
		if (yamlCameras && yamlCameras.isSequence() && yamlCameras.size() > 0) {
			this->loadCamerasFromYaml(yamlCameras);
		}
	}

	// Map flags (nibble-packed: 512 bytes → 1024 flags, low then high nibble).
	app->resource->read(&IS, 512);
	int cnt = 0;
	for (int i = 0; i < 512; i++) {
		short flags = app->resource->shiftUByte();
		this->mapFlags[cnt++] |= (uint8_t)(flags & 0xF);
		this->mapFlags[cnt++] |= (uint8_t)((flags >> 4) & 0xF);
	}
	app->resource->readMarker(&IS);
	IS.close();

	// heightMap YAML override — per-tile floor bytes set by level.yaml.
	if (levelYaml) {
		DataNode hm = levelYaml["height_map"];
		if (hm && hm.isSequence()) {
			for (int row = 0; row < 32 && row < (int)hm.size(); row++) {
				DataNode rowNode = hm[row];
				if (!rowNode || !rowNode.isSequence()) continue;
				for (int col = 0; col < 32 && col < (int)rowNode.size(); col++)
					this->heightMap[row * 32 + col] = (uint8_t)rowNode[col].asInt(0);
			}
		}
	}

	this->loadScriptsYaml(mapNameID);

	app->canvas->updateLoadingBar(false);
	// Attach sprites to BSP leaves — without this, placed entities sit in
	// mapSprites[] but never get linked into the per-node lists the 3D
	// traversal walks.
	this->postProcessSprites();

	// --- Sky (full reload only) ---
	if (!opts.keepSky) {
		const auto& gc = *this->gameConfig;
		std::string skyTexPath;
		if (auto lit = gc.levelInfos.find(this->mapNameID); lit != gc.levelInfos.end()) {
			skyTexPath = lit->second.skyTexture;
		}
		if (!skyTexPath.empty()) this->loadSkyFromPng(skyTexPath);
	}
	app->canvas->updateLoadingBar(false);

	if (!opts.keepMedia) {
		app->canvas->changeMapStarted = false;
		this->destDizzy = 0;
		this->baseDizzy = 0;
	}

	return true;
}

void Render::renderBSP() {


	this->nodeCount = 0;
	this->leafCount = 0;
	this->lineCount = 0;
	this->lineRasterCount = 0;
	this->spriteCount = 0;
	this->spriteRasterCount = 0;
	this->bspTime = app->upTimeMs;
	this->numVisibleNodes = 0;
	this->numSplitSprites = 0;
	if (!this->skipBSP) {
		this->walkNode(0);
	}

	this->bspTime = app->upTimeMs - this->bspTime;
	app->tinyGL->resetCounters();
	for (int i = this->numVisibleNodes - 1; i >= 0; --i) {
		this->drawNodeGeometry(this->nodeIdxs[i]);
		this->viewSprites = -1;
		this->addNodeSprites(this->nodeIdxs[i]);
		if (!this->skipSprites) {
			for (short viewSprites = this->viewSprites; viewSprites != -1;
			     viewSprites = this->mapSprites[this->S_VIEWNEXT + viewSprites]) {
				++this->spriteCount;
				this->renderSpriteObject(viewSprites);
			}
			// printf("this->spriteCount %d\viewPitch", this->spriteCount);
		}
	}
	for (int j = 0; j <= 1; j++) {
		if (this->useMastermindHack && this->delayedSpriteBuffer[j] != -1) {
			this->renderSpriteObject(this->delayedSpriteBuffer[j]);
		}
	}
	if (this->useCaldexHack && this->delayedSpriteBuffer[2] != -1) {
		this->renderSpriteObject(this->delayedSpriteBuffer[2]);
	}
}
void Render::render(int viewX, int viewY, int viewZ, int viewAngle, int viewPitch, int viewRoll, int viewFov) {
	if (this->headless) { return; }


	// printf("Render::render\n");

	// Original BREW version
	// this->skyMapX = ((1023 - viewAngle) / 4) << 3;
	// this->skyMapY = (((256 - this->screenHeight) / 2) << 3) + (((1023 - viewPitch) / 2) << 3);

	// [GEC] It is adjusted like this to be consistent with the IOS version
	this->skyMapX = ((1023 - (viewAngle & 0x3FF))) << 3;
	this->skyMapY = ((1023 - 0 /*viewPitch*/)) << 3;

	this->currentFrameTime = app->upTimeMs;

	int viewAspect = (viewFov << 14) / ((app->tinyGL->viewportWidth << 14) / app->tinyGL->viewportHeight);
	int n6 = 0;
	int max = std::max(app->player->statusEffects[33], app->player->statusEffects[34]);
	if (max != this->destDizzy) {
		this->baseDizzy = this->destDizzy;
		this->destDizzy = max;
		this->dizzyTime = app->time;
	}
	if (this->destDizzy != this->baseDizzy && this->dizzyTime + 2048 > app->time) {
		max = this->baseDizzy +
		      ((this->destDizzy - this->baseDizzy) * ((app->time - this->dizzyTime << 16) / 2048) >> 16);
	}
	int n7 = (max << 8) / 30;
	if (app->canvas->state == Canvas::ST_CAMERA || app->menuSystem->menu == Menus::MENU_LEVEL_STATS) {
		n7 = 0;
	}
	if (app->game->scriptStateVars[6] != 0) {
		n6 = 8;
	} else if (n7 != 0) {
		n6 = 8 * n7 >> 8;
	}
	if (n6 != 0) {
		viewFov = viewFov - n6 + (n6 * this->sinTable[app->time / 2 & 0x3FF] >> 16);
		viewAspect = viewAspect - n6 + (n6 * this->sinTable[app->time / 2 + 256 & 0x3FF] >> 16);
	}
	if (app->time < this->rockViewTime + this->rockViewDur) {
		int n8 = this->sinTable[(app->time - this->rockViewTime << 16) / (this->rockViewDur << 8) + 256 & 0x3FF] >> 8;
		viewX += n8 * (this->rockViewX - viewX << 8) >> 16;
		viewY += n8 * (this->rockViewY - viewY << 8) >> 16;
		viewZ += n8 * (this->rockViewZ - viewZ << 8) >> 16;
	}
	if (n7 != 0) {
		int time = app->time;
		int n9 = (this->sinTable[time * 5 / 8 & 0x3FF] >> 8) * n7 >> 8;
		int n10 = (this->sinTable[time * 4 / 8 + 256 & 0x3FF] >> 8) * n7 >> 8;
		int n11 = (this->sinTable[time * 6 / 8 & 0x3FF] >> 8) * n7 >> 8;
		int n12 = (this->sinTable[time * 3 / 8 & 0x3FF] >> 8) * n7 >> 8;
		viewX += n9 * 12288 >> 16;
		viewY += n10 * 12288 >> 16;
		viewZ += n11 * 8192 >> 16;
		viewAngle += n12 * 2048 >> 16;
	}
	if (app->time < app->canvas->shakeTime) {
		int n13 = this->sinTable[viewAngle + 256 & 0x3FF];
		viewX += (app->canvas->shakeX << 4) * this->sinTable[viewAngle + 256 + 256 & 0x3FF] >> 16;
		viewY += (app->canvas->shakeX << 4) * -n13 >> 16;
		viewZ += app->canvas->shakeY << 4;
	}
	if (app->player->isFamiliar) {
		viewZ += (this->sinTable[app->time >> 2 & 0x3FF] >> 11) + 128;
	}
	this->viewSin = this->sinTable[viewAngle & 0x3FF];
	this->viewCos = this->sinTable[viewAngle + 256 & 0x3FF];
	int n14 = viewAngle - 256 & 0x3FF;
	this->viewRightStepX = this->sinTable[n14 + 256 & 0x3FF] >> 10;
	this->viewRightStepY = -this->sinTable[n14 & 0x3FF] >> 10;
	if (!this->skipViewNudge) {
		viewX -= 160 * this->viewCos >> 16;
		viewY += 160 * this->viewSin >> 16;
	}

	if (this->chatZoom) { // WolfRpg ?
		Entity* curTarget = app->combat->curTarget;
		if (curTarget && ((curTarget->def->eType == Enums::ET_MONSTER) || (curTarget->def->eType == Enums::ET_NPC))) {
			int sprite = curTarget->getSprite();
			viewX = (-(32 * this->viewCos) >> 12) + (app->render->mapSprites[this->S_X + sprite] << 4);
			viewY = ((32 * this->viewSin) >> 12) + (app->render->mapSprites[this->S_Y + sprite] << 4);
			viewZ = (4 + app->render->mapSprites[sprite + this->S_Z + sprite]) << 4;
			viewPitch = 0;
		}
	}

	this->viewX = viewX;
	this->viewY = viewY;
	this->viewZ = viewZ;
	this->viewAngle = viewAngle;
	app->tinyGL->setView(this->viewX, this->viewY, this->viewZ, viewAngle, viewPitch, viewRoll, viewFov, viewAspect);
	if (this->fogLerpTime != 0) {
		if (app->time < this->fogLerpStart + this->fogLerpTime) {
			int n15 = (app->time - this->fogLerpStart << 12) / this->fogLerpTime;
			app->tinyGL->fogMin = this->baseFogMin + ((this->destFogMin - this->baseFogMin) * n15 >> 12);
			app->tinyGL->fogRange = this->baseFogRange + ((this->destFogRange - this->baseFogRange) * n15 >> 12);
			if (app->tinyGL->fogRange == 0) {
				app->tinyGL->fogRange = 1;
			}
		} else {
			this->fogLerpTime = 0;
			app->tinyGL->fogMin = this->destFogMin;
			app->tinyGL->fogRange = this->destFogRange;
		}
	}
	for (int i = 0; i < this->screenWidth; ++i) {
		app->tinyGL->columnScale[i] = TinyGL::COLUMN_SCALE_INIT;
	}

	// app->game->scriptStateVars[5] = 1; // IOS

	this->clearColorBuffer = app->upTimeMs;
	if (app->canvas->state != Canvas::ST_AUTOMAP) {
		if ((this->renderMode & 0x20) != 0x0) {
			this->_gles->ClearBuffer(0xFFFF00FF);
		} else if (this->skyMapTexels != nullptr && app->game->scriptStateVars[5] != 0 && !(this->renderMode & 0x20)) {
			this->_gles->DrawSkyMap();
		} else {
			int fogColor = 0;
			if (app->tinyGL->fogRange > 1) {
				fogColor = app->tinyGL->fogColor;
			}
			this->_gles->ClearBuffer(fogColor);
		}
	}
	this->clearColorBuffer = app->upTimeMs - this->clearColorBuffer;

	this->portalInView = false;
	this->closestPortalDist = 0x7FFFFFFF;
	this->_gles->DrawSkyMap();
	this->renderBSP();
	if (!this->portalInView) {
		this->portalState = Render::PORTAL_DNE;
		this->previousPortalState = Render::PORTAL_DNE;
	}
	this->frameTime = app->upTimeMs - this->currentFrameTime;
	this->shotsFired = false;

	this->_gles->ResetGLState();
}

// ------------------------------------------------------------------
// Map loading helpers (extracted from beginLoadMap for readability)
// ------------------------------------------------------------------

void Render::loadMapLevelOverrides(int mapNameID, DataNode& levelYaml, bool& spritesFromYaml, DataNode& yamlSpritesNode) {
	const auto& gc = *this->gameConfig;
	auto lit = gc.levelInfos.find(mapNameID);
	if (lit == gc.levelInfos.end()) return;

	levelYaml = DataNode::loadFile(lit->second.configFile.c_str());
	DataNode& lvl = levelYaml;

	// Player spawn point override
	DataNode spawn = lvl ? lvl["player_spawn"] : DataNode();
	if (spawn) {
		int sx = spawn["x"].asInt(0);
		int sy = spawn["y"].asInt(0);
		this->mapSpawnIndex = sy * 32 + sx;

		std::string dir = spawn["dir"].asString("east");
		if (dir == "east")       this->mapSpawnDir = 0;
		else if (dir == "south") this->mapSpawnDir = 2;
		else if (dir == "west")  this->mapSpawnDir = 4;
		else if (dir == "north") this->mapSpawnDir = 6;
		else this->mapSpawnDir = spawn["dir"].asInt(0);
	}

	// Header value overrides
	if (lvl) {
		if (lvl["total_secrets"]) app->game->totalSecrets = lvl["total_secrets"].asInt(app->game->totalSecrets);
		if (lvl["total_loot"]) app->game->totalLoot = lvl["total_loot"].asInt(app->game->totalLoot);
		if (lvl["flags_bitmask"]) this->mapFlagsBitmask = lvl["flags_bitmask"].asInt(this->mapFlagsBitmask);
	}

	DataNode sprites = lvl ? lvl["entities"] : DataNode();
	if (sprites && sprites.isSequence() && sprites.size() > 0) {
		spritesFromYaml = true;
		yamlSpritesNode = sprites;
		int normalCount = 0, zCount = 0;
		for (int i = 0; i < (int)sprites.size(); i++) {
			if (sprites[i]["z"]) zCount++; else normalCount++;
		}
		this->numNormalSprites = normalCount;
		this->numZSprites = zCount;
	}
}

void Render::loadSpritesFromYaml(const DataNode& sprites) {
	int normalIdx = 0, zIdx = this->numNormalSprites;
	for (int i = 0; i < (int)sprites.size(); i++) {
		DataNode s = sprites[i];
		bool isZ = (bool)s["z"];
		int idx = isZ ? zIdx++ : normalIdx++;

		this->mapSprites[this->S_X + idx] = (short)s["x"].asInt(0);
		this->mapSprites[this->S_Y + idx] = (short)s["y"].asInt(0);
		if (isZ) {
			this->mapSprites[this->S_Z + idx] = (short)s["z"].asInt(Render::SPRITE_Z_DEFAULT);
		}

		// Parse tile: accepts sprite name (e.g. "monster_zombie") or numeric ID
		int tile = 0;
		DataNode tileNode = s["tile"];
		if (tileNode) {
			std::string tileStr = tileNode.asString("");
			if (!tileStr.empty() && (tileStr[0] < '0' || tileStr[0] > '9')) {
				tile = SpriteDefs::getIndex(tileStr);
			} else {
				tile = tileNode.asInt(0);
			}
		}
		uint32_t info = 0;
		if (tile >= 257) {
			info |= ((tile - 257) & 0xFF);
			info |= 0x400000;
		} else {
			info |= (tile & 0xFF);
		}

		// Parse flags: supports named list [invisible, flip_h, ...] or hex value
		uint32_t flags = 0;
		DataNode flagsNode = s["flags"];
		if (flagsNode) {
			if (flagsNode.isSequence()) {
				for (int fi = 0; fi < (int)flagsNode.size(); fi++) {
					std::string fname = flagsNode[fi].asString("");
					if (fname == "invisible")   flags |= 0x0001;
					else if (fname == "flip_h")  flags |= 0x0002;
					else if (fname == "animation") flags |= 0x0010;
					else if (fname == "non_entity") flags |= 0x0020;
					else if (fname == "sprite_wall") flags |= 0x0080;
					else if (fname == "south")   flags |= 0x0100;
					else if (fname == "north")   flags |= 0x0200;
					else if (fname == "east")    flags |= 0x0400;
					else if (fname == "west")    flags |= 0x0800;
					else flags |= (uint32_t)flagsNode[fi].asULong(0);
				}
			} else {
				flags = (uint32_t)flagsNode.asULong(0);
			}
		}
		info |= (flags << 16);
		if (isZ) {
			info |= ((s["z_anim"].asInt(0) & 0xFF) << 8);
		}

		this->mapSpriteInfo[idx] = (int)info;
	}
}

void Render::loadCamerasFromYaml(const DataNode& cameras) {
	int numCams = (int)cameras.size();
	app->game->totalMayaCameras = numCams;

	// Count total keys and tween data across all cameras
	int totalKeys = 0;
	int totalTweens = 0;
	for (int c = 0; c < numCams; c++) {
		DataNode cam = cameras[c];
		DataNode keys = cam["keys"];
		int nk = (keys && keys.isSequence()) ? (int)keys.size() : 0;
		totalKeys += nk;
		DataNode tc = cam["tween_counts"];
		if (tc && tc.isSequence()) {
			for (int ch = 0; ch < (int)tc.size() && ch < 6; ch++)
				totalTweens += tc[ch].asInt(0);
		}
	}
	app->game->totalMayaCameraKeys = totalKeys;

	// Reallocate arrays
	delete[] app->game->mayaCameras;
	delete[] app->game->mayaCameraKeys;
	delete[] app->game->mayaTweenIndices;
	delete[] app->game->mayaCameraTweens;
	app->game->mayaCameras = new MayaCamera[numCams];
	app->game->mayaCameraKeys = new short[totalKeys * 7];
	app->game->mayaTweenIndices = new short[totalKeys * 6];
	app->game->mayaCameraTweens = new int8_t[totalTweens > 0 ? totalTweens : 1];
	app->game->setKeyOffsets();

	// Compute tween offsets
	int tweenOffset = 0;
	for (int ch = 0; ch < 6; ch++) {
		app->game->ofsMayaTween[ch] = tweenOffset;
		for (int c = 0; c < numCams; c++) {
			DataNode tc = cameras[c]["tween_counts"];
			if (tc && tc.isSequence() && ch < (int)tc.size())
				tweenOffset += tc[ch].asInt(0);
		}
	}
	app->game->totalMayaTweens = tweenOffset;

	// Populate per-camera data
	int keyOffset = 0;
	int tweenIdxOffset = 0;
	int tweenDataOffset[6] = {};
	for (int ch = 0; ch < 6; ch++) tweenDataOffset[ch] = app->game->ofsMayaTween[ch];

	for (int c = 0; c < numCams; c++) {
		DataNode cam = cameras[c];
		MayaCamera& mc = app->game->mayaCameras[c];
		mc.sampleRate = cam["sample_rate"].asInt(30);
		DataNode keys = cam["keys"];
		int nk = (keys && keys.isSequence()) ? (int)keys.size() : 0;
		mc.numKeys = nk;
		mc.keyOffset = keyOffset;
		mc.complete = false;
		mc.keyThreadResumeCount = nk;

		for (int k = 0; k < nk; k++) {
			DataNode key = keys[k];
			app->game->mayaCameraKeys[app->game->OFS_MAYAKEY_X + keyOffset + k] = (short)key["x"].asInt(0);
			app->game->mayaCameraKeys[app->game->OFS_MAYAKEY_Y + keyOffset + k] = (short)key["y"].asInt(0);
			app->game->mayaCameraKeys[app->game->OFS_MAYAKEY_Z + keyOffset + k] = (short)key["z"].asInt(0);
			app->game->mayaCameraKeys[app->game->OFS_MAYAKEY_PITCH + keyOffset + k] = (short)key["pitch"].asInt(0);
			app->game->mayaCameraKeys[app->game->OFS_MAYAKEY_YAW + keyOffset + k] = (short)key["yaw"].asInt(0);
			app->game->mayaCameraKeys[app->game->OFS_MAYAKEY_ROLL + keyOffset + k] = (short)key["roll"].asInt(0);
			app->game->mayaCameraKeys[app->game->OFS_MAYAKEY_MS + keyOffset + k] = (short)key["ms"].asInt(0);
		}

		DataNode ti = cam["tween_indices"];
		if (ti && ti.isSequence()) {
			for (int k = 0; k < nk && k < (int)ti.size(); k++) {
				DataNode row = ti[k];
				for (int ch = 0; ch < 6; ch++) {
					app->game->mayaTweenIndices[tweenIdxOffset + k * 6 + ch] =
						(row && row.isSequence() && ch < (int)row.size()) ? (short)row[ch].asInt(-1) : -1;
				}
			}
		}
		tweenIdxOffset += nk * 6;

		DataNode td = cam["tween_data"];
		DataNode tc = cam["tween_counts"];
		if (td && td.isSequence() && tc && tc.isSequence()) {
			int tdIdx = 0;
			for (int ch = 0; ch < 6; ch++) {
				int count = (ch < (int)tc.size()) ? tc[ch].asInt(0) : 0;
				for (int j = 0; j < count && tdIdx < (int)td.size(); j++, tdIdx++) {
					app->game->mayaCameraTweens[tweenDataOffset[ch]++] = (int8_t)td[tdIdx].asInt(0);
				}
			}
		}

		keyOffset += nk;
	}
}

void Render::loadScriptsYaml(int mapNameID) {
	const auto& gc = *this->gameConfig;
	auto lit = gc.levelInfos.find(mapNameID);
	if (lit == gc.levelInfos.end()) return;

	std::string scriptsPath = lit->second.dir + "/scripts.yaml";
	DataNode scripts = DataNode::loadFile(scriptsPath.c_str());
	if (!scripts) return;

	// Static functions
	DataNode sf = scripts["static_funcs"];
	if (sf) {
		struct { const char* name; int idx; } sfMap[] = {
			{"init_map",0}, {"end_game",1}, {"boss_75",2}, {"boss_50",3},
			{"boss_25",4}, {"boss_dead",5}, {"per_turn",6}, {"attack_npc",7},
			{"monster_death",8}, {"monster_activate",9}, {"chicken_kicked",10}, {"item_pickup",11}
		};
		for (int i = 0; i < 12; i++) this->staticFuncs[i] = 0xFFFF;
		for (const auto& m : sfMap) {
			DataNode v = sf[m.name];
			if (v) this->staticFuncs[m.idx] = (uint16_t)v.asInt(0xFFFF);
		}
	}

	// Tile events
	DataNode te = scripts["tile_events"];
	if (te && te.isSequence() && te.size() > 0) {
		int numEvts = (int)te.size();
		if (numEvts != this->numTileEvents) {
			delete[] this->tileEvents;
			this->numTileEvents = numEvts;
			this->tileEvents = new int[numEvts * 2];
		}
		for (int i = 0; i < 1024; i++) this->mapFlags[i] &= ~0x40;

		for (int i = 0; i < numEvts; i++) {
			DataNode ev = te[i];
			DataNode tile = ev["tile"];
			int col = tile ? tile["x"].asInt(0) : 0;
			int row = tile ? tile["y"].asInt(0) : 0;
			int tileIdx = row * 32 + col;
			int ip = ev["ip"].asInt(0);
			this->tileEvents[i * 2] = (tileIdx & 0x3FF) | ((ip & 0xFFFF) << 16);

			int param = 0;
			DataNode on = ev["on"];
			if (on && on.isSequence()) {
				for (int j = 0; j < (int)on.size(); j++) {
					std::string s = on[j].asString("");
					if (s == "enter") param |= 0x1;
					else if (s == "exit") param |= 0x2;
					else if (s == "trigger") param |= 0x4;
					else if (s == "face") param |= 0x8;
				}
			}
			DataNode dir = ev["dir"];
			if (dir && dir.isSequence()) {
				for (int j = 0; j < (int)dir.size(); j++) {
					std::string s = dir[j].asString("");
					if (s == "east") param |= 0x010;
					else if (s == "northeast") param |= 0x020;
					else if (s == "north") param |= 0x040;
					else if (s == "northwest") param |= 0x080;
					else if (s == "west") param |= 0x100;
					else if (s == "southwest") param |= 0x200;
					else if (s == "south") param |= 0x400;
					else if (s == "southeast") param |= 0x800;
				}
			}
			DataNode atk = ev["attack"];
			if (atk && atk.isSequence()) {
				for (int j = 0; j < (int)atk.size(); j++) {
					std::string s = atk[j].asString("");
					if (s == "melee") param |= 0x1000;
					else if (s == "ranged") param |= 0x2000;
					else if (s == "explosion") param |= 0x4000;
				}
			}
			DataNode fl = ev["flags"];
			if (fl && fl.isSequence()) {
				for (int j = 0; j < (int)fl.size(); j++) {
					std::string s = fl[j].asString("");
					if (s == "block_input") param |= 0x10000;
					else if (s == "exit_goto") param |= 0x20000;
					else if (s == "skip_turn") param |= 0x40000;
					else if (s == "disable") param |= 0x80000;
				}
			}
			this->tileEvents[i * 2 + 1] = param;
			if (tileIdx < 1024) this->mapFlags[tileIdx] |= 0x40;
		}
	}

	// Bytecode (hex string)
	DataNode bc = scripts["bytecode"];
	if (bc) {
		std::string hexStr = bc.asString("");
		std::string clean;
		for (char c : hexStr) {
			if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
				clean += c;
		}
		int bcSize = (int)clean.size() / 2;
		if (bcSize > 0) {
			delete[] this->mapByteCode;
			this->mapByteCodeSize = bcSize;
			this->mapByteCode = new uint8_t[bcSize];
			for (int i = 0; i < bcSize; i++) {
				char hi = clean[i * 2], lo = clean[i * 2 + 1];
				auto hexVal = [](char c) -> uint8_t {
					if (c >= '0' && c <= '9') return c - '0';
					if (c >= 'A' && c <= 'F') return 10 + c - 'A';
					if (c >= 'a' && c <= 'f') return 10 + c - 'a';
					return 0;
				};
				this->mapByteCode[i] = (hexVal(hi) << 4) | hexVal(lo);
			}
		}
	}
}
