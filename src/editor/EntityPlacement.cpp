#include "EntityPlacement.h"

#include "CAppContainer.h"
#include "App.h"
#include "EntityDef.h"
#include "Enums.h"
#include "Render.h"
#include "SpriteDefs.h"

#include <algorithm>

namespace editor {

namespace {

// Weapon-pickup animation frame. Convention established by the shipped
// sprites.yaml: every weapon has `frames: 4` with frame 2 dedicated to the
// "laying on ground" view. Used by the engine's sprite rasteriser whenever
// it samples a weapon sprite placed as a pickup (flags include z_anim).
constexpr int kWeaponPickupFrame = 2;

// True if the named flag is already on the entity.
bool hasFlag(const Entity& e, const char* flag) {
	return std::find(e.flags.begin(), e.flags.end(), flag) != e.flags.end();
}

}  // namespace

void applyPlacementDefaults(Entity& e) {
	if (e.tileId <= 0) return;

	auto* app = CAppContainer::getInstance()->app;
	if (!app || !app->entityDefManager) return;
	EntityDef* def = app->entityDefManager->lookup(e.tileId);
	if (!def) return;

	// Weapon pickup — laying flat on the floor. A Z-sprite at
	// 2*SPRITE_Z_DEFAULT ends up at the same vertical position as a normal
	// sprite standing on the floor (see Render::SPRITE_Z_DEFAULT docstring).
	// z_anim selects the ground-pickup animation frame.
	if (def->eType == Enums::ET_ITEM && def->eSubType == Enums::ITEM_WEAPON) {
		if (e.z < 0) e.z = 2 * Render::SPRITE_Z_DEFAULT;
		if (e.zAnim == 0) e.zAnim = kWeaponPickupFrame;
		return;
	}

	// Future categories go here. Intentionally empty for monsters (they work
	// fine without placement tweaks) and NPCs (the shipped data uses sprite-
	// specific `z:` and orientation flags that we'll cover when the user asks
	// for them).
}

std::vector<PreviewLayer> getPreviewLayers(int tileId) {
	std::vector<PreviewLayer> out;
	if (tileId <= 0) return out;

	auto* app = CAppContainer::getInstance()->app;
	EntityDef* def = (app && app->entityDefManager)
	                     ? app->entityDefManager->lookup(tileId)
	                     : nullptr;

	// Floaters and special bosses take custom render paths in the engine
	// (Render::renderFloaterAnim, Render::renderSpecialBossAnim) — the
	// legs/torso/head decomposition from BodyPartData does NOT apply to them.
	// Trying to apply it produces artefacts like a Lost Soul showing three
	// stacked faces, because its sprite strip has one-frame-per-pose rather
	// than body-part-per-frame.
	const bool isFloater     = def && def->hasRenderFlag(EntityDef::RFLAG_FLOATER);
	const bool isSpecialBoss = def && def->hasRenderFlag(EntityDef::RFLAG_SPECIAL_BOSS);
	if (isFloater) {
		const auto& fl = def->floater;
		if (fl.type == EntityDef::FloaterData::FLOATER_MULTIPART) {
			// Sentinel-style: torso + head (idleFrontFrame, idleFrontFrame+1).
			out.push_back({ tileId, fl.idleFrontFrame,     0 });
			// headZOffset is signed (commonly negative = head above torso).
			// The preview stacks bottom-up by zOffset, so flip the sign.
			out.push_back({ tileId, fl.idleFrontFrame + 1, -fl.headZOffset });
		} else {
			// Lost Soul / Cacodemon: just frame 0 (idleFrontFrame unused for
			// simple floaters; the runtime resets frame to 0 before rendering).
			out.push_back({ tileId, 0, 0 });
		}
		return out;
	}
	if (isSpecialBoss) {
		// Special bosses (VIOS, Mastermind, Arachnotron, Boss Pinky) have
		// non-trivial multi-part rendering we don't mirror here. Showing a
		// single idle frame is a reasonable preview fallback — the actual
		// in-game look diverges from a static stack anyway.
		out.push_back({ tileId, def->specialBoss.idleSpriteIdx, 0 });
		return out;
	}

	// Monsters and NPCs that go through Render::renderSpriteAnim reuse the
	// same sprite sheet across body parts via EntityDef::bodyParts. For the
	// MANIM_IDLE path the head is unconditionally drawn (the noHeadOnAttack
	// flag only gates attack frames, not idle), so we include all three
	// parts.
	const bool isMonster = def && def->eType == Enums::ET_MONSTER;
	const bool isNPC     = def && def->eType == Enums::ET_NPC;
	if (isMonster || isNPC) {
		const auto& bp = def->bodyParts;
		out.push_back({ tileId, bp.legsFrame,  0 });
		out.push_back({ tileId, bp.torsoFrame, bp.idleTorsoZ + 40 });
		out.push_back({ tileId, bp.headFrame,  bp.idleHeadZ  + 80 });
		return out;
	}

	// Everything else: primary + composite + glow via SpriteDefs (frame 0).
	for (const auto& layer : SpriteDefs::getRenderedLayers(tileId)) {
		out.push_back({ layer.sprite, 0, layer.zMult });
	}
	return out;
}

}  // namespace editor
