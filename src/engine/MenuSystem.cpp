#include <stdexcept>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include "Log.h"

#include "CAppContainer.h"
#include "DataNode.h"
#include "App.h"
#include "Hud.h"
#include "Game.h"
#include "Sound.h"
#include "Canvas.h"
#include "Player.h"
#include "MenuSystem.h"
#include "MenuStrings.h"
#include "CombatEntity.h"
#include "HackingGame.h"
#include "SentryBotGame.h"
#include "VendingMachine.h"
#include "MenuItem.h"
#include "Graphics.h"
#include "JavaStream.h"
#include "Resource.h"
#include "Button.h"
#include "Image.h"
#include "Sounds.h"
#include "Render.h"
#include "Combat.h"
#include "ComicBook.h"
#include "Enums.h"
#include "Input.h"
#include "Menus.h"
#include "SDLGL.h"
#include "Utils.h"
#include "TinyGL.h"
#include "GLES.h"
#include "SoundNames.h"
#include "Sounds.h"

MenuSystem::MenuSystem() = default;

MenuSystem::~MenuSystem() = default;

bool MenuSystem::startup() {
	this->app = CAppContainer::getInstance()->app;
	Applet* app = this->app;
	LOG_INFO("[menu] startup\n");

	LOG_INFO("[menu] loading from menus.yaml\n");
	if (!this->loadMenusFromYAML("menus.yaml")) {
		app->Error("Failed to load menus.yaml\n");
		return false;
	}

	this->numItems = 0;
	this->background = nullptr;
	this->menu = Menus::MENU_NONE;
	this->selectedIndex = 0;
	this->scrollIndex = 0;
	this->type = 0;
	this->maxItems = 0;
	this->cheatCombo = 0;
	this->goBackToStation = false;
	this->changeSfxVolume = false;
	this->changeMusicVolume = false; // [GEC]
	this->changeButtonsAlpha = false; // [GEC]
	this->changeValues = false; // [GEC]
	this->setBinding = false; // [GEC]
	this->changeVibrationIntensity = false; // [GEC]
	this->changeDeadzone = false; // [GEC]

	this->imgVendingButtonLarge = app->loadImage("vending_button_large.bmp", true);
	this->imgInGameMenuOptionButton = app->loadImage("inGame_menu_option_button.bmp", true);
	this->imgMenuButtonBackground = app->loadImage("menu_button_background.bmp", true);
	this->imgMenuButtonBackgroundOn = app->loadImage("menu_button_background_on.bmp", true);
	this->imgMenuArrowDown = app->loadImage("menu_arrow_down.bmp", true);
	this->imgMenuArrowUp = app->loadImage("menu_arrow_up.bmp", true);
	this->imgVendingArrowUpGlow = app->loadImage("vending_arrow_up_glow.bmp", true);
	this->imgVendingArrowDownGlow = app->loadImage("vending_arrow_down_glow.bmp", true);
	this->imgMenuDial = app->loadImage("Menu_Dial.bmp", true);
	this->imgMenuDialKnob = app->loadImage("Menu_Dial_Knob.bmp", true);
	this->imgMenuMainBOX = app->loadImage("Menu_Main_BOX.bmp", true);
	this->imgMainMenuOverLay = app->loadImage("Main_Menu_OverLay.bmp", true);
	this->imgMainHelpOverLay = app->loadImage("Main_Help_OverLay.bmp", true);
	this->imgMainAboutOverLay = app->loadImage("Main_About_OverLay.bmp", true);
	this->imgMenuYesNoBOX = app->loadImage("Menu_YesNo_BOX.bmp", true);
	this->imgMenuChooseDIFFBOX = app->loadImage("Menu_ChooseDIFF_BOX.bmp", true);
	this->imgMenuLanguageBOX = app->loadImage("Menu_Language_BOX.bmp", true);
	this->imgSwitchLeftNormal = app->loadImage("Switch_Left_Normal.bmp", true);
	this->imgSwitchLeftActive = app->loadImage("Switch_Left_Active.bmp", true);
	this->imgMenuOptionBOX3 = app->loadImage("Menu_Option_BOX3.bmp", true);
	this->imgMenuOptionBOX4 = app->loadImage("Menu_Option_BOX4.bmp", true);
	this->imgMenuOptionSliderBar = app->loadImage("Menu_Option_SliderBar.bmp", true);
	this->imgMenuOptionSliderOFF = app->loadImage("Menu_Option_SliderOFF.bmp", true);
	this->imgMenuOptionSliderON = app->loadImage("Menu_Option_SliderON.bmp", true);
	this->imgHudNumbers = app->loadImage("Hud_Numbers.bmp", true);
	this->imgGameMenuPanelbottom = app->loadImage("gameMenu_Panel_bottom.bmp", true);
	this->imgGameMenuPanelBottomSentrybot = app->loadImage("gameMenu_Panel_bottom_sentrybot.bmp", true);
	this->imgGameMenuHealth = app->loadImage("gameMenu_Health.bmp", true);
	this->imgGameMenuShield = app->loadImage("gameMenu_Shield.bmp", true);
	this->imgGameMenuInfoButtonPressed = app->loadImage("gameMenu_infoButton_Pressed.bmp", true);
	this->imgGameMenuInfoButtonNormal = app->loadImage("gameMenu_infoButton_Normal.bmp", true);
	this->imgVendingButtonHelp = app->loadImage("vending_button_help.bmp", true);
	this->imgGameMenuTornPage = app->loadImage("gameMenu_TornPage.bmp", true);
	this->imgGameMenuBackground = app->loadImage("gameMenu_Background.bmp", true);
	this->imgMainMenuDialA_Anim = app->loadImage("Main_Menu_DialA_anim.bmp", true);
	this->imgMainMenuDialC_Anim = app->loadImage("Main_Menu_DialC_anim.bmp", true);
	this->imgMainMenuSlide_Anim = app->loadImage("Main_Menu_Slide_anim.bmp", true);
	this->imgGameMenuScrollBar = app->loadImage("gameMenu_ScrollBar.bmp", true);
	this->imgGameMenuTopSlider = app->loadImage("gameMenu_topSlider.bmp", true);
	this->imgGameMenuMidSlider = app->loadImage("gameMenu_midSlider.bmp", true);
	this->imgGameMenuBottomSlider = app->loadImage("gameMenu_bottomSlider.bmp", true);
	this->imgVendingScrollBar = app->loadImage("vending_scroll_bar.bmp", true);
	this->imgVendingScrollButtonTop = app->loadImage("vending_scroll_button_top.bmp", true);
	this->imgVendingScrollButtonMiddle = app->loadImage("vending_scroll_button_middle.bmp", true);
	this->imgVendingScrollButtonBottom = app->loadImage("vending_scroll_button_bottom.bmp", true);
	this->imgLogo = app->loadImage("logo2.bmp", true);

	this->imgMenuButtonBackgroundExt = app->loadImage("menu_button_background.bmp", true); // [GEC]
	this->imgMenuButtonBackgroundExtOn = app->loadImage("menu_button_background_on.bmp", true); // [GEC]

	// [GEC] Fix the image
	fixImage(this->imgMenuButtonBackground);
	fixImage(this->imgMenuButtonBackgroundExt);

	enlargeButtonImage(this->imgMenuButtonBackgroundExt); // [GEC]
	enlargeButtonImage(this->imgMenuButtonBackgroundExtOn); // [GEC]

	this->imgMenuBtnBackground = this->imgMenuButtonBackground; // [GEC] Default
	this->imgMenuBtnBackgroundOn = this->imgMenuButtonBackgroundOn;  // [GEC] Default

	this->isMainMenuScrollBar = false;
	this->isMainMenu = false;
	this->menuItem_height = 46;
	this->menuItem_width = 162;
	this->menuItem_fontPaddingBottom = 0;
	this->menuItem_paddingBottom = 0;
	this->drawHelpText = false;
	this->selectedHelpIndex = -1;
	this->dialA_Anim1 = 0;
	this->dialC_Anim1 = 0;
	this->dialA_Anim2 = 0;
	this->dialC_Anim2 = 0;
	this->slideAnim1 = 0;
	this->slideAnim2 = 0;
	this->animTime = 0;

	// Load UI definitions (button containers + themes) from ui.yaml
	// Must be called after images are loaded since it references them for button creation
	LOG_INFO("[menu] loading from ui.yaml\n");
	if (!this->loadUIFromYAML("ui.yaml")) {
		app->Error("Failed to load ui.yaml\n");
		return false;
	}

	this->sliderID = 0;
	this->sfxVolumeScroll = 0;
	this->musicVolumeScroll = 0;
	this->alphaScroll = 0;
	this->vibrationIntensityScroll = 0; // [GEC]
	this->deadzoneScroll = 0; // [GEC]
	this->updateSlider = false;

	return true;
}

// --- Menu type/action/flag lookup maps (loaded from menus.yaml) ---
static const std::unordered_map<std::string, int> s_menuTypes = {
	{"default", 0}, {"list", 1}, {"confirm", 2}, {"confirm2", 3}, {"main", 4},
	{"help", 5}, {"vcenter", 6}, {"notebook", 7}, {"main_list", 8}, {"vending_machine", 9}
};
static const std::unordered_map<std::string, int> s_actions = {
	{"none", 0}, {"goto", 1}, {"back", 2}, {"load", 3}, {"save", 4},
	{"backtomain", 5}, {"togsound", 6}, {"newgame", 7}, {"exit", 8},
	{"changestate", 9}, {"difficulty", 10}, {"returntogame", 11},
	{"restartlevel", 12}, {"savequit", 13}, {"offersuccess", 14},
	{"changesfxvolume", 15}, {"showdetails", 16}, {"changemap", 17},
	{"useitemweapon", 18}, {"select_language", 19}, {"useitemsyring", 20},
	{"useitemother", 21}, {"continue", 22}, {"main_special", 23},
	{"confirmuse", 24}, {"saveexit", 25}, {"backtwo", 26}, {"minigame", 27},
	{"confirmbuy", 28}, {"buydrink", 29}, {"buysnack", 30},
	{"return_to_player", 33}, {"flip_controls", 35}, {"control_layout", 36},
	{"changemusicvolume", 37}, {"changealpha", 38}, {"change_vid_mode", 39},
	{"tog_vsync", 40}, {"change_resolution", 41}, {"apply_changes", 42},
	{"set_binding", 43}, {"default_bindings", 44}, {"tog_vibration", 45},
	{"change_vibration_intensity", 46}, {"change_deadzone", 47}, {"tog_tinygl", 48},
	{"debug", 100}, {"giveall", 102}, {"givemap", 103}, {"noclip", 104},
	{"disableai", 105}, {"nohelp", 106}, {"godmode", 107}, {"showlocation", 108},
	{"rframes", 109}, {"rspeeds", 110}, {"rskipflats", 111}, {"rskipcull", 112},
	{"rskipbsp", 114}, {"rskiplines", 115}, {"rskipsprites", 116},
	{"ronlyrender", 117}, {"rskipdecals", 118}, {"rskip2dstretch", 119},
	{"driving_mode", 120}, {"render_mode", 121}, {"equipformap", 122},
	{"oneshot", 123}, {"debug_font", 124}, {"sys_test", 125},
	{"skip_minigames", 126}, {"show_heap", 127}
};
static const std::unordered_map<std::string, int> s_flags = {
	{"noselect", 0x1}, {"nodehyphenate", 0x2}, {"disabled", 0x4}, {"align_center", 0x8},
	{"showdetails", 0x20}, {"divider", 0x40}, {"selector", 0x80}, {"block_text", 0x100},
	{"highlight", 0x200}, {"checked", 0x400}, {"right_arrow", 0x2000}, {"left_arrow", 0x4000},
	{"hidden", 0x8000}, {"scrollbar", 0x20000}, {"scrollbartwo", 0x40000},
	{"disabledtwo", 0x80000}, {"padding", 0x100000}, {"binding", 0x200000}
};

static int menuTypeFromString(const std::string& str) {
	auto it = s_menuTypes.find(str);
	if (it != s_menuTypes.end()) return it->second;
	try { return std::stoi(str); } catch (...) { return 0; }
}

static int actionFromString(const std::string& str) {
	auto it = s_actions.find(str);
	if (it != s_actions.end()) return it->second;
	try { return std::stoi(str); } catch (...) { return 0; }
}

static int flagsFromString(const std::string& str) {
	if (str == "normal" || str == "0") return 0;
	int result = 0;
	size_t start = 0;
	while (start < str.size()) {
		size_t end = str.find(',', start);
		if (end == std::string::npos) end = str.size();
		std::string token = str.substr(start, end - start);
		while (!token.empty() && token[0] == ' ') token.erase(0, 1);
		while (!token.empty() && token.back() == ' ') token.pop_back();
		auto it = s_flags.find(token);
		if (it != s_flags.end()) {
			result |= it->second;
		} else {
			try { result |= std::stoi(token, nullptr, 0); } catch (...) {}
		}
		start = end + 1;
	}
	return result;
}

// --- Menu name tables (for comments in export) ---
struct MenuNameEntry { int id; const char* name; };
static const MenuNameEntry menuNameTable[] = {
	{-6, "MENU_MAIN_CONTROLLER"}, {-5, "MENU_MAIN_BINDINGS"},
	{-4, "MENU_MAIN_CONTROLS"}, {-3, "MENU_MAIN_OPTIONS_SOUND"},
	{-2, "MENU_MAIN_OPTIONS_VIDEO"}, {-1, "MENU_MAIN_OPTIONS_INPUT"},
	{0, "MENU_NONE"}, {1, "MENU_LEVEL_STATS"}, {2, "MENU_DRAWSWORLD"},
	{3, "MENU_MAIN"}, {4, "MENU_MAIN_HELP"}, {5, "MENU_MAIN_ARMORHELP"},
	{6, "MENU_MAIN_EFFECTHELP"}, {7, "MENU_MAIN_ITEMHELP"},
	{8, "MENU_MAIN_ABOUT"}, {9, "MENU_MAIN_GENERAL"},
	{10, "MENU_MAIN_MOVE"}, {11, "MENU_MAIN_ATTACK"},
	{12, "MENU_MAIN_SNIPER"}, {13, "MENU_MAIN_EXIT"},
	{14, "MENU_MAIN_CONFIRMNEW"}, {15, "MENU_MAIN_CONFIRMNEW2"},
	{16, "MENU_MAIN_DIFFICULTY"}, {17, "MENU_MAIN_OPTIONS"},
	{18, "MENU_MAIN_MINIGAME"}, {19, "MENU_MAIN_MORE_GAMES"},
	{20, "MENU_MAIN_HACKER_HELP"}, {21, "MENU_MAIN_MATRIX_SKIP_HELP"},
	{22, "MENU_MAIN_POWER_UP_HELP"}, {23, "MENU_SELECT_LANGUAGE"},
	{24, "MENU_END_RANKING"}, {25, "MENU_ENABLE_SOUNDS"},
	{26, "MENU_END_"}, {27, "MENU_END_FINALQUIT"},
	{28, "MENU_INHERIT_BACKMENU"}, {29, "MENU_INGAME"},
	{30, "MENU_INGAME_STATUS"}, {31, "MENU_INGAME_PLAYER"},
	{32, "MENU_INGAME_LEVEL"}, {33, "MENU_INGAME_GRADES"},
	{35, "MENU_INGAME_OPTIONS"}, {36, "MENU_INGAME_LANGUAGE"},
	{37, "MENU_INGAME_HELP"}, {38, "MENU_INGAME_GENERAL"},
	{39, "MENU_INGAME_MOVE"}, {40, "MENU_INGAME_ATTACK"},
	{41, "MENU_INGAME_SNIPER"}, {42, "MENU_INGAME_EXIT"},
	{43, "MENU_INGAME_ARMORHELP"}, {44, "MENU_INGAME_EFFECTHELP"},
	{45, "MENU_INGAME_ITEMHELP"}, {46, "MENU_INGAME_QUESTLOG"},
	{47, "MENU_INGAME_RECIPES"}, {48, "MENU_INGAME_SAVE"},
	{49, "MENU_INGAME_LOAD"}, {50, "MENU_INGAME_LOADNOSAVE"},
	{51, "MENU_INGAME_DEAD"}, {52, "MENU_INGAME_RESTARTLVL"},
	{53, "MENU_INGAME_SAVEQUIT"}, {57, "MENU_INGAME_KICKING"},
	{58, "MENU_INGAME_SPECIAL_EXIT"}, {59, "MENU_INGAME_HACKER_HELP"},
	{60, "MENU_INGAME_MATRIX_SKIP_HELP"}, {61, "MENU_INGAME_POWER_UP_HELP"},
	{62, "MENU_INGAME_CONTROLS"}, {65, "MENU_DEBUG"},
	{66, "MENU_DEBUG_MAPS"}, {67, "MENU_DEBUG_STATS"},
	{68, "MENU_DEBUG_CHEATS"}, {69, "MENU_DEVELOPER_VARS"},
	{70, "MENU_DEBUG_SYS"}, {71, "MENU_SHOWDETAILS"},
	{72, "MENU_ITEMS"}, {73, "MENU_ITEMS_WEAPONS"},
	{75, "MENU_ITEMS_DRINKS"}, {77, "MENU_ITEMS_CONFIRM"},
	{79, "MENU_ITEMS_HEALTHMSG"}, {80, "MENU_ITEMS_ARMORMSG"},
	{81, "MENU_ITEMS_SYRINGEMSG"}, {82, "MENU_ITEMS_HOLY_WATER_MAX"},
	{83, "MENU_VENDING_MACHINE"}, {84, "MENU_VENDING_MACHINE_DRINKS"},
	{85, "MENU_VENDING_MACHINE_SNACKS"}, {86, "MENU_VENDING_MACHINE_CONFIRM"},
	{87, "MENU_VENDING_MACHINE_CANT_BUY"}, {88, "MENU_VENDING_MACHINE_DETAILS"},
	{89, "MENU_COMIC_BOOK"},
	{54, "MENU_INGAME_BINDINGS"}, {55, "MENU_INGAME_OPTIONS_SOUND"},
	{56, "MENU_INGAME_OPTIONS_VIDEO"}, {63, "MENU_INGAME_OPTIONS_INPUT"},
	{64, "MENU_INGAME_CONTROLLER"},
};
static const int numMenuNames = sizeof(menuNameTable) / sizeof(menuNameTable[0]);

static const char* menuIdToName(int id) {
	for (int i = 0; i < numMenuNames; i++) {
		if (menuNameTable[i].id == id) return menuNameTable[i].name;
	}
	return "MENU_UNKNOWN";
}

// Lowercase menu name -> id lookup (e.g. "main_help" -> 4)
static const struct { const char* name; int id; } menuNameLookup[] = {
	{"main_controller", -6}, {"main_bindings", -5}, {"main_controls", -4},
	{"main_options_sound", -3}, {"main_options_video", -2}, {"main_options_input", -1},
	{"none", 0}, {"level_stats", 1}, {"drawsworld", 2},
	{"main", 3}, {"main_help", 4}, {"main_armorhelp", 5},
	{"main_effecthelp", 6}, {"main_itemhelp", 7}, {"main_about", 8},
	{"main_general", 9}, {"main_move", 10}, {"main_attack", 11},
	{"main_sniper", 12}, {"main_exit", 13}, {"main_confirmnew", 14},
	{"main_confirmnew2", 15}, {"main_difficulty", 16}, {"main_options", 17},
	{"main_minigame", 18}, {"main_more_games", 19}, {"main_hacker_help", 20},
	{"main_matrix_skip_help", 21}, {"main_power_up_help", 22}, {"select_language", 23},
	{"end_ranking", 24}, {"enable_sounds", 25}, {"end", 26},
	{"end_finalquit", 27}, {"inherit_backmenu", 28}, {"ingame", 29},
	{"ingame_status", 30}, {"ingame_player", 31}, {"ingame_level", 32},
	{"ingame_grades", 33}, {"ingame_options", 35}, {"ingame_language", 36},
	{"ingame_help", 37}, {"ingame_general", 38}, {"ingame_move", 39},
	{"ingame_attack", 40}, {"ingame_sniper", 41}, {"ingame_exit", 42},
	{"ingame_armorhelp", 43}, {"ingame_effecthelp", 44}, {"ingame_itemhelp", 45},
	{"ingame_questlog", 46}, {"ingame_recipes", 47}, {"ingame_save", 48},
	{"ingame_load", 49}, {"ingame_loadnosave", 50}, {"ingame_dead", 51},
	{"ingame_restartlvl", 52}, {"ingame_savequit", 53},
	{"ingame_kicking", 57}, {"ingame_special_exit", 58},
	{"ingame_hacker_help", 59}, {"ingame_matrix_skip_help", 60},
	{"ingame_power_up_help", 61}, {"ingame_controls", 62},
	{"debug", 65}, {"debug_maps", 66}, {"debug_stats", 67},
	{"debug_cheats", 68}, {"developer_vars", 69}, {"debug_sys", 70},
	{"showdetails", 71}, {"items", 72}, {"items_weapons", 73},
	{"items_drinks", 75}, {"items_confirm", 77},
	{"items_healthmsg", 79}, {"items_armormsg", 80},
	{"items_syringemsg", 81}, {"items_holy_water_max", 82},
	{"vending_machine", 83}, {"vending_machine_drinks", 84},
	{"vending_machine_snacks", 85}, {"vending_machine_confirm", 86},
	{"vending_machine_cant_buy", 87}, {"vending_machine_details", 88},
	{"comic_book", 89},
	{"ingame_bindings", 54}, {"ingame_options_sound", 55},
	{"ingame_options_video", 56}, {"ingame_options_input", 63},
	{"ingame_controller", 64},
};
static const int numMenuNameLookup = sizeof(menuNameLookup) / sizeof(menuNameLookup[0]);

static int menuNameToId(const std::string& name) {
	for (int i = 0; i < numMenuNameLookup; i++) {
		if (name == menuNameLookup[i].name) return menuNameLookup[i].id;
	}
	// Try parsing as integer fallback
	try { return std::stoi(name); } catch (...) {}
	return 0;
}

