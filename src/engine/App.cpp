#include <stdexcept>
#include <cstdio>
#include <algorithm>
#include <sstream>

#include "SDLGL.h"
#include "ZipFile.h"

#include "CAppContainer.h"
#include "App.h"
#include "IGameModule.h"
#include "IDIB.h"
#include "Text.h"
#include "Resource.h"
#include "Render.h"
#include "TinyGL.h"
#include "Canvas.h"
#include "Combat.h"
#include "Game.h"
#include "MenuSystem.h"
#include "Player.h"
#include "Sound.h"
#include "Hud.h"
#include "EntityDef.h"
#include "ParticleSystem.h"
#include "HackingGame.h"
#include "SentryBotGame.h"
#include "VendingMachine.h"
#include "ComicBook.h"
#include "JavaStream.h"
#include "Image.h"
#include "Graphics.h"
#include <yaml-cpp/yaml.h>
#include "Input.h"
#include "Enums.h"

Applet::Applet() {
	std::memset(this, 0, sizeof(Applet));
}

Applet::~Applet() {}

void Applet::setGameModule(IGameModule* module) {
	this->gameModule = module;
}

bool Applet::startup() {
	printf("Applet::startup\n");

	this->closeApplet = false;
	this->fontType = 0;
	this->accelerationIndex = 0;
	this->field_0x290 = '\0';
	this->field_0x291 = '\0';

	// Iphone Only
	{
		for (int i = 0; i < 32; i++) {
			accelerationX[i] = 0.0f;
			accelerationY[i] = 0.0f;
			accelerationZ[i] = 0.0f;
		}
	}

	this->field_0x414 = 0;
	this->field_0x418 = 0;
	this->field_0x41c = 0;
	this->field_0x420 = 0;
	this->field_0x424 = 0;
	this->field_0x428 = 0;

	this->backBuffer = new IDIB;
	this->backBuffer->pBmp = new uint8_t[480 * 320 * 2];
	std::memset(this->backBuffer->pBmp, 0, 480 * 320 * 2);
	this->backBuffer->pRGB888 = nullptr;
	this->backBuffer->pRGB565 = nullptr;
	this->backBuffer->width = CAppContainer::getInstance()->sdlGL->vidWidth;
	this->backBuffer->height = CAppContainer::getInstance()->sdlGL->vidHeight;
	this->backBuffer->pitch = CAppContainer::getInstance()->sdlGL->vidWidth;

	printf("w: %d || h: %d\n", backBuffer->width, backBuffer->height);

	this->initLoadImages = false;
	this->time = 0;
	this->upTimeMs = 0;
	this->field_0x7c = 0;
	this->field_0x80 = 0;
	// Engine subsystems
	this->canvas = new Canvas;
	this->resource = new Resource;
	this->localization = new Localization;
	this->render = new Render;
	this->tinyGL = new TinyGL;
	this->menuSystem = new MenuSystem;
	this->sound = new Sound;
	this->hud = new Hud;
	this->particleSystem = new ParticleSystem;

	// Game objects — created by the active game module
	if (this->gameModule) {
		printf("Applet: creating game objects via %s module\n", this->gameModule->getName());
		this->gameModule->createGameObjects(this);
	} else {
		this->Error("No game module set. Call setGameModule() before startup().");
	}

	Applet::loadConfig();
	// this->moreGames = getStartupVarBool("More_Games", false);
	// this->systemFont = getStartupVarBool("System_Font", false);
	int time = this->upTimeMs;
	this->gameTime = this->upTimeMs;
	this->startupMemory = Applet::MAXMEMORY;

	// Engine subsystem startup
	if (!this->canvas->startup()) { printf("error fatal: canvas\n"); return false; }
	this->testImg = Applet::loadImage("cockpit.bmp", true);
	this->canvas->loadMiniGameImages();
	if (!this->localization->startup()) { printf("error fatal: localization\n"); return false; }
	if (!this->render->startup()) { printf("error fatal: render\n"); return false; }
	this->resource->initTableLoading();
	this->loadTables();
	if (!this->tinyGL->startup(this->render->screenWidth, this->render->screenHeight)) {
		printf("error fatal: tinyGL\n"); return false;
	}

	// Game module startup (EntityDefManager -> Player -> Game -> Combat)
	if (!this->gameModule->startup(this)) { printf("error fatal: game module\n"); return false; }

	// Remaining engine subsystems that depend on game objects
	if (!this->menuSystem->startup()) { printf("error fatal: menuSystem\n"); return false; }
	if (!this->sound->startup()) { printf("error fatal: sound\n"); return false; }
	if (!this->particleSystem->startup()) { printf("error fatal: particleSystem\n"); return false; }

	// Game module post-startup
	this->gameModule->loadConfig(this);
	this->gameModule->registerOpcodes(this);

	if (this->canvas->isFlipControls != false) {
		this->canvas->isFlipControls = false;
		this->canvas->flipControls();
	}

	this->canvas->setControlLayout();
	this->canvas->clearEvents(1);
	this->canvas->setState(Canvas::ST_LOGO);
	this->canvas->graphics.backBuffer = this->backBuffer;
	this->canvas->graphics.graphClipRect[0] = 0;
	this->canvas->graphics.graphClipRect[1] = 0;
	this->canvas->graphics.graphClipRect[2] = this->backBuffer->width;
	this->canvas->graphics.graphClipRect[3] = this->backBuffer->height;

	this->accelerationIndex = 0;
	this->field_0x290 = false;
	this->field_0x291 = '\0';
	printf("**** Startup took %i ms\n", this->upTimeMs - time);
	printf("**** Fragment size %i ms\n", 0);

	return true;
}

