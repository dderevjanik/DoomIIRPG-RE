#include <algorithm>
#include <cstring>
#include <expected>
#include <map>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "Log.h"

#include "GameDataParsers.h"
#include "App.h"
#include "DataNode.h"
#include "CAppContainer.h"
#include "Combat.h"
#include "Player.h"
#include "Render.h"
#include "SpriteDefs.h"
#include "ItemDefs.h"
#include "Sounds.h"
#include "EntityNames.h"
#include "Canvas.h"
#include "Resource.h"
#include "Game.h"
#include "Enums.h"
#include "ParticleSystem.h"
#include "VendingMachine.h"
#include "EntityDef.h"
#include "Input.h"
#include <cstring>

// --- Helpers for data parsing (file-static) ---

// Parse a single drop entry. `entry` keeps item-name semantics compatible with the
// legacy scalar `drop:` schema. In the new `drops:` list form, each element is a
// map with `item`, optional `quantity`, optional `chance`, and optional `trinket`.
// In the legacy form, `entry` is the top-level `loot:` node and holds `drop`,
// `quantity`, `trinket` keys directly.
static bool parseLootDropEntry(const DataNode& entry, MonsterDef::LootDropEntry& out) {
	DataNode itemNode = entry["item"];
	if (!itemNode) itemNode = entry["drop"]; // legacy alias
	if (!itemNode) return false;

	std::string dropName = itemNode.asString("");
	if (dropName == "trinket") {
		out.base = 0;
		out.modulus = 0;
		out.offset = 0;
		out.trinketText = entry["trinket"].asString("");
	} else {
		auto [category, parm] = EntityNames::resolveDropName(dropName);
		out.base = (int16_t)((category << 12) | (parm << 6));
		std::string qtyStr = entry["quantity"].asString("1");
		int qmin = 0, qmax = 0;
		if (auto dash = qtyStr.find('-'); dash != std::string::npos) {
			qmin = std::stoi(qtyStr.substr(0, dash));
			qmax = std::stoi(qtyStr.substr(dash + 1));
		} else {
			qmin = qmax = std::stoi(qtyStr);
		}
		out.offset = (int8_t)qmin;
		out.modulus = (int8_t)(qmax - qmin + 1);
	}

	int chance = entry["chance"].asInt(100);
	if (chance < 0) chance = 0;
	if (chance > 100) chance = 100;
	out.chancePct = (uint8_t)chance;
	return true;
}

