#include <stdexcept>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include "Log.h"

#include "CAppContainer.h"
#include "ConfigEnums.h"
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
				std::string actionName = item["action"].asString("none");
				int action = actionFromString(actionName.c_str());
				int param = item["param"].asInt(0);
				DataNode gotoNode = item["goto"];
				if (gotoNode) {
					param = menuNameToId(gotoNode.asString(""));
				}
				if (actionName != "goto") {
					DataNode namedParam = item[actionName.c_str()];
					if (namedParam) {
						param = ConfigEnums::resolveEnum(actionName, namedParam.asString(""));
					}
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
					std::string actionName = item["action"].asString("none");
					mi.action = actionFromString(actionName.c_str());
					mi.param = item["param"].asInt(0);
					DataNode gotoNode = item["goto"];
					if (gotoNode) {
						mi.param = menuNameToId(gotoNode.asString(""));
					}
					if (actionName != "goto") {
						DataNode namedParam = item[actionName.c_str()];
						if (namedParam) {
							mi.param = ConfigEnums::resolveEnum(actionName, namedParam.asString(""));
						}
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