bool MenuSystem::loadUIFromYAML(const char* path) {
	DataNode config = DataNode::loadFile(path);
	if (!config) {
		LOG_ERROR("[menu] failed to load %s\n", path);
		return false;
	}

	// Build image name -> Image* lookup from member variables
	// Maps ui.yaml image names to the already-loaded and processed member images
	std::unordered_map<std::string, Image*> imageMap;
	imageMap["menu_btn_bg"] = this->imgMenuBtnBackground;
	imageMap["menu_btn_bg_on"] = this->imgMenuBtnBackgroundOn;
	imageMap["menu_arrow_down"] = this->imgMenuArrowDown;
	imageMap["menu_arrow_up"] = this->imgMenuArrowUp;
	imageMap["switch_left_normal"] = this->imgSwitchLeftNormal;
	imageMap["switch_left_active"] = this->imgSwitchLeftActive;
	imageMap["slider_bar"] = this->imgMenuOptionSliderBar;
	imageMap["slider_on"] = this->imgMenuOptionSliderON;
	imageMap["slider_off"] = this->imgMenuOptionSliderOFF;
	imageMap["info_btn_normal"] = this->imgGameMenuInfoButtonNormal;
	imageMap["info_btn_pressed"] = this->imgGameMenuInfoButtonPressed;
	imageMap["vending_arrow_up_glow"] = this->imgVendingArrowUpGlow;
	imageMap["vending_arrow_down_glow"] = this->imgVendingArrowDownGlow;
	imageMap["vending_btn_large"] = this->imgVendingButtonLarge;
	imageMap["vending_btn_help"] = this->imgVendingButtonHelp;
	imageMap["ingame_option_btn"] = this->imgInGameMenuOptionButton;
	imageMap["menu_btn_bg_ext"] = this->imgMenuButtonBackgroundExt;
	imageMap["menu_btn_bg_ext_on"] = this->imgMenuButtonBackgroundExtOn;
	imageMap["menu_dial"] = this->imgMenuDial;
	imageMap["menu_dial_knob"] = this->imgMenuDialKnob;
	imageMap["menu_main_box"] = this->imgMenuMainBOX;
	imageMap["main_menu_overlay"] = this->imgMainMenuOverLay;
	imageMap["main_help_overlay"] = this->imgMainHelpOverLay;
	imageMap["main_about_overlay"] = this->imgMainAboutOverLay;
	imageMap["menu_yesno_box"] = this->imgMenuYesNoBOX;
	imageMap["menu_choose_diff_box"] = this->imgMenuChooseDIFFBOX;
	imageMap["menu_language_box"] = this->imgMenuLanguageBOX;
	imageMap["menu_option_box3"] = this->imgMenuOptionBOX3;
	imageMap["menu_option_box4"] = this->imgMenuOptionBOX4;
	imageMap["hud_numbers"] = this->imgHudNumbers;
	imageMap["game_menu_panel_bottom"] = this->imgGameMenuPanelbottom;
	imageMap["game_menu_panel_bottom_sentry"] = this->imgGameMenuPanelBottomSentrybot;
	imageMap["game_menu_health"] = this->imgGameMenuHealth;
	imageMap["game_menu_shield"] = this->imgGameMenuShield;
	imageMap["game_menu_torn_page"] = this->imgGameMenuTornPage;
	imageMap["game_menu_background"] = this->imgGameMenuBackground;
	imageMap["main_menu_dial_a_anim"] = this->imgMainMenuDialA_Anim;
	imageMap["main_menu_dial_c_anim"] = this->imgMainMenuDialC_Anim;
	imageMap["main_menu_slide_anim"] = this->imgMainMenuSlide_Anim;
	imageMap["game_menu_scrollbar"] = this->imgGameMenuScrollBar;
	imageMap["game_menu_top_slider"] = this->imgGameMenuTopSlider;
	imageMap["game_menu_mid_slider"] = this->imgGameMenuMidSlider;
	imageMap["game_menu_bottom_slider"] = this->imgGameMenuBottomSlider;
	imageMap["vending_scrollbar"] = this->imgVendingScrollBar;
	imageMap["vending_scroll_btn_top"] = this->imgVendingScrollButtonTop;
	imageMap["vending_scroll_btn_mid"] = this->imgVendingScrollButtonMiddle;
	imageMap["vending_scroll_btn_bottom"] = this->imgVendingScrollButtonBottom;
	imageMap["logo"] = this->imgLogo;

	// Build UI sound alias -> ResID lookup from sounds.yaml ui_sounds section
	std::unordered_map<std::string, int> soundMap;
	{
		DataNode sndConfig = DataNode::loadFile("sounds.yaml");
		DataNode uiSounds = sndConfig["ui_sounds"];
		if (uiSounds) {
			for (auto it = uiSounds.begin(); it != uiSounds.end(); ++it) {
				std::string alias = it.key().asString();
				std::string soundName = it.value().asString("");
				int resId = Sounds::getResIDByName(soundName);
				if (resId >= 0) {
					soundMap[alias] = resId;
				}
			}
		}
		if (!sndConfig) {
			LOG_WARN("[menu] warning: could not load ui_sounds from sounds.yaml\n");
		}
	}

	// Button creation helpers
	auto resolveSound = [&](const DataNode& btn) -> int {
		std::string sndName = btn["sound"].asString("");
		auto sit = soundMap.find(sndName);
		return (sit != soundMap.end()) ? sit->second : 0;
	};

	auto resolveImage = [&](const std::string& imgName) -> Image* {
		auto iit = imageMap.find(imgName);
		return (iit != imageMap.end()) ? iit->second : nullptr;
	};

	auto createButton = [&](const DataNode& btn, int id, int x, int y, int w, int h) -> fmButton* {
		int soundId = resolveSound(btn);
		fmButton* button = new fmButton(id, x, y, w, h, soundId);

		bool visible = btn["visible"].asBool(true);
		button->drawButton = visible;

		std::string imgName = btn["image"].asString("");
		if (!imgName.empty()) {
			Image* img = resolveImage(imgName);
			if (img) button->SetImage(img, false);
		}

		std::string imgHighName = btn["image_highlight"].asString("");
		if (!imgHighName.empty()) {
			Image* imgH = resolveImage(imgHighName);
			if (imgH) button->SetHighlightImage(imgH, false);
		}

		button->normalRenderMode = btn["render_mode"].asInt(0);
		button->highlightRenderMode = btn["highlight_render_mode"].asInt(0);

		return button;
	};

	auto parseButtonNode = [&](const DataNode& btn, fmButtonContainer* container) {
		DataNode range = btn["id_range"];
		if (range) {
			int startId = range[0].asInt(0);
			int endId = range[1].asInt(0);
			int x = btn["x"].asInt(0);
			int startY = btn["start_y"].asInt(0);
			int stepY = btn["step_y"].asInt(0);
			int w = btn["width"].asInt(0);
			int h = btn["height"].asInt(0);

			int posY = startY;
			for (int id = startId; id <= endId; id++) {
				fmButton* button = createButton(btn, id, x, posY, w, h);
				container->AddButton(button);
				posY += stepY;
			}
		} else {
			int id = btn["id"].asInt(0);
			int x = btn["x"].asInt(0);
			int w = btn["width"].asInt(0);
			int h = btn["height"].asInt(0);
			int y = 0;

			bool sizeFromImage = btn["size_from_image"].asBool(false);
			std::string imgName = btn["image"].asString("");
			Image* img = resolveImage(imgName);

			if (sizeFromImage && img) {
				w = img->width;
				h = img->height;
			}

			std::string yStr = btn["y"].asString("");
			if (yStr == "image_relative") {
				int yBase = btn["y_base"].asInt(0);
				std::string yOffset = btn["y_offset"].asString("");
				if (yOffset == "-height" && img) {
					y = yBase - img->height;
				} else {
					std::string refImg = yOffset.substr(0, yOffset.find('.'));
					Image* refImage = resolveImage(refImg);
					y = refImage ? yBase + refImage->height : yBase;
				}
			} else {
				y = btn["y"].asInt(0);
			}

			fmButton* button = createButton(btn, id, x, y, w, h);
			container->AddButton(button);
		}
	};

	// Load screens, routing buttons to the right container
	// Also build button name -> ID lookup for resolving visible_buttons names
	std::unordered_map<std::string, int> buttonNameToId;
	DataNode screens = config["screens"];
	if (screens) {
		if (this->m_menuButtons) delete this->m_menuButtons;
		if (this->m_infoButtons) delete this->m_infoButtons;
		if (this->m_vendingButtons) delete this->m_vendingButtons;
		this->m_menuButtons = new fmButtonContainer();
		this->m_infoButtons = new fmButtonContainer();
		this->m_vendingButtons = new fmButtonContainer();

		for (auto sit = screens.begin(); sit != screens.end(); ++sit) {
			DataNode screen = sit.value();
			std::string containerName = sit.key().asString();
			fmButtonContainer* target = nullptr;
			if (containerName == "menu") target = this->m_menuButtons;
			else if (containerName == "info") target = this->m_infoButtons;
			else if (containerName == "vending") target = this->m_vendingButtons;
			if (!target) continue;

			for (auto bit = screen.begin(); bit != screen.end(); ++bit) {
				std::string btnName = bit.key().asString();
				DataNode btn = bit.value();

				// Register button name -> ID mapping
				if (btn["id"]) {
					buttonNameToId[btnName] = btn["id"].asInt();
				}

				parseButtonNode(btn, target);
			}
		}
	}

	// scrollbar (top-level)
	DataNode scrollbar = config["scrollbar"];
	if (scrollbar) {
		if (this->m_scrollBar) delete this->m_scrollBar;
		std::string sndName = scrollbar["sound"].asString("");
		int soundId = 0;
		auto sit = soundMap.find(sndName);
		if (sit != soundMap.end()) soundId = sit->second;
		bool vertical = scrollbar["vertical"].asBool(true);
		this->m_scrollBar = new fmScrollButton(0, 0, 0, 0, vertical, soundId);
	}

	// Resolve inline theme from each menu's sourceNode (stored during loadMenusFromYAML)
	int resolvedCount = 0;
	for (auto& def : this->yamlMenuDefs) {
		DataNode btnNode = def.sourceNode["button"];
		if (!btnNode) continue;

		MenuTheme theme;

		if (btnNode.isMap()) {
			theme.btnImage = resolveImage(btnNode["image"].asString(""));
			theme.btnHighlightImage = resolveImage(btnNode["image_highlight"].asString(""));
			theme.btnRenderMode = btnNode["render_mode"].asInt(0);
			theme.btnHighlightRenderMode = btnNode["highlight_render_mode"].asInt(0);
		}

		DataNode ibNode = def.sourceNode["info_button"];
		if (ibNode && ibNode.isMap()) {
			theme.infoBtnImage = resolveImage(ibNode["image"].asString(""));
			theme.infoBtnHighlightImage = resolveImage(ibNode["image_highlight"].asString(""));
			theme.infoBtnRenderMode = ibNode["render_mode"].asInt(0);
			theme.infoBtnHighlightRenderMode = ibNode["highlight_render_mode"].asInt(0);
		}

		theme.itemHeight = def.sourceNode["item_height"].asInt(46);
		theme.itemPaddingBottom = def.sourceNode["item_padding_bottom"].asInt(0);

		DataNode sb = def.sourceNode["scrollbar"];
		if (sb) {
			std::string style = sb["style"].asString("");
			if (style == "dial") {
				theme.scrollbar.style = SB_DIAL;
				theme.scrollbar.defaultX = sb["default_x"].asInt(408);
				theme.scrollbar.defaultY = sb["default_y"].asInt(81);
			} else if (style == "bar") {
				theme.scrollbar.style = SB_BAR;
				theme.scrollbar.x = sb["x"].asInt(0);
				theme.scrollbar.width = sb["width"].asInt(50);
				DataNode imgs = sb["images"];
				if (imgs && imgs.size() >= 4) {
					theme.scrollbar.barImg = resolveImage(imgs[0].asString(""));
					theme.scrollbar.topImg = resolveImage(imgs[1].asString(""));
					theme.scrollbar.midImg = resolveImage(imgs[2].asString(""));
					theme.scrollbar.bottomImg = resolveImage(imgs[3].asString(""));
				}
			}
		}

		def.resolvedTheme = theme;
		def.hasTheme = true;
		resolvedCount++;
	}

	// Resolve button names to IDs for all menu defs
	for (auto& def : this->yamlMenuDefs) {
		for (const auto& name : def.visibleButtonNames) {
			auto bit = buttonNameToId.find(name);
			if (bit != buttonNameToId.end()) {
				def.visibleButtons.push_back(bit->second);
			} else {
				LOG_WARN("[menu] warning: unknown button name '%s' in menu '%s'\n", name.c_str(), def.name.c_str());
			}
		}
		for (const auto& name : def.visibleButtonsConditionalNames) {
			auto bit = buttonNameToId.find(name);
			if (bit != buttonNameToId.end()) {
				def.visibleButtonsConditional.push_back(bit->second);
			} else {
				LOG_WARN("[menu] warning: unknown conditional button name '%s' in menu '%s'\n", name.c_str(), def.name.c_str());
			}
		}
	}

	int containerCount = (this->m_menuButtons ? 1 : 0) + (this->m_infoButtons ? 1 : 0) + (this->m_vendingButtons ? 1 : 0);
	LOG_INFO("[menu] loaded %d screens, resolved %d menu themes, %d button names from %s\n",
		containerCount, resolvedCount, (int)buttonNameToId.size(), path);
	return true;
}


bool MenuSystem::loadYAMLMenuItems(int menuId) {
	auto it = this->yamlMenuById.find(menuId);
	if (it == this->yamlMenuById.end()) return false;

	const YAMLMenuDef& def = this->yamlMenuDefs[it->second];
	// Check if there are any non-image items (image items are visual-only)
	bool hasRegularItems = false;
	for (const auto& item : def.items) {
		if (!(item.flags & Menus::ITEM_IMAGE)) { hasRegularItems = true; break; }
	}
	if (!hasRegularItems) {
		if (def.maxItems > 0) this->maxItems = def.maxItems;
		return false;
	}

	Applet* app = this->app;
	Text* textbuff = app->localization->getSmallBuffer();

	if (def.maxItems > 0) {
		this->maxItems = def.maxItems;
	}

	int argIndex = 0;
	for (const auto& item : def.items) {
		if (item.flags & Menus::ITEM_IMAGE) continue;
		int textField = MenuSystem::EMPTY_TEXT;
		int textField2 = MenuSystem::EMPTY_TEXT;
		int flags = item.flags;
		int action = item.action;
		int param = item.param;
		int helpField = item.helpField;

		if (item.textField != 0) {
			textField = item.textField;
		}
		if (item.textField2 != 0) {
			textField2 = item.textField2;
		}

		// Handle inline text via addTextArg
		if (!item.text.empty()) {
			textField = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1 + argIndex);
			textbuff->setLength(0);
			textbuff->append(item.text.c_str());
			app->localization->addTextArg(textbuff);
			argIndex++;
		}
		if (!item.text2.empty()) {
			textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1 + argIndex);
			textbuff->setLength(0);
			textbuff->append(item.text2.c_str());
			app->localization->addTextArg(textbuff);
			argIndex++;
		}

		this->items[this->numItems++].Set(textField, textField2, flags, action, param, helpField);
	}

	textbuff->dispose();
	return true;
}

static MenuSystem::LayoutValueMode parseLayoutMode(const std::string& s) {
	if (s == "center") return MenuSystem::LVM_CENTER;
	if (s == "btn_width") return MenuSystem::LVM_BTN_WIDTH;
	if (s == "btn_h_x4") return MenuSystem::LVM_BTN_H_X4;
	if (s == "btn_h_x5") return MenuSystem::LVM_BTN_H_X5;
	if (s == "panel_h") return MenuSystem::LVM_PANEL_H;
	if (s == "yesno_h") return MenuSystem::LVM_YESNO_H;
	if (s == "lang_h") return MenuSystem::LVM_LANG_H;
	if (s == "logo_box") return MenuSystem::LVM_LOGO_BOX;
	if (s == "logo_yesno") return MenuSystem::LVM_LOGO_YESNO;
	if (s == "logo_diff") return MenuSystem::LVM_LOGO_DIFF;
	if (s == "logo_lang") return MenuSystem::LVM_LOGO_LANG;
	if (s == "logo_15") return MenuSystem::LVM_LOGO_15;
	return MenuSystem::LVM_LITERAL;
}

static void parseLayoutValue(const DataNode& node, MenuSystem::LayoutValueMode& mode, int& val) {
	if (!node) return;
	if (node.isScalar()) {
		std::string s = node.asString("");
		// Try as integer first
		try {
			val = std::stoi(s);
			mode = MenuSystem::LVM_LITERAL;
			return;
		} catch (...) {}
		// Parse as named mode
		mode = parseLayoutMode(s);
	}
}

static MenuSystem::MenuLayout parseLayout(const DataNode& node) {
	MenuSystem::MenuLayout layout;
	if (!node || !node.isMap()) return layout;
	layout.isSet = true;
	parseLayoutValue(node["x"], layout.xMode, layout.xVal);
	parseLayoutValue(node["y"], layout.yMode, layout.yVal);
	parseLayoutValue(node["w"], layout.wMode, layout.wVal);
	parseLayoutValue(node["h"], layout.hMode, layout.hVal);
	return layout;
}

bool MenuSystem::loadMenusFromYAML(const char* path) {
	DataNode config = DataNode::loadFile(path);
	if (!config) {
		LOG_ERROR("[menu] failed to load %s\n", path);
		return false;
	}

	DataNode menus = config["menus"];
	if (!menus || !menus.isMap() || menus.size() == 0) {
		LOG_ERROR("[menu] menus.yaml has missing or empty 'menus' map\n");
		return false;
	}

	int totalCount = (int)menus.size();

	// Pre-count non-inject entries for binary menuData allocation
	int binaryCount = 0;
	for (auto it = menus.begin(); it != menus.end(); ++it) {
		if (!it.value()["inject"].asBool(false)) binaryCount++;
	}

	this->menuDataCount = (uint32_t)binaryCount;
	this->menuData = new uint32_t[binaryCount > 0 ? binaryCount : 1];

	std::vector<uint32_t> itemWords;
	int itemOffset = 0;
	int binaryIndex = 0;
	int yamlMenuCount = 0;
	int injectCount = 0;

	for (auto it = menus.begin(); it != menus.end(); ++it) {
		DataNode menu = it.value();
		std::string menuName = it.key().asString();

		bool isInjected = menu["inject"].asBool(false);
		int menuId = menu["menu_id"].asInt(0);
		int menuType = menuTypeFromString(menu["type"].asString("default").c_str());
		int menuMaxItems = menu["max_items"].asInt(0);

		DataNode items = menu["items"];
		int totalItemCount = (items && items.isSequence()) ? (int)items.size() : 0;

		// Count regular (non-image) items for binary packing
		int itemCount = 0;
		for (int j = 0; j < totalItemCount; j++) {
			std::string itype = items[j]["type"].asString("");
			if (itype != "background" && itype != "image") itemCount++;
		}

		if (!isInjected) {
			// Check if this menu uses extended features (text, text2, image items, or GEC extended flags)
			bool hasExtendedFeatures = false;
			for (int j = 0; j < totalItemCount && !hasExtendedFeatures; j++) {
				DataNode item = items[j];
				std::string itype = item["type"].asString("");
				if (itype == "background" || itype == "image") {
					hasExtendedFeatures = true;
				}
				if (item["text"] || item["text2"]) {
					hasExtendedFeatures = true;
				}
				int flags = flagsFromString(item["flags"].asString("normal").c_str());
				if (flags > 0xFFFF) {
					hasExtendedFeatures = true;
				}
			}

			// Store in binary-packed format (for loadMenuItems compatibility)
			uint8_t storedId = (uint8_t)(menuId & 0xFF);
			int endOffset = itemOffset + itemCount * 2;
			this->menuData[binaryIndex] = (uint32_t)storedId
				| (uint32_t)((endOffset & 0xFFFF) << 8)
				| (uint32_t)((menuType & 0xFF) << 24);
			binaryIndex++;

			// Parse items for extended menus (text, text2, image items, or GEC extended flags)
			// (stored in YAMLMenuDef below, but hasExtendedFeatures is only for binary menus)
			menuMaxItems = menuMaxItems; // keep for def

			// Always pack regular items into binary format (skip image items)
			for (int j = 0; j < totalItemCount; j++) {
				DataNode item = items[j];
				std::string itype = item["type"].asString("");
				if (itype == "background" || itype == "image") continue;
				int stringId = item["string_id"].asInt(0);
				int flags = flagsFromString(item["flags"].asString("normal").c_str());
				int action = actionFromString(item["action"].asString("none").c_str());
				int param = item["param"].asInt(0);
				DataNode gotoNode = item["goto"];
				if (gotoNode) {
					param = menuNameToId(gotoNode.asString(""));
				}
				int helpString = item["help_string"].asInt(0);
				itemWords.push_back((uint32_t)((stringId & 0xFFFF) << 16) | (uint32_t)(flags & 0xFFFF));
				itemWords.push_back((uint32_t)((helpString & 0xFFFF) << 16)
					| (uint32_t)((action & 0xFF) << 8)
					| (uint32_t)(param & 0xFF));
				itemOffset += 2;
			}

			// Check extended features for YAML items
			if (hasExtendedFeatures) yamlMenuCount++;

			// Store YAML-native menu definition
			YAMLMenuDef def;
			def.name = menuName;
			def.menuId = menuId;
			def.type = menuType;
			def.maxItems = menuMaxItems;
			def.sourceNode = menu;

			// Parse presentation properties (inline on the menu entry)
			def.helpResource = menu["help_resource"].asInt(-1);
			def.selectedIndex = menu["selected_index"].asInt(-1);
			def.showInfoButtons = menu["show_info_buttons"].asBool(false);
			def.itemWidth = menu["item_width"].asInt(0);
			def.vibrationY = menu["vibration_y"].asInt(-1);
			def.loadStartItem = menu["load_start_item"].asInt(0);
			def.scrollIndex = menu["scroll_index"].asInt(-1);

			DataNode sbOff = menu["scrollbar_offset"];
			if (sbOff) {
				def.scrollbarXOffset = sbOff["x"].asInt(-1);
				def.scrollbarYOffset = sbOff["y"].asInt(-1);
			}

			DataNode yn = menu["yesno"];
			if (yn) {
				def.yesnoStringId = yn["string_id"].asInt(-1);
				def.yesnoSelectYes = yn["select_yes"].asBool(false) ? 1 : 0;
				def.yesnoYesAction = actionFromString(yn["yes_action"].asString("0"));
				def.yesnoYesParam = yn["yes_param"].asInt(0);
				def.yesnoNoAction = actionFromString(yn["no_action"].asString("back"));
				def.yesnoNoParam = yn["no_param"].asInt(0);
				def.yesnoClearStack = yn["clear_stack"].asBool(false);
			}

			DataNode vb = menu["visible_buttons"];
			if (vb) {
				for (int v = 0; v < vb.size(); v++)
					def.visibleButtonNames.push_back(vb[v].asString(""));
			}
			DataNode vbc = menu["visible_buttons_conditional"];
			if (vbc) {
				for (auto it = vbc.begin(); it != vbc.end(); ++it)
					def.visibleButtonsConditionalNames.push_back(it.key().asString());
			}

			DataNode layoutNode = menu["layout"];
			if (layoutNode && layoutNode.isMap()) {
				def.layout = parseLayout(layoutNode);
			}

			// Parse items for extended menus
			if (hasExtendedFeatures) {
				for (int j = 0; j < totalItemCount; j++) {
					DataNode item = items[j];
					YAMLMenuItem mi = {};
					std::string itype = item["type"].asString("");
					if (itype == "background" || itype == "image") {
						mi.itemType = itype;
						mi.imageName = item["image"].asString("");
						std::string xStr = item["x"].asString("");
						mi.imageX = (xStr == "center") ? -1 : item["x"].asInt(0);
						mi.imageY = item["y"].asInt(0);
						std::string anchorStr = item["anchor"].asString("");
						mi.imageAnchor = (anchorStr == "center") ? 3 : item["anchor"].asInt(0);
						mi.imageRenderMode = item["render_mode"].asInt(0);
						mi.flags = Menus::ITEM_NOSELECT | Menus::ITEM_IMAGE;
						def.items.push_back(mi);
						continue;
					}
					mi.textField = item["string_id"].asInt(0);
					mi.flags = flagsFromString(item["flags"].asString("normal").c_str());
					mi.action = actionFromString(item["action"].asString("none").c_str());
					mi.param = item["param"].asInt(0);
					DataNode gotoNode = item["goto"];
					if (gotoNode) {
						mi.param = menuNameToId(gotoNode.asString(""));
					}
					mi.helpField = item["help_string"].asInt(0);
					mi.text = item["text"].asString("");
					mi.text2 = item["text2"].asString("");
					def.items.push_back(mi);
				}
			}

			this->yamlMenuById[menuId] = (int)this->yamlMenuDefs.size();
			this->yamlMenuDefs.push_back(std::move(def));
		} else {
			// Inject menu — no binary data, just YAMLMenuDef
			if (this->yamlMenuById.find(menuId) != this->yamlMenuById.end()) continue;

			YAMLMenuDef def;
			def.name = menuName;
			def.menuId = menuId;
			def.type = menuType;
			def.isMissing = true;
			def.sourceNode = menu;

			def.helpResource = menu["help_resource"].asInt(-1);
			def.selectedIndex = menu["selected_index"].asInt(-1);
			def.showInfoButtons = menu["show_info_buttons"].asBool(false);
			def.itemWidth = menu["item_width"].asInt(0);
			def.vibrationY = menu["vibration_y"].asInt(-1);
			def.loadStartItem = menu["load_start_item"].asInt(0);
			def.scrollIndex = menu["scroll_index"].asInt(-1);

			DataNode yn = menu["yesno"];
			if (yn) {
				def.yesnoStringId = yn["string_id"].asInt(-1);
				def.yesnoSelectYes = yn["select_yes"].asBool(false) ? 1 : 0;
				def.yesnoYesAction = actionFromString(yn["yes_action"].asString("0"));
				def.yesnoYesParam = yn["yes_param"].asInt(0);
				def.yesnoNoAction = actionFromString(yn["no_action"].asString("back"));
				def.yesnoNoParam = yn["no_param"].asInt(0);
				def.yesnoClearStack = yn["clear_stack"].asBool(false);
			}

			DataNode vb = menu["visible_buttons"];
			if (vb) {
				for (int v = 0; v < vb.size(); v++)
					def.visibleButtonNames.push_back(vb[v].asString(""));
			}
			DataNode vbc = menu["visible_buttons_conditional"];
			if (vbc) {
				for (auto it = vbc.begin(); it != vbc.end(); ++it)
					def.visibleButtonsConditionalNames.push_back(it.key().asString());
			}

			DataNode layoutNode = menu["layout"];
			if (layoutNode && layoutNode.isMap()) {
				def.layout = parseLayout(layoutNode);
			}

			// Parse image items for inject menus
			DataNode injectItems = menu["items"];
			if (injectItems && injectItems.isSequence()) {
				for (int j = 0; j < (int)injectItems.size(); j++) {
					DataNode item = injectItems[j];
					std::string itype = item["type"].asString("");
					if (itype == "background" || itype == "image") {
						YAMLMenuItem mi = {};
						mi.itemType = itype;
						mi.imageName = item["image"].asString("");
						std::string xStr = item["x"].asString("");
						mi.imageX = (xStr == "center") ? -1 : item["x"].asInt(0);
						mi.imageY = item["y"].asInt(0);
						std::string anchorStr = item["anchor"].asString("");
						mi.imageAnchor = (anchorStr == "center") ? 3 : item["anchor"].asInt(0);
						mi.imageRenderMode = item["render_mode"].asInt(0);
						mi.flags = Menus::ITEM_NOSELECT | Menus::ITEM_IMAGE;
						def.items.push_back(mi);
					}
				}
			}

			this->yamlMenuById[menuId] = (int)this->yamlMenuDefs.size();
			this->yamlMenuDefs.push_back(std::move(def));
			injectCount++;
		}
	}

	this->menuItemsCount = (uint32_t)itemWords.size();
	this->menuItems = new uint32_t[itemWords.size() > 0 ? itemWords.size() : 1];
	std::memcpy(this->menuItems, itemWords.data(), itemWords.size() * sizeof(uint32_t));

	LOG_INFO("[menu] loaded %d menus (%d extended) + %d injected, %d item words from %s\n",
		binaryCount, yamlMenuCount, injectCount, this->menuItemsCount, path);
	return true;
}

void MenuSystem::buildDivider(Text* text, int i) {
	int cnt = (i - (text->length() + 2)) / 2;
	if (text->length() > 0) {
		text->insert(' ', 0);
		text->append(' ');
	}
	else {
		text->append('\x80');
		text->append('\x80');
		text->append('\x80');
	}
	for (int j = 0; j < cnt; j++) {
		text->insert('\x80', 0);
		text->append('\x80');
	}
}

bool MenuSystem::enterDigit(int i) {


	this->cheatCombo *= 10;
	this->cheatCombo += i;
	if (this->cheatCombo == 3666) {
		this->cheatCombo = 0;
		this->selectedIndex = 0;
		this->scrollIndex = 0;
		this->gotoMenu(Menus::MENU_DEBUG);
		return true;
	}
	if (this->cheatCombo == 1666) {
		app->canvas->loadMap(app->canvas->loadMapID, true, false);
		return true;
	}
	if (this->cheatCombo == 4332) {
		app->player->giveAll();
	}
	else if (this->cheatCombo == 3366) {
		this->cheatCombo = 0;
		if (this->menu >= Menus::MENU_INGAME) {
			app->canvas->startSpeedTest(false);
		}
	}
	return false;
}

void MenuSystem::scrollDown() { // J2ME
	if ((this->items[this->selectedIndex].flags & 0x100) != 0x0) {
		this->shiftBlockText(this->selectedIndex, 1, this->maxItems);
	}
	else {
		this->soundClick(); // [GEC]
		this->moveDir(1);
	}
}

void MenuSystem::scrollUp() { // J2ME
	if ((this->items[this->selectedIndex].flags & 0x100) != 0x0) {
		this->shiftBlockText(this->selectedIndex, -1, this->maxItems);
	}
	else {
		this->soundClick(); // [GEC]
		this->moveDir(-1);
	}
}

bool MenuSystem::scrollPageDown() { // J2ME
	if ((this->items[this->selectedIndex].flags & 0x100) != 0x0) {
		return this->shiftBlockText(this->selectedIndex, this->maxItems, this->maxItems);
	}
	this->soundClick(); // [GEC]
	int selectedIndex = this->selectedIndex;
	for (int n = 0; n < this->maxItems && this->selectedIndex != this->numItems - 1; ++n) {
		this->moveDir(1);
		if (this->selectedIndex < selectedIndex) {
			this->moveDir(-1);
			break;
		}
	}
	return selectedIndex != this->selectedIndex;
}

void MenuSystem::scrollPageUp() { // J2ME
	if ((this->items[this->selectedIndex].flags & 0x100) != 0x0) {
		this->shiftBlockText(this->selectedIndex, -this->maxItems, this->maxItems);
	}
	else {
		this->soundClick(); // [GEC]
		int selectedIndex = this->selectedIndex;
		for (int n = 0; n < this->maxItems && this->selectedIndex != 0; ++n) {
			this->moveDir(-1);
			if (this->selectedIndex >= selectedIndex) {
				this->moveDir(1);
				break;
			}
		}
	}
}

bool MenuSystem::shiftBlockText(int n, int i, int j) { // J2ME

	if (n > this->numItems || (this->items[n].flags & 0x100) == 0x0) {
		return false;
	}
	Text* largeBuffer = app->localization->getLargeBuffer();
	app->localization->composeText(this->items[n].textField, largeBuffer);
	int n2 = this->items[n].param >> 26 & 0x3F;
	int b = this->items[n].param >> 20 & 0x3F;
	int n4;
	int n3 = n4 = (this->items[n].param & 0x3FF);
	if (i > 0) {
		for (i = std::min(i, n2 - (b + j)); i > 0; --i) {
			n3 = largeBuffer->findFirstOf('|', n3) + 1;
			++b;
		}
	}
	else {
		for (i = std::min(std::abs(i), b); i > 0; --i) {
			n3 = largeBuffer->findLastOf('|', n3 - 1) + 1;
			--b;
		}
	}
	int n5;
	for (n5 = n3; j > 0; --j, ++n5) {
		n5 = largeBuffer->findFirstOf('|', n5);
		if (n5 == -1) {
			n5 = largeBuffer->length();
			break;
		}
	}
	this->items[n].param = ((n2 & 0x3F) << 26 | (b & 0x3F) << 20 | (n5 - n3 & 0x3FF) << 10 | (n3 & 0x3FF));
	largeBuffer->dispose();
	return n3 != n4;
}