void Applet::loadConfig() {}

Image* Applet::createImage(InputStream* inputStream, bool isTransparentMask) {
	Image* img;
	int Width, Height, offBeg, BitsPerPixel, ColorsUsed, rgb, pitch;
	img = (Image*)std::malloc(sizeof(Image));
	img->texture = -1;
	img->piDIB = (IDIB*)std::malloc(sizeof(IDIB));
	img->piDIB->pBmp = nullptr;
	img->piDIB->pRGB888 = nullptr;
	img->piDIB->pRGB565 = nullptr;

	// read header
	inputStream->offsetCursor(10);
	offBeg = inputStream->readInt();
	inputStream->offsetCursor(4);
	Width = inputStream->readInt();
	Height = inputStream->readInt();
	inputStream->offsetCursor(2);
	BitsPerPixel = inputStream->readShort();
	inputStream->offsetCursor(16);
	ColorsUsed = inputStream->readInt();
	inputStream->offsetCursor(4);

	// printf("offBeg %d\n", offBeg);
	// printf("Width %d\n", Width);
	// printf("Height %d\n", Height);
	// printf("BitsPerPixel %d\n", BitsPerPixel);
	// printf("ColorsUsed %d\n", ColorsUsed);

	// read data

	if (BitsPerPixel == 4 || BitsPerPixel == 8) {

		if (ColorsUsed == 0) {
			ColorsUsed = 1 << (BitsPerPixel & 0xff);
		}
		// printf("ColorsUsed %d\n", ColorsUsed);

		img->piDIB->cntRGB = ColorsUsed;
		img->piDIB->nColorScheme = 0;
		img->piDIB->pRGB888 = (uint32_t*)std::malloc(img->piDIB->cntRGB * sizeof(uint32_t));
		img->piDIB->pRGB565 = (uint16_t*)std::malloc(img->piDIB->cntRGB * sizeof(uint16_t));

		// read palette
		std::memcpy(img->piDIB->pRGB888, inputStream->getTop(), img->piDIB->cntRGB * sizeof(uint32_t));
		for (uint32_t i = 0; i < img->piDIB->cntRGB; i++) {
			img->piDIB->pRGB888[i] = SDL_SwapLE32(img->piDIB->pRGB888[i]);
		}
		inputStream->offsetCursor(img->piDIB->cntRGB * sizeof(uint32_t));

		img->isTransparentMask = isTransparentMask;

		if (isTransparentMask) {
			for (uint32_t i = 0; i < img->piDIB->cntRGB; i++) {
				rgb = img->piDIB->pRGB888[i];
				if (rgb == 0xff00ff) {
					img->piDIB->ncTransparent = i;
				} else {
					if (((rgb >> 8 & 0xf800U) | (rgb >> 5 & 0x07e0U) | ((rgb >> 3) & 0x001f)) ==
					    0) { // rgb888 to rgb565
						img->piDIB->pRGB888[i] = 8;
					}
				}
			}
		}

		for (uint32_t i = 0; i < img->piDIB->cntRGB; i++) {
			rgb = img->piDIB->pRGB888[i];
			img->piDIB->pRGB565[i] =
			    (rgb >> 8 & 0xf800) | (rgb >> 5 & 0x07e0) | (rgb >> 3 & 0x001f); // rgb888 to rgb565
		}

		// read pixels
		img->width = img->piDIB->width;
		img->height = img->piDIB->height;
		img->depth = img->piDIB->depth;

		pitch = BitsPerPixel * Width;
		int _pitch = pitch;
		if ((pitch & 7) != 0) {
			// printf("pitch 7 %d\n", pitch);
			ColorsUsed = (uint32_t)((int)pitch >> 0x1f) >> 0x1d;
			_pitch = (pitch - ((pitch + ColorsUsed & 7) - ColorsUsed)) + 8;
		}

		if ((pitch & 0x1f) != 0) {
			// printf("pitch 31 %d\n", pitch);
			ColorsUsed = (uint32_t)((int)pitch >> 0x1f) >> 0x1b;
			pitch = (pitch - ((pitch + ColorsUsed & 0x1f) - ColorsUsed)) + 0x20;
		}
		ColorsUsed = pitch;
		if ((int)pitch < 0) {
			ColorsUsed = pitch + 7;
		}
		int iVar8 = (int)ColorsUsed >> 3;
		ColorsUsed = pitch + 7;
		if ((int)_pitch < 0) {
			ColorsUsed = _pitch + 7;
		}
		int sVar6 = Height * Width;
		if ((int)_pitch >= 0) {
			ColorsUsed = _pitch;
		}

		// printf("sVar6 %d\n", sVar6);
		img->piDIB->pBmp = (uint8_t*)std::malloc(sVar6 * sizeof(int16_t));
		img->piDIB->width = Width;
		img->piDIB->pitch = Width;
		img->piDIB->depth = BitsPerPixel;
		img->piDIB->height = Height;

		bool bVar7 = BitsPerPixel != 4;

		if (bVar7) {
			Width = 0;
		}
		img->height = img->piDIB->height;
		if (!bVar7) {
			Width = Height - 1;
		}
		img->depth = img->piDIB->depth;

		uint8_t* data = inputStream->getTop();
		if (bVar7) {
			for (; Width < Height; Width = Width + 1) {
				std::memcpy(img->piDIB->pBmp + img->piDIB->pitch * Width, data + iVar8 * (Height + (-1 - Width)),
				            (int)ColorsUsed >> 3);
			}
		} else {
			for (int i = 0; i < Height; i++) {
				std::memcpy(img->piDIB->pBmp + (i * img->piDIB->pitch >> 1), data + iVar8 * Width,
				            (int)ColorsUsed >> 3);
				Width = Width - 1;
			}
		}
		inputStream->offsetCursor(iVar8 * Height);

		if ((short)BitsPerPixel == 4) {
			uint8_t* pbVar4 = (uint8_t*)std::malloc(sVar6);
			uint8_t* _data = pbVar4;
			for (ColorsUsed = 0; ColorsUsed < sVar6 >> 1; ColorsUsed = ColorsUsed + 1) {
				uint8_t bVar1 = img->piDIB->pBmp[ColorsUsed];
				_data[0] = bVar1 >> 4;
				_data[1] = bVar1 & 0xf;
				_data += 2;
			}
			for (ColorsUsed = 0; ColorsUsed < sVar6; ColorsUsed = ColorsUsed + 1) {
				img->piDIB->pBmp[ColorsUsed] = pbVar4[ColorsUsed];
			}
			std::free(pbVar4);
		}
		img->width = img->piDIB->width;
		img->height = img->piDIB->height;
	} else {
		img->~Image();
		img = nullptr;

		Error("Expected image bpp 4 or 8. Found bpp %d", BitsPerPixel);
	}

	return img;
}

