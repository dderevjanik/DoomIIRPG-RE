#pragma once
#include "EntityMonster.h"
#include "AIComponent.h"
#include "LootComponent.h"
#include "GameSprite.h"
#include "LerpSprite.h"
#include "ScriptThread.h"

class EntityMonster;
class MayaCamera;
class Entity;
class GameSprite;
class LerpSprite;
class ScriptThread;
class OutputStream;
class InputStream;
class EntityDef;

class Applet;
class SDLGL;
struct GameConfig;

class Game
{
private:

public:
    Applet* app = nullptr;  // Set in startup(), replaces CAppContainer::getInstance()->app
    const GameConfig* gameConfig = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->gameConfig
    SDLGL* sdlGL = nullptr;

    static constexpr int32_t dropDirs[] { 64, 0, 64, 64, 0, 64, -64, 64, -64, 0, -64, -64, 0, -64, 64, -64 };
    static constexpr int MAX_GRID_ENTITIES = 9;
    static constexpr int DEFAULT_MAX_ENTITIES = 275;
    int maxEntities = 0;
    static constexpr int FILE_NAME_FULLWORLD = 0;
    static constexpr int FILE_NAME_CONFIG = 1;
    static constexpr int FILE_NAME_BRIEFPLAYER = 2;
    static constexpr int FILE_NAME_FULLPLAYER = 3;
    static constexpr int FILE_NAME_BRIEFWORLD = 4;

    uint8_t* monsterSounds = nullptr;
	int OFS_MAYAKEY_X = 0;
	int OFS_MAYAKEY_Y = 0;
	int OFS_MAYAKEY_Z = 0;
	int OFS_MAYAKEY_PITCH = 0;
	int OFS_MAYAKEY_YAW = 0;
	int OFS_MAYAKEY_ROLL = 0;
	int OFS_MAYAKEY_MS = 0;
    short* ofsMayaTween = nullptr;
    int camPlayerX = 0;
    int camPlayerY = 0;
    int camPlayerZ = 0;
    int camPlayerYaw = 0;
    int camPlayerPitch = 0;
    MayaCamera* mayaCameras = nullptr;
    int totalMayaCameras = 0;
    int16_t* mayaCameraKeys = nullptr;
    int8_t* mayaCameraTweens = nullptr;
    int16_t* mayaTweenIndices = nullptr;
    int totalMayaCameraKeys = 0;
    int totalMayaTweens = 0;
    // Entity pool caps. Originally sized to feature-phone RAM; bump as needed for
    // larger encounters / mods. All loops that iterate the pools should use the
    // matching `num*` runtime count, NOT the cap directly.
    static constexpr int MAX_ENTITY_DB = 1024;
    static constexpr int MAX_MONSTERS = 80;
    static constexpr int MAX_AI_COMPONENTS = 80;
    static constexpr int MAX_LOOT_COMPONENTS = 128;