void MenuSystem::moveDir(int n) { // J2ME


	//this->menuMode = this->MODE_FULLREFRESH;
	if (this->type == 5 || this->type == 7) {
		if (n < 0 && this->scrollIndex > 0) {
			this->scrollIndex += n;
		}
		else if (n > 0 && this->scrollIndex < this->numItems - this->maxItems) {
			this->scrollIndex += n;
		}
		this->selectedIndex = this->scrollIndex;
	}
	else {
		do {
			this->selectedIndex += n;
			if (this->selectedIndex >= this->numItems || this->selectedIndex < 0) {
				if (this->selectedIndex < 0) {
					this->selectedIndex = this->numItems - 1;
				}
				else {
					this->selectedIndex = 0;
					if (this->type == 9) {
						this->scrollIndex = 0;
					}
				}
				while (this->items[this->selectedIndex].textField == this->EMPTY_TEXT || (this->items[this->selectedIndex].flags & 0x8001) != 0x0) {
					this->selectedIndex += n;
				}
				break;
			}
		} while (this->items[this->selectedIndex].textField == this->EMPTY_TEXT || (this->items[this->selectedIndex].flags & 0x8001) != 0x0);
		
		if (this->maxItems != 0 && n < 0) {
			if (this->selectedIndex - this->maxItems + 1 > this->scrollIndex) {
				this->scrollIndex = this->selectedIndex - this->maxItems + 1;
			}
			else if (this->selectedIndex < this->scrollIndex) {
				this->scrollIndex = this->selectedIndex;
			}
			else if (this->type == 9 && this->selectedIndex - 3 < this->scrollIndex && this->scrollIndex > 0) {
				--this->scrollIndex;
			}
		}
		else if (this->maxItems != 0) {
			if (this->selectedIndex > this->scrollIndex + this->maxItems - 1) {
				this->scrollIndex = this->selectedIndex - this->maxItems + 1;
			}
			else if (this->scrollIndex > this->selectedIndex) {
				this->scrollIndex = this->selectedIndex;
			}
			else if (this->type == 9 && this->selectedIndex + 3 > this->scrollIndex + this->maxItems - 1 && this->scrollIndex + this->maxItems < this->numItems) {
				++this->scrollIndex;
			}
		}
	}

	//printf("this->type %d\n", this->type);
	//printf("this->numItems %d\n", this->numItems);
	//printf("this->scrollIndex %d\n", this->scrollIndex);
	//printf("this->selectedIndex %d\n", this->selectedIndex);

	// [GEC] Actualiza la posicion del scroll touch
	if (this->m_scrollBar->field_0x0_ && this->menu != Menus::MENU_COMIC_BOOK) {
		int numItems = this->numItems;
		int maxScroll = (this->m_scrollBar->field_0x40_ - this->m_scrollBar->field_0x3c_);
		int maxScroll2 = ((this->m_scrollBar->barRect).h - this->m_scrollBar->field_0x4c_);
		int iVar4 = (maxScroll / numItems) * 2;
		int iVar2 = (maxScroll2 / numItems) * 2;

		int begItem = 0;
		int endItem = 0;
		int begY1 = 0;
		int begY2 = 0;

		for (int i = 0; i < numItems; i++) { // Ajusta la posici�n si es necesario
			if (!(this->items[i].flags & 0x8001)) {
				endItem = i;
			}
		}

		//printf("endItem %d\n", endItem);
		for (int i = 0; i < numItems; i++) { // Ajusta la posici�n si es necesario
			if (!(this->items[i].flags & 0x8001)) {
				begItem = i;
				break;
			}
			else {
				if (!(this->items[i].flags & 0x8000)) {
					begY1 += this->getMenuItemHeight2(i);
					begY2 += iVar2;

					//if ((this->items[i].flags & Menus::ITEM_SCROLLBAR) != 0) { // [GEC]
						//begY1 += 10 + Applet::FONT_HEIGHT[app->fontType];
					//}
				}
			}
		}

		this->m_scrollBar->field_0x44_ = 0;
		this->m_scrollBar->field_0x48_ = 0;

		if (this->type == 5 || this->type == 7) {
			this->m_scrollBar->field_0x44_ = this->selectedIndex * iVar4;
			this->m_scrollBar->field_0x48_ = this->selectedIndex * iVar2;
			this->m_scrollBar->field_0x44_ = std::min(this->m_scrollBar->field_0x44_, maxScroll);
			this->m_scrollBar->field_0x48_ = std::min(this->m_scrollBar->field_0x48_, maxScroll2);
		}
		else {
			int y1 = 0;
			int y2 = 0;

			if (this->selectedIndex == this->scrollIndex) {
				for (int i = 0; i < this->selectedIndex; i++) { // Ajusta la posici�n si es necesario
					if (!(this->items[i].flags & 0x8000)) {
						y1 += this->getMenuItemHeight2(i);
						y2 += iVar2;
					}
				}
			}

			for (int j = 0; j < this->scrollIndex; j++) {
				if (y1 == 0) { this->m_scrollBar->field_0x44_ += this->getMenuItemHeight2(this->selectedIndex) - y1; }
				else { this->m_scrollBar->field_0x44_ = y1; }
				if (y2 == 0) { this->m_scrollBar->field_0x48_ += iVar2; }
				else { this->m_scrollBar->field_0x48_ = y2; }
				this->m_scrollBar->field_0x44_ = std::min(this->m_scrollBar->field_0x44_, maxScroll);
				this->m_scrollBar->field_0x48_ = std::min(this->m_scrollBar->field_0x48_, maxScroll2);
			}

			if (this->selectedIndex == begItem) {  // Ajusta la posici�n si es necesario
				this->scrollIndex = 0;
				this->m_scrollBar->field_0x44_ -= begY1;
				this->m_scrollBar->field_0x48_ -= begY2;
				this->m_scrollBar->field_0x44_ = std::max(this->m_scrollBar->field_0x44_, 0);
				this->m_scrollBar->field_0x48_ = std::max(this->m_scrollBar->field_0x48_, 0);
			}

			if (this->old_0x48 != this->m_scrollBar->field_0x48_) {
				app->sound->playSound(Sounds::getResIDByName(SoundName::MENU_SCROLL), 0, 5, false); // [GEC]
			}

			this->old_0x44 = this->m_scrollBar->field_0x44_;
			this->old_0x48 = this->m_scrollBar->field_0x48_;
		}
	}
}


void MenuSystem::doDetailsSelect()
{
	if (this->type == 7 || this->type == 5) {
		return;
	}
	if (this->menu == Menus::MENU_VENDING_MACHINE_CANT_BUY) {
		return;
	}
	this->cheatCombo = 0;

	if (this->items[this->selectedIndex].flags & 0x20) {
		this->showDetailsMenu();
	}
	else if (this->items[this->selectedIndex].action != 0) {
		this->select(this->selectedIndex);
	}
}

void MenuSystem::back() {

	//printf("back:: this->menu %d\n", this->menu);

	if ((this->menu == Menus::MENU_MAIN_BINDINGS) || (this->menu == Menus::MENU_INGAME_BINDINGS)) {
		// Apply changes
		std::memcpy(keyMapping, keyMappingTemp, sizeof(keyMapping));
	}

	if (this->menu != Menus::MENU_MAIN_MORE_GAMES && this->menu != Menus::MENU_MAIN) {
		if (!app->sound->isSoundPlaying(1122)) {
			app->sound->playSound(Sounds::getResIDByName(SoundName::SWITCH_EXIT), 0, 5, false); // [GEC]
		}
	}

	if (this->menu == Menus::MENU_MAIN_OPTIONS || this->menu == Menus::MENU_INGAME_OPTIONS) {
		this->leaveOptionsMenu();
	}

	if ((this->menu == Menus::MENU_MAIN_OPTIONS_VIDEO) || (this->menu == Menus::MENU_INGAME_OPTIONS_VIDEO)) { // [GEC]
		CAppContainer::getInstance()->sdlGL->restore();
	}

	if (this->stackCount != 0) {
		if (this->stackCount - 1 < 0){
			this->app->Error("Menu stack is empty");
		}
		int y1;
		int y2;
		int index;
		this->setMenu(this->popMenu(this->poppedIdx, &y1, &y2, &index));
		this->selectedIndex = this->poppedIdx[0];
		this->m_scrollBar->field_0x44_ = y1; // [GEC]
		this->m_scrollBar->field_0x48_ = y2; // [GEC]
		this->scrollIndex = index; // [GEC]
	}
	else if (this->menu == Menus::MENU_INGAME || this->menu == Menus::MENU_ITEMS || this->menu == Menus::MENU_ITEMS_DRINKS ||
		this->menu == Menus::MENU_INGAME_QUESTLOG || this->menu == Menus::MENU_INGAME_SNIPER) {
		this->returnToGame();
	}
	else if (this->menu == Menus::MENU_MAIN_MINIGAME || this->menu == Menus::MENU_COMIC_BOOK) {
		this->setMenu(Menus::MENU_MAIN);
	}
}

void MenuSystem::setMenu(int menu) {


	LOG_INFO("[menu] menu %d\n", menu);
	this->cheatCombo = 0;
	this->menuMode = 0;
	if ((menu == Menus::MENU_MAIN_BEGIN || menu == Menus::MENU_INGAME) || (menu == Menus::MENU_INGAME_KICKING)) {
		this->clearStack();
	}
	if (menu == Menus::MENU_ENABLE_SOUNDS) {
		this->menuMode = 0;
	}

	// Restaura
	if (this->menu == Menus::MENU_DEBUG || this->menu == Menus::MENU_MAIN_BINDINGS || this->menu == Menus::MENU_INGAME_BINDINGS || this->menu == Menus::MENU_INGAME_OPTIONS_VIDEO) { // [GEC]
		this->old_0x44 = this->m_scrollBar->field_0x44_;
		this->old_0x48 = this->m_scrollBar->field_0x48_;
	}
	else {
		this->old_0x44 = 0; // [GEC]
		this->old_0x48 = 0; // [GEC]
	}
	/*this->old_0x44 = this->btnScroll->field_0x44_;
	this->old_0x48 = this->btnScroll->field_0x48_;
	if (this->oldMenu != menu) {
		this->old_0x44 = 0; // [GEC]
		this->old_0x48 = 0; // [GEC]
	}*/

	this->oldMenu = this->menu;
	this->menu = menu;

	//if (this->menu == Menus::MENU_DEBUG) { // [GEC]
		//this->oldMenu = menu;
	//}

	if (this->menu != Menus::MENU_NONE) {
		this->initMenu(this->menu);
		if (app->canvas->state != Canvas::ST_MENU) {
			app->canvas->setState(Canvas::ST_MENU);
		}
	}
	else {
		this->returnToGame();
		this->menu = Menus::MENU_NONE;
	}

	if (this->oldMenu == Menus::MENU_DEBUG || this->oldMenu == Menus::MENU_MAIN_BINDINGS || this->oldMenu == Menus::MENU_INGAME_BINDINGS || this->oldMenu == Menus::MENU_INGAME_OPTIONS_VIDEO) { // [GEC]
		this->m_scrollBar->field_0x44_ = this->old_0x44;
		this->m_scrollBar->field_0x48_ = this->old_0x48;
	}
}


void MenuSystem::paint(Graphics* graphics) {

	Canvas* canvas; // r1
	int* screenRect; // r5
	int ScrollPos; // r0
	int yPos; // r10
	bool v17; // zf
	bool v22; // zf
	int v29; // r3
	bool v30; // zf
	bool v31; // zf
	MenuItem* items; // r5
	int v33; // r6
	int numItems; // r2
	int v35; // r10
	int* v36; // r12
	int v37; // r0
	int menuItem_width; // r3
	int flags; // r3
	int v40; // r0
	int action; // r1
	int v42; // r12
	int v43; // r2
	int textField2; // r3
	int v45; // r4
	int v46; // r3
	bool v47; // zf
	bool v49; // zf
	int v50; // r4
	int v55; // r6
	int v56; // r0
	Image* imgGameMenuTornPage; // r10
	VendingMachine* vendingMachine; // r1
	float v59; // r12
	int v60; // r3
	float v61; // r6
	Text* v62; // r0
	Canvas* v63; // r5
	Text* v64; // r4
	int* menuRect; // [sp+3Ch] [bp-44h]
	Text* textBuffer1; // [sp+40h] [bp-40h]
	Text* textBuffer2; // [sp+44h] [bp-3Ch]
	int v76; // [sp+48h] [bp-38h]
	int v78; // [sp+50h] [bp-30h]
	int v79; // [sp+54h] [bp-2Ch]
	int v80; // [sp+58h] [bp-28h]
	int v81; // [sp+5Ch] [bp-24h]
	int v82; // [sp+60h] [bp-20h]
	int menuHelpMaxChars; // [sp+64h] [bp-1Ch]

	if (this->menu == Menus::MENU_COMIC_BOOK) {
		app->comicBook->Draw(graphics);
		return;
	}
	canvas = app->canvas.get();
	menuRect = canvas->menuRect;
	screenRect = canvas->screenRect;
	textBuffer1 = app->localization->getLargeBuffer();
	textBuffer2 = app->localization->getLargeBuffer();
	graphics->clipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
	ScrollPos = this->getScrollPos();
	v76 = ScrollPos;

	if (this->menu >= Menus::MENU_INGAME) {
		graphics->drawImage(app->menuSystem->imgGameMenuPanelbottom, 0, Applet::IOS_HEIGHT - this->imgGameMenuPanelbottom->height, 0, 0, 0);
		if (this->menu >= Menus::MENU_VENDING_MACHINE && this->menu <= Menus::MENU_VENDING_MACHINE_LAST) {
			graphics->drawImage(app->vendingMachine->imgVendingBG, 0, 0, 0, 0, 0);
		}
		else {
			graphics->drawImage(this->imgGameMenuBackground, 0, 0, 0, 0, 0);
			yPos = 20 + (Applet::IOS_HEIGHT - this->imgGameMenuPanelbottom->height);
			// Health
			textBuffer1->setLength(0);
			textBuffer1->append(app->player->ce->getStat(Enums::STAT_HEALTH))->append("/")->append(app->player->ce->getStat(Enums::STAT_MAX_HEALTH));
			while (textBuffer1->length() <= 6) {
				textBuffer1->append(' ');
			}
			textBuffer1->append("  ");

			// Shield
			textBuffer2->setLength(0);
			textBuffer2->append(app->player->ce->getStat(Enums::STAT_ARMOR))->append("/")->append(200);
			while (textBuffer2->length() <= 6) {
				textBuffer2->insert(' ', 0);
			}
			textBuffer1->append(textBuffer2);
			graphics->drawString(textBuffer1, app->canvas->SCR_CX, yPos + 3, 3);
			graphics->drawImage(this->imgGameMenuHealth, ((Applet::IOS_WIDTH - textBuffer1->getStringWidth()) >> 1) - 5, yPos + 3, 10, 0, 0);
			graphics->drawImage(this->imgGameMenuShield, ((textBuffer1->getStringWidth() + Applet::IOS_WIDTH) >> 1) + 5, yPos + 3, 6, 0, 0);
		}
	}
	else if ((this->menu == Menus::MENU_END_RANKING) || (this->menu == Menus::MENU_LEVEL_STATS)) {
		graphics->drawImage(app->canvas->imgEndOfLevelStatsBG, 0, 0, 0, 0, 0);
	}
	else {
		// Render image items (type: background / type: image) from YAML menu def
		const YAMLMenuDef* imgDef = getMenuDef(this->menu);
		if (imgDef) {
			for (const auto& mi : imgDef->items) {
				if (!(mi.flags & Menus::ITEM_IMAGE)) continue;
				Image* img = resolveMenuImage(mi.imageName);
				if (!img) continue;
				int x = (mi.imageX == -1) ? ((Applet::IOS_WIDTH - img->width) >> 1) : mi.imageX;
				int y = mi.imageY;
				graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
				graphics->drawImage(img, x, y, mi.imageAnchor, 0, mi.imageRenderMode);
			}
		}
	}

	this->drawSoftkeyButtons(graphics);

	graphics->setClipRect(screenRect[0], menuRect[1], screenRect[2], menuRect[3]);

	if (this->menu == Menus::MENU_INGAME_CONTROLS) {
		app->hud->drawArrowControls(graphics);
	}

	if (this->menu == Menus::MENU_MAIN_CONTROLS) {
		graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
		app->hud->drawArrowControls(graphics);
	}

	graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);

	if (this->menu == Menus::MENU_MAIN_OPTIONS || 
		this->menu == Menus::MENU_INGAME_OPTIONS ||
		this->menu == Menus::MENU_INGAME_CONTROLS) {
		//this->drawOptionsScreen(graphics); // Old
	}
	else if (this->menu < Menus::MENU_INGAME || this->menu != Menus::MENU_MAIN_MORE_GAMES) {
		this->drawButtonFrame(graphics); // Usado en Wolfenstein RPG
	}

	// Clip rect: menuRect is authoritative (set by setMenuSettings from YAML layout)
	// Ingame/vending menus use tight clip; main menus use loose clip for visual overflow
	if (this->menu >= Menus::MENU_INGAME) {
		graphics->setClipRect(screenRect[0], menuRect[1], screenRect[2], menuRect[3]);
	} else {
		graphics->setClipRect(screenRect[0], menuRect[1] - 15, screenRect[2], menuRect[3] + 30);
	}
	this->drawTouchButtons(graphics, false);

	if (this->menu != Menus::MENU_MAIN_OPTIONS) {
		goto LABEL_66;
	}
	if (this->HasVibration()) {
		this->drawTouchButtons(graphics, true);
	}
	if (this->menu != Menus::MENU_MAIN_OPTIONS || !HasVibration()) {
	LABEL_66:
		this->drawTouchButtons(graphics, true);
	}

	if ((this->menu != Menus::MENU_MAIN_CONFIRMNEW) &&
		(this->menu != Menus::MENU_MAIN_OPTIONS) &&
		(this->menu != Menus::MENU_MAIN_DIFFICULTY) &&
		(this->menu != Menus::MENU_MAIN_MINIGAME) &&
		(this->menu != Menus::MENU_MAIN_MORE_GAMES) &&
		(this->menu != Menus::MENU_MAIN_EXIT) &&
		(this->menu != Menus::MENU_END_) &&
		(this->menu != Menus::MENU_END_FINALQUIT)
		) {
		this->drawScrollbar(graphics);
	}

	/*v29 = this->menu;
	v30 = v29 == Menus::MENU_MAIN_CONFIRMNEW;
	if (v29 != Menus::MENU_MAIN_CONFIRMNEW)
		v30 = v29 == Menus::MENU_MAIN_OPTIONS;
	if (!v30)
	{
		v31 = v29 == Menus::MENU_MAIN_DIFFICULTY;
		if (v29 != Menus::MENU_MAIN_DIFFICULTY)
			v31 = v29 == Menus::MENU_MAIN_MINIGAME;
		if (!v31)
		{
			v31 = v29 == Menus::MENU_MAIN_MORE_GAMES;
			if (v29 != Menus::MENU_MAIN_MORE_GAMES)
			{
				v31 = v29 == Menus::MENU_MAIN_EXIT;
				if (v29 != Menus::MENU_MAIN_EXIT)
					v31 = v29 == Menus::MENU_END_;
			}
		}
		if (!v31 && v29 != Menus::MENU_END_FINALQUIT) {
			this->drawScrollbar(graphics);
		}
	}*/

	items = this->items;
	v33 = -v76;
	numItems = this->numItems;
	v79 = 0;
	v78 = menuRect[2] / Applet::CHAR_SPACING[app->fontType];

	while (v79 < numItems && v33 < app->canvas->menuRect[3])
	{
		bool isScrollBar = (items->flags & Menus::ITEM_SCROLLBAR); // [GEC]
		bool isScrollBarTwo = (items->flags & Menus::ITEM_SCROLLBARTWO); // [GEC]
		action = (items->action && !isScrollBar && !isScrollBarTwo) ? items->action : 0; // [GEC]

		if ((items->flags & 0x8000) != 0)
		{
			v35 = v33;
			goto LABEL_155;
		}
		v35 = v33 + this->getMenuItemHeight(v79);

		if ((items->flags & Menus::ITEM_DISABLEDTWO) != 0) { // [GEC]
			app->setFontRenderMode(2);
		}

		if (v35 > 0)
		{
			v80 = v33 + menuRect[1];
			if (items->textField != 3072 || (items->flags & 0x40) != 0)
			{
				textBuffer1->setLength(0);
				if ((items->flags & 0x400) != 0)
				{
					textBuffer1->append('\x87');
					textBuffer1->append(" ");
				}
				app->localization->composeTextField(items->textField, textBuffer1);
				if ((items->flags & 2) == 0) {
					textBuffer1->dehyphenate();
				}
				if (action)
				{
					v37 = textBuffer1->getStringWidth();
					menuItem_width = this->menuItem_width;
					if (v37 + 10 > menuItem_width)
					{
						textBuffer1->setLength((menuItem_width - 10) / Applet::FONT_WIDTH[app->fontType]);
						textBuffer1->append('\x85');
					}
				}
				flags = items->flags;
				if ((flags & 0x40) != 0)
				{
					this->buildDivider(textBuffer1, v78);
					flags = items->flags;
				}
				if ((flags & 8) != 0)
				{
					v40 = textBuffer1->getStringWidth(false);
					v42 = v40 >> 1;
					if (action)
					{
						v81 = menuRect[0] + (this->menuItem_width >> 1) - v42;
						goto LABEL_130;
					}
					v81 = menuRect[0] + (menuRect[2] >> 1) - v42;
				LABEL_121:
					v46 = this->menu;
					v47 = v46 == Menus::MENU_MAIN_EXIT;
					if (v46 != Menus::MENU_MAIN_EXIT)
						v47 = v46 == Menus::MENU_END_RANKING;
					if (v47 || v46 == Menus::MENU_MAIN_DIFFICULTY || v46 == Menus::MENU_MAIN_MINIGAME || v46 == Menus::MENU_END_ || v46 == Menus::MENU_END_FINALQUIT || v46 == Menus::MENU_SELECT_LANGUAGE)
					{
						v80 += 8;
						goto LABEL_132;
					}
				}
				else
				{
					v43 = menuRect[0];

					if (action)
						flags = v43 + 8;
					else
						v81 = menuRect[0];

					if (action)
						v81 = flags;

					textField2 = items->textField2;
					if (textField2 != 3072)
					{
						if (action)
							textField2 = this->menuItem_width;
						else
							v36 = menuRect + 2;

						if (action)
							v43 += textField2;
						else
							textField2 = *v36;

						if (action)
							v45 = v43 - 8;
						else
							v45 = v43 + textField2;

						textBuffer2->setLength(0);
						app->localization->composeTextField(items->textField2, textBuffer2);
						if (!(items->flags & Menus::ITEM_NODEHYPHENATE)) {
							textBuffer2->dehyphenate();
						}

						if ((items->flags & Menus::ITEM_BINDING) && ((this->menu == Menus::MENU_MAIN_BINDINGS)  || (this->menu == Menus::MENU_INGAME_BINDINGS))) { // New Flag
							if (app->time > this->nextMsgTime) {
								this->nextMsgTime = app->time + 1000;
								this->nextMsg++;
							}

							int j;
							for (j = 0; j < KEYBINDS_MAX; j++) {
								if (keyMappingTemp[items->param].keyBinds[j] == -1) {
									break;
								}
							}

							if (j != 0) {
								textBuffer2->setLength(0);
								if (keyMappingTemp[items->param].keyBinds[this->nextMsg % j] & IS_CONTROLLER_BUTTON) {
									textBuffer2->append(buttonNames[keyMappingTemp[items->param].keyBinds[this->nextMsg % j] & ~(IS_CONTROLLER_BUTTON | IS_MOUSE_BUTTON)]);
								}
								else {
									textBuffer2->append((char*)SDL_GetScancodeName((SDL_Scancode)keyMappingTemp[items->param].keyBinds[this->nextMsg % j]));
								}
							} else if (textBuffer2->length() == 0) {
								textBuffer2->append("Unbound");
							}
						}
						
						if (action) {
							graphics->drawString(textBuffer2, v45, v80 + (this->menuItem_height >> 1), 10);
						}
						else {
							graphics->drawString(textBuffer2, v45, v80, 24);
						}
						
					}
					if (!action)
						goto LABEL_121;
				LABEL_130:
					v46 = this->menu;
				}
				v49 = v46 == Menus::MENU_MAIN_CONFIRMNEW;
				if (v46 == Menus::MENU_MAIN_CONFIRMNEW)
					v49 = v79 == 3;
				if (v49)
					v80 += 7;


				if (isScrollBarTwo) { // [GEC]
					v81 -= (v81 - ((480 - this->imgMenuOptionBOX3->width) >> 1)) - 15;
				}

				if (this->type != 5 && this->type != 7 && this->menu != Menus::MENU_VENDING_MACHINE_CANT_BUY && this->menu != Menus::MENU_VENDING_MACHINE_DETAILS && this->menu != Menus::MENU_VENDING_MACHINE_CONFIRM && v79 == this->selectedIndex) {
					int n11 = v81 + canvas->OSC_CYCLE[app->time / 100 % 4];
					if (action) {
						graphics->drawCursor(n11 + 3, v80 + (this->menuItem_height >> 1) - 8, 8);
					}
					else {
						graphics->drawCursor(n11 + 3, v80, 8);
					}
					v81 += 8;
				}

				if (action)
					graphics->drawString(textBuffer1, v81, v80 + (this->menuItem_height >> 1), 2);
				else
					LABEL_132:
				graphics->drawString(textBuffer1, v81, v80, 0);


				if (isScrollBar || isScrollBarTwo) { //[GEC] New system
					this->drawCustomScrollbar(graphics, items, textBuffer1, (action) ? v80 + (this->menuItem_height >> 1) : v80);
				}
				
				if (items->flags & Menus::ITEM_DISABLED) {
					if (action) {
						if (this->menu == Menus::MENU_MAIN_OPTIONS) {
							graphics->FMGL_fillRect(menuRect[0] + 4, v33 + menuRect[1] + 6,
								this->menuItem_width - 7, this->menuItem_height - 15,
								0.2, 0.2, 0.2, 0.5);
						}
						else {
							graphics->FMGL_fillRect(menuRect[0],v33 + menuRect[1],
								this->menuItem_width, this->menuItem_height,
								this->m_menuButtons->GetButton(0)->normalRed,
								this->m_menuButtons->GetButton(0)->normalGreen,
								this->m_menuButtons->GetButton(0)->normalBlue,
								this->m_menuButtons->GetButton(0)->normalAlpha);
						}
					}
					else
					{
						v55 = textBuffer1->length();
						textBuffer1->setLength(0);
						v50 = 0;
						while (v50 < v55)
						{
							textBuffer1->append('\x89');
							++v50;
						}
						graphics->drawString(textBuffer1, v81, v80, 0);
					}
				}
				if (items->flags & Menus::ITEM_LEFT_ARROW) {
					graphics->drawRegion(app->hud->imgAttArrow, 0, 0, 12, 12, v81 - 17, v80, 0, 1, 0);
				}
				if (items->flags & Menus::ITEM_RIGHT_ARROW) {
					v56 = textBuffer1->getStringWidth(false);
					graphics->drawRegion(app->hud->imgAttArrow, 0, 0, 12, 12, v56 + v81 + 5, v80, 0, 3, 0);
				}
			}
		}
		app->setFontRenderMode(0); // [GEC]
		numItems = this->numItems;
	LABEL_155:
		++items;
		v33 = v35;
		++v79;
	}
	if (this->drawLogo) {
		graphics->setClipRect(0, 0, 480, 320);
		graphics->drawImage(this->imgLogo, (480 - this->imgLogo->width) >> 1, 0, 0, 0, 0);
	}
	if (this->drawHelpText)
	{
		imgGameMenuTornPage = this->imgGameMenuTornPage;
		graphics->setClipRect(0, 0, 480, 320);
		graphics->FMGL_fillRect(0, 0, 480, 320, 0.0, 0.0, 0.0, 0.5);
		v60 = this->menu;
		v61 = 230 - (imgGameMenuTornPage->width >> 1);
		if (v60 <= Menus::MENU_ITEMS_HOLY_WATER_MAX)
		{
			v82 = 10;
		}
		else
		{
			v59 = 0.0;
			vendingMachine = app->vendingMachine.get();
		}
		if (v60 >= Menus::MENU_VENDING_MACHINE)
		{
			imgGameMenuTornPage = vendingMachine->imgVendingBG;;
			v61 = v59;
			v82 = (v59);
		}
		graphics->drawImage(imgGameMenuTornPage, v61, v82, 0, 0, 0);
		v62 = app->localization->getLargeBuffer();
		v63 = app->canvas.get();
		v64 = v62;
		menuHelpMaxChars = v63->menuHelpMaxChars;
		v63->menuHelpMaxChars = this->imgGameMenuTornPage->width / Applet::FONT_WIDTH[app->fontType];
		this->items[this->selectedHelpIndex].WrapHelpText(v62);
		app->canvas->menuHelpMaxChars = menuHelpMaxChars;
		graphics->drawString(
			v64,
			v61 + (imgGameMenuTornPage->width >> 1),
			v82 + (this->imgGameMenuTornPage->height >> 1),
			3);
		v64->dispose();
	}



	if (this->setBinding) {
		graphics->FMGL_fillRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0, 0, 0, 0.75f);
		textBuffer1->setLength(0);
		textBuffer1->append("Press New Key For");
		graphics->drawString(textBuffer1, canvas->SCR_CX, canvas->SCR_CY, Graphics::ANCHORS_CENTER);
		textBuffer1->setLength(0);
		app->localization->composeTextField(this->items[this->selectedIndex].textField, textBuffer1);
		textBuffer1->deleteAt(textBuffer1->length()-1, 1);
		graphics->drawString(textBuffer1, canvas->SCR_CX, canvas->SCR_CY + Applet::FONT_HEIGHT[app->fontType], Graphics::ANCHORS_CENTER);
	}


	textBuffer1->dispose();
	textBuffer2->dispose();
}