Image* Applet::loadImage(char* fileName, bool isTransparentMask) {
	InputStream iStream = InputStream();
	Image* img;

	if (iStream.loadResource(fileName) == 0) {
		Applet::Error("Failed to open image: %s", fileName);
	}

	img = this->createImage(&iStream, isTransparentMask);

	iStream.close();
	iStream.~InputStream();
	return img;
}

#include <stdarg.h> //va_list|va_start|va_end
void Applet::Error(const char* fmt, ...) {

	char errMsg[256];
	va_list ap;
	va_start(ap, fmt);
#ifdef _WIN32
	vsprintf_s(errMsg, sizeof(errMsg), fmt, ap);
#else
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
#endif
	va_end(ap);

	SDL_SetWindowFullscreen(CAppContainer::getInstance()->sdlGL->window, 0);

	printf("%s", errMsg);

	const SDL_MessageBoxButtonData buttons[] = {
	    {/* .flags, .buttonid, .text */ 0, 0, "Ok"},
	};
	const SDL_MessageBoxColorScheme colorScheme = {{/* .colors (.r, .g, .b) */
	                                                /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
	                                                {255, 0, 0},
	                                                /* [SDL_MESSAGEBOX_COLOR_TEXT] */
	                                                {0, 255, 0},
	                                                /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
	                                                {255, 255, 0},
	                                                /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
	                                                {0, 0, 255},
	                                                /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
	                                                {255, 0, 255}}};
	const SDL_MessageBoxData messageboxdata = {
	    SDL_MESSAGEBOX_ERROR,                                               /* .flags */
	    NULL,                                                               /* .window */
	    (CAppContainer::getInstance()->gameConfig.name + " Error").c_str(), /* .title */
	    errMsg,                                                             /* .message */
	    SDL_arraysize(buttons),                                             /* .numbuttons */
	    buttons,                                                            /* .buttons */
	    &colorScheme                                                        /* .colorScheme */
	};

	SDL_ShowMessageBox(&messageboxdata, NULL);

	CAppContainer::getInstance()->~CAppContainer();
	exit(0);
}

void Applet::Error(int id) {
	std::printf("Error id: %i\n", id);
	this->Error("App Error");
	this->idError = id;
}

void Applet::beginImageLoading() {}

void Applet::endImageLoading() {}