    Entity* entities = nullptr;
    int numEntities = 0;
    Entity* entityDb[MAX_ENTITY_DB] = {};
    EntityMonster entityMonsters[MAX_MONSTERS] = {};
    int numMonsters = 0;
    AIComponent aiComponents[MAX_AI_COMPONENTS] = {};
    int numAIComponents = 0;
    LootComponent lootComponents[MAX_LOOT_COMPONENTS] = {};
    int numLootComponents = 0;
    int spawnParam = 0;
    bool disableAI = false;
    bool skipMinigames = false;
    Entity* inactiveMonsters = nullptr;
    Entity* activeMonsters = nullptr;
    Entity* combatMonsters = nullptr;
    int cinUnpauseTime = 0;
    int lastTurnTime = 0;
    bool pauseGameTime = false;
    int firstDropIndex = 0;
    int dropIndex = 0;
    int lastDropEntIndex = 0;
    bool skippingCinematic = false;
    bool skipDialog = false;
    bool skipAdvanceTurn = false;
    bool queueAdvanceTurn = false;
    char keycode[7] = {};
    int monstersTurn = 0;
    bool monstersUpdated = false;
    bool interpolatingMonsters = false;
    bool gotoTriggered = false;
    bool isSaved = false;
    bool isLoaded = false;
    int eventFlags[2] = {};
    GameSprite gsprites[48] = {};
    int activeSprites = 0;
    int activePropogators = 0;
    int animatingEffects = 0;
    int changeMapParam = 0;
    int saveStateMap = 0;
    uint8_t difficulty = 0;
    int activeLoadType = 0;
    bool updateAutomap = false;
    bool hasSeenIntro = false;
    bool skipMovie = false;
    int levelVars[8] = {};
    int16_t* levelNames = nullptr;
    int levelNamesCount = 0; // Port: New
    int totalLevelTime = 0;
    int curLevelTime = 0;
    uint16_t lootFound = 0;
    uint16_t totalLoot = 0;
    MayaCamera* activeCamera = nullptr;
    int activeCameraTime = 0;
    int activeCameraKey = -1;
    bool activeCameraView = false;
    Entity* watchLine = nullptr;
    LerpSprite lerpSprites[16] = {};
    int numLerpSprites = 0;
    ScriptThread scriptThreads[20] = {};
    int numScriptThreads = 0;
    bool pathVisitedl[64] = {};
    short pathParents[64] = {};
    short pathSearches[64] = {};
    int numPathSearches = 0;
    bool secretActive = false;
    Entity* openDoors[6] = {};
    int viewX = 0;
    int viewY = 0;
    int viewAngle = 0;
    int destX = 0;
    int destY = 0;
    int destAngle = 0;
    int viewSin = 0;
    int viewCos = 0;
    int viewStepX = 0;
    int viewStepY = 0;
    int viewRightStepX = 0;
    int viewRightStepY = 0;
    short scriptStateVars[128] = {};
    uint8_t mapSecretsFound = 0;
    uint8_t totalSecrets = 0;
    int cinematicWeapon = 0;
    short entityDeathFunctions[128] = {};
    int numDestroyableObj = 0;
    int destroyedObj = 0;
    int placedBombs[4] = {};
    Entity* gridEntities[Game::MAX_GRID_ENTITIES] = {};
    int64_t curPath = 0;
    long unused_0x486c = 0; // never used
    int pathDepth = 0;
    int pathSearchDepth = 0;
    int64_t closestPath = 0;
    int closestPathDepth = 0;
    int closestPathDist = 0;
    int lineOfSight = 0;
    int lineOfSightWeight = 0;
    Entity* findEnt = nullptr;
    Entity* skipEnt = nullptr;
    int interactClipMask = 0;
    uint8_t visitOrder[8] = {};
    int visitDist[8] = {};
    int visitedTiles[32] = {};
    int baseVisitedTiles[32] = {};
    int numMallocsForVIOS = 0;
    bool angryVIOS = false;
    short numLevelLoads[10] = {};
    int totalPlayTime = 0;
    int lastSaveTime = 0;
    Entity* traceEntities[32] = {};
    int traceFracs[32] = {};
    int numTraceEntities = 0;
    int traceBoundingBox[4] = {};
    int tracePoints[4] = {};
    int traceCollisionX = 0;
    int traceCollisionY = 0;
    int traceCollisionZ = 0;
    Entity* traceEntity = nullptr;
    int posShift = 0;
    int angleShift = 0;
    int numCallThreads = 0;
    ScriptThread* callThreads[16] = {};

	// Constructor
	Game();
	// Destructor
	~Game();

