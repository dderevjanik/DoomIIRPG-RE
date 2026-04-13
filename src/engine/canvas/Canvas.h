#pragma once
#include "Graphics.h"

class Applet;
class SDLGL;
class ICanvasState;
class Image;
class Text;
class Entity;
class EntityDef;
class ScriptThread;
class Graphics;
struct GameConfig;

class fmButtonContainer;
class fmScrollButton;
class fmButton;
class fmSwipeArea;

struct DialogStyleDef {
	int index;
	int bgColor;
	int altBgColor;       // alternate bg when dialogFlags & 1 (-1 = none)
	int headerColor;      // header bar color (-1 = no header color override)
	int yAdjust;          // pixel offset to default Y position
	int posTopOnFlag;     // move to top when dialogFlags & this != 0 (0 = disabled)
	bool positionTop;     // always position at top
	bool hasHeader;       // draw header bar above dialog (styles 2, 9, 16)
	bool hasGradient;     // apply gradient fill effect (style 8)
	bool hasPortrait;     // draw player portrait in dialog (style 8)
	bool customDraw;      // use custom scroll-style rendering (style 3)
	bool hasChestHeader;  // draw mini header when dialogItem present (style 4)
	int iconSrcX;         // imgUIImages source X for icon (-1 = no icon)
	int iconSrcY;         // imgUIImages source Y
	int iconW, iconH;     // icon dimensions
	bool iconBottom;      // true = draw icon at bottom of dialog, false = top
	bool greenTerminal;   // use green text color (style 9)
};

class Canvas
{
private:

public:
	Applet* app = nullptr;  // Set in startup(), replaces CAppContainer::getInstance()->app
	const GameConfig* gameConfig = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->gameConfig
	bool headless = false;
	SDLGL* sdlGL = nullptr;

	static constexpr int KNOCKBACK_HEIGHT = 10;
	static constexpr int FALLDOWNDEATHTIME = 750;
	static constexpr int DEATHFADETIME = 2000;
	static constexpr int TOTALDEATHTIME = 2750;
	static constexpr int FALLDOWNDEATHTIME_FAMILIAR = 750;
	static constexpr int DEATHFADETIME_FAMILIAR = 750;
	static constexpr int TOTALDEATHTIME_FAMILIAR = 1500;
	static constexpr int LOOTING_CROUCH_TIME = 500;
	static constexpr int SPD_RENDER = 0;
	static constexpr int SPD_BSP = 1;
	static constexpr int SPD_HUD = 2;
	static constexpr int SPD_WEAPON = 3;
	static constexpr int SPD_BLIT = 4;
	static constexpr int SPD_FLUSH = 5;
	static constexpr int SPD_PAUSED = 6;
	static constexpr int SPD_LOOP = 7;
	static constexpr int SPD_TOTAL = 8;
	static constexpr int SPD_DEBUG = 9;
	static constexpr int SPD_NUM_FIELDS = 13;
	static constexpr int MAX_EVENTS = 4;
	static constexpr int MAX_STATE_VARS = 9;
	static constexpr int HELPMSG_TYPE_ITEM = 1;
	static constexpr int HELPMSG_TYPE_CODESTRING = 2;
	static constexpr int HELPMSG_TYPE_ACHIEVEMENT = 3;

	static constexpr int FADE_FLAG_NONE = 0;
	static constexpr int FADE_FLAG_FADEOUT = 1;
	static constexpr int FADE_FLAG_FADEIN = 2;

