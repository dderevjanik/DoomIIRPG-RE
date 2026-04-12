#include <algorithm>
#include <cstdio>
#include <format>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "Hud.h"
#include "Text.h"
#include "Game.h"
#include "Sound.h"
#include "Entity.h"
#include "Player.h"
#include "Combat.h"
#include "Canvas.h"
#include "Render.h"
#include "TinyGL.h"
#include "Resource.h"
#include "MayaCamera.h"
#include "LerpSprite.h"
#include "JavaStream.h"
#include "MenuSystem.h"
#include "ParticleSystem.h"
#include "Enums.h"
#include "Utils.h"
#include "SDLGL.h"
#include "Input.h"
#include "GLES.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include "SoundNames.h"
#include "Sounds.h"
#include "ConfigEnums.h"

// --- Human-readable config helpers ---

// Config enum name<->value lookups are now data-driven via config_enums.yaml and ConfigEnums class

static std::string resolutionToString(int index) {
	if (index >= 0 && index < 18) {
		return std::format("{}x{}", sdlResVideoModes[index].width, sdlResVideoModes[index].height);
	}
	return "480x320";
}

static int resolutionFromString(const std::string& str) {
	int w = 0, h = 0;
	if (sscanf(str.c_str(), "%dx%d", &w, &h) == 2) {
		for (int i = 0; i < 18; i++) {
			if (sdlResVideoModes[i].width == w && sdlResVideoModes[i].height == h) {
				return i;
			}
		}
	}
	return 0;
}

static const char* scancodeToName(int code) {
	if (code == -1) {
		return "none";
	}
	if (code & IS_MOUSE_BUTTON) {
		int btn = code & ~IS_MOUSE_BUTTON;
		switch (btn) {
			case 1:
				return "mouse_left";
			case 2:
				return "mouse_middle";
			case 3:
				return "mouse_right";
			case 4:
				return "mouse_x1";
			case 5:
				return "mouse_x2";
			default:
				return "mouse_unknown";
		}
	}
	if (code & IS_CONTROLLER_BUTTON) {
		int btn = code & ~IS_CONTROLLER_BUTTON;
		switch (btn) {
			case (int)GamepadInput::BTN_A:
				return "pad_a";
			case (int)GamepadInput::BTN_B:
				return "pad_b";
			case (int)GamepadInput::BTN_X:
				return "pad_x";
			case (int)GamepadInput::BTN_Y:
				return "pad_y";
			case (int)GamepadInput::BTN_BACK:
				return "pad_back";
			case (int)GamepadInput::BTN_GUIDE:
				return "pad_guide";
			case (int)GamepadInput::BTN_START:
				return "pad_start";
			case (int)GamepadInput::BTN_LEFT_STICK:
				return "pad_leftstick";
			case (int)GamepadInput::BTN_RIGHT_STICK:
				return "pad_rightstick";
			case (int)GamepadInput::BTN_LEFT_SHOULDER:
				return "pad_leftshoulder";
			case (int)GamepadInput::BTN_RIGHT_SHOULDER:
				return "pad_rightshoulder";
			case (int)GamepadInput::BTN_DPAD_UP:
				return "pad_dpad_up";
			case (int)GamepadInput::BTN_DPAD_DOWN:
				return "pad_dpad_down";
			case (int)GamepadInput::BTN_DPAD_LEFT:
				return "pad_dpad_left";
			case (int)GamepadInput::BTN_DPAD_RIGHT:
				return "pad_dpad_right";
			case (int)GamepadInput::BTN_LAXIS_UP:
				return "pad_laxis_up";
			case (int)GamepadInput::BTN_LAXIS_DOWN:
				return "pad_laxis_down";
			case (int)GamepadInput::BTN_LAXIS_LEFT:
				return "pad_laxis_left";
			case (int)GamepadInput::BTN_LAXIS_RIGHT:
				return "pad_laxis_right";
			case (int)GamepadInput::BTN_RAXIS_UP:
				return "pad_raxis_up";
			case (int)GamepadInput::BTN_RAXIS_DOWN:
				return "pad_raxis_down";
			case (int)GamepadInput::BTN_RAXIS_LEFT:
				return "pad_raxis_left";
			case (int)GamepadInput::BTN_RAXIS_RIGHT:
				return "pad_raxis_right";
			default:
				return "pad_unknown";
		}
	}
	// SDL scancode — use SDL_GetScancodeName
	const char* name = SDL_GetScancodeName((SDL_Scancode)code);
	if (name && name[0] != '\0') {
		return name;
	}
	return "unknown";
}

static int scancodeFromName(const std::string& name) {
	if (name == "none" || name == "-1") {
		return -1;
	}
	// Mouse buttons
	if (name == "mouse_left")
		return IS_MOUSE_BUTTON | 1;
	if (name == "mouse_middle")
		return IS_MOUSE_BUTTON | 2;
	if (name == "mouse_right")
		return IS_MOUSE_BUTTON | 3;
	if (name == "mouse_x1")
		return IS_MOUSE_BUTTON | 4;
	if (name == "mouse_x2")
		return IS_MOUSE_BUTTON | 5;
	// Controller buttons
	if (name == "pad_a")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_A;
	if (name == "pad_b")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_B;
	if (name == "pad_x")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_X;
	if (name == "pad_y")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_Y;
	if (name == "pad_back")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_BACK;
	if (name == "pad_guide")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_GUIDE;
	if (name == "pad_start")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_START;
	if (name == "pad_leftstick")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_LEFT_STICK;
	if (name == "pad_rightstick")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_RIGHT_STICK;
	if (name == "pad_leftshoulder")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_LEFT_SHOULDER;
	if (name == "pad_rightshoulder")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_RIGHT_SHOULDER;
	if (name == "pad_dpad_up")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_DPAD_UP;
	if (name == "pad_dpad_down")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_DPAD_DOWN;
	if (name == "pad_dpad_left")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_DPAD_LEFT;
	if (name == "pad_dpad_right")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_DPAD_RIGHT;
	if (name == "pad_laxis_up")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_LAXIS_UP;
	if (name == "pad_laxis_down")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_LAXIS_DOWN;
	if (name == "pad_laxis_left")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_LAXIS_LEFT;
	if (name == "pad_laxis_right")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_LAXIS_RIGHT;
	if (name == "pad_raxis_up")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_RAXIS_UP;
	if (name == "pad_raxis_down")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_RAXIS_DOWN;
	if (name == "pad_raxis_left")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_RAXIS_LEFT;
	if (name == "pad_raxis_right")
		return IS_CONTROLLER_BUTTON | (int)GamepadInput::BTN_RAXIS_RIGHT;
	// SDL scancode by name
	SDL_Scancode sc = SDL_GetScancodeFromName(name.c_str());
	if (sc != SDL_SCANCODE_UNKNOWN) {
		return (int)sc;
	}
	// Try parsing as integer for backward compatibility
	try {
		return std::stoi(name);
	} catch (...) {
		return -1;
	}
}

