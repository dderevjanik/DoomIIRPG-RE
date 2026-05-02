#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Image;
class gles;
class Graphics;
class Applet;
class SDLGL;
class Entity;
class TGLVert;
class DataNode;
struct GameConfig;

class Render
{
private:

public:
	Applet* app = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->app
	const GameConfig* gameConfig = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->gameConfig
	SDLGL* sdlGL = nullptr;
	bool headless = false; // Set in startup(); when true, render() and renderPortal() are no-ops.

	static constexpr int RENDER_NORMAL = 0;
	static constexpr int RENDER_BLEND25 = 1;
	static constexpr int RENDER_BLEND50 = 2;
	static constexpr int RENDER_ADD = 3;
	static constexpr int RENDER_ADD75 = 4;
	static constexpr int RENDER_ADD50 = 5;
	static constexpr int RENDER_ADD25 = 6;
	static constexpr int RENDER_SUB = 7;
	static constexpr int RENDER_PERF = 9;
	static constexpr int RENDER_NONE = 10;
	static constexpr int RENDER_BLEND75 = 12; // New from IOS
	static constexpr int RENDER_BLENDSPECIALALPHA = 13; // New from IOS

	// Reference height (in pixels) the sprite art was authored against on the
	// original 176×208 BREW phone target. Used as a fixed-point scale anchor in
	// sprite-size math (`scaledHeight = SPRITE_DESIGN_HEIGHT * scaleFactor / 65536`).
	// NOT correlated with the current canvas height — this is a design constant.
	static constexpr int SPRITE_DESIGN_HEIGHT = 176;

	// Resolve a render mode name (e.g. "add", "blend50") to its integer constant.
	// Falls back to parsing as integer, then defaults to RENDER_NORMAL.
	static inline int renderModeFromName(const std::string& name) {
		static const std::unordered_map<std::string, int> modes = {
			{"normal", RENDER_NORMAL}, {"blend25", RENDER_BLEND25},
			{"blend50", RENDER_BLEND50}, {"add", RENDER_ADD},
			{"add75", RENDER_ADD75}, {"add50", RENDER_ADD50},
			{"add25", RENDER_ADD25}, {"sub", RENDER_SUB},
			{"perf", RENDER_PERF}, {"none", RENDER_NONE},
			{"blend75", RENDER_BLEND75},
			{"blend_special_alpha", RENDER_BLENDSPECIALALPHA},
		};
		auto it = modes.find(name);
		if (it != modes.end()) return it->second;
		try { return std::stoi(name); } catch (...) { return 0; }
	}

