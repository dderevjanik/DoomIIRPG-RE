#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

#include <yaml-cpp/yaml.h>

#include "SpriteLoader.h"

// Asset category tabs
enum class AssetCategory {
	Entities,
	Monsters,
	Weapons,
	Projectiles,
	Characters,
	Effects,
	Animations,
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

	// Panel drawing
	void drawCategoryTabs();
	void drawEntitiesPanel();
	void drawMonstersPanel();
	void drawWeaponsPanel();
	void drawProjectilesPanel();
	void drawCharactersPanel();
	void drawEffectsPanel();
	void drawAnimationsPanel();
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

	// Data
	std::vector<EntityEntry> entities_;
	std::vector<MonsterEntry> monsters_;
	std::vector<WeaponEntry> weapons_;
	std::vector<ProjectileEntry> projectiles_;
	std::vector<CharacterEntry> characters_;
	std::vector<BuffEntry> buffs_;
	std::vector<AnimationEntry> animations_;
};