// Key binding action names for INI file
static const char* keyBindingNames[KEY_MAPPIN_MAX] = {"move_forward", "move_backward", "turn_left",   "turn_right",
                                                      "strafe_left",  "strafe_right",  "next_weapon", "prev_weapon",
                                                      "action",       "pass_turn",     "automap",     "menu",
                                                      "items_info",   "drinks",        "pda",         "bot_discard"};

void Game::saveWorldState(OutputStream* OS, bool b) {

	const char* name;

	short* mapSprites = app->render->mapSprites;
	int* mapSpriteInfo = app->render->mapSpriteInfo;
	app->canvas->updateLoadingBar(false);
	OS->writeInt(app->render->mapCompileDate);
	int gameTime = app->gameTime;
	this->totalLevelTime += gameTime - this->curLevelTime;
	this->curLevelTime = gameTime;
	OS->writeInt(this->totalLevelTime);
	OS->writeByte(this->mapSecretsFound);
	app->render->snapFogLerp();
	OS->writeInt(app->tinyGL->fogColor);
	OS->writeInt(app->tinyGL->fogMin);
	OS->writeInt(app->tinyGL->fogRange);
	OS->writeInt(app->render->playerFogColor);
	OS->writeInt(app->render->playerFogMin);
	OS->writeInt(app->render->playerFogRange);
	if (app->combat->getWeaponFlags(app->player->ce->weapon).isThrowableItem) {
		OS->writeByte((uint8_t)app->player->activeWeaponDef->tileIndex);
	}
	for (int i = 0; i < 4; ++i) {
		if (!b) {
			OS->writeShort(this->placedBombs[i]);
		} else {
			OS->writeShort(0);
		}
	}
	this->saveEntityStates(OS, b);
	app->canvas->updateLoadingBar(false);
	int numLines = app->render->numLines;
	int n = 0;
	int n2 = 0;
	for (int j = 0; j < numLines; ++j) {
		if (n2 == 8) {
			OS->writeByte((uint8_t)n);
			n2 = 0;
			n = 0;
		}
		if ((app->render->lineFlags[j >> 1] & 8 << ((j & 0x1) << 2)) != 0x0) {
			n |= 1 << n2;
		}
		++n2;
	}
	if (n2 != 0) {
		OS->writeByte((uint8_t)n);
	}
	int b2 = 0;
	for (int k = 0; k < 64; ++k) {
		if (this->entityDeathFunctions[k * 2] != -1) {
			b2 = (uint8_t)(b2 + 1);
		}
	}
	OS->writeByte(b2);
	for (int l = 0; l < 64; ++l) {
		if (this->entityDeathFunctions[l * 2] != -1) {
			OS->writeShort(this->entityDeathFunctions[l * 2 + 0]);
			OS->writeShort(this->entityDeathFunctions[l * 2 + 1]);
		}
	}
	int numMapSprites = app->render->numMapSprites;
	int n3 = 0;
	while (n3 < numMapSprites) {
		int v = 0;
		for (int n4 = 0; n4 < 8; ++n4, ++n3) {
			int n5 = 0;
			if (b && mapSprites[app->render->S_ENT + n3] != -1) {
				Entity* entity = &this->entities[mapSprites[app->render->S_ENT + n3]];
				EntityDef* def = entity->def;
				if (def->eType == Enums::ET_CORPSE && def->eSubType != Enums::CORPSE_SKELETON && entity->isMonster() && (entity->monsterFlags & 0x80) == 0x0) {
					n5 = 1;
				}
			}
			if (n5 == 0 && 0x0 != (app->render->mapSpriteInfo[n3] & 0x10000)) {
				n5 = 1;
			}
			if (n5 != 0) {
				v |= 1 << n4;
			}
			++n4;
			if (0x0 != (mapSpriteInfo[n3] & 0x200000)) {
				v |= 1 << n4;
			}
		}
		OS->writeByte(v);
	}
	for (int n6 = 1024, n7 = 0; n7 < n6; n7 += 8) {
		int v2 = 0;
		for (int n8 = 0; n8 < 8; ++n8) {
			if (0x0 != (app->render->mapFlags[n7 + n8] & 0x80)) {
				v2 |= 1 << n8;
			}
		}
		OS->writeByte(v2);
	}
	short* scriptStateVars = this->scriptStateVars;
	for (int n9 = 0; n9 < 128; ++n9) {
		if (n9 != 15) {
			OS->writeShort(scriptStateVars[n9]);
		}
	}
	int numTileEvents = app->render->numTileEvents;
	int n10 = 0;
	int n11 = 0;
	for (int n12 = 0; n12 < numTileEvents; ++n12) {
		n11 = n12 % 32;
		n10 |= (app->render->tileEvents[n12 * 2 + 1] & 0x80000) >> 19 << n11;
		if (n11 == 31) {
			OS->writeInt(n10);
			n10 = 0;
		}
	}
	if (n11 != 31) {
		OS->writeInt(n10);
	}
	app->canvas->updateLoadingBar(false);
	OS->writeByte(this->numLerpSprites);
	int n13 = 0;
	for (int n14 = 0; n14 < 16; ++n14) {
		LerpSprite lerpSprite = this->lerpSprites[n14];
		if (lerpSprite.hSprite != 0) {
			lerpSprite.saveState(OS);
			++n13;
		}
	}
	OS->writeByte(this->numScriptThreads);
	for (int n15 = 0; n15 < 20; ++n15) {
		this->scriptThreads[n15].saveState(OS);
	}
	OS->writeInt(this->dropIndex);
	OS->writeInt(app->player->moves);
	OS->writeByte((uint8_t)app->player->numNotebookIndexes);
	OS->writeByte(app->player->questComplete);
	OS->writeByte(app->player->questFailed);
	for (int n16 = 0; n16 < 8; ++n16) {
		OS->writeShort(app->player->notebookIndexes[n16]);
		OS->writeShort(app->player->notebookPositions[n16]);
	}
	OS->writeByte(app->render->mapFlagsBitmask);
	OS->writeByte((uint8_t)(app->render->useMastermindHack ? 1 : 0));
	OS->writeByte((uint8_t)(app->render->useCaldexHack ? 1 : 0));
	if (this->watchLine == nullptr) {
		OS->writeShort(-1);
	} else {
		OS->writeShort(this->watchLine->getIndex());
	}
	int numMallocsForVIOS = this->numMallocsForVIOS;
	if (this->angryVIOS) {
		numMallocsForVIOS |= 0x40000000;
	}
	OS->writeInt(numMallocsForVIOS);
	OS->writeShort(this->lootFound);
	OS->writeShort(this->destroyedObj);
	app->canvas->updateLoadingBar(false);
}