void Applet::loadTables() {
	// Try loading from tables.yaml in CWD first
	FILE* f = std::fopen("tables.yaml", "rb");
	if (f) {
		std::fclose(f);
		printf("Applet::loadTables: loading from tables.yaml\n");
		if (this->loadTablesFromYAML("tables.yaml")) {
			return;
		}
		printf("Applet::loadTables: tables.yaml failed, falling back to tables.bin\n");
	}

	printf("Applet::loadTables: loading from tables.bin (resource pack)\n");

	int count01, count02, count03, count04, count05, count06, count07;
	int count08, count09, count10, count11, count12, count13, count14;

	count01 = this->resource->getNumTableShorts(0);  // count TBL_COMBAT_MONSTERATTACKS
	count02 = this->resource->getNumTableBytes(1);   // count TBL_COMBAT_WEAPONINFO
	count03 = this->resource->getNumTableBytes(2);   // count TBL_COMBAT_WEAPONDATA
	count04 = this->resource->getNumTableBytes(3);   // count TBL_COMBAT_MONSTERSTATS
	count05 = this->resource->getNumTableInts(4);    // count TBL_COMBAT_COMBATMASKS
	count06 = this->resource->getNumTableBytes(5);   // count TBL_CANVAS_KEYSNUMERIC
	count07 = this->resource->getNumTableBytes(6);   // count TBL_ENUMS_OSC_CYCLE
	count08 = this->resource->getNumTableShorts(7);  // count TBL_GAME_LEVELNAMES
	count09 = this->resource->getNumTableBytes(8);   // count TBL_MONSTER_COLORS
	count10 = this->resource->getNumTableInts(9);    // count TBL_RENDER_SINETABLE
	count11 = this->resource->getNumTableShorts(10); // count TBL_ENERGY_DRINK_DATA
	count12 = this->resource->getNumTableBytes(11);  // count TBL_MONSTER_WEAKNESS
	count13 = this->resource->getNumTableInts(12);   // count TBL_UNK
	count14 = this->resource->getNumTableBytes(13);  // count TBL_MONSTER_SOUNDS

	this->combat->monsterAttacks = new short[count01];
	this->combat->wpinfo = new int8_t[count02];
	this->combat->weapons = new int8_t[count03];
	this->combat->monsterStats = new int8_t[count04];
	this->combat->tableCombatMasks = new int32_t[count05];
	this->canvas->keys_numeric = new int8_t[count06];
	this->canvas->OSC_CYCLE = new int8_t[count07];
	this->game->levelNames = new int16_t[count08];
	this->particleSystem->monsterColors = new uint8_t[count09];
	this->render->sinTable = new int32_t[count10];
	this->vendingMachine->energyDrinkData = new int16_t[count11];
	this->combat->monsterWeakness = new int8_t[count12];
	this->canvas->movieEffects = new int32_t[count13];
	this->game->monsterSounds = new uint8_t[count14];

	this->resource->beginTableLoading();
	this->resource->loadShortTable(this->combat->monsterAttacks, 0);
	this->resource->loadByteTable(this->combat->wpinfo, 1);
	this->resource->loadByteTable(this->combat->weapons, 2);
	this->resource->loadByteTable(this->combat->monsterStats, 3);
	this->resource->loadIntTable(this->combat->tableCombatMasks, 4);
	this->resource->loadByteTable(this->canvas->keys_numeric, 5);
	this->resource->loadByteTable(this->canvas->OSC_CYCLE, 6);
	this->game->levelNamesCount = this->resource->loadShortTable(this->game->levelNames, 7);
	this->resource->loadByteTable((int8_t*)this->particleSystem->monsterColors, 8);
	this->resource->loadIntTable(this->render->sinTable, 9);
	this->resource->loadShortTable(this->vendingMachine->energyDrinkData, 10);
	this->resource->loadByteTable(this->combat->monsterWeakness, 11);
	this->resource->loadIntTable(this->canvas->movieEffects, 12);
	this->resource->loadByteTable((int8_t*)this->game->monsterSounds, 13);
	this->resource->finishTableLoading();

	printf("Applet::loadTables: loaded 14 tables from tables.bin\n");
}

// --- Helpers for tables.ini parsing ---

static const char* tblWeaponNames[] = {"assault_rifle",
                                       "chainsaw",
                                       "holy_water_pistol",
                                       "shooting_sentry_bot",
                                       "exploding_sentry_bot",
                                       "red_shooting_sentry_bot",
                                       "red_exploding_sentry_bot",
                                       "super_shotgun",
                                       "chaingun",
                                       "assault_rifle_with_scope",
                                       "plasma_gun",
                                       "rocket_launcher",
                                       "bfg",
                                       "soul_cube",
                                       "item",
                                       "m_bite",
                                       "m_claw",
                                       "m_punch",
                                       "m_charge",
                                       "m_flesh_throw",
                                       "m_fireball",
                                       "m_plasma",
                                       "m_floor_strike",
                                       "m_fire",
                                       "m_machine_gun",
                                       "m_chain_gun",
                                       "m_rockets",
                                       "m_acid_spit",
                                       "m_plasma_gun",
                                       "m_vios_plasma",
                                       "m_vios_lightning",
                                       "m_vios_poison"};