void MenuSystem::setItemsFromText(int i, Text* text, int i2, int i3, int i4) {


	this->numItems = i;
	text->wrapText(i2);
	int n5;
	int first;
	for (n5 = 0; (first = text->findFirstOf('|', n5)) >= 0; n5 = first + 1) {
		app->localization->addTextArg(text, n5, first);
		this->items[i++].Set(getLastArgString(), MenuSystem::EMPTY_TEXT, 0x2 | i3, 0, i4, MenuSystem::EMPTY_TEXT);
	}
	app->localization->addTextArg(n5, text->length());
	this->items[i++].Set(getLastArgString(), MenuSystem::EMPTY_TEXT, 0x2 | i3, 0, i4, MenuSystem::EMPTY_TEXT);
	this->numItems = i;
	text->dispose();
}


void MenuSystem::returnToGame() {


	this->numItems = 0;
	app->time = app->lastTime = app->upTimeMs;
	if (app->game->isCameraActive()) {
		app->canvas->setState(Canvas::ST_INTER_CAMERA);
	}
	else {
		app->canvas->setState(Canvas::ST_PLAYING);
	}
}

void MenuSystem::initMenu(int menu) {

	Text* textbuff;
	int flags = 0;

	if (this->oldMenu != menu) {
		this->scrollIndex = 0;
		this->selectedIndex = 0;
	}
	this->maxItems = 4;  // [GEC] 4 por defecto
	this->drawLogo = false; // [GEC]

	this->numItems = 0;
	for (int i = 0; i < 50; i++) {
		this->items[i].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
	}

	textbuff = app->localization->getLargeBuffer();
	app->localization->resetTextArgs();
	//printf("initMenu %d\n", menu);
	this->setMenuSettings();

	// Apply YAML-driven init properties (type, selectedIndex, etc.)
	const YAMLMenuDef* menuDef = getMenuDef(menu);
	if (menuDef) {
		if (menuDef->type > 0) {
			this->type = menuDef->type;
		}
		if (menuDef->selectedIndex > 0) {
			this->selectedIndex = menuDef->selectedIndex;
		}
		if (menuDef->scrollIndex >= 0) {
			this->scrollIndex = menuDef->scrollIndex;
		}
		// Help menus: load resource and return early
		if (menuDef->helpResource >= 0) {
			this->LoadHelpResource((short)menuDef->helpResource);
			if (menu == Menus::MENU_MAIN_EFFECTHELP) {
				this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
			}
			return;
		}
		// SetYESNO dialogs: apply from YAML and return early
		if (menuDef->yesnoStringId >= 0) {
			if (menuDef->yesnoClearStack) {
				this->clearStack();
			}
			this->scrollIndex = 0;
			this->SetYESNO((short)menuDef->yesnoStringId, menuDef->yesnoSelectYes,
						   menuDef->yesnoYesAction, menuDef->yesnoYesParam,
						   menuDef->yesnoNoAction, menuDef->yesnoNoParam);
			return;
		}
	}

	switch (menu) {
		case Menus::MENU_LEVEL_STATS: {
			this->loadMenuItems(menu, 0, -1);
			this->items[3].textField = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1);
			this->items[5].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2);
			this->items[6].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3);
			this->items[7].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4);
			this->items[8].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5);
			this->items[9].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6);
			this->items[10].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7);
			this->fillStatus(true, false, false);
			break;
		}
		case Menus::MENU_MAIN: {
			int index = 2;
			if (app->game->hasSavedState()) {
				index++;
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::CONTINUE_ITEM), MenuSystem::EMPTY_TEXT, 8, 22, 0, MenuSystem::EMPTY_TEXT);
			}
			// [GEC] Don't load "other games" item
			{
				this->loadMenuItems(menu, 0, 2);
				this->loadMenuItems(menu, 3, -1);
			}

			break;
		}
		// MENU_MAIN_DIFFICULTY, MENU_MAIN_MINIGAME, MENU_MAIN_HELP
		case Menus::MENU_INGAME_OPTIONS: // [GEC]
		case Menus::MENU_MAIN_OPTIONS: {
			if (!this->loadYAMLMenuItems(menu)) {
				// Fallback: build items in C++ (GEC menus without YAML items)
				if (menu == Menus::MENU_MAIN_OPTIONS) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_OPTIONS_SOUND, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_OPTIONS_VIDEO, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_OPTIONS_INPUT, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_OPTIONS_SOUND, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_OPTIONS_VIDEO, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_OPTIONS_INPUT, MenuSystem::EMPTY_TEXT);
				}
				// Text args for item labels
				textbuff->setLength(0);
				textbuff->append("Options");
				app->localization->addTextArg(textbuff);
				textbuff->setLength(0);
				textbuff->append("Sound");
				app->localization->addTextArg(textbuff);
				textbuff->setLength(0);
				textbuff->append("Video");
				app->localization->addTextArg(textbuff);
				textbuff->setLength(0);
				textbuff->append("Input");
				app->localization->addTextArg(textbuff);
			}
			break;
		}
		case Menus::MENU_SELECT_LANGUAGE: {
			flags = 8;
		}
		case Menus::MENU_INGAME_LANGUAGE: {
			this->oldLanguageSetting = app->localization->defaultLanguage;
			this->type = ((menu == Menus::MENU_INGAME_LANGUAGE) ? 1 : 4);
			this->scrollIndex = 0;
			if (this->peekMenu() != 25) {
				textbuff->setLength(0);
				app->localization->composeText((short)3, (short)80, textbuff);
				if (this->type == 4) {
					while (textbuff->length() < 8) {
						textbuff->append(' ');
					}
				}
				app->localization->addTextArg(textbuff);
				this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, flags, 2, 0, MenuSystem::EMPTY_TEXT);
			}

			//for (int l = 0; l < 5; ++l) {
			for (int l = 0; l < 1; ++l) {
				app->localization->loadGroupFromYAML(l, 14);
				textbuff->setLength(0);
				app->localization->composeText((short)14, (short)0, textbuff);
				if (this->type == 1) {
					textbuff->trim();
				}
				textbuff->dehyphenate();
				app->localization->addTextArg(textbuff);
				this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, flags, 19, l, MenuSystem::EMPTY_TEXT);
			}
			app->localization->loadGroupFromYAML(app->localization->defaultLanguage, 14);
			break;
		}
		case Menus::MENU_END_RANKING: {
			this->clearStack();
			this->scrollIndex = 0;
			this->selectedIndex = 0;
			this->FillRanking();
			break;
		}
		// MENU_ENABLE_SOUNDS now handled by YAML-driven yesno above
		case Menus::MENU_INGAME: {
			if (app->player->isFamiliar) {
				this->loadMenuItems(menu, 0, 1);
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)355), MenuSystem::EMPTY_TEXT, 0, 33, 0, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)356));
				this->loadMenuItems(menu, 1, -1);
				this->items[0].flags = 4;
				this->items[6].flags = 4;
			}
			else {
				this->loadMenuItems(menu, app->player->isFamiliar, -1);
				if (app->player->inventory[18] == 0) {
					this->items[2].flags = app->player->inventory[18] + 4;
				}
				if (app->game->isCameraActive()) {
					this->items[3].flags = 4;
					this->items[4].flags = 4;
					this->items[5].flags = 4;
					this->items[10].flags = 4;
					this->items[11].action = 1;
					this->items[11].param = 58;
				}
			}
			break;
		}

		case Menus::MENU_INGAME_PLAYER: {
			this->loadMenuItems(menu, 0, -1);
			this->items[2].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1);
			this->items[3].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2);
			this->items[4].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3);
			this->items[5].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4);
			this->items[6].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5);
			this->items[7].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6);
			this->items[8].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7);
			this->items[9].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT8);
			this->items[10].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT9);
			this->items[11].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT10);
			this->items[12].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT11);
			this->items[13].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT12);
			this->items[14].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT13);
			this->items[15].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT14);
			this->items[16].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT15);
			this->items[17].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT16);
			this->items[18].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT17);
			this->fillStatus(false, true, true);
			break;
		}

		case Menus::MENU_INGAME_LEVEL: {
			this->loadMenuItems(menu, 0, -1);
			this->items[1].textField = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1);
			this->items[2].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2);
			this->items[3].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3);
			this->items[4].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4);
			this->items[5].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5);
			this->items[6].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6);
			this->items[7].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7);
			this->fillStatus(true, false, false);
			break;
		}

		case Menus::MENU_INGAME_GRADES: {
			this->loadMenuItems(menu, 0, -1);
			this->buildLevelGrades(textbuff);
			break;
		}

#if 0 // Old
		case Menus::MENU_INGAME_OPTIONS: {
			this->loadMenuItems(menu, 0, 1); // Controls item
			// [GEC] New
			{
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGESFXVOLUME, 1, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGEMUSICVOLUME, 2, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
				textbuff->setLength(0);
				textbuff->append(" SoundFx Volume");
				app->localization->addTextArg(textbuff);
				textbuff->setLength(0);
				textbuff->append(" Music Volume");
				app->localization->addTextArg(textbuff);
				if (!isUserMusicOn()) {
					this->items[3].flags |= Menus::ITEM_DISABLEDTWO;
				}
			}
			//this->items[1].textField = Localization::STRINGID(MenuSystem::INDEX_OTHER, MenuStrings::SOUND_FX_VOLUME);


#if 0 // BREW Only
			this->loadMenuItems(menu, 0, -1);
			int v22;
			if (app->sound->allowSounds)
			{
				this->items[1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::SOUND_LABEL);
				this->items[2].flags = (this->changeSfxVolume) ? 0x10000 : 0 | this->items[2].flags & 0xFFFFFFFB;
				this->items[2].textField2 = this->getLastArgString();
			}
			else {
				this->items[1].textField2 =  Localization::STRINGID(Strings::FILE_MENUSTRINGS, (app->canvas->vibrateEnabled) ? 147 : 69);
				this->items[2].flags |= 0x8000u;
				this->items[2].textField2 = MenuSystem::EMPTY_TEXT;
			}

			this->items[3].textField2 = Localization::STRINGID(Strings::FILE_TRANSLATIONS, (short)0);
			this->items[3].param = 36;
			--this->numItems;
#endif
			break;
		}
#endif

		case Menus::MENU_INGAME_QUESTLOG: {
			this->LoadNotebook();
			break;
		}

		// Menus handled by YAML early return or default:
		// help menus (help_resource), SetYESNO dialogs (yesno),
		// ingame_recipes, ingame_loadnosave, comic_book

#if 0 // Old
		case Menus::MENU_INGAME_CONTROLS: {
			this->loadMenuItems(menu, 0, -1);
			textbuff->setLength(0);
			if (app->canvas->field_0xd10_ == 0) {
				this->items[0].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)392);
			}
			else if (app->canvas->field_0xd10_ == 1) {
				this->items[0].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)393);
			}
			else if (app->canvas->field_0xd10_ == 2) {
				this->items[0].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)394);
			}
			if (app->canvas->isFlipControls) {
				this->items[1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)197);
			}
			else {
				this->items[1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)198);
			}

			// [GEC]
			{
				this->items[this->numItems++].Set(Localization::STRINGID(MenuSystem::INDEX_OTHER, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGEALPHA, 3, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);

				textbuff->setLength(0);
				textbuff->append(" Alpha");
				app->localization->addTextArg(textbuff);
			}

			break;
		}
#endif

		case Menus::MENU_DEBUG: {
			this->loadMenuItems(menu, 0, -1);
			if (this->peekMenu() == 3) {
				this->items[0].action = 1;
				this->items[0].param = 3;
			}
			this->items[5].textField2 = this->onOffValue(app->player->noclip);
			this->items[6].textField2 = this->onOffValue(app->combat->oneShotCheat);
			this->items[7].textField2 = this->onOffValue(app->game->disableAI);
			this->items[8].textField2 = this->onOffValue(app->game->skipMinigames);
			this->items[9].textField2 = this->onOffValue(app->player->enableHelp);
			this->items[10].textField2 = this->onOffValue(app->player->god);
			this->items[11].textField2 = this->onOffValue(app->canvas->showLocation);
			app->localization->addTextArg(app->canvas->animFrames);
			this->items[12].textField2 = Localization::STRINGID((short)3, (short)241);
			this->items[13].textField2 = this->onOffValue(app->canvas->showSpeeds);
			this->items[14].textField2 = this->onOffValue(app->render->skipFlats);
			this->items[15].textField2 = this->onOffValue(app->render->skipCull);
			this->items[16].textField2 = this->onOffValue(app->render->skipBSP);
			this->items[17].textField2 = this->onOffValue(app->render->skipLines);
			this->items[18].textField2 = this->onOffValue(app->render->skipSprites);
			this->items[19].textField2 = this->onOffValue(app->canvas->renderOnly);
			this->items[20].textField2 = this->onOffValue(app->render->skipDecals);
			this->items[21].textField2 = this->onOffValue(app->render->skip2DStretch);
			if (app->render->renderMode == 0) {
				this->items[22].textField2 = Localization::STRINGID((short)3, (short)298);
			}
			else if (app->render->renderMode == 63) {
				this->items[22].textField2 = Localization::STRINGID((short)3, (short)299);
			}
			else if (app->render->renderMode == 31) {
				this->items[22].textField2 = Localization::STRINGID((short)3, (short)300);
			}
			else {
				Text* smallBuffer = app->localization->getSmallBuffer();
				smallBuffer->setLength(0);
				if ((app->render->renderMode & 0x1) != 0x0) {
					smallBuffer->append('T');
				}
				if ((app->render->renderMode & 0x2) != 0x0) {
					smallBuffer->append('C');
				}
				if ((app->render->renderMode & 0x4) != 0x0) {
					smallBuffer->append('P');
				}
				if ((app->render->renderMode & 0x8) != 0x0) {
					smallBuffer->append('S');
				}
				if ((app->render->renderMode & 0x10) != 0x0) {
					smallBuffer->append('R');
				}
				app->localization->addTextArg(smallBuffer);
				smallBuffer->dispose();
				this->items[22].textField2 = this->getLastArgString();
			}
			this->items[26].textField2 = this->onOffValue(app->canvas->showFreeHeap);
			break;
		}

		case Menus::MENU_DEBUG_MAPS: { // Menus::MENU_DEBUG_MAPS
			this->type = 1;
			this->addItem(Localization::STRINGID((short)3, (short)80), MenuSystem::EMPTY_TEXT, 0, 2, 0, MenuSystem::EMPTY_TEXT);
			if (this->peekMenu() == 77) {
				items[0].action = 26;
			}
			for (int k = 0; k < 10; ++k) {
				this->addItem(Localization::STRINGID((short)3, app->game->levelNames[k]), MenuSystem::EMPTY_TEXT, 0, 17, k, MenuSystem::EMPTY_TEXT);
			}
			break;
		}

		case Menus::MENU_DEBUG_STATS: {
			app->localization->resetTextArgs();
			/*app->localization->addTextArg((app->getTotalMemory() + 1023) / 1024);
			app->localization->addTextArg((int)(App.initialMemory + 1023L) / 1024);
			app->localization->addTextArg((App.getMemFootprint() + 1023) / 1024);
			app->localization->addTextArg((App.getFreeMemory() + 1023) / 1024);
			app->localization->addTextArg((App.findLargestMemoryBlock() + 1023) / 1024);
			app->localization->addTextArg(((int)App.peakMemoryUsage + 1023) / 1024);
			app->localization->addTextArg((Render.mapMemoryUsage + 1023) / 1024);
			app->localization->addTextArg((Render.spriteMem + 1023) / 1024);
			app->localization->addTextArg((Render.lineMem + 1023) / 1024);
			app->localization->addTextArg((Render.nodeMem + 1023) / 1024);
			largeBuffer.setLength(0);
			largeBuffer.append((Render.texelMemoryUsage + 1023) / 1024);
			largeBuffer.append('/');
			largeBuffer.append((Render.paletteMemoryUsage + 1023) / 1024);
			Text.addTextArg(largeBuffer);
			Text.addTextArg(App.imageMemory);
			loadMenuItems(n);
			items[1].textField2 = Text.STRINGID((short)3, (short)235);
			items[2].textField2 = Text.STRINGID((short)3, (short)236);
			items[3].textField2 = Text.STRINGID((short)3, (short)237);
			items[4].textField2 = Text.STRINGID((short)3, (short)238);
			items[5].textField2 = Text.STRINGID((short)3, (short)239);
			items[6].textField2 = Text.STRINGID((short)3, (short)240);
			items[7].textField2 = Text.STRINGID((short)3, (short)241);
			items[8].textField2 = Text.STRINGID((short)3, (short)242);
			items[9].textField2 = Text.STRINGID((short)3, (short)243);
			items[10].textField2 = Text.STRINGID((short)3, (short)244);
			items[11].textField2 = Text.STRINGID((short)3, (short)245);
			items[12].textField2 = Text.STRINGID((short)3, (short)246);
			*/
			break;
		}

		case Menus::MENU_SHOWDETAILS: {
			Text* weaponStatStr = nullptr;
			if (this->detailsDef != nullptr && this->detailsDef->eType == Enums::ET_ITEM && this->detailsDef->eSubType == Enums::ITEM_WEAPON) {
				weaponStatStr = app->combat->getWeaponStatStr(this->detailsDef->parm);
			}
			app->localization->resetTextArgs();
			this->type = 7;
			this->setItemsFromText(0, this->detailsTitle, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
			for (int n15 = 0; n15 < this->numItems; ++n15) {
				this->items[n15].flags |= 0x8;
			}
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 73, 0, 0, MenuSystem::EMPTY_TEXT);
			this->setItemsFromText(this->numItems, this->detailsHelpText, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
			if (weaponStatStr != nullptr) {
				this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 73, 0, 0, MenuSystem::EMPTY_TEXT);
				this->setItemsFromText(this->numItems, weaponStatStr, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
				weaponStatStr->dispose();
			}
			this->detailsTitle->dispose();
			this->detailsHelpText->dispose();
			break;
		}

		case Menus::MENU_ITEMS: {
			//this->loadMenuItems(menu, 0, 3); // BREW "resume game" item
			int n4 = 1;
			for (int n5 = 16; n5 < 18; ++n5) {
				short n6 = app->player->inventory[n5 - 0];
				if (n6 > 0) {
					if (n4 != 0) {
						n4 = 0;
						this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)112), MenuSystem::EMPTY_TEXT, 73, 0, 0, MenuSystem::EMPTY_TEXT);
					}
					textbuff->setLength(0);
					textbuff->append(n6);
					EntityDef* find = app->entityDefManager->find(6, 0, n5);
					app->localization->addTextArg(textbuff);
					this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->name), this->getLastArgString(), 32, 24, n5, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
				}
			}
			int fwIdx = app->combat->fountainWeaponIdx;
			int fwAmmoType = app->combat->fountainAmmoType;
			if (fwIdx >= 0 && fwAmmoType >= 0 && app->player->ammo[fwAmmoType] >= 25 && (app->player->weapons & (1 << fwIdx)) != 0x0) {
				textbuff->setLength(0);
				textbuff->append(app->player->ammo[fwAmmoType]);
				EntityDef* find2 = app->entityDefManager->find(6, 2, fwAmmoType);
				app->localization->addTextArg(textbuff);
				this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find2->name), this->getLastArgString(), 32, 24, 22, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find2->description));
			}
			int n7 = 1;
			for (int n8 = 11; n8 < 13; ++n8) {
				short n9 = app->player->inventory[n8 - 0];
				if (n9 > 0) {
					if (n7 != 0) {
						n7 = 0;
						this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)113), MenuSystem::EMPTY_TEXT, 73, 0, 0, MenuSystem::EMPTY_TEXT);
					}
					textbuff->setLength(0);
					textbuff->append(n9);
					EntityDef* find3 = app->entityDefManager->find(6, 0, n8);
					app->localization->addTextArg(textbuff);
					addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find3->name), this->getLastArgString(), 32, 24, n8, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find3->description));
				}
			}
			//this->loadMenuItems(menu, 3, 1);
			this->loadMenuItems(menu, 1, 1); // Load "inventory label" item
			if (app->player->hasANanoDrink()) {
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)178), Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)0), 0, 1, 75, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)179));
			}
			this->loadMenuItems(menu, 2, -1); // Load the remaining items

			//this->loadMenuItems(menu, 4, -1);
			textbuff->setLength(0);
			textbuff->append(app->player->inventory[24]);
			app->localization->addTextArg(textbuff);
			EntityDef* find4 = app->entityDefManager->find(6, 0, 24);
			this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find4->name), this->getLastArgString(), 32/*33*/, 16, 24, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find4->description));
			this->loadIndexes(4);

			this->selectedIndex = 0; // [GEC]
			break;
		}

		case Menus::MENU_ITEMS_WEAPONS: {
			this->loadMenuItems(menu, 0, -1);
			this->loadIndexes(1);

			this->selectedIndex = 0; // [GEC]

			for (int n10 = 0; n10 < 15; ++n10) {
				if ((app->player->weapons & 1 << n10) != 0x0) {
					int n11 = (app->combat->getWeaponFlags(app->player->ce->weapon).isThrowableItem && n10 != app->player->ce->weapon) ? 4 : 0;
					EntityDef* find5 = app->entityDefManager->find(6, 1, (uint8_t)n10);
					if (app->player->ce->weapon == n10) {
						n11 |= 0x400;
					}
					int n12 = n10 * 9;
					textbuff->setLength(0);
					if (app->combat->weapons[n12 + 4] != 0) {
						int ammoType = app->combat->weapons[n12 + 4];
						if (app->combat->getWeaponFlags(n10).soulAmmoDisplay) {
							textbuff->append(app->player->ammo[ammoType]);
							textbuff->append('/');
							textbuff->append(app->combat->weapons[n12 + Combat::WEAPON_FIELD_AMMOUSAGE]);
						}
						else {
							textbuff->append(app->player->ammo[ammoType]);
						}
					}
					else {
						textbuff->append('\x80');
					}
					app->localization->addTextArg(textbuff);
					this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find5->name), this->getLastArgString(), n11 | 0x20, 18, n10, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find5->description));
				}
			}
			break;
		}

		case Menus::MENU_ITEMS_DRINKS: {
			this->loadMenuItems(menu, 0, -1);
			this->loadIndexes(2);
			this->selectedIndex = 0; // [GEC]

			bool b = false;
			for (int n13 = 0; n13 < 11; ++n13) {
				short n14 = app->player->inventory[n13 - 0];
				if (n14 > 0) {
					b = true;
					textbuff->setLength(0);
					textbuff->append(n14);
					EntityDef* find6 = app->entityDefManager->find(6, 0, n13);
					app->localization->addTextArg(textbuff);
					this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find6->name), this->getLastArgString(), 32, 24, n13, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find6->description));
				}
			}
			if (!b) {
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)1), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT); // [None]
			}
			break;
		}

		case Menus::MENU_ITEMS_CONFIRM: {
			this->type = 6;
			this->scrollIndex = 0;
			EntityDef* find7 = app->entityDefManager->find(6, 0, this->menuParam);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_ENTITYSTRINGS, find7->name, textbuff);
			app->localization->addTextArg(textbuff);
			if (this->peekMenu() == 75) {
				this->SetYESNO((short)184, 1, 20, this->menuParam);
				break;
			}
			this->SetYESNO((short)184, 1, 21, this->menuParam);
			break;
		}

		case Menus::MENU_VENDING_MACHINE: {
			//this->scrollIndex = 0; // [GEC]
			//this->selectedIndex = 3; // [GEC]

			this->loadMenuItems(menu, 0, 1);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(app->player->inventory[24]);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, (short)197, textbuff);
			app->localization->resetTextArgs();
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);
			break;
		}

		case Menus::MENU_VENDING_MACHINE_DRINKS: {
			//this->scrollIndex = 0; // [GEC]
			//this->selectedIndex = 3; // [GEC]

			this->loadMenuItems(menu, 0, 1);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(app->player->inventory[24]);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, (short)197, textbuff);
			app->localization->resetTextArgs();
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);

			for (int n16 = 0; n16 < 11; ++n16) {
				if (app->vendingMachine->drinkInThisVendingMachine(n16)) {
					textbuff->setLength(0);
					textbuff->append(app->vendingMachine->getDrinkPrice(n16));
					EntityDef* entDef = app->entityDefManager->find(6, 0, n16);
					app->localization->addTextArg(textbuff);
					this->addItem(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entDef->name), this->getLastArgString(), 32, 28, n16, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entDef->description));
				}
			}
			break;
		}

		case Menus::MENU_VENDING_MACHINE_SNACKS: {
			//this->scrollIndex = 0; // [GEC]
			//this->selectedIndex = 3; // [GEC]

			this->loadMenuItems(menu, 0, 1);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(app->player->inventory[24]);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, (short)197, textbuff);
			app->localization->resetTextArgs();
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);
			this->fillVendingMachineSnacks(app->canvas->loadMapID, textbuff);
			break;
		}

		case Menus::MENU_VENDING_MACHINE_CONFIRM: {
			bool b2 = false;
			if (this->menuParam >= 0 && this->menuParam < 11) {
				b2 = true;
			}
			EntityDef* entityDef;
			int currentItemPrice;
			int n17;
			int n18;
			if (b2) {
				entityDef = app->entityDefManager->find(6, 0, this->menuParam);
				currentItemPrice = app->vendingMachine->getDrinkPrice(this->menuParam);
				n17 = Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entityDef->name);
				n18 = 29;
			}
			else {
				entityDef = app->entityDefManager->find(6, 0, 16);
				currentItemPrice = app->vendingMachine->getSnackPrice();
				n17 = this->menuParam;
				n18 = 30;
			}

			app->vendingMachine->currentItemPrice = currentItemPrice;
			this->loadMenuItems(menu, 0, 1);
			Text* smallBuffer5 = app->localization->Localization::getSmallBuffer();
			app->localization->resetTextArgs();
			smallBuffer5->setLength(0);
			smallBuffer5->append(app->player->inventory[24]);
			app->localization->addTextArg(smallBuffer5);
			smallBuffer5->setLength(0);
			app->localization->composeText(0, 197, smallBuffer5);
			Text* smallBuffer6 = app->localization->getSmallBuffer();
			app->localization->resetTextArgs();
			smallBuffer6->setLength(0);
			app->localization->composeText(n17, smallBuffer6);
			app->localization->addTextArg(smallBuffer6);
			smallBuffer6->setLength(0);
			smallBuffer6->append(app->vendingMachine->currentItemQuantity);
			app->localization->addTextArg(smallBuffer6);
			smallBuffer6->setLength(0);
			app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::COST_ITEM, smallBuffer6);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(currentItemPrice * app->vendingMachine->currentItemQuantity);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::COST_ITEM_EMPTY, textbuff);
			app->localization->resetTextArgs();

			app->localization->addTextArg(smallBuffer5);
			smallBuffer5->dispose();
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);

			app->localization->addTextArg(smallBuffer6);
			smallBuffer6->dispose();
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 0, n18, this->menuParam, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entityDef->description));
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);

			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem((Localization::STRINGID(Strings::FILE_MENUSTRINGS, 346)), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem((Localization::STRINGID(Strings::FILE_MENUSTRINGS, 347)), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem((Localization::STRINGID(Strings::FILE_MENUSTRINGS, 348)), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem((Localization::STRINGID(Strings::FILE_MENUSTRINGS, 349)), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			break;
		}

		case Menus::MENU_VENDING_MACHINE_CANT_BUY: {
			bool b3 = false;
			if (this->menuParam >= 0 && this->menuParam < 11) {
				b3 = true;
			}
			EntityDef* entityDef2;
			int n21;
			int n22;
			if (b3) {
				entityDef2 = app->entityDefManager->find(6, 0, this->menuParam);
				n21 = app->vendingMachine->getDrinkPrice(this->menuParam);
				n22 = Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entityDef2->name);
			}
			else {
				entityDef2 = app->entityDefManager->find(6, 0, 16);
				n21 = app->vendingMachine->getSnackPrice();
				n22 = this->menuParam;
			}
			this->loadMenuItems(menu, 0, 1);
			Text* smallBuffer7 = app->localization->getSmallBuffer();
			app->localization->resetTextArgs();
			smallBuffer7->setLength(0);
			smallBuffer7->append(app->player->inventory[24]);
			app->localization->addTextArg(smallBuffer7);
			smallBuffer7->setLength(0);
			app->localization->composeText((short)0, (short)197, smallBuffer7);
			Text* smallBuffer8 = app->localization->getSmallBuffer();
			app->localization->resetTextArgs();
			smallBuffer8->setLength(0);
			app->localization->composeText(n22, smallBuffer8);
			app->localization->addTextArg(smallBuffer8);
			smallBuffer8->setLength(0);
			smallBuffer8->append(app->vendingMachine->currentItemQuantity);
			app->localization->addTextArg(smallBuffer8);
			smallBuffer8->setLength(0);
			app->localization->composeText(Strings::FILE_MENUSTRINGS, (short)350, smallBuffer8);
			app->localization->resetTextArgs();
			textbuff->setLength(0);
			textbuff->append(n21 * app->vendingMachine->currentItemQuantity);
			app->localization->addTextArg(textbuff);
			textbuff->setLength(0);
			app->localization->composeText(Strings::FILE_MENUSTRINGS, (short)351, textbuff);
			app->localization->resetTextArgs();
			app->localization->addTextArg(smallBuffer7);
			smallBuffer7->dispose();
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->loadMenuItems(menu, 1, -1);
			app->localization->addTextArg(smallBuffer8);
			smallBuffer8->dispose();
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
			app->localization->addTextArg(textbuff);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)354), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
			this->addItem(Localization::STRINGID(Strings::FILE_CODESTRINGS, (short)96), MenuSystem::EMPTY_TEXT, 8, 2, 0, Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, entityDef2->description));
			break;
		}

		case Menus::MENU_VENDING_MACHINE_DETAILS: {
			app->localization->resetTextArgs();
			this->type = 9;
			this->setItemsFromText(0, this->detailsTitle, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
			for (int n19 = 0; n19 < this->numItems; ++n19) {
				this->items[n19].flags |= 0x8;
			}
			this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 72, 0, 0, MenuSystem::EMPTY_TEXT);
			this->setItemsFromText(this->numItems, this->detailsHelpText, app->canvas->ingameScrollWithBarMaxChars, 0, 0);
			items[0].action = 28;
			items[0].param = this->menuParam;
			for (int n20 = 1; n20 < this->numItems; ++n20) {
				this->items[n20].flags |= 0x1;
			}
			this->detailsTitle->dispose();
			this->detailsHelpText->dispose();
			break;
		}

		case Menus::MENU_INGAME_OPTIONS_SOUND:
		case Menus::MENU_MAIN_OPTIONS_SOUND: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_OPTIONS_SOUND) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBARTWO, Menus::ACTION_CHANGESFXVOLUME, 1, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBARTWO, Menus::ACTION_CHANGEMUSICVOLUME, 2, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGESFXVOLUME, 1, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGEMUSICVOLUME, 2, MenuSystem::EMPTY_TEXT);
				}
				textbuff->setLength(0); textbuff->append("Sound Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("SoundFx Volume:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Music Volume:"); app->localization->addTextArg(textbuff);
			}
			if (!isUserMusicOn()) {
				this->items[3].flags |= Menus::ITEM_DISABLEDTWO;
			}
			break;
		}

		case Menus::MENU_INGAME_OPTIONS_VIDEO:
		case Menus::MENU_MAIN_OPTIONS_VIDEO: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_OPTIONS_VIDEO) {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
				} else {
					this->maxItems = 5;
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				}
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), 0, Menus::ACTION_CHANGE_VID_MODE, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_TOG_VSYNC, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6), 0, Menus::ACTION_CHANGE_RESOLUTION, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_TOG_TINYGL, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT8), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_APPLY_CHANGES, 0, MenuSystem::EMPTY_TEXT);
				textbuff->setLength(0); textbuff->append("Video Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Window Mode:"); app->localization->addTextArg(textbuff);
			}
			// Dynamic runtime value updates for text2 fields
			{
				int argIdx = app->localization->numTextArgs;
				int windowMode = CAppContainer::getInstance()->sdlGL->windowMode;
				textbuff->setLength(0);
				if (windowMode == 0) textbuff->append("Windowed");
				else if (windowMode == 1) textbuff->append("Borderless");
				else if (windowMode == 2) textbuff->append("FullScreen");
				app->localization->addTextArg(textbuff);
				this->items[1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1 + argIdx);
				argIdx++;

				this->items[2].textField2 = this->onOffValue(CAppContainer::getInstance()->sdlGL->vSync);

				int resolutionIndex = CAppContainer::getInstance()->sdlGL->resolutionIndex;
				textbuff->setLength(0);
				textbuff->append("(")->append(sdlResVideoModes[resolutionIndex].width)->append("x")->append(sdlResVideoModes[resolutionIndex].height)->append(")");
				app->localization->addTextArg(textbuff);
				this->items[3].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1 + argIdx);
				argIdx++;

				if (!_glesObj->isInit) {
					this->items[4].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)197);
				} else {
					this->items[4].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)198);
				}

				textbuff->setLength(0); textbuff->append("VSync:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Resolution:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("TinyGL:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Apply Changes"); app->localization->addTextArg(textbuff);
			}
			break;
		}

		case Menus::MENU_INGAME_OPTIONS_INPUT:
		case Menus::MENU_MAIN_OPTIONS_INPUT: {  // [GEC]
			this->nextMsgTime = 0;
			this->nextMsg = 0;
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_OPTIONS_INPUT) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_CONTROLS, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_BINDINGS, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, Menus::ITEM_ALIGN_CENTER, Menus::ACTION_GOTO, Menus::MENU_MAIN_CONTROLLER, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_CONTROLS, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_BINDINGS, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_GOTO, Menus::MENU_INGAME_CONTROLLER, MenuSystem::EMPTY_TEXT);
				}
				textbuff->setLength(0); textbuff->append("Input Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Touch Controls"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Bindings      "); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Controller    "); app->localization->addTextArg(textbuff);
			}
			break;
		}

		case Menus::MENU_INGAME_CONTROLS:  // [GEC]
		case Menus::MENU_MAIN_CONTROLS: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				int itemIndx = 2;
				if (menu == Menus::MENU_MAIN_CONTROLS) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
				} else {
					itemIndx = 1;
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				}
				textbuff->setLength(0); textbuff->append("Touch Controls Options"); app->localization->addTextArg(textbuff);
				this->loadMenuItems(Menus::MENU_INGAME_CONTROLS, 0, -1);
				if (menu == Menus::MENU_MAIN_CONTROLS) {
					this->items[this->numItems++].Set(Localization::STRINGID(MenuSystem::INDEX_OTHER, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBARTWO, Menus::ACTION_CHANGEALPHA, 3, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(MenuSystem::INDEX_OTHER, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_SCROLLBAR, Menus::ACTION_CHANGEALPHA, 3, MenuSystem::EMPTY_TEXT);
				}
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
				textbuff->setLength(0); textbuff->append("Alpha:"); app->localization->addTextArg(textbuff);
			}
			// Dynamic runtime value updates for control layout and flip controls
			{
				int itemIndx = (menu == Menus::MENU_MAIN_CONTROLS) ? 2 : 1;
				if (app->canvas->m_controlLayout == 0) {
					this->items[itemIndx].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)392);
				}
				else if (app->canvas->m_controlLayout == 1) {
					this->items[itemIndx].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)393);
				}
				else if (app->canvas->m_controlLayout == 2) {
					this->items[itemIndx].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)394);
				}

				if (app->canvas->isFlipControls) {
					this->items[itemIndx + 1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)197);
				}
				else {
					this->items[itemIndx + 1].textField2 = Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)198);
				}
			}
			break;
		}

		case Menus::MENU_INGAME_BINDINGS:
		case Menus::MENU_MAIN_BINDINGS: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_BINDINGS) {
					this->maxItems = 8;
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
				} else {
					this->maxItems = 5;
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				}
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT4), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT5), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 1, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT6), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 4, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT7), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 5, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT8), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 2, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT9), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 3, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT10), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT11), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 8, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT12), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 6, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT13), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 7, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT14), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 9, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT15), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 10, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT16), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 11, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT17), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 12, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT18), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 13, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT19), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 14, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT20), Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT3), Menus::ITEM_BINDING, Menus::ACTION_SET_BINDING, 15, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT21), MenuSystem::EMPTY_TEXT, 0, Menus::ACTION_DEFAULT_BINDINGS, 0, MenuSystem::EMPTY_TEXT);
				this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT, 0, 0, MenuSystem::EMPTY_TEXT);
				textbuff->setLength(0); textbuff->append("Bindings Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("MOVEMENT"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Unbound"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Move Forward:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Move Back:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Strafe Left:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Strafe Right:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Turn Left:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Turn Right:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("ACTIONS"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Attack/Use:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Next Weapon:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Prev Weapon:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Pass Turn:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Automap:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Items:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Drinks:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("PDA:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Sniper Mode:"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Reset Defaults"); app->localization->addTextArg(textbuff);
			}
			break;
		}

		case Menus::MENU_INGAME_CONTROLLER:
		case Menus::MENU_MAIN_CONTROLLER: {  // [GEC]
			if (!this->loadYAMLMenuItems(menu)) {
				if (menu == Menus::MENU_MAIN_CONTROLLER) {
					this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_PADDING, 0, 14, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
				} else {
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT1), MenuSystem::EMPTY_TEXT, Menus::ITEM_NOSELECT | Menus::ITEM_ALIGN_CENTER | Menus::ITEM_DIVIDER, 0, 0, MenuSystem::EMPTY_TEXT);
					this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::ARGUMENT2), MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
				}
				textbuff->setLength(0); textbuff->append("Controller Options"); app->localization->addTextArg(textbuff);
				textbuff->setLength(0); textbuff->append("Vibration:"); app->localization->addTextArg(textbuff);
			}
			// Dynamic runtime value: vibration on/off
			{
				int itemIndx = (menu == Menus::MENU_MAIN_CONTROLLER) ? 2 : 1;
				this->items[itemIndx].textField2 = this->onOffValue(app->canvas->vibrateEnabled);
			}
			break;
		}

		default: {
			// Missing menus (injected from YAML, not in menuData) have no binary items
			if (menuDef && menuDef->isMissing) break;
			int startItem = (menuDef) ? menuDef->loadStartItem : 0;
			this->loadMenuItems(menu, startItem, -1);
			break;
		}
	}

	this->updateTouchButtonState();
	textbuff->dispose();

	// [GEC] Restored From J2ME/BREW
	if (this->selectedIndex >= this->numItems) {
		this->moveDir(-1);
	}
	else if (this->items[this->selectedIndex].textField == MenuSystem::EMPTY_TEXT || (this->items[selectedIndex].flags & 0x8001) != 0) {
		this->moveDir(1);
	}

	int y, h;
	int iVar16, iVar14;
	int iVar18 = 0;
	int oldMenu = this->numItems;
	MenuSystem* pMVar15 = this;
	for (iVar16 = 0; iVar16 < this->numItems; iVar16++) {
		if ((pMVar15->items[iVar16].flags & 0x8000U) == 0) {
			iVar14 = this->getMenuItemHeight(iVar16);
			iVar18 = iVar18 + iVar14;
		}
	}

	//printf("menuRect[3] %d\n", app->canvas->menuRect[3]);
	//printf("iVar18 %d\n", iVar18);
	if (app->canvas->menuRect[3] < iVar18) {
		//printf("true\n");
		this->m_scrollBar->SetScrollBarImages(nullptr, nullptr, nullptr, nullptr);

		// Data-driven scrollbar configuration from theme
		const MenuTheme* sbTheme = getThemeForMenu(menu);
		const YAMLMenuDef* sbDef = getMenuDef(menu);
		ScrollbarStyle sbStyle = (sbTheme && sbTheme->scrollbar.style != SB_NONE) ? sbTheme->scrollbar.style : SB_DIAL;

		if (sbStyle == SB_BAR) {
			const ScrollbarConfig& sbc = sbTheme->scrollbar;
			h = sbc.barImg ? sbc.barImg->height : 0;
			y = app->canvas->menuRect[1] + (app->canvas->menuRect[3] - h >> 1);
			this->m_scrollBar->SetScrollBarImages(sbc.barImg, sbc.topImg, sbc.midImg, sbc.bottomImg);
			iVar16 = sbc.x;
			iVar14 = sbc.width;
			oldMenu = 0;
		} else {
			// Dial scrollbar (main menu style)
			if (sbDef && sbDef->scrollbarXOffset >= 0) {
				iVar16 = app->canvas->menuRect[0] + sbDef->scrollbarXOffset;
				y = app->canvas->menuRect[1] + sbDef->scrollbarYOffset;
			} else {
				int defX = (sbTheme) ? sbTheme->scrollbar.defaultX : 408;
				int defY = (sbTheme) ? sbTheme->scrollbar.defaultY : 81;
				iVar16 = defX;
				y = defY;
			}
			iVar14 = this->imgMenuDial->width;
			h = this->imgMenuDial->height;
			oldMenu = h + 0xf;
			if (-1 < h) {
				oldMenu = h;
			}
			oldMenu = oldMenu >> 4;
			this->isMainMenuScrollBar = true;
		}

		this->m_scrollBar->field_0x0_ = 1;
		this->m_scrollBar->barRect.Set(iVar16, y, iVar14, h);
		if (oldMenu == 0) {
			this->m_scrollBar->SetScrollBox
			(app->canvas->menuRect[0], app->canvas->menuRect[1], app->canvas->menuRect[2],
				app->canvas->menuRect[3], iVar18);
		}
		else {
			this->m_scrollBar->SetScrollBox
			(app->canvas->menuRect[0], app->canvas->menuRect[1], app->canvas->menuRect[2],
				app->canvas->menuRect[3], iVar18 - 50, oldMenu);
		}

		// [GEC] salva posiciones actuales
		//this->btnScroll->field_0x44_ = this->old_0x44;
		//this->btnScroll->field_0x48_ = this->old_0x48;

	}
	else {
		this->m_scrollBar->field_0x0_ = 0;
	}
	if (((menu < Menus::MENU_INGAME) && (this->type != 5 && menu != Menus::MENU_LEVEL_STATS)) && (menu != Menus::MENU_END_RANKING)) {
		this->isMainMenu = true;
	}
}

