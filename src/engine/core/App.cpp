#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <map>

#include "SDLGL.h"

#include "CAppContainer.h"
#include "App.h"
#include "EventBus.h"
#include "Log.h"
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
#include "LootDistributor.h"
#include "MinigameUI.h"
#include "StoryRenderer.h"
#include "DialogManager.h"
#include "JavaStream.h"
#include "Image.h"
#include "Graphics.h"
#include "Input.h"
#include "ResourceManager.h"


Applet::Applet() = default;

Applet::~Applet() = default;

void Applet::setGameModule(IGameModule* module) {
	this->gameModule = module;
}

bool Applet::startup() {
	LOG_INFO("[app] Applet::startup\n");

	this->sdlGL = CAppContainer::getInstance()->sdlGL;
	this->closeApplet = false;
	this->fontType = 0;
	this->accelerationIndex = 0;
	this->accelCalibrated = '\0';
	this->accelHasSamples = '\0';

	// Iphone Only
	{
		for (int i = 0; i < 32; i++) {
			accelerationX[i] = 0.0f;
			accelerationY[i] = 0.0f;
			accelerationZ[i] = 0.0f;
		}
	}

	this->accelAvgX = 0;
	this->accelAvgY = 0;
	this->accelAvgZ = 0;
	this->accelBaseX = 0;
	this->accelBaseY = 0;
	this->accelBaseZ = 0;

	this->backBuffer = new IDIB;
	this->backBuffer->pBmp = new uint8_t[Applet::IOS_WIDTH * Applet::IOS_HEIGHT * 2];
	std::memset(this->backBuffer->pBmp, 0, Applet::IOS_WIDTH * Applet::IOS_HEIGHT * 2);
	this->backBuffer->pRGB888 = nullptr;
	this->backBuffer->pRGB565 = nullptr;
	this->backBuffer->width = this->sdlGL->vidWidth;
	this->backBuffer->height = this->sdlGL->vidHeight;
	this->backBuffer->pitch = this->sdlGL->vidWidth;

	LOG_INFO("[app] w: {} || h: {}\n", backBuffer->width, backBuffer->height);

	this->initLoadImages = false;
	this->time = 0;
	this->upTimeMs = 0;
	this->unused_0x7c = 0;
	this->unused_0x80 = 0;
	// Engine subsystems
	this->eventBus = std::make_unique<EventBus>();
	this->canvas = std::make_unique<Canvas>();
	this->resource = std::make_unique<Resource>();
	this->localization = std::make_unique<Localization>();
	this->render = std::make_unique<Render>();
	this->tinyGL = std::make_unique<TinyGL>();
	this->menuSystem = std::make_unique<MenuSystem>();
	this->sound = std::make_unique<Sound>();
	this->hud = std::make_unique<Hud>();
	this->particleSystem = std::make_unique<ParticleSystem>();

	// Game objects — created by the active game module
	if (this->gameModule) {
		LOG_INFO("[app] creating game objects via {} module\n", this->gameModule->getName());
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
	this->resource->startup();
	if (!this->canvas->startup()) { LOG_ERROR("[app] Canvas::startup() failed — check above for font/image loading errors\n"); return false; }
	this->testImg = Applet::loadImage("cockpit.bmp", true);
	this->minigameUI->loadImages();
	if (!this->localization->startup()) { LOG_ERROR("[app] Localization::startup() failed — check string files in game.yaml\n"); return false; }
	if (!this->render->startup()) { LOG_ERROR("[app] Render::startup() failed — check texture/level assets\n"); return false; }
	this->loadTables();
	if (!this->tinyGL->startup(this->render->screenWidth, this->render->screenHeight)) {
		LOG_ERROR("[app] TinyGL::startup() failed ({}x{}) — software renderer init error\n", this->render->screenWidth, this->render->screenHeight); return false;
	}

	// Game module startup (EntityDefManager -> Player -> Game -> Combat)
	if (!this->gameModule->startup(this)) { LOG_ERROR("[app] GameModule::startup() failed — check entity definitions and game data\n"); return false; }

	// Remaining engine subsystems that depend on game objects
	if (!this->menuSystem->startup()) { LOG_ERROR("[app] MenuSystem::startup() failed — check menus.yaml\n"); return false; }
	if (!this->sound->startup()) { LOG_ERROR("[app] Sound::startup() failed — check OpenAL and audio config\n"); return false; }
	if (!this->particleSystem->startup()) { LOG_ERROR("[app] ParticleSystem::startup() failed\n"); return false; }

	// Game module post-startup
	this->gameModule->loadConfig(this);
	this->gameModule->registerOpcodes(this);
	this->gameModule->registerEventListeners(this);

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
	this->accelCalibrated = false;
	this->accelHasSamples = '\0';
	LOG_INFO("[app] Startup took {} ms\n", this->upTimeMs - time);
	LOG_INFO("[app] Fragment size {} ms\n", 0);

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
		int pitchBytes = (int)ColorsUsed >> 3;
		ColorsUsed = pitch + 7;
		if ((int)_pitch < 0) {
			ColorsUsed = _pitch + 7;
		}
		int pixelCount = Height * Width;
		if ((int)_pitch >= 0) {
			ColorsUsed = _pitch;
		}

		// printf("pixelCount %d\n", pixelCount);
		img->piDIB->pBmp = (uint8_t*)std::malloc(pixelCount * sizeof(int16_t));
		img->piDIB->width = Width;
		img->piDIB->pitch = Width;
		img->piDIB->depth = BitsPerPixel;
		img->piDIB->height = Height;

		bool isNot4Bit = BitsPerPixel != 4;

		if (isNot4Bit) {
			Width = 0;
		}
		img->height = img->piDIB->height;
		if (!isNot4Bit) {
			Width = Height - 1;
		}
		img->depth = img->piDIB->depth;

		uint8_t* data = inputStream->getTop();
		if (isNot4Bit) {
			for (; Width < Height; Width = Width + 1) {
				std::memcpy(img->piDIB->pBmp + img->piDIB->pitch * Width, data + pitchBytes * (Height + (-1 - Width)),
				            (int)ColorsUsed >> 3);
			}
		} else {
			for (int i = 0; i < Height; i++) {
				std::memcpy(img->piDIB->pBmp + (i * img->piDIB->pitch >> 1), data + pitchBytes * Width,
				            (int)ColorsUsed >> 3);
				Width = Width - 1;
			}
		}
		inputStream->offsetCursor(pitchBytes * Height);

		if ((short)BitsPerPixel == 4) {
			uint8_t* pbVar4 = (uint8_t*)std::malloc(pixelCount);
			uint8_t* _data = pbVar4;
			for (ColorsUsed = 0; ColorsUsed < pixelCount >> 1; ColorsUsed = ColorsUsed + 1) {
				uint8_t bVar1 = img->piDIB->pBmp[ColorsUsed];
				_data[0] = bVar1 >> 4;
				_data[1] = bVar1 & 0xf;
				_data += 2;
			}
			for (ColorsUsed = 0; ColorsUsed < pixelCount; ColorsUsed = ColorsUsed + 1) {
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

	SDL_SetWindowFullscreen(this->sdlGL->window, 0);

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
	    nullptr,                                                               /* .window */
	    (CAppContainer::getInstance()->gameConfig.name + " Error").c_str(), /* .title */
	    errMsg,                                                             /* .message */
	    SDL_arraysize(buttons),                                             /* .numbuttons */
	    buttons,                                                            /* .buttons */
	    &colorScheme                                                        /* .colorScheme */
	};

	SDL_ShowMessageBox(&messageboxdata, nullptr);

	CAppContainer::getInstance()->~CAppContainer();
	exit(0);
}

void Applet::Error(int id) {
	LOG_ERROR("[app] Error id: {}\n", id);
	this->Error("App Error");
	this->idError = id;
}

void Applet::beginImageLoading() {}

void Applet::endImageLoading() {}

void Applet::loadTables() {
	ResourceManager* rm = CAppContainer::getInstance()->resourceManager;
	if (rm && this->gameModule) {
		this->gameModule->registerLoaders(this, rm);
		if (auto result = rm->loadAllDefinitions(); !result) {
			this->Error("ResourceManager: failed to load definitions: %s", result.error().c_str());
		}
	}
}

// Global sprite animation overrides, populated from sprites.yaml sprite_anims section
std::flat_map<int, SpriteAnimDef> gSpriteAnimDefs;


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

		if (this->player->characterChoice == 2) {
			this->hud->imgPlayerFaces = this->loadImage("Hud_PlayerDoom.bmp", true);
			this->hud->imgPlayerActive = this->loadImage("HUD_PlayerDoom_Active.bmp", true);
		} else if (this->player->characterChoice == 3) {
			this->hud->imgPlayerFaces = this->loadImage("Hud_PlayerScientist.bmp", true);
			this->hud->imgPlayerActive = this->loadImage("HUD_PlayerScientist_Active.bmp", true);
		} else {
			// Default (characterChoice 0 or 1): standard marine face
			this->hud->imgPlayerFaces = this->loadImage("Hud_Player.bmp", true);
			this->hud->imgPlayerActive = this->loadImage("HUD_Player_Active.bmp", true);
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

	int v7 = (uint8_t)this->accelHasSamples;
	int v8 = v7 == 0;
	if (!v7) {
		v8 = this->accelerationIndex == 0;
	}
	if (v8) {
		this->accelHasSamples = v7 + 1;
	}
	// this->comicBook->UpdateAccelerometer(x, y, z);
}

void Applet::StartAccelerometer() {
	this->accelerationIndex = 0;
	this->accelCalibrated = false;
	this->accelHasSamples = false;
}

void Applet::StopAccelerometer() {
	this->accelerationIndex = 0;
	this->accelCalibrated = false;
	this->accelHasSamples = false;
}

void Applet::CalcAccelerometerAngles() {
	bool v2;
	float y;
	int v5;
	float x;
	float z;
	float v9;
	float v10;
	int zoomAngle;
	int v14;
	int zoomMaxAngle;
	int zoomPitch;

	v2 = this->accelHasSamples == false;
	if (this->accelHasSamples) {
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

		this->accelAvgX = x * 0.03125;
		this->accelAvgY = y * 0.03125;
		this->accelAvgZ = z * 0.03125;

		if (!this->accelCalibrated) {
			this->accelCalibrated = true;
			this->accelBaseX = x * 0.03125;
			this->accelBaseY = y * 0.03125;
			this->accelBaseZ = z * 0.03125;
			return;
		}
		this->canvas->zoomAngle = (int)(float)((float)(this->accelAvgX - this->accelBaseX) * 420.0);

		zoomAngle = this->canvas->zoomAngle;
		if (zoomAngle >= -200) {
			if (zoomAngle <= 200)
				goto calc_zoom_pitch;
			v14 = 200;
		} else {
			v14 = -200;
		}
		this->canvas->zoomAngle = v14;
	calc_zoom_pitch:
		this->canvas->zoomPitch = (int)(float)((float)(this->accelAvgY - this->accelBaseY) * 420.0);
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