	bool startup();
    void unlinkEntity(Entity* entity);
    void linkEntity(Entity* entity, int tileX, int tileY);
    void unlinkWorldEntity(int tileX, int tileY);
    void linkWorldEntity(int tileX, int tileY);
    void removeEntity(Entity* entity);
    void uncoverAutomapAt(int destX, int destY);
    void trace(int startX, int startY, int endX, int endY, Entity* entity, int typeMask, int radius);
    void trace(int startX, int startY, int startZ, int traceCollisionX, int traceCollisionY, int traceCollisionZ, Entity* entity, int typeMask, int radius, bool checkZ);
    void loadMapEntities();
    void loadTableCamera(int i, int i2);
    void setKeyOffsets();
    void loadMayaCameras(InputStream* IS);
    void unloadMapData();
    bool touchTile(int worldX, int worldY, bool touchAll);
    void prepareMonsters();
    Entity* findMapEntity(int worldX, int worldY);
    Entity* findMapEntity(int worldX, int worldY, int typeMask);
    void activate(Entity* entity);
    void activate(Entity* entity, bool triggerScript, bool checkRange, bool playAlertSound, bool b4);
    void killAll();
    void deactivate(Entity* entity);
    void UpdatePlayerVars();
    void monsterAI();
    void monsterLerp();
    void spawnPlayer();
    void eventFlagsForMovement(int n, int n2, int n3, int n4);
    int eventFlagForDirection(int n, int n2);
    void givemap(int n, int n2, int n3, int n4);
    bool performDoorEvent(int action, Entity* entity, int snapMode);
    bool performDoorEvent(int action, Entity* entity, int snapMode, bool isSecret);
    bool performDoorEvent(int action, ScriptThread* scriptThread, Entity* watchLine, int snapMode, bool isSecret);
    void lerpSpriteAsDoor(int sprite, int action, ScriptThread* scriptThread);
    void updatePlayerDoors(Entity* entity, bool addDoor);
    bool CanCloseDoor(Entity* entity);
    void advanceTurn();
    void gsprite_clear(int flagMask);
    GameSprite* gsprite_alloc(int tileIdx, int frame, int flags);
    GameSprite* gsprite_allocAnim(int animId, int x, int y, int z);
    void gsprite_destroy(GameSprite* gameSprite);
    void snapGameSprites();
    void gsprite_update(int currentTime);
    void saveWorldState(OutputStream* OS, bool briefSave);
    void loadWorldState();
    bool loadWorldState(InputStream* IS);
    void saveConfig();
    void loadConfig();
    void saveState(int lastMapID, int loadMapID, int viewX, int viewY, int viewAngle, int viewPitch, int prevX, int prevY, int saveX, int saveY, int saveZ, int saveAngle, int savePitch, int saveType);
    void saveLevelSnapshot();
    bool savePlayerState(OutputStream* OS,int loadMapID, int viewX, int viewY, int viewAngle, int viewPitch, int prevX, int prevY, int saveX, int saveY, int saveZ, int saveAngle, int savePitch);
    bool loadPlayerState(InputStream* IS);
    bool loadState(int activeLoadType);
    bool hasConfig();
    bool hasElement(const char* name);
    bool hasSavedState();
    void removeState(bool showProgress);
    void saveEmptyConfig();
    bool canSnapMonsters();
    bool snapMonsters(bool forceSnap);
    void endMonstersTurn();
    void updateMonsters();
    void setLineLocked(Entity* entity, bool lock);
    void snapAllMovers();
    void skipCinematic();
    int getNextBombIndex();
    void updateBombs();
    int setDynamite(int x, int y, bool centered);
    Entity* getFreeDropEnt();
    void allocDynamite(int x, int y, int z, int spriteFlags, int bombSlotIdx, int bombTimer);
    Entity* spawnDropItem(int x, int y, int tileIdx, EntityDef* def, int param, bool throwAnim);
    Entity* spawnDropItem(int x, int y, int tileIdx, int eType, int eSubType, int defParm, int param, bool throwAnim);
    void spawnDropItem(Entity* entity);
    Entity* spawnPlayerEntityCopy(int x, int y);
    Entity* spawnSentryBotCorpse(int x, int y, int botSubType, int ammoLoot, int healthLoot);
    void throwDropItem(int dstX, int dstY, int height, Entity* entity);
    int updateLerpSprite(LerpSprite* lerpSprite);
    void snapLerpSprites(int targetSprite);
    void updateLerpSprites();
    LerpSprite* allocLerpSprite(ScriptThread* thread, int sprite, bool isAnimEffect);
    void freeLerpSprite(LerpSprite* lerpSprite);
    void runScriptThreads(int n);
    ScriptThread* allocScriptThread();
    void freeScriptThread(ScriptThread* scriptThread);
    bool isCameraActive();
    int executeTile(int n, int n2, int n3);
    bool doesScriptExist(int n, int n2, int n3);
    int executeTile(int n, int n2, int n3, bool b);
    int executeStaticFunc(int n);
    int queueStaticFunc(int n);
    int getSaveVersion();
    void loadEntityStates(InputStream* IS);
    void saveEntityStates(OutputStream* OS, bool briefSave);
    bool tileObstructsAttack(int tileX, int tileY);
    bool isInputBlockedByScript();
    void updateScriptVars();
    void awardSecret(bool playSecretSound);
    void addEntityDeathFunc(Entity* entity, int funcId);
    void removeEntityFunc(Entity* entity);
    void executeEntityFunc(Entity* entity, bool throwAwayLoot);
    void foundLoot(int sprite, int lootValue);
    void foundLoot(int x, int y, int z, int lootValue);
    void destroyedObject(int unused);
    void raiseCorpses();
    bool isInFront(int tileX, int tileY);
    int VecToDir(int dx, int dy, bool highShift);
    void NormalizeVec(int vecX, int vecY, int* array);
    void setMonsterClip(int tileX, int tileY);
    void unsetMonsterClip(int tileX, int tileY);
    bool monsterClipExists(int tileX, int tileY);

    void cleanUpCamMemory();

    const char* GetSaveFile(int fileType, int mapIdx);
    char* getProfileSaveFileName(const char* name);
    int getMonsterSound(int monsterIdx, int soundType);
};