	static constexpr int ST_MENU = 1;
	static constexpr int ST_INTRO_MOVIE = 2;
	static constexpr int ST_PLAYING = 3;
	static constexpr int ST_INTER_CAMERA = 4; // Wolf -> ST_DRIVING
	static constexpr int ST_COMBAT = 5;
	static constexpr int ST_AUTOMAP = 6;
	static constexpr int ST_LOADING = 7;
	static constexpr int ST_DIALOG = 8;
	static constexpr int ST_INTRO = 9;
	static constexpr int ST_BENCHMARK = 10;
	static constexpr int ST_BENCHMARKDONE = 11;
	static constexpr int ST_MINI_GAME = 12;
	static constexpr int ST_DYING = 13;
	static constexpr int ST_EPILOGUE = 14;
	static constexpr int ST_CREDITS = 15;
	static constexpr int ST_SAVING = 16;
	static constexpr int ST_CAMERA = 18;
	static constexpr int ST_ERROR = 17;
	static constexpr int ST_TAF = 19;
	static constexpr int ST_MIXING = 20; // ? change
	static constexpr int ST_TRAVELMAP = 21;
	static constexpr int ST_CHARACTER_SELECTION = 22;
	static constexpr int ST_LOOTING = 23;
	static constexpr int ST_TREADMILL = 24;
	static constexpr int ST_BOT_DYING = 25;
	static constexpr int ST_LOGO = 26;

	static constexpr int CROSS_HAIR_CORDS[] = { 64, 33, 60, 45, 60, 56, 110, 52, 38, 85, 83, 125, 61, 45, 72, 44, 67, 49 };
	static constexpr int UAC_BUILDING_LOCATION_ON_EARTH[] = { 82, 48 };
	static constexpr int moonCoords[] = { 137, 216 };
	static constexpr int earthCoords[] = { 123, 150, 113, 140};
	static constexpr int hellCoords[] = { 150, 11 };
	static constexpr int LOCATOR_BOX_CORDS[] = { 86, -15, 106, 16, 106, 48, 136, 22, 136, 65, 136, 109, 90, 8, 100, 33, 92, 57 };
	static constexpr int moonNameCoords[] = { 124, 255, 165, 64};
	static constexpr int moonPathCoords[] = { 200, 202 };
	static constexpr int earthNameCoords[] = { 126, 160, 40, 226};
	static constexpr int earthPathCoords[] = { 167, 206 };
	static constexpr int hellNameCoords[] = { 184, 23 };
	static constexpr int hellPathCoords[] = { 137, 52 };

	static constexpr int viewStepValues[] = { 64, 0, 64, -64, 0, -64, -64, -64, -64, 0, -64, 64, 0, 64, 64, 64 };

	// Text layout constants
	static constexpr int TEXT_CHAR_WIDTH = 9;
	static constexpr int DIALOG_PADDING = 2;
	static constexpr int SCROLLBAR_PADDING = 9;
	static constexpr int INGAME_SCROLLBAR_PADDING = 34;
	static constexpr int HELP_PADDING = 32;
	static constexpr int STATUS_BAR_HEIGHT = 35;
	static constexpr int VIEW_RECT_Y = 20;

	// Dialog colors are now in DialogStyleDef defaults (GameDataParsers.cpp)
	static constexpr int PLAYER_DLG_COLOR = 0xFF005617; // Referenced by defaults array

	// Dialog style data loaded from dialogs.yaml (pointer to avoid memset UB)
	DialogStyleDef* dialogStyleDefs = nullptr;
	int dialogStyleDefCount = 0;

	static constexpr int REPAINT_CLEAR = 1;
	static constexpr int REPAINT_SOFTKEYS = 2;
	static constexpr int REPAINT_MENU = 4;
	static constexpr int REPAINT_STARTUP_LOGO = 8;
	static constexpr int REPAINT_HUD = 16;
	static constexpr int REPAINT_PARTICLES = 32;
	static constexpr int REPAINT_VIEW3D = 64;
	static constexpr int REPAINT_LOADING_BAR = 128;

	static bool isLoaded;

