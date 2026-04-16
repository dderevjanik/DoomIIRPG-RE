#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <climits>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <SDL.h>
#include "SDLGL.h"
#include "VFS.h"
#include "DataNode.h"

#include "CAppContainer.h"
#include "App.h"
#include "IGameModule.h"
#include "DoomIIRPGGame.h"
#include "Image.h"
#include "Resource.h"
#include "Render.h"
#include "GLES.h"
#include "JavaStream.h"

#include "Canvas.h"
#include "Graphics.h"
#include "Player.h"
#include "Game.h"
#include "Graphics.h"
#include "Utils.h"
#include "TinyGL.h"
#include "Input.h"
#include "GameScript.h"
#include "CrashHandler.h"
#include "Log.h"

#include "DevConsole.h"
#include "ModManager.h"

#include <dirent.h>
#include <signal.h>

static DevConsole g_devConsole;
static ModManager g_modManager;

static void handleSIGTERM(int) {
	// Gracefully request shutdown instead of letting the signal kill the process
	if (CAppContainer::getInstance() && CAppContainer::getInstance()->app) {
		CAppContainer::getInstance()->app->shutdown();
	} else {
		_exit(0);
	}
}

void drawView(SDLGL* sdlGL);

int main(int argc, char* args[]) {
	CrashHandler_Init();
	logInit("engine.log");
	signal(SIGTERM, handleSIGTERM);

	// Startup diagnostics
	LOG_INFO("DRPGEngine {} ({})\n", PROJECT_VERSION,
#ifdef NDEBUG
		"Release"
#else
		"Debug"
#endif
	);
#if defined(__APPLE__)
	LOG_INFO("Platform: macOS (arm64: {})\n",
#if defined(__aarch64__)
		"yes"
#else
		"no"
#endif
	);
#elif defined(_WIN32)
	LOG_INFO("Platform: Windows\n");
#elif defined(__linux__)
	LOG_INFO("Platform: Linux\n");
#else
	LOG_INFO("Platform: Unknown\n");
#endif

	int UpTime = 0;

	const char* gameDir = ".";
	const char* gameName = nullptr;
	const char* customMap = nullptr;
	bool headless = false;
	int maxTicks = 0;           // 0 = unlimited (normal mode)
	unsigned int seed = 0;      // 0 = don't seed (normal mode)
	bool hasSeed = false;
	const char* scriptFile = nullptr;
	std::vector<std::string> modDirs;  // --mod directories (in order specified)

	for (int i = 1; i < argc; i++) {
		if (strcmp(args[i], "--help") == 0 || strcmp(args[i], "-h") == 0) {
			printf("DRPGEngine %s\n", PROJECT_VERSION);
			printf("Usage: %s [options]\n\n", args[0]);
			printf("Options:\n");
			printf("  -h, --help              Show this help message and exit\n");
			printf("  --game <name>           Select game from games/ directory\n");
			printf("  --gamedir <path>        Set game directory (default: auto-detect)\n");
			printf("  --map <path>            Load a level directory or map file\n");
			printf("  --minigame <name>       Launch a specific minigame\n");
			printf("  --skip-intro            Skip the intro sequence\n");
			printf("  --skip-travel-map       Skip the travel map screen\n");
			printf("  --headless              Run without a window (deterministic loop)\n");
			printf("  --ticks <n>             Quit after n ticks (headless/script mode)\n");
			printf("  --seed <n>              Set random seed for deterministic runs\n");
			printf("  --script <file>         Load and replay an input script\n");
			printf("  --mod <dir|zip>         Load a mod directory or .zip (repeatable)\n");
			return 0;
		} else if (strcmp(args[i], "--gamedir") == 0 && i + 1 < argc) {
			gameDir = args[++i];
		} else if (strcmp(args[i], "--game") == 0 && i + 1 < argc) {
			gameName = args[++i];
		} else if (strcmp(args[i], "--map") == 0 && i + 1 < argc) {
			customMap = args[++i];
		} else if (strcmp(args[i], "--minigame") == 0 && i + 1 < argc) {
			CAppContainer::getInstance()->minigameName = args[++i];
		} else if (strcmp(args[i], "--skip-travel-map") == 0) {
			CAppContainer::getInstance()->skipTravelMap = true;
		} else if (strcmp(args[i], "--skip-intro") == 0) {
			CAppContainer::getInstance()->skipIntro = true;
		} else if (strcmp(args[i], "--headless") == 0) {
			headless = true;
		} else if (strcmp(args[i], "--ticks") == 0 && i + 1 < argc) {
			maxTicks = atoi(args[++i]);
		} else if (strcmp(args[i], "--seed") == 0 && i + 1 < argc) {
			seed = (unsigned int)atoi(args[++i]);
			hasSeed = true;
		} else if (strcmp(args[i], "--script") == 0 && i + 1 < argc) {
			scriptFile = args[++i];
		} else if (strcmp(args[i], "--mod") == 0 && i + 1 < argc) {
			modDirs.push_back(args[++i]);
		}
	}

	if (headless) {
		CAppContainer::getInstance()->headless = true;
	}
	if (hasSeed) {
		std::srand(seed);
		LOG_INFO("[main] Seed: {}\n", seed);
	}

	// --game <name> is sugar for --gamedir games/<name>
	// Default to doom2rpg if no game/gamedir specified
	std::string gameDirStr;
	if (gameName) {
		gameDirStr = std::string("games/") + gameName;
		gameDir = gameDirStr.c_str();
		LOG_INFO("[main] Game: {} (directory: {})\n", gameName, gameDir);
	} else if (strcmp(gameDir, ".") == 0) {
		// Auto-detect: scan games/ for directories containing game.yaml
		std::vector<std::string> detectedGames;
		DIR* gamesDir = opendir("games");
		if (gamesDir) {
			struct dirent* entry;
			while ((entry = readdir(gamesDir)) != nullptr) {
				if (entry->d_name[0] == '.') continue;
				std::string yamlPath = std::string("games/") + entry->d_name + "/game.yaml";
				if (access(yamlPath.c_str(), F_OK) == 0) {
					detectedGames.push_back(entry->d_name);
				}
			}
			closedir(gamesDir);
		}

		if (detectedGames.size() == 1) {
			gameDirStr = "games/" + detectedGames[0];
			gameDir = gameDirStr.c_str();
			LOG_INFO("[main] Auto-detected game: {}\n", detectedGames[0]);
		} else if (detectedGames.size() > 1) {
			LOG_ERROR("[main] Multiple games found. Use --game <name> to select one:\n");
			for (const auto& g : detectedGames) {
				LOG_ERROR("  --game {}\n", g);
			}
			return 1;
		} else if (access("Doom 2 RPG.ipa", F_OK) == 0) {
			// IPA found but not yet converted — run converter automatically
			LOG_INFO("[main] Found 'Doom 2 RPG.ipa' but no converted assets.\n");
			LOG_INFO("[main] Running drpg-convert to extract game assets...\n");

			std::string converterCmd;
			std::string selfDir;
			{
				char selfPath[4096] = {};
#ifdef __APPLE__
				uint32_t selfPathSize = sizeof(selfPath);
				if (_NSGetExecutablePath(selfPath, &selfPathSize) == 0) {
					char* slash = strrchr(selfPath, '/');
					if (slash) {
						selfDir = std::string(selfPath, slash - selfPath + 1);
					}
				}
#elif defined(__linux__)
				ssize_t len = readlink("/proc/self/exe", selfPath, sizeof(selfPath) - 1);
				if (len > 0) {
					selfPath[len] = '\0';
					char* slash = strrchr(selfPath, '/');
					if (slash) {
						selfDir = std::string(selfPath, slash - selfPath + 1);
					}
				}
#endif
			}

			if (!selfDir.empty()) {
				std::string candidates[] = {
					selfDir + "drpg-convert",
					selfDir + "converter/drpg-convert",
					selfDir + "../../../converter/drpg-convert",  // macOS .app bundle: MacOS/ -> Contents/ -> .app/ -> src/
				};
				for (const auto& c : candidates) {
					if (access(c.c_str(), X_OK) == 0) {
						converterCmd = "\"" + c + "\"";
						break;
					}
				}
			}
			if (converterCmd.empty()) {
				converterCmd = "drpg-convert";
			}

			std::string cmd = converterCmd + " --ipa \"Doom 2 RPG.ipa\" --output games/doom2rpg";
			LOG_INFO("[main]   {}\n", cmd);
			int ret = system(cmd.c_str());
			if (ret != 0) {
				LOG_ERROR("[main] Converter failed (exit code {})\n", ret);
				LOG_ERROR("[main] You can also run it manually:\n  {}\n", cmd);
				return 1;
			}

			if (access("games/doom2rpg/game.yaml", F_OK) == 0) {
				gameDirStr = "games/doom2rpg";
				gameDir = gameDirStr.c_str();
				LOG_INFO("[main] Auto-conversion complete. Game directory: {}\n", gameDir);
			} else {
				LOG_ERROR("[main] Conversion ran but game.yaml not found in games/doom2rpg/\n");
				return 1;
			}
		}
	}

	// Resolve script and mod paths to absolute before chdir
	std::string scriptPathStr;
	if (scriptFile) {
		char resolved[PATH_MAX];
		if (realpath(scriptFile, resolved)) {
			scriptPathStr = resolved;
			scriptFile = scriptPathStr.c_str();
		}
	}
	for (auto& modDir : modDirs) {
		char resolved[PATH_MAX];
		if (realpath(modDir.c_str(), resolved)) {
			modDir = resolved;
		}
	}

	// Change working directory to gamedir so config files are found there
	if (strcmp(gameDir, ".") != 0) {
		if (chdir(gameDir) != 0) {
			LOG_ERROR("[main] Cannot change to gamedir '{}'\n", gameDir);
			return 1;
		}
		LOG_INFO("[main] Working directory: {}\n", gameDir);
	}

	// Load game.yaml early (before VFS) for game name, save dir, etc.
	{
		DataNode config = DataNode::loadFile("game.yaml");
		if (!config) {
			LOG_ERROR("[main] Could not load game.yaml in '{}'\n", gameDir);
			LOG_ERROR("[main] Use --game <name> to select a game from games/ directory.\n");
			return 1;
		}
		DataNode game = config["game"];
		if (game) {
			GameConfig& gc = CAppContainer::getInstance()->gameConfig;
			if (game["module"]) gc.module = game["module"].asString();
			if (game["name"]) gc.name = game["name"].asString();
			if (game["window_title"]) gc.windowTitle = game["window_title"].asString();
			else gc.windowTitle = gc.name;
			if (game["save_dir"]) gc.saveDir = game["save_dir"].asString();
			if (game["entry_map"]) gc.entryMap = game["entry_map"].asString();

			DataNode searchDirs = game["search_dirs"];
			if (searchDirs) {
				for (auto it = searchDirs.begin(); it != searchDirs.end(); ++it) {
					gc.searchDirs.push_back(it.value().asString());
				}
			}

			DataNode entitiesNode = game["entities"];
			if (entitiesNode) {
				gc.entityFiles.clear();
				for (auto it = entitiesNode.begin(); it != entitiesNode.end(); ++it) {
					gc.entityFiles.push_back(it.value().asString());
				}
			}

			DataNode stringsNode = game["strings"];
			if (stringsNode && stringsNode.isMap()) {
				for (auto it = stringsNode.begin(); it != stringsNode.end(); ++it) {
					int groupId = std::atoi(it.key().asString().c_str());
					std::string filePath = it.value().asString("");
					if (!filePath.empty()) {
						gc.stringFiles[groupId] = filePath;
					}
				}
			}

			// Parse per-level directory mappings
			DataNode levelsNode = game["levels"];
			if (levelsNode && levelsNode.isMap()) {
				for (auto it = levelsNode.begin(); it != levelsNode.end(); ++it) {
					int mapId = std::atoi(it.key().asString().c_str());
					std::string dir = it.value().asString("");
					if (!dir.empty() && mapId > 0) {
						LevelInfo info;
						info.dir = dir;
						info.mapFile = dir + "/map.bin";
						info.modelFile = dir + "/model.bin";
						info.configFile = dir + "/level.yaml";
						gc.levelInfos[mapId] = std::move(info);
					}
				}
			}

			gc.maxEntities = game["max_entities"].asInt(gc.maxEntities);
			gc.maxWeaponButtons = game["max_weapon_buttons"].asInt(gc.maxWeaponButtons);

			// Combat section
			DataNode combat = game["combat"];
			if (combat) {
				gc.combatMaxActiveMissiles = combat["max_active_missiles"].asInt(gc.combatMaxActiveMissiles);
				gc.combatPlacingBombTime = combat["placing_bomb_time"].asInt(gc.combatPlacingBombTime);
				gc.combatBombRecoverTime = combat["bomb_recover_time"].asInt(gc.combatBombRecoverTime);
				gc.combatWeaponScale = combat["weapon_scale"].asInt(gc.combatWeaponScale);
				gc.combatLowerWeaponTime = combat["lower_weapon_time"].asInt(gc.combatLowerWeaponTime);
				gc.combatLoweredWeaponY = combat["lowered_weapon_y"].asInt(gc.combatLoweredWeaponY);
				gc.combatExplosionOffset = combat["explosion_offset"].asInt(gc.combatExplosionOffset);
				gc.combatExplosionOffset2 = combat["explosion_offset2"].asInt(gc.combatExplosionOffset2);
				gc.combatPlacingBombZ = combat["placing_bomb_z"].asInt(gc.combatPlacingBombZ);
				gc.combatRockTimeDamage = combat["rock_time_damage"].asInt(gc.combatRockTimeDamage);
				gc.combatRockTimeDodge = combat["rock_time_dodge"].asInt(gc.combatRockTimeDodge);
				gc.combatRockDistCombat = combat["rock_dist_combat"].asInt(gc.combatRockDistCombat);
			}

			// HUD section
			DataNode hud = game["hud"];
			if (hud) {
				gc.hudMsgDisplayTime = hud["msg_display_time"].asInt(gc.hudMsgDisplayTime);
				gc.hudMsgFlashTime = hud["msg_flash_time"].asInt(gc.hudMsgFlashTime);
				gc.hudScrollStartDelay = hud["scroll_start_delay"].asInt(gc.hudScrollStartDelay);
				gc.hudMsPerChar = hud["ms_per_char"].asInt(gc.hudMsPerChar);
				gc.hudBubbleTextTime = hud["bubble_text_time"].asInt(gc.hudBubbleTextTime);
				gc.hudSentryBotIconsPadding = hud["sentry_bot_icons_padding"].asInt(gc.hudSentryBotIconsPadding);
				gc.hudDamageOverlayTime = hud["damage_overlay_time"].asInt(gc.hudDamageOverlayTime);
				gc.hudActionIconSize = hud["action_icon_size"].asInt(gc.hudActionIconSize);
				gc.hudArrowsSize = hud["arrows_size"].asInt(gc.hudArrowsSize);
			}

			// Render section
			DataNode renderCfg = game["render"];
			if (renderCfg) {
				gc.renderChangemapFadeTime = renderCfg["changemap_fade_time"].asInt(gc.renderChangemapFadeTime);
				gc.renderHoldBrightnessTime = renderCfg["hold_brightness_time"].asInt(gc.renderHoldBrightnessTime);
				gc.renderFadeBrightnessTime = renderCfg["fade_brightness_time"].asInt(gc.renderFadeBrightnessTime);
				gc.renderMaxVScrollVelocity = renderCfg["max_vscroll_velocity"].asInt(gc.renderMaxVScrollVelocity);
				gc.renderMaxInitialVScrollVelocity = renderCfg["max_initial_vscroll_velocity"].asInt(gc.renderMaxInitialVScrollVelocity);
				gc.renderPortalMaxRadius = renderCfg["portal_max_radius"].asInt(gc.renderPortalMaxRadius);
				gc.renderAnimIdleTime = renderCfg["anim_idle_time"].asInt(gc.renderAnimIdleTime);
				gc.renderAnimIdleSwitchTime = renderCfg["anim_idle_switch_time"].asInt(gc.renderAnimIdleSwitchTime);
				gc.renderMaxDizzy = renderCfg["max_dizzy"].asInt(gc.renderMaxDizzy);
				gc.renderLatencyAdjust = renderCfg["latency_adjust"].asInt(gc.renderLatencyAdjust);
				gc.renderFearEyeSize = renderCfg["fear_eye_size"].asInt(gc.renderFearEyeSize);
			}

			// Player section
			DataNode player = game["player"];
			if (player) {
				gc.startingMaxHealth = player["starting_max_health"].asInt(gc.startingMaxHealth);
				gc.outOfCombatTurns = player["out_of_combat_turns"].asInt(gc.outOfCombatTurns);
				gc.playerExpireDuration = player["expire_duration"].asInt(gc.playerExpireDuration);
				gc.playerMaxDisplayBuffs = player["max_display_buffs"].asInt(gc.playerMaxDisplayBuffs);
				gc.playerIceFogDist = player["ice_fog_dist"].asInt(gc.playerIceFogDist);
				gc.playerMaxNotebookIndexes = player["max_notebook_indexes"].asInt(gc.playerMaxNotebookIndexes);
				gc.playerBitsPerVmTry = player["bits_per_vm_try"].asInt(gc.playerBitsPerVmTry);

				DataNode lu = player["level_up"];
				if (lu) {
					gc.levelUpHealth = lu["health"].asInt(gc.levelUpHealth);
					gc.levelUpDefense = lu["defense"].asInt(gc.levelUpDefense);
					gc.levelUpStrength = lu["strength"].asInt(gc.levelUpStrength);
					gc.levelUpAccuracy = lu["accuracy"].asInt(gc.levelUpAccuracy);
					gc.levelUpAgility = lu["agility"].asInt(gc.levelUpAgility);
				}

				DataNode xp = player["xp_formula"];
				if (xp) {
					gc.xpLinear = xp["linear"].asInt(gc.xpLinear);
					gc.xpCubic = xp["cubic"].asInt(gc.xpCubic);
				}

				DataNode caps = player["caps"];
				if (caps) {
					gc.capCredits = caps["credits"].asInt(gc.capCredits);
					gc.capInventory = caps["inventory"].asInt(gc.capInventory);
					gc.capAmmo = caps["ammo"].asInt(gc.capAmmo);
					gc.capBotFuel = caps["bot_fuel"].asInt(gc.capBotFuel);
				}
			}

			DataNode sc = game["scoring"];
			if (sc) {
				gc.scorePerLevel = sc["per_level"].asInt(gc.scorePerLevel);
				gc.scoreAllLevelsBonus = sc["all_levels_bonus"].asInt(gc.scoreAllLevelsBonus);
				gc.scoreNoDeathsBonus = sc["no_deaths_bonus"].asInt(gc.scoreNoDeathsBonus);
				gc.scoreDeathPenaltyBase = sc["death_penalty_base"].asInt(gc.scoreDeathPenaltyBase);
				gc.scoreDeathPenaltyMult = sc["death_penalty_mult"].asInt(gc.scoreDeathPenaltyMult);
				gc.scoreManyDeathsPenalty = sc["many_deaths_penalty"].asInt(gc.scoreManyDeathsPenalty);
				gc.scoreDeathThreshold = sc["death_threshold"].asInt(gc.scoreDeathThreshold);
				gc.scoreTimeBonusMinutes = sc["time_bonus_minutes"].asInt(gc.scoreTimeBonusMinutes);
				gc.scoreTimeBonusMult = sc["time_bonus_mult"].asInt(gc.scoreTimeBonusMult);
				gc.scoreMoveThreshold = sc["move_threshold"].asInt(gc.scoreMoveThreshold);
				gc.scoreMoveDivisor = sc["move_divisor"].asInt(gc.scoreMoveDivisor);
				gc.scorePerSecret = sc["per_secret"].asInt(gc.scorePerSecret);
				gc.scoreAllSecretsBonus = sc["all_secrets_bonus"].asInt(gc.scoreAllSecretsBonus);
			}

			DataNode dvd = game["damage_vignette_dirs"];
			if (dvd) {
				gc.damageVignetteDirs.clear();
				for (auto it = dvd.begin(); it != dvd.end(); ++it) {
					gc.damageVignetteDirs.push_back(it.value().asInt(0));
				}
			}

			DataNode bc = game["bubble_colors"];
			if (bc) {
				gc.bubbleColors.clear();
				for (auto it = bc.begin(); it != bc.end(); ++it) {
					DataNode entry = it.value();
					unsigned int color = static_cast<unsigned int>(entry["color"].asLongLong(0));
					gc.bubbleColors.push_back({
						color,
						entry["offset"].asInt(0),
						entry["tail_width"].asInt(0)
					});
				}
			}

			DataNode tp = game["target_practice"];
			if (tp) {
				gc.tpHeadPoints = tp["head_points"].asInt(gc.tpHeadPoints);
				gc.tpBodyPoints = tp["body_points"].asInt(gc.tpBodyPoints);
				gc.tpLegPoints = tp["leg_points"].asInt(gc.tpLegPoints);
				gc.tpHitDisplayMs = tp["hit_display_ms"].asInt(gc.tpHitDisplayMs);
				gc.tpWeaponIdx = tp["weapon_index"].asInt(gc.tpWeaponIdx);
				gc.tpAmmoType = tp["ammo_type"].asInt(gc.tpAmmoType);
				gc.tpAmmoCount = tp["ammo_count"].asInt(gc.tpAmmoCount);
			}

			DataNode vm = game["vending_machine"];
			if (vm) {
				gc.vendSliderMin = vm["slider_min"].asInt(gc.vendSliderMin);
				gc.vendSliderMax = vm["slider_max"].asInt(gc.vendSliderMax);
				gc.vendSliderStart = vm["slider_start"].asInt(gc.vendSliderStart);
				DataNode hints = vm["iq_hints"];
				if (hints) {
					gc.vendIQHints.clear();
					for (auto it = hints.begin(); it != hints.end(); ++it) {
						DataNode h = it.value();
						gc.vendIQHints.push_back({
							h["iq"].asInt(0),
							h["hints"].asInt(0)
						});
					}
				}
			}

			DataNode am = game["automap"];
			if (am) {
				DataNode dc = am["door_colors"];
				if (dc) {
					gc.automapDoorColors.clear();
					for (auto it = dc.begin(); it != dc.end(); ++it) {
						DataNode entry = it.value();
						int16_t tile = (int16_t)entry["tile"].asInt(0);
						uint32_t color = (uint32_t)entry["color"].asULong(0);
						gc.automapDoorColors.push_back({tile, color});
					}
				}
				gc.automapDoorDefault = (uint32_t)am["door_default_color"].asULong(gc.automapDoorDefault);
				DataNode hd = am["hidden_decors"];
				if (hd) {
					gc.automapHiddenDecors.clear();
					for (auto it = hd.begin(); it != hd.end(); ++it) {
						gc.automapHiddenDecors.push_back((int16_t)it.value().asInt(0));
					}
				}

				// Automap layout constants
				gc.automapCellSize = am["cell_size"].asInt(gc.automapCellSize);
				gc.automapOffsetX = am["offset_x"].asInt(gc.automapOffsetX);
				gc.automapOffsetY = am["offset_y"].asInt(gc.automapOffsetY);
				gc.automapBlinkInterval = am["blink_interval"].asInt(gc.automapBlinkInterval);

				// Automap entity colors
				DataNode ec = am["entity_colors"];
				if (ec) {
					auto& ac = gc.automapColors;
					ac.entrance    = (uint32_t)ec["entrance"].asULong(ac.entrance);
					ac.exit        = (uint32_t)ec["exit"].asULong(ac.exit);
					ac.visited     = (uint32_t)ec["visited"].asULong(ac.visited);
					ac.background  = (uint32_t)ec["background"].asULong(ac.background);
					ac.ladder      = (uint32_t)ec["ladder"].asULong(ac.ladder);
					ac.ladderStripe = (uint32_t)ec["ladder_stripe"].asULong(ac.ladderStripe);
					ac.npc         = (uint32_t)ec["npc"].asULong(ac.npc);
					ac.interactive = (uint32_t)ec["interactive"].asULong(ac.interactive);
					ac.monster     = (uint32_t)ec["monster"].asULong(ac.monster);
					ac.wall        = (uint32_t)ec["wall"].asULong(ac.wall);
					ac.godModeItem = (uint32_t)ec["god_mode_item"].asULong(ac.godModeItem);
					ac.questBlinkA = (uint32_t)ec["quest_blink_a"].asULong(ac.questBlinkA);
					ac.questBlinkB = (uint32_t)ec["quest_blink_b"].asULong(ac.questBlinkB);
				}
			}

			LOG_INFO("[main] Game: {} (module: {}, save: {})\n", gc.name, gc.module, gc.saveDir);
		}

		// Load per-level data from level.yaml files in each level directory
		{
			GameConfig& gc2 = CAppContainer::getInstance()->gameConfig;
			for (const auto& [mapId, info] : gc2.levelInfos) {
				DataNode levelNode = DataNode::loadFile(info.configFile.c_str());
				if (!levelNode) continue;

				// fog: false disables fog for this map (defaults to true)
				DataNode fogNode = levelNode["fog"];
				if (fogNode && !fogNode.asBool(true)) {
					gc2.noFogMaps.push_back(mapId);
				}

				DataNode skyNode = levelNode["sky_box"];
				if (skyNode) {
					gc2.levelInfos[mapId].skyBox = skyNode.asString("");
				}

				DataNode skyTexNode = levelNode["sky_texture"];
				if (skyTexNode) {
					gc2.levelInfos[mapId].skyTexture = skyTexNode.asString("");
				}

				DataNode introNode = levelNode["intro"];
				if (introNode) {
					auto& li = gc2.levelInfos[mapId];
					li.introType = introNode["type"].asString("");
					li.introFile = introNode["file"].asString("");
				}
			}
		}
	}

	const GameConfig& gc = CAppContainer::getInstance()->gameConfig;

	// Apply save directory from game config
	setSaveDir(gc.saveDir);

	VFS vfs;
	// Mount CWD at highest priority (game directory or project root)
	vfs.mountDir(".", 200);
	// Mount basedata for shared/fallback assets
	vfs.mountDir("basedata", 100);

	// Register search subdirectories from game.yaml (e.g. ui/, hud/, fonts/, audio/)
	for (const auto& dir : gc.searchDirs) {
		vfs.addSearchDir(dir.c_str());
	}

	// Discover and mount mods: auto-discover from mods/ directory + CLI --mod flags
	g_modManager.discoverMods("mods", modDirs);
	g_modManager.mountAll(vfs, 300);
	CAppContainer::getInstance()->modManager = &g_modManager;

	SDLGL sdlGL;
	if (headless) {
		sdlGL.InitializeHeadless();
	} else {
		sdlGL.Initialize();
	}

	Input input;

	// Set up the game module — custom games would provide their own IGameModule here
	DoomIIRPGGame doom2rpgModule;

	CAppContainer::getInstance()->Construct(&sdlGL, &vfs, &doom2rpgModule);

	// Initialize ImGui developer console (F12 to toggle)
	if (!headless && sdlGL.window) {
		g_devConsole.init(sdlGL.window, SDL_GL_GetCurrentContext());
		CAppContainer::getInstance()->devConsole = &g_devConsole;
	}

	input.init(); // [GEC] Port: set default Binds — must be after Construct() so app pointer exists

	if (customMap) {
		// Try to resolve --map argument against registered levels in game.yaml
		const auto& gc = CAppContainer::getInstance()->gameConfig;
		std::string mapArg(customMap);
		int resolvedID = 0;

		// Match against levelInfos directories and map files
		for (const auto& [id, info] : gc.levelInfos) {
			if (mapArg == info.dir || mapArg == info.mapFile ||
			    mapArg == info.dir + "/" || mapArg == info.dir + "/map.bin") {
				resolvedID = id;
				break;
			}
		}

		if (resolvedID > 0) {
			CAppContainer::getInstance()->customMapID = resolvedID;
			CAppContainer::getInstance()->skipTravelMap = true;
			LOG_INFO("[main] Custom map: level {} ({})\n", resolvedID,
			       gc.levelInfos.at(resolvedID).dir);
		} else {
			// Fall back to raw file path for standalone .bin files
			CAppContainer::getInstance()->customMapFile = customMap;
			CAppContainer::getInstance()->skipTravelMap = true;
			LOG_INFO("[main] Custom map file: {}\n", customMap);
		}
	}

	// Load script if specified (works in both headless and windowed modes)
	GameScript script;
	bool hasScript = false;
	if (scriptFile) {
		hasScript = script.loadFromFile(scriptFile);
		if (!hasScript) {
			LOG_ERROR("[main] Failed to load script '{}'\n", scriptFile);
			return 1;
		}
		// Auto-set ticks from script if not explicitly given
		if (maxTicks == 0) {
			maxTicks = script.lastTick() + 100; // run 100 ticks past last command
		}
	}

	if (headless) {
		// Headless game loop — deterministic fixed timestep, no window
		const int fixedTimestepMs = 15;
		int ticksRun = 0;

		LOG_INFO("[headless] Running{}{}\n",
			maxTicks > 0 ? " " + std::to_string(maxTicks) + " ticks" : "",
			hasScript ? " with script" : "");

		while (!CAppContainer::getInstance()->app->closeApplet && (maxTicks == 0 || ticksRun < maxTicks)) {
			CAppContainer::getInstance()->headlessTimeMs += fixedTimestepMs;

			if (hasScript) {
				script.injectForTick(CAppContainer::getInstance()->app, ticksRun);
			}

			CAppContainer::getInstance()->DoLoop(fixedTimestepMs);
			ticksRun++;

			if (ticksRun % 100 == 0) {
				LOG_DEBUG("[headless] tick {}/{}, state={}\n",
					ticksRun, maxTicks, CAppContainer::getInstance()->app->canvas->state);
			}
		}

		LOG_INFO("[headless] Completed {} ticks. Final state={}\n",
			ticksRun, CAppContainer::getInstance()->app->canvas->state);
	} else {
		// Normal windowed game loop
		sdlGL.updateVideo(); // [GEC]

		float x = 0.0f, y = 30.0f;
		int vp_cx = Applet::IOS_WIDTH;
		int vp_cy = Applet::IOS_HEIGHT;

		Uint8 state;
		int mX, mY;                                 /* mouse location*/
		float mousePressX = 0.f, mousePressY = 0.f; /* mouse location float*/
		int winVidWidth = sdlGL.winVidWidth;
		int winVidHeight = sdlGL.winVidHeight;
		bool useMouse = false;
		int ticksRun = 0;

		while (CAppContainer::getInstance()->app->closeApplet != true) {

			int currentTimeMillis = CAppContainer::getInstance()->getTimeMS();

			if (currentTimeMillis > UpTime) {
				input.handleEvents();

				if (hasScript) {
					script.injectForTick(CAppContainer::getInstance()->app, ticksRun);
				}

				UpTime = currentTimeMillis + 15;
				drawView(&sdlGL);
				input.consumeEvents();
				ticksRun++;

				// Auto-quit when script + ticks are done
				if (maxTicks > 0 && ticksRun >= maxTicks) {
					LOG_INFO("[script] Completed {} ticks. Exiting.\n", ticksRun);
					break;
				}
			}
		}
	}

	LOG_INFO("[main] APP_QUIT\n");
	g_devConsole.shutdown();
	CAppContainer::getInstance()->devConsole = nullptr;
	CAppContainer::getInstance()->~CAppContainer();
	logShutdown();
	_exit(0);
}

static uint32_t lastTimems = 0;

void drawView(SDLGL* sdlGL) {

	int cx, cy;
	int w = sdlGL->vidWidth;
	int h = sdlGL->vidHeight;

	if (lastTimems == 0) {
		lastTimems = CAppContainer::getInstance()->getTimeMS();
	}

	SDL_GL_GetDrawableSize(sdlGL->window, &cx, &cy);
	if (w != cx || h != cy) {
		w = cx;
		h = cy;
	}

	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, -1.0, 1.0);
	// glRotatef(90.0, 0.0, 0.0, 1.0);
	// glTranslatef(0.0, -320.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	uint32_t startTime = CAppContainer::getInstance()->getTimeMS();
	uint32_t passedTime = startTime - lastTimems;
	lastTimems = startTime;

	if (passedTime >= 125) {
		passedTime = 125;
	}
	// printf("passedTime %d\n", passedTime);

	CAppContainer::getInstance()->DoLoop(passedTime);

	// Render ImGui overlay on top of the game frame
	g_devConsole.newFrame();
	g_devConsole.render();

	SDL_GL_SwapWindow(sdlGL->window); // Swap the window/pBmp to display the result.
}