	static constexpr int RENDER_FLAG_BLUESHIFT = 32;
	static constexpr int RENDER_FLAG_GREENSHIFT = 64;
	static constexpr int RENDER_FLAG_REDSHIFT = 128;
	static constexpr int RENDER_FLAG_BRIGHTREDSHIFT = 256;
	static constexpr int RENDER_FLAG_PULSATE = 512;
	static constexpr int RENDER_FLAG_SCALE_WEAPON = 1024; // [GEC] New
	static constexpr int RENDER_FLAG_MULTYPLYSHIFT = 2048; // [GEC] New
	static constexpr int MAPTILE_SIZE = 64;
	static constexpr int TILE_MASK = 63;
	static constexpr int MAPTILE_MIDDLE = 32;
	static constexpr int MAP_MAXWORLDVALUE = 2047;
	static constexpr int SPRITE_SIZE = 64;
	static constexpr int TILE_SIZE = 64;
	static constexpr int FLAT_SIZE = 64;
	static constexpr int MAX_CUSTOM_SPRITES = 48;
	static constexpr int MAX_DROP_SPRITES = 16;
	static constexpr int S_NUMFIELDS = 9;
	static constexpr int S_INDEX_X = 0;
	static constexpr int S_INDEX_Y = 1;
	static constexpr int S_INDEX_Z = 2;
	static constexpr int S_INDEX_RENDERMODE = 3;
	static constexpr int S_INDEX_NODE = 4;
	static constexpr int S_INDEX_NODENEXT = 5;
	static constexpr int S_INDEX_VIEWNEXT = 6;
	static constexpr int S_INDEX_ENT = 7;
	static constexpr int S_INDEX_SCALEFACTOR = 8;
	static constexpr short BASIC_SCALE_FACTOR = 64;
	static constexpr int SINFO_NUMFIELDS = 2;
	static constexpr int MAX_WALL_TEXTURES_MAP = 16;
	static constexpr int MAX_VISIBLE_NODES = 256;
	static constexpr int INVALID_NODE = -1;
	static constexpr int MAPEVENTSIZE = 2;
	static constexpr int MAX_LADDERS_PER_MAP = 10;
	static constexpr int MAX_KEEP_PITCH_LEVEL_TILES = 20;
	static constexpr int RENDER_DEFAULT = 31;
	static constexpr int FADE_FLAG_FADEOUT = 1;
	static constexpr int FADE_FLAG_FADEIN = 2;
	static constexpr int FADE_FLAG_CHANGEMAP = 4;
	static constexpr int FADE_FLAG_SHOWSTATS = 8;
	static constexpr int FADE_FLAG_EPILOGUE = 32;
	static constexpr int FADE_SPECIAL_FLAG_MASK = -29;
	static constexpr int CHANGEMAP_FADE_TIME = 1000;
	static constexpr int ROCKTIMEDAMAGE = 200;
	static constexpr int ROCKTIMEDODGE = 800;
	static constexpr int ROCKDISTCOMBAT = 6;
	// Default sprite Z offset (canvas units, scaled <<4 for the engine). A
	// normal (non-Z) sprite has its S_Z initialised to this; postProcessSprites
	// then subtracts the same value from Z-sprites so yaml_z becomes an
	// absolute-from-floor height. Consequently, a Z-sprite with yaml_z ==
	// 2*SPRITE_Z_DEFAULT is visually equivalent to a normal sprite sitting on
	// the floor — useful for placing pickups whose animation frame must be
	// controlled via z_anim (only Z-sprites get that field applied).
	static constexpr int SPRITE_Z_DEFAULT = 32;
	static constexpr int MEDIA_FLAG_REFERENCE = 0x80000000;
	static constexpr int MEDIA_PALETTE_REGISTERED = 0x40000000;
	static constexpr int MEDIA_TEXELS_REGISTERED = 0x40000000;
	static constexpr int MEDIA_MAX_MAPPINGS = 512;
	static constexpr int MEDIA_MAX_IMAGES = 1024;
	static constexpr int MEDIA_MAX_IMAGES_MASK = 1023;
	static constexpr int SKY_MAP_WIDTH = 128;
	static constexpr int SKY_MAP_HEIGHT = 128;
	static constexpr int SKY_MAP_SMASK = 127;
	static constexpr int SKY_MAP_TMASK = 16256;
	static constexpr int SKY_MAP_VIRTUAL_WIDTH = 384;
	static constexpr int SKY_MAP_VIRTUAL_HEIGHT = 384;
	static constexpr int SKY_MAP_HORIZONTAL_REPEATS = 2;
	static constexpr int SKY_MAP_SHIFT = 8;
	static constexpr int SKY_MAP_STEP = 85;
	static constexpr int MAX_BRIGHTNESS_FACTOR = 100;
	static constexpr int HOLD_BRIGHTNESS_TIME = 500;
	static constexpr int FADE_BRIGHTNESS_TIME = 500;
	static constexpr int V_SCROLL_CORRECTION = 5;
	static constexpr int MAX_VSCROLL_VELOCITY = 30;
	static constexpr int MIN_INITIAL_VSCROLL_VELOCITY = 0;
	static constexpr int MAX_INITIAL_VSCROLL_VELOCITY = 90;
	static constexpr int FOG_SHIFT = 8;
	static constexpr int FOG_ONE = 256;
	static constexpr int MAP_HEADER_SIZE = 46;
	static constexpr int MAX_SPLIT_SPRITES = 8;
	static constexpr int SPLIT_SPRITE_BOUNDS = 8;
	static constexpr int ANIM_IDLE_TIME = 8192;
	static constexpr int ANIM_IDLE_MASK = 8191;
	static constexpr int ANIM_IDLE_SWITCH_TIME = 256;
	static constexpr int ANIM_IDLE_DOWN_TIME = 7936;
	static constexpr int ADDITIVE_SHIFT = 488;
	static constexpr int MAX_DIZZY = 30;
	static constexpr int LATENCY_ADJUST = 50;
	static constexpr int portalMaxRadius = 66;
	static constexpr int PORTAL_DNE = 0;
	static constexpr int PORTAL_SPINNING_UP = 1;
	static constexpr int PORTAL_SPINNING = 2;
	static constexpr int PORTAL_SPINNING_DOWN = 3;
	static constexpr int PORTAL_GIVE_UP_CONTROL = 4;
	static constexpr int FEAR_EYE_SIZE = 8;

