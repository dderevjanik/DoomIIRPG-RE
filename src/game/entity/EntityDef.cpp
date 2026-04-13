#include <stdexcept>
#include <string>
#include <vector>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "DataNode.h"
#include "EntityDef.h"
#include "EntityNames.h"
#include "Enums.h"
#include "SpriteDefs.h"
#include "Combat.h"
#include "JavaStream.h"
#include "Resource.h"

// -----------------------
// EntityDefManager Class
// -----------------------

// EntityDefManager default constructor is = default (header)

EntityDefManager::~EntityDefManager() {}

bool EntityDefManager::startup() {
	LOG_INFO("[entitydef] startup\n");
	// Entity definitions are now loaded via ResourceManager::loadAllDefinitions()
	// startup() only needs to verify they were loaded
	return (this->numDefs > 0);
}

std::expected<void, std::string> EntityDefManager::parse(EntityDefManager* mgr, const DataNode& config) {
	EntityDef*& list = mgr->list;
	int& numDefs = mgr->numDefs;
	Applet* app = CAppContainer::getInstance()->app;

	DataNode entities = config["entities"];
	if (!entities || !entities.isMap() || entities.size() == 0) {
		return std::unexpected("entities.yaml: missing or empty 'entities' map");
	}

	int count = (int)entities.size();
	numDefs = count;
	list = new EntityDef[count];

	int monsterCount = 0;
	int i = 0;
	for (auto eit = entities.begin(); eit != entities.end(); ++eit, ++i) {
		DataNode e = eit.value();

		// Sprite reference: accepts name from sprites.yaml or numeric tile index
		std::string spriteRef = e["sprite"].asString("");
		if (spriteRef.empty()) spriteRef = e["tile_index"].asString("0"); // backwards compat
		if (!spriteRef.empty() && spriteRef != "0") {
			int idx = SpriteDefs::getIndex(spriteRef);
			if (idx != 0) {
				list[i].tileIndex = (int16_t)idx;
			} else {
				try { list[i].tileIndex = (int16_t)std::stoi(spriteRef); } catch (...) { list[i].tileIndex = 0; }
			}
		} else {
			list[i].tileIndex = 0;
		}
		list[i].eType = (uint8_t)EntityNames::entityTypeFromString(e["type"].asString("world"));
		list[i].eSubType = (uint8_t)EntityNames::lookupSubtype(list[i].eType, e["subtype"].asString("0"));
		list[i].parm = (uint8_t)EntityNames::lookupParm(list[i].eType, list[i].eSubType, e["parm"].asString("0"));

		// Assign sequential monster index
		if (list[i].eType == Enums::ET_MONSTER) {
			list[i].monsterIdx = monsterCount++;
		}

		// Text resource IDs: support both nested text: group and flat fields
		DataNode text = e["text"];
		if (text) {
			list[i].name = (int16_t)text["name"].asInt(0);
			list[i].longName = (int16_t)text["long_name"].asInt(0);
			list[i].description = (int16_t)text["description"].asInt(0);
		} else {
			list[i].name = (int16_t)e["name"].asInt(0);
			list[i].longName = (int16_t)e["long_name"].asInt(0);
			list[i].description = (int16_t)e["description"].asInt(0);
		}

		// Rendering data: all values come from YAML, zero-initialized defaults
		DataNode r = e["rendering"];

		// Render flags from rendering.flags array
		list[i].renderFlags = EntityDef::RFLAG_NONE;
		if (r && r["flags"]) {
			for (int fi = 0; fi < (int)r["flags"].size(); fi++) {
				std::string flag = r["flags"][fi].asString("");
				if (flag == "floater") list[i].renderFlags |= EntityDef::RFLAG_FLOATER;
				else if (flag == "gun_flare") list[i].renderFlags |= EntityDef::RFLAG_GUN_FLARE;
				else if (flag == "special_boss") list[i].renderFlags |= EntityDef::RFLAG_SPECIAL_BOSS;
				else if (flag == "npc") list[i].renderFlags |= EntityDef::RFLAG_NPC;
				else if (flag == "no_flare_alt_attack") list[i].renderFlags |= EntityDef::RFLAG_NO_FLARE_ALT_ATTACK;
				else if (flag == "tall_hitbox") list[i].renderFlags |= EntityDef::RFLAG_TALL_HITBOX;
			}
		}

		// Helper: read from rendering.section.key, then flat key, then default
		#define R_INT(section, key, flat, def) \
			((r && r[section] && r[section][key]) ? r[section][key].asInt(def) : e[flat].asInt(def))
		#define R_BOOL(section, key, flat, def) \
			((r && r[section] && r[section][key]) ? r[section][key].asBool(def) : e[flat].asBool(def))

		// Fear eye offsets
		EntityDef::FearEyeData defEyes;
		list[i].fearEyes.eyeL = (int8_t)R_INT("fear_eyes", "eye_l", "fear_eye_l", defEyes.eyeL);
		list[i].fearEyes.eyeR = (int8_t)R_INT("fear_eyes", "eye_r", "fear_eye_r", defEyes.eyeR);
		list[i].fearEyes.zAdd = (int16_t)R_INT("fear_eyes", "z_add", "fear_eye_z_add", defEyes.zAdd);
		list[i].fearEyes.zAlwaysPre = (int16_t)R_INT("fear_eyes", "z_always_pre", "fear_eye_z_always", defEyes.zAlwaysPre);
		list[i].fearEyes.zIdlePre = (int16_t)R_INT("fear_eyes", "z_idle_pre", "fear_eye_z_idle", defEyes.zIdlePre);
		list[i].fearEyes.singleEye = R_BOOL("fear_eyes", "single_eye", "fear_single_eye", defEyes.singleEye);
		list[i].fearEyes.eyeLFlip = (int8_t)R_INT("fear_eyes", "eye_l_flip", "fear_eye_l_flip", defEyes.eyeLFlip);
		list[i].fearEyes.eyeRFlip = (int8_t)R_INT("fear_eyes", "eye_r_flip", "fear_eye_r_flip", defEyes.eyeRFlip);

		// Gun flare offsets
		EntityDef::GunFlareData defFlare;
		list[i].gunFlare.dualFlare = R_BOOL("gun_flare", "dual", "gun_flare_dual", defFlare.dualFlare);
		list[i].gunFlare.flash1X = (int8_t)R_INT("gun_flare", "flash1_x", "gun_flare1_x", defFlare.flash1X);
		list[i].gunFlare.flash1Z = (int16_t)R_INT("gun_flare", "flash1_z", "gun_flare1_z", defFlare.flash1Z);
		list[i].gunFlare.flash2X = (int8_t)R_INT("gun_flare", "flash2_x", "gun_flare2_x", defFlare.flash2X);
		list[i].gunFlare.flash2Z = (int16_t)R_INT("gun_flare", "flash2_z", "gun_flare2_z", defFlare.flash2Z);

		// Body part offsets
		EntityDef::BodyPartData defBody;
		list[i].bodyParts.idleTorsoZ = (int16_t)R_INT("body_parts", "idle_torso_z", "idle_torso_z", defBody.idleTorsoZ);
		list[i].bodyParts.idleHeadZ = (int16_t)R_INT("body_parts", "idle_head_z", "idle_head_z", defBody.idleHeadZ);
		list[i].bodyParts.walkTorsoZ = (int16_t)R_INT("body_parts", "walk_torso_z", "walk_torso_z", defBody.walkTorsoZ);
		list[i].bodyParts.walkHeadZ = (int16_t)R_INT("body_parts", "walk_head_z", "walk_head_z", defBody.walkHeadZ);
		list[i].bodyParts.attackTorsoZF0 = (int16_t)R_INT("body_parts", "attack_torso_z_f0", "attack_torso_z_f0", defBody.attackTorsoZF0);
		list[i].bodyParts.attackTorsoZF1 = (int16_t)R_INT("body_parts", "attack_torso_z_f1", "attack_torso_z_f1", defBody.attackTorsoZF1);
		list[i].bodyParts.attackHeadZ = (int16_t)R_INT("body_parts", "attack_head_z", "attack_head_z", defBody.attackHeadZ);
		list[i].bodyParts.attackHeadX = (int8_t)R_INT("body_parts", "attack_head_x", "attack_head_x", defBody.attackHeadX);
		list[i].bodyParts.noHeadOnBack = R_BOOL("body_parts", "no_head_on_back", "no_head_on_back", defBody.noHeadOnBack);
		list[i].bodyParts.sentryHeadFlip = R_BOOL("body_parts", "sentry_head_flip", "sentry_head_flip", defBody.sentryHeadFlip);
		list[i].bodyParts.flipTorsoWalk = R_BOOL("body_parts", "flip_torso_walk", "flip_torso_walk", defBody.flipTorsoWalk);
		list[i].bodyParts.noHeadOnAttack = R_BOOL("body_parts", "no_head_on_attack", "no_head_on_attack", defBody.noHeadOnAttack);
		list[i].bodyParts.noHeadOnMancAtk = R_BOOL("body_parts", "no_head_on_manc_atk", "no_head_on_manc_atk", defBody.noHeadOnMancAtk);
		list[i].bodyParts.attackRevAtk2TorsoZ = (int16_t)R_INT("body_parts", "attack_rev_atk2_torso_z", "attack_rev_atk2_torso_z", defBody.attackRevAtk2TorsoZ);
		// Sub-sprite frame indices
		list[i].bodyParts.legsFrame = (int8_t)R_INT("body_parts", "legs_frame", "legs_frame", defBody.legsFrame);
		list[i].bodyParts.torsoFrame = (int8_t)R_INT("body_parts", "torso_frame", "torso_frame", defBody.torsoFrame);
		list[i].bodyParts.headFrame = (int8_t)R_INT("body_parts", "head_frame", "head_frame", defBody.headFrame);
		list[i].bodyParts.backViewOffset = (int8_t)R_INT("body_parts", "back_view_offset", "back_view_offset", defBody.backViewOffset);
		list[i].bodyParts.attack1Frame = (int8_t)R_INT("body_parts", "attack1_frame", "attack1_frame", defBody.attack1Frame);
		list[i].bodyParts.attack2Frame = (int8_t)R_INT("body_parts", "attack2_frame", "attack2_frame", defBody.attack2Frame);
		list[i].bodyParts.painFrame = (int8_t)R_INT("body_parts", "pain_frame", "pain_frame", defBody.painFrame);
		list[i].bodyParts.deadFrame = (int8_t)R_INT("body_parts", "dead_frame", "dead_frame", defBody.deadFrame);
		list[i].bodyParts.slapTorsoFrame = (int8_t)R_INT("body_parts", "slap_torso_frame", "slap_torso_frame", defBody.slapTorsoFrame);
		list[i].bodyParts.slapHeadFrame = (int8_t)R_INT("body_parts", "slap_head_frame", "slap_head_frame", defBody.slapHeadFrame);

		// Floater rendering data
		EntityDef::FloaterData defFloat;
		list[i].floater.type = (EntityDef::FloaterData::Type)R_INT("floater", "type", "floater_type", (int)defFloat.type);
		list[i].floater.zOffset = (int16_t)R_INT("floater", "z_offset", "floater_z_offset", defFloat.zOffset);
		list[i].floater.hasIdleFrameIncrement = R_BOOL("floater", "has_idle_frame_increment", "floater_idle_frame_inc", defFloat.hasIdleFrameIncrement);
		list[i].floater.hasBackExtraSprite = R_BOOL("floater", "has_back_extra_sprite", "floater_back_extra_sprite", defFloat.hasBackExtraSprite);
		list[i].floater.backExtraSpriteIdx = (int8_t)R_INT("floater", "back_extra_sprite_idx", "floater_back_extra_idx", defFloat.backExtraSpriteIdx);
		list[i].floater.backViewFrame = (int8_t)R_INT("floater", "back_view_frame", "floater_back_view_frame", defFloat.backViewFrame);
		list[i].floater.idleFrontFrame = (int8_t)R_INT("floater", "idle_front_frame", "floater_idle_front_frame", defFloat.idleFrontFrame);
		list[i].floater.headZOffset = (int16_t)R_INT("floater", "head_z_offset", "floater_head_z", defFloat.headZOffset);
		list[i].floater.hasDeadLoot = R_BOOL("floater", "has_dead_loot", "floater_dead_loot", defFloat.hasDeadLoot);
		// Floater sub-sprite frame indices
		list[i].floater.attackFrame = (int8_t)R_INT("floater", "attack_frame", "floater_attack_frame", defFloat.attackFrame);
		list[i].floater.attack2Offset = (int8_t)R_INT("floater", "attack2_offset", "floater_attack2_offset", defFloat.attack2Offset);
		list[i].floater.painFrame = (int8_t)R_INT("floater", "pain_frame", "floater_pain_frame", defFloat.painFrame);
		list[i].floater.deadFrame = (int8_t)R_INT("floater", "dead_frame", "floater_dead_frame", defFloat.deadFrame);

		// Special boss rendering data
		EntityDef::SpecialBossData defBoss;
		// Parse type as string or int
		{
			int bossType = (int)defBoss.type;
			if (r && r["special_boss"] && r["special_boss"]["type"]) {
				std::string ts = r["special_boss"]["type"].asString("");
				if (ts == "multipart") bossType = 0;
				else if (ts == "ethereal") bossType = 1;
				else if (ts == "spider") bossType = 2;
				else bossType = r["special_boss"]["type"].asInt(bossType);
			} else {
				bossType = e["boss_type"].asInt(bossType);
			}
			list[i].specialBoss.type = (EntityDef::SpecialBossData::Type)bossType;
		}
		list[i].specialBoss.torsoZ = (int16_t)R_INT("special_boss", "torso_z", "boss_torso_z", defBoss.torsoZ);
		list[i].specialBoss.idleSpriteIdx = (int8_t)R_INT("special_boss", "idle_sprite_idx", "boss_idle_sprite", defBoss.idleSpriteIdx);
		list[i].specialBoss.attackSpriteIdx = (int8_t)R_INT("special_boss", "attack_sprite_idx", "boss_attack_sprite", defBoss.attackSpriteIdx);
		list[i].specialBoss.painSpriteIdx = (int8_t)R_INT("special_boss", "pain_sprite_idx", "boss_pain_sprite", defBoss.painSpriteIdx);
		list[i].specialBoss.renderModeOverride = (int8_t)R_INT("special_boss", "render_mode_override", "boss_render_mode", defBoss.renderModeOverride);
		list[i].specialBoss.painRenderMode = (int8_t)R_INT("special_boss", "pain_render_mode", "boss_pain_render_mode", defBoss.painRenderMode);
		list[i].specialBoss.legLateral = (int16_t)R_INT("special_boss", "leg_lateral", "boss_leg_lateral", defBoss.legLateral);
		list[i].specialBoss.legBaseZ = (int16_t)R_INT("special_boss", "leg_base_z", "boss_leg_base_z", defBoss.legBaseZ);
		list[i].specialBoss.idleTorsoZ = (int16_t)R_INT("special_boss", "idle_torso_z", "boss_idle_torso_z", defBoss.idleTorsoZ);
		list[i].specialBoss.idleBobDiv = (int8_t)R_INT("special_boss", "idle_bob_div", "boss_idle_bob_div", defBoss.idleBobDiv);
		list[i].specialBoss.idleTorsoZBase = (int16_t)R_INT("special_boss", "idle_torso_z_base", "boss_idle_torso_z_base", defBoss.idleTorsoZBase);
		list[i].specialBoss.walkTorsoZ = (int16_t)R_INT("special_boss", "walk_torso_z", "boss_walk_torso_z", defBoss.walkTorsoZ);
		list[i].specialBoss.attackTorsoZ = (int16_t)R_INT("special_boss", "attack_torso_z", "boss_attack_torso_z", defBoss.attackTorsoZ);
		list[i].specialBoss.painTorsoZ = (int16_t)R_INT("special_boss", "pain_torso_z", "boss_pain_torso_z", defBoss.painTorsoZ);
		list[i].specialBoss.painLegsZ = (int16_t)R_INT("special_boss", "pain_legs_z", "boss_pain_legs_z", defBoss.painLegsZ);
		list[i].specialBoss.painLegPos = (int8_t)R_INT("special_boss", "pain_leg_pos", "boss_pain_leg_pos", defBoss.painLegPos);
		list[i].specialBoss.hasAttackFlare = R_BOOL("special_boss", "has_attack_flare", "boss_attack_flare", defBoss.hasAttackFlare);
		list[i].specialBoss.flareZOffset = (int16_t)R_INT("special_boss", "flare_z_offset", "boss_flare_z", defBoss.flareZOffset);
		list[i].specialBoss.flareLateralPos = (int8_t)R_INT("special_boss", "flare_lateral_pos", "boss_flare_lateral", defBoss.flareLateralPos);
		list[i].specialBoss.flareTorsoZExtra = (int16_t)R_INT("special_boss", "flare_torso_z_extra", "boss_flare_torso_z_extra", defBoss.flareTorsoZExtra);
		list[i].specialBoss.deadFrame = (int8_t)R_INT("special_boss", "dead_frame", "boss_dead_frame", defBoss.deadFrame);

		// Sprite anim overrides
		EntityDef::SpriteAnimData defAnim;
		list[i].spriteAnim.clampScale = R_BOOL("sprite_anim", "clamp_scale", "anim_clamp_scale", defAnim.clampScale);
		list[i].spriteAnim.attackLegOffset = (int8_t)R_INT("sprite_anim", "attack_leg_offset", "anim_attack_leg_offset", defAnim.attackLegOffset);
		list[i].spriteAnim.attackLegOffsetOnFlip = R_BOOL("sprite_anim", "attack_leg_offset_on_flip", "anim_attack_leg_on_flip", defAnim.attackLegOffsetOnFlip);
		list[i].spriteAnim.hasFrameDependentHead = R_BOOL("sprite_anim", "has_frame_dependent_head", "anim_frame_dep_head", defAnim.hasFrameDependentHead);
		list[i].spriteAnim.headZFrame0 = (int16_t)R_INT("sprite_anim", "head_z_frame0", "anim_head_z_f0", defAnim.headZFrame0);
		list[i].spriteAnim.headXFrame0 = (int8_t)R_INT("sprite_anim", "head_x_frame0", "anim_head_x_f0", defAnim.headXFrame0);
		list[i].spriteAnim.headZFrame1 = (int16_t)R_INT("sprite_anim", "head_z_frame1", "anim_head_z_f1", defAnim.headZFrame1);

		#undef R_INT
		#undef R_BOOL
	}

	mgr->numMonsterDefs = monsterCount;
	LOG_INFO("[entitydef] loaded {} entities ({} monsters)\n", numDefs, monsterCount);

	return {};
}

EntityDef* EntityDefManager::find(int eType, int eSubType) {
	return this->find(eType, eSubType, -1);
}

EntityDef* EntityDefManager::find(int eType, int eSubType, int parm) {
	for (int i = 0; i < this->numDefs; i++) {
		if (list[i].eType == eType && list[i].eSubType == eSubType &&
		    (parm == -1 || list[i].parm == parm)) {
			return &list[i];
		}
	}
	return nullptr;
}

EntityDef* EntityDefManager::lookup(int tileIndex) {
	for (int i = 0; i < this->numDefs; ++i) {
		if (list[i].tileIndex == tileIndex) {
			return &list[i];
		}
	}
	return nullptr;
}

// ----------------
// EntityDef Class
// ----------------

// EntityDef default constructor is = default (header)