	int8_t* OSC_CYCLE = nullptr;
	int sysSoundDelayTime = 0;
	int sysSoundTime = 0;
	int8_t* keys_numeric = nullptr;
	int lastBacklightRefresh = 0;
	int blockInputTime = 0;
	bool changeMapStarted = false;
	int storyX = 0;
	int storyY = 0;
	int storyPage = 0;
	int storyTotalPages = 0;
	int storyIndexes[4] = {};
	int fontRenderMode = 0;
	Image* imgFont_16p_Light = nullptr;
	Image* imgFont_16p_Dark = nullptr;
	Image* imgFont_18p_Light = nullptr;
	Image* imgWarFont = nullptr;
	Image* imgFont = nullptr;
	Image* imgUIImages = nullptr;
	Image* imgMapCursor = nullptr;
	Image* imgLoadingFire = nullptr;
	Image* imgIcons_Buffs = nullptr;
	Image* imgFabricBG = nullptr;
	Image* imgDialogScroll = nullptr;
	Image* imgEndOfLevelStatsBG = nullptr;
	Image* imgGameHelpBG = nullptr;
	Image* imgInventoryBG = nullptr;
	Image* imgBootL = nullptr;
	Image* imgBootR = nullptr;
	Image* imgStartupLogo = nullptr;
	Image* imgProlog = nullptr;
	Image* imgMagGlass = nullptr;
	Image* imgTravelBG = nullptr;
	Image* imgTravelPath = nullptr;
	Image* imgNameHighlight = nullptr;
	Image* imgSpaceShip = nullptr;
	Image* imgTierCloseUp = nullptr;
	Image* imgEarthCloseUp = nullptr;
	Image* imgStarField = nullptr;
	Image* imgMapHorzGridLines = nullptr;
	Image* imgMapVertGridLines = nullptr;
	Image* imgScientistMugs = nullptr;
	Image* imgCharacter_upperbar = nullptr;
	Image* imgMajorMugs = nullptr;
	Image* imgSargeMugs = nullptr;
	Image* imgCharacterSelectionAssets = nullptr;
	Image* imgCharSelectionBG = nullptr;
	Image* imgCharacter_select_stat_bar = nullptr;
	Image* imgCharacter_select_stat_header = nullptr;
	Image* imgTopBarFill = nullptr;
	Image* imgMajor_legs = nullptr;
	Image* imgMajor_torso = nullptr;
	Image* imgRiley_legs = nullptr;
	Image* imgRiley_torso = nullptr;
	Image* imgSarge_legs = nullptr;
	Image* imgSarge_torso = nullptr;
	bool m_controlButtonIsTouched = false;
	fmButton* m_controlButton = nullptr;
	int m_controlButtonTime = 0;
	bool abortMove = false;
	int SCR_CX = 0;
	int SCR_CY = 0;
	int saveX = 0;
	int saveY = 0;
	int saveZ = 0;
	int saveAngle = 0;
	int savePitch = 0;
	short saveCeilingHeight = 0;
	int viewX = 0;
	int viewY = 0;
	int viewZ = 0;
	int viewAngle = 0;
	int viewPitch = 0;
	int viewRoll = 0;
	int destX = 0;
	int destY = 0;
	int destZ = 0;
	int destAngle = 0;
	int destPitch = 0;
	int destRoll = 0;
	int zStep = 0;
	int pitchStep = 0;
	int prevX = 0;
	int prevY = 0;
	int viewSin = 0;
	int viewCos = 0;
	int viewStepX = 0;
	int viewStepY = 0;
	int viewRightStepX = 0;
	int viewRightStepY = 0;
	int knockbackX = 0;
	int knockbackY = 0;
	int knockbackDist = 0;
	int knockbackStart = 0;
	int knockbackWorldDist = 0;
	int animFrames = 0;
	int animPos = 0;
	int animAngle = 0;
	bool automapDrawn = false;
	int automapTime = 0;
	int specialLootIcon = 0;
	bool showSpeeds = false;
	bool showLocation = false;
	bool showFreeHeap = false;
	bool tellAFriend = false;
	bool showMoreGames = false;
	bool combatDone = false;
	int scrollWithBarMaxChars = 0;
	int scrollMaxChars = 0;
	int dialogMaxChars = 0;
	int dialogWithBarMaxChars = 0;
	int menuHelpMaxChars = 0;
	int subtitleMaxChars = 0;
	int menuScrollWithBarMaxChars = 0;
	int ingameScrollWithBarMaxChars = 0;
	int state = 0;
	int oldState = 0;
	bool stateChanged = false;
	int loadMapID = 0;
	short loadMapStringID = 0;
	int lastMapID = 0;
	int startupMap = 0;
	int loadType = 0;
	int saveType = 0;
	bool renderOnly = false;
	bool skipIntro = false;
	bool vibrateEnabled = false;
	int vibrateTime = 0;
	bool recentBriefSave = false;
	int shakeTime = 0;
	int shakeIntensity = 0;
	int shakeX = 0;
	int shakeY = 0;
	short dialogIndexes[1024] = {};
	Text* dialogBuffer = nullptr;
	EntityDef* dialogItem = nullptr;
	int dialogViewLines = 0;
	int dialogLineStartTime = 0;
	int dialogStartTime = 0;
	int dialogTypeLineIdx = 0;
	int dialogStyle = 0;
	int dialogType = 0;
	int dialogFlags = 0;
	bool dialogResumeScriptAfterClosed = false;
	bool dialogResumeMenu = false;
	bool dialogClosing = false;
	ScriptThread* dialogThread = nullptr;
	int numDialogLines = 0;
	int currentDialogLine = 0;
	bool showingLoot = false;
	ScriptThread* armorRepairThread = nullptr;
	bool repairingArmor = false;
	ScriptThread* targetPracticeThread = nullptr;
	ScriptThread* gotoThread = nullptr;
	int beforeRender = 0;
	int afterRender = 0;
	int flushTime = 0;
	int loopStart = 0;
	int loopEnd = 0;
	int pauseTime = 0;
	int lastPacifierUpdate = 0;
	int lastRenderTime = 0;
	int lastFrameTime = 0;
	int debugTime = 0;
	int deathTime = 0;
	int familiarDeathTime = 0;
	bool familiarSelfDestructed = false;
	int scrollingTextStart = 0;
	int scrollingTextEnd = 0;
	int scrollingTextMSLine = 0;
	int scrollingTextLines = 0;
	int scrollingTextSpacing = 0;
	bool scrollingTextDone = false;
	int scrollingTextFontHeight = 0;
	int scrollingTextSpacingHeight = 0;
	int lootingTime = 0;
	bool crouchingForLoot = false;
	bool field_0xac5_ = false;
	int st_fields[Canvas::SPD_NUM_FIELDS] = {};
	bool st_enabled = false;
	int st_count = 0;
	int events[Canvas::MAX_EVENTS] = {};
	int numEvents = 0;
	int stateVars[Canvas::MAX_STATE_VARS] = {};
	int pacifierX = 0;
	int automapBlinkTime = 0;
	bool automapBlinkState = false;
	bool endingGame = false;
	bool running = false;
	bool displaySoftKeys = false;
	int readyWeaponSound = 0;
	int displayRect[4] = {};
	int screenRect[4] = {};
	int viewRect[4] = {};
	int hudRect[4] = {};
	int menuRect[4] = {};
	int cinRect[4] = {};
	int softKeyY = 0;
	int softKeyLeftID = 0;
	int softKeyRightID = 0;
	int helpMessageTypes[16] = {};
	int helpMessageInts[16] = {};
	void* helpMessageObjs[16] = {};
	char helpMessageThreads[16] = {};
	int numHelpMessages = 0;
	int renderSceneCount = 0;
	int staleTime = 0;
	bool staleView = false;
	int keyPressedTime = 0;
	int lastKeyPressedTime = 0;
	bool pushedWall = false;
	int pushedTime = 0;
	int lootSource = 0;
	Text* errorBuffer = nullptr;
	Graphics graphics{};
	float blendSpecialAlpha = 0.0f;
	int repaintFlags = 0;
	char updateChar = 0;
	int totalFrameTime = 0;
	int ignoreFrameInput = 0;
	bool forcePump = false;
	bool updateFacingEntity = false;
	bool touched = false;
	int m_controlMode = 0;
	bool isFlipControls = false;
	int m_controlGraphic = 0;
	int m_controlLayout = 0;
	int m_controlAlpha = 0;
	Image* imgDpad_default = nullptr;
	Image* imgDpad_up_press = nullptr;
	Image* imgDpad_down_press = nullptr;
	Image* imgDpad_left_press = nullptr;
	Image* imgDpad_right_press = nullptr;
	Image* imgArrows[16] = {};
	Image* imgPageUP_Icon = nullptr;
	Image* imgPageDOWN_Icon = nullptr;
	Image* imgPageOK_Icon = nullptr;
	Image* imgSniperScope_Dial = nullptr;
	Image* imgSniperScope_Knob = nullptr;
	fmButtonContainer* m_controlButtons[6] = {};
	fmButtonContainer* m_sniperScopeButtons = nullptr;
	fmScrollButton* m_sniperScopeDialScrollButton = nullptr;
	fmButtonContainer* m_dialogButtons = nullptr;
	fmButtonContainer* m_characterButtons = nullptr;
	fmButtonContainer* m_softKeyButtons = nullptr;
	fmButtonContainer* m_mixingButtons = nullptr;
	fmButtonContainer* m_storyButtons = nullptr;
	fmButtonContainer* m_treadmillButtons = nullptr;
	fmSwipeArea* m_swipeArea[2] = {};

