#pragma once

#include "MapProject.h"

namespace editor {

// Fills in sensible `z`, `zAnim`, and `flags` defaults for a freshly-placed
// entity based on the engine's own taxonomy of the sprite (queried via
// app->entityDefManager->lookup(tileId)). No-op when the sprite has no
// registered EntityDef (unknown / decor).
//
// Kept in one file so future categories (NPCs, ammo, consumables, corpses)
// can extend the switch without touching EditorState::applyEntity, and so
// the engine constants we defer to (Render::SPRITE_Z_DEFAULT, sprite
// animation-frame conventions) stay visible in one place.
void applyPlacementDefaults(Entity& e);

}  // namespace editor
