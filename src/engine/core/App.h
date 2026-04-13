#pragma once
#include <cstring>
#include <flat_map>
#include <memory>
#include <stdint.h>

#include "IDIB.h"

struct SpriteAnimDef {
	int renderMode = -1;  // -1 = don't override
	int scale = -1;       // -1 = don't override
	int numFrames = -1;   // -1 = don't override
	int duration = -1;    // -1 = don't override
	bool zAtGround = false;
	int zOffset = 0;
	bool randomFlip = false;
	bool facePlayer = false;
	int posOffset = 0;
};

extern std::flat_map<int, SpriteAnimDef> gSpriteAnimDefs;

class SDLGL;
class IDIB;
class Image;
class IGameModule;
class EventBus;

class Localization;
class Resource;
class Render;
class TinyGL;
class Canvas;
class Game;
class MenuSystem;
class Player;
class Sound;
class Combat;
class Hud;
class EntityDefManager;
class ParticleSystem;
class HackingGame;
class SentryBotGame;
class VendingMachine;
class ComicBook;
class InputStream;

class Applet
{
private:

public:
	// Defines
	static constexpr int MAXMEMORY = 1000000000;
	static constexpr int IOS_WIDTH = 480;
	static constexpr int IOS_HEIGHT = 320;

	static constexpr int FONT_HEIGHT[4] = { 16, 16, 18, 25 };
	static constexpr int FONT_WIDTH[4] = { 12, 12, 13, 22 };
	static constexpr int CHAR_SPACING[4] = { 11, 11, 12, 22 };

	//------------------
	SDLGL* sdlGL = nullptr;
	IDIB* backBuffer = nullptr;
	int upTimeMs = 0;
	int lastTime = 0;
	int time = 0;
	int gameTime = 0;
	int startupMemory = 0;
	int imageMemory = 0;
	char* peakMemoryDesc = nullptr;
	int peakMemoryUsage = 0;
	int idError = 0;
	bool initLoadImages = false;

	int sysAdvTime = 0;
	int osTime[8] = {};
	int codeTime[8] = {};
	int field_0x26c = 0;
	int field_0x270 = 0;
	int field_0x278 = 0;
	int field_0x27c = 0;
	int fontType = 0;
	int accelerationIndex = 0;
	bool field_0x290 = false;
	bool field_0x291 = false;
	int field_0x7c = 0;
	int field_0x80 = 0;

	// Iphone Only
	float accelerationX[32] = {};
	float accelerationY[32] = {};
	float accelerationZ[32] = {};

	float field_0x414 = 0.0f;
	float field_0x418 = 0.0f;
	float field_0x41c = 0.0f;
	float field_0x420 = 0.0f;
	float field_0x424 = 0.0f;
	float field_0x428 = 0.0f;
	bool closeApplet = false;

	//--- Owned subsystems (engine) ---
	std::unique_ptr<EventBus> eventBus;
	std::unique_ptr<Localization> localization;
	std::unique_ptr<Resource> resource;
	std::unique_ptr<Render> render;
	std::unique_ptr<TinyGL> tinyGL;
	std::unique_ptr<Canvas> canvas;
	std::unique_ptr<MenuSystem> menuSystem;
	std::unique_ptr<Sound> sound;
	std::unique_ptr<Hud> hud;
	std::unique_ptr<ParticleSystem> particleSystem;
	//--- Owned subsystems (game module) ---
	std::unique_ptr<Game> game;
	std::unique_ptr<Player> player;
	std::unique_ptr<Combat> combat;
	std::unique_ptr<EntityDefManager> entityDefManager;
	std::unique_ptr<HackingGame> hackingGame;
	std::unique_ptr<SentryBotGame> sentryBotGame;
	std::unique_ptr<VendingMachine> vendingMachine;
	std::unique_ptr<ComicBook> comicBook;
	//--- Non-owning pointers ---
	IGameModule* gameModule = nullptr;
	Image* testImg = nullptr;
	int seed = 0;

	// Constructor
	Applet();
	// Destructor
	~Applet();

	void setGameModule(IGameModule* module);
	[[nodiscard]] bool startup();
	void loadConfig();

	[[nodiscard]] Image* createImage(InputStream* inputStream, bool isTransparentMask);
	[[nodiscard]] Image* loadImage(char* fileName, bool isTransparentMask);

	void beginImageLoading();
	void endImageLoading();

	void Error(const char* fmt, ...);
	void Error(int id);
	void loadTables();

	void loadRuntimeImages();
	void freeRuntimeImages();
	void setFont(int fontType);
	void shutdown();
	uint32_t nextInt();
	uint32_t nextByte();
	void setFontRenderMode(int fontRenderMode);

	void AccelerometerUpdated(float x, float y, float z);
	void StartAccelerometer();
	void StopAccelerometer();
	void CalcAccelerometerAngles();
};
