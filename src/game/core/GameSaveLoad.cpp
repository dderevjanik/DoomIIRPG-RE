#include <algorithm>
#include <cstdio>
#include <format>
#include <memory>
#include <print>
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
#include "Resource.h"
#include "MayaCamera.h"
#include "LerpSprite.h"
#include "BinaryStream.h"
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

// Helpers `inputCodeToName`/`inputCodeFromName` and `keyBindingNames[]` live in Input.cpp/Input.h
// so the controls.yaml defaults parser can share them with the save-game settings round-trip.

void Game::saveWorldState(OutputStream* OS, bool briefSave) {

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
	OS->writeInt(app->render->fogColor);
	OS->writeInt(app->render->fogMin);
	OS->writeInt(app->render->fogRange);
	OS->writeInt(app->render->playerFogColor);
	OS->writeInt(app->render->playerFogMin);
	OS->writeInt(app->render->playerFogRange);
	if (app->combat->getWeaponFlags(app->player->ce->weapon).isThrowableItem) {
		OS->writeByte((uint8_t)app->player->activeWeaponDef->tileIndex);
	}
	for (int i = 0; i < 4; ++i) {
		if (!briefSave) {
			OS->writeShort(this->placedBombs[i]);
		} else {
			OS->writeShort(0);
		}
	}
	this->saveEntityStates(OS, briefSave);
	app->canvas->updateLoadingBar(false);
	int numLines = app->render->numLines;
	int bitPack = 0;
	int bitIdx = 0;
	for (int j = 0; j < numLines; ++j) {
		if (bitIdx == 8) {
			OS->writeByte((uint8_t)bitPack);
			bitIdx = 0;
			bitPack = 0;
		}
		if ((app->render->lineFlags[j >> 1] & 8 << ((j & 0x1) << 2)) != 0x0) {
			bitPack |= 1 << bitIdx;
		}
		++bitIdx;
	}
	if (bitIdx != 0) {
		OS->writeByte((uint8_t)bitPack);
	}
	int numDeathFuncs = 0;
	for (int k = 0; k < 64; ++k) {
		if (this->entityDeathFunctions[k * 2] != -1) {
			numDeathFuncs = (uint8_t)(numDeathFuncs + 1);
		}
	}
	OS->writeByte(numDeathFuncs);
	for (int l = 0; l < 64; ++l) {
		if (this->entityDeathFunctions[l * 2] != -1) {
			OS->writeShort(this->entityDeathFunctions[l * 2 + 0]);
			OS->writeShort(this->entityDeathFunctions[l * 2 + 1]);
		}
	}
	int numMapSprites = app->render->numMapSprites;
	int spriteIdx = 0;
	while (spriteIdx < numMapSprites) {
		int packedByte = 0;
		for (int spriteBitIdx = 0; spriteBitIdx < 8; ++spriteBitIdx, ++spriteIdx) {
			int hideBit = 0;
			if (briefSave && mapSprites[app->render->S_ENT + spriteIdx] != -1) {
				Entity* entity = &this->entities[mapSprites[app->render->S_ENT + spriteIdx]];
				EntityDef* def = entity->def;
				if (def->eType == Enums::ET_CORPSE && def->eSubType != Enums::CORPSE_SKELETON && entity->isMonster() && (entity->monsterFlags & Enums::MFLAG_NOTRACK) == 0x0) {
					hideBit = 1;
				}
			}
			if (hideBit == 0 && 0x0 != (app->render->getSpriteInfoRaw(spriteIdx) & 0x10000)) {
				hideBit = 1;
			}
			if (hideBit != 0) {
				packedByte |= 1 << spriteBitIdx;
			}
			++spriteBitIdx;
			if (0x0 != (mapSpriteInfo[spriteIdx] & 0x200000)) {
				packedByte |= 1 << spriteBitIdx;
			}
		}
		OS->writeByte(packedByte);
	}
	for (int numCells = 1024, cellIdx = 0; cellIdx < numCells; cellIdx += 8) {
		int flagBits = 0;
		for (int cellBitIdx = 0; cellBitIdx < 8; ++cellBitIdx) {
			if (0x0 != (app->render->mapFlags[cellIdx + cellBitIdx] & 0x80)) {
				flagBits |= 1 << cellBitIdx;
			}
		}
		OS->writeByte(flagBits);
	}
	short* scriptStateVars = this->scriptStateVars;
	for (int varIdx = 0; varIdx < 128; ++varIdx) {
		if (varIdx != 15) {
			OS->writeShort(scriptStateVars[varIdx]);
		}
	}
	int numTileEvents = app->render->numTileEvents;
	int eventBits = 0;
	int eventBitIdx = 0;
	for (int eventIdx = 0; eventIdx < numTileEvents; ++eventIdx) {
		eventBitIdx = eventIdx % 32;
		eventBits |= (app->render->tileEvents[eventIdx * 2 + 1] & 0x80000) >> 19 << eventBitIdx;
		if (eventBitIdx == 31) {
			OS->writeInt(eventBits);
			eventBits = 0;
		}
	}
	if (eventBitIdx != 31) {
		OS->writeInt(eventBits);
	}
	app->canvas->updateLoadingBar(false);
	OS->writeByte(this->numLerpSprites);
	int savedLerpCount = 0;
	for (int lerpIdx = 0; lerpIdx < 16; ++lerpIdx) {
		LerpSprite lerpSprite = this->lerpSprites[lerpIdx];
		if (lerpSprite.hSprite != 0) {
			lerpSprite.saveState(OS);
			++savedLerpCount;
		}
	}
	OS->writeByte(this->numScriptThreads);
	for (int threadIdx = 0; threadIdx < 20; ++threadIdx) {
		this->scriptThreads[threadIdx].saveState(OS);
	}
	OS->writeInt(this->dropIndex);
	OS->writeInt(app->player->moves);
	OS->writeByte((uint8_t)app->player->numNotebookIndexes);
	OS->writeByte(app->player->questComplete);
	OS->writeByte(app->player->questFailed);
	for (int noteIdx = 0; noteIdx < 8; ++noteIdx) {
		OS->writeShort(app->player->notebookIndexes[noteIdx]);
		OS->writeShort(app->player->notebookPositions[noteIdx]);
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

	LOG_INFO("[game] loadWorldState: mapId={} activeLoadType={}\n", app->canvas->loadMapID, this->activeLoadType);
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
		app->render->fogColor = IS->readInt();
		app->render->fogMin = IS->readInt();
		app->render->fogRange = IS->readInt();
		app->render->playerFogColor = IS->readInt();
		app->render->playerFogMin = IS->readInt();
		app->render->playerFogRange = IS->readInt();

		app->canvas->updateLoadingBar(false);
		app->render->buildFogTables(app->render->fogColor);

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
		uint8_t lineByte = 0;
		int bitIdx = 8;
		for (int j = 0; j < numLines; ++j) {
			if (bitIdx == 8) {
				lineByte = IS->readByte();
				bitIdx = 0;
			}
			if ((lineByte & 1 << bitIdx) != 0x0) {
				app->render->lineFlags[j >> 1] |= (uint8_t)(8 << ((j & 0x1) << 2));
			}
			++bitIdx;
		}
		for (uint8_t numDeathFuncs = IS->readByte(), funcIdx = 0; funcIdx < numDeathFuncs; ++funcIdx) {
			this->entityDeathFunctions[funcIdx * 2 + 0] = IS->readShort();
			this->entityDeathFunctions[funcIdx * 2 + 1] = IS->readShort();
		}

		app->canvas->updateLoadingBar(false);
		int numMapSprites = app->render->numMapSprites;
		int spriteIdx = 0;
		while (spriteIdx < numMapSprites) {
			uint8_t spriteByte = IS->readByte();
			for (int spriteBitIdx = 0; spriteBitIdx < 8; ++spriteBitIdx, ++spriteIdx) {
				if ((spriteByte & 1 << spriteBitIdx) != 0x0) {
					app->render->setSpriteInfoFlag(spriteIdx, 0x10000);
				} else {
					app->render->setSpriteInfoRaw(spriteIdx, app->render->getSpriteInfoRaw(spriteIdx) & ~0x10000);
				}
				++spriteBitIdx;
				if ((spriteByte & 1 << spriteBitIdx) != 0x0) {
					app->render->setSpriteInfoFlag(spriteIdx, 0x200000);
				} else {
					app->render->setSpriteInfoRaw(spriteIdx, app->render->getSpriteInfoRaw(spriteIdx) & ~0x200000);
				}
			}
		}

		app->canvas->updateLoadingBar(false);
		for (int numCells = 1024, cellIdx = 0; cellIdx < numCells; cellIdx += 8) {
			uint8_t flagByte = IS->readByte();
			for (int cellBitIdx = 0; cellBitIdx < 8; ++cellBitIdx) {
				if ((flagByte & 1 << cellBitIdx) != 0x0) {
					app->render->mapFlags[cellIdx + cellBitIdx] |= (uint8_t)0x80;
				}
			}
		}
		short* scriptStateVars = this->scriptStateVars;
		for (int varIdx = 0; varIdx < 128; ++varIdx) {
			if (varIdx != 15) {
				scriptStateVars[varIdx] = IS->readShort();
			}
		}

		app->canvas->updateLoadingBar(false);
		int numTileEvents = app->render->numTileEvents;
		int eventBits = IS->readInt();
		for (int eventIdx = 0; eventIdx < numTileEvents; ++eventIdx) {
			int eventBitIdx = eventIdx % 32;
			app->render->tileEvents[eventIdx * 2 + 1] =
			    ((app->render->tileEvents[eventIdx * 2 + 1] & 0xFFF7FFFF) | (eventBits >> eventBitIdx & 0x1) << 19);
			if (eventBitIdx == 31 && eventIdx < numTileEvents - 1) {
				eventBits = IS->readInt();
			}
		}

		this->numLerpSprites = IS->readByte();
		for (int lerpIdx = 0; lerpIdx < 16; ++lerpIdx) {
			LerpSprite* lerpSprite = &this->lerpSprites[lerpIdx];
			if (lerpIdx < this->numLerpSprites) {
				lerpSprite->loadState(IS);
			} else {
				lerpSprite->hSprite = 0;
			}
		}

		this->numScriptThreads = IS->readByte();
		for (int threadIdx = 0; threadIdx < 20; ++threadIdx) {
			ScriptThread* scriptThread = &this->scriptThreads[threadIdx];
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
		for (int noteIdx = 0; noteIdx < 8; ++noteIdx) {
			app->player->notebookIndexes[noteIdx] = IS->readShort();
			app->player->notebookPositions[noteIdx] = IS->readShort();
		}
		app->render->mapFlagsBitmask = IS->readByte();
		app->render->useMastermindHack = (IS->readByte() == 1);
		app->render->useCaldexHack = (IS->readByte() == 1);
		short watchLineIdx = IS->readShort();
		if (watchLineIdx != -1) {
			this->watchLine = &this->entities[watchLineIdx];
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

	SDLGL* sdlGL = this->sdlGL;
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
			out << inputCodeToName(keyMapping[i].keyBinds[j]);
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

	SDLGL* sdlGL = this->sdlGL;

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
					keyMapping[i].keyBinds[j] = inputCodeFromName(binds[j].as<std::string>("none"));
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

	LOG_INFO("[game] saveState: lastMap={} loadMap={} saveType={}\n", lastMapID, loadMapID, saveType);
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
		bool hasWorldSave = false;
		for (int i = 0; i < 10; i++) {
			if (this->hasElement(this->GetSaveFile(Game::FILE_NAME_BRIEFWORLD, i))) {
				hasWorldSave = true;
			}
		}
		if (this->hasElement(this->GetSaveFile(Game::FILE_NAME_BRIEFPLAYER, 0))) {
			if (this->hasElement(this->GetSaveFile(Game::FILE_NAME_FULLPLAYER, 0))) {
				if (hasWorldSave) {
					return true;
				}
			}
		}
	}

	return false;
}

void Game::removeState(bool showProgress) {

	char* namePath;

	if (showProgress) {
		app->canvas->updateLoadingBar(false);
	}

	this->saveEmptyConfig();

	if (showProgress) {
		app->canvas->updateLoadingBar(false);
	}

	namePath = this->getProfileSaveFileName(this->GetSaveFile(Game::FILE_NAME_FULLWORLD, 0));
	std::remove(namePath);
	if (namePath) {
		delete[] namePath;
	}

	if (showProgress) {
		app->canvas->updateLoadingBar(false);
	}

	namePath = this->getProfileSaveFileName(this->GetSaveFile(Game::FILE_NAME_FULLPLAYER, 0));
	std::remove(namePath);
	if (namePath) {
		delete[] namePath;
	}

	if (showProgress) {
		app->canvas->updateLoadingBar(false);
	}

	namePath = this->getProfileSaveFileName(this->GetSaveFile(Game::FILE_NAME_BRIEFPLAYER, 0));
	std::remove(namePath);
	if (namePath) {
		delete[] namePath;
	}

	if (showProgress) {
		app->canvas->updateLoadingBar(false);
	}
	for (int i = 0; i < 10; i++) {
		namePath = this->getProfileSaveFileName(this->GetSaveFile(Game::FILE_NAME_BRIEFWORLD, i));
		std::remove(namePath);
		if (namePath) {
			delete[] namePath;
		}
		if (showProgress) {
			app->canvas->updateLoadingBar(false);
		}
	}
}

void Game::saveEmptyConfig() {

	SDLGL* sdlGL = this->sdlGL;
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
			out << inputCodeToName(keyMapping[i].keyBinds[j]);
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

	for (short numStates = IS->readShort(), entityIdx = 0; entityIdx < numStates; ++entityIdx) {
		app->resource->readMarker(IS);
		int index = IS->readInt();
		this->entities[index & 0xFFFF].loadState(IS, index);
	}
}
void Game::saveEntityStates(OutputStream* OS, bool briefSave) {

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
		int saveHandle = this->entities[i].getSaveHandle(briefSave);
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

const char* Game::GetSaveFile(int fileType, int mapIdx) {
	const char* name;
	switch (fileType) {
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
		case 4: // SDBWORLD — per-level brief snapshot ("SB_<N>" for map ID N = mapIdx + 1)
			if (mapIdx >= 0) {
				thread_local char sbBuf[16];
				std::snprintf(sbBuf, sizeof(sbBuf), "SB_%d", mapIdx + 1);
				name = sbBuf;
			} else {
				name = nullptr;
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
		std::println(stderr, "getProfileSaveFileName2: ERROR -> filename is NULL!");
		return nullptr;
	}
}