void Game::loadWorldState() {

	LOG_INFO("[game] loadWorldState: mapId=%d activeLoadType=%d\n", app->canvas->loadMapID, this->activeLoadType);
	const char* name;
	InputStream IS;

	if (app->canvas->loadMapID > 10) {
		return;
	}

	if (this->activeLoadType == 1) {
		name = this->GetSaveFile(Game::FILE_NAME_FULLWORLD, 0);
	} else {
		name = this->GetSaveFile(Game::FILE_NAME_BRIEFWORLD, app->canvas->loadMapID - 1);
	}

	if (!IS.loadFile(name, LT_FILE) || (this->loadWorldState(&IS) == 0)) {
		this->totalLevelTime = 0;
	}
}

bool Game::loadWorldState(InputStream* IS) {


	int mapCompileDate = IS->readInt();
	if (app->render->mapCompileDate == mapCompileDate) {
		this->totalLevelTime = IS->readInt();
		this->mapSecretsFound = IS->readByte();
		app->tinyGL->fogColor = IS->readInt();
		app->tinyGL->fogMin = IS->readInt();
		app->tinyGL->fogRange = IS->readInt();
		app->render->playerFogColor = IS->readInt();
		app->render->playerFogMin = IS->readInt();
		app->render->playerFogRange = IS->readInt();

		app->canvas->updateLoadingBar(false);
		app->render->buildFogTables(app->tinyGL->fogColor);

		app->canvas->updateLoadingBar(false);
		if (app->combat->getWeaponFlags(app->player->ce->weapon).isThrowableItem) {
			app->player->setPickUpWeapon(IS->readByte() & 0xFF);
		}
		for (int i = 0; i < 4; ++i) {
			this->placedBombs[i] = IS->readShort();
		}
		this->loadEntityStates(IS);

		app->canvas->updateLoadingBar(false);
		int numLines = app->render->numLines;
		uint8_t byte1 = 0;
		int n = 8;
		for (int j = 0; j < numLines; ++j) {
			if (n == 8) {
				byte1 = IS->readByte();
				n = 0;
			}
			if ((byte1 & 1 << n) != 0x0) {
				app->render->lineFlags[j >> 1] |= (uint8_t)(8 << ((j & 0x1) << 2));
			}
			++n;
		}
		for (uint8_t byte2 = IS->readByte(), b = 0; b < byte2; ++b) {
			this->entityDeathFunctions[b * 2 + 0] = IS->readShort();
			this->entityDeathFunctions[b * 2 + 1] = IS->readShort();
		}

		app->canvas->updateLoadingBar(false);
		int numMapSprites = app->render->numMapSprites;
		int k = 0;
		while (k < numMapSprites) {
			uint8_t byte3 = IS->readByte();
			for (int l = 0; l < 8; ++l, ++k) {
				if ((byte3 & 1 << l) != 0x0) {
					app->render->mapSpriteInfo[k] |= 0x10000;
				} else {
					app->render->mapSpriteInfo[k] &= ~0x10000;
				}
				++l;
				if ((byte3 & 1 << l) != 0x0) {
					app->render->mapSpriteInfo[k] |= 0x200000;
				} else {
					app->render->mapSpriteInfo[k] &= ~0x200000;
				}
			}
		}

		app->canvas->updateLoadingBar(false);
		for (int n7 = 1024, n8 = 0; n8 < n7; n8 += 8) {
			uint8_t byte4 = IS->readByte();
			for (int n9 = 0; n9 < 8; ++n9) {
				if ((byte4 & 1 << n9) != 0x0) {
					app->render->mapFlags[n8 + n9] |= (uint8_t)0x80;
				}
			}
		}
		short* scriptStateVars = this->scriptStateVars;
		for (int n11 = 0; n11 < 128; ++n11) {
			if (n11 != 15) {
				scriptStateVars[n11] = IS->readShort();
			}
		}

		app->canvas->updateLoadingBar(false);
		int numTileEvents = app->render->numTileEvents;
		int n12 = IS->readInt();
		for (int n13 = 0; n13 < numTileEvents; ++n13) {
			int n14 = n13 % 32;
			app->render->tileEvents[n13 * 2 + 1] =
			    ((app->render->tileEvents[n13 * 2 + 1] & 0xFFF7FFFF) | (n12 >> n14 & 0x1) << 19);
			if (n14 == 31 && n13 < numTileEvents - 1) {
				n12 = IS->readInt();
			}
		}

		this->numLerpSprites = IS->readByte();
		for (int n15 = 0; n15 < 16; ++n15) {
			LerpSprite* lerpSprite = &this->lerpSprites[n15];
			if (n15 < this->numLerpSprites) {
				lerpSprite->loadState(IS);
			} else {
				lerpSprite->hSprite = 0;
			}
		}

		this->numScriptThreads = IS->readByte();
		for (int n16 = 0; n16 < 20; ++n16) {
			ScriptThread* scriptThread = &this->scriptThreads[n16];
			scriptThread->loadState(IS);
			if (scriptThread->stackPtr > 0) {
				scriptThread->inuse = true;
			} else {
				scriptThread->inuse = false;
			}
		}

		this->updateLerpSprites();

		this->dropIndex = IS->readInt();
		app->player->moves = IS->readInt();
		app->player->numNotebookIndexes = IS->readByte();
		app->player->questComplete = IS->readByte();
		app->player->questFailed = IS->readByte();
		for (int n17 = 0; n17 < 8; ++n17) {
			app->player->notebookIndexes[n17] = IS->readShort();
			app->player->notebookPositions[n17] = IS->readShort();
		}
		app->render->mapFlagsBitmask = IS->readByte();
		app->render->useMastermindHack = (IS->readByte() == 1);
		app->render->useCaldexHack = (IS->readByte() == 1);
		short short1 = IS->readShort();
		if (short1 != -1) {
			this->watchLine = &this->entities[short1];
		}
		int numMallocsForVIOS = IS->readInt();
		this->numMallocsForVIOS = (numMallocsForVIOS & 0x3fffffff);
		this->angryVIOS = (numMallocsForVIOS & 0x40000000) ? true : false;
		this->lootFound = IS->readShort();
		this->destroyedObj = IS->readShort();
		app->hud->playerDestHealth = 0;
		app->hud->playerStartHealth = 0;
		app->hud->playerHealthChangeTime = 0;
		this->prepareMonsters();
		return true;
	} else {
		if (mapCompileDate) {
			this->spawnParam = 0;
		}
		this->totalLevelTime = 0;
		return false;
	}
}