void MenuSystem::gotoMenu(int menu) {
	if (menu != this->menu) {
		this->pushMenu(this->menu, this->selectedIndex, this->m_scrollBar->field_0x44_, this->m_scrollBar->field_0x48_, this->scrollIndex); // [GEC]
	}

	this->setMenu(menu);
}

void MenuSystem::handleMenuEvents(int key, int keyAction) {


	key &= ~AVK_MENU_NUMBER; // [GEC]
	if (((key - AVK_0) < 10) && (this->enterDigit(key - AVK_0) != 0)) {
		return;
	}

	if (this->menu == Menus::MENU_COMIC_BOOK) { // [GEC]
		if (keyAction == Enums::ACTION_MENU) {
			this->app->comicBook->DeleteImages();
			this->back();
		} else {
			this->app->comicBook->handleComicBookEvents(key, keyAction);
		}
		return;
	}

	if (!this->changeValues) { // Old changeSfxVolume
		// [GEC] Evita cualquer movimiento si esta activo
		if (this->drawHelpText) {
			if ((keyAction == Enums::ACTION_MENU) ||
				(keyAction == Enums::ACTION_FIRE) ||
				(keyAction == Enums::ACTION_MENU_ITEM_INFO)) {
				this->drawHelpText = false;
				this->selectedHelpIndex = -1;
			}
			return;
		}
		else if ((keyAction == Enums::ACTION_MENU_ITEM_INFO)) {
			if (this->menu != Menus::MENU_MAIN_OPTIONS) {
				if (this->items[this->selectedIndex].helpField != MenuSystem::EMPTY_TEXT && (this->selectedIndex < this->numItems)) {
					this->soundClick();
					this->drawHelpText = true;
					this->selectedHelpIndex = this->selectedIndex;
				}
			}
		}

		if (keyAction == Enums::ACTION_UP) {
			if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) {
				if (app->player->inventory[24] >= app->vendingMachine->currentItemPrice * (app->vendingMachine->currentItemQuantity + 1)) {
					++app->vendingMachine->currentItemQuantity;
					this->setMenu(this->menu);
				}
			}
			else if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
				this->scrollUp();
			}
		}
		else if (keyAction == Enums::ACTION_DOWN) {
			if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) {
				if (app->vendingMachine->currentItemQuantity > 1) {
					--app->vendingMachine->currentItemQuantity;
					this->setMenu(this->menu);
				}
			}
			else if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
				this->scrollDown();
			}
		}
		else if (keyAction == Enums::ACTION_LEFT) {
			if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
				this->scrollPageUp();
			}
		}
		else if (keyAction == Enums::ACTION_RIGHT) {
			if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
				this->scrollPageDown();
			}
		}
		else if (keyAction == Enums::ACTION_FIRE) {
			this->select(this->selectedIndex);
		}
		else if (keyAction != Enums::ACTION_PASSTURN) {
			if (keyAction == Enums::ACTION_AUTOMAP) {
				/*if (Canvas.softKeyRightID == Text.STRINGID((short)0, (short)29)) {
					returnToGame();
				}
				else {
					doDetailsSelect();
				}*/

				if ((this->menu == Menus::MENU_LEVEL_STATS || this->menu == Menus::MENU_END_RANKING)) {
					this->select(this->selectedIndex);
				}
				else if (this->menu == Menus::MENU_SHOWDETAILS) {
					if (this->goBackToStation) {
						this->goBackToStation = 0;
						this->returnToGame();
					}
					else {
						this->doDetailsSelect();
					}
				}
				else if (this->menu == Menus::MENU_MAIN_MORE_GAMES) {
					this->doDetailsSelect();
				}
				else if (this->moreGamesPage == 3) {
					this->back();
				}	
			}
			else if (keyAction == Enums::ACTION_MENU) {
				if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
					/*if (this->menu == Menus::MENU_MAIN) {
						this->gotoMenu(13);
					}
					else */if (this->menu == Menus::MENU_VENDING_MACHINE) {
						app->vendingMachine->returnFromBuying();
					}
					else {
						this->back();
					}
				}
				else if (this->moreGamesPage == 0) {
					this->back();
				}
			}
			else if (keyAction == Enums::ACTION_BACK) {
				if (this->menu != Menus::MENU_ENABLE_SOUNDS) {
					if (this->menu == Menus::MENU_MAIN) {
						this->gotoMenu(Menus::MENU_MAIN_EXIT);
					}
					else if (this->menu == Menus::MENU_LEVEL_STATS || this->menu == Menus::MENU_END_ || this->menu == Menus::MENU_END_RANKING || this->menu == Menus::MENU_END_FINALQUIT) {
						this->select(this->selectedIndex);
					}
					else if (this->menu != Menus::MENU_MAIN_MORE_GAMES) {
						if (this->menu == Menus::MENU_VENDING_MACHINE) {
							app->vendingMachine->returnFromBuying();
						}
						else {
							this->back();
						}
					}
					else if (this->moreGamesPage == 0) {
						this->back();
					}
				}
			}
		}
	}
	else {
		if (keyAction == Enums::ACTION_RIGHT || keyAction == Enums::ACTION_LEFT) {
			this->setMenu(this->menu);
		}
		else if ((keyAction <= Enums::ACTION_FIRE || keyAction >= Enums::ACTION_BACK)) {
			this->select(this->selectedIndex);
		}
	}
}

