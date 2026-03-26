#include <stdexcept>
#include <cstdio>
#include <string>
#include <vector>

#include "CAppContainer.h"
#include "App.h"
#include "EntityDef.h"
#include "EntityNames.h"
#include "Combat.h"
#include "JavaStream.h"
#include "Resource.h"
#include <yaml-cpp/yaml.h>

// -----------------------
// EntityDefManager Class
// -----------------------

EntityDefManager::EntityDefManager() {
	std::memset(this, 0, sizeof(EntityDefManager));
}

EntityDefManager::~EntityDefManager() {}

bool EntityDefManager::startup() {
	printf("[entitydef] startup\n");

	this->numDefs = 0;

	printf("[entitydef] loading from entities.yaml\n");
	return this->loadFromYAML("entities.yaml");
}

bool EntityDefManager::loadFromYAML(const char* path) {
	Applet* app = CAppContainer::getInstance()->app;

	YAML::Node config;
	try {
		config = YAML::LoadFile(path);
	} catch (const YAML::Exception& e) {
		app->Error("Failed to load %s: %s\n", path, e.what());
		return false;
	}

	YAML::Node entities = config["entities"];
	if (!entities || !entities.IsMap() || entities.size() == 0) {
		app->Error("entities.yaml: missing or empty 'entities' map\n");
		return false;
	}

	int count = (int)entities.size();
	this->numDefs = count;
	this->list = new EntityDef[count];

	int i = 0;
	for (auto eit = entities.begin(); eit != entities.end(); ++eit, ++i) {
		YAML::Node e = eit->second;

		this->list[i].tileIndex = (int16_t)e["tile_index"].as<int>(0);
		this->list[i].eType = (uint8_t)EntityNames::entityTypeFromString(e["type"].as<std::string>("world"));
		this->list[i].eSubType = (uint8_t)EntityNames::lookupSubtype(this->list[i].eType, e["subtype"].as<std::string>("0"));
		this->list[i].parm = (uint8_t)EntityNames::lookupParm(this->list[i].eType, this->list[i].eSubType, e["parm"].as<std::string>("0"));

		// Text resource IDs: support both nested text: group and flat fields
		if (YAML::Node text = e["text"]) {
			this->list[i].name = (int16_t)text["name"].as<int>(0);
			this->list[i].longName = (int16_t)text["long_name"].as<int>(0);
			this->list[i].description = (int16_t)text["description"].as<int>(0);
		} else {
			this->list[i].name = (int16_t)e["name"].as<int>(0);
			this->list[i].longName = (int16_t)e["long_name"].as<int>(0);
			this->list[i].description = (int16_t)e["description"].as<int>(0);
		}

		// Rendering data: all values come from YAML, zero-initialized defaults
		YAML::Node r = e["rendering"];

		// Render flags from rendering.flags array
		this->list[i].renderFlags = EntityDef::RFLAG_NONE;
		if (r && r["flags"]) {
			for (int fi = 0; fi < (int)r["flags"].size(); fi++) {
				std::string flag = r["flags"][fi].as<std::string>("");
				if (flag == "floater") this->list[i].renderFlags |= EntityDef::RFLAG_FLOATER;
				else if (flag == "gun_flare") this->list[i].renderFlags |= EntityDef::RFLAG_GUN_FLARE;
				else if (flag == "special_boss") this->list[i].renderFlags |= EntityDef::RFLAG_SPECIAL_BOSS;
				else if (flag == "npc") this->list[i].renderFlags |= EntityDef::RFLAG_NPC;
				else if (flag == "imp_type") this->list[i].renderFlags |= EntityDef::RFLAG_IMP_TYPE;
				else if (flag == "revenant_type") this->list[i].renderFlags |= EntityDef::RFLAG_REVENANT_TYPE;
			}
		}

		// Helper: read from rendering.section.key, then flat key, then default
		#define R_INT(section, key, flat, def) \
			((r && r[section] && r[section][key]) ? r[section][key].as<int>(def) : e[flat].as<int>(def))
		#define R_BOOL(section, key, flat, def) \
			((r && r[section] && r[section][key]) ? r[section][key].as<bool>(def) : e[flat].as<bool>(def))

		// Fear eye offsets
		EntityDef::FearEyeData defEyes;
		this->list[i].fearEyes.eyeL = (int8_t)R_INT("fear_eyes", "eye_l", "fear_eye_l", defEyes.eyeL);
		this->list[i].fearEyes.eyeR = (int8_t)R_INT("fear_eyes", "eye_r", "fear_eye_r", defEyes.eyeR);
		this->list[i].fearEyes.zAdd = (int16_t)R_INT("fear_eyes", "z_add", "fear_eye_z_add", defEyes.zAdd);
		this->list[i].fearEyes.zAlwaysPre = (int16_t)R_INT("fear_eyes", "z_always_pre", "fear_eye_z_always", defEyes.zAlwaysPre);
		this->list[i].fearEyes.zIdlePre = (int16_t)R_INT("fear_eyes", "z_idle_pre", "fear_eye_z_idle", defEyes.zIdlePre);
		this->list[i].fearEyes.singleEye = R_BOOL("fear_eyes", "single_eye", "fear_single_eye", defEyes.singleEye);
		this->list[i].fearEyes.eyeLFlip = (int8_t)R_INT("fear_eyes", "eye_l_flip", "fear_eye_l_flip", defEyes.eyeLFlip);
		this->list[i].fearEyes.eyeRFlip = (int8_t)R_INT("fear_eyes", "eye_r_flip", "fear_eye_r_flip", defEyes.eyeRFlip);

		// Gun flare offsets
		EntityDef::GunFlareData defFlare;
		this->list[i].gunFlare.dualFlare = R_BOOL("gun_flare", "dual", "gun_flare_dual", defFlare.dualFlare);
		this->list[i].gunFlare.flash1X = (int8_t)R_INT("gun_flare", "flash1_x", "gun_flare1_x", defFlare.flash1X);
		this->list[i].gunFlare.flash1Z = (int16_t)R_INT("gun_flare", "flash1_z", "gun_flare1_z", defFlare.flash1Z);
		this->list[i].gunFlare.flash2X = (int8_t)R_INT("gun_flare", "flash2_x", "gun_flare2_x", defFlare.flash2X);
		this->list[i].gunFlare.flash2Z = (int16_t)R_INT("gun_flare", "flash2_z", "gun_flare2_z", defFlare.flash2Z);

		// Body part offsets
		EntityDef::BodyPartData defBody;
		this->list[i].bodyParts.idleTorsoZ = (int16_t)R_INT("body_parts", "idle_torso_z", "idle_torso_z", defBody.idleTorsoZ);
		this->list[i].bodyParts.idleHeadZ = (int16_t)R_INT("body_parts", "idle_head_z", "idle_head_z", defBody.idleHeadZ);
		this->list[i].bodyParts.walkTorsoZ = (int16_t)R_INT("body_parts", "walk_torso_z", "walk_torso_z", defBody.walkTorsoZ);
		this->list[i].bodyParts.walkHeadZ = (int16_t)R_INT("body_parts", "walk_head_z", "walk_head_z", defBody.walkHeadZ);
		this->list[i].bodyParts.attackTorsoZF0 = (int16_t)R_INT("body_parts", "attack_torso_z_f0", "attack_torso_z_f0", defBody.attackTorsoZF0);
		this->list[i].bodyParts.attackTorsoZF1 = (int16_t)R_INT("body_parts", "attack_torso_z_f1", "attack_torso_z_f1", defBody.attackTorsoZF1);
		this->list[i].bodyParts.attackHeadZ = (int16_t)R_INT("body_parts", "attack_head_z", "attack_head_z", defBody.attackHeadZ);
		this->list[i].bodyParts.attackHeadX = (int8_t)R_INT("body_parts", "attack_head_x", "attack_head_x", defBody.attackHeadX);
		this->list[i].bodyParts.noHeadOnBack = R_BOOL("body_parts", "no_head_on_back", "no_head_on_back", defBody.noHeadOnBack);
		this->list[i].bodyParts.sentryHeadFlip = R_BOOL("body_parts", "sentry_head_flip", "sentry_head_flip", defBody.sentryHeadFlip);
		this->list[i].bodyParts.flipTorsoWalk = R_BOOL("body_parts", "flip_torso_walk", "flip_torso_walk", defBody.flipTorsoWalk);
		this->list[i].bodyParts.noHeadOnAttack = R_BOOL("body_parts", "no_head_on_attack", "no_head_on_attack", defBody.noHeadOnAttack);
		this->list[i].bodyParts.noHeadOnMancAtk = R_BOOL("body_parts", "no_head_on_manc_atk", "no_head_on_manc_atk", defBody.noHeadOnMancAtk);
		this->list[i].bodyParts.attackRevAtk2TorsoZ = (int16_t)R_INT("body_parts", "attack_rev_atk2_torso_z", "attack_rev_atk2_torso_z", defBody.attackRevAtk2TorsoZ);

		// Floater rendering data
		EntityDef::FloaterData defFloat;
		this->list[i].floater.type = (EntityDef::FloaterData::Type)R_INT("floater", "type", "floater_type", (int)defFloat.type);
		this->list[i].floater.zOffset = (int16_t)R_INT("floater", "z_offset", "floater_z_offset", defFloat.zOffset);
		this->list[i].floater.hasIdleFrameIncrement = R_BOOL("floater", "has_idle_frame_increment", "floater_idle_frame_inc", defFloat.hasIdleFrameIncrement);
		this->list[i].floater.hasBackExtraSprite = R_BOOL("floater", "has_back_extra_sprite", "floater_back_extra_sprite", defFloat.hasBackExtraSprite);
		this->list[i].floater.backExtraSpriteIdx = (int8_t)R_INT("floater", "back_extra_sprite_idx", "floater_back_extra_idx", defFloat.backExtraSpriteIdx);
		this->list[i].floater.backViewFrame = (int8_t)R_INT("floater", "back_view_frame", "floater_back_view_frame", defFloat.backViewFrame);
		this->list[i].floater.idleFrontFrame = (int8_t)R_INT("floater", "idle_front_frame", "floater_idle_front_frame", defFloat.idleFrontFrame);
		this->list[i].floater.headZOffset = (int16_t)R_INT("floater", "head_z_offset", "floater_head_z", defFloat.headZOffset);
		this->list[i].floater.hasDeadLoot = R_BOOL("floater", "has_dead_loot", "floater_dead_loot", defFloat.hasDeadLoot);

		// Special boss rendering data
		EntityDef::SpecialBossData defBoss;
		// Parse type as string or int
		{
			int bossType = (int)defBoss.type;
			if (r && r["special_boss"] && r["special_boss"]["type"]) {
				std::string ts = r["special_boss"]["type"].as<std::string>("");
				if (ts == "multipart") bossType = 0;
				else if (ts == "ethereal") bossType = 1;
				else if (ts == "spider") bossType = 2;
				else bossType = r["special_boss"]["type"].as<int>(bossType);
			} else {
				bossType = e["boss_type"].as<int>(bossType);
			}
			this->list[i].specialBoss.type = (EntityDef::SpecialBossData::Type)bossType;
		}
		this->list[i].specialBoss.torsoZ = (int16_t)R_INT("special_boss", "torso_z", "boss_torso_z", defBoss.torsoZ);
		this->list[i].specialBoss.idleSpriteIdx = (int8_t)R_INT("special_boss", "idle_sprite_idx", "boss_idle_sprite", defBoss.idleSpriteIdx);
		this->list[i].specialBoss.attackSpriteIdx = (int8_t)R_INT("special_boss", "attack_sprite_idx", "boss_attack_sprite", defBoss.attackSpriteIdx);
		this->list[i].specialBoss.painSpriteIdx = (int8_t)R_INT("special_boss", "pain_sprite_idx", "boss_pain_sprite", defBoss.painSpriteIdx);
		this->list[i].specialBoss.renderModeOverride = (int8_t)R_INT("special_boss", "render_mode_override", "boss_render_mode", defBoss.renderModeOverride);
		this->list[i].specialBoss.painRenderMode = (int8_t)R_INT("special_boss", "pain_render_mode", "boss_pain_render_mode", defBoss.painRenderMode);
		this->list[i].specialBoss.legLateral = (int16_t)R_INT("special_boss", "leg_lateral", "boss_leg_lateral", defBoss.legLateral);
		this->list[i].specialBoss.legBaseZ = (int16_t)R_INT("special_boss", "leg_base_z", "boss_leg_base_z", defBoss.legBaseZ);
		this->list[i].specialBoss.idleTorsoZ = (int16_t)R_INT("special_boss", "idle_torso_z", "boss_idle_torso_z", defBoss.idleTorsoZ);
		this->list[i].specialBoss.idleBobDiv = (int8_t)R_INT("special_boss", "idle_bob_div", "boss_idle_bob_div", defBoss.idleBobDiv);
		this->list[i].specialBoss.idleTorsoZBase = (int16_t)R_INT("special_boss", "idle_torso_z_base", "boss_idle_torso_z_base", defBoss.idleTorsoZBase);
		this->list[i].specialBoss.walkTorsoZ = (int16_t)R_INT("special_boss", "walk_torso_z", "boss_walk_torso_z", defBoss.walkTorsoZ);
		this->list[i].specialBoss.attackTorsoZ = (int16_t)R_INT("special_boss", "attack_torso_z", "boss_attack_torso_z", defBoss.attackTorsoZ);
		this->list[i].specialBoss.painTorsoZ = (int16_t)R_INT("special_boss", "pain_torso_z", "boss_pain_torso_z", defBoss.painTorsoZ);
		this->list[i].specialBoss.painLegsZ = (int16_t)R_INT("special_boss", "pain_legs_z", "boss_pain_legs_z", defBoss.painLegsZ);
		this->list[i].specialBoss.painLegPos = (int8_t)R_INT("special_boss", "pain_leg_pos", "boss_pain_leg_pos", defBoss.painLegPos);
		this->list[i].specialBoss.hasAttackFlare = R_BOOL("special_boss", "has_attack_flare", "boss_attack_flare", defBoss.hasAttackFlare);
		this->list[i].specialBoss.flareZOffset = (int16_t)R_INT("special_boss", "flare_z_offset", "boss_flare_z", defBoss.flareZOffset);
		this->list[i].specialBoss.flareLateralPos = (int8_t)R_INT("special_boss", "flare_lateral_pos", "boss_flare_lateral", defBoss.flareLateralPos);
		this->list[i].specialBoss.flareTorsoZExtra = (int16_t)R_INT("special_boss", "flare_torso_z_extra", "boss_flare_torso_z_extra", defBoss.flareTorsoZExtra);

		// Sprite anim overrides
		EntityDef::SpriteAnimData defAnim;
		this->list[i].spriteAnim.clampScale = R_BOOL("sprite_anim", "clamp_scale", "anim_clamp_scale", defAnim.clampScale);
		this->list[i].spriteAnim.attackLegOffset = (int8_t)R_INT("sprite_anim", "attack_leg_offset", "anim_attack_leg_offset", defAnim.attackLegOffset);
		this->list[i].spriteAnim.attackLegOffsetOnFlip = R_BOOL("sprite_anim", "attack_leg_offset_on_flip", "anim_attack_leg_on_flip", defAnim.attackLegOffsetOnFlip);
		this->list[i].spriteAnim.hasFrameDependentHead = R_BOOL("sprite_anim", "has_frame_dependent_head", "anim_frame_dep_head", defAnim.hasFrameDependentHead);
		this->list[i].spriteAnim.headZFrame0 = (int16_t)R_INT("sprite_anim", "head_z_frame0", "anim_head_z_f0", defAnim.headZFrame0);
		this->list[i].spriteAnim.headXFrame0 = (int8_t)R_INT("sprite_anim", "head_x_frame0", "anim_head_x_f0", defAnim.headXFrame0);
		this->list[i].spriteAnim.headZFrame1 = (int16_t)R_INT("sprite_anim", "head_z_frame1", "anim_head_z_f1", defAnim.headZFrame1);

		#undef R_INT
		#undef R_BOOL
	}

	printf("[entitydef] loaded %d entities from %s\n", this->numDefs, path);

	return true;
}

EntityDef* EntityDefManager::find(int eType, int eSubType) {
	return this->find(eType, eSubType, -1);
}

EntityDef* EntityDefManager::find(int eType, int eSubType, int parm) {
	for (int i = 0; i < this->numDefs; i++) {
		if (this->list[i].eType == eType && this->list[i].eSubType == eSubType &&
		    (parm == -1 || this->list[i].parm == parm)) {
			return &this->list[i];
		}
	}
	return nullptr;
}

EntityDef* EntityDefManager::lookup(int tileIndex) {
	for (int i = 0; i < this->numDefs; ++i) {
		if (this->list[i].tileIndex == tileIndex) {
			return &this->list[i];
		}
	}
	return nullptr;
}

// ----------------
// EntityDef Class
// ----------------

EntityDef::EntityDef() {
	std::memset(this, 0, sizeof(EntityDef));
}