	int screenWidth = 0;
	int screenHeight = 0;

	// Texture-region state populated by Render::setupTexture and read by
	// the sprite renderer. Moved here from TinyGL — they had nothing to
	// do with the legacy Q-format pipeline; they're Render's bookkeeping.
	int imageBounds[4] = {};
	int sWidth = 0;
	int tHeight = 0;
	uint32_t textureBaseSize = 0;

	// Render viewport rect (logical pixel coords). Set by TinyGL::setViewport
	// (still owns the viewport-clamp math) and read by sprite/BSP rendering.
	int viewportX = 0;
	int viewportY = 0;
	int viewportWidth = 0;
	int viewportHeight = 0;
	int viewportClampX1 = 0;
	int viewportClampX2 = 0;

	int S_X = 0;
	int S_Y = 0;
	int S_Z = 0;
	int S_RENDERMODE = 0;
	int S_NODE = 0;
	int S_NODENEXT = 0;
	int S_VIEWNEXT = 0;
	int S_ENT = 0;
	int S_SCALEFACTOR = 0;
	int SINFO_SORTZ = 0;
	short* mapSprites = nullptr;
	int* mapSpriteInfo = nullptr;
	int numMapSprites = 0;
	int numSprites = 0;
	int numNormalSprites = 0;
	int numZSprites = 0;
	short* nodeIdxs = nullptr;
	int numVisibleNodes = 0;
	int numNodes = 0;
	uint8_t* nodeNormalIdxs = nullptr;
	short* nodeOffsets = nullptr;
	short* nodeChildOffset1 = nullptr;
	short* nodeChildOffset2 = nullptr;
	uint8_t* nodeBounds = nullptr;
	short* nodeSprites = nullptr;
	uint8_t* nodePolys = nullptr;
	int numLines = 0;
	uint8_t* lineFlags = nullptr;
	uint8_t* lineXs = nullptr;
	uint8_t* lineYs = nullptr;
	int numNormals = 0;
	short* normals = nullptr;
	uint8_t* heightMap = nullptr;
	int* tileEvents = nullptr;
	int numTileEvents = 0;
	int lastTileEvent = 0;
	int* staticFuncs = nullptr;
	uint8_t* mapByteCode = nullptr;
	int mapByteCodeSize = 0;
	uint8_t* mapFlags = nullptr;
	short mapEntranceAutomap = 0;
	short mapExitAutomap = 0;
	short mapLadders[Render::MAX_LADDERS_PER_MAP] = {};
	short mapKeepPitchLevelTiles[Render::MAX_KEEP_PITCH_LEVEL_TILES] = {};
	int mapNameField = 0;
	int mapNameID = 0;
	int mapSpawnIndex = 0;
	int mapSpawnDir = 0;
	int mapCompileDate = 0;
	uint8_t mapFlagsBitmask = 0;
	int* customSprites = nullptr;
	int* dropSprites = nullptr;
	int firstDropSprite = 0;
	int mapMemoryUsage = 0;
	int texelMemoryUsage = 0;
	int paletteMemoryUsage = 0;
	int32_t* sinTable = nullptr;
	bool skipCull = false;
	bool skipBSP = false;
	bool skipLines = false;
	bool skipFlats = false;
	bool skipSprites = false;
	bool skipDecals = false;
	bool skip2DStretch = false;
	bool skipViewNudge = false;
	int renderMode = 0;
	int lineCount = 0;
	int lineRasterCount = 0;
	int nodeCount = 0;
	int leafCount = 0;
	int spriteCount = 0;
	int spriteRasterCount = 0;
	int viewX = 0;
	int viewY = 0;
	int viewZ = 0;
	int viewAngle = 0;
	// Signed-normalized pitch (-512..511) — mirrored from TinyGL::setView so
	// renderer code can read pitch without going through tinyGL.
	int viewPitch = 0;
	int viewSin = 0;
	int viewCos = 0;
	int viewRightStepX = 0;
	int viewRightStepY = 0;
	short viewSprites = 0;
	short viewNodes = 0;
	int currentFrameTime = 0;
	int frameTime = 0;
	int bspTime = 0;
	int clearColorBuffer = 0;
	int fadeTime = 0;
	int fadeDuration = 0;
	int fadeFlags = 0;
	int rockViewTime = 0;
	int rockViewDur = 0;
	int rockViewX = 0;
	int rockViewY = 0;
	int rockViewZ = 0;
	bool disableRenderActivate = false;
	int monsterIdleTime[18] = {};
	bool chatZoom = false;
	bool shotsFired = false;
	int postProcessMode = 0;
	bool brightenPostProcess = false;
	int brightenPostProcessBeginTime = 0;
	int screenVScrollOffset = 0;
	bool useMastermindHack = false;
	bool useCaldexHack = false;
	int delayedSpriteBuffer[3] = {};
	uint8_t* skyMapTexels = nullptr;
	uint16_t* skyMapPalette = nullptr;
	bool isSkyMap = false;
	int skyMapX = 0;
	int skyMapY = 0;
	int maxLocalBrightness = 0;
	int brightenPPMaxReachedTime = 0;
	int brightnessInitialBoost = 0;
	int vScrollVelocity = 0;
	int lastScrollChangeTime = 0;
	short* mediaMappings = nullptr;
	int* mediaPalColors = nullptr;
	int* mediaTexelSizes = nullptr;
	uint8_t* mediaDimensions = nullptr;
	short* mediaBounds = nullptr;
	uint8_t** mediaTexels = nullptr;
	int* mediaTexelSizes2 = nullptr;
	uint16_t** mediaPalettes = nullptr;
	int* mediaPalettesSizes = nullptr;
	int* splitSprites = nullptr;
	int numSplitSprites = 0;
	int traceLine[4] = {};
	// Active fog parameters consumed by gles::BeginFrame each frame. Moved
	// here from TinyGL — fog has nothing to do with the legacy CPU pipeline,
	// it's a renderer concern. Writers: PlayerBuffs (status-effect fog),
	// Hud (low-HP fog), RenderEffects (fog lerp / save/load), GameSaveLoad.
	int fogMin = 0;
	int fogRange = 0;
	int fogColor = 0;