void MenuSystem::select(int i) {


	//printf("select %d\n", i);

	this->cheatCombo = 0;
	this->menuMode = 0;
	if ((i < app->menuSystem->numItems) &&
		((this->items[i].flags & (Menus::ITEM_DISABLED | Menus::ITEM_DISABLEDTWO)) != 0)) {
		return;
	}

	if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) {
		app->sound->playSound(Sounds::getResIDByName(SoundName::VENDING_SALE), 0, 3, false);
	}

	int action = this->items[i].action;
	//printf("action %d\n", action);

	if(this->menu == Menus::MENU_END_RANKING) {
		app->sound->soundStop();
		app->canvas->setState(Canvas::ST_CREDITS);
		return;
	}

	if (this->menu == Menus::MENU_LEVEL_STATS) {
		if (!app->canvas->endingGame) {
			app->canvas->loadMap(this->LEVEL_STATS_nextMap, true, false);
			return;
		}
		this->gotoMenu(Menus::MENU_END_RANKING);
	}
	else if(this->menu != Menus::MENU_MAIN_MORE_GAMES) {
		if (this->type == 5 || this->type == 7) {
			if (this->menu == Menus::MENU_SHOWDETAILS && this->goBackToStation) {
				this->returnToGame();
				return;
			}
			this->back();
			return;
		}

		app->sound->playSound(Sounds::getResIDByName(SoundName::PISTOL), 0, 5, false); // [GEC] Pistol Sound
	}
	else {
		//SysIPhoneOpenURL("http://www.idsoftware.com/iphone-games/index.htm");
		return;
	}

	switch (action) {
		case 0: { // ACTION_NONE
			break;
		}
		case 1: { // ACTION_GOTO
			if (this->items[i].param == 75 || this->items[i].param == 73) {
				this->saveIndexes(4);
			}
			this->gotoMenu(this->items[i].param);
			break;
		}
		case 2: { // ACTION_BACK
			this->back();
			break;
		}
		case 3: { // ACTION_LOAD
			app->canvas->loadState(app->canvas->getRecentLoadType(), (short)3, (short)194);
			break;
		}
		case 4: { // ACTION_SAVE
			int n2 = 3;
			int n3;
			if (this->items[i].param == 1) {
				n3 = (n2 | 0x100);
			}
			else {
				n3 = (n2 | 0x80);
			}
			app->canvas->saveState(n3, (short)3, (short)196);
			break;
		}
		case 5:{ // ACTION_BACKTOMAIN
			app->canvas->backToMain(false);
			break;
		}
		case 6: { // ACTION_TOGSOUND
			app->sound->allowSounds = true;
			if (this->menu == Menus::MENU_ENABLE_SOUNDS) {
				app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_GENERAL), 1u, 3, 0);
				app->canvas->setState(Canvas::ST_INTRO_MOVIE);
				break;
			}

			app->canvas->vibrateEnabled ^= true;
			if (app->canvas->vibrateEnabled) {
				app->canvas->startShake(0, 0, 500);
			}
			this->items[0].textField2 = this->onOffValue(app->canvas->vibrateEnabled);

			this->setMenu(this->menu);
			break;
		}
		case 7: { // ACTION_NEWGAME
			app->canvas->m_controlAlpha = 0x32;
			if (app->game->hasSavedState()) {
				this->gotoMenu(Menus::MENU_MAIN_CONFIRMNEW);
			}
			else {
				this->gotoMenu(Menus::MENU_MAIN_DIFFICULTY);
			}
			break;
		}
		case 8: { // ACTION_EXIT
			app->shutdown();
			break;
		}
		case 9: { // ACTION_CHANGESTATE
			if (this->items[i].param == Canvas::ST_CREDITS) {
				if (app->canvas->loadMapID != 0) {
					app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_TITLE), 1, 3, false);
				}
			}
			app->canvas->setState(this->items[i].param);
			break;
		}
		case 10: { // ACTION_DIFFICULTY
			app->game->difficulty = (uint8_t)this->items[i].param;
			app->canvas->clearSoftKeys();
			this->startGame(true);
			break;
		}
		case 11: { // ACTION_RETURNTOGAME
			this->returnToGame();
			break;
		}
		case 12: { // ACTION_RESTARTLEVEL
			app->canvas->loadState(2, (short)3, (short)194);
			if (this->menu != Menus::MENU_INGAME_DEAD) {
				app->hud->addMessage((short)3, (short)195);
				break;
			}
			break;
		}
		case 13: { // ACTION_SAVEQUIT
			app->canvas->saveState(67, (short)3, (short)196);
			break;
		}
		case 14: { // ACTION_OFFERSUCCESS
			break;
		}
		case Menus::ACTION_CHANGESFXVOLUME: { // ACTION_CHANGESFXVOLUME
			this->changeSfxVolume = !this->changeSfxVolume;
			this->changeValues = !this->changeValues; //[GEC]
			this->setMenu(this->menu);
			break;
		}
		case 16: { // ACTION_SHOWDETAILS
			this->showDetailsMenu();
			break;
		}
		case 17: { // ACTION_CHANGEMAP
			app->game->spawnParam = 0;
			app->canvas->loadMap(1 + this->items[i].param, false, false);
			break;
		}
		case 18: { // ACTION_USEITEMWEAPON
			this->saveIndexes(1);
			if (i > 0) {
				app->player->selectWeapon((short)this->items[i].param);
				this->returnToGame();
				break;
			}
			break;
		}
		case 19: { // ACTION_SELECT_LANGUAGE
			app->localization->setLanguage(this->items[i].param);
			this->back();
			break;
		}
		case 20: // ACTION_USEITEMSYRING
		case 21: { // ACTION_USEITEMOTHER
			bool useItem = app->player->useItem((short)this->menuParam);
			if (app->game->animatingEffects != 0) {
				this->returnToGame();
				break;
			}
			app->game->snapMonsters(true);
			app->game->snapLerpSprites(-1);
			if (app->canvas->state != Canvas::ST_MENU) {
				break;
			}
			if (useItem) {
				this->back();
				break;
			}
			if (this->menuParam == 22) {
				this->gotoMenu(Menus::MENU_ITEMS_HOLY_WATER_MAX);
				break;
			}
			if (this->menuParam >= 16 && this->menuParam < 18) {
				this->gotoMenu(Menus::MENU_ITEMS_HEALTHMSG);
				break;
			}
			if (this->menuParam >= 11 && this->menuParam < 13) {
				this->gotoMenu(Menus::MENU_ITEMS_ARMORMSG);
				break;
			}
			this->gotoMenu(Menus::MENU_ITEMS_SYRINGEMSG);
			break;
		}
		case 22: { // ACTION_CONTINUE
			this->startGame(false);
			break;
		}
		case 23: { // ACTION_MAIN_SPECIAL
			app->canvas->backToMain(false);
			break;
		}
		case 24: { // ACTION_CONFIRMUSE
			if (this->menu == Menus::MENU_ITEMS_DRINKS) {
				this->saveIndexes(2);
			}
			else if (this->menu == Menus::MENU_ITEMS) {
				this->saveIndexes(4);
			}
			this->menuParam = this->items[i].param;
			this->gotoMenu(Menus::MENU_ITEMS_CONFIRM);
			break;
		}
		case 25: { // ACTION_SAVEEXIT
			app->canvas->saveState(7, (short)3, (short)196);
			break;
		}
		case 26: { // ACTION_BACKTWO
			this->popMenu(this->poppedIdx, &this->m_scrollBar->field_0x44_, &this->m_scrollBar->field_0x48_, &this->scrollIndex);
			this->back();
			break;
		}
		case 27: { // ACTION_MINIGAME
			if (this->items[i].param == 2) {
				app->sound->soundStop();
				app->hackingGame->playFromMainMenu();
			}
			else if (this->items[i].param == 0) {
				app->sound->soundStop();
				app->sentryBotGame->playFromMainMenu();
			}
			else if (this->items[i].param == 4) {
				app->sound->soundStop();
				app->vendingMachine->playFromMainMenu();
			}
			break;
		}
		case 28: { // ACTION_CONFIRMBUY
			this->menuParam = this->items[i].param;
			app->vendingMachine->currentItemQuantity = 1;
			if (this->menu == Menus::MENU_VENDING_MACHINE_LAST) {
				this->setMenu(Menus::MENU_VENDING_MACHINE_CONFIRM);
			}
			else {
				this->gotoMenu(Menus::MENU_VENDING_MACHINE_CONFIRM);
			}
			break;
		}
		case 29: { // ACTION_BUYDRINK
			if (app->vendingMachine->buyDrink(this->menuParam)) {
				this->back();
			}
			else {
				this->setMenu(Menus::MENU_VENDING_MACHINE_CANT_BUY);
			}
			break;
		}
		case 30: { // ACTION_BUYSNACK
			if (app->player->inventory[24] < app->vendingMachine->currentItemPrice * app->vendingMachine->currentItemQuantity) {
				this->setMenu(Menus::MENU_VENDING_MACHINE_CANT_BUY);
			}
			else {
				for (int i = 0; i < app->vendingMachine->currentItemQuantity; ++i) {
					app->player->inventory[24] -= (short)app->vendingMachine->getSnackPrice();
					app->player->give(0, 16, 1, false);
				}
				this->back();
			}
			break;
		}
		case 33: { // ACTION_RETURN_TO_PLAYER
			app->player->familiarReturnsToPlayer(false);
			this->returnToGame();
			break;
		}
		case 35: { // ACTION_FLIP_CONTROLS
			app->canvas->flipControls();
			this->setMenu(this->menu);
			break;
		}
		case 36: { // ACTION_CONTROL_LAYOUT
			++app->canvas->m_controlLayout;
			if (app->canvas->m_controlLayout > 2) {
				app->canvas->m_controlLayout = 0;
			}
			app->canvas->setControlLayout();
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_CHANGEMUSICVOLUME: { // [GEC]
			this->changeMusicVolume = !this->changeMusicVolume;
			this->changeValues = !this->changeValues;
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_CHANGEALPHA: { // [GEC]
			this->changeButtonsAlpha = !this->changeButtonsAlpha;
			this->changeValues = !this->changeValues;
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_CHANGE_VID_MODE: { // [GEC]
			if (++CAppContainer::getInstance()->sdlGL->windowMode > 2) {
				CAppContainer::getInstance()->sdlGL->windowMode = 0;
			}
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_TOG_VSYNC: { // [GEC]
			CAppContainer::getInstance()->sdlGL->vSync = !CAppContainer::getInstance()->sdlGL->vSync;
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_CHANGE_RESOLUTION: { // [GEC]
			if (++CAppContainer::getInstance()->sdlGL->resolutionIndex >= 18) {
				CAppContainer::getInstance()->sdlGL->resolutionIndex = 0;
			}
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_APPLY_CHANGES: { // [GEC]
			CAppContainer::getInstance()->sdlGL->updateVideo();
			app->game->saveConfig();
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_SET_BINDING: { // [GEC]	
			this->setBinding = !this->setBinding;
			this->changeValues = !this->changeValues;
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_DEFAULT_BINDINGS: { // [GEC]
			// Apply changes to default
			std::memcpy(keyMappingTemp, keyMappingDefault, sizeof(keyMapping));
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_TOG_VIBRATION: { // [GEC]
			app->canvas->vibrateEnabled ^= true;
			if (app->canvas->vibrateEnabled) {
				app->canvas->startShake(0, 0, 500);
			}
			this->items[(this->menu == Menus::MENU_MAIN_CONTROLLER) ? 2 : 1].textField2 = this->onOffValue(app->canvas->vibrateEnabled);
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_CHANGE_VIBRATION_INTENSITY: { // [GEC]
			this->changeVibrationIntensity = !this->changeVibrationIntensity;
			this->changeValues = !this->changeValues;
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_CHANGE_DEADZONE: { // [GEC]
			this->changeDeadzone = !this->changeDeadzone;
			this->changeValues = !this->changeValues;
			this->setMenu(this->menu);
			break;
		}

		case Menus::ACTION_TOG_TINYGL: { // [GEC]
			Canvas* canvas = this->app->canvas.get();
			TinyGL* tinyGL = this->app->tinyGL.get();
			_glesObj->isInit = !_glesObj->isInit;

			if (canvas->state == Canvas::ST_CAMERA) {
				tinyGL->setViewport(canvas->cinRect[0], canvas->cinRect[1], canvas->cinRect[2], canvas->cinRect[3]);
			}
			else {
				tinyGL->resetViewPort();
			}

			this->setMenu(this->menu);
			break;
		}

		case 102: { // ACTION_GIVEALL
			app->player->giveAll();
			break;
		}
		case 103: { // ACTION_GIVEMAP
			app->game->givemap(0, 0, 32, 32);
			break;
		}
		case 104: { // ACTION_NOCLIP
			app->player->noclip = !app->player->noclip;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 105: { // ACTION_DISABLEAI
			app->game->disableAI = !app->game->disableAI;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 106: { // ACTION_NOHELP
			app->player->enableHelp = !app->player->enableHelp;
			app->game->saveConfig();
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 107: { // ACTION_GODMODE
			app->player->god = !app->player->god;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 108: { // ACTION_SHOWLOCATION
			app->canvas->showLocation = !app->canvas->showLocation;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 109: { // ACTION_RFRAMES
			int animFrames = app->canvas->animFrames + 1;
			if (animFrames > 15) {
				animFrames = 2;
			}
			app->canvas->setAnimFrames(animFrames);
			app->game->saveConfig();
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 110: { // ACTION_RSPEEDS
			app->canvas->showSpeeds = !app->canvas->showSpeeds;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 111: { // ACTION_RSKIPFLATS
			app->render->skipFlats = !app->render->skipFlats;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 112: { // ACTION_RSKIPCULL
			app->render->skipCull = !app->render->skipCull;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 114: { // ACTION_RSKIPBSP
			app->render->skipBSP = !app->render->skipBSP;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 115: { // ACTION_RSKIPLINES
			app->render->skipLines = !app->render->skipLines;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 116: { // ACTION_RSKIPSPRITES
			app->render->skipSprites = !app->render->skipSprites;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 117: { // ACTION_RONLYRENDER
			app->canvas->renderOnly = !app->canvas->renderOnly;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 118: { // ACTION_RSKIPDECALS
			app->render->skipDecals = !app->render->skipDecals;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 119: { // ACTION_RSKIP2DSTRETCH
			app->render->skip2DStretch = !app->render->skip2DStretch;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 120: { // ACTION_DRIVING_MODE
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 121: { // ACTION_RENDER_MODE
			if (app->render->renderMode == 0) {
				app->render->renderMode = 63;
			}
			else {
				app->render->renderMode >>= 1;
			}
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 122: { // ACTION_EQUIPFORMAP
			app->player->equipForLevel(app->canvas->loadMapID);
			break;
		}
		case 123: { // ACTION_ONESHOT
			app->combat->oneShotCheat = !app->combat->oneShotCheat;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 124: { // ACTION_DEBUG_FONT
			app->canvas->enqueueHelpDialog((short)3, (short)342, (uint8_t)(-1));
			this->returnToGame();
			break;
		}
		case 125: { // ACTION_SYS_TEST
			this->systemTest(this->items[i].param + 332);
			break;
		}
		case 126: { // ACTION_SKIP_MINIGAMES
			app->game->skipMinigames = !app->game->skipMinigames;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		case 127: { // ACTION_SHOW_HEAP
			app->canvas->showFreeHeap = !app->canvas->showFreeHeap;
			this->setMenu(Menus::MENU_DEBUG);
			break;
		}
		default:
			app->Error("Unhandled Menu Action: %i", this->items[i].action);
			break;
	}
}


void MenuSystem::infiniteLoop() {
	for (int i = 1024; ; i += 4) {
		i = 0;
	}
}

int MenuSystem::infiniteRecursion(int* array) {
	int n = 0;
	++array[n];
	if (array[0] > 0) {
		array[0] = 0;
		return infiniteRecursion(array);
	}
	return array[0];
}

void MenuSystem::systemTest(int sysType) {

	int i = 0;
	int j = 0;

	switch (sysType)
	{
		case 332: { // SYS_LOOP
			//this->infiniteLoop();
			break;
		}
		case 333: { // SYS_RECURSION
			/*if (i + 1 > 0) {
				i = 0;
				this->infiniteRecursion(&i);
			}*/
			break;
		}
		case 334: { // SYS_ERR
			/*for (j = 0; j != 1024; ++j) {
				//app->setTimerTest();
			}
			do
			{
				//app->clearSetTimerTest();
				--j;
			} while (j);*/
			break;
		}
		case 338: { // SYS_SOUND
			app->canvas->sysSoundDelayTime = app->canvas->sysSoundDelayTime == 0;
			break;
		}
		case 339: { // SYS_SOUND_DELAY
			app->canvas->sysSoundDelayTime = 100 * ((app->canvas->sysSoundDelayTime + 100) / 100) % 2000;
			break;
		}
		case 341: { // SYS_ADV_TIME
			app->sysAdvTime = (app->sysAdvTime + 5) % 35;
			break;
		}
	}
}

void MenuSystem::startGame(bool b) {


	if (this->background != this->imgMainBG) {
		this->background->~Image();
	}

	this->imgMainBG->~Image();

	this->background = nullptr;
	this->imgMainBG = nullptr;

	app->sound->soundStop();

	if (b) {
		app->canvas->setLoadingBarText(-1, -1);
		app->game->removeState(true);

		app->game->activeLoadType = 0;
		app->canvas->loadType = 0;
		app->canvas->loadMapID = 0;
		app->canvas->lastMapID = 0;
		app->player->reset();
		app->player->totalDeaths = 0;
		app->player->currentLevelDeaths = 0;
		app->player->helpBitmask = 0;
		app->player->invHelpBitmask = 0;
		app->player->ammoHelpBitmask = 0;
		app->player->weaponHelpBitmask = 0;
		app->player->armorHelpBitmask = 0;
		app->player->currentGrades = 0;
		app->player->ce->weapon = -1;
		app->canvas->clearEvents(1);
		app->canvas->setState(Canvas::ST_CHARACTER_SELECTION);
	}
	else {
		app->canvas->loadState(app->canvas->getRecentLoadType(), 3, 194);
	}
}

void MenuSystem::SetYESNO(short i, int i2, int i3, int i4) {

	Text* text;

	text = app->localization->getLargeBuffer();
	app->localization->composeText(3, i, text);
	this->SetYESNO(text, i2, i3, i4, 2, 0);
	text->dispose();
}

void MenuSystem::SetYESNO(short i, int i2, int i3, int i4, int i5, int i6) {

	Text* text;

	text = app->localization->getLargeBuffer();
	app->localization->composeText(3, i, text);
	this->SetYESNO(text, i2, i3, i4, i5, i6);
	text->dispose();
}

void MenuSystem::SetYESNO(Text* text, int i, int i2, int i3, int i4, int i5) {


	app->localization->resetTextArgs();
	if (text->findFirstOf('\n', 0) >= 0) {
		int n6;
		int first;
		for (n6 = 0; (first = text->findFirstOf('\n', n6)) >= 0; n6 = first + 1) {
			app->localization->addTextArg(text, n6, first);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
		}
		app->localization->addTextArg(text, n6, text->length());
		this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
	}
	else {
		app->localization->addTextArg(text);
		this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
	}

	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 9, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::YES_LABEL), MenuSystem::EMPTY_TEXT, 8, i2, i3, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::NO_LABEL), MenuSystem::EMPTY_TEXT, 8, i4, i5, MenuSystem::EMPTY_TEXT);

	if (i == 1) {
		this->selectedIndex = this->numItems - 2;
	}
	else {
		this->selectedIndex = this->numItems - 1;
	}
}

void MenuSystem::LoadHelpResource(short i) {

	Text* text;

	this->scrollIndex = 0;
	this->selectedIndex = 0;
	text = app->localization->getLargeBuffer();

	app->localization->loadText(2);
	app->localization->composeText(2, i, text);
	app->localization->unloadText(2);

	text->wrapText((this->menu >= Menus::MENU_INGAME) ? app->canvas->ingameScrollWithBarMaxChars : 35);

	this->LoadHelpItems(text, 0);

	int h = 0;
	for(int j = 0; j < this->numItems; j++) {
		if (!(this->items[0].flags & 0x8000)) {
			h += this->getMenuItemHeight(j);
		}
	}

	if ((app->canvas->menuRect[3] < h) && (this->menu >= Menus::MENU_INGAME)) {
		this->numItems = 0;
		app->localization->resetTextArgs();
		text->dispose();

		text = app->localization->getLargeBuffer();
		app->localization->loadText(2);
		app->localization->composeText(2, i, text);
		app->localization->unloadText(2);

		app->canvas->menuRect[2] -= 27;
		text->wrapText(app->canvas->menuRect[2] / Applet::CHAR_SPACING[app->fontType]);
		this->LoadHelpItems(text, 0);
	}

	app->localization->unloadText(2);
	text->dispose();
}

void MenuSystem::FillRanking() {

	int max = std::max(app->player->finalCurrentGrade(), 1);
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0, 0, 0, MenuSystem::EMPTY_TEXT);
	if (app->game->difficulty == Enums::DIFFICULTY_EASY) {
		this->addItem(Localization::STRINGID((short)3, (short)200), MenuSystem::EMPTY_TEXT, 72, 0, 0, MenuSystem::EMPTY_TEXT);
	}
	else if (app->game->difficulty == Enums::DIFFICULTY_NORMAL) {
		this->addItem(Localization::STRINGID((short)3, (short)201), MenuSystem::EMPTY_TEXT, 72, 0, 0, MenuSystem::EMPTY_TEXT);
	}
	else {
		this->addItem(Localization::STRINGID((short)3, (short)202), MenuSystem::EMPTY_TEXT, 72, 0, 0, MenuSystem::EMPTY_TEXT);
	}
	short n = 0;
	short n2 = 0;
	switch (max) {
		case 6: {
			n = 28;
			n2 = 29;
			break;
		}
		case 5: {
			n = 26;
			n2 = 27;
			break;
		}
		case 4: {
			n = 24;
			n2 = 25;
			break;
		}
		case 3: {
			if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE) {
				n = 24;
				n2 = 25;
				break;
			}
			n = 22;
			n2 = 23;
			break;
		}
		default: {
			if (app->game->difficulty == Enums::DIFFICULTY_NIGHTMARE) {
				n = 24;
				n2 = 25;
				break;
			}
			if (app->game->difficulty == Enums::DIFFICULTY_NORMAL) {
				n = 22;
				n2 = 23;
				break;
			}
			n = 20;
			n2 = 21;
			break;
		}
	}
	app->localization->resetTextArgs();
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID((short)3, (short)203), MenuSystem::EMPTY_TEXT, 8, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID((short)3, app->player->gradeToString(max)), MenuSystem::EMPTY_TEXT, 8, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID((short)3, (short)204), MenuSystem::EMPTY_TEXT, 8, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(Localization::STRINGID((short)0, n), MenuSystem::EMPTY_TEXT, 8, 0, 0, MenuSystem::EMPTY_TEXT);
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
	Text* largeBuffer = app->localization->getLargeBuffer();
	app->localization->composeText((short)0, n2, largeBuffer);
	largeBuffer->wrapText(app->canvas->menuRect[2] / Applet::CHAR_SPACING[app->fontType]);
	this->LoadHelpItems(largeBuffer, 8);
	largeBuffer->dispose();
}

void MenuSystem::LoadNotebook() {

	this->detailsHelpText = app->localization->getLargeBuffer();
	this->type = 7;
	this->items[this->numItems++].Set(app->render->mapNameField, MenuSystem::EMPTY_TEXT, 9);
	this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 73);
	for (int i = 0; i < app->player->numNotebookIndexes; ++i) {
		this->detailsHelpText->setLength(0);
		int n = 0;
		app->localization->composeText(app->canvas->loadMapStringID, app->player->notebookIndexes[i], this->detailsHelpText);
		if (app->player->isQuestFailed(i)) {
			this->items[this->numItems++].Set(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)227), MenuSystem::EMPTY_TEXT, 0);
			n = 4;
		}
		else if (app->player->isQuestDone(i)) {
			this->detailsHelpText->insert('\x87', 0);
			this->detailsHelpText->insert(" ", 1);
		}
		this->detailsHelpText->wrapText(app->canvas->ingameScrollWithBarMaxChars);
		this->LoadHelpItems(this->detailsHelpText, n);
		this->items[this->numItems++].Set(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 0);
	}
	int n2 = (app->canvas->displayRect[3] - 26) / 16;
	int index = this->numItems - n2;
	this->scrollIndex = index;
	this->selectedIndex = index;
	if (index < 0) {
		this->selectedIndex = 0;
		this->scrollIndex = 0;
	}

	this->detailsHelpText->dispose();
}

void MenuSystem::LoadHelpItems(Text* text, int i) {


	int n2 = 2;
	int n3;
	int first;
	for (n3 = 0; (first = text->findFirstOf('|', n3)) >= 0; n3 = first + 1) {
		if (first > n3 && text->charAt(n3) == '#') {
			app->localization->addTextArg(text, n3 + 1, first);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, n2 | 0x8 | 0x40 | i, 0, 0, MenuSystem::EMPTY_TEXT);
		}
		else {
			app->localization->addTextArg(text, n3, first);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, n2 | i, 0, 0, MenuSystem::EMPTY_TEXT);
		}
	}
	int length = text->length();
	if (n3 < length) {
		if (text->charAt(n3) == '#') {
			app->localization->addTextArg(text, n3, length);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, n2 | 0x8 | 0x40 | 0x40 | i, 0, 0, MenuSystem::EMPTY_TEXT);
		}
		else {
			app->localization->addTextArg(text, n3, text->length());
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, n2 | i, 0, 0, MenuSystem::EMPTY_TEXT);
		}
	}
}

void MenuSystem::buildFraction(int i, int i2, Text* text) {
	text->setLength(0);
	if (i < 0) {
		text->append('-');
	}
	text->append(i);
	text->append("/");
	if (i2 < 0) {
		text->append('-');
	}
	text->append(i2);
}

void MenuSystem::buildModStat(int i, int i2, Text* text) {
	text->setLength(0);
	text->append(i);
	if (i2 == 0) {
		return;
	}
	text->append((i2 > 0) ? "(+" : "(-");
	text->append(i2);
	text->append(')');
}

void MenuSystem::buildLevelGrades(Text* text) {

	int n = 30;
	app->localization->resetTextArgs();
	text->setLength(0);
	Text* smallBuffer = app->localization->getSmallBuffer();
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, (short)47, smallBuffer);
	smallBuffer->dehyphenate();
	for (int i = text->length() + smallBuffer->length(); i < n; ++i) {
		text->append('\x80');
	}
	text->append(smallBuffer);
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, (short)48, smallBuffer);
	smallBuffer->dehyphenate();
	for (int j = text->length() + smallBuffer->length(); j < (n + 8); ++j) {
		text->append('\x80');
	}
	text->append(smallBuffer);
	smallBuffer->dispose();
	this->addItem(MenuSystem::EMPTY_TEXT, MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
	app->localization->addTextArg(text);
	this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
	for (int k = 0; k < app->game->levelNamesCount - 1; ++k) {
		int n2 = k + 1;
		if (app->player->getCurrentGrade(n2) != 0 || app->player->getBestGrade(n2) != 0 || n2 <= app->canvas->loadMapID) {
			this->buildLevelGrade(n2, text, n, n + 8);
			app->localization->addTextArg(text);
			this->addItem(this->getLastArgString(), MenuSystem::EMPTY_TEXT, 1, 0, 0, MenuSystem::EMPTY_TEXT);
		}
	}
}

void MenuSystem::buildLevelGrade(int i, Text* text, int i2, int i3) {

	int n4 = (i == app->canvas->loadMapID) ? app->player->levelGrade(false) : app->player->getCurrentGrade(i);
	int bestGrade = app->player->getBestGrade(i);
	text->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->game->levelNames[i - 1], text);
	text->dehyphenate();
	Text* smallBuffer = app->localization->getSmallBuffer();
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(n4), smallBuffer);
	smallBuffer->dehyphenate();
	int length = smallBuffer->length();
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(n4), smallBuffer);
	for (int i = text->length() + length; i < i2; ++i) {
		text->append(" ");
	}
	text->append(smallBuffer);
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(bestGrade), smallBuffer);
	smallBuffer->dehyphenate();
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(bestGrade), smallBuffer);
	for (int j = i2 + 1; j < i3; ++j) {
		text->append(" ");
	}
	text->append(smallBuffer);
	smallBuffer->dispose();
}

void MenuSystem::fillStatus(bool b, bool b2, bool b3) {

	app->localization->resetTextArgs();
	Text* largeBuffer = app->localization->getLargeBuffer();
	if (b2) {
		this->buildFraction(app->player->ce->getStat(0), app->player->ce->getStat(1), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildFraction(app->player->ce->getStat(2), 200, largeBuffer);
		app->localization->addTextArg(largeBuffer);
		app->localization->addTextArg(app->player->level);
		app->localization->addTextArg(app->player->currentXP);
		app->localization->addTextArg(app->player->nextLevelXP);
		this->buildModStat(app->player->baseCe->getStat(3), app->player->ce->getStat(3) - app->player->baseCe->getStat(3), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildModStat(app->player->baseCe->getStat(4), app->player->ce->getStat(4) - app->player->baseCe->getStat(4), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildModStat(app->player->baseCe->getStat(5), app->player->ce->getStat(5) - app->player->baseCe->getStat(5), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildModStat(app->player->baseCe->getStat(6), app->player->ce->getStat(6) - app->player->baseCe->getStat(6), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildModStat(app->player->baseCe->getStat(7), app->player->ce->getStat(7) - app->player->baseCe->getStat(7), largeBuffer);
		app->localization->addTextArg(largeBuffer);
	}
	if (b) {
		if (b && !b2 && !b3) {
			app->localization->addTextArg(Strings::FILE_MENUSTRINGS, (short)(app->render->mapNameField & 0x3FF));
		}
		app->player->formatTime(app->game->totalLevelTime + (app->gameTime - app->game->curLevelTime), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		app->player->fillMonsterStats();
		this->buildFraction(app->game->mapSecretsFound, app->game->totalSecrets, largeBuffer);
		app->localization->addTextArg(largeBuffer);
		this->buildFraction(app->player->monsterStats[0], app->player->monsterStats[1], largeBuffer);
		app->localization->addTextArg(largeBuffer);
		app->localization->addTextArg(app->player->moves);
		app->localization->addTextArg(app->player->currentLevelDeaths);
		Text* smallBuffer = app->localization->getSmallBuffer();
		app->localization->composeText(Strings::FILE_MENUSTRINGS, app->player->gradeToString(app->player->levelGrade(false)), smallBuffer);
		app->localization->addTextArg(smallBuffer);
		smallBuffer->dispose();
	}
	if (b3) {
		largeBuffer->setLength(0);
		app->player->formatTime(app->player->totalTime + (app->gameTime - app->player->playTime), largeBuffer);
		app->localization->addTextArg(largeBuffer);
		app->localization->addTextArg(app->player->totalMoves);
		app->localization->addTextArg(app->player->totalDeaths);
		app->localization->addTextArg(app->player->counters[6]);
		app->localization->addTextArg(app->player->counters[7]);
		app->localization->addTextArg(app->player->counters[1]);
		app->localization->addTextArg(app->player->counters[2]);
	}
	largeBuffer->dispose();
}

void MenuSystem::saveIndexes(int i) {
	this->indexes[i * 2] = this->selectedIndex;
	this->indexes[i * 2 + 1] = this->scrollIndex;
}

void MenuSystem::loadIndexes(int i) {
	this->selectedIndex = this->indexes[i * 2];
	this->scrollIndex = this->indexes[i * 2 + 1];
}

void MenuSystem::showDetailsMenu() {
	// Pendiente
}

void MenuSystem::addItem(int textField, int textField2, int flags, int action, int param, int helpField) {

	if (this->numItems == 50) {
		app->Error(100); // ERR_MAXMENUITEMS
	}
	this->items[this->numItems++].Set(textField, textField2, flags, action, param, helpField);
}

void MenuSystem::loadMenuItems(int menu, int begItem, int numItems) {


	int length = this->menuDataCount;
	for (int j = 0; j < length; ++j) {
		if ((this->menuData[j] & 0xFF) == menu) {
			int n4 = this->menuData[j];
			this->type = (n4 & 0xFF000000) >> 24;
			int n5 = 0;
			if (j != 0) {
				n5 = (this->menuData[j - 1] & 0xFFFF00) >> 8;
			}
			if (numItems == -1) {
				numItems = (((n4 & 0xFFFF00) >> 8) - n5) / 2;
				numItems -= begItem;
			}
			int n6 = n5 + begItem * 2;
			for (int k = 0; k < numItems; ++k) {
				int n7 = this->menuItems[n6 + 0];
				int n8 = this->menuItems[n6 + 1];
				this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (n7 >> 16)),
					Localization::STRINGID(Strings::FILE_MENUSTRINGS, 0), (n7 & 0xFFFF), ((n8 & 0xFF00) >> 8), (n8 & 0xFF), 
					Localization::STRINGID(Strings::FILE_MENUSTRINGS, ((n8 & 0xFFFF0000) >> 16)));
				n6 += 2;
			}
			return;
		}
	}

	app->Error(29); // ERR_LOADMENUITEMS
}

int MenuSystem::onOffValue(bool b) {
	return Localization::STRINGID(Strings::FILE_MENUSTRINGS, (b ? MenuStrings::ON_LABEL : MenuStrings::OFF_LABEL));
}

void MenuSystem::leaveOptionsMenu(void) {


	//app->canvas->pacifierX = app->canvas->SCR_CX - 66;
	//app->canvas->repaintFlags |= Canvas::REPAINT_CLEAR;
	//app->canvas->setLoadingBarText(3, 0xe4);
	//app->canvas->updateLoadingBar(true);
	app->game->saveConfig();
}

int MenuSystem::getStackCount() {
	return this->stackCount;
}

void MenuSystem::clearStack() {
	this->stackCount = 0;
}

void MenuSystem::pushMenu(int i, int i2, int Y1, int Y2, int index2) {

	if (this->stackCount + 1 >= 10) {
		app->Error("Menu stack is full.");
	}
	this->menuIdxStack[this->stackCount] = i2;
	this->scrollY1Stack[this->stackCount] = Y1; // [GEC]
	this->scrollY2Stack[this->stackCount] = Y2; // [GEC]
	this->scrollI2Stack[this->stackCount] = index2; // [GEC]
	this->menuStack[this->stackCount++] = i;
}

int MenuSystem::popMenu(int* array, int* Y1, int* Y2, int *index2) {

	if (this->stackCount - 1 < 0) {
		app->Error("Menu stack is empty");
	}
	array[0] = this->menuIdxStack[this->stackCount - 1];
	*Y1 = this->scrollY1Stack[this->stackCount - 1]; // [GEC]
	*Y2 = this->scrollY2Stack[this->stackCount - 1]; // [GEC]
	*index2 = this->scrollI2Stack[this->stackCount - 1]; // [GEC]
	return this->menuStack[--this->stackCount];
}

int MenuSystem::peekMenu() {
	if (this->stackCount - 1 < 0) {
		return -1;
	}
	return this->menuStack[this->stackCount - 1];
}

int MenuSystem::getLastArgString() {


	if (app->localization->numTextArgs > 0) {
		return Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)(MenuStrings::ARGUMENT1 + (app->localization->numTextArgs - 1)));
	}

	return MenuSystem::EMPTY_TEXT;
}

void MenuSystem::fillVendingMachineSnacks(int i, Text* text) {


	int n2 = 1;
	if (i >= 1 && i <= 3) {
		n2 = 0;
	}
	else if (i >= 7 && i <= 9) {
		n2 = 2;
	}
	if (n2 == 0) {
		text->setLength(0);
		text->append(app->vendingMachine->getSnackPrice());
		EntityDef* find = app->entityDefManager->find(6, 0, 16);
		app->localization->addTextArg(text);
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)358), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)358), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)359), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)359), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)360), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)360), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)361), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)361), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)362), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)362), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)363), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)363), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)364), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)364), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)365), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)365), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)366), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)366), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)367), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)367), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)368), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)368), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
	}
	else if (n2 == 1) {
		text->setLength(0);
		text->append(app->vendingMachine->getSnackPrice());
		EntityDef* find = app->entityDefManager->find(6, 0, 16);
		app->localization->addTextArg(text);
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)370), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)370), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)371), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)371), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)372), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)372), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)373), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)373), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)374), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)374), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)375), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)375), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)376), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)376), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)377), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)377), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)378), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)378), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)379), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)379), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)368), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)368), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
	}
	else if (n2 == 2) {
		text->setLength(0);
		text->append(app->vendingMachine->getSnackPrice());
		EntityDef* find = app->entityDefManager->find(6, 0, 16);
		app->localization->addTextArg(text);
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)380), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)380), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)381), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)381), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)382), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)382), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)383), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)383), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)384), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)384), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)385), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)385), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)386), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)386), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)387), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)387), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)388), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)388), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)389), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)389), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)390), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)390), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
		this->addItem(Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), this->getLastArgString(), 0, 28, Localization::STRINGID(Strings::FILE_MENUSTRINGS, (short)369), Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, find->description));
	}
}