static const int numTblWeaponNames = 32;

static int weaponNameToIndex(const std::string& name) {
	for (int i = 0; i < numTblWeaponNames; i++) {
		if (name == tblWeaponNames[i])
			return i;
	}
	try {
		return std::stoi(name);
	} catch (...) {
		return 0;
	}
}

static int projTypeFromName(const std::string& name) {
	if (name == "none")
		return -1;
	if (name == "bullet")
		return 0;
	if (name == "melee")
		return 1;
	if (name == "water")
		return 2;
	if (name == "plasma")
		return 3;
	if (name == "rocket")
		return 4;
	if (name == "bfg")
		return 5;
	if (name == "flesh")
		return 6;
	if (name == "fire")
		return 7;
	if (name == "caco_plasma")
		return 8;
	if (name == "thorns")
		return 9;
	if (name == "acid")
		return 10;
	if (name == "electric")
		return 11;
	if (name == "soul_cube")
		return 12;
	if (name == "item")
		return 13;
	try {
		return std::stoi(name);
	} catch (...) {
		return -1;
	}
}

static std::vector<int> parseIntList(const std::string& str) {
	std::vector<int> result;
	std::stringstream ss(str);
	std::string item;
	while (std::getline(ss, item, ',')) {
		// trim
		size_t start = item.find_first_not_of(" \t");
		if (start == std::string::npos) {
			result.push_back(0);
			continue;
		}
		item = item.substr(start);
		try {
			result.push_back(std::stoi(item));
		} catch (...) {
			result.push_back(0);
		}
	}
	return result;
}

