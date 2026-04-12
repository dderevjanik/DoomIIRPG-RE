#pragma once
#include "Text.h"
#include "MenuItem.h"
#include "MenuStrings.h"
#include "DataNode.h"
#include <string>
#include <vector>
#include <unordered_map>

class Applet;
class EntityDef;
class Image;
class MenuItem;
class Text;
class fmButtonContainer;
class fmScrollButton;
class Graphics;
class MenuSystem;

class MenuSystem
{
private:

public:
	Applet* app; // Set in startup(), replaces CAppContainer::getInstance()->app

	static constexpr int NO = 0;
	static constexpr int YES = 1;

	static constexpr int INDEX_UAC_CREDITS = 0;
	static constexpr int INDEX_WEAPONS = 1;
	static constexpr int INDEX_ENERGY_DRINKS = 2;
	static constexpr int INDEX_OTHER = 3;
	static constexpr int INDEX_ITEMS = 4;
	static constexpr int MAX_SAVED_INDEXES = 5;

	static constexpr int BLOCK_LINE_MASK = 63;
	static constexpr int BLOCK_NUMLINES_SHIFT = 26;
	static constexpr int BLOCK_CURLINE_SHIFT = 20;
	static constexpr int BLOCK_OFS_MASK = 1023;
	static constexpr int BLOCK_CHARS_TO_DRAW_SHIFT = 10;
	static constexpr int MAX_MENUITEMS = 50;
	static constexpr int EMPTY_TEXT = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::EMPTY_STRING);
	static constexpr short MAIN_BG_OFFSET_Y = 20;
	static constexpr int MAXSTACKCOUNT = 10;
	static constexpr int TOP_Y = 1;
	static constexpr int MAX_MORE_GAMES = 4;
	static constexpr int TITLE_VENDING_Y_OFFSET = 8;
	static constexpr int TOUCH_MENU_TYPE_MAIN = 0;
	static constexpr int TOUCH_MENU_TYPE_INGAME = 1;
	static constexpr int TOUCH_MENU_TYPE_VENDING = 2;
    static constexpr int MAIN_MENU_PADDING_LEFT = 22;
    static constexpr int MAIN_MENU_PADDING_RIGHT = 23;
    static constexpr int MAIN_MENU_PADDING_TOP = 23;
    static constexpr int MAIN_MENU_PADDING_BOTTOM = 18;
    static constexpr int MAIN_MENU_SCROLL_BAR_WIDTH = 15;
    static constexpr int MAIN_MENU_SCROLL_HANDLE_WIDTH = 30;
    static constexpr int MAIN_MENU_SCROLL_HANDLE_HEIGHT = 36;
    static constexpr int MAIN_MENU_SCROLL_HANDLE_PADDING_TOP = -3;
    static constexpr int MAIN_MENU_SCROLL_HANDLE_PADDING_LEFT = -10;
    static constexpr int MAIN_MENU_ITEM_HEIGHT = 41;
    static constexpr int MAIN_MENU_ITEM_PADDING_BOTTOM = 9;
    static constexpr int MAIN_MENU_ITEM_INFO_WIDTH = 0;
    static constexpr int MAIN_MENU_ITEM_INFO_PADDING_LEFT = 0;
    static constexpr int MAIN_MENU_TITLE_PADDING_ABOVE = 10;
    static constexpr int MAIN_MENU_TITLE_PADDING_BELOW = 25;
    static constexpr int INGAME_MENU_PADDING_LEFT = 17;
    static constexpr int INGAME_MENU_PADDING_RIGHT = 24;
    static constexpr int INGAME_MENU_PADDING_TOP = 23;
    static constexpr int INGAME_MENU_PADDING_BOTTOM = 19;
    static constexpr int INGAME_MENU_SCROLL_BAR_WIDTH = 20;
    static constexpr int INGAME_MENU_SCROLL_HANDLE_WIDTH = 24;
    static constexpr int INGAME_MENU_SCROLL_HANDLE_HEIGHT = 62;
    static constexpr int INGAME_MENU_SCROLL_HANDLE_PADDING_TOP = 10;
    static constexpr int INGAME_MENU_SCROLL_HANDLE_PADDING_LEFT = -3;
    static constexpr int INGAME_MENU_ITEM_HEIGHT = 20;
    static constexpr int INGAME_MENU_ITEM_PADDING_BOTTOM = 17;
    static constexpr int INGAME_MENU_ITEM_INFO_WIDTH = 24;
    static constexpr int INGAME_MENU_ITEM_INFO_PADDING_LEFT = 3;
    static constexpr int INGAME_MENU_TITLE_PADDING_ABOVE = 80;
    static constexpr int INGAME_MENU_TITLE_PADDING_BELOW = 25;
    static constexpr int VENDING_MENU_PADDING_LEFT = 10;
    static constexpr int VENDING_MENU_PADDING_RIGHT = 11;
    static constexpr int VENDING_MENU_PADDING_TOP = 15;
    static constexpr int VENDING_MENU_PADDING_BOTTOM = 4;
    static constexpr int VENDING_MENU_SCROLL_BAR_WIDTH = 31;
    static constexpr int VENDING_MENU_SCROLL_HANDLE_WIDTH = 0;
    static constexpr int VENDING_MENU_SCROLL_HANDLE_HEIGHT = 0;
    static constexpr int VENDING_MENU_SCROLL_HANDLE_PADDING_TOP = 0;
    static constexpr int VENDING_MENU_SCROLL_HANDLE_PADDING_LEFT = 0;
    static constexpr int VENDING_MENU_ITEM_HEIGHT = 59;
    static constexpr int VENDING_MENU_ITEM_PADDING_BOTTOM = 10;
    static constexpr int VENDING_MENU_ITEM_INFO_WIDTH = 38;
    static constexpr int VENDING_MENU_ITEM_INFO_PADDING_LEFT = 4;
    static constexpr int VENDING_MAIN_MENU_ITEM_HEIGHT = 36;
    static constexpr int VENDING_MAIN_MENU_ITEM_PADDING_BOTTOM = 16;
    static constexpr int VENDING_MENU_TITLE_PADDING_ABOVE = 0;
    static constexpr int VENDING_MENU_TITLE_PADDING_BELOW = 11;
    static constexpr int VENDING_MAIN_MENU_TITLE_PADDING_BELOW = 21;
    static constexpr int MENU_TOP_SECTION_HEIGHT = 250;
    static constexpr int MENU_TOP_SECTION_OVERLAP = 0;
    static constexpr int MENU_SOFTKEY_WIDTH = 99;
    static constexpr int MENU_SOFTKEY_HEIGHT = 37;
    static constexpr int MENU_SOFTKEY_PADDING_ABOVE = 16;
    static constexpr int MENU_SOFTKEY_PADDING_SIDE = 19;
    static constexpr int MENU_TOP_SECTION_HEIGHT_VENDING = 223;
    static constexpr int MENU_TOP_SECTION_OVERLAP_VENDING = 0;
    static constexpr int MENU_SOFTKEY_WIDTH_VENDING = 104;
    static constexpr int MENU_SOFTKEY_HEIGHT_VENDING = 36;
    static constexpr int MENU_SOFTKEY_PADDING_ABOVE_VENDING = 0;
    static constexpr int MENU_SOFTKEY_PADDING_SIDE_VENDING = 14;
    static constexpr int MENU_NON_FULL_SCREEN_OFFSET_Y = 144;
    static constexpr int POP_UP_WIDTH = 240;
    static constexpr int POP_UP_TEXT_OFFSET_X = 25;
    static constexpr int UP_ARROW_OFFSET_X = 151;
    static constexpr int UP_ARROW_OFFSET_Y = 100;
    static constexpr int DOWN_ARROW_PADDING_ABOVE = 32;
    static constexpr int ARROW_WIDTH = 45;
    static constexpr int ARROW_HEIGHT = 39;
    static constexpr int SOFTKEY_PRESS_LEFT = 0;
    static constexpr int SOFTKEY_PRESS_RIGHT = 1;

    int touchMe = 0;
    uint32_t* menuData = nullptr;
    uint32_t menuDataCount = 0;
    uint32_t* menuItems = nullptr;
    uint32_t menuItemsCount = 0;
    short LEVEL_STATS_nextMap = 0;
    int menuParam = 0;
    EntityDef* detailsDef = nullptr;
    int indexes[10] = {};
    Image* imgMainBG = nullptr;
    Image* imgLogo = nullptr;
    Image* background = nullptr;
    bool drawLogo = false;
    int lastOffer = 0;
    MenuItem items[50] = {};
    int numItems = 0;
    int menu = 0;
    int oldMenu = 0;
    int selectedIndex = 0;
    int scrollIndex = 0;
    int type = 0;
    int maxItems = 0;
    int cheatCombo = 0;
    int startTime = 0;
    int menuMode = 0;
    int stackCount = 0;
    int menuStack[10] = {};
    uint8_t menuIdxStack[10] = {};
    int poppedIdx[1] = {};
    Text* detailsTitle = nullptr;
    Text* detailsHelpText = nullptr;
    bool goBackToStation = false;
    int moreGamesPage = 0;
    enum class SliderMode { None, SfxVolume, MusicVolume, ButtonsAlpha, Binding, VibrationIntensity, Deadzone };
    SliderMode activeSlider = SliderMode::None;
    bool isChangingValues() const { return activeSlider != SliderMode::None; }
    int oldLanguageSetting = 0;
    int sfxVolumeScroll = 0;
    int musicVolumeScroll = 0;
    int alphaScroll = 0;
    bool updateSlider = false;
    int sliderID = 0;
    bool drawHelpText = false;
    int selectedHelpIndex = 0;
    fmButtonContainer* m_menuButtons = nullptr;
    fmButtonContainer* m_infoButtons = nullptr;
    fmScrollButton* m_scrollBar = nullptr;
    fmButtonContainer* m_vendingButtons = nullptr;
    bool isMainMenuScrollBar = false;
    bool isMainMenu = false;
    int menuItem_fontPaddingBottom = 0;
    int menuItem_paddingBottom = 0;
    int menuItem_height = 0;
    int menuItem_width = 0;
    Image* imgVendingButtonLarge = nullptr;
    Image* imgInGameMenuOptionButton = nullptr;
    Image* imgMenuButtonBackground = nullptr;
    Image* imgMenuButtonBackgroundOn = nullptr;
    Image* imgMenuArrowDown = nullptr;
    Image* imgMenuArrowUp = nullptr;
    Image* imgVendingArrowUpGlow = nullptr;
    Image* imgVendingArrowDownGlow = nullptr;
    Image* imgMenuDial = nullptr;
    Image* imgMenuDialKnob = nullptr;
    Image* imgMenuMainBOX = nullptr;
    Image* imgMainMenuOverLay = nullptr;
    Image* imgMainHelpOverLay = nullptr;
    Image* imgMainAboutOverLay = nullptr;
    Image* imgMenuYesNoBOX = nullptr;
    Image* imgMenuChooseDIFFBOX = nullptr;
    Image* imgMenuLanguageBOX = nullptr;
    Image* imgSwitchLeftNormal = nullptr;
    Image* imgSwitchLeftActive = nullptr;
    Image* imgMenuOptionBOX3 = nullptr;
    Image* imgMenuOptionBOX4 = nullptr;
    Image* imgMenuOptionSliderBar = nullptr;
    Image* imgMenuOptionSliderON = nullptr;
    Image* imgMenuOptionSliderOFF = nullptr;
    Image* imgHudNumbers = nullptr;
    Image* imgGameMenuPanelbottom = nullptr;
    Image* imgGameMenuPanelBottomSentrybot = nullptr;
    Image* imgGameMenuHealth = nullptr;
    Image* imgGameMenuShield = nullptr;
    Image* imgGameMenuInfoButtonPressed = nullptr;
    Image* imgGameMenuInfoButtonNormal = nullptr;
    Image* imgVendingButtonHelp = nullptr;
    Image* imgGameMenuTornPage = nullptr;
    Image* imgMainMenuDialA_Anim = nullptr;
    Image* imgMainMenuDialC_Anim = nullptr;
    Image* imgMainMenuSlide_Anim = nullptr;
    Image* imgGameMenuScrollBar = nullptr;
    Image* imgGameMenuTopSlider = nullptr;
    Image* imgGameMenuMidSlider = nullptr;
    Image* imgGameMenuBottomSlider = nullptr;
    Image* imgVendingScrollBar = nullptr;
    Image* imgVendingScrollButtonTop = nullptr;
    Image* imgVendingScrollButtonMiddle = nullptr;
    Image* imgVendingScrollButtonBottom = nullptr;
    Image* imgGameMenuBackground = nullptr;
    int dialA_Anim1 = 0;
    int dialC_Anim1 = 0;
    int dialA_Anim2 = 0;
    int dialC_Anim2 = 0;
    int slideAnim1 = 0;
    int slideAnim2 = 0;
    int animTime = 0;

    // [GEC]
    int old_0x44 = 0;
    int old_0x48 = 0;
    int scrollY1Stack[10] = {};
    int scrollY2Stack[10] = {};
    int scrollI2Stack[10] = {};
    int nextMsgTime = 0;
    int nextMsg = 0;
    Image* imgMenuButtonBackgroundExt = nullptr;
    Image* imgMenuButtonBackgroundExtOn = nullptr;

    Image* imgMenuBtnBackground = nullptr; // [GEC]
    Image* imgMenuBtnBackgroundOn = nullptr; // [GEC]
    int vibrationIntensityScroll = 0; // [GEC]
    int deadzoneScroll = 0; // [GEC]
    int resolutionIndex = 0; // [GEC]

    // YAML-native menu definitions for extended (GEC) menus
    // These avoid the 16-bit flag truncation of the binary-packed format
    struct YAMLMenuItem {
        int textField;      // string_id or 0 if using inline text
        int textField2;     // secondary string_id or 0
        int flags;          // full 32-bit flags including GEC extended
        int action;
        int param;
        int helpField;
        std::string text;   // inline text (empty if using string_id)
        std::string text2;  // inline secondary text
        // Image item fields (type: background / type: image)
        std::string itemType;    // "background", "image", or "" for regular items
        std::string imageName;   // image name from imageMap (e.g. "main_bg", "logo")
        int imageX = 0;          // x position (-1 = center horizontally)
        int imageY = 0;          // y position
        int imageAnchor = 0;     // Graphics anchor flags (0=top-left, 3=center)
        int imageRenderMode = 0; // render mode passed to drawImage
    };

    // Layout value modes for YAML-driven canvas dimensions
    enum LayoutValueMode {
        LVM_LITERAL = 0,
        LVM_CENTER,         // x = (480 - w) / 2
        LVM_BTN_WIDTH,      // w = theme button image width
        LVM_BTN_H_X4,       // h = button height * 4
        LVM_BTN_H_X5,       // h = button height * 5
        LVM_PANEL_H,        // h = 320 - panel_bottom.height
        LVM_YESNO_H,        // h = imgMenuYesNoBOX->height
        LVM_LANG_H,         // h = imgMenuLanguageBOX->height
        LVM_LOGO_BOX,       // y = logo-center using imgMenuMainBOX
        LVM_LOGO_YESNO,     // y = logo-center using imgMenuYesNoBOX
        LVM_LOGO_DIFF,      // y = logo-center using imgMenuChooseDIFFBOX
        LVM_LOGO_LANG,      // y = logo-center using imgMenuLanguageBOX
        LVM_LOGO_15,        // y = logo.height + 15
    };

    struct MenuLayout {
        LayoutValueMode xMode = LVM_LITERAL;
        int xVal = 0;
        LayoutValueMode yMode = LVM_LITERAL;
        int yVal = 0;
        LayoutValueMode wMode = LVM_LITERAL;
        int wVal = 0;
        LayoutValueMode hMode = LVM_LITERAL;
        int hVal = 0;
        bool isSet = false;
    };

    // Scrollbar style
    enum ScrollbarStyle { SB_NONE = 0, SB_DIAL, SB_BAR };

    struct ScrollbarConfig {
        ScrollbarStyle style = SB_NONE;
        Image* barImg = nullptr;
        Image* topImg = nullptr;
        Image* midImg = nullptr;
        Image* bottomImg = nullptr;
        int x = 0;
        int width = 50;
        int defaultX = 408;
        int defaultY = 81;
    };

    struct MenuTheme {
        Image* btnImage = nullptr;
        Image* btnHighlightImage = nullptr;
        int btnRenderMode = 0;
        int btnHighlightRenderMode = 0;
        Image* infoBtnImage = nullptr;
        Image* infoBtnHighlightImage = nullptr;
        int infoBtnRenderMode = 0;
        int infoBtnHighlightRenderMode = 0;
        int itemWidth = 0;
        int itemHeight = 46;
        int itemPaddingBottom = 0;
        ScrollbarConfig scrollbar;
    };

    struct YAMLMenuDef {
        std::string name;
        int menuId;
        int type;
        int maxItems;
        std::vector<YAMLMenuItem> items;
        int helpResource = -1;
        int selectedIndex = -1;
        bool showInfoButtons = false;
        std::vector<std::string> visibleButtonNames;
        std::vector<std::string> visibleButtonsConditionalNames;
        std::vector<int> visibleButtons;           // resolved from names in loadUIFromYAML
        std::vector<int> visibleButtonsConditional; // resolved from names in loadUIFromYAML
        int itemWidth = 0;
        int vibrationY = -1;
        MenuLayout layout;
        int scrollbarXOffset = -1;
        int scrollbarYOffset = -1;
        int loadStartItem = 0;
        int scrollIndex = -1;
        bool isMissing = false;
        int yesnoStringId = -1;
        int yesnoSelectYes = 0;
        int yesnoYesAction = 0;
        int yesnoYesParam = 0;
        int yesnoNoAction = 2;
        int yesnoNoParam = 0;
        bool yesnoClearStack = false;
        // Raw YAML node for deferred theme resolution (DataNode uses shared_ptr, safe to store)
        DataNode sourceNode;
        // Resolved theme (populated after images load)
        MenuTheme resolvedTheme;
        bool hasTheme = false;
    };

    std::vector<YAMLMenuDef> yamlMenuDefs;
    std::unordered_map<int, int> yamlMenuById; // menuId -> index in yamlMenuDefs

	// Constructor
	MenuSystem();
	// Destructor
	~MenuSystem();

	bool startup();
	bool loadMenusFromYAML(const char* path);
	bool loadUIFromYAML(const char* path);
	bool loadYAMLMenuItems(int menuId);
	const MenuTheme* getThemeForMenu(int menuId) const;
	const YAMLMenuDef* getMenuDef(int menuId) const;
	Image* resolveMenuImage(const std::string& name) const;
	int resolveLayoutValue(LayoutValueMode mode, int literal, int w = 0) const;
    void buildDivider(Text* text, int i);
    bool enterDigit(int i);
    void scrollDown();
    void scrollUp();
    bool scrollPageDown();
    void scrollPageUp();
    bool shiftBlockText(int n, int i, int j);
    void moveDir(int n);
    void doDetailsSelect();
    void back();
    void setMenu(int menu);
    void paint(Graphics* graphics);
    void setItemsFromText(int i, Text* text, int i2, int i3, int i4);
    void returnToGame();
    void initMenu(int menu);
    void gotoMenu(int menu);
    void handleMenuEvents(int key, int keyAction);
    void select(int i);
    void selectDebugAction(int i);
    void selectVideoSettings(int i);
    void selectControlSettings(int i);
    void selectVendingAction(int i);
    void selectItemAction(int i);
    void infiniteLoop();
    int infiniteRecursion(int* array);
    void systemTest(int sysType);
    void startGame(bool b);
    void SetYESNO(short i, int i2, int i3, int i4);
    void SetYESNO(short i, int i2, int i3, int i4, int i5, int i6);
    void SetYESNO(Text* text, int i, int i2, int i3, int i4, int i5);
    void LoadHelpResource(short i);
    void FillRanking();
    void LoadNotebook();
    void LoadHelpItems(Text* text, int i);
    void buildFraction(int i, int i2, Text* text);
    void buildModStat(int i, int i2, Text* text);
    void buildLevelGrades(Text* text);
    void buildLevelGrade(int i, Text* text, int i2, int i3);
    void fillStatus(bool b, bool b2, bool b3);
    void saveIndexes(int i);
    void loadIndexes(int i);
    void showDetailsMenu();
    void addItem(int textField, int textField2, int flags, int action, int param, int helpField);
    void loadMenuItems(int menu, int begItem, int numItems);
    int onOffValue(bool b);
    void leaveOptionsMenu();
    int getStackCount();
    void clearStack();
    void pushMenu(int i, int i2, int Y1, int Y2, int index2);
    int popMenu(int* array, int *Y1, int* Y2, int *index2);
    int peekMenu();
    int getLastArgString();
    void fillVendingMachineSnacks(int i, Text* text);

    void setMenuSettings();
    void updateTouchButtonState();
    void handleUserTouch(int x, int y, bool b);
    void handleUserMoved(int x, int y);
    int getScrollPos();
    int getMenuItemHeight(int i);
    int getMenuItemHeight2(int i); // [GEC]
    void drawScrollbar(Graphics* graphics);
    void drawButtonFrame(Graphics* graphics);
    void drawTouchButtons(Graphics* graphics, bool b);
    void drawSoftkeyButtons(Graphics* graphics);
    int drawCustomScrollbar(Graphics* graphics, MenuItem *item, Text* text, int yPos); // [GEC]
    void drawOptionsScreen(Graphics* graphics);
    void drawNumbers(Graphics* graphics, int x, int y, int space, int number);
    bool HasVibration();
    bool isUserMusicOn();
    bool updateVolumeSlider(int buttonId, int x);
    void refresh();
    void soundClick();
};