//--------------------------------------------------------------------------

const MenuSystem::MenuTheme* MenuSystem::getThemeForMenu(int menuId) const {
	const YAMLMenuDef* def = getMenuDef(menuId);
	if (def && def->hasTheme) return &def->resolvedTheme;
	return nullptr;
}

const MenuSystem::YAMLMenuDef* MenuSystem::getMenuDef(int menuId) const {
	auto it = yamlMenuById.find(menuId);
	if (it != yamlMenuById.end() && it->second >= 0 && it->second < (int)yamlMenuDefs.size()) {
		return &yamlMenuDefs[it->second];
	}
	return nullptr;
}

Image* MenuSystem::resolveMenuImage(const std::string& name) const {
	if (name == "main_bg") return this->imgMainBG;
	if (name == "logo") return this->imgLogo;
	// Add more dynamic image mappings here as needed
	return nullptr;
}

int MenuSystem::resolveLayoutValue(LayoutValueMode mode, int literal, int w) const {
	switch (mode) {
		case LVM_LITERAL: return literal;
		case LVM_CENTER: return (Applet::IOS_WIDTH - w) / 2;
		case LVM_BTN_WIDTH:
			return this->imgMenuBtnBackground ? this->imgMenuBtnBackground->width : 162;
		case LVM_BTN_H_X4:
			return this->imgMenuBtnBackground ? this->imgMenuBtnBackground->height * 4 : 184;
		case LVM_BTN_H_X5:
			return this->imgMenuBtnBackground ? this->imgMenuBtnBackground->height * 5 : 230;
		case LVM_PANEL_H:
			return 320 - (this->imgGameMenuPanelbottom ? this->imgGameMenuPanelbottom->height : 0);
		case LVM_YESNO_H:
			return this->imgMenuYesNoBOX ? this->imgMenuYesNoBOX->height : 100;
		case LVM_LANG_H:
			return this->imgMenuLanguageBOX ? this->imgMenuLanguageBOX->height : 200;
		case LVM_LOGO_BOX: {
			int imgH = this->imgLogo ? (Applet::IOS_HEIGHT - this->imgLogo->height) : Applet::IOS_HEIGHT;
			int logoY = this->imgLogo ? this->imgLogo->height : 0;
			int boxH = this->imgMenuMainBOX ? this->imgMenuMainBOX->height : 100;
			return logoY + ((imgH - boxH) / 2) - 2;
		}
		case LVM_LOGO_YESNO: {
			int imgH = this->imgLogo ? (Applet::IOS_HEIGHT - this->imgLogo->height) : Applet::IOS_HEIGHT;
			int logoY = this->imgLogo ? this->imgLogo->height : 0;
			int boxH = this->imgMenuYesNoBOX ? this->imgMenuYesNoBOX->height : 100;
			return logoY + ((imgH - boxH) / 2);
		}
		case LVM_LOGO_DIFF: {
			int imgH = this->imgLogo ? (Applet::IOS_HEIGHT - this->imgLogo->height) : Applet::IOS_HEIGHT;
			int logoY = this->imgLogo ? this->imgLogo->height : 0;
			int boxH = this->imgMenuChooseDIFFBOX ? this->imgMenuChooseDIFFBOX->height : 100;
			return logoY + ((imgH - boxH) / 2);
		}
		case LVM_LOGO_LANG: {
			int imgH = this->imgLogo ? (Applet::IOS_HEIGHT - this->imgLogo->height) : Applet::IOS_HEIGHT;
			int logoY = this->imgLogo ? this->imgLogo->height : 0;
			int boxH = this->imgMenuLanguageBOX ? this->imgMenuLanguageBOX->height : 200;
			return logoY + ((imgH - boxH) / 2);
		}
		case LVM_LOGO_15: {
			int logoY = this->imgLogo ? this->imgLogo->height : 0;
			return logoY + 15;
		}
	}
	return literal;
}

void MenuSystem::setMenuSettings() {

	fmButton* button;
	int x, y, w, h;

	this->menuItem_height = 46;
	this->menuItem_width = 162;
	this->isMainMenuScrollBar = false;
	this->isMainMenu = false;
	this->menuItem_fontPaddingBottom = 0;
	this->menuItem_paddingBottom = 0;

	this->imgMenuBtnBackground = this->imgMenuButtonBackground; // [GEC] Default
	this->imgMenuBtnBackgroundOn = this->imgMenuButtonBackgroundOn;  // [GEC] Default

	// Resolve theme for current menu (overrides defaults if available)
	const MenuTheme* theme = getThemeForMenu(this->menu);
	if (theme && theme->btnImage) {
		this->imgMenuBtnBackground = theme->btnImage;
		this->imgMenuBtnBackgroundOn = theme->btnHighlightImage ? theme->btnHighlightImage : theme->btnImage;
	}

	app->canvas->setMenuDimentions(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);

	for (int i = 0; i < 9; i++) {
		button = this->m_menuButtons->GetButton(i);
		button->normalRenderMode = 0;
		button->highlightRenderMode = 0;
	}

	if (this->menu >= Menus::MENU_INGAME) {
		app->canvas->setMenuDimentions(70, 0, 340, Applet::IOS_HEIGHT - this->imgGameMenuPanelbottom->height);
	}

	// Apply theme item dimensions (or fallback to original logic)
	if (theme) {
		if (theme->itemWidth > 0) {
			this->menuItem_width = theme->itemWidth;
		} else if (theme->btnImage) {
			this->menuItem_width = theme->btnImage->width;
		}
		if (theme->itemHeight > 0) this->menuItem_height = theme->itemHeight;
		this->menuItem_paddingBottom = theme->itemPaddingBottom;
	} else if (this->menu >= Menus::MENU_INGAME) {
		this->menuItem_width = this->imgInGameMenuOptionButton->width;
		this->menuItem_paddingBottom = 10;
	}

	// Apply info button theme (or fallback to original logic)
	if (theme && theme->infoBtnImage) {
		for (int i = 0; i != 9; i++) {
			button = this->m_infoButtons->GetButton(i);
			button->SetImage(theme->infoBtnImage, false);
			button->SetHighlightImage(theme->infoBtnHighlightImage, false);
			button->normalRenderMode = theme->infoBtnRenderMode;
			button->highlightRenderMode = theme->infoBtnHighlightRenderMode;
		}
	} else {
		if (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) {
			for (int i = 0; i != 9; i++) {
				button = this->m_infoButtons->GetButton(i);
				button->SetImage(this->imgGameMenuInfoButtonNormal, false);
				button->SetHighlightImage(this->imgGameMenuInfoButtonPressed, false);
				button->normalRenderMode = 0;
				button->highlightRenderMode = 0;
			}
		} else {
			this->menuItem_width = this->imgVendingButtonLarge->width;
			this->menuItem_paddingBottom = 3;
			for (int i = 0; i != 9; i++) {
				button = this->m_infoButtons->GetButton(i);
				button->SetImage(this->imgVendingButtonHelp, false);
				button->SetHighlightImage(nullptr, false);
				button->normalRenderMode = 2;
				button->highlightRenderMode = 0;
			}
		}
	}

	// Try data-driven layout from YAML first
	const YAMLMenuDef* def = getMenuDef(this->menu);
	if (def && def->layout.isSet) {
		if (def->itemWidth > 0) {
			this->menuItem_width = def->itemWidth;
		}
		w = resolveLayoutValue(def->layout.wMode, def->layout.wVal);
		h = resolveLayoutValue(def->layout.hMode, def->layout.hVal);
		x = resolveLayoutValue(def->layout.xMode, def->layout.xVal, w);
		y = resolveLayoutValue(def->layout.yMode, def->layout.yVal);
		if (def->vibrationY >= 0 && this->HasVibration()) {
			y = def->vibrationY;
		}
		app->canvas->setMenuDimentions(x, y, w, h);
		return;
	}

	// No fallback layout needed — all menus should have YAML layout
}

void MenuSystem::updateTouchButtonState() {
	fmButton* button01;
	fmButton* button02;
	//printf("MenuSystem::updateTouchButtonState\n");

	for (int i = 0; i < 9; i++) {
		this->m_menuButtons->GetButton(i)->SetTouchArea(Applet::IOS_WIDTH, 0, 0, 0);
		this->m_infoButtons->GetButton(i)->SetTouchArea(Applet::IOS_WIDTH, 0, 0, 0);
	}

	for (int i = 0; i < 9; i++) {
		button01 = this->m_menuButtons->GetButton(i);
		button02 = this->m_infoButtons->GetButton(i);

		button01->drawButton = false;
		button02->drawButton = false;

		button02->SetHighlighted(false);
		button02->normalRenderMode = 2;
		button02->highlightRenderMode = 0;

		// Apply theme-driven button images, with original fallback
		const MenuTheme* theme = getThemeForMenu(this->menu);
		if (theme && theme->btnImage) {
			button01->SetImage(theme->btnImage, false);
			button01->SetHighlightImage(theme->btnHighlightImage, false);
			button01->normalRenderMode = theme->btnRenderMode;
			button01->highlightRenderMode = theme->btnHighlightRenderMode;
		} else {
			// Fallback: original button image setup by menu ID range
			if (this->menu <= Menus::MENU_INHERIT_BACKMENU) {
				button01->SetImage(this->imgMenuBtnBackground, false);
				button01->SetHighlightImage(this->imgMenuBtnBackgroundOn, false);
				button01->normalRenderMode = 1;
				button01->highlightRenderMode = 0;
			}
			else {
				if (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) {
					button01->SetImage(this->imgInGameMenuOptionButton, false);
				}
				else {
					button01->SetImage(this->imgVendingButtonLarge, false);
				}
				button01->normalRenderMode = 1;
				button01->highlightRenderMode = 0;
				button01->SetHighlightImage(nullptr, false);
			}
		}

		button01->drawTouchArea = false;
	}

	int buttonID = 0;
	for (int i = 0; (i < this->numItems) && (buttonID <= 8); i++) {

		if (this->items[i].action != 0) {
			button01 = this->m_menuButtons->GetButton(buttonID);
			button01->drawButton = true;

			// Data-driven info button visibility, with original fallback
			const YAMLMenuDef* infoDef = getMenuDef(this->menu);
			if (infoDef && infoDef->showInfoButtons) {
				button02 = this->m_infoButtons->GetButton(buttonID);
				button02->drawButton = true;
			} else if (!infoDef) {
				// Fallback: original hardcoded info button check
				uint32_t uVar2 = this->menu;
				bool bVar5 = Menus::MENU_INGAME_LANGUAGE < uVar2;
				if (uVar2 != Menus::MENU_INGAME_HELP) {
					bVar5 = uVar2 != Menus::MENU_INGAME;
				}
				if (((((!bVar5 || (uVar2 == Menus::MENU_INGAME_HELP || uVar2 == Menus::MENU_INGAME_STATUS)) || (uVar2 == Menus::MENU_INGAME_HELP)) || (uVar2 == Menus::MENU_ITEMS)) ||
					((uVar2 == Menus::MENU_ITEMS_WEAPONS || (uVar2 == Menus::MENU_ITEMS_DRINKS)))) ||
					((uVar2 == Menus::MENU_VENDING_MACHINE_CONFIRM || ((uVar2 == Menus::MENU_VENDING_MACHINE_DRINKS || (uVar2 == Menus::MENU_VENDING_MACHINE_SNACKS)))))) {
					button02 = this->m_infoButtons->GetButton(buttonID);
					button02->drawButton = true;
				}
			}

			buttonID++;
		}
	}

	for (int i = 0; i <= 17 /*15*/; i++) {
		this->m_menuButtons->GetButton(i)->SetHighlighted(false);
	}

	// Reset button visibility for buttons 11-17
	for (int i = 11; i <= 17; i++) {
		this->m_menuButtons->GetButton(i)->drawButton = false;
	}

	// Data-driven button visibility from YAML, with original fallback
	const YAMLMenuDef* btnDef = getMenuDef(this->menu);
	if (btnDef && !btnDef->visibleButtons.empty()) {
		for (int btnId : btnDef->visibleButtons) {
			this->m_menuButtons->GetButton(btnId)->drawButton = true;
		}
		// Conditional buttons (e.g., music slider visible only when music is on)
		for (int btnId : btnDef->visibleButtonsConditional) {
			if (isUserMusicOn()) {
				this->m_menuButtons->GetButton(btnId)->drawButton = true;
			}
		}
	}
	// No fallback needed — all menus should have visible_buttons in YAML
}

void MenuSystem::handleUserTouch(int x, int y, bool b) {

	bool v4; // r6
	int v5; // s16
	int v8; // s17
	fmButton* Button; // r0
	fmScrollButton* v10; // r0
	fmScrollButton* btnScroll; // r2
	int i; // r4
	fmButton* v13; // r0
	int j; // r4
	fmButton* v15; // r0
	int TouchedButtonID; // r8
	int v17; // r4
	fmButton* v18; // r0
	fmButtonContainer* btnContainer02; // r0
	fmButton* v20; // r0
	int selectedIndex; // r1
	int v24; // r1
	int v25; // r2
	fmButtonContainer* btnContainer03; // r0
	bool v27; // r3
	int v28; // r0
	VendingMachine* v29; // r2
	int v30; // r12
	VendingMachine* vendingMachine; // r2
	int currentItemQuantity; // r3

	v4 = b;
	v5 = x;
	v8 = y;
	if (this->menu == Menus::MENU_COMIC_BOOK) {
		app->comicBook->Touch(x, y, b);
		return;
	}

	if (this->updateSlider)
	{
		if (!b)
		{
			this->m_menuButtons->GetButton(this->sliderID)->SetHighlighted(false);
			this->updateSlider = 0;
			return;
		}
	}
	else if (!b)
	{
		btnScroll = this->m_scrollBar;
		if (btnScroll->field_0x14_)
		{
			btnScroll->field_0x14_ = 0;
			return;
		}
		if (btnScroll->field_0x38_)
		{
			btnScroll->field_0x38_ = 0;
			return;
		}
		goto LABEL_17;
	}
	if (this->m_scrollBar->field_0x0_ && this->m_scrollBar->barRect.ContainsPoint(x, y))
	{
		if (this->isMainMenuScrollBar){
			this->m_scrollBar->SetTouchOffset(v5, v8);
		}
		else {
			this->m_scrollBar->field_0x54_ = 0;
			this->m_scrollBar->Update(v5, v8);
		}
		this->m_scrollBar->field_0x14_ = 1;
		return;
	}
LABEL_17:
	for (i = 0; i < 16; i++) {
		this->m_menuButtons->GetButton(i)->SetHighlighted(false);
	}
	for (j = 0; j < 9; j++) {
		this->m_infoButtons->GetButton(j)->SetHighlighted(false);
	}
	if (!this->drawHelpText)
	{
		if (app->canvas->touched) {
			if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) { // Port: new
				this->m_vendingButtons->HighlightButton(x, y, false);
				return;
			}
			return;
		}

		TouchedButtonID = this->m_menuButtons->GetTouchedButtonID(v5, v8);
		v17 = this->m_infoButtons->GetTouchedButtonID(v5, v8);

		if (v17 < 0)
		{
			if (TouchedButtonID < 0)
			{
				if (!v4)
				{
					if (this->menu != Menus::MENU_END_RANKING && this->menu != Menus::MENU_LEVEL_STATS)
					{
					LABEL_49:
						if (this->menu != Menus::MENU_VENDING_MACHINE_CONFIRM)
							return;
						if (v4)
						{
							v24 = v5;
							v25 = v8;
							btnContainer03 = this->m_vendingButtons;
							v27 = 1;
						LABEL_61:
							btnContainer03->HighlightButton(v24, v25, v27);
							return;
						}
						v28 = this->m_vendingButtons->GetTouchedButtonID(v5, v8);
						if (v28)
						{
							if (v28 != 1)
								goto LABEL_60;
							vendingMachine = app->vendingMachine.get();
							currentItemQuantity = vendingMachine->currentItemQuantity;
							if (currentItemQuantity <= 1)
								goto LABEL_60;
							vendingMachine->currentItemQuantity = currentItemQuantity - 1;
						}
						else
						{
							v29 = app->vendingMachine.get();
							v30 = v29->currentItemQuantity;
							if (app->player->inventory[24] < v29->currentItemPrice + v30 * v29->currentItemPrice)
								goto LABEL_60;
							v29->currentItemQuantity = v30 + 1;
						}
						this->setMenu(Menus::MENU_VENDING_MACHINE_CONFIRM);
					LABEL_60:
						v24 = 0;
						btnContainer03 = this->m_vendingButtons;
						v25 = 0;
						v27 = 0;
						goto LABEL_61;
					}
					app->canvas->handleEvent(13);
				}
			}
			else if (this->m_menuButtons->GetButton(TouchedButtonID)->drawButton)
			{
				if (v4)
				{
					this->m_menuButtons->GetButton(TouchedButtonID)->SetHighlighted(true);
					if (this->updateVolumeSlider(TouchedButtonID, v5))
					{
						this->sliderID = TouchedButtonID;
						this->updateSlider = 1;
					}
				}
				else if (TouchedButtonID == 11) {
					this->back();
				}
				else if (TouchedButtonID == 15) {
					this->returnToGame();
				}
				else {
					this->selectedIndex = this->m_menuButtons->GetButton(TouchedButtonID)->selectedIndex;
					this->select(this->selectedIndex);
				}
			}
		}
		else if (this->m_infoButtons->GetButton(v17)->drawButton)
		{
			if (v4) {
				this->m_infoButtons->GetButton(v17)->SetHighlighted(true);
			}
			else {
				this->drawHelpText = true;
				this->selectedHelpIndex = this->m_infoButtons->GetButton(v17)->selectedIndex;
			}
		}
		goto LABEL_49;
	}
	if (!v4)
	{
		this->drawHelpText = false;
		this->selectedHelpIndex = -1;
	}
}

void MenuSystem::handleUserMoved(int x, int y) {
	fmScrollButton* pfVar1;
	int iVar2;
	fmButton* pfVar3;
	int iVar4;
	int _x;
	int _y;

	_x = x;
	_y = y;

	if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) { // [GEC]: new line
		this->m_vendingButtons->HighlightButton(x, y, true);
		return;
	}

	if (this->menu == Menus::MENU_COMIC_BOOK) {
		this->app->comicBook->TouchMove(_x, _y);
	}
	else {
		//printf("field_0x674_ %d\n", field_0x674_);
		if (this->updateSlider != false) {
			this->updateVolumeSlider(this->sliderID, _x);
			return;
		}

		// [GEC] Hasta que este fuera del limite del rectangulo, permitir� el desplasamiento de los items del menu
		const int begMouseX = (int)(gBegMouseX * Applet::IOS_WIDTH);
		const int begMouseY = (int)(gBegMouseY * Applet::IOS_HEIGHT);
		if (pointInRectangle(x, y, begMouseX - 3, begMouseY - 3, 6, 6)) {
			return;
		}

		pfVar1 = this->m_scrollBar;
		if (pfVar1->field_0x14_ == 0) {
			if ((pfVar1->field_0x0_ == 0) ||
				(iVar2 = pfVar1->barRect.ContainsPoint(_x, _y), iVar2 == 0)) {
				if (this->drawHelpText == false) {
					iVar2 = 0;
					do {
						iVar4 = iVar2;
						pfVar3 = this->m_menuButtons->GetButton(iVar4);
						pfVar3->SetHighlighted(false);
						iVar2 = iVar4 + 1;
					} while (iVar4 + 1 != 0x10);

					iVar4 = iVar4 + -0xf;
					do {
						pfVar3 = this->m_infoButtons->GetButton(iVar4);
						iVar4 = iVar4 + 1;
						pfVar3->SetHighlighted(false);
					} while (iVar4 != 9);

					pfVar1 = this->m_scrollBar;
					if (pfVar1->field_0x38_ == 0) {
						if ((pfVar1->field_0x0_ == 0) ||
							(iVar2 = pfVar1->boxRect.ContainsPoint(_x, _y), iVar2 == 0)) {
							iVar2 = this->m_menuButtons->GetTouchedButtonID(_x, _y);
							_y = this->m_infoButtons->GetTouchedButtonID(_x, _y);
							if (_y < 0) {
								if ((-1 < iVar2) &&
									(pfVar3 = this->m_menuButtons->GetButton(iVar2),
										pfVar3->drawButton != false)) {
									pfVar3 = this->m_menuButtons->GetButton(iVar2);
									pfVar3->SetHighlighted(true);
									if (this->updateVolumeSlider(iVar2, _x)) {
										this->sliderID = iVar2;
										this->updateSlider = true;
									}
								}
							}
							else {
								pfVar3 = this->m_infoButtons->GetButton(_y);
								if (pfVar3->drawButton != false) {
									pfVar3 = this->m_infoButtons->GetButton(_y);
									pfVar3->SetHighlighted(true);
								}
							}
						}
						else {
							this->m_scrollBar->SetContentTouchOffset(_x, _y);
							this->m_scrollBar->field_0x38_ = 1;
						}
					}
					else {
						pfVar1->UpdateContent(_x, _y);
					}
				}
			}
			else {
				if (this->isMainMenuScrollBar == false) {
					this->m_scrollBar->field_0x54_ = 0;
					this->m_scrollBar->Update(_x, _y);
				}
				else {
					this->m_scrollBar->SetTouchOffset(_x, _y);
				}
				this->m_scrollBar->field_0x14_ = 1;
			}
		}
		else {
			pfVar1->Update(_x, _y);
		}
	}
}

int MenuSystem::getScrollPos() {
	int height;
	int posY;
	int pos;

	pos = this->m_scrollBar->field_0x0_;
	if (pos != 0) {
		pos = this->m_scrollBar->field_0x44_;
	}

	if (this->isMainMenu != false) {
		posY = 0;
		for (int i = 0; i < this->numItems; i++) {
			if (!(this->items[i].flags & 0x8000)) {
				height = this->getMenuItemHeight(i);
				if (pos < (posY + (height >> 1))) {
					return posY;
				}
				posY += height;
			}
		}
	}
	return pos;
}

int MenuSystem::getMenuItemHeight(int i) {


	int padding;
	int height = 29; // default
	int sheight = 0; //[GEC]

	//if ((this->menu != Menus::MENU_MAIN_OPTIONS)  || !HasVibration()) // Old
	{
		if (this->items[i].flags & Menus::ITEM_PADDING) { // [GEC]
			sheight += this->items[i].param;
		}

		if (this->items[i].flags & Menus::ITEM_SCROLLBAR) { // [GEC]
			sheight += Applet::FONT_HEIGHT[app->fontType];
		}
		else if (this->items[i].flags & Menus::ITEM_SCROLLBARTWO) { // [GEC]
			sheight += (Applet::FONT_HEIGHT[app->fontType] << 1);
		}

		if (this->items[i].action && !(this->items[i].flags & (Menus::ITEM_SCROLLBAR | Menus::ITEM_SCROLLBARTWO))) { // [GEC]
			height = this->menuItem_height;
			if (i == (this->numItems - 1)) {
				return height;
			}
			padding = this->menuItem_paddingBottom;
		}
		else {
			height = Applet::FONT_HEIGHT[app->fontType];
			if (i == (this->numItems - 1)) {
				return height;
			}
			padding = this->menuItem_fontPaddingBottom;
		}
		height += padding;
	}
	return height + sheight;
}

int MenuSystem::getMenuItemHeight2(int i) { //[GEC]


	int padding;
	int height = 29; // default
	int sheight = 0; //[GEC]

	//if ((this->menu != Menus::MENU_MAIN_OPTIONS) || !HasVibration()) // Old
	{
		if (this->items[i].flags & Menus::ITEM_PADDING) { // [GEC]
			sheight += this->items[i].param;
		}

		if (this->items[i].flags & Menus::ITEM_SCROLLBAR) { // [GEC]
			sheight += Applet::FONT_HEIGHT[app->fontType];
		}
		else if (this->items[i].flags & Menus::ITEM_SCROLLBARTWO) { // [GEC]
			sheight += (Applet::FONT_HEIGHT[app->fontType] << 1);
		}

		if (this->items[i].action && !(this->items[i].flags & (Menus::ITEM_SCROLLBAR | Menus::ITEM_SCROLLBARTWO))) { // [GEC]
			height = this->menuItem_height;
			padding = this->menuItem_paddingBottom;
		}
		else {

			height = Applet::FONT_HEIGHT[app->fontType];
			padding = this->menuItem_fontPaddingBottom;
		}
		height += padding;
	}
	return height + sheight;
}

void MenuSystem::drawScrollbar(Graphics* graphics) {
	fmScrollButton* pfVar1;
	int uVar2;
	int x, y;

	if(this->isMainMenuScrollBar) {
		x = this->m_scrollBar->barRect.x;
		y = this->m_scrollBar->barRect.y;
		graphics->drawImage(this->imgMenuDial, x, y, 0, 0, 0);
		pfVar1 = this->m_scrollBar;
		uVar2 = pfVar1->field_0x0_;
		if (uVar2 != 0) {
			uVar2 = pfVar1->field_0x48_ + (pfVar1->field_0x4c_ >> 1);
		}
		graphics->drawImage(this->imgMenuDialKnob, x + 12, y + ((uVar2 * 4) / 5 - (this->imgMenuDialKnob->height >> 1)) + 16, 0, 0, 0);
	}
	else {
		if (this->menu >= Menus::MENU_INGAME || this->type != 5 || this->menu == Menus::MENU_END_RANKING || this->menu == Menus::MENU_LEVEL_STATS) {
			this->m_scrollBar->Render(graphics);
		}
	}
}

void MenuSystem::drawButtonFrame(Graphics* graphics) {


	if ((this->menu != Menus::MENU_END_) &&
		((this->menu != Menus::MENU_MAIN_CONFIRMNEW && this->menu != Menus::MENU_MAIN_CONFIRMNEW2) && this->menu != Menus::MENU_END_FINALQUIT) &&
		((this->menu != Menus::MENU_MAIN_EXIT && this->menu != Menus::MENU_ENABLE_SOUNDS) && this->menu != Menus::MENU_END_) &&
		(this->menu != Menus::MENU_MAIN_MINIGAME && this->menu != Menus::MENU_MAIN_DIFFICULTY) &&
		(this->menu != Menus::MENU_MAIN_MORE_GAMES && this->menu == Menus::MENU_SELECT_LANGUAGE))
	{
		int posY = 0;
		for (int i = 0; i < this->numItems; i = i + 1) {
			if (!(this->items[i].flags & 0x8000)) {
				posY += this->getMenuItemHeight(i);
			}
		}
		graphics->drawImage(this->imgMenuLanguageBOX, app->canvas->menuRect[0], posY + app->canvas->menuRect[1] + 1, 32, 0, 0);
	}
}