	int baseFogMin = 0;
	int baseFogRange = 0;
	int destFogMin = 0;
	int destFogRange = 0;
	int fogLerpStart = 0;
	int fogLerpTime = 0;
	int playerFogMin = 0;
	int playerFogRange = 0;
	int playerFogColor = 0;
	int* temp = nullptr;
	int imageFrameBounds[3][4] = {};
	int baseDizzy = 0;
	int destDizzy = 0;
	int dizzyTime = 0;
	int portalSpinTime = 0;
	int currentPortalMod = 0;
	bool portalModIncreasing = false;
	int nextPortalFrame = 0;
	int currentPortalTheta = 0;
	int portalCX = 0;
	int portalCY = 0;
	int portalScale = 0;
	int portalState = 0;
	int previousPortalState = 0;
	bool portalScripted = false;
	bool portalInView = false;
	int closestPortalDist = 0;
	int bltTime = 0;
	gles* _gles = nullptr;
	Image* imgPortal = nullptr;

	// [GEC] New
	bool fixWaterAnim1 = false;
	bool fixWaterAnim2 = false;
	bool fixWaterAnim3 = false;
	bool fixWaterAnim4 = false;

	// Constructor
	Render();
	// Destructor
	~Render();

	bool startup();
	void shutdown();
	int getNextEventIndex();
	int findEventIndex(int n);
	void unloadMap();
	inline static int upSamplePixel(int pixel) { return (pixel >> 8 & 0xf800) | (pixel >> 5 & 0x07e0) | (pixel >> 3 & 0x001f); } // rgb888 to rgb565
	inline static int RGB888ToRGB565(int r, int g, int b) { return ((r >> 3 & 0x1f) << 11) | ((g >> 2 & 0x3f) << 5) | (b >> 3 & 0x1f); }; // rgb888 to rgb565
	void RegisterMedia(int n);
	void FinalizeMedia();
	void FinalizeMediaFromYaml(DataNode& palYaml, DataNode& texYaml);
	// Incrementally register + finalize a single media ID without touching the
	// existing registrations. Returns true on success. Used by the editor's
	// hot-reload path to add new entity sprites without a full teardown.
	bool registerAndFinalizeMedia(int id);
	bool loadSkyFromPng(const std::string& path);