void Game::saveConfig() {

	SDLGL* sdlGL = CAppContainer::getInstance()->sdlGL;
	YAML::Emitter out;
	out << YAML::BeginMap;

	// General
	out << YAML::Key << "version" << YAML::Value << 11;
	out << YAML::Key << "difficulty" << YAML::Value << ConfigEnums::difficultyToString(this->difficulty);
	out << YAML::Key << "language" << YAML::Value << ConfigEnums::languageToString(app->localization->defaultLanguage);

	// Sound
	out << YAML::Key << "sound_enabled" << YAML::Value << app->sound->allowSounds;
	out << YAML::Key << "fx_volume" << YAML::Value << app->sound->soundFxVolume;
	out << YAML::Key << "music_volume" << YAML::Value << app->sound->musicVolume;

	// Controls
	out << YAML::Key << "vibrate_enabled" << YAML::Value << app->canvas->vibrateEnabled;
	out << YAML::Key << "vibration_intensity" << YAML::Value << gVibrationIntensity;
	out << YAML::Key << "control_layout" << YAML::Value << ConfigEnums::controlLayoutToString(app->canvas->m_controlLayout);
	out << YAML::Key << "control_alpha" << YAML::Value << app->canvas->m_controlAlpha;
	out << YAML::Key << "flip_controls" << YAML::Value << app->canvas->isFlipControls;
	out << YAML::Key << "dead_zone" << YAML::Value << gDeadZone;

	// Display
	out << YAML::Key << "window_mode" << YAML::Value << ConfigEnums::windowModeToString(sdlGL->windowMode);
	out << YAML::Key << "vsync" << YAML::Value << sdlGL->vSync;
	out << YAML::Key << "resolution" << YAML::Value << resolutionToString(sdlGL->resolutionIndex);
	out << YAML::Key << "anim_frames" << YAML::Value << app->canvas->animFrames;
	out << YAML::Key << "gles_init" << YAML::Value << _glesObj->isInit;

	// Help
	out << YAML::Key << "help_enabled" << YAML::Value << app->player->enableHelp;
	out << YAML::Key << "help_bitmask" << YAML::Value << app->player->helpBitmask;
	out << YAML::Key << "inv_help_bitmask" << YAML::Value << app->player->invHelpBitmask;
	out << YAML::Key << "ammo_help_bitmask" << YAML::Value << app->player->ammoHelpBitmask;
	out << YAML::Key << "weapon_help_bitmask" << YAML::Value << app->player->weaponHelpBitmask;
	out << YAML::Key << "armor_help_bitmask" << YAML::Value << app->player->armorHelpBitmask;

	// Stats
	out << YAML::Key << "current_level_deaths" << YAML::Value << app->player->currentLevelDeaths;
	out << YAML::Key << "total_deaths" << YAML::Value << app->player->totalDeaths;
	this->totalPlayTime += (app->upTimeMs - this->lastSaveTime) / 1000;
	out << YAML::Key << "total_play_time" << YAML::Value << this->totalPlayTime;
	out << YAML::Key << "current_grades" << YAML::Value << app->player->currentGrades;
	out << YAML::Key << "best_grades" << YAML::Value << app->player->bestGrades;
	out << YAML::Key << "has_seen_intro" << YAML::Value << this->hasSeenIntro;
	out << YAML::Key << "recent_brief_save" << YAML::Value << app->canvas->recentBriefSave;
	this->lastSaveTime = app->upTimeMs;

	// LevelLoads
	out << YAML::Key << "level_loads" << YAML::Value << YAML::Flow << YAML::BeginSeq;
	for (int j = 0; j < 10; ++j) {
		out << this->numLevelLoads[j];
	}
	out << YAML::EndSeq;

	// Menu indexes
	out << YAML::Key << "menu_indexes" << YAML::Value << YAML::Flow << YAML::BeginSeq;
	for (int i = 0; i < 10; ++i) {
		out << app->menuSystem->indexes[i];
	}
	out << YAML::EndSeq;

	// KeyBindings
	out << YAML::Key << "key_bindings" << YAML::Value << YAML::BeginMap;
	for (int i = 0; i < KEY_MAPPIN_MAX; i++) {
		out << YAML::Key << keyBindingNames[i] << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int j = 0; j < KEYBINDS_MAX; j++) {
			out << scancodeToName(keyMapping[i].keyBinds[j]);
		}
		out << YAML::EndSeq;
	}
	out << YAML::EndMap;

	out << YAML::EndMap;

	const char* name = this->getProfileSaveFileName("config.yaml");
	std::ofstream fout(name);
	if (!fout) {
		app->Error("I/O Error in saveConfig");
		return;
	}
	fout << out.c_str() << "\n";
}