	int headShotTime = 0;
	int bodyShotTime = 0;
	int legShotTime = 0;
	bool keyDown = false;
	bool keyDownCausedMove = false;
	bool mediaLoading = false;
	bool areSoundsAllowed = false;
	bool inInitMap = false;
	int pacLogoTime = 0;
	int dialogRect[4] = {};
	bool selfDestructScreenShakeStarted = false;
	int CAMERAVIEW_BAR_HEIGHT = 0;
	int loadingFlags = 0;
	short loadingStringID = 0;
	short loadingStringType = 0;
	bool isZoomedIn = false;
	int zoomFOV = 0;
	int zoomDestFOV = 0;
	int zoomMinFOVPercent = 0;
	int zoomCurFOVPercent = 0;
	int zoomTime = 0;
	int zoomAngle = 0;
	int zoomPitch = 0;
	int zoomStateTime = 0;
	int zoomAccuracy = 0;
	int zoomMaxAngle = 0;
	int zoomCollisionX = 0;
	int zoomCollisionY = 0;
	int zoomCollisionZ = 0;
	int zoomTurn = 0;
	Image* imgFlak = nullptr;
	Image* imgFire = nullptr;
	int32_t* movieEffects = nullptr;
	int* fadeRect = nullptr;
	int fadeFlags = 0;
	int fadeTime = 0;
	int fadeDuration = 0;
	int fadeColor = 0;
	short lootPoolIndices[18] = {};
	int lootPool[9] = {};
	int lootPoolCredits = 0;
	int numPoolItems = 0;
	int numLootItems = 0;
	Text* lootText = nullptr;
	int lootLineNum = 0;
	int lootingCachedPitch = 0;
	int treadmillNumSteps = 0;
	int treadmillLastStep = 0;
	int treadmillReturnCode = 0;
	int treadmillLastStepTime = 0;
	short TM_LastLevelId = 0;
	short TM_LoadLevelId = 0;
	bool TM_NewGame = false;
	int totalTMTimeInPastAnimations = 0;
	int targetX = 0;
	int targetY = 0;
	int xDiff = 0;
	int yDiff = 0;
	int mapWidth = 0;
	int mapHeight = 0;
	int _field_0xf20 = 0;
	int _field_0xf24 = 0;
	int _field_0xf28 = 0;
	int _field_0xf2c = 0;
	int miniGameHelpScrollPosition = 0;
	int helpTextNumberOfLines = 0;

