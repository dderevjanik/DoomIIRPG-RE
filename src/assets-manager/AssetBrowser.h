#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

#include <yaml-cpp/yaml.h>

#include "SpriteLoader.h"
#include "ImageLoader.h"
#include "SpriteDefs.h"

// Asset category tabs
enum class AssetCategory {
	Entities,
	Monsters,
	Weapons,
	Projectiles,
	Characters,
	Effects,
	Animations,
	Menus,
	UI,
	Sounds,
	Game,
	Levels,
	COUNT
};

// Lightweight structs for display (parsed from YAML)

struct EntityEntry {
	std::string name;
	int tileIndex;
	std::string type;
	std::string subtype;
	int parm;
	int nameStr;
	int longNameStr;
	int descStr;
	// Body part offsets (from entities.yaml rendering section)
	SpriteLoader::BodyPartOffsets bodyParts;
	bool isFloater = false;
	bool isSpecialBoss = false;
	bool noHeadOnAttack = false;
};

struct MonsterTier {
	int health, armor, defense, strength, accuracy, agility;
	std::string attack1, attack2;
	int chance;
	std::vector<std::pair<std::string, float>> weaknesses;
};

struct MonsterEntry {
	std::string name;
	int index;
	std::string bloodColor;
	std::vector<MonsterTier> tiers;
	// Sounds
	std::string alertSnd1, alertSnd2, alertSnd3;
	std::string attackSnd1, attackSnd2;
	std::string idleSnd, painSnd, deathSnd;
};

struct WeaponEntry {
	std::string name;
	int index;
	std::string attackSound;
	int damageMin, damageMax;
	int rangeMin, rangeMax;
	std::string ammoType;
	int ammoUsage;
	std::string projectile;
	int shots;
	int shotHold;
};

struct ProjectileEntry {
	std::string name;
	// Launch
	std::string launchRenderMode;
	std::string launchAnim;
	int launchSpeed;
	// Impact
	std::string impactAnim;
	std::string impactRenderMode;
	std::string impactSound;
};

struct CharacterEntry {
	std::string name;
	int defense, strength, accuracy, agility, iq, credits;
};

struct BuffEntry {
	std::string name;
	int maxStacks;
	bool hasAmount;
	bool drawAmount;
};

struct AnimationEntry {
	std::string name;
	int tileIndex;
	SpriteSourceType sourceType = SpriteSourceType::Bin;
	std::string pngFile; // only set for PNG type
};

// Menu item (from menus.yaml)
struct MenuItemEntry {
	int stringId = 0;
	std::string flags;
	std::string action;
	std::string gotoMenu;
	int param = 0;
	int helpString = 0;
	std::string text;
	std::string text2;
	std::string widget;
};

struct MenuEntry {
	std::string name;
	int menuId = 0;
	std::string type;
	std::string theme;
	int maxItems = 0;
	// Presentation properties
	std::string background;
	bool drawLogo = false;
	int helpResource = -1;
	int selectedIndex = -1;
	bool showInfoButtons = false;
	std::string layout;
	std::string drawLayout;
	std::vector<int> visibleButtons;
	std::vector<MenuItemEntry> items;
};

// UI button (from ui.yaml)
struct UIButtonEntry {
	int id = -1;
	int idRangeStart = -1;
	int idRangeEnd = -1;
	std::string name;
	std::string screen;
	std::string component;
	int x = 0, y = 0, width = 0, height = 0;
	std::string sound;
	std::string image;
	std::string imageHighlight;
	int renderMode = 0;
	int highlightRenderMode = 0;
	bool visible = true;
	bool sizeFromImage = false;
};

struct UIWidgetMapping {
	std::string widget;
	std::string flags;
};

// Sound entry (from sounds.yaml)
struct SoundEntry {
	std::string name;
	std::string file;
	int index = 0; // position in array
};

// Game config (from game.yaml)
struct GameConfig {
	std::string name;
	std::string description;
	std::string version;
	std::string windowTitle;
	std::string saveDir;
	std::string entryMap;
	int startingMaxHealth = 100;
	struct { int health, defense, strength, accuracy, agility; } levelUp = {};
	struct { int credits, inventory, ammo, botFuel; } caps = {};
};

// Level entry (from levels.yaml)
struct LevelEntry {
	int mapNumber = 0;
	std::vector<std::string> weapons;
	int armor = 0;
	int xp = 0;
	struct { int defense, strength, accuracy, iq; } stats = {};
};

class AssetBrowser {
public:
	void init(const std::string& gameDir);
	void draw();

private:
	// Data loading
	void loadEntities();
	void loadMonsters();
	void loadWeapons();
	void loadProjectiles();
	void loadCharacters();
	void loadEffects();
	void loadAnimations();
	void loadMenus();
	void loadUI();
	void loadSounds();
	void loadGame();
	void loadLevels();

	// Panel drawing
	void drawCategoryTabs();
	void drawEntitiesPanel();
	void drawMonstersPanel();
	void drawWeaponsPanel();
	void drawProjectilesPanel();
	void drawCharactersPanel();
	void drawEffectsPanel();
	void drawAnimationsPanel();
	void drawMenusPanel();
	void drawUIPanel();
	void drawSoundsPanel();
	void drawGamePanel();
	void drawLevelsPanel();
	void drawEntityDetail(const EntityEntry& e);
	void drawMonsterDetail(const MonsterEntry& m);
	void drawWeaponDetail(const WeaponEntry& w);

	// Sprite preview helpers
	void drawSpritePreview(int tileIndex, float scale = 4.0f);
	void drawAnimatedSprite(int tileIndex, float scale = 4.0f);
	void drawCompositeMonster(int tileIndex, const SpriteLoader::BodyPartOffsets& offsets,
	                          bool isFloater, bool isSpecialBoss, float scale = 4.0f);

	// State
	std::string gameDir_;
	AssetCategory currentCategory_ = AssetCategory::Entities;
	bool loaded_ = false;

	// Sprite loader
	SpriteLoader sprites_;

	// Image loader for PNG sprites
	ImageLoader imageLoader_;

	// Animation playback
	float animTime_ = 0.0f;
	float animSpeed_ = 8.0f; // frames per second
	bool animPlaying_ = true;
	int selectedPose_ = 0; // 0=idle, 1=walk, 2=attack1, 3=attack2, 4=idle_back, 5=walk_back

	// Tile name -> tile index mapping (from sprites.yaml)
	std::unordered_map<std::string, int> tileNameToIndex_;

	// Filter
	char filterText_[128] = {};

	// Selection indices
	int selectedEntity_ = -1;
	int selectedMonster_ = -1;
	int selectedWeapon_ = -1;
	int selectedProjectile_ = -1;
	int selectedCharacter_ = -1;
	int selectedBuff_ = -1;
	int selectedAnimation_ = -1;
	int selectedMenu_ = -1;
	int selectedButton_ = -1;
	int selectedSound_ = -1;
	int selectedLevel_ = -1;

	// Data
	std::vector<EntityEntry> entities_;
	std::vector<MonsterEntry> monsters_;
	std::vector<WeaponEntry> weapons_;
	std::vector<ProjectileEntry> projectiles_;
	std::vector<CharacterEntry> characters_;
	std::vector<BuffEntry> buffs_;
	std::vector<AnimationEntry> animations_;
	std::vector<MenuEntry> menus_;
	std::vector<UIButtonEntry> uiButtons_;
	std::vector<UIWidgetMapping> uiWidgetMappings_;
	std::vector<SoundEntry> sounds_;
	GameConfig gameConfig_;
	std::vector<LevelEntry> levels_;
};