void Game::loadConfig() {

	SDLGL* sdlGL = CAppContainer::getInstance()->sdlGL;

	const char* name = this->getProfileSaveFileName("config.yaml");
	YAML::Node cfg;
	try {
		cfg = YAML::LoadFile(name);
	} catch (const YAML::Exception&) {
		return; // Config file doesn't exist, use defaults
	}

	if (cfg["version"].as<int>(0) != 11) {
		return; // Invalid or old version, use defaults
	}

	// General
	this->difficulty = ConfigEnums::difficultyFromString(cfg["difficulty"].as<std::string>("normal"));
	int lang = ConfigEnums::languageFromString(cfg["language"].as<std::string>("english"));
	if (lang != app->localization->defaultLanguage) {
		app->localization->setLanguage(lang);
	}

	// Sound
	app->sound->allowSounds = cfg["sound_enabled"].as<bool>(true);
	app->canvas->areSoundsAllowed = app->sound->allowSounds;
	app->sound->soundFxVolume = cfg["fx_volume"].as<int>(100);
	app->sound->musicVolume = cfg["music_volume"].as<int>(100);

	// Controls
	app->canvas->vibrateEnabled = cfg["vibrate_enabled"].as<bool>(true);
	gVibrationIntensity = cfg["vibration_intensity"].as<int>(100);
	app->canvas->m_controlLayout = ConfigEnums::controlLayoutFromString(cfg["control_layout"].as<std::string>("chevrons"));
	if (app->canvas->m_controlLayout > 2) {
		app->canvas->m_controlLayout = 0;
	}
	app->canvas->m_controlAlpha = cfg["control_alpha"].as<int>(255);
	app->canvas->isFlipControls = cfg["flip_controls"].as<bool>(false);
	gDeadZone = cfg["dead_zone"].as<int>(8000);

	// Display
	sdlGL->windowMode = ConfigEnums::windowModeFromString(cfg["window_mode"].as<std::string>("windowed"));
	sdlGL->vSync = cfg["vsync"].as<bool>(true);
	sdlGL->resolutionIndex = resolutionFromString(cfg["resolution"].as<std::string>("480x320"));
	int animFrames = cfg["anim_frames"].as<int>(8);
	if (animFrames < 2) {
		animFrames = 2;
	} else if (animFrames > 64) {
		animFrames = 64;
	}
	app->canvas->setAnimFrames(animFrames);
	_glesObj->isInit = cfg["gles_init"].as<bool>(false);

	// Help
	app->player->enableHelp = cfg["help_enabled"].as<bool>(true);
	app->player->helpBitmask = cfg["help_bitmask"].as<int>(0);
	app->player->invHelpBitmask = cfg["inv_help_bitmask"].as<int>(0);
	app->player->ammoHelpBitmask = cfg["ammo_help_bitmask"].as<int>(0);
	app->player->weaponHelpBitmask = cfg["weapon_help_bitmask"].as<int>(0);
	app->player->armorHelpBitmask = cfg["armor_help_bitmask"].as<int>(0);

	// Stats
	app->player->currentLevelDeaths = cfg["current_level_deaths"].as<int>(0);
	app->player->totalDeaths = cfg["total_deaths"].as<int>(0);
	this->totalPlayTime = cfg["total_play_time"].as<int>(0);
	app->player->currentGrades = cfg["current_grades"].as<int>(0);
	app->player->bestGrades = cfg["best_grades"].as<int>(0);
	this->hasSeenIntro = cfg["has_seen_intro"].as<bool>(false);
	app->canvas->recentBriefSave = cfg["recent_brief_save"].as<bool>(false);
	this->lastSaveTime = app->upTimeMs;

	// LevelLoads
	if (cfg["level_loads"] && cfg["level_loads"].IsSequence()) {
		for (int j = 0; j < 10 && j < (int)cfg["level_loads"].size(); ++j) {
			this->numLevelLoads[j] = cfg["level_loads"][j].as<int>(0);
		}
	}

	// Menu indexes
	if (cfg["menu_indexes"] && cfg["menu_indexes"].IsSequence()) {
		for (int i = 0; i < 10 && i < (int)cfg["menu_indexes"].size(); ++i) {
			app->menuSystem->indexes[i] = cfg["menu_indexes"][i].as<int>(0);
		}
	}

	// KeyBindings
	if (cfg["key_bindings"] && cfg["key_bindings"].IsMap()) {
		for (int i = 0; i < KEY_MAPPIN_MAX; i++) {
			YAML::Node binds = cfg["key_bindings"][keyBindingNames[i]];
			if (binds && binds.IsSequence()) {
				for (int j = 0; j < KEYBINDS_MAX && j < (int)binds.size(); j++) {
					keyMapping[i].keyBinds[j] = scancodeFromName(binds[j].as<std::string>("none"));
				}
			}
		}
	}

	SDL_memcpy(keyMappingTemp, keyMapping, sizeof(keyMapping));
	app->sound->updateVolume();
}