bool Applet::loadTablesFromYAML(const char* path) {
	YAML::Node config;
	try {
		config = YAML::LoadFile(path);
	} catch (const YAML::Exception&) {
		return false;
	}

	// === Weapons ===
	if (YAML::Node weapons = config["weapons"]) {
		int numWeapons = (int)weapons.size();
		int weaponBytes = numWeapons * 9;
		this->combat->weapons = new int8_t[weaponBytes];
		std::memset(this->combat->weapons, 0, weaponBytes);
		for (int w = 0; w < numWeapons; w++) {
			YAML::Node wp = weapons[w];
			int base = w * 9;
			this->combat->weapons[base + 0] = (int8_t)wp["str_min"].as<int>(0);
			this->combat->weapons[base + 1] = (int8_t)wp["str_max"].as<int>(0);
			this->combat->weapons[base + 2] = (int8_t)wp["range_min"].as<int>(0);
			this->combat->weapons[base + 3] = (int8_t)wp["range_max"].as<int>(0);
			this->combat->weapons[base + 4] = (int8_t)wp["ammo_type"].as<int>(0);
			this->combat->weapons[base + 5] = (int8_t)wp["ammo_usage"].as<int>(0);
			this->combat->weapons[base + 6] = (int8_t)projTypeFromName(wp["proj_type"].as<std::string>("none").c_str());
			this->combat->weapons[base + 7] = (int8_t)wp["num_shots"].as<int>(1);
			this->combat->weapons[base + 8] = (int8_t)wp["shot_hold"].as<int>(0);
		}
	}

	// === WeaponInfo ===
	if (YAML::Node wpinfos = config["weapon_info"]) {
		int numWpInfo = (int)wpinfos.size();
		int wpinfoBytes = numWpInfo * 6;
		this->combat->wpinfo = new int8_t[wpinfoBytes];
		std::memset(this->combat->wpinfo, 0, wpinfoBytes);
		for (int w = 0; w < numWpInfo; w++) {
			YAML::Node wi = wpinfos[w];
			int base = w * 6;
			this->combat->wpinfo[base + 0] = (int8_t)wi["idle_x"].as<int>(0);
			this->combat->wpinfo[base + 1] = (int8_t)wi["idle_y"].as<int>(0);
			this->combat->wpinfo[base + 2] = (int8_t)wi["attack_x"].as<int>(0);
			this->combat->wpinfo[base + 3] = (int8_t)wi["attack_y"].as<int>(0);
			this->combat->wpinfo[base + 4] = (int8_t)wi["flash_x"].as<int>(0);
			this->combat->wpinfo[base + 5] = (int8_t)wi["flash_y"].as<int>(0);
		}
	}

	// === MonsterAttacks ===
	if (YAML::Node attacks = config["monster_attacks"]) {
		int numMonsterAttacks = (int)attacks.size();
		int maShorts = numMonsterAttacks * 9;
		this->combat->monsterAttacks = new short[maShorts];
		std::memset(this->combat->monsterAttacks, 0, maShorts * sizeof(short));
		for (int m = 0; m < numMonsterAttacks; m++) {
			YAML::Node ma = attacks[m];
			int base = m * 9;
			for (int p = 0; p < 3; p++) {
				std::string a1key = "parm" + std::to_string(p) + "_attack1";
				std::string a2key = "parm" + std::to_string(p) + "_attack2";
				std::string chkey = "parm" + std::to_string(p) + "_chance";
				this->combat->monsterAttacks[base + p * 3 + 0] = (short)weaponNameToIndex(ma[a1key].as<std::string>("0").c_str());
				this->combat->monsterAttacks[base + p * 3 + 1] = (short)weaponNameToIndex(ma[a2key].as<std::string>("0").c_str());
				this->combat->monsterAttacks[base + p * 3 + 2] = (short)ma[chkey].as<int>(0);
			}
		}
	}

	// === MonsterStats ===
	if (YAML::Node ms = config["monster_stats"]) {
		YAML::Node data = ms["data"];
		if (data && data.IsSequence()) {
			int count = (int)data.size();
			this->combat->monsterStats = new int8_t[count];
			for (int i = 0; i < count; i++) {
				this->combat->monsterStats[i] = (int8_t)data[i].as<int>(0);
			}
		}
	}

	// === CombatMasks ===
	if (YAML::Node masks = config["combat_masks"]) {
		int count = (int)masks.size();
		this->combat->tableCombatMasks = new int32_t[count];
		for (int i = 0; i < count; i++) {
			std::string val = masks[i].as<std::string>("0");
			if (val.substr(0, 2) == "0x" || val.substr(0, 2) == "0X") {
				this->combat->tableCombatMasks[i] = (int32_t)std::stoul(val, nullptr, 16);
			} else {
				this->combat->tableCombatMasks[i] = (int32_t)std::stoi(val);
			}
		}
	}

	// === KeysNumeric ===
	if (YAML::Node kn = config["keys_numeric"]) {
		YAML::Node data = kn["data"];
		if (data && data.IsSequence()) {
			int count = (int)data.size();
			this->canvas->keys_numeric = new int8_t[count];
			for (int i = 0; i < count; i++) {
				this->canvas->keys_numeric[i] = (int8_t)data[i].as<int>(0);
			}
		}
	}

	// === OSCCycle ===
	if (YAML::Node osc = config["osc_cycle"]) {
		YAML::Node data = osc["data"];
		if (data && data.IsSequence()) {
			int count = (int)data.size();
			this->canvas->OSC_CYCLE = new int8_t[count];
			for (int i = 0; i < count; i++) {
				this->canvas->OSC_CYCLE[i] = (int8_t)data[i].as<int>(0);
			}
		}
	}

	// === LevelNames ===
	if (YAML::Node ln = config["level_names"]) {
		int count = (int)ln.size();
		this->game->levelNames = new int16_t[count];
		for (int i = 0; i < count; i++) {
			this->game->levelNames[i] = (int16_t)ln[i].as<int>(0);
		}
		this->game->levelNamesCount = count;
	}

	// === MonsterColors ===
	if (YAML::Node mc = config["monster_colors"]) {
		static const char* monsterNames[] = {"zombie", "zombie_commando", "lost_soul", "imp", "sawcubus", "pinky"};
		int count = 6 * 3;
		this->particleSystem->monsterColors = new uint8_t[count];
		std::memset(this->particleSystem->monsterColors, 0, count);
		int idx = 0;
		for (int i = 0; i < 6; i++) {
			if (YAML::Node rgb = mc[monsterNames[i]]) {
				if (rgb.IsSequence() && rgb.size() >= 3) {
					this->particleSystem->monsterColors[idx + 0] = (uint8_t)rgb[0].as<int>(0);
					this->particleSystem->monsterColors[idx + 1] = (uint8_t)rgb[1].as<int>(0);
					this->particleSystem->monsterColors[idx + 2] = (uint8_t)rgb[2].as<int>(0);
				}
			}
			idx += 3;
		}
	}

	// === SinTable ===
	if (YAML::Node st = config["sin_table"]) {
		YAML::Node data = st["data"];
		if (data && data.IsSequence()) {
			int count = (int)data.size();
			this->render->sinTable = new int32_t[count];
			for (int i = 0; i < count; i++) {
				this->render->sinTable[i] = (int32_t)data[i].as<int>(0);
			}
		}
	}

	// === EnergyDrinkData ===
	if (YAML::Node ed = config["energy_drink_data"]) {
		YAML::Node data = ed["data"];
		if (data && data.IsSequence()) {
			int count = (int)data.size();
			this->vendingMachine->energyDrinkData = new int16_t[count];
			for (int i = 0; i < count; i++) {
				this->vendingMachine->energyDrinkData[i] = (int16_t)data[i].as<int>(0);
			}
		}
	}

	// === MonsterWeakness ===
	if (YAML::Node mw = config["monster_weakness"]) {
		static const char* monsterNames[] = {
		    "zombie",   "zombie_commando", "lost_soul", "imp",       "sawcubus",   "pinky",      "cacodemon",
		    "sentinel", "mancubus",        "revenant",  "arch_vile", "sentry_bot", "cyberdemon", "mastermind",
		    "phantom",  "archvile_ghost",  "belphegor", "apollyon"};
		int count = 18;
		this->combat->monsterWeakness = new int8_t[count];
		std::memset(this->combat->monsterWeakness, 0, count);
		for (int i = 0; i < count; i++) {
			this->combat->monsterWeakness[i] = (int8_t)mw[monsterNames[i]].as<int>(0);
		}
	}

	// === MovieEffects ===
	if (YAML::Node me = config["movie_effects"]) {
		YAML::Node data = me["data"];
		if (data && data.IsSequence()) {
			int count = (int)data.size();
			this->canvas->movieEffects = new int32_t[count];
			for (int i = 0; i < count; i++) {
				this->canvas->movieEffects[i] = (int32_t)data[i].as<int>(0);
			}
		}
	}

	// === MonsterSounds ===
	if (YAML::Node msounds = config["monster_sounds"]) {
		int numMonsterSounds = (int)msounds.size();
		int msoundBytes = numMonsterSounds * 8;
		this->game->monsterSounds = new uint8_t[msoundBytes];
		std::memset(this->game->monsterSounds, 255, msoundBytes);
		static const char* msoundFields[] = {"alert1", "alert2", "alert3", "attack1",
		                                     "attack2", "idle", "pain", "death"};
		for (int m = 0; m < numMonsterSounds; m++) {
			YAML::Node ms = msounds[m];
			int base = m * 8;
			for (int f = 0; f < 8; f++) {
				std::string val = ms[msoundFields[f]].as<std::string>("none");
				if (val == "none") {
					this->game->monsterSounds[base + f] = 255;
				} else {
					this->game->monsterSounds[base + f] = (uint8_t)std::atoi(val.c_str());
				}
			}
		}
	}

	printf("Applet::loadTables: loaded 14 tables from %s\n", path);
	return true;
}

