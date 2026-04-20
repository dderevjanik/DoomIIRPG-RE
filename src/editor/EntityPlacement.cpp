#include "EntityPlacement.h"

#include "CAppContainer.h"
#include "App.h"
#include "EntityDef.h"
#include "Enums.h"
#include "Render.h"

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

}  // namespace editor