void Game::saveState(int lastMapID, int loadMapID, int viewX, int viewY, int viewAngle, int viewPitch, int prevX,
                     int prevY, int saveX, int saveY, int saveZ, int saveAngle, int savePitch, int saveType) {

	const char* name;
	OutputStream OS;

	LOG_INFO("[game] saveState: lastMap=%d loadMap=%d saveType=%d\n", lastMapID, loadMapID, saveType);
	app->canvas->recentBriefSave = ((saveType & 0x20) != 0x0);
	bool briefSave = app->canvas->recentBriefSave;
	app->canvas->freeRuntimeData();
	app->canvas->updateLoadingBar(false);

	if ((saveType & 0x10) != 0x0) {
		app->player->levelGrade(true);
	}

	this->saveConfig();

	if ((saveType & 0x1) != 0x0) {
		app->canvas->updateLoadingBar(false);
		if (briefSave) {
			name = this->GetSaveFile(Game::FILE_NAME_BRIEFPLAYER, 0);
		} else {
			name = this->GetSaveFile(Game::FILE_NAME_FULLPLAYER, 0);
		}

		if (OS.openFile(name, 1)) {
			if (!this->savePlayerState(&OS, loadMapID, viewX, viewY, viewAngle, viewPitch, prevX, prevY, saveX, saveY,
			                           saveZ, saveAngle, savePitch)) {
				OS.~OutputStream();
				OS.close();
				return;
			}
			OS.close();
		} else {
			app->Error("saveState: failed to open player file");
		}
	}

	if ((saveType & 0x2) != 0x0) {
		app->canvas->updateLoadingBar(false);
		if (briefSave) {
			name = this->GetSaveFile(Game::FILE_NAME_BRIEFWORLD, (lastMapID - 1));
		} else {
			name = this->GetSaveFile(Game::FILE_NAME_FULLWORLD, 0);
		}
		if (OS.openFile(name, 1)) {
			this->saveWorldState(&OS, briefSave);
			OS.close();
		} else {
			app->Error("saveState: failed to open world file");
		}
	}

	app->canvas->loadRuntimeData();

	OS.~OutputStream();
}

void Game::saveLevelSnapshot() {

	OutputStream OS;

	const char* name = this->GetSaveFile(Game::FILE_NAME_BRIEFPLAYER, 0);
	if (OS.openFile(name, 1)) {
		app->canvas->updateLoadingBar(false);
		if (this->savePlayerState(&OS, app->canvas->loadMapID, app->canvas->viewX, app->canvas->viewY,
		                          app->canvas->viewAngle, app->canvas->viewPitch, app->canvas->viewX,
		                          app->canvas->viewY, app->canvas->viewX, app->canvas->viewY, app->canvas->viewZ,
		                          app->canvas->saveAngle, app->canvas->savePitch)) {
			OS.close();
			const char* name = this->GetSaveFile(Game::FILE_NAME_BRIEFWORLD, app->canvas->loadMapID - 1);
			if (OS.openFile(name, 1)) {
				app->canvas->updateLoadingBar(false);
				this->saveWorldState(&OS, true);
				OS.close();
				app->canvas->updateLoadingBar(false);
			}
		}
	}
	OS.~OutputStream();
}

bool Game::savePlayerState(OutputStream* OS, int loadMapID, int viewX, int viewY, int viewAngle, int viewPitch,
                           int prevX, int prevY, int saveX, int saveY, int saveZ, int saveAngle, int savePitch) {

	OS->writeShort(loadMapID);
	OS->writeShort(viewX);
	OS->writeShort(viewY);
	OS->writeShort(viewAngle);
	OS->writeShort(viewPitch);
	OS->writeInt(prevX);
	OS->writeInt(prevY);
	OS->writeInt(saveX);
	OS->writeInt(saveY);
	OS->writeInt(saveZ);
	OS->writeShort(saveAngle);
	OS->writeShort(savePitch);
	return app->player->saveState(OS);
}

bool Game::loadPlayerState(InputStream* IS) {

	this->saveStateMap = IS->readShort();
	app->canvas->viewX = (app->canvas->destX = IS->readShort());
	app->canvas->viewY = (app->canvas->destY = IS->readShort());
	app->canvas->viewAngle = (app->canvas->destAngle = IS->readShort());
	app->canvas->viewPitch = (app->canvas->destPitch = IS->readShort());
	app->canvas->viewZ = (app->canvas->destZ = 36);
	app->canvas->prevX = IS->readInt();
	app->canvas->prevY = IS->readInt();
	app->canvas->saveX = IS->readInt();
	app->canvas->saveY = IS->readInt();
	app->canvas->saveZ = IS->readInt();
	app->canvas->saveAngle = IS->readShort();
	app->canvas->savePitch = IS->readShort();
	this->spawnParam = 0;
	return app->player->loadState(IS);
}

