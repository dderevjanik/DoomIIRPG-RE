#pragma once
class Image;
class Text;
class Entity;
class fmButtonContainer;
class Graphics;
class Applet;

class Hud
{
private:

public:
    Applet* app = nullptr; // Set in startup(), replaces CAppContainer::getInstance()->app
    static constexpr int MSG_DISPLAY_TIME = 700;
    static constexpr int MSG_FLASH_TIME = 100;
    static constexpr int SCROLL_START_DELAY = 750;
    static constexpr int MS_PER_CHAR = 64;
    static constexpr int MAX_MESSAGES = 5;
    static constexpr int REPAINT_EFFECTS = 1;
    static constexpr int REPAINT_TOP_BAR = 2;
    static constexpr int REPAINT_BOTTOM_BAR = 4;
    static constexpr int REPAINT_BUBBLE_TEXT = 8;
    static constexpr int REPAINT_SUBTITLES = 16;
    static constexpr int REPAINT_HUD_OVERDRAW = 32;
    static constexpr int REPAINT_DPAD = 64;
    static constexpr int REPAINT_PLAYING_FLAGS = 107;
    static constexpr int REPAINT_CAMERA_FLAGS = 24;
    static constexpr int MSG_FLAG_NONE = 0;
    static constexpr int MSG_FLAG_FORCE = 1;
    static constexpr int MSG_FLAG_CENTER = 2;
    static constexpr int MSG_FLAG_IMPORTANT = 4;
    static constexpr int STATUSBAR_ICON_PICKUP = 0;
    static constexpr int STATUSBAR_ICON_ATTACK = 1;
    static constexpr int STATUSBAR_ICON_CHAT = 2;
    static constexpr int STATUSBAR_ICON_USE = 3;
    static constexpr int HUDARROWS_SIZE = 12;
    static constexpr int BUBBLE_TEXT_TIME = 1500;
    static constexpr int SENTRY_BOT_ICONS_PADDING = 15;
    static constexpr int DAMAGE_OVERLAY_TIME = 1000;
    static constexpr int ACTION_ICON_SIZE = 18;

    static constexpr int MAX_WEAPON_BUTTONS = 15;


	int touchMe = 0;
	int repaintFlags = 0;
    Image* imgScope = nullptr;
    Image* imgActions = nullptr;
    Image* imgAttArrow = nullptr;
    Image* imgDamageVignette = nullptr;
    Image* imgDamageVignetteBot = nullptr;
    Image* imgBottomBarIcons = nullptr;
    Image* imgAmmoIcons = nullptr;
    Image* imgSoftKeyFill = nullptr;
    Image* imgCockpitOverlay = nullptr;
    bool cockpitOverlayRaw = false;
    Image* imgPortraitsSM = nullptr;
    Image* imgPlayerFaces = nullptr;
    Image* imgPlayerActive = nullptr;
    Image* imgPlayerFrameNormal = nullptr;
    Image* imgPlayerFrameActive = nullptr;
    Image* imgHudFill = nullptr;
    Image* imgIce = nullptr;
    Image* imgSentryBotFace = nullptr;
    Image* imgSentryBotActive = nullptr;
    bool isInWeaponSelect = false;
    Image* imgPanelTop = nullptr;
    Image* imgPanelTopSentrybot = nullptr;
    Image* imgWeaponNormal = nullptr;
    Image* imgWeaponActive = nullptr;
    Image* imgShieldNormal = nullptr;
    Image* imgShieldButtonActive = nullptr;
    Image* imgKeyNormal = nullptr;
    Image* imgKeyActive = nullptr;
    Image* imgHealthNormal = nullptr;
    Image* imgHealthButtonActive = nullptr;
    Image* imgSwitchRightNormal = nullptr;
    Image* imgSwitchRightActive = nullptr;
    Image* imgSwitchLeftNormal = nullptr;
    Image* imgSwitchLeftActive = nullptr;
    Image* imgVendingSoftkeyPressed = nullptr;
    Image* imgVendingSoftkeyNormal = nullptr;
    Image* imgInGameMenuSoftkey = nullptr;
    Image* imgNumbers = nullptr;
    Image* imgHudTest = nullptr;
    Text* messages[Hud::MAX_MESSAGES] = {};
    int messageFlags[Hud::MAX_MESSAGES] = {};
    int msgCount = 0;
    int msgTime = 0;
    int msgDuration = 0;
    int subTitleID = 0;
    int subTitleTime = 0;
    int cinTitleID = 0;
    int cinTitleTime = 0;
    Text* bubbleText = nullptr;
    int bubbleTextTime = 0;
    int bubbleColor = 0;
    int damageTime = 0;
    int damageCount = 0;
    int damageDir = 0;
    Entity* lastTarget = nullptr;
    int monsterStartHealth = 0;
    int monsterDestHealth = 0;
    int playerStartHealth = 0;
    int playerDestHealth = 0;
    int monsterHealthChangeTime = 0;
    int playerHealthChangeTime = 0;
    bool showCinPlayer = false;
    int drawTime = 0;
    fmButtonContainer* m_hudButtons = nullptr;
    fmButtonContainer* m_weaponsButtons = nullptr;
    int weaponPressTime = 0;

	// Constructor
	Hud();
	// Destructor
	~Hud();

	bool startup();
    void shiftMsgs();
    void calcMsgTime();
    void addMessage(short i);
    void addMessage(short i, short i2);
    void addMessage(short i, int i2);
    void addMessage(short i, short i2, int i3);
    void addMessage(Text* text);
    void addMessage(Text* text, int flags);
    Text* getMessageBuffer();
    Text* getMessageBuffer(int flags);
    void finishMessageBuffer();
    bool isShiftingCenterMsg();
    void drawTopBar(Graphics* graphics);
    void drawImportantMessage(Graphics* graphics, Text* text, int color);
    void drawCenterMessage(Graphics* graphics, Text* text, int color);
    void drawCinematicText(Graphics* graphics);
    void drawEffects(Graphics* graphics);
    void drawDamageVignette(Graphics* graphics);
    void smackScreen(int vScrollVelocity);
    void stopScreenSmack();
    void brightenScreen(int maxLocalBrightness, int brightnessInitialBoost);
    void stopBrightenScreen();
    void drawOverlay(Graphics* graphics);
    void drawHudOverdraw(Graphics* graphics);
    void drawBottomBar(Graphics* graphics);
    void draw(Graphics* graphics);
    void drawMonsterHealth(Graphics* graphics);
    void showSpeechBubble(int i, int i2);
    void drawBubbleText(Graphics* graphics);
    void drawArrowControls(Graphics* graphics);
    void drawWeapon(Graphics* graphics, int x, int y, int weapon, bool highlighted);
    void drawNumbers(Graphics* graphics, int x, int y, int space, int num, int weapon);
    void drawCurrentKeys(Graphics* graphics, int x, int y);
    void drawWeaponSelection(Graphics* graphics);
    void handleUserMoved(int pressX, int pressY);
    void handleUserTouch(int pressX, int pressY, bool highlighted);
    void update();

private:
    void drawDelimitedLines(Graphics* graphics, Text* text, char delimiter, int x, int y, int lineHeight, int anchorFlags);
};