	// What to preserve across a loadMap call. Default = tear down and rebuild
	// everything (full reload). Hot reload sets both flags true.
	struct LoadOptions {
		// Reuse the existing media arrays (mediaMappings/palettes/texels/GPU
		// textures). Only valid if a prior full reload has populated them, and
		// if the caller has already incrementally registered any new media IDs
		// via registerAndFinalizeMedia().
		bool keepMedia = false;
		// Reuse the sky texture loaded on the prior reload.
		bool keepSky   = false;
	};

	// Canonical map-load entry point. `beginLoadMap` and `reloadGeometryOnly`
	// are thin wrappers over this with different LoadOptions. Keeps the two
	// paths from drifting — every engine step (postProcessSprites, height_map
	// override, loadMayaCameras, etc.) lives here exactly once.
	bool loadMap(const std::vector<uint8_t>& binBytes, int mapNameID, LoadOptions opts);

	bool beginLoadMap(int mapNameID);
	// Fast path for the map editor: re-read the .bin from an in-memory buffer
	// and swap geometry + heightMap + sprites in place, **without** tearing
	// down or re-registering media, reloading the sky, or hitting disk. Only
	// valid when the media set is unchanged from the previous load — see
	// EditorState::compileAndLoadMap for the gate.
	bool reloadGeometryOnly(const std::vector<uint8_t>& binBytes, int mapNameID);