	// State object registry — states with ICanvasState implementations are dispatched through them
	// Max state ID is ST_LOGO = 26, so 32 slots is enough
	static constexpr int MAX_STATE_ID = 32;
	ICanvasState* stateHandlers[MAX_STATE_ID] = {};

	// Register a state handler (Canvas does NOT own the pointer)
	void registerStateHandler(int stateId, ICanvasState* handler);

	// Constructor
	Canvas();
	// Destructor
	~Canvas();

	bool startup();
	void flushGraphics();
	void backPaint(Graphics* graphics);
	void run();
	void clearEvents(int ignoreFrameInput);
	void loadRuntimeData();
	void freeRuntimeData();
	void startShake(int i, int i2, int i3);
	void setState(int state);
	void setAnimFrames(int animFrames);
	void checkFacingEntity();
	void finishMovement();
	int flagForWeapon(int i);
	int flagForFacingDir(int i);
	void startRotation(bool b);
	void finishRotation(bool b);
	int getKeyAction(int state);
	bool attemptMove(int n, int n2);
	void loadState(int loadType, short n, short n2);
	void saveState(int saveType, short n, short n2);
	void loadMap(int loadMapID, bool b, bool tm_NewGame);
	void loadPrologueText();
	void loadEpilogueText();
	void setupCharacterSelection();
	void disposeIntro();
	void disposeEpilogue();
	void drawScroll(Graphics* graphics, int n, int n2, int n3, int n4);
	void initScrollingText(short i, short i2, bool dehyphenate, int spacingHeight, int numLines, int textMSLine);
	void drawCredits(Graphics* graphics);
	void drawScrollingText(Graphics* graphics);
	// handleDialogEvents moved to DialogState::handleInput()
	bool handlePlayingEvents(int key, int action);
	bool handleCinematicInput(int action);
	bool shouldFakeCombat(int n, int n2, int n3);
	bool endOfHandlePlayingEvent(int action, bool b);
	bool handleEvent(int state);
	void runInputEvents();
	bool loadMedia();
	// combatState moved to CombatState::update()
	void dialogState(Graphics* graphics);
	void automapState();
	void renderOnlyState();
	// playingState moved to PlayingState::update()
	// menuState moved to MenuState::update()
	// dyingState moved to DyingState::update()
	// familiarDyingState moved to BotDyingState::update()
	void logoState();
	void drawScrollBar(Graphics* graphics, int i, int i2, int i3, int i4, int i5, int i6, int i7);
	void uncoverAutomap();
	void drawAutomap(Graphics* graphics, bool b);
	void closeDialog(bool skipDialog);
	void prepareDialog(Text* text, int dialogStyle, int dialogFlags);
	void startDialog(ScriptThread* scriptThread, short n, int n2, int n3);
	void startDialog(ScriptThread* scriptThread, short n, short n2, int n3, int n4, bool b);
	void startDialog(ScriptThread* scriptThread, Text* text, int n, int n2);
	void startDialog(ScriptThread* dialogThread, Text* text, int n, int n2, bool dialogResumeScriptAfterClosed);
	void renderScene(int viewX, int viewY, int viewZ, int viewAngle, int viewPitch, int viewRoll, int viewFov);
	void startSpeedTest(bool b);
	void backToMain(bool b);
	void drawPlayingSoftKeys();
	void changeStoryPage(int i);
	void drawStory(Graphics* graphics);
	int getCharacterConstantByOrder(int i);
	void drawCharacterSelection(Graphics* graphics);
	void drawCharacterSelectionAvatar(int i, int x, int y, Graphics* graphics);
	void drawCharacterSelectionStats(int i, Text* text, int x, int y, Graphics* graphics);
	void dequeueHelpDialog();
	void dequeueHelpDialog(bool b);
	void enqueueHelpDialog(short n);
	bool enqueueHelpDialog(short n, short n2, uint8_t b);
	bool enqueueHelpDialog(Text* text);
	bool enqueueHelpDialog(Text* text, int n);
	void enqueueHelpDialog(EntityDef* entityDef);
	void updateView();
	void clearSoftKeys();
	void clearLeftSoftKey();
	void clearRightSoftKey();
	void setLeftSoftKey(short i, short i2);
	void setRightSoftKey(short i, short i2);
	void setSoftKeys(short n, short n2, short n3, short n4);
	void checkHudEvents();
	void drawSoftKeys(Graphics* graphics);
	void setLoadingBarText(short loadingStringID, short loadingStringType);
	void updateLoadingBar(bool b);
	void drawLoadingBar(Graphics* graphics);
	void unloadMedia();
	void invalidateRect();
	int getRecentLoadType();
	void initZoom();
	void zoomOut();
	bool handleZoomEvents(int key, int action);
	bool handleZoomEvents(int key, int action, bool b);
	void handleCharacterSelectionInput(int key, int action);
	void handleStoryInput(int key, int action);
	void lootingState();
	void handleLootingEvents(int action);
	void drawLootingMenu(Graphics* graphics);
	void poolLoot(int* array);
	void giveLootPool();
	bool handleTreadmillEvents(int action);
	void treadmillState();
	bool treadmillFall();
	void drawTreadmillReadout(Graphics* graphics);
	void drawTargetPracticeScore(Graphics* graphics);
	void drawTravelMap(Graphics* graphics);
	bool newLevelSamePlanet();
	void drawAppropriateCloseup(Graphics* graphics, int n, bool b);
	bool drawDottedLine(Graphics* graphics, int n);
	bool drawDottedLine(Graphics* graphics);
	bool drawMarsToMoonLinePlusSpaceShip(Graphics* graphics, int n);
	int yCoordOfSpaceShip(int n);
	bool drawMoonToEarthLine(Graphics* graphics, int n, bool b);
	bool drawEarthToHellLine(Graphics* graphics, int n, bool b);
	void drawLocatorBoxAndName(Graphics* graphics, bool b, int n, Text* text);
	void drawGridLines(Graphics* graphics, int i);
	bool onMoon(int n);
	bool onEarth(int n);
	bool inHell(int n);
	void handleTravelMapInput(int key, int action);
	void finishTravelMapAndLoadLevel();
	bool drawLocatorLines(Graphics* graphics, int n, bool b, bool b2);
	void initTravelMap();
	void disposeTravelMap();
	void drawStarFieldPage(Graphics* graphics);
	void drawStarField(Graphics* graphics, int x, int y);
	void runStarFieldFrame();
	