void Applet::loadRuntimeImages() {
	// printf("Applet::loadRuntimeImages\n");
	// printf("this->initLoadImages %d\n", this->initLoadImages);
	if (this->initLoadImages == false) {
		this->imageMemory = 1000000000;

		this->canvas->imgMapCursor = this->loadImage("Automap_Cursor.bmp", true);
		this->canvas->imgUIImages = this->loadImage("ui_images.bmp", true);
		this->hud->imgCockpitOverlay = this->loadImage("cockpit.bmp", true);
		this->hud->imgDamageVignette = this->loadImage("damage.bmp", true);
		this->hud->imgDamageVignetteBot = this->loadImage("damage_bot.bmp", true);
		this->canvas->imgDialogScroll = this->loadImage("DialogScroll.bmp", true);
		this->hud->imgActions = this->loadImage("Hud_Actions.bmp", true);
		this->hud->imgBottomBarIcons = this->loadImage("Hud_Fill.bmp", true);
		this->hud->imgHudFill = this->loadImage("Hud_Actions.bmp", true);

		this->hud->imgPlayerFrameNormal->~Image();
		this->hud->imgPlayerFrameNormal = nullptr;
		this->hud->imgPlayerFrameActive->~Image();
		this->hud->imgPlayerFrameActive = nullptr;

		this->hud->imgPlayerFrameNormal = this->loadImage("HUD_Player_frame_Normal.bmp", true);
		this->hud->imgPlayerFrameActive = this->loadImage("HUD_Player_frame_Active.bmp", true);

		this->hud->imgPlayerFaces->~Image();
		this->hud->imgPlayerFaces = nullptr;
		this->hud->imgPlayerActive->~Image();
		this->hud->imgPlayerActive = nullptr;

		if (this->player->characterChoice == 1) {
			this->hud->imgPlayerFaces = this->loadImage("Hud_Player.bmp", true);
			this->hud->imgPlayerActive = this->loadImage("HUD_Player_Active.bmp", true);
		} else if (this->player->characterChoice == 2) {
			this->hud->imgPlayerFaces = this->loadImage("Hud_PlayerDoom.bmp", true);
			this->hud->imgPlayerActive = this->loadImage("HUD_PlayerDoom_Active.bmp", true);
		} else if (this->player->characterChoice == 3) {
			this->hud->imgPlayerFaces = this->loadImage("Hud_PlayerScientist.bmp", true);
			this->hud->imgPlayerActive = this->loadImage("HUD_PlayerScientist_Active.bmp", true);
		}

		this->hud->imgSentryBotFace = this->loadImage("Hud_Sentry.bmp", true);
		this->hud->imgSentryBotActive = this->loadImage("HUD_sentry_active.bmp", true);

		this->hud->imgHudTest = this->loadImage("Hud_Test.bmp", true);
		this->hud->imgScope = this->loadImage("scope.bmp", true);

		this->canvas->updateLoadingBar(false);
		this->imageMemory = this->imageMemory + -1000000000;
	}
}