	// Next free compacted palette / texel index (updated by FinalizeMediaFromYaml
	// and registerAndFinalizeMedia). Used to hand out new slots incrementally.
	int nextPalIdx = 0;
	int nextTexIdx = 0;
private:
	// Frees all geometry/sprite/tile-event arrays but leaves media pointers
	// (palettes, texels, GPU textures, sky) intact. Used by reloadGeometryOnly.
	void unloadGeometryOnly();
public:
	void draw2DSprite(int tileNum, int frame, int x, int y, int flags, int renderMode, int renderFlags, int scaleFactor);
	void renderSprite(int x, int y, int z, int tileNum, int frame, int flags, int renderMode, int scaleFactor, int renderFlags);
	void renderSprite(int x, int y, int z, int tileNum, int frame, int flags, int renderMode, int scaleFactor, int renderFlags, int palIndex);
	void occludeSpriteLine(int n);
	void drawNodeLines(short n);
	bool cullBoundingBox(int n, int n2, bool b);
	bool cullBoundingBox(int n, int n2, int n3, int n4, bool b);
	void addSprite(short n);
	void addSplitSprite(int n, int n2);
	void addNodeSprites(short n);
	int nodeClassifyPoint(int n, int n2, int n3, int n4);
	void drawNodeGeometry(short n);
	void walkNode(short n);
	int dot(int n, int n2, int n3, int n4);
	int CapsuleToCircleTrace(int* array, int n, int n2, int n3, int n4);
	int CapsuleToLineTrace(int* array, int n, int* array2);
	int traceWorld(int n, int* array, int n2, int* array2, int n3);
	bool renderStreamSpriteGL(TGLVert* array, int n); // Port
	void renderStreamSprite(int n);
	void renderSpriteObject(int n);
	void renderBSP();
	void loadPlayerFog();
	void savePlayerFog();
	void snapFogLerp();
	void startFogLerp(int n, int n2, int fogLerpTime);
	void buildFogTables(int fogColor);
	int(*getImageFrameBounds(int n, int n2, int n3, int n4))[4];
	uint16_t* getPalette(int n, int n2);
	void setupTexture(int n, int n2, int renderMode, int renderFlags);
	void render(int viewX, int viewY, int viewZ, int viewAngle, int viewPitch, int viewRoll, int viewFov);
	void unlinkSprite(int n);
	void unlinkSprite(int n, int n2, int n3);
	void relinkSprite(int n);
	void relinkSprite(int n, int n2, int n3, int n4);
	short getNodeForPoint(int n, int n2, int n3, int n4);
	int getHeight(int x, int y);
	bool checkTileVisibilty(int n, int n2);
	bool isFading();
	int getFadeFlags();
	void startFade(int fadeDuration, int fadeFlags);
	void endFade();
	bool fadeScene(Graphics* graphics);
	void postProcessView(Graphics* graphics);
	int convertToGrayscale(int color);
	bool checkPortalVisibility(int x, int y, int z);
	void renderPortal();
	void drawRGB(Graphics* graphics);
	void rockView(int rockViewDur, int x, int y, int z);
	bool isNPC(int n);
	bool hasNoFlareAltAttack(int n);
	bool hasGunFlare(int n);
	bool isFloater(int n);
	bool isSpecialBoss(int n);
	void renderFearEyes(Entity* entity, int frame, int x, int y, int z, int scaleFactor, bool flipH);
	void renderSpriteAnim(int n, int frame, int x, int y, int z, int tileNum, int flags, int renderMode, int scaleFactor, int renderFlags);
	void renderFloaterAnim(int n, int frame, int x, int y, int z, int tileNum, int flags, int renderMode, int scaleFactor, int renderFlags);
	void renderSpecialBossAnim(int n, int frame, int x, int y, int z, int tileNum, int flags, int renderMode, int scaleFactor, int renderFlags);
	
	void postProcessSprites();
	void handleMonsterIdleSounds(Entity* entity);

	// Map loading helpers (extracted from beginLoadMap)
	void loadMapMedia();
	void loadSpritesFromYaml(const DataNode& sprites);
	void loadCamerasFromYaml(const DataNode& cameras);
	void loadScriptsYaml(int mapNameID);
	void loadMapLevelOverrides(int mapNameID, DataNode& levelYaml, bool& spritesFromYaml, DataNode& yamlSpritesNode);

	// --- Sprite accessor API (encapsulates mapSprites[] layout) ---
	inline short getSpriteX(int sprite) const { return mapSprites[S_X + sprite]; }
	inline short getSpriteY(int sprite) const { return mapSprites[S_Y + sprite]; }
	inline short getSpriteZ(int sprite) const { return mapSprites[S_Z + sprite]; }
	inline short getSpriteRenderMode(int sprite) const { return mapSprites[S_RENDERMODE + sprite]; }
	inline short getSpriteScaleFactor(int sprite) const { return mapSprites[S_SCALEFACTOR + sprite]; }
	inline short getSpriteEnt(int sprite) const { return mapSprites[S_ENT + sprite]; }