bool Game::loadState(int activeLoadType) {

	const char* name;
	InputStream IS;
	bool rtn = false;

	if (activeLoadType == 1) {
		name = this->GetSaveFile(Game::FILE_NAME_FULLPLAYER, 0);
	} else if (activeLoadType == 2) {
		app->player->currentLevelDeaths = 0;
		name = this->GetSaveFile(Game::FILE_NAME_BRIEFPLAYER, 0);
	}

	if (IS.loadFile(name, LT_FILE)) {
		if (this->loadPlayerState(&IS)) {
			this->activeLoadType = activeLoadType;
			if (app->canvas->viewX != 0 && app->canvas->viewY != 0) {
				this->spawnParam = ((app->canvas->viewX >> 6 & 0x1F) | (app->canvas->viewY >> 6 & 0x1F) << 5 |
				                    (app->canvas->viewAngle & 0x3FF) >> 7 << 10);
			}
			app->canvas->loadMap(this->saveStateMap, false, false);
			this->isSaved = false;
			this->isLoaded = true;
			rtn = true;
		}
	} else {
		app->Error("loadState: failed to open player file");
	}

	IS.~InputStream();
	return rtn;
}

bool Game::hasConfig() {
	const char* name = this->getProfileSaveFileName("config.yaml");
	try {
		YAML::Node cfg = YAML::LoadFile(name);
		return cfg["version"].as<int>(0) == 11;
	} catch (const YAML::Exception&) {
		return false;
	}
}

bool Game::hasElement(const char* name) {
	if (name != nullptr) {
		char* namePath = this->getProfileSaveFileName(name);
		FILE* file = std::fopen(namePath, "rb");
		if (namePath) {
			delete[] namePath;
		}
		if (file != nullptr) {
			std::fclose(file);
			return true;
		}
	}
	return false;
}

bool Game::hasSavedState() {
	if (this->hasConfig()) {
		bool v3 = false;
		for (int i = 0; i < 10; i++) {
			if (this->hasElement(this->GetSaveFile(Game::FILE_NAME_BRIEFWORLD, i))) {
				v3 = true;
			}
		}
		if (this->hasElement(this->GetSaveFile(Game::FILE_NAME_BRIEFPLAYER, 0))) {
			if (this->hasElement(this->GetSaveFile(Game::FILE_NAME_FULLPLAYER, 0))) {
				if (v3) {
					return true;
				}
			}
		}
	}

	return false;
}

void Game::removeState(bool b) {

	char* namePath;

	if (b) {
		app->canvas->updateLoadingBar(false);
	}

	this->saveEmptyConfig();

	if (b) {
		app->canvas->updateLoadingBar(false);
	}

	namePath = this->getProfileSaveFileName(this->GetSaveFile(Game::FILE_NAME_FULLWORLD, 0));
	std::remove(namePath);
	if (namePath) {
		delete[] namePath;
	}

	if (b) {
		app->canvas->updateLoadingBar(false);
	}

	namePath = this->getProfileSaveFileName(this->GetSaveFile(Game::FILE_NAME_FULLPLAYER, 0));
	std::remove(namePath);
	if (namePath) {
		delete[] namePath;
	}

	if (b) {
		app->canvas->updateLoadingBar(false);
	}

	namePath = this->getProfileSaveFileName(this->GetSaveFile(Game::FILE_NAME_BRIEFPLAYER, 0));
	std::remove(namePath);
	if (namePath) {
		delete[] namePath;
	}

	if (b) {
		app->canvas->updateLoadingBar(false);
	}
	for (int i = 0; i < 10; i++) {
		namePath = this->getProfileSaveFileName(this->GetSaveFile(Game::FILE_NAME_BRIEFWORLD, i));
		std::remove(namePath);
		if (namePath) {
			delete[] namePath;
		}
		if (b) {
			app->canvas->updateLoadingBar(false);
		}
	}
}

