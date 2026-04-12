#pragma once
#include <cstdint>

class Entity;

// --- Combat events ---

struct MonsterDeathEvent {
	Entity* monster;
	Entity* killer;
	int xpAwarded;
	bool byExplosion;
	int mapX;
	int mapY;
};

struct MonsterPainEvent {
	Entity* monster;
	Entity* attacker;
	int damage;
	int remainingHealth;
};

struct PlayerDamageEvent {
	int damage;
	Entity* source;
	int healthBefore;
	int healthAfter;
};

struct PlayerHealEvent {
	int amount;
	int healthBefore;
	int healthAfter;
};

// --- Item / inventory events ---

struct ItemPickupEvent {
	Entity* item;
	int itemType;
	int itemParam;
	int quantity;
	bool wasDropped;
};

struct WeaponSwitchEvent {
	int previousWeapon;
	int newWeapon;
};

// --- World events ---

struct LevelLoadEvent {
	int mapID;
	bool isNewGame;
};

struct LevelLoadCompleteEvent {
	int mapID;
	int entityCount;
};

struct DoorEvent {
	Entity* door;
	bool opening;
	int mapX;
	int mapY;
};

// --- HUD events ---

struct HudRepaintEvent {
	int flags;
};

// --- Player progression ---

struct XPGainEvent {
	int amount;
	int totalXP;
};