	inline void setSpriteX(int sprite, short val) { mapSprites[S_X + sprite] = val; }
	inline void setSpriteY(int sprite, short val) { mapSprites[S_Y + sprite] = val; }
	inline void setSpriteZ(int sprite, short val) { mapSprites[S_Z + sprite] = val; }
	inline void setSpriteRenderMode(int sprite, short val) { mapSprites[S_RENDERMODE + sprite] = val; }
	inline void setSpriteScaleFactor(int sprite, short val) { mapSprites[S_SCALEFACTOR + sprite] = val; }
	inline void setSpriteEnt(int sprite, short val) { mapSprites[S_ENT + sprite] = val; }

	// --- SpriteInfo accessor API (encapsulates mapSpriteInfo[] bitmask layout) ---
	inline int getSpriteTileNum(int sprite) const { return mapSpriteInfo[sprite] & 0xFF; }
	inline int getSpriteFrame(int sprite) const { return (mapSpriteInfo[sprite] >> 8) & 0xFF; }
	inline void setSpriteTileNum(int sprite, int tileNum) { mapSpriteInfo[sprite] = (mapSpriteInfo[sprite] & ~0xFF) | (tileNum & 0xFF); }
	inline void setSpriteFrame(int sprite, int frame) { mapSpriteInfo[sprite] = (mapSpriteInfo[sprite] & ~0xFF00) | ((frame & 0xFF) << 8); }
	inline int getSpriteInfoRaw(int sprite) const { return mapSpriteInfo[sprite]; }
	inline void setSpriteInfoRaw(int sprite, int val) { mapSpriteInfo[sprite] = val; }
	inline void clearSpriteInfoFlag(int sprite, int flag) { mapSpriteInfo[sprite] &= ~flag; }
	inline void setSpriteInfoFlag(int sprite, int flag) { mapSpriteInfo[sprite] |= flag; }

	void fixTexels(int offset, int i, int mediaID, int* rowHeight); // [GEC] New

	// Cached sprite indices from SpriteDefs (initialized once at startup)
	void initSpriteDefs();

	// Individual tiles
	int TILE_SKY_BOX = 0;
	int TILE_BOSS_VIOS = 0;
	int TILE_BOSS_VIOS5 = 0;
	int TILE_OBJ_CRATE = 0;
	int TILE_TERMINAL_TARGET = 0;
	int TILE_TERMINAL_HACKING = 0;
	int TILE_CLOSED_PORTAL_EYE = 0;
	int TILE_EYE_PORTAL = 0;
	int TILE_PORTAL_SOCKET = 0;
	int TILE_RED_DOOR_LOCKED = 0;
	int TILE_BLUE_DOOR_UNLOCKED = 0;
	int TILE_FLAT_LAVA = 0;
	int TILE_FLAT_LAVA2 = 0;
	int TILE_HELL_HANDS = 0;
	int TILE_FADE = 0;
	int TILE_SCORCH_MARK = 0;
	int TILE_WATER_STREAM = 0;
	int TILE_OBJ_FIRE = 0;
	int TILE_OBJ_TORCHIERE = 0;
	int TILE_SFX_LIGHTGLOW1 = 0;
	int TILE_WATER_SPOUT = 0;
	int TILE_TREE_TOP = 0;
	int TILE_TREE_TRUNK = 0;
	int TILE_PRACTICE_TARGET = 0;
	int TILE_OBJ_CORPSE = 0;
	int TILE_OBJ_OTHER_CORPSE = 0;
	int TILE_OBJ_SCIENTIST_CORPSE = 0;
	int TILE_MONSTER_SENTRY_BOT = 0;
	int TILE_MONSTER_RED_SENTRY_BOT = 0;
	int TILE_TREADMILL_SIDE = 0;
	int TILE_SENTINEL_SPIKES_DUMMY = 0;
	int TILE_SHADOW = 0;
	int TILE_GLASS = 0;

};