void MenuSystem::drawTouchButtons(Graphics* graphics, bool b) {


	Text* textBuff;
	fmButton* button;
	bool v12;
	int buttonID;
	int height;
	int posY;

	textBuff = app->localization->getLargeBuffer();
	buttonID = 0;
	posY = 0 - this->getScrollPos();

	int maxItemsMain = 0;
	int maxItemsGame = 0;
	for (int i = 0; (i < this->numItems) && (posY < app->canvas->menuRect[3]); i++)
	{
		if (!(this->items[i].flags & 0x8000)) {
			height = this->getMenuItemHeight(i);

			if ((posY + height) > 0) { // [GEC]
				maxItemsMain++;
			}

			if ((posY + height) > 0 && this->items[i].action && !(this->items[i].flags & (Menus::ITEM_SCROLLBAR | Menus::ITEM_SCROLLBARTWO))) // [GEC]
			{
				button = this->m_menuButtons->GetButton(buttonID);
				button->selectedIndex = i;
				button->SetTouchArea(app->canvas->menuRect[0], posY + app->canvas->menuRect[1], this->menuItem_width, this->menuItem_height);

				if (this->menu >= Menus::MENU_INGAME)
				{
					v12 = buttonID == 15;
					if (buttonID != 15) {
						v12 = buttonID == 11;
					}
					if (!v12 && posY > 210) {// Old -> posY > 230
						maxItemsGame++;
						button->SetTouchArea(app->canvas->menuRect[0], 350, this->menuItem_width, this->menuItem_height, false); // Port: add param "false"
					}
				}

				if (!(this->items[i].flags & (Menus::ITEM_DISABLED | Menus::ITEM_DISABLEDTWO)))
				{
					if (b)
					{
						if (button->highlighted)
						{
							button->Render(graphics);
						}
					}
					else if (!button->highlighted)
					{
						button->Render(graphics);
					}
				}
				else {
					if (!b)
					{
						if (this->menu == Menus::MENU_MAIN_OPTIONS)
						{
							if (button->highlighted) {
								button->highlighted = false;
							}
							button->Render(graphics);
						}
					}
				}

				if (this->menu <= Menus::MENU_INGAME_STATUS ||
					this->menu == Menus::MENU_INGAME_HELP || 
					this->menu == Menus::MENU_ITEMS || 
					this->menu == Menus::MENU_ITEMS_WEAPONS || 
					this->menu == Menus::MENU_ITEMS_DRINKS ||
					this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM || 
					this->menu == Menus::MENU_VENDING_MACHINE_DRINKS || 
					this->menu == Menus::MENU_VENDING_MACHINE_SNACKS)
				{
					button = this->m_infoButtons->GetButton(buttonID);
					button->selectedIndex = i;
					button->SetTouchArea(app->canvas->menuRect[0] + this->menuItem_width + this->menuItem_paddingBottom,
						posY + app->canvas->menuRect[1], this->imgGameMenuInfoButtonPressed->width, this->imgGameMenuInfoButtonPressed->height);

					if (this->menu >= Menus::MENU_INGAME && posY > 210) { // Old -> posY > 230
						button->SetTouchArea(app->canvas->menuRect[0], 350, this->menuItem_width, this->menuItem_height, false); // Port: add param "false"
					}

					if (b)
					{
						if (button->highlighted)
						{
							button->Render(graphics);
						}
					}
					else if (!button->highlighted)
					{
						button->Render(graphics);
					}
				}
				buttonID++;
			}
			posY += height;
		}
	}


	//this->maxItems = maxItemsMain - maxItemsGame;

	//printf("maxItemsMain %d\n", maxItemsMain);
	//printf("maxItemsGame %d\n", maxItemsGame);

	if (this->menu == Menus::MENU_VENDING_MACHINE_CONFIRM) {
		this->m_vendingButtons->Render(graphics);
	}

	textBuff->dispose();
}

void MenuSystem::drawSoftkeyButtons(Graphics* graphics)
{

	Text* textBuff;
	fmButton* button;
	int curFontType;
	int strX, strY;

	textBuff = app->localization->getSmallBuffer();

	button = this->m_menuButtons->GetButton(11);
	if (button->drawButton)
	{
		curFontType = app->fontType;
		app->setFont(0);

		if ((this->menu >= Menus::MENU_INGAME) && (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX)) {
			button->SetTouchArea(9, 268, app->hud->imgInGameMenuSoftkey->width, app->hud->imgInGameMenuSoftkey->height);

			if (button->highlighted) {
				graphics->drawImage(app->hud->imgInGameMenuSoftkey, 9, 268, 0, 0, 0);
			}
			else {
				graphics->drawRegion(app->hud->imgInGameMenuSoftkey, 0, 0,
					app->hud->imgInGameMenuSoftkey->width, app->hud->imgInGameMenuSoftkey->height,
					9, 268, 0, 0, 2);
			}

			strX = 42;
			strY = 295;
		}
		else if (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) {
			button->SetTouchArea(10, 262, this->imgSwitchLeftActive->width, this->imgSwitchLeftActive->height);
			graphics->drawImage(button->highlighted ? this->imgSwitchLeftActive : this->imgSwitchLeftNormal, 10, 262, 0, 0, 0);

			strX = 8; // old = -2;
			strY = 314; // old = 320;
		}
		else {
			button->SetTouchArea(56, 277, app->hud->imgVendingSoftkeyNormal->width, app->hud->imgVendingSoftkeyNormal->height);

			if (button->highlighted) {
				graphics->drawImage(app->hud->imgVendingSoftkeyPressed, 56, 277, 0, 0, 0);
			}
			else {
				graphics->drawRegion(app->hud->imgVendingSoftkeyNormal, 0, 0,
					app->hud->imgVendingSoftkeyNormal->width, app->hud->imgVendingSoftkeyNormal->height,
					56, 277, 0, 0, 0);
			}

			strX = 67;
			strY = 307;
		}
		
		textBuff->setLength(0);
		app->localization->composeText(3, 80, textBuff);
		textBuff->dehyphenate();
		graphics->drawString(textBuff, strX, strY, 36);
		app->setFont(curFontType);
	}

	button = this->m_menuButtons->GetButton(15);
	if (button->drawButton)
	{
		curFontType = app->fontType;
		app->setFont(0);

		
		if ((this->menu >= Menus::MENU_INGAME) && (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX)) {
			button->SetTouchArea(372, 268, app->hud->imgInGameMenuSoftkey->width, app->hud->imgInGameMenuSoftkey->height);

			if (button->highlighted) {
				graphics->drawImage(app->hud->imgInGameMenuSoftkey, 372, 268, 0, 0, 0);
			}
			else {
				graphics->drawRegion(app->hud->imgInGameMenuSoftkey, 0, 0,
					app->hud->imgInGameMenuSoftkey->width, app->hud->imgInGameMenuSoftkey->height,
					372, 268, 0, 0, 2);
			}

			strX = 448;
			strY = 295;
		}
		else if (this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) {
			button->SetTouchArea(438, 268, app->hud->imgSwitchRightActive->width, app->hud->imgSwitchRightActive->height);
			graphics->drawImage(button->highlighted ? app->hud->imgSwitchRightActive : app->hud->imgSwitchRightNormal, 438, 268, 0, 0, 0);
			strX = 478;
			strY = 320;
		}
		else {
			button->SetTouchArea(362, 277, app->hud->imgVendingSoftkeyNormal->width, app->hud->imgVendingSoftkeyNormal->height);

			if (button->highlighted) {
				graphics->drawImage(app->hud->imgVendingSoftkeyPressed, 362, 277, 0, 0, 0);
			}
			else {
				graphics->drawRegion(app->hud->imgVendingSoftkeyNormal, 0, 0,
					app->hud->imgVendingSoftkeyNormal->width, app->hud->imgVendingSoftkeyNormal->height,
					362, 277, 0, 0, 0);
			}

			strX = 411;
			strY = 307;
		}

		textBuff->setLength(0);
		textBuff->append((this->menu <= Menus::MENU_ITEMS_HOLY_WATER_MAX) ? "Resume" : "Exit");
		textBuff->dehyphenate();
		graphics->drawString(textBuff, strX, strY, 40);
		app->setFont(curFontType);
	}

	textBuff->dispose();
}

int MenuSystem::drawCustomScrollbar(Graphics* graphics, MenuItem* item, Text* text, int yPos) { // [GEC]


	int menuItem_width = this->menuItem_width;
	int v70 = this->menuItem_height >> 1;
	int v69 = v70 - 2;
	int v23 = app->canvas->menuRect[0];

	int value = 0;
	int valueScroll = 0;
	fmButton* button =  nullptr;
	bool disable = false;
	bool change = false;

	text->setLength(0);
	if (item->param == 1) { // SfxVolume
		value = (!app->sound->allowSounds) ? 0 : app->sound->soundFxVolume;
		valueScroll = this->sfxVolumeScroll;
		button = this->m_menuButtons->GetButton(12);
		disable = false;
		change = app->menuSystem->changeSfxVolume;
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}
	else if (item->param == 2) { // MusicVolume
		value = (!app->sound->allowMusics) ? 0 : app->sound->musicVolume;
		valueScroll = this->musicVolumeScroll;
		button = this->m_menuButtons->GetButton(13);
		disable = !isUserMusicOn();
		change = app->menuSystem->changeMusicVolume;
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}
	else if (item->param == 3) { // Alpha
		value = app->canvas->m_controlAlpha;
		valueScroll = this->alphaScroll;
		button = this->m_menuButtons->GetButton(14);
		disable = false;
		change = app->menuSystem->changeButtonsAlpha;
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}
	else if (item->param == 4) { // Vibration Intensity
		value = gVibrationIntensity;
		valueScroll = this->vibrationIntensityScroll;
		button = this->m_menuButtons->GetButton(16);
		disable = false;
		change = app->menuSystem->changeVibrationIntensity;
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}
	else if (item->param == 5) { // Deadzone
		value = gDeadZone;
		valueScroll = this->deadzoneScroll;
		button = this->m_menuButtons->GetButton(17);
		disable = false;
		change = app->menuSystem->changeDeadzone;
		if (change) { text->append("< "); }
		text->append(value);
		if (change) { text->append(" >"); }
	}

	if (item->flags & Menus::ITEM_SCROLLBAR) {
		if (disable) {
			app->setFontRenderMode(2);
		}

		graphics->drawString(text, menuItem_width + v23 - 4, yPos, 24);
		app->setFontRenderMode(0);
		int v27 = yPos + Applet::FONT_HEIGHT[app->fontType];
		if (button) {
			button->SetTouchArea(v23, v27 + 1, menuItem_width, v69);
		}
		graphics->setColor(0xFF323232);
		graphics->drawRect(v23, v27 + 1, menuItem_width - 1, v70 - 3);
		graphics->FMGL_fillRect(v23 + 1, v27 + 2, menuItem_width - 2, v70 - 4, 0.42, 0.35, 0.31, 0.7);

		int v66 = menuItem_width + v23;
		int v29 = v27 + 1;
		int v68 = v70 - 3;
		int v30 = v27 + 2;
		int v67 = v70 - 4;
		graphics->setColor(0xFF323232);
		if (button && button->highlighted) {
			int v32 = valueScroll - 12;
			if (v23 <= valueScroll - 12) {
				if (v66 < valueScroll + 12) {
					v32 = v66 - 24;
				}
			}
			else {
				v32 = v23;
			}
			graphics->drawRect(v32, v29, 23, v68);
			graphics->FMGL_fillRect(v32 + 1, v30, 22, v67, 0.9, 0.9, 0.65, 1.0);
		}
		else
		{
			int v33 = v23 + ((menuItem_width - 24) * value) / 100;
			graphics->drawRect(v33, v29, 23, v68);
			graphics->FMGL_fillRect(v33 + 1, v30, 22, v67, 0.75, 0.69, 0.65, 1.0);
		}

		if (disable) {
			graphics->FMGL_fillRect(v23, v30, menuItem_width, v69, 0.5, 0.5, 0.5, 0.3);
		}

		return 0;
	}
	else if (item->flags & Menus::ITEM_SCROLLBARTWO) {
		int v6 = ((480 - this->imgMenuOptionBOX3->width) >> 1) + 1;
		int v59 = (480 - this->imgMenuOptionBOX3->width) >> 1;
		int v27 = yPos + Applet::FONT_HEIGHT[app->fontType];

		int width = this->imgMenuOptionSliderON->width;
		if (button) {
			button->SetTouchArea(v6, v27 - 20, this->imgMenuOptionSliderBar->width, this->imgMenuOptionSliderBar->height);
		}
		graphics->drawImage(this->imgMenuOptionSliderBar, v6, v27 - 20, 0, 0, 0);
		int v9 = valueScroll - (width >> 1);
		int v10;
		if (v6 <= v9) {
			v10 = v59 + 246;
			if (width + v9 > v59 + 246)
				v9 = v10 - width;
		}
		else {
			v9 = v6;
			v10 = v59 + 246;
		}
		if (button && button->highlighted) {
			graphics->drawImage(this->imgMenuOptionSliderON, v9, v27, 0, 0, 0);
		}
		else {
			graphics->drawImage(this->imgMenuOptionSliderOFF, (v6 + ((245 - width) * value) / 100), v27, 0, 0, 0);
		}

		if (change) {
			v10 += 5;
			text->setLength(0); 
			text->append("<");
			graphics->drawString(text, v10, v27+5, 24);
			v10 += 5;
		}
		this->drawNumbers(graphics, v10, v27+3, 0, value);
		if (change) {
			v10 += 40;
			text->setLength(0);
			text->append(">");
			graphics->drawString(text, v10, v27+5, 24);
		}

		if (disable) {
			graphics->FMGL_fillRect(v6, v27 - 20, 244, 20, 0.2, 0.2, 0.2, 0.5);
		}
		return 0;
	}

	return 0;
}

void MenuSystem::drawOptionsScreen(Graphics* graphics)
{


	Text* LargeBuffer; // r0
	int menu; // r3
	int v6; // r5
	int width; // r6
	fmButton* Button; // r0
	int v9; // r4
	int v10; // r10
	fmButton* v11; // r0
	Sound* sound; // r2
	int v13; // r3
	fmButton* v14; // r0
	int v15; // r4
	fmButton* v16; // r0
	Sound* v17; // r0
	int v18; // r3
	int v19; // r6
	int numItems; // r2
	int v22; // r5
	int v23; // r10
	int MenuItemHeight; // r0
	Sound* v25; // r3
	int v26; // r4
	int v27; // r4
	fmButton* v28; // r0
	int v29; // r5
	int v30; // r6
	int field_0x668; // r3
	int v32; // r4
	int v33; // r4
	int v34; // r5
	Sound* v35; // r3
	int v36; // r4
	int v37; // r5
	fmButton* v38; // r0
	int field_0x66c; // r0
	int v40; // r4
	int v41; // r4
	int v42; // r6
	int v43; // r2
	int v44; // r10
	int i; // r5
	int v47; // r0
	int buttonsAlpha; // r4
	int v49; // r4
	fmButton* v50; // r0
	int v51; // r6
	int v52; // r5
	int field_0x670; // r0
	int v54; // r4
	float green; // r3
	int v57; // [sp+0h] [bp-84h]
	float red; // [sp+4h] [bp-80h]
	int v59; // [sp+14h] [bp-70h]
	int v60; // [sp+1Ch] [bp-68h]
	int v61; // [sp+24h] [bp-60h]
	int v62; // [sp+28h] [bp-5Ch]
	Text* v64; // [sp+34h] [bp-50h]
	int v65; // [sp+38h] [bp-4Ch]
	int v66; // [sp+40h] [bp-44h]
	int v67; // [sp+44h] [bp-40h]
	int v68; // [sp+50h] [bp-34h]
	int v69; // [sp+60h] [bp-24h]
	int v70; // [sp+64h] [bp-20h]
	int menuItem_width; // [sp+68h] [bp-1Ch]

	LargeBuffer = app->localization->getLargeBuffer();
	menu = this->menu;
	v64 = LargeBuffer;
	if (menu == Menus::MENU_MAIN_OPTIONS)
	{
		v6 = ((480 - this->imgMenuOptionBOX3->width) >> 1) + 1;
		v59 = (480 - this->imgMenuOptionBOX3->width) >> 1;

		LargeBuffer->setLength(0);
		app->localization->composeText(MenuSystem::INDEX_OTHER, MenuStrings::OPTIONS_ITEM, v64);
		v64->dehyphenate();
		graphics->drawString(v64, v6 + (this->imgMenuOptionBOX3->width >> 1), 157, 3);
		width = this->imgMenuOptionSliderON->width;
		Button = this->m_menuButtons->GetButton(12);
		Button->SetTouchArea(v6, 180, this->imgMenuOptionSliderBar->width, this->imgMenuOptionSliderBar->height);
		graphics->drawImage(this->imgMenuOptionSliderBar, v6, 180, 0, 0, 0);
		v9 = this->sfxVolumeScroll - (width >> 1);
		if (v6 <= v9)
		{
			v10 = v59 + 246;
			if (width + v9 > v59 + 246)
				v9 = v10 - width;
		}
		else
		{
			v9 = v6;
			v10 = v59 + 246;
		}
		v11 = this->m_menuButtons->GetButton(12);
		if (v11->highlighted)
			graphics->drawImage(this->imgMenuOptionSliderON, v9, 200, 0, 0, 0);
		else
			graphics->drawImage(this->imgMenuOptionSliderOFF,
				(v6 + ((245 - width) * app->sound->soundFxVolume) / 100),
				200,
				0,
				0,
				0);

		v64->setLength(0);
		app->localization->composeText(MenuSystem::INDEX_OTHER, MenuStrings::SOUND_FX_VOLUME, v64);
		v64->dehyphenate();
		graphics->drawString(v64, v59 + 15, 200, 36);
		sound = app->sound.get();
		v13 = sound->soundFxVolume;
		if (!sound->allowSounds)
			v13 = 0;

		this->drawNumbers(graphics, v10, 200, 0, v13);
		v14 = m_menuButtons->GetButton(13);
		v14->SetTouchArea(v6, 247, this->imgMenuOptionSliderBar->width, this->imgMenuOptionSliderBar->height);
		graphics->drawImage(this->imgMenuOptionSliderBar, v6, 247, 0, 0, 0);
		v15 = this->musicVolumeScroll - (width >> 1);
		if (v6 <= v15)
		{
			if (v10 < width + v15)
				v15 = v10 - width;
		}
		else
		{
			v15 = v6;
		}
		v16 = this->m_menuButtons->GetButton(13);
		if (v16->highlighted)
			graphics->drawImage(this->imgMenuOptionSliderON, v15, 267, 0, 0, 0);
		else
			graphics->drawImage(this->imgMenuOptionSliderOFF,
				(v6 + ((245 - width) * app->sound->musicVolume) / 100),
				267,
				0,
				0,
				0);

		v64->setLength(0);
		app->localization->composeText(MenuSystem::INDEX_OTHER, MenuStrings::SOUND_MUSIC_VOLUME, v64);
		v64->dehyphenate();
		graphics->drawString(v64, v59 + 15, 267, 36);
		v17 = app->sound.get();
		v18 = app->sound->musicVolume;
		if (!app->sound->allowMusics) {
			v18 = 0;
		}
		this->drawNumbers(graphics, v10, 267, 0, v18);
		if (!this->isUserMusicOn()) {
			graphics->FMGL_fillRect(v6, 247, 244, 20, 0.2, 0.2, 0.2, 0.5);
		}
	}
	else if (menu == Menus::MENU_INGAME_OPTIONS)
	{
		menuItem_width = this->menuItem_width;
		v19 = 0;
		numItems = this->numItems;
		v70 = this->menuItem_height >> 1;
		v69 = v70 - 2;
		v23 = app->canvas->menuRect[0];

		for (v22 = 0; v22 < numItems; ++v22) {
			if (!(this->items[v22].flags & 0x8000)) {
				MenuItemHeight = MenuSystem::getMenuItemHeight(v22);
				numItems = this->numItems;
				v19 += MenuItemHeight;
			}
		}
		v64->setLength(0);
		v64->append("SoundFx Volume");
		v64->dehyphenate();
		graphics->drawString(v64, v23 + 4, v19 + 10, 20);
		v25 = app->sound.get();
		v26 = app->sound->soundFxVolume;
		if (!app->sound->allowSounds)
			v26 = 0;
		v64->setLength(0);
		v64->append(v26);
		v66 = menuItem_width + v23;
		graphics->drawString(v64, menuItem_width + v23 - 4, v19 + 10, 24);
		v27 = v19 + 10 + Applet::FONT_HEIGHT[app->fontType];
		v28 = this->m_menuButtons->GetButton(12);
		v29 = v27 + 1;
		v28->SetTouchArea(v23, v27 + 1, menuItem_width, v69);
		graphics->setColor(-13487566);
		v68 = v70 - 3;
		graphics->drawRect(v23, v27 + 1, menuItem_width - 1, v70 - 3);
		v30 = v27 + 2;
		v67 = v70 - 4;
		graphics->FMGL_fillRect(v23 + 1, v27 + 2, menuItem_width - 2, v70 - 4, 0.42, 0.35, 0.31, 0.7);
		graphics->setColor(-13487566);
		if (this->m_menuButtons->GetButton(12)->highlighted)
		{
			field_0x668 = this->sfxVolumeScroll;
			v32 = field_0x668 - 12;
			if (v23 <= field_0x668 - 12)
			{
				if (v66 < field_0x668 + 12)
					v32 = v66 - 24;
			}
			else
			{
				v32 = v23;
			}
			graphics->drawRect(v32, v29, 23, v68);
			graphics->FMGL_fillRect(v32 + 1, v30, 22, v67, 0.9, 0.9, 0.65, 1.0);
		}
		else
		{
			v33 = v23 + ((menuItem_width - 24) * app->sound->soundFxVolume) / 100;
			graphics->drawRect(v33, v29, 23, v68);
			graphics->FMGL_fillRect(v33 + 1, v30, 22, v67, 0.75, 0.69, 0.65, 1.0);
		}
		v34 = v29 + v70 + 8;
		v64->setLength(0);
		v64->append("Music Volume");
		v64->dehyphenate();

		if (!this->isUserMusicOn()) {
			app->setFontRenderMode(2);
		}

		graphics->drawString(v64, v23 + 4, v34, 20);
		v35 = app->sound.get();
		v36 = app->sound->musicVolume;
		if (!app->sound->allowMusics)
			v36 = 0;
		v64->setLength(0);
		v64->append(v36);
		graphics->drawString(v64, menuItem_width + v23 - 4, v34, 24);
		app->setFontRenderMode(0);

		v37 = v34 + Applet::FONT_HEIGHT[app->fontType];
		v38 = this->m_menuButtons->GetButton(13);
		v38->SetTouchArea(v23, v37, menuItem_width, v69);
		graphics->setColor(-13487566);
		graphics->drawRect(v23, v37, menuItem_width - 1, v68);
		graphics->FMGL_fillRect(v23 + 1, v37 + 1, menuItem_width - 2, v67, 0.42, 0.35, 0.31, 0.7);
		graphics->setColor(-13487566);
		if (this->m_menuButtons->GetButton(13)->highlighted)
		{
			field_0x66c = this->musicVolumeScroll;
			v40 = field_0x66c - 12;
			if (v23 <= field_0x66c - 12)
			{
				if (v66 < field_0x66c + 12)
					v40 = v66 - 24;
			}
			else
			{
				v40 = v23;
			}
			graphics->drawRect(v40, v37, 23, v68);
			graphics->FMGL_fillRect(v40 + 1, v37 + 1, 22, v67, 0.9, 0.9, 0.65, 1.0);
		}
		else
		{
			v41 = v23 + ((menuItem_width - 24) * app->sound->musicVolume) / 100;
			graphics->drawRect(v41, v37, 23, v68);
			graphics->FMGL_fillRect(v41 + 1, v37 + 1, 22, v67, 0.75, 0.69, 0.65, 1.0);
		}
		if (!isUserMusicOn()) {
			graphics->FMGL_fillRect(v23, v37, menuItem_width, v69, 0.5, 0.5, 0.5, 0.3);
		}
	}
	else
	{
		v65 = this->menuItem_width;
		v61 = this->menuItem_height >> 1;
		v42 = 0;
		v43 = this->numItems;
		v44 = app->canvas->menuRect[0];
		for (i = 0; i < v43; ++i) {
			if (!(this->items[i].flags & 0x8000)) {
				v47 = this->getMenuItemHeight(i);
				v43 = this->numItems;
				v42 += v47;
			}
		}
		v64->setLength( 0);
		v64->append("Alpha");
		v64->dehyphenate();
		graphics->drawString(v64, v44 + 4, v42 + 10, 20);
		buttonsAlpha = app->canvas->m_controlAlpha;
		v64->setLength(0);
		v64->append(buttonsAlpha);
		v60 = v65 + v44;
		graphics->drawString(v64, v65 + v44 - 4, v42 + 10, 24);
		v49 = v42 + 10 + Applet::FONT_HEIGHT[app->fontType];
		v50 = this->m_menuButtons->GetButton(14);
		v51 = v49 + 1;
		v50->SetTouchArea(v44, v49 + 1, v65, v61 - 2);
		graphics->setColor(-13487566);
		v52 = v61 - 3;
		graphics->drawRect(v44, v49 + 1, v65 - 1, v61 - 3);
		v62 = v49 + 2;
		graphics->FMGL_fillRect(v44 + 1, v49 + 2, v65 - 2, v61 - 4, 0.42, 0.35, 0.31, 0.7);
		graphics->setColor(-13487566);
		if (this->m_menuButtons->GetButton(14)->highlighted)
		{
			field_0x670 = this->alphaScroll;
			v54 = field_0x670 - 12;
			if (v44 <= field_0x670 - 12)
			{
				if (v60 < field_0x670 + 12)
					v54 = v60 - 24;
			}
			else
			{
				v54 = v44;
			}
			graphics->drawRect(v54, v51, 23, v52);
			green = 0.9f;
			v57 = v61 - 4;
			red = 0.9f;
		}
		else
		{
			v54 = v44 + (v65 - 24) * app->canvas->m_controlAlpha / 100;
			graphics->drawRect(v54, v51, 23, v52);
			red = 0.75f;
			green = 0.69f;
			v57 = v61 - 4;
		}
		graphics->FMGL_fillRect(v54 + 1, v62, 22, v57, red, green, 0.65, 1.0);
	}
	v64->dispose();
}

void MenuSystem::drawNumbers(Graphics* graphics, int x, int y, int space, int number)
{
	int num;

	if (number < 1000) {
		num = (number % 100) / 10;
		graphics->drawRegion(this->imgHudNumbers, 0, (9 - number / 100) * 20, 10, 20, x, y, 20, 0, 0);
		x += space + 10;
		graphics->drawRegion(this->imgHudNumbers, 0, (9 - num) * 20, 10, 20, x, y, 20, 0, 0);
		x += space + 10;
		graphics->drawRegion(this->imgHudNumbers, 0, ((num * 10 - number % 100) + 9) * 20, 10, 20, x, y, 20, 0, 0);
	}
	else {
		puts("ERROR: drawnumbers() does not currently support values over 999 ");
	}
}

bool MenuSystem::HasVibration() {
	return true;
}
bool MenuSystem::isUserMusicOn() {
	return true;
}

bool MenuSystem::updateVolumeSlider(int buttonId, int x)
{

	int* value;
	int v9;
	int v10;
	//printf("buttonId %d\n", buttonId);

	if ((buttonId >= 12 && buttonId <= 14) || (buttonId >= 16 && buttonId <= 17)) {
		if (buttonId == 12) {
			this->sfxVolumeScroll = x;
			value = &app->sound->soundFxVolume;
		}
		else if (buttonId == 13) {
			this->musicVolumeScroll = x;
			value = &app->sound->musicVolume;
		}
		else if (buttonId == 14) {
			this->alphaScroll = x;
			value = &app->canvas->m_controlAlpha;
		}
		else if (buttonId == 16) { // [GEC]
			this->vibrationIntensityScroll = x;
			value = &gVibrationIntensity;
		}
		else if (buttonId == 17) { // [GEC]
			this->deadzoneScroll = x;
			value = &gDeadZone;
		}
		if (this->menu == Menus::MENU_MAIN_OPTIONS || this->menu == Menus::MENU_MAIN_OPTIONS_SOUND || this->menu == Menus::MENU_MAIN_CONTROLS || this->menu == Menus::MENU_MAIN_CONTROLLER) {
			v9 = 245 - this->imgMenuOptionSliderON->width;
			v10 = 100 * (x - ((this->imgMenuOptionSliderON->width >> 1) + ((480 - this->imgMenuOptionBOX3->width) >> 1) + 4));
		}
		else
		{
			v9 = this->menuItem_width - 24;
			v10 = 100 * (x - (app->canvas->menuRect[0] + 12));
		}
		*value = v10 / v9;
		if (*value < 0) {
			*value = 0;
		}
		if (*value > 100) {
			*value = 100;
		}
		app->sound->updateVolume();
		return true;
	}

	return false;
}

void MenuSystem::refresh() {
	this->setMenu(this->menu);
}

void MenuSystem::soundClick() {
	this->app->sound->playSound(Sounds::getResIDByName(SoundName::DIALOG_HELP), 0, 5, false);
}