void Game::saveEmptyConfig() {

	SDLGL* sdlGL = CAppContainer::getInstance()->sdlGL;
	YAML::Emitter out;
	out << YAML::BeginMap;

	// General - reset difficulty to normal, keep language
	out << YAML::Key << "version" << YAML::Value << 11;
	out << YAML::Key << "difficulty" << YAML::Value << "normal";
	out << YAML::Key << "language" << YAML::Value << ConfigEnums::languageToString(app->localization->defaultLanguage);

	// Sound - keep current settings
	out << YAML::Key << "sound_enabled" << YAML::Value << app->sound->allowSounds;
	out << YAML::Key << "fx_volume" << YAML::Value << app->sound->soundFxVolume;
	out << YAML::Key << "music_volume" << YAML::Value << app->sound->musicVolume;

	// Controls - keep current settings
	out << YAML::Key << "vibrate_enabled" << YAML::Value << app->canvas->vibrateEnabled;
	out << YAML::Key << "vibration_intensity" << YAML::Value << gVibrationIntensity;
	out << YAML::Key << "control_layout" << YAML::Value << ConfigEnums::controlLayoutToString(app->canvas->m_controlLayout);
	out << YAML::Key << "control_alpha" << YAML::Value << app->canvas->m_controlAlpha;
	out << YAML::Key << "flip_controls" << YAML::Value << app->canvas->isFlipControls;
	out << YAML::Key << "dead_zone" << YAML::Value << gDeadZone;

	// Display - keep current settings
	out << YAML::Key << "window_mode" << YAML::Value << ConfigEnums::windowModeToString(sdlGL->windowMode);
	out << YAML::Key << "vsync" << YAML::Value << sdlGL->vSync;
	out << YAML::Key << "resolution" << YAML::Value << resolutionToString(sdlGL->resolutionIndex);
	out << YAML::Key << "anim_frames" << YAML::Value << app->canvas->animFrames;
	out << YAML::Key << "gles_init" << YAML::Value << _glesObj->isInit;

	// Help - reset bitmasks
	out << YAML::Key << "help_enabled" << YAML::Value << app->player->enableHelp;
	out << YAML::Key << "help_bitmask" << YAML::Value << 0;
	out << YAML::Key << "inv_help_bitmask" << YAML::Value << 0;
	out << YAML::Key << "ammo_help_bitmask" << YAML::Value << 0;
	out << YAML::Key << "weapon_help_bitmask" << YAML::Value << 0;
	out << YAML::Key << "armor_help_bitmask" << YAML::Value << 0;

	// Stats - reset player stats but keep some data
	out << YAML::Key << "current_level_deaths" << YAML::Value << 0;
	out << YAML::Key << "total_deaths" << YAML::Value << 0;
	this->totalPlayTime += (app->upTimeMs - this->lastSaveTime) / 1000;
	out << YAML::Key << "total_play_time" << YAML::Value << this->totalPlayTime;
	out << YAML::Key << "current_grades" << YAML::Value << 0;
	out << YAML::Key << "best_grades" << YAML::Value << 0;
	out << YAML::Key << "has_seen_intro" << YAML::Value << this->hasSeenIntro;
	out << YAML::Key << "recent_brief_save" << YAML::Value << false;
	this->lastSaveTime = app->upTimeMs;

	// LevelLoads - keep level load counts
	out << YAML::Key << "level_loads" << YAML::Value << YAML::Flow << YAML::BeginSeq;
	for (int j = 0; j < 10; ++j) {
		out << this->numLevelLoads[j];
	}
	out << YAML::EndSeq;

	// Menu - reset menu indexes
	out << YAML::Key << "menu_indexes" << YAML::Value << YAML::Flow << YAML::BeginSeq;
	for (int i = 0; i < 10; ++i) {
		out << 0;
	}
	out << YAML::EndSeq;

	// KeyBindings - keep current key mappings
	out << YAML::Key << "key_bindings" << YAML::Value << YAML::BeginMap;
	for (int i = 0; i < KEY_MAPPIN_MAX; i++) {
		out << YAML::Key << keyBindingNames[i] << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int j = 0; j < KEYBINDS_MAX; j++) {
			out << scancodeToName(keyMapping[i].keyBinds[j]);
		}
		out << YAML::EndSeq;
	}
	out << YAML::EndMap;

	out << YAML::EndMap;

	const char* name = this->getProfileSaveFileName("config.yaml");
	std::ofstream fout(name);
	if (!fout) {
		app->Error("I/O Error in saveConfig");
		return;
	}
	fout << out.c_str() << "\n";
}

int Game::getSaveVersion() {
	return 11;
}

void Game::loadEntityStates(InputStream* IS) {

	for (short short1 = IS->readShort(), n = 0; n < short1; ++n) {
		app->resource->readMarker(IS);
		int index = IS->readInt();
		this->entities[index & 0xFFFF].loadState(IS, index);
	}
}
void Game::saveEntityStates(OutputStream* OS, bool b) {

	int indices[Game::DEFAULT_MAX_ENTITIES > 0 ? Game::DEFAULT_MAX_ENTITIES : 275];
	// Use heap if maxEntities exceeds default
	int* indicesPtr = indices;
	std::unique_ptr<int[]> heapIndices;
	if (this->maxEntities > Game::DEFAULT_MAX_ENTITIES) {
		heapIndices.reset(new int[this->maxEntities]);
		indicesPtr = heapIndices.get();
	}

	int16_t stateCount = 0;
	for (int i = 0; i < this->numEntities; i++) {
		int saveHandle = this->entities[i].getSaveHandle(b);
		if (saveHandle != -1) {
			indicesPtr[stateCount++] = saveHandle;
		}
	}
	OS->writeShort(stateCount);
	for (int i = 0; i < stateCount; i++) {
		Entity* entity = &this->entities[indicesPtr[i] & 0xFFFF];
		app->resource->writeMarker(OS);
		OS->writeInt(indicesPtr[i]);
		entity->saveState(OS, indicesPtr[i]);
	}
}

const char* Game::GetSaveFile(int i, int i2) {
	const char* name;
	switch (i) {
		case 0: // SDFWORLD
			name = "FWORLD";
			break;
		case 1: // SDCONFIG
			name = "CONFIGFILENAME";
			break;
		case 2: // SDBPLAYER
			name = "BRIEFPLAYER";
			break;
		case 3: // SDFPLAYER
			name = "FULLPLAYER";
			break;
		case 4: // SDBWORLD
			switch (i2) {
				case 0:
					name = "SB_1";
					break;
				case 1:
					name = "SB_2";
					break;
				case 2:
					name = "SB_3";
					break;
				case 3:
					name = "SB_4";
					break;
				case 4:
					name = "SB_5";
					break;
				case 5:
					name = "SB_6";
					break;
				case 6:
					name = "SB_7";
					break;
				case 7:
					name = "SB_8";
					break;
				case 8:
					name = "SB_9";
					break;
				case 9: // IOS Missing File
					name = "SB_10";
					break;
				default:
					name = nullptr;
					break;
			}
			break;
		default:
			name = nullptr;
			break;
	}

	return name;
}

char* Game::getProfileSaveFileName(const char* name) {
	if (name != nullptr) {
		// Ensure save directory exists
		struct stat sb;
		const char* saveDir = getSaveDir().c_str();
		if (stat(saveDir, &sb) != 0) {
#ifdef _WIN32
			_mkdir(saveDir);
#else
			mkdir(saveDir, 0755);
#endif
		}

		int len1 = std::strlen(saveDir);
		int len2 = std::strlen(name);
		char* namePath = new char[len1 + len2 + 2]; // +2 for '/' and '\0'
		std::snprintf(namePath, len1 + len2 + 2, "%s/%s", saveDir, name);
		return namePath;
	} else {
		puts("getProfileSaveFileName2: ERROR -> filename is NULL! ");
		return nullptr;
	}
}
