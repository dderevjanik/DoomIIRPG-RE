#pragma once
// String constants for sound names used in hardcoded playSound calls.
// These map to the "name" field in sounds.yaml entries.
// Using constants gives compile-time safety for name-based sound lookups.

namespace SoundName {
	constexpr const char* ARACHNOTRON_ACTIVE = "arachnotron_active";
	constexpr const char* ARACHNOTRON_WALK = "arachnotron_walk";
	constexpr const char* CHIME = "chime";
	constexpr const char* DIALOG_HELP = "dialog_help";
	constexpr const char* DOOR_CLOSE = "door_close";
	constexpr const char* DOOR_OPEN = "door_open";
	constexpr const char* EXPLOSION = "explosion";
	constexpr const char* FIREBALL_IMPACT = "fireball_impact";
	constexpr const char* GIB = "gib";
	constexpr const char* GLASS = "glass";
	constexpr const char* HACK_FAIL = "hack_fail";
	constexpr const char* HACK_SUCCESS = "hack_success";
	constexpr const char* HOLYWATERPISTOL_REFILL = "holywaterpistol_refill";
	constexpr const char* HOOF = "hoof";
	constexpr const char* ITEM_PICKUP = "item_pickup";
	constexpr const char* LOOT = "loot";
	constexpr const char* MENU_OPEN = "menu_open";
	constexpr const char* MENU_SCROLL = "menu_scroll";
	constexpr const char* MUSIC_GENERAL = "music_general";
	constexpr const char* MUSIC_LEVEL_END = "music_level_end";
	constexpr const char* MUSIC_LEVELUP = "music_levelup";
	constexpr const char* MUSIC_TITLE = "music_title";
	constexpr const char* NO_USE = "no_use";
	constexpr const char* NO_USE2 = "no_use2";
	constexpr const char* PISTOL = "pistol";
	constexpr const char* PLASMA = "plasma";
	constexpr const char* PLAYER_DEATH1 = "player_death1";
	constexpr const char* PLAYER_DEATH2 = "player_death2";
	constexpr const char* PLAYER_DEATH3 = "player_death3";
	constexpr const char* PLAYER_PAIN1 = "player_pain1";
	constexpr const char* PLAYER_PAIN2 = "player_pain2";
	constexpr const char* SECRET = "secret";
	constexpr const char* SENTRYBOT_ACTIVATE = "sentrybot_activate";
	constexpr const char* SENTRYBOT_DEATH = "sentrybot_death";
	constexpr const char* SENTRYBOT_PAIN = "sentrybot_pain";
	constexpr const char* SENTRYBOT_RETURN = "sentrybot_return";
	constexpr const char* SLIDER = "slider";
	constexpr const char* SWITCH_EXIT = "switch_exit";
	constexpr const char* TELEPORT = "teleport";
	constexpr const char* USE_ITEM = "use_item";
	constexpr const char* VENDING_SALE = "vending_sale";
	constexpr const char* WEAPON_PICKUP = "weapon_pickup";
	constexpr const char* WEAPON_SNIPER_SCOPE = "weapon_sniper_scope";
} // namespace SoundName