void Applet::freeRuntimeImages() {
	this->canvas->imgMapCursor->~Image();
	this->canvas->imgMapCursor = nullptr;
	this->canvas->imgUIImages->~Image();
	this->canvas->imgUIImages = nullptr;
	this->canvas->imgDialogScroll->~Image();
	this->canvas->imgDialogScroll = nullptr;
	this->hud->imgScope->~Image();
	this->hud->imgScope = nullptr;
	this->hud->imgDamageVignette->~Image();
	this->hud->imgDamageVignette = nullptr;
	this->hud->imgActions->~Image();
	this->hud->imgActions = nullptr;
	this->hud->imgBottomBarIcons->~Image();
	this->hud->imgBottomBarIcons = nullptr;
	this->hud->imgHudFill->~Image();
	this->hud->imgHudFill = nullptr;
	this->hud->imgPlayerFaces->~Image();
	this->hud->imgPlayerFaces = nullptr;
	this->hud->imgPlayerActive->~Image();
	this->hud->imgPlayerActive = nullptr;
	this->hud->imgDamageVignetteBot->~Image();
	this->hud->imgDamageVignetteBot = nullptr;
	this->hud->imgHudTest->~Image();
	this->hud->imgHudTest = nullptr;
	this->hud->imgSentryBotFace->~Image();
	this->hud->imgSentryBotFace = nullptr;
	this->hud->imgSentryBotActive->~Image();
	this->hud->imgSentryBotActive = nullptr;
	this->hud->imgCockpitOverlay->~Image();
	this->hud->imgCockpitOverlay = nullptr;
}

void Applet::setFont(int fontType) {
	if (fontType <= 3) {
		this->fontType = fontType;
	}
}

void Applet::shutdown() {
	this->closeApplet = true;
}

uint32_t Applet::nextInt() {
	return std::rand() & INT32_MAX;
}

uint32_t Applet::nextByte() {
	return this->nextInt() & UINT8_MAX;
}

void Applet::setFontRenderMode(int fontRenderMode) {
	this->canvas->fontRenderMode = fontRenderMode;
}

void Applet::AccelerometerUpdated(float x, float y, float z) {

	this->accelerationX[this->accelerationIndex] = x;
	this->accelerationY[this->accelerationIndex] = y;
	this->accelerationZ[this->accelerationIndex] = z;
	this->accelerationIndex = (this->accelerationIndex + 1) % 32;

	int v7 = (uint8_t)this->field_0x291;
	int v8 = v7 == 0;
	if (!v7) {
		v8 = this->accelerationIndex == 0;
	}
	if (v8) {
		this->field_0x291 = v7 + 1;
	}
	// this->comicBook->UpdateAccelerometer(x, y, z);
}

void Applet::StartAccelerometer() {
	this->accelerationIndex = 0;
	this->field_0x290 = false;
	this->field_0x291 = false;
}

void Applet::StopAccelerometer() {
	this->accelerationIndex = 0;
	this->field_0x290 = false;
	this->field_0x291 = false;
}

void Applet::CalcAccelerometerAngles() {
	bool v2;          // zf
	float y;          // s13
	int v5;           // r2
	float x;          // s14
	float z;          // s12
	float v9;         // s11
	float v10;        // s14
	int zoomAngle;    // r3
	int v14;          // r3
	int zoomMaxAngle; // r3
	int zoomPitch;    // r1

	v2 = this->field_0x291 == false;
	if (this->field_0x291) {
		v2 = !this->canvas->isZoomedIn;
	}
	if (!v2) {
		v5 = 0;
		x = 0.0;
		y = 0.0;
		z = 0.0;
		do {
			x += this->accelerationX[v5];
			y += this->accelerationY[v5];
			z += this->accelerationZ[v5];
		} while (++v5 < 32);

		this->field_0x414 = x * 0.03125;
		this->field_0x418 = y * 0.03125;
		this->field_0x41c = z * 0.03125;

		if (!this->field_0x290) {
			this->field_0x290 = true;
			this->field_0x420 = x * 0.03125;
			this->field_0x424 = y * 0.03125;
			this->field_0x428 = z * 0.03125;
			return;
		}
		this->canvas->zoomAngle = (int)(float)((float)(this->field_0x414 - this->field_0x420) * 420.0);

		zoomAngle = this->canvas->zoomAngle;
		if (zoomAngle >= -200) {
			if (zoomAngle <= 200)
				goto LABEL_13;
			v14 = 200;
		} else {
			v14 = -200;
		}
		this->canvas->zoomAngle = v14;
		this->canvas = this->canvas;
	LABEL_13:
		this->canvas->zoomPitch = (int)(float)((float)(this->field_0x418 - this->field_0x424) * 420.0);
		zoomMaxAngle = this->canvas->zoomMaxAngle;
		zoomPitch = this->canvas->zoomPitch;
		if (zoomPitch >= -zoomMaxAngle) {
			if (zoomPitch > zoomMaxAngle)
				this->canvas->zoomPitch = zoomMaxAngle;
		} else {
			this->canvas->zoomPitch = -zoomMaxAngle;
		}
	}
}