	void loadMiniGameImages();
	void playIntroMovie(Graphics* graphics);
	void exitIntroMovie(bool b);

	void setMenuDimentions(int x, int y, int w, int h);
	void setBlendSpecialAlpha(float alpha);
	void touchStart(int pressX, int pressY);
	void touchMove(int pressX, int pressY);
	void touchEnd(int pressX, int pressY);
	void touchEndUnhighlight();

	
	void initMiniGameHelpScreen();
	void drawMiniGameHelpScreen(Graphics* graphics, int i, int i2, Image* image);
	void drawMiniGameHelpText(Graphics* graphics, int i, int i2);
	void handleMiniGameHelpScreenScroll(int i);

	
	
	void disposeCharacterSelection();

	bool pitchIsControlled(int n, int n2, int n3);
	int touchToKey_Play(int pressX, int pressY);
	bool startArmorRepair(ScriptThread* armorRepairThread);
	void endArmorRepair();
	
	void drawTouchSoftkeyBar(Graphics* graphics, bool highlighted_Left, bool highlighted_Right);
	void touchSwipe(int swDir);

	
	void turnEntityIntoWaterSpout(Entity* entity);
	void flipControls();
	void setControlLayout();
	void evaluateMiniGameResults(int n);
	void addEvents(int event); // [GEC]
};