static void parseLootConfig(const DataNode& loot, MonsterDef::LootConfig& lc) {
	lc.drops.clear();

	if (loot.isSequence()) {
		// Shorthand: `loots:` is a sequence of drop entries directly.
		int n = loot.size();
		lc.drops.reserve(n);
		for (int i = 0; i < n; ++i) {
			MonsterDef::LootDropEntry e;
			if (parseLootDropEntry(loot[i], e)) {
				lc.drops.push_back(std::move(e));
			}
		}
		return;
	}

	if (DataNode dropsNode = loot["drops"]; dropsNode && dropsNode.isSequence()) {
		int n = dropsNode.size();
		lc.drops.reserve(n);
		for (int i = 0; i < n; ++i) {
			MonsterDef::LootDropEntry e;
			if (parseLootDropEntry(dropsNode[i], e)) {
				lc.drops.push_back(std::move(e));
			}
		}
	} else if (loot["drop"]) {
		// Legacy single-drop shorthand: `drop: <item>` + optional `quantity:` / `trinket:`
		MonsterDef::LootDropEntry e;
		if (parseLootDropEntry(loot, e)) {
			lc.drops.push_back(std::move(e));
		}
	} else if (loot["base"]) {
		// Low-level fallback (raw packed fields)
		MonsterDef::LootDropEntry e;
		e.base = (int16_t)loot["base"].asInt(0);
		e.modulus = (int8_t)loot["modulus"].asInt(0);
		e.offset = (int8_t)loot["offset"].asInt(0);
		int chance = loot["chance"].asInt(100);
		if (chance < 0) chance = 0;
		if (chance > 100) chance = 100;
		e.chancePct = (uint8_t)chance;
		lc.drops.push_back(std::move(e));
	}

	lc.noCorpseLoot = loot["no_corpse_loot"].asBool(lc.noCorpseLoot);
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

static int ammoTypeFromName(const std::string& name) {
	int idx = ItemDefs::getAmmoIndex(name);
	return idx >= 0 ? idx : 0;
}

static int tileFromName(const std::string& name) {
	if (name.empty() || name == "0" || name == "none")
		return 0;
	int idx = SpriteDefs::getIndex(name);
	if (idx != 0)
		return idx;
	try { return std::stoi(name); } catch (...) {
		LOG_WARN("[app] Warning: unknown tile name '{}'\n", name.c_str());
		return 0;
	}
}

static int renderModeFromName(const std::string& name) {
	return Render::renderModeFromName(name);
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

// --- Parse functions ---

std::expected<void, std::string> parseSpriteAnims(Applet* app, const DataNode& config) {
	DataNode spriteAnims = config["sprite_anims"];
	if (spriteAnims && spriteAnims.isMap()) {
		for (auto it = spriteAnims.begin(); it != spriteAnims.end(); ++it) {
			int tileId = tileFromName(it.key().asString());
			if (tileId == 0) continue;
			DataNode sa = it.value();
			SpriteAnimDef def;
			DataNode rmNode = sa["render_mode"];
			if (rmNode)
				def.renderMode = renderModeFromName(rmNode.asString());
			DataNode scNode = sa["scale"];
			if (scNode)
				def.scale = scNode.asInt(0);
			DataNode nfNode = sa["num_frames"];
			if (nfNode)
				def.numFrames = nfNode.asInt(0);
			DataNode durNode = sa["duration"];
			if (durNode)
				def.duration = durNode.asInt(0);
			def.zAtGround = sa["z_at_ground"].asBool(false);
			def.zOffset = sa["z_offset"].asInt(0);
			def.randomFlip = sa["random_flip"].asBool(false);
			def.facePlayer = sa["face_player"].asBool(false);
			def.posOffset = sa["pos_offset"].asInt(0);
			gSpriteAnimDefs[tileId] = def;
		}
		LOG_INFO("[app] Animations: loaded {} sprite anim overrides\n", (int)gSpriteAnimDefs.size());
	}

	return {};
}

std::expected<void, std::string> parseWeapons(Applet* app, const DataNode& config) {
	DataNode weapons = config["weapons"];
	if (!weapons || !weapons.isMap()) {
		return std::unexpected("weapons section missing or not a map");
	}

	// First pass: find max index
	int maxIdx = -1;
	for (auto it = weapons.begin(); it != weapons.end(); ++it) {
		int idx = it.value()["index"].asInt(0);
		if (idx > maxIdx)
			maxIdx = idx;
	}
	int numWeapons = maxIdx + 1;

	// Allocate flat arrays
	app->combat->weapons = new int8_t[numWeapons * 9];
	std::memset(app->combat->weapons, 0, numWeapons * 9);
	app->combat->wpinfo = new int8_t[numWeapons * 6];
	std::memset(app->combat->wpinfo, 0, numWeapons * 6);
	app->combat->wpDisplayOffsetY = new int8_t[numWeapons];
	std::memset(app->combat->wpDisplayOffsetY, 0, numWeapons);
	app->combat->wpSwOffsetX = new int8_t[numWeapons];
	std::memset(app->combat->wpSwOffsetX, 0, numWeapons);
	app->combat->wpSwOffsetY = new int8_t[numWeapons];
	std::memset(app->combat->wpSwOffsetY, 0, numWeapons);
	app->combat->wpAttackSound = new int16_t[numWeapons];
	app->combat->wpAttackSoundAlt = new int16_t[numWeapons];
	app->combat->wpViewTile = new int16_t[numWeapons];
	app->combat->numWeaponViewTiles = numWeapons;
	app->combat->wpHudTexRow = new int8_t[numWeapons];
	app->combat->wpHudShowAmmo = new bool[numWeapons];
	for (int j = 0; j < numWeapons; j++) {
		app->combat->wpAttackSound[j] = -1;
		app->combat->wpAttackSoundAlt[j] = -1;
		app->combat->wpViewTile[j] = 0;  // 0 = not set, will use fallback formula
		app->combat->wpHudTexRow[j] = -1;  // -1 = not set, will use default
		app->combat->wpHudShowAmmo[j] = false;
	}
	app->combat->wpFlags = new Combat::WeaponFlags[numWeapons];
	app->combat->numWeaponFlags = numWeapons;
	std::memset(app->combat->wpFlags, 0, sizeof(Combat::WeaponFlags) * numWeapons);
	for (int j = 0; j < numWeapons; j++) {
		app->combat->wpFlags[j].attackSoundOverride = -1;
		app->combat->wpFlags[j].missSoundOverride = -1;
	}

	// Second pass: populate arrays
	for (auto wit = weapons.begin(); wit != weapons.end(); ++wit) {
		DataNode w = wit.value();
		int idx = w["index"].asInt(0);
		int base = idx * 9;

		// Damage
		DataNode dmg = w["damage"];
		if (dmg) {
			app->combat->weapons[base + 0] = (int8_t)dmg["min"].asInt(0);
			app->combat->weapons[base + 1] = (int8_t)dmg["max"].asInt(0);
		}

		// Range
		DataNode rng = w["range"];
		if (rng) {
			app->combat->weapons[base + 2] = (int8_t)rng["min"].asInt(0);
			app->combat->weapons[base + 3] = (int8_t)rng["max"].asInt(0);
		}

		// Ammo
		DataNode ammo = w["ammo"];
		if (ammo) {
			app->combat->weapons[base + 4] = (int8_t)ammoTypeFromName(ammo["type"].asString("none"));
			app->combat->weapons[base + 5] = (int8_t)ammo["usage"].asInt(0);
		}

		// Projectile, shots, shot_hold
		app->combat->weapons[base + 6] = (int8_t)projTypeFromName(w["projectile"].asString("none"));
		app->combat->weapons[base + 7] = (int8_t)w["shots"].asInt(1);
		app->combat->weapons[base + 8] = (int8_t)w["shot_hold"].asInt(0);

		// View offsets (idle/attack/flash positions)
		DataNode sprite = w["view_offsets"];
		if (sprite) {
			int wpBase = idx * 6;
			DataNode idle = sprite["idle"];
			if (idle) {
				app->combat->wpinfo[wpBase + 0] = (int8_t)idle["x"].asInt(0);
				app->combat->wpinfo[wpBase + 1] = (int8_t)idle["y"].asInt(0);
			}
			DataNode atk = sprite["attack"];
			if (atk) {
				app->combat->wpinfo[wpBase + 2] = (int8_t)atk["x"].asInt(0);
				app->combat->wpinfo[wpBase + 3] = (int8_t)atk["y"].asInt(0);
			}
			DataNode flash = sprite["flash"];
			if (flash) {
				app->combat->wpinfo[wpBase + 4] = (int8_t)flash["x"].asInt(0);
				app->combat->wpinfo[wpBase + 5] = (int8_t)flash["y"].asInt(0);
			}
		}

		// Display offsets (optional)
		DataNode disp = w["display"];
		if (disp) {
			app->combat->wpDisplayOffsetY[idx] = (int8_t)disp["offset_y"].asInt(0);
			DataNode sw = disp["sw_offset"];
			if (sw) {
				app->combat->wpSwOffsetX[idx] = (int8_t)sw["x"].asInt(0);
				app->combat->wpSwOffsetY[idx] = (int8_t)sw["y"].asInt(0);
			}
		}

		// Attack sound (optional)
		std::string atkSnd = w["attack_sound"].asString("");
		if (!atkSnd.empty()) {
			app->combat->wpAttackSound[idx] = (int16_t)Sounds::getResIDByName(atkSnd);
		}
		std::string atkSndAlt = w["attack_sound_alt"].asString("");
		if (!atkSndAlt.empty()) {
			app->combat->wpAttackSoundAlt[idx] = (int16_t)Sounds::getResIDByName(atkSndAlt);
		}

		// First-person weapon sprite tile index
		std::string vtName = w["sprite"].asString("");
		if (!vtName.empty()) {
			app->combat->wpViewTile[idx] = (int16_t)tileFromName(vtName);
		}

		// HUD texture row and ammo display
		DataNode hud = w["hud"];
		if (hud) {
			app->combat->wpHudTexRow[idx] = (int8_t)hud["tex_row"].asInt(-1);
			app->combat->wpHudShowAmmo[idx] = hud["show_ammo"].asBool(false);
		}

		// Weapon behavior flags
		DataNode beh = w["behavior"];
		if (beh) {
			auto& f = app->combat->wpFlags[idx];
			f.vibrateAnim = beh["vibrate_anim"].asBool(false);
			f.chainsawHitEvent = beh["chainsaw_hit_event"].asBool(false);
			f.alwaysHits = beh["always_hits"].asBool(false);
			f.doubleDamage = beh["double_damage"].asBool(false);
			f.isThrowableItem = beh["is_throwable_item"].asBool(false);
			f.showFlashFrame = beh["show_flash_frame"].asBool(false);
			f.hasFlashSprite = beh["has_flash_sprite"].asBool(false);
			f.soulAmmoDisplay = beh["soul_ammo_display"].asBool(false);
			f.autoHit = beh["auto_hit"].asBool(false);
			f.isMelee = beh["is_melee"].asBool(false);
			f.noRecoil = beh["no_recoil"].asBool(false);
			f.splashDamage = beh["splash_damage"].asBool(false);
			f.splashDivisor = beh["splash_divisor"].asInt(0);
			f.noBloodOnHit = beh["no_blood_on_hit"].asBool(false);
			f.drawDoubleSprite = beh["draw_double_sprite"].asBool(false);
			f.consumeOnUse = beh["consume_on_use"].asBool(false);
			f.chargeAttack = beh["charge_attack"].asBool(false);
			f.flipSpriteOnEnd = beh["flip_sprite_on_end"].asBool(false);
			f.poisonOnHit = beh["poison_on_hit"].asBool(false);
			std::string atkSndOvr = beh["attack_sound_override"].asString("none");
			f.attackSoundOverride = (atkSndOvr == "none") ? -1 : (int16_t)Sounds::getResIDByName(atkSndOvr);
			std::string missSndOvr = beh["miss_sound_override"].asString("none");
			f.missSoundOverride = (missSndOvr == "none") ? -1 : (int16_t)Sounds::getResIDByName(missSndOvr);
			f.meleeImpactAnim = (int16_t)beh["melee_impact_anim"].asInt(0);
			f.interactFlags = beh["interact_flags"].asInt(0);
			f.canLootCorpses = beh["can_loot_corpses"].asBool(false);
			f.fountainWeapon = beh["fountain_weapon"].asBool(false);
			f.isScoped = beh["is_scoped"].asBool(false);
			f.fullAgility = beh["full_agility"].asBool(false);
			f.requiresLineOfSight = beh["requires_line_of_sight"].asBool(false);
			f.blockableByShield = beh["blockable_by_shield"].asBool(false);
			f.outOfRangeStillFires = beh["out_of_range_still_fires"].asBool(false);
			f.requiresLosPath = beh["requires_los_path"].asBool(false);
		}

		// On-kill stat grant (e.g. chainsaw: every 30 kills grants +2 strength)
		DataNode okg = w["on_kill_grant"];
		if (okg) {
			auto& f = app->combat->wpFlags[idx];
			f.onKillGrantKills = okg["kills"].asInt(0);
			f.onKillGrantStrength = okg["strength"].asInt(0);
		}
	}

	// Cache special weapon indices and ammo types for quick lookups
	app->combat->throwableItemAmmoType = -1;
	app->combat->throwableItemWeaponIdx = -1;
	app->combat->fountainWeaponIdx = -1;
	app->combat->fountainAmmoType = -1;
	app->combat->soulWeaponIdx = -1;
	for (int w = 0; w < numWeapons; w++) {
		if (app->combat->wpFlags[w].isThrowableItem && app->combat->throwableItemWeaponIdx < 0) {
			app->combat->throwableItemWeaponIdx = w;
			app->combat->throwableItemAmmoType = app->combat->weapons[w * 9 + Combat::WEAPON_FIELD_AMMOTYPE];
		}
		if (app->combat->wpFlags[w].fountainWeapon && app->combat->fountainWeaponIdx < 0) {
			app->combat->fountainWeaponIdx = w;
			app->combat->fountainAmmoType = app->combat->weapons[w * 9 + Combat::WEAPON_FIELD_AMMOTYPE];
		}
		if (app->combat->wpFlags[w].soulAmmoDisplay && app->combat->soulWeaponIdx < 0) {
			app->combat->soulWeaponIdx = w;
		}
	}

	// Build weapon name->index map for familiar cross-references
	std::map<std::string, int> weaponNameToIndex;
	for (auto wit2 = weapons.begin(); wit2 != weapons.end(); ++wit2) {
		std::string name = wit2.key().asString("");
		int idx = wit2.value()["index"].asInt(-1);
		if (!name.empty() && idx >= 0) {
			weaponNameToIndex[name] = idx;
		}
	}

	// Load familiar definitions from weapons with familiar: section
	std::vector<Combat::FamiliarDef> famDefs;
	for (auto wit3 = weapons.begin(); wit3 != weapons.end(); ++wit3) {
		DataNode fam = wit3.value()["familiar"];
		if (!fam) continue;

		Combat::FamiliarDef fd{};
		fd.weaponIndex = wit3.value()["index"].asInt(0);
		fd.familiarType = (int16_t)fam["type"].asInt(1);
		fd.postProcess = (int8_t)fam["post_process"].asInt(0);
		fd.explodes = fam["explodes"].asBool(false);
		fd.explodeWeaponIndex = -1;
		fd.deathRemainsWeapon = -1;
		fd.helpId = (int16_t)fam["help_id"].asInt(12);

		std::string explWp = fam["explode_weapon"].asString("");
		if (!explWp.empty()) {
			if (auto it = weaponNameToIndex.find(explWp); it != weaponNameToIndex.end()) fd.explodeWeaponIndex = it->second;
		}
		std::string deathWp = fam["death_remains_weapon"].asString("");
		if (!deathWp.empty()) {
			if (auto it = weaponNameToIndex.find(deathWp); it != weaponNameToIndex.end()) fd.deathRemainsWeapon = it->second;
		}

		fd.canShoot = fam["can_shoot"].asBool(false);
		fd.selfDestructs = fam["self_destructs"].asBool(false);
		fd.hudFaceRow = (int8_t)fam["hud_face_row"].asInt(0);

		famDefs.push_back(fd);
	}

	app->combat->familiarDefCount = (int)famDefs.size();
	if (!famDefs.empty()) {
		app->combat->familiarDefs = new Combat::FamiliarDef[famDefs.size()];
		std::ranges::copy(famDefs, app->combat->familiarDefs);
	}
	app->combat->defaultFamiliarType = 1;

	// Cache familiar ammo type from first familiar weapon
	app->combat->familiarAmmoType = -1;
	if (!famDefs.empty()) {
		int famW = famDefs[0].weaponIndex;
		app->combat->familiarAmmoType = app->combat->weapons[famW * 9 + Combat::WEAPON_FIELD_AMMOTYPE];
	}

	LOG_INFO("[app] Weapons: loaded {} weapon definitions ({} familiars)\n",
		(int)weapons.size(), (int)famDefs.size());
	return {};
}

std::expected<void, std::string> parseProjectiles(Applet* app, const DataNode& config) {
	DataNode projectilesNode = config["projectiles"];
	if (!projectilesNode) {
		LOG_WARN("[app] Warning: projectiles.yaml empty, using defaults\n");
		return {};
	}

	if (!projectilesNode.isMap()) {
		LOG_WARN("[app] Warning: projectiles section is not a map\n");
		return {};
	}
	int count = (int)projectilesNode.size();
	app->combat->numProjTypes = count;
	app->combat->projVisuals = new Combat::ProjVisual[count];
	std::memset(app->combat->projVisuals, 0, count * sizeof(Combat::ProjVisual));
	for (int i = 0; i < count; i++) {
		app->combat->projVisuals[i].launchRenderMode = -1; // -1 = no missile
		app->combat->projVisuals[i].impactSound = -1;
		app->combat->projVisuals[i].reflectBuffId = -1;
	}

	int i = 0;
	for (auto pit = projectilesNode.begin(); pit != projectilesNode.end(); ++pit, ++i) {
		DataNode p = pit.value();

		DataNode launch = p["launch"];
		if (launch) {
			auto& pv = app->combat->projVisuals[i];
			pv.launchRenderMode = (int8_t)renderModeFromName(launch["render_mode"].asString("normal"));
			pv.launchAnim = (int16_t)tileFromName(launch["sprite"].asString(launch["anim"].asString("0")));
			pv.launchAnimMonster = (int16_t)tileFromName(launch["sprite_monster"].asString(launch["anim_monster"].asString("0")));
			pv.launchSpeed = (int16_t)launch["speed"].asInt(0);
			pv.launchSpeedAdd = (int16_t)launch["speed_add"].asInt(0);
			pv.launchAnimFromWeapon = launch["sprite_from_weapon"].asBool(launch["anim_from_weapon"].asBool(false));
			pv.launchZOffset = (int16_t)launch["z_offset"].asInt(0);
			DataNode off = launch["offset"];
			if (off) {
				pv.launchOffsetXR = (int8_t)off["x_right"].asInt(0);
				pv.launchOffsetYR = (int8_t)off["y_right"].asInt(0);
				pv.launchOffsetZ = (int8_t)off["z"].asInt(0);
			}
			// Support sprite_player as override for sprite
			DataNode spritePlayer = launch["sprite_player"];
			if (!spritePlayer) spritePlayer = launch["anim_player"]; // backwards compat
			if (spritePlayer)
				pv.launchAnim = (int16_t)tileFromName(spritePlayer.asString("0"));
			DataNode spriteMonster = launch["sprite_monster"];
			if (!spriteMonster) spriteMonster = launch["anim_monster"]; // backwards compat
			if (spriteMonster)
				pv.launchAnimMonster = (int16_t)tileFromName(spriteMonster.asString("0"));
			// Launch behaviors
			int crZAdj = launch["close_range_z_adjust"].asInt(0);
			if (crZAdj != 0) {
				pv.closeRangeZAdjust = true;
				pv.closeRangeZAmount = (int16_t)crZAdj;
			}
			pv.monsterDamageBoost = launch["monster_damage_boost"].asBool(false);
			pv.resetThornParticles = launch["reset_thorn_particles"].asBool(false);
		}

		DataNode impact = p["impact"];
		if (impact) {
			auto& pv = app->combat->projVisuals[i];
			pv.impactAnim = (int16_t)tileFromName(impact["sprite"].asString(impact["anim"].asString("0")));
			pv.impactRenderMode = (int8_t)renderModeFromName(impact["render_mode"].asString("normal"));
			std::string snd = impact["impact_sound"].asString("");
			if (!snd.empty()) {
				pv.impactSound = (int16_t)Sounds::getResIDByName(snd);
			}
			DataNode shake = impact["screen_shake"];
			if (shake) {
				pv.impactScreenShake = true;
				pv.shakeDuration = (int16_t)shake["duration"].asInt(500);
				pv.shakeIntensity = (int8_t)shake["intensity"].asInt(4);
				pv.shakeFade = (int16_t)shake["fade"].asInt(500);
			}
			// Impact behaviors
			pv.causesFear = impact["causes_fear"].asBool(false);
			pv.soulCubeReturn = impact["soul_cube_return"].asBool(false);
			pv.impactParticlesOnSprite = impact["particles_on_sprite"].asBool(false);
			pv.impactZOffset = (int16_t)impact["impact_z_offset"].asInt(0);
			int reflectBuff = impact["reflects_with_buff"].asInt(-1);
			if (reflectBuff >= 0) {
				pv.reflectsWithBuff = true;
				pv.reflectBuffId = (int8_t)reflectBuff;
				pv.reflectAnim = (int16_t)tileFromName(impact["reflect_sprite"].asString(impact["reflect_anim"].asString("0")));
				pv.reflectRenderMode = (int8_t)renderModeFromName(impact["reflect_render_mode"].asString("normal"));
			}
			DataNode particles = impact["particles"];
			if (particles) {
				pv.impactParticles = true;
				std::string colorStr = particles["color"].asString("0xFFFFFFFF");
				pv.particleColor = (uint32_t)std::stoul(colorStr, nullptr, 0);
				pv.particleZOffset = (int16_t)particles["z_offset"].asInt(0);
			}
		}
	}

	LOG_INFO("[app] Projectiles: loaded {} types\n", count);
	return {};
}

// Parse a hex color from yaml. Accepts integer (0xAARRGGBB) or string ("0xAARRGGBB" / "AARRGGBB").
static bool parseHexColor(const DataNode& node, uint32_t& out) {
	if (!node) return false;
	if (node.isScalar()) {
		std::string s = node.asString("");
		if (s.empty()) return false;
		try { out = (uint32_t)std::stoul(s, nullptr, 0); return true; }
		catch (...) { return false; }
	}
	out = (uint32_t)node.asInt(0);
	return true;
}

std::expected<void, std::string> parseParticles(Applet* app, const DataNode& config) {
	if (!app->particleSystem) return {};
	std::vector<uint32_t> palette;
	if (config) {
		DataNode p = config["default_palette"];
		if (p && p.size() > 0) {
			for (int i = 0; i < p.size(); i++) {
				uint32_t c;
				if (parseHexColor(p[i], c)) palette.push_back(c);
			}
		}
	}
	app->particleSystem->setLevelPalette(palette); // empty -> default fallback
	LOG_INFO("[app] Particles: palette has {} colors\n", (int)app->particleSystem->levelColors.size());
	return {};
}

std::expected<void, std::string> parseControls(Applet* app, const DataNode& config) {
	(void)app;
	if (!config) return {};
	DataNode actions = config["actions"];
	if (!actions) return {};

	// Build action_name → slot index map from the canonical keyBindingNames table.
	std::unordered_map<std::string, int> slotByName;
	for (int i = 0; i < KEY_MAPPIN_MAX; i++) {
		slotByName[keyBindingNames[i]] = i;
	}

	for (auto it = actions.begin(); it != actions.end(); ++it) {
		std::string actionName = it.key().asString("");
		auto sit = slotByName.find(actionName);
		if (sit == slotByName.end()) {
			LOG_WARN("[app] controls.yaml: unknown action '{}' — ignored\n", actionName);
			continue;
		}
		int slot = sit->second;
		DataNode binds = it.value();
		// Reset all keybinds for this slot to "unbound", then fill from yaml.
		for (int j = 0; j < KEYBINDS_MAX; j++) {
			keyMappingDefault[slot].keyBinds[j] = -1;
		}
		if (binds.isScalar()) {
			keyMappingDefault[slot].keyBinds[0] = inputCodeFromName(binds.asString(""));
		} else {
			int n = std::min((int)binds.size(), (int)KEYBINDS_MAX);
			for (int j = 0; j < n; j++) {
				keyMappingDefault[slot].keyBinds[j] = inputCodeFromName(binds[j].asString(""));
			}
		}
	}

	// Re-apply defaults to live mappings so this takes effect even though
	// Input::init() already ran with the compiled-in defaults.
	std::memcpy(keyMapping, keyMappingDefault, sizeof(keyMapping));
	std::memcpy(keyMappingTemp, keyMappingDefault, sizeof(keyMapping));

	LOG_INFO("[app] Controls: applied {} action overrides from controls.yaml\n", (int)actions.size());
	return {};
}

std::expected<void, std::string> parseEffects(Applet* app, const DataNode& config) {
	// Initialize defaults matching original hardcoded values
	Player* p = app->player.get();
	for (int i = 0; i < 15; i++) {
		p->buffMaxStacks[i] = 3;
		p->buffBlockedBy[i] = -1;
		p->buffApplySound[i] = -1;
		p->buffPerTurnDamage[i] = 0;
		p->buffPerTurnHealByAmount[i] = false;
	}
	// Default buff durations (matching original hardcoded values)
	for (int i = 0; i < Player::MAX_BUFF_IDS; i++) {
		p->buffDuration[i] = 30; // DEF_STATUS_TURNS
	}
	p->buffDuration[Enums::BUFF_HASTE] = 20;
	p->buffDuration[Enums::BUFF_ANTIFIRE] = 10;
	p->buffDuration[Enums::BUFF_PURIFY] = 10;
	p->buffDuration[Enums::BUFF_FEAR] = 6;
	p->buffDuration[17] = 5; // cold/frost fog effect (buff ID 17, beyond enum range)
	// Original hardcoded: wp_poison and antifire have max 1 stack
	p->buffMaxStacks[Enums::BUFF_WP_POISON] = 1;
	p->buffMaxStacks[Enums::BUFF_ANTIFIRE] = 1;
	// Original hardcoded: fire blocked by antifire
	p->buffBlockedBy[Enums::BUFF_FIRE] = Enums::BUFF_ANTIFIRE;
	// Original hardcoded: fire per-turn damage = 3
	p->buffPerTurnDamage[Enums::BUFF_FIRE] = 3;
	// Original hardcoded: regen heals by amount
	p->buffPerTurnHealByAmount[Enums::BUFF_REGEN] = true;
	// Original hardcoded constants
	p->buffNoAmountMask = Enums::BUFF_NO_AMOUNT;
	p->buffAmtNotDrawnMask = Enums::BUFF_AMT_NOT_DRAWN;
	p->buffWarningTime = Enums::BUFF_WARNING_TIME;

	DataNode wtNode = config["warning_time"];
	if (wtNode) {
		p->buffWarningTime = wtNode.asInt(10);
	}

	DataNode buffs = config["buffs"];
	if (!buffs) {
		LOG_WARN("[app] Warning: effects.yaml has no buffs section, using defaults\n");
		return {};
	}

	const int maxBuffs = 15; // matches buffMaxStacks[15] etc. in Player.h
	if ((int)buffs.size() > maxBuffs) {
		LOG_WARN("[app] effects.yaml has {} buffs, but engine cap is {}. Extra buffs ignored.\n",
			(int)buffs.size(), maxBuffs);
	}
	int count = std::min((int)buffs.size(), maxBuffs);

	// Build name->index lookup from order (must match buff IDs 0-14)
	std::map<std::string, int> nameToIndex;
	{
		int bi = 0;
		for (auto it = buffs.begin(); it != buffs.end() && bi < count; ++it, ++bi) {
			std::string name = it.key().asString("");
			if (!name.empty()) {
				nameToIndex[name] = bi;
			}
		}
	}

	// Rebuild bitmasks from per-buff flags
	p->buffNoAmountMask = 0;
	p->buffAmtNotDrawnMask = 0;

	int i = 0;
	for (auto bit = buffs.begin(); bit != buffs.end() && i < count; ++bit, ++i) {
		DataNode b = bit.value();

		p->buffMaxStacks[i] = (int8_t)b["max_stacks"].asInt(3);

		// duration: override default if specified in YAML
		DataNode durNode = b["duration"];
		if (durNode) {
			p->buffDuration[i] = (int8_t)durNode.asInt(30);
		}

		bool hasAmount = b["has_amount"].asBool(true);
		bool drawAmount = b["draw_amount"].asBool(true);
		if (!hasAmount) {
			p->buffNoAmountMask |= (1 << i);
			p->buffAmtNotDrawnMask |= (1 << i);
		} else if (!drawAmount) {
			p->buffAmtNotDrawnMask |= (1 << i);
		}

		// blocked_by: resolve buff name to index
		std::string blockedBy = b["blocked_by"].asString("");
		if (!blockedBy.empty()) {
			if (auto it = nameToIndex.find(blockedBy); it != nameToIndex.end()) {
				p->buffBlockedBy[i] = (int8_t)it->second;
			}
		}

		// on_apply_sound: resolve sound name
		std::string applySound = b["on_apply_sound"].asString("");
		if (!applySound.empty()) {
			p->buffApplySound[i] = (int16_t)Sounds::getResIDByName(applySound);
		}

		// per_turn_damage: flat damage per turn
		p->buffPerTurnDamage[i] = (int8_t)b["per_turn_damage"].asInt(0);

		// per_turn: heal_by_amount
		std::string perTurn = b["per_turn"].asString("");
		p->buffPerTurnHealByAmount[i] = (perTurn == "heal_by_amount");
	}

	auto resolve = [&](const char* name, int fallback) -> int {
		auto it = nameToIndex.find(name);
		return (it != nameToIndex.end()) ? it->second : fallback;
	};
	p->reflectBuffIdx   = resolve("reflect",   Enums::BUFF_REFLECT);
	p->purifyBuffIdx    = resolve("purify",    Enums::BUFF_PURIFY);
	p->hasteBuffIdx     = resolve("haste",     Enums::BUFF_HASTE);
	p->regenBuffIdx     = resolve("regen",     Enums::BUFF_REGEN);
	p->defenseBuffIdx   = resolve("defense",   Enums::BUFF_DEFENSE);
	p->strengthBuffIdx  = resolve("strength",  Enums::BUFF_STRENGTH);
	p->agilityBuffIdx   = resolve("agility",   Enums::BUFF_AGILITY);
	p->focusBuffIdx     = resolve("focus",     Enums::BUFF_FOCUS);
	p->angerBuffIdx     = resolve("anger",     Enums::BUFF_ANGER);
	p->antifireBuffIdx  = resolve("antifire",  Enums::BUFF_ANTIFIRE);
	p->fortitudeBuffIdx = resolve("fortitude", Enums::BUFF_FORTITUDE);
	p->fearBuffIdx      = resolve("fear",      Enums::BUFF_FEAR);
	p->wpPoisonBuffIdx  = resolve("wp_poison", Enums::BUFF_WP_POISON);
	p->fireBuffIdx      = resolve("fire",      Enums::BUFF_FIRE);
	p->diseaseBuffIdx   = resolve("disease",   Enums::BUFF_DISEASE);

	LOG_INFO("[app] Effects: loaded {} buffs\n", count);
	return {};
}

std::expected<void, std::string> parseDialogStyles(Applet* app, const DataNode& config) {
	Canvas* c = app->canvas.get();

	// Initialize with hardcoded defaults
	//                                                                                              hasHdr grad  port  cust  chest iconX iconY  iW  iH  iBot green
	static const DialogStyleDef defaults[] = {
		{ 0, (int)0xFF000000, -1, -1, 0, 0, false,                                                 false, false, false, false, false, -1, 0, 0, 0, false, false },  // normal
		{ 1, (int)0xFF002864, -1, -1, 0, 0, false,                                                 false, false, false, false, false, 10, 0, 10, 6, false, false },  // npc (icon: speech bubble)
		{ 2, (int)0xFF000000, -1, (int)0xFF000000, 0, 0, false,                                    true,  false, false, false, false, -1, 0, 0, 0, false, false },  // help
		{ 3, 0, -1, -1, 0, 0, false,                                                               false, false, false, true,  false, -1, 0, 0, 0, false, false },  // scroll (custom draw)
		{ 4, (int)0xFF005A00, (int)0xFFB18A01, -1, 0, 0, false,                                    false, false, false, false, true,  -1, 0, 0, 0, false, false },  // chest
		{ 5, (int)0xFF800000, -1, -1, 0, 2, false,                                                 false, false, false, false, false,  0,12, 10, 6, false, false },  // monster (icon: skull, top/bottom by flag)
		{ 6, (int)0xFF002864, -1, -1, 0, 0, false,                                                 false, false, false, false, false, -1, 0, 0, 0, false, false },  // ghost
		{ 7, (int)0xFF000000, -1, -1, 0, 0, false,                                                 false, false, false, false, false, -1, 0, 0, 0, false, false },  // yell
		{ 8, Canvas::PLAYER_DLG_COLOR, -1, -1, -64, 0, false,                                      false, true,  true,  false, false, 30, 0, 15, 9, true,  false },  // player (gradient + portrait + icon)
		{ 9, (int)0xFF000000, -1, (int)0xFF000000, 0, 0, false,                                    true,  false, false, false, false, -1, 0, 0, 0, false, true  },  // terminal (header + green text)
		{10, (int)0xFF2E0854, -1, -1, 0, 0, true,                                                  false, false, false, false, false, 20, 6, 10, 6, true,  false },  // elevator (icon at bottom)
		{11, (int)0xFF800000, -1, -1, 0, 2, false,                                                 false, false, false, false, false, -1, 0, 0, 0, false, false },  // vios
		{12, (int)0xFFB18A01, -1, -1, 0, 0, false,                                                 false, false, false, false, false, -1, 0, 0, 0, false, false },  // self_destruct_confirm
		{13, (int)0xFFB18A01, -1, -1, 0, 0, false,                                                 false, false, false, false, false, -1, 0, 0, 0, false, false },  // armor_repair
		{14, (int)0xFF002864, -1, -1, -20, 0, false,                                               false, false, false, false, false, 45, 0, 15, 9, true,  false },  // comm_link (icon at bottom)
		{15, (int)0xFFFF9600, -1, -1, 0, 0, false,                                                 false, false, false, false, false, -1, 0, 0, 0, false, false },  // sal
		{16, (int)0xFF000000, -1, (int)0xFF000066, 0, 0, false,                                    true,  false, false, false, false, -1, 0, 0, 0, false, false },  // special (header)
	};
	int defaultCount = sizeof(defaults) / sizeof(defaults[0]);

	if (c->dialogStyleDefs) {
		delete[] c->dialogStyleDefs;
	}
	c->dialogStyleDefs = new DialogStyleDef[defaultCount];
	c->dialogStyleDefCount = defaultCount;
	std::copy(defaults, defaults + defaultCount, c->dialogStyleDefs);

	DataNode styles = config["dialog_styles"];
	if (!styles) {
		LOG_WARN("[app] Warning: dialogs.yaml has no dialog_styles section\n");
		return {};
	}

	for (auto it = styles.begin(); it != styles.end(); ++it) {
		DataNode s = it.value();
		int idx = s["index"].asInt(-1);
		if (idx < 0 || idx >= c->dialogStyleDefCount) continue;

		DialogStyleDef& def = c->dialogStyleDefs[idx];
		DataNode bgNode = s["bg_color"];
		if (bgNode)
			def.bgColor = (int)(unsigned int)bgNode.asInt((int)def.bgColor);
		DataNode altBgNode = s["alt_bg_color"];
		if (altBgNode)
			def.altBgColor = (int)(unsigned int)altBgNode.asInt((int)def.altBgColor);
		DataNode headerNode = s["header_color"];
		if (headerNode)
			def.headerColor = (int)(unsigned int)headerNode.asInt((int)def.headerColor);
		DataNode yAdjNode = s["y_adjust"];
		if (yAdjNode)
			def.yAdjust = yAdjNode.asInt(def.yAdjust);
		DataNode ptofNode = s["position_top_on_flag"];
		if (ptofNode)
			def.posTopOnFlag = ptofNode.asInt(def.posTopOnFlag);
		DataNode ptNode = s["position_top"];
		if (ptNode)
			def.positionTop = ptNode.asBool(def.positionTop);
	}

	LOG_INFO("[app] Dialog styles: loaded\n");
	return {};
}

std::expected<void, std::string> parseItems(Applet* app, const DataNode& config, const DataNode& effectsConfig) {
	Player* p = app->player.get();
	if (!p->itemDefs) {
		p->itemDefs = new std::vector<ItemDef>();
	}
	p->itemDefs->clear();

	// Build buff name->index map from effects config order
	std::map<std::string, int> buffNameToIndex;
	DataNode ecBuffs = effectsConfig["buffs"];
	if (ecBuffs) {
		int bi = 0;
		for (auto it = ecBuffs.begin(); it != ecBuffs.end() && bi < 15; ++it, ++bi) {
			std::string name = it.key().asString("");
			if (!name.empty()) {
				buffNameToIndex[name] = bi;
			}
		}
	}
	if (buffNameToIndex.empty()) {
		// Hardcoded fallback
		buffNameToIndex = {
			{"reflect", 0}, {"purify", 1}, {"haste", 2}, {"regen", 3},
			{"defense", 4}, {"strength", 5}, {"agility", 6}, {"focus", 7},
			{"anger", 8}, {"antifire", 9}, {"fortitude", 10}, {"fear", 11},
			{"wp_poison", 12}, {"fire", 13}, {"disease", 14}
		};
	}

	auto resolveBuff = [&](const std::string& name) -> int {
		if (auto it = buffNameToIndex.find(name); it != buffNameToIndex.end()) return it->second;
		return -1;
	};

	auto parseEffect = [&](const DataNode& node) -> ItemEffect {
		ItemEffect e{};
		std::string type = node["type"].asString("");
		if (type == "health") {
			e.type = ItemEffect::HEALTH;
			e.amount = node["amount"].asInt(0);
		} else if (type == "armor") {
			e.type = ItemEffect::ARMOR;
			e.amount = node["amount"].asInt(0);
		} else if (type == "status_effect") {
			e.type = ItemEffect::STATUS_EFFECT;
			e.buffIndex = resolveBuff(node["buff"].asString(""));
			e.amount = node["amount"].asInt(0);
			e.duration = node["duration"].asInt(0);
		}
		return e;
	};

	DataNode items = config["items"];
	if (!items) {
		return std::unexpected("items.yaml has no items section");
	}

	for (auto it = items.begin(); it != items.end(); ++it) {
		DataNode node = it.value();
		ItemDef def{};
		def.index = node["index"].asInt(-1);
		if (def.index < 0) continue;

		std::string mode = node["require_mode"].asString("all");
		def.requireAll = (mode == "all");
		def.advanceTurn = node["advance_turn"].asBool(true);
		def.consume = node["consume"].asBool(true);
		def.skipEmptyCheck = node["skip_empty_check"].asBool(false);

		std::string reqNoBuff = node["requires_no_buff"].asString("");
		def.requiresNoBuff = reqNoBuff.empty() ? -1 : resolveBuff(reqNoBuff);

		DataNode effectsNode = node["effects"];
		if (effectsNode) {
			for (auto eit = effectsNode.begin(); eit != effectsNode.end(); ++eit) {
				def.effects.push_back(parseEffect(eit.value()));
			}
		}
		DataNode bonusNode = node["bonus_effects"];
		if (bonusNode) {
			for (auto eit = bonusNode.begin(); eit != bonusNode.end(); ++eit) {
				def.bonusEffects.push_back(parseEffect(eit.value()));
			}
		}
		DataNode rbNode = node["remove_buffs"];
		if (rbNode) {
			for (auto rit = rbNode.begin(); rit != rbNode.end(); ++rit) {
				int idx = resolveBuff(rit.value().asString(""));
				if (idx >= 0) def.removeBuffs.push_back(idx);
			}
		}

		def.ammoCostType = -1;
		def.ammoCostAmount = 0;
		DataNode acNode = node["ammo_cost"];
		if (acNode) {
			def.ammoCostType = acNode["type"].asInt(-1);
			def.ammoCostAmount = acNode["amount"].asInt(0);
		}

		p->itemDefs->push_back(def);
	}

	LOG_INFO("[app] Items: loaded {} item definitions\n", (int)p->itemDefs->size());
	return {};
}

std::expected<void, std::string> parseTables(Applet* app, const DataNode& config) {
	// === CombatMasks ===
	DataNode masks = config["combat_masks"];
	if (masks) {
		int count = (int)masks.size();
		app->combat->tableCombatMasks = new int32_t[count];
		for (int i = 0; i < count; i++) {
			std::string val = masks[i].asString("0");
			if (val.starts_with("0x") || val.starts_with("0X")) {
				app->combat->tableCombatMasks[i] = (int32_t)std::stoul(val, nullptr, 16);
			} else {
				app->combat->tableCombatMasks[i] = (int32_t)std::stoi(val);
			}
		}
	}

	// === KeysNumeric ===
	DataNode kn = config["keys_numeric"];
	if (kn) {
		DataNode data = kn["data"];
		if (data && data.isSequence()) {
			int count = (int)data.size();
			app->canvas->keys_numeric = new int8_t[count];
			for (int i = 0; i < count; i++) {
				app->canvas->keys_numeric[i] = (int8_t)data[i].asInt(0);
			}
		}
	}

	// === OSCCycle ===
	DataNode osc = config["osc_cycle"];
	if (osc) {
		DataNode data = osc["data"];
		if (data && data.isSequence()) {
			int count = (int)data.size();
			app->canvas->OSC_CYCLE = new int8_t[count];
			for (int i = 0; i < count; i++) {
				app->canvas->OSC_CYCLE[i] = (int8_t)data[i].asInt(0);
			}
		}
	}

	// === LevelNames ===
	DataNode ln = config["level_names"];
	if (ln) {
		int count = (int)ln.size();
		app->game->levelNames = new int16_t[count];
		for (int i = 0; i < count; i++) {
			app->game->levelNames[i] = (int16_t)ln[i].asInt(0);
		}
		app->game->levelNamesCount = count;
	}

	// === SinTable ===
	DataNode st = config["sin_table"];
	if (st) {
		DataNode data = st["data"];
		if (data && data.isSequence()) {
			int count = (int)data.size();
			app->render->sinTable = new int32_t[count];
			for (int i = 0; i < count; i++) {
				app->render->sinTable[i] = (int32_t)data[i].asInt(0);
			}
		}
	}

	// === EnergyDrinkData ===
	DataNode ed = config["energy_drink_data"];
	if (ed) {
		DataNode data = ed["data"];
		if (data && data.isSequence()) {
			int count = (int)data.size();
			app->vendingMachine->energyDrinkData = new int16_t[count];
			for (int i = 0; i < count; i++) {
				app->vendingMachine->energyDrinkData[i] = (int16_t)data[i].asInt(0);
			}
		}
	}

	// === MovieEffects ===
	DataNode me = config["movie_effects"];
	if (me) {
		DataNode data = me["data"];
		if (data && data.isSequence()) {
			int count = (int)data.size();
			app->canvas->movieEffects = new int32_t[count];
			for (int i = 0; i < count; i++) {
				app->canvas->movieEffects[i] = (int32_t)data[i].asInt(0);
			}
		}
	}

	LOG_INFO("[app] loadTables: loaded tables\n");
	return {};
}

// parseMonsters() removed — combat data now loaded from entities.yaml by parseMonsterCombatFromEntities()

#if 0 // Legacy parseMonsters — kept for reference, will be deleted
bool parseMonsters(Applet* app, const DataNode& config) {
	DataNode monsters = config["monsters"];
	if (!monsters || !monsters.isMap()) {
		return false;
	}

	// First pass: find max index to size arrays
	int maxIdx = -1;
	for (auto it = monsters.begin(); it != monsters.end(); ++it) {
		int idx = it.value()["index"].asInt(0);
		if (idx > maxIdx) maxIdx = idx;
	}
	int numTypes = maxIdx + 1;
	int numTiers = 3;
	int totalEntities = numTypes * numTiers;

	// Store counts for dynamic monster array sizing
	app->combat->numMonsterTypes = numTypes;
	app->combat->tiersPerMonster = numTiers;
	app->combat->monsterSlotCount = totalEntities;

	// Allocate flat arrays
	app->combat->monsterAttacks = new short[numTypes * 9];
	std::memset(app->combat->monsterAttacks, 0, numTypes * 9 * sizeof(short));

	app->combat->monsterStats = new int8_t[totalEntities * 6];
	std::memset(app->combat->monsterStats, 0, totalEntities * 6);

	app->combat->monsterWeakness = new int8_t[totalEntities * 8];
	std::memset(app->combat->monsterWeakness, 0, totalEntities * 8);

	app->game->monsterSounds = new uint8_t[totalEntities * 8];
	std::memset(app->game->monsterSounds, 255, totalEntities * 8);

	app->particleSystem->monsterColors = new uint8_t[numTypes * 3];
	std::memset(app->particleSystem->monsterColors, 0, numTypes * 3);

	app->combat->monsterDefs = new MonsterDef[numTypes]();
	app->combat->monsterBehaviors = app->combat->monsterDefs;

	static const char* soundFields[] = {"alert1", "alert2", "alert3", "attack1",
	                                    "attack2", "idle", "pain", "death"};
	static const char* statFields[] = {"health", "armor", "defense", "strength",
	                                   "accuracy", "agility"};

	// Build monster name -> index map
	for (auto mit = monsters.begin(); mit != monsters.end(); ++mit) {
		std::string name = mit.key().asString("");
		int idx = mit.value()["index"].asInt(0);
		if (!name.empty()) {
			app->combat->monsterNameToIndex[name] = idx;
		}
	}

	// Second pass: populate arrays
	for (auto mit = monsters.begin(); mit != monsters.end(); ++mit) {
		DataNode m = mit.value();
		int idx = m["index"].asInt(0);

		// Blood color (optional) - hex string "#RRGGBB" or legacy [R, G, B] array
		DataNode bc = m["blood_color"];
		if (bc) {
			if (bc.isScalar()) {
				std::string hex = bc.asString();
				if (hex.size() >= 7 && hex[0] == '#') {
					app->particleSystem->monsterColors[idx * 3 + 0] = (uint8_t)std::stoi(hex.substr(1, 2), nullptr, 16);
					app->particleSystem->monsterColors[idx * 3 + 1] = (uint8_t)std::stoi(hex.substr(3, 2), nullptr, 16);
					app->particleSystem->monsterColors[idx * 3 + 2] = (uint8_t)std::stoi(hex.substr(5, 2), nullptr, 16);
				}
			} else if (bc.isSequence() && bc.size() >= 3) {
				app->particleSystem->monsterColors[idx * 3 + 0] = (uint8_t)bc[0].asInt(0);
				app->particleSystem->monsterColors[idx * 3 + 1] = (uint8_t)bc[1].asInt(0);
				app->particleSystem->monsterColors[idx * 3 + 2] = (uint8_t)bc[2].asInt(0);
			}
		}

		// Sounds (type-level defaults, copied to all tiers)
		DataNode snd = m["sounds"];
		if (snd) {
			uint8_t typeSounds[8];
			for (int f = 0; f < 8; f++) {
				std::string val = snd[soundFields[f]].asString("none");
				if (val == "none") {
					typeSounds[f] = 255;
				} else {
					int soundIdx = Sounds::getIndexByName(val);
					typeSounds[f] = (soundIdx >= 0) ? (uint8_t)soundIdx : (uint8_t)std::atoi(val.c_str());
				}
			}
			// Copy to all tiers for this monster type
			for (int t = 0; t < numTiers; t++) {
				int sndBase = (idx * numTiers + t) * 8;
				std::memcpy(&app->game->monsterSounds[sndBase], typeSounds, 8);
			}
		}

		// Behaviors
		DataNode beh = m["behaviors"];
		if (beh) {
			MonsterBehaviors& mb = app->combat->monsterBehaviors[idx];
			mb.isBoss = beh["is_boss"].asBool(false);
			mb.bossMinTier = beh["boss_min_tier"].asInt(0);
			mb.fearImmune = beh["fear_immune"].asBool(false);
			mb.evading = beh["evading"].asBool(false);
			mb.moveToAttack = beh["move_to_attack"].asBool(false);
			mb.canResurrect = beh["can_resurrect"].asBool(false);
			mb.onHitPoison = beh["on_hit_poison"].asBool(false);
			mb.floats = beh["floats"].asBool(false);
			mb.isVios = beh["is_vios"].asBool(false);
			mb.boneGibs = beh["bone_gibs"].asBool(false);
			mb.smallParm0Scale = beh["small_parm0_scale"].asInt(-1);
			// AI behavior parameters
			mb.pathSearchDepth = beh["path_search_depth"].asInt(8);
			mb.movementTimeMs = beh["movement_time_ms"].asInt(275);
			mb.chaseLosWeight = beh["chase_los_weight"].asInt(-4);
			mb.retreatLosWeight = beh["retreat_los_weight"].asInt(0);
			mb.goalMaxTurns = beh["goal_max_turns"].asInt(16);
			mb.resurrectSearchRadius = beh["resurrect_search_radius"].asInt(0x19000);
			mb.diagonalAttack = beh["diagonal_attack"].asBool(false);
			mb.diagonalAttackField = (int8_t)beh["diagonal_attack_field"].asInt(0);
			mb.orthogonalAttackOnly = beh["orthogonal_attack_only"].asBool(false);
			// Teleport behavior
			DataNode tp = beh["teleport"];
			if (tp) {
				mb.teleport.enabled = true;
				mb.teleport.range = tp["range"].asInt(4);
				mb.teleport.maxAttempts = tp["max_attempts"].asInt(30);
				mb.teleport.cooldownMin = tp["cooldown_min"].asInt(3);
				mb.teleport.cooldownMax = tp["cooldown_max"].asInt(5);
				mb.teleport.particleId = tp["particle_id"].asInt(7);
			}
			DataNode poison = beh["on_hit_poison_params"];
			if (poison) {
				mb.onHitPoisonId = poison["id"].asInt(13);
				mb.onHitPoisonDuration = poison["duration"].asInt(5);
				mb.onHitPoisonPower = poison["power"].asInt(3);
			}
			std::string kbWeapon = beh["knockback_weapon"].asString("none");
			mb.knockbackWeaponId = (kbWeapon == "none") ? -1 : EntityNames::weaponToIndex(kbWeapon);
			std::string ws = beh["walk_sound"].asString("none");
			mb.walkSoundResId = (ws == "none") ? -1 : Sounds::getResIDByName(ws);
			DataNode wmods = beh["weakness_modifiers"];
			if (wmods) {
				for (auto wit = wmods.begin(); wit != wmods.end(); ++wit) {
					std::string wname = wit.key().asString();
					int weaponIdx = EntityNames::weaponToIndex(wname);
					if (weaponIdx >= 0 && weaponIdx < MonsterBehaviors::MAX_WEAKNESS_MODS) {
						std::string val = wit.value().asString("0");
						if (val == "immune") {
							mb.weaknessMods[weaponIdx] = -1;
						} else {
							mb.weaknessMods[weaponIdx] = (int8_t)std::atoi(val.c_str());
						}
					}
				}
			}
			DataNode rds = beh["random_death_sounds"];
			if (rds) {
				int count = std::min((int)rds.size(), 4);
				mb.numRandomDeathSounds = (int8_t)count;
				for (int d = 0; d < count; d++) {
					std::string sname = rds[d].asString("none");
					mb.randomDeathSounds[d] = (sname == "none") ? -1 : (int16_t)Sounds::getResIDByName(sname);
				}
			}
			// Loot drop config (type-level default)
			DataNode loot = beh["loot"];
			if (loot) {
				parseLootConfig(loot, mb.lootConfig);
			}
		}

		// Tiers
		DataNode tiers = m["tiers"];
		if (tiers) {
			int numParms = std::min((int)tiers.size(), numTiers);
			for (int p = 0; p < numParms; p++) {
				DataNode tier = tiers[p];
				int entityIdx = idx * numTiers + p;

				// Stats (6 bytes per entity)
				DataNode stats = tier["stats"];
				if (stats) {
					int statsBase = entityIdx * 6;
					for (int s = 0; s < 6; s++) {
						app->combat->monsterStats[statsBase + s] = (int8_t)stats[statFields[s]].asInt(0);
					}
				}

				// Attacks (3 shorts per tier, 9 per type)
				DataNode atk = tier["attacks"];
				if (atk) {
					int atkBase = idx * 9 + p * 3;
					app->combat->monsterAttacks[atkBase + 0] = (short)EntityNames::weaponToIndex(atk["attack1"].asString("0").c_str());
					app->combat->monsterAttacks[atkBase + 1] = (short)EntityNames::weaponToIndex(atk["attack2"].asString("0").c_str());
					app->combat->monsterAttacks[atkBase + 2] = (short)atk["chance"].asInt(0);
				}

				// Weakness: named weapon map or legacy packed byte array
				DataNode weak = tier["weakness"];
				if (weak) {
					int weakBase = entityIdx * 8;
					if (weak.isMap()) {
						// Named format: {weapon_name: damage_percent, ...}
						// Default all nibbles to 7 (100% = normal damage)
						for (int w = 0; w < 8; w++) {
							app->combat->monsterWeakness[weakBase + w] = 0x77;
						}
						for (auto it = weak.begin(); it != weak.end(); ++it) {
							std::string wname = it.key().asString();
							double pct = (double)it.value().asFloat(100.0f);
							int weaponIdx = EntityNames::weaponToIndex(wname);
							if (weaponIdx >= 0 && weaponIdx < 16) {
								int nibble = std::max(0, std::min(15, (int)(pct / 12.5) - 1));
								int byteIdx = weaponIdx / 2;
								int8_t& b = app->combat->monsterWeakness[weakBase + byteIdx];
								uint8_t ub = (uint8_t)b;
								if (weaponIdx % 2 == 0) {
									ub = (ub & 0xF0) | (nibble & 0x0F);
								} else {
									ub = (ub & 0x0F) | ((nibble & 0x0F) << 4);
								}
								b = (int8_t)ub;
							}
						}
					} else if (weak.isSequence()) {
						// Legacy packed byte array format
						int weakCount = std::min((int)weak.size(), 8);
						for (int w = 0; w < weakCount; w++) {
							app->combat->monsterWeakness[weakBase + w] = (int8_t)weak[w].asInt(0);
						}
					}
				}

				// Per-tier loot override
				DataNode tloot = tier["loot"];
				if (tloot) {
					auto& lt = app->combat->monsterBehaviors[idx].lootTiers[p];
					lt = app->combat->monsterBehaviors[idx].lootConfig;
					parseLootConfig(tloot, lt);
					app->combat->monsterBehaviors[idx].hasLootTiers = true;
				} else if (p < MonsterBehaviors::MAX_LOOT_TIERS) {
					app->combat->monsterBehaviors[idx].lootTiers[p] = app->combat->monsterBehaviors[idx].lootConfig;
				}

				// Per-tier sound overrides (override type-level sounds for this tier)
				DataNode tsnd = tier["sounds"];
				if (tsnd) {
					int sndBase = entityIdx * 8;
					for (int f = 0; f < 8; f++) {
						DataNode sv = tsnd[soundFields[f]];
						if (sv) {
							std::string val = sv.asString("none");
							if (val == "none") {
								app->game->monsterSounds[sndBase + f] = 255;
							} else {
								int soundIdx = Sounds::getIndexByName(val);
								app->game->monsterSounds[sndBase + f] = (soundIdx >= 0) ? (uint8_t)soundIdx : (uint8_t)std::atoi(val.c_str());
							}
						}
					}
				}
			}
		}
	}

	LOG_INFO("[app] loadMonstersFromYAML: loaded {} monsters\n", (int)monsters.size());
	return true;
}
#endif

// ---------------------------------------------------------------------------
// New parser: reads combat data from entities.yaml "combat:" blocks.
// Replaces parseMonsters() — no more tier system, each entity is self-contained.
// ---------------------------------------------------------------------------
std::expected<void, std::string> parseMonsterCombatFromEntities(Applet* app, const DataNode& config) {
	DataNode entities = config["entities"];
	if (!entities || !entities.isMap()) {
		return std::unexpected("entities section missing or not a map for monster combat data");
	}

	int N = app->entityDefManager->getNumMonsterDefs();
	if (N == 0) {
		LOG_INFO("[monster_combat] no monster entities found\n");
		return {};
	}

	// Allocate all arrays indexed by monsterIdx
	app->combat->numMonsterDefs = N;
	app->combat->monsterAttacks = new short[N * 3];
	std::memset(app->combat->monsterAttacks, 0, N * 3 * sizeof(short));

	app->combat->monsterStats = new int8_t[N * 6];
	std::memset(app->combat->monsterStats, 0, N * 6);

	app->combat->monsterWeakness = new int8_t[N * 8];
	std::memset(app->combat->monsterWeakness, 0, N * 8);

	app->game->monsterSounds = new uint8_t[N * 8];
	std::memset(app->game->monsterSounds, 255, N * 8);

	app->particleSystem->monsterColors = new uint8_t[N * 3];
	std::memset(app->particleSystem->monsterColors, 0, N * 3);

	app->combat->monsterDefs = new MonsterDef[N]();
	app->combat->monsterBehaviors = app->combat->monsterDefs;

	static const char* soundFields[] = {"alert1", "alert2", "alert3", "attack1",
	                                    "attack2", "idle", "pain", "death"};
	static const char* statFields[] = {"health", "armor", "defense", "strength",
	                                   "accuracy", "agility"};

	int loaded = 0;
	for (auto eit = entities.begin(); eit != entities.end(); ++eit) {
		DataNode e = eit.value();
		std::string typeStr = e["type"].asString("");
		if (typeStr != "monster") continue;

		// Find the EntityDef to get monsterIdx
		uint8_t eType = (uint8_t)EntityNames::entityTypeFromString(typeStr);
		uint8_t eSubType = (uint8_t)EntityNames::lookupSubtype(eType, e["subtype"].asString("0"));
		uint8_t parm = (uint8_t)EntityNames::lookupParm(eType, eSubType, e["parm"].asString("0"));

		EntityDef* def = app->entityDefManager->find(eType, eSubType, parm);
		if (!def || def->monsterIdx < 0) continue;
		int mi = def->monsterIdx;

		DataNode combat = e["combat"];
		if (!combat) continue;

		// Stats
		DataNode stats = combat["stats"];
		if (stats) {
			int base = mi * 6;
			for (int s = 0; s < 6; s++) {
				app->combat->monsterStats[base + s] = (int8_t)stats[statFields[s]].asInt(0);
			}
		}

		// Attacks
		DataNode atk = combat["attacks"];
		if (atk) {
			int base = mi * 3;
			app->combat->monsterAttacks[base + 0] = (short)EntityNames::weaponToIndex(atk["attack1"].asString("0").c_str());
			app->combat->monsterAttacks[base + 1] = (short)EntityNames::weaponToIndex(atk["attack2"].asString("0").c_str());
			app->combat->monsterAttacks[base + 2] = (short)atk["attack1_chance"].asInt(0);
		}

		// Weakness
		DataNode weak = combat["weakness"];
		if (weak) {
			int weakBase = mi * 8;
			if (weak.isMap()) {
				for (int w = 0; w < 8; w++) {
					app->combat->monsterWeakness[weakBase + w] = 0x77;
				}
				for (auto it = weak.begin(); it != weak.end(); ++it) {
					std::string wname = it.key().asString();
					double pct = (double)it.value().asFloat(100.0f);
					int weaponIdx = EntityNames::weaponToIndex(wname);
					if (weaponIdx >= 0 && weaponIdx < 16) {
						int nibble = std::max(0, std::min(15, (int)(pct / 12.5) - 1));
						int byteIdx = weaponIdx / 2;
						int8_t& b = app->combat->monsterWeakness[weakBase + byteIdx];
						uint8_t ub = (uint8_t)b;
						if (weaponIdx % 2 == 0) {
							ub = (ub & 0xF0) | (nibble & 0x0F);
						} else {
							ub = (ub & 0x0F) | ((nibble & 0x0F) << 4);
						}
						b = (int8_t)ub;
					}
				}
			} else if (weak.isSequence()) {
				int weakCount = std::min((int)weak.size(), 8);
				for (int w = 0; w < weakCount; w++) {
					app->combat->monsterWeakness[weakBase + w] = (int8_t)weak[w].asInt(0);
				}
			}
		}

		// Sounds
		DataNode snd = combat["sounds"];
		if (snd) {
			int sndBase = mi * 8;
			for (int f = 0; f < 8; f++) {
				DataNode sv = snd[soundFields[f]];
				if (sv) {
					std::string val = sv.asString("none");
					if (val == "none") {
						app->game->monsterSounds[sndBase + f] = 255;
					} else {
						int soundIdx = Sounds::getIndexByName(val);
						app->game->monsterSounds[sndBase + f] = (soundIdx >= 0) ? (uint8_t)soundIdx : (uint8_t)std::atoi(val.c_str());
					}
				}
			}
		}

		DataNode blood = combat["blood"];
		if (blood) {
			DataNode bc = blood["color"];
			if (!bc) {
				return std::unexpected("monster '" + eit.key().asString() +
				                       "': blood block requires a 'color' field");
			}
			std::string hex = bc.asString("");
			if (hex.size() == 7 && hex[0] == '#') {
				app->particleSystem->monsterColors[mi * 3 + 0] = (uint8_t)std::stoi(hex.substr(1, 2), nullptr, 16);
				app->particleSystem->monsterColors[mi * 3 + 1] = (uint8_t)std::stoi(hex.substr(3, 2), nullptr, 16);
				app->particleSystem->monsterColors[mi * 3 + 2] = (uint8_t)std::stoi(hex.substr(5, 2), nullptr, 16);
			}
		}

		// XP reward (sibling of behavior:, stats:, attacks:, loot: under combat:)
		// Default 0 means no XP awarded — modders should set explicit values.
		// Boss bonus is baked into the YAML value (no +130 in code).
		app->combat->monsterBehaviors[mi].xp = combat["xp"].asInt(0);

		// Behaviors
		DataNode beh = combat["behavior"];
		if (beh) {
			MonsterBehaviors& mb = app->combat->monsterBehaviors[mi];
			mb.isBoss = beh["is_boss"].asBool(false);
			mb.bossMinTier = beh["boss_min_tier"].asInt(0);
			mb.fearImmune = beh["fear_immune"].asBool(false);
			mb.evading = beh["evading"].asBool(false);
			mb.moveToAttack = beh["move_to_attack"].asBool(false);
			mb.canResurrect = beh["can_resurrect"].asBool(false);
			mb.onHitPoison = beh["on_hit_poison"].asBool(false);
			mb.floats = beh["floats"].asBool(false);
			mb.isVios = beh["is_vios"].asBool(false);
			mb.boneGibs = beh["bone_gibs"].asBool(false);
			mb.smallParm0Scale = beh["small_parm0_scale"].asInt(-1);
			// AI behavior parameters
			mb.pathSearchDepth = beh["path_search_depth"].asInt(8);
			mb.movementTimeMs = beh["movement_time_ms"].asInt(275);
			mb.chaseLosWeight = beh["chase_los_weight"].asInt(-4);
			mb.retreatLosWeight = beh["retreat_los_weight"].asInt(0);
			mb.goalMaxTurns = beh["goal_max_turns"].asInt(16);
			mb.resurrectSearchRadius = beh["resurrect_search_radius"].asInt(0x19000);
			mb.diagonalAttack = beh["diagonal_attack"].asBool(false);
			mb.diagonalAttackField = (int8_t)beh["diagonal_attack_field"].asInt(0);
			mb.orthogonalAttackOnly = beh["orthogonal_attack_only"].asBool(false);
			// Teleport behavior
			DataNode tp = beh["teleport"];
			if (tp) {
				mb.teleport.enabled = true;
				mb.teleport.range = tp["range"].asInt(4);
				mb.teleport.maxAttempts = tp["max_attempts"].asInt(30);
				mb.teleport.cooldownMin = tp["cooldown_min"].asInt(3);
				mb.teleport.cooldownMax = tp["cooldown_max"].asInt(5);
				mb.teleport.particleId = tp["particle_id"].asInt(7);
			}
			DataNode poison = beh["on_hit_poison_params"];
			if (poison) {
				mb.onHitPoisonId = poison["id"].asInt(13);
				mb.onHitPoisonDuration = poison["duration"].asInt(5);
				mb.onHitPoisonPower = poison["power"].asInt(3);
			}
			std::string kbWeapon = beh["knockback_weapon"].asString("none");
			mb.knockbackWeaponId = (kbWeapon == "none") ? -1 : EntityNames::weaponToIndex(kbWeapon);
			std::string ws = beh["walk_sound"].asString("none");
			mb.walkSoundResId = (ws == "none") ? -1 : Sounds::getResIDByName(ws);
			DataNode wmods = beh["weakness_modifiers"];
			if (wmods) {
				for (auto wit = wmods.begin(); wit != wmods.end(); ++wit) {
					std::string wname = wit.key().asString();
					int weaponIdx = EntityNames::weaponToIndex(wname);
					if (weaponIdx >= 0 && weaponIdx < MonsterBehaviors::MAX_WEAKNESS_MODS) {
						std::string val = wit.value().asString("0");
						if (val == "immune") {
							mb.weaknessMods[weaponIdx] = -1;
						} else {
							mb.weaknessMods[weaponIdx] = (int8_t)std::atoi(val.c_str());
						}
					}
				}
			}
			DataNode rds = beh["random_death_sounds"];
			if (rds) {
				int count = std::min((int)rds.size(), 4);
				mb.numRandomDeathSounds = (int8_t)count;
				for (int d = 0; d < count; d++) {
					std::string sname = rds[d].asString("none");
					mb.randomDeathSounds[d] = (sname == "none") ? -1 : (int16_t)Sounds::getResIDByName(sname);
				}
			}
		}

		// Loot (stored per-tier when parm < MAX_LOOT_TIERS).
		// Two schemas: legacy `loot:` map, or `loots:` sequence shorthand
		// where `no_corpse_loot:` lives as a sibling under `combat:`.
		DataNode loots = combat["loots"];
		DataNode loot = loots ? loots : combat["loot"];
		if (loot) {
			MonsterBehaviors& mb = app->combat->monsterBehaviors[mi];
			bool hasNoCorpseSibling = (bool)combat["no_corpse_loot"];
			bool noCorpseSibling = combat["no_corpse_loot"].asBool(false);
			if (parm < MonsterBehaviors::MAX_LOOT_TIERS) {
				mb.lootTiers[parm] = mb.lootConfig;
				parseLootConfig(loot, mb.lootTiers[parm]);
				if (hasNoCorpseSibling) mb.lootTiers[parm].noCorpseLoot = noCorpseSibling;
				mb.hasLootTiers = true;
			}
			if (parm == 0) {
				parseLootConfig(loot, mb.lootConfig);
				if (hasNoCorpseSibling) mb.lootConfig.noCorpseLoot = noCorpseSibling;
			}
		}

		loaded++;
	}

	// Build global trinket string table from per-monster trinketText fields
	{
		auto& trinketStrings = CAppContainer::getInstance()->gameConfig.trinketStrings;
		std::unordered_map<std::string, int16_t> trinketTextToIdx;
		auto registerTrinket = [&](MonsterDef::LootConfig& lc) {
			for (auto& e : lc.drops) {
				if (e.trinketText.empty()) continue;
				auto [it, inserted] = trinketTextToIdx.try_emplace(e.trinketText, (int16_t)trinketStrings.size());
				if (inserted) {
					trinketStrings.push_back(e.trinketText);
				}
				e.trinketStringIdx = it->second;
			}
		};
		for (int i = 0; i < N; i++) {
			auto& mb = app->combat->monsterBehaviors[i];
			registerTrinket(mb.lootConfig);
			if (mb.hasLootTiers) {
				for (int t = 0; t < MonsterBehaviors::MAX_LOOT_TIERS; t++) {
					registerTrinket(mb.lootTiers[t]);
				}
			}
		}
		LOG_INFO("[monster_combat] registered {} unique trinket strings\n", (int)trinketStrings.size());
	}

	LOG_INFO("[monster_combat] loaded combat data for {}/{} monster entities\n", loaded, N);
	return {};
}
