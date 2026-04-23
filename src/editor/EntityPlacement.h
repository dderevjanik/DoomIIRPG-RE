#pragma once

#include "MapProject.h"

#include <vector>

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

// One billboard that the editor's sprite preview should render for an entity.
// `sprite` is the source tile, `frame` is the frame index into its strip, and
// `zOffset` is a vertical stacking offset (canvas units, relative to the
// bottom layer). Exactly mirrors how the engine's renderSpriteAnim layers a
// monster's legs / torso / head at runtime.
struct PreviewLayer {
	int sprite;
	int frame;
	int zOffset;
};

// Every billboard the editor should draw to preview tile `tileId`:
//   * monsters → legs (frame 0) + torso (frame 2) + head (frame 3), stacked
//     by their EntityDef's BodyPartData::idleTorsoZ / idleHeadZ offsets;
//   * other sprites → primary + composite + glow layers from SpriteDefs.
// Returned list is empty only when tileId <= 0; otherwise it has at least one
// entry (the primary sprite at frame 0, zOffset 0).
std::vector<PreviewLayer> getPreviewLayers(int tileId);

}  // namespace editor
