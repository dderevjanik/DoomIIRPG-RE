#!/usr/bin/env python3
"""
BSP-to-BIN Converter (v2 — tile-grid based)
Converts Doom 1 RPG .bsp level files into Doom II RPG .bin level format.

Instead of converting D1's BSP tree, this reads the D1 blockmap (32x32 tile grid)
and builds a fresh D2-compatible BSP from scratch. This avoids all painter's
algorithm draw-order issues from mismatched BSP structures.

Usage:
    python tools/bsp-to-bin.py <input.bsp> <output.bin>
    python tools/bsp-to-bin.py assets/          # converts all .bsp files
"""

import argparse
import os
import struct
import sys
from pathlib import Path

# ─── Constants ───────────────────────────────────────────────────────────────

MAP_SIZE = 32
TILE_SIZE = 64
WALL_HEIGHT_WORLD = 64  # wall height in world units (same as D2 BSPCompiler)
FLOOR_HEIGHT = 56       # floor height byte (world Z = 56*8 = 448)

# D2 placeholder textures
D2_WALL_TILE = 258
D2_FLOOR_TILE = 451
D2_CEIL_TILE = 462

# D1 door textures (0-10)
D1_DOOR_TEXTURES = set(range(0, 11))

# Poly flags (per docs/d2-rpg/BSP_COMPILER_NOTES.md and engine decoder in RenderBSP.cpp)
# Final flag byte: ((numVerts - 2) & 7) | AXIS_NONE | (SWAPXY if wall else 0)
# Floor/ceiling: 0x1a. Wall: 0x5a.
POLY_FLAG_AXIS_NONE = 0x18
POLY_FLAG_SWAPXY = 0x40

# ─── Binary helpers ──────────────────────────────────────────────────────────

def read_byte(data, pos):
    return data[pos], pos + 1

def read_short(data, pos):
    return struct.unpack_from('<h', data, pos)[0], pos + 2

def read_ushort(data, pos):
    return struct.unpack_from('<H', data, pos)[0], pos + 2

def read_int(data, pos):
    return struct.unpack_from('<i', data, pos)[0], pos + 4

def write_byte(val):
    return struct.pack('<B', val & 0xFF)

def write_short(val):
    return struct.pack('<h', val)

def write_ushort(val):
    return struct.pack('<H', val & 0xFFFF)

def write_int(val):
    return struct.pack('<i', val)

def write_marker(marker=0xDEADBEEF):
    return struct.pack('<I', marker)

MARKER_DEAD = 0xDEADBEEF
MARKER_CAFE = 0xCAFEBABE

# ─── BSP Map Parser ─────────────────────────────────────────────────────────

class BSPMap:
    def __init__(self):
        self.map_name = ""
        self.floor_color = (0, 0, 0)
        self.ceiling_color = (0, 0, 0)
        self.floor_tex = 0
        self.ceiling_tex = 0
        self.spawn_index = 0
        self.spawn_dir = 0
        self.block_map = [0] * 1024
        self.lines = []
        self.nodes = []
        self.sprites = []
        self.tile_events = []
        self.bytecodes = []
        self.strings = []
        self.plane_textures = [[0]*1024, [0]*1024]

def parse_bsp(data):
    bsp = BSPMap()
    pos = 0
    bsp.map_name = data[0:16].split(b'\x00')[0].decode('ascii', errors='replace')
    pos = 16
    fr, pos = read_byte(data, pos); fg, pos = read_byte(data, pos); fb, pos = read_byte(data, pos)
    bsp.floor_color = (fr, fg, fb)
    cr, pos = read_byte(data, pos); cg, pos = read_byte(data, pos); cb, pos = read_byte(data, pos)
    bsp.ceiling_color = (cr, cg, cb)
    bsp.floor_tex, pos = read_byte(data, pos)
    bsp.ceiling_tex, pos = read_byte(data, pos)
    pos += 3  # intro color
    pos += 1  # loadMapID
    bsp.spawn_index, pos = read_short(data, pos)
    bsp.spawn_dir, pos = read_byte(data, pos)
    pos += 2  # cameraSpawnIndex

    # Skip nodes
    nn, pos = read_short(data, pos)
    for _ in range(nn):
        x1 = data[pos] << 3; pos += 1
        y1 = data[pos] << 3; pos += 1
        x2 = data[pos] << 3; pos += 1
        y2 = data[pos] << 3; pos += 1
        a1h, pos = read_byte(data, pos)
        a1l = data[pos] << 3; pos += 1
        a2l, pos = read_short(data, pos)
        a2h, pos = read_short(data, pos)
        args1 = a1l | (a1h << 16)
        args2 = (a2l & 0xFFFF) | (a2h << 16)
        bsp.nodes.append((x1, y1, x2, y2, args1, args2))

    # Read lines
    nl, pos = read_short(data, pos)
    for _ in range(nl):
        v1x = data[pos] << 3; pos += 1
        v1y = data[pos] << 3; pos += 1
        v2x = data[pos] << 3; pos += 1
        v2y = data[pos] << 3; pos += 1
        tex, pos = read_short(data, pos)
        flags, pos = read_int(data, pos)
        bsp.lines.append((v1x, v1y, v2x, v2y, tex, flags))

    # Skip sprites, events, bytecodes, strings
    ns, pos = read_short(data, pos)
    pos += ns * 5
    ne, pos = read_short(data, pos)
    pos += ne * 4
    nb, pos = read_short(data, pos)
    pos += nb * 9
    nstr, pos = read_short(data, pos)
    for _ in range(nstr):
        slen, pos = read_short(data, pos)
        pos += slen

    # BlockMap
    i = 0
    for _ in range(256):
        flags, pos = read_byte(data, pos)
        bsp.block_map[i+0] = flags & 3
        bsp.block_map[i+1] = (flags >> 2) & 3
        bsp.block_map[i+2] = (flags >> 4) & 3
        bsp.block_map[i+3] = (flags >> 6) & 3
        i += 4

    # Plane textures
    for plane in range(2):
        for tile in range(1024):
            bsp.plane_textures[plane][tile], pos = read_byte(data, pos)

    return bsp

# ─── Tile Grid → Geometry ────────────────────────────────────────────────────

def is_solid(block_map, col, row):
    if col < 0 or col >= MAP_SIZE or row < 0 or row >= MAP_SIZE:
        return True
    return (block_map[row * MAP_SIZE + col] & 1) != 0

def is_door_tile(bsp, col, row):
    """Check if any D1 line at this tile edge is a door."""
    # Check all lines to see if any door line touches this tile
    cx = col * TILE_SIZE + TILE_SIZE // 2
    cy = row * TILE_SIZE + TILE_SIZE // 2
    for v1x, v1y, v2x, v2y, tex, flags in bsp.lines:
        if tex not in D1_DOOR_TEXTURES:
            continue
        mx = (v1x + v2x) // 2
        my = (v1y + v2y) // 2
        if abs(mx - cx) <= TILE_SIZE and abs(my - cy) <= TILE_SIZE:
            return True
    return False

def generate_geometry(bsp):
    """Generate wall quads, floor quads, and collision lines from the D1 tile grid.

    Returns:
        polys: list of (verts, tile_num, is_wall, owner_tile)
               owner_tile = (col, row) of the open tile this poly belongs to
               - Floor/ceiling quad: the tile itself
               - Wall quad: the adjacent OPEN neighbor (not the solid source)
        lines: list of (x0, x1, y0, y1, flags, owner_tile)
               owner_tile = (col, row) of the adjacent open tile
    """
    polys = []
    lines = []

    floor_z = FLOOR_HEIGHT * 8  # world Z for floor
    ceil_z = floor_z + WALL_HEIGHT_WORLD  # world Z for ceiling

    dirs = [(1, 0), (-1, 0), (0, 1), (0, -1)]

    for row in range(MAP_SIZE):
        for col in range(MAP_SIZE):
            solid = is_solid(bsp.block_map, col, row)

            if solid:
                # Wall quads on edges adjacent to non-solid tiles
                for dc, dr in dirs:
                    nc, nr = col + dc, row + dr
                    if not is_solid(bsp.block_map, nc, nr):
                        # Wall edge world coords
                        if dc == 1:  # East edge
                            wx0, wy0 = (col+1)*64, row*64
                            wx1, wy1 = (col+1)*64, (row+1)*64
                        elif dc == -1:  # West edge
                            wx0, wy0 = col*64, (row+1)*64
                            wx1, wy1 = col*64, row*64
                        elif dr == 1:  # South edge
                            wx0, wy0 = (col+1)*64, (row+1)*64
                            wx1, wy1 = col*64, (row+1)*64
                        else:  # North edge
                            wx0, wy0 = col*64, row*64
                            wx1, wy1 = (col+1)*64, row*64

                        # Vertex bytes: engine does byte<<7, viewX = world*16
                        # So byte = world*16/128 = world/8 = world>>3
                        bx0, by0 = wx0 >> 3, wy0 >> 3
                        bx1, by1 = wx1 >> 3, wy1 >> 3
                        bzl, bzh = floor_z >> 3, ceil_z >> 3

                        # UV matches real D2 maps — 0..16 span per tile
                        # (t=0 at floor, t=16 at ceiling; s grows along wall).
                        # Engine decodes s<<6 = texel, so 16<<6 = 1024 texels.
                        verts = [
                            (bx0, by0, bzl, 0, 0),
                            (bx1, by1, bzl, 16, 0),
                            (bx1, by1, bzh, 16, 16),
                            (bx0, by0, bzh, 0, 16),
                        ]
                        polys.append((verts, D2_WALL_TILE, True, (nc, nr)))

                        # Collision line byte coords: traceWorld does byte<<3 for world coords
                        lx0, ly0 = wx0 >> 3, wy0 >> 3
                        lx1, ly1 = wx1 >> 3, wy1 >> 3
                        # Clamp to byte range
                        lx0 = min(255, lx0); ly0 = min(255, ly0)
                        lx1 = min(255, lx1); ly1 = min(255, ly1)
                        # Type 4 = passable for door tiles, 0 = solid wall
                        line_type = 4 if is_door_tile(bsp, nc, nr) else 0
                        lines.append((lx0, lx1, ly0, ly1, line_type, (nc, nr)))

            else:
                # Floor quad
                wx0, wy0 = col * 64, row * 64
                wx1, wy1 = (col+1) * 64, (row+1) * 64
                bx0, by0 = wx0 >> 3, wy0 >> 3
                bx1, by1 = wx1 >> 3, wy1 >> 3
                bz = floor_z >> 3

                verts = [
                    (bx0, by0, bz, 0, 0),
                    (bx1, by0, bz, 16, 0),
                    (bx1, by1, bz, 16, 16),
                    (bx0, by1, bz, 0, 16),
                ]
                polys.append((verts, D2_FLOOR_TILE, False, (col, row)))

                # Ceiling quad (reversed winding)
                bzc = ceil_z >> 3
                verts = [
                    (bx0, by1, bzc, 0, 16),
                    (bx1, by1, bzc, 16, 16),
                    (bx1, by0, bzc, 16, 0),
                    (bx0, by0, bzc, 0, 0),
                ]
                polys.append((verts, D2_CEIL_TILE, False, (col, row)))

    return polys, lines

# ─── BSP Tree Builder ────────────────────────────────────────────────────────

class BSPNode:
    def __init__(self):
        self.is_leaf = False
        self.axis = 0          # 0=X, 1=Y
        self.split_coord = 0   # world coords
        self.left = -1
        self.right = -1
        self.poly_indices = []
        self.line_indices = []
        self.tile_set = set()  # leaves only: set of (col, row)
        self.min_x = 0
        self.min_y = 0
        self.max_x = 0
        self.max_y = 0


def _count_crossings_x(open_tiles, k, r0, r1):
    """Open-tile crossings at vertical split between col k-1 and col k, within rows [r0, r1)."""
    return sum(1 for r in range(r0, r1)
               if (k - 1, r) in open_tiles and (k, r) in open_tiles)


def _count_crossings_y(open_tiles, k, c0, c1):
    """Open-tile crossings at horizontal split between row k-1 and row k, within cols [c0, c1)."""
    return sum(1 for c in range(c0, c1)
               if (c, k - 1) in open_tiles and (c, k) in open_tiles)


def _has_open_in_rect(open_tiles, c0, r0, c1, r1):
    for c in range(c0, c1):
        for r in range(r0, r1):
            if (c, r) in open_tiles:
                return True
    return False


def build_bsp(open_tiles, c0, r0, c1, r1, depth, nodes):
    """Tile-centric BSP builder (see docs/d2-rpg/BSP_COMPILER_NOTES.md).

    Partitions the set of OPEN tiles using axis-aligned splits at tile boundaries.
    Each candidate split is scored by the number of open-tile crossings (open on
    both sides with no wall between); the split with the FEWEST crossings wins,
    tiebreak = closest to the region midpoint.

    Leaves hold a tile_set; poly/line attachment happens afterward in
    attach_polys_lines_to_leaves().

    Bounds are TILE indices; c1/r1 are exclusive.
    Returns the index of the root of this subtree in `nodes`.
    """
    idx = len(nodes)
    nodes.append(BSPNode())
    node = nodes[idx]

    # AABB in byte space for cullBoundingBox. Engine reads byte << 7.
    node.min_x = min(255, (c0 * TILE_SIZE) >> 3)
    node.min_y = min(255, (r0 * TILE_SIZE) >> 3)
    node.max_x = min(255, (c1 * TILE_SIZE) >> 3)
    node.max_y = min(255, (r1 * TILE_SIZE) >> 3)

    tile_set = {(c, r) for (c, r) in open_tiles if c0 <= c < c1 and r0 <= r < r1}

    # Leaf termination
    if (c1 - c0 <= 1 and r1 - r0 <= 1) or not tile_set:
        node.is_leaf = True
        node.tile_set = tile_set
        return idx

    preferred = 'x' if depth % 2 == 0 else 'y'
    other = 'y' if preferred == 'x' else 'x'
    best = None  # (crossings, dist_from_mid, axis, k)

    for axis in (preferred, other):
        if axis == 'x':
            if c1 - c0 <= 1:
                continue
            candidates = range(c0 + 1, c1)
            mid = (c0 + c1) / 2
        else:
            if r1 - r0 <= 1:
                continue
            candidates = range(r0 + 1, r1)
            mid = (r0 + r1) / 2

        axis_best = None
        for k in candidates:
            if axis == 'x':
                left_has = _has_open_in_rect(open_tiles, c0, r0, k, r1)
                right_has = _has_open_in_rect(open_tiles, k, r0, c1, r1)
            else:
                left_has = _has_open_in_rect(open_tiles, c0, r0, c1, k)
                right_has = _has_open_in_rect(open_tiles, c0, k, c1, r1)
            if not (left_has and right_has):
                continue

            if axis == 'x':
                crossings = _count_crossings_x(open_tiles, k, r0, r1)
            else:
                crossings = _count_crossings_y(open_tiles, k, c0, c1)
            score = (crossings, abs(k - mid))
            if axis_best is None or score < axis_best[:2]:
                axis_best = (score[0], score[1], axis, k)

        if axis_best is not None:
            if best is None or axis_best[:2] < best[:2]:
                best = axis_best
            # Honor preferred axis: if we found any valid split on it, commit.
            if axis == preferred:
                break

    if best is None:
        node.is_leaf = True
        node.tile_set = tile_set
        return idx

    _, _, axis, k = best
    node.is_leaf = False
    node.axis = 0 if axis == 'x' else 1
    # split_coord is a WORLD coord at the tile boundary; convert to render-space
    # offset later in the emit step (splitCoord_world * 16).
    node.split_coord = k * TILE_SIZE

    if axis == 'x':
        left_idx = build_bsp(open_tiles, c0, r0, k, r1, depth + 1, nodes)
        right_idx = build_bsp(open_tiles, k, r0, c1, r1, depth + 1, nodes)
    else:
        left_idx = build_bsp(open_tiles, c0, r0, c1, k, depth + 1, nodes)
        right_idx = build_bsp(open_tiles, c0, k, c1, r1, depth + 1, nodes)

    nodes[idx].left = left_idx
    nodes[idx].right = right_idx
    return idx


def attach_polys_lines_to_leaves(nodes, polys, lines):
    """After build_bsp, populate each leaf's poly_indices and line_indices
    based on each record's owner_tile falling within the leaf's tile_set."""
    # Build tile -> poly/line index maps for O(1) lookup.
    tile_polys = {}
    for i, rec in enumerate(polys):
        owner = rec[3]
        tile_polys.setdefault(owner, []).append(i)

    tile_lines = {}
    for i, rec in enumerate(lines):
        owner = rec[5]
        tile_lines.setdefault(owner, []).append(i)

    for node in nodes:
        if not node.is_leaf:
            continue
        poly_idxs = []
        line_idxs = []
        for tile in node.tile_set:
            poly_idxs.extend(tile_polys.get(tile, ()))
            line_idxs.extend(tile_lines.get(tile, ()))
        node.poly_indices = poly_idxs
        node.line_indices = line_idxs

# ─── Pack into D2 .bin format ────────────────────────────────────────────────

def pack_polys(polys, poly_indices):
    """Pack polygon data for a leaf node, grouped by texture.

    Polys are 4-tuples: (verts, tile_num, is_wall, owner_tile).
    """
    from collections import OrderedDict
    by_tex = OrderedDict()
    for pi in poly_indices:
        rec = polys[pi]
        tile_num = rec[1]
        if tile_num not in by_tex:
            by_tex[tile_num] = []
        by_tex[tile_num].append(rec)

    data = bytearray()
    data.append(len(by_tex))  # meshCount

    for tile_num, entries in by_tex.items():
        assert len(entries) <= 127, (
            f"pack_polys: mesh for tile {tile_num} has {len(entries)} polys "
            f"(max 127 per mesh). Split this mesh or reduce leaf size."
        )
        data.extend(b'\x00\x00\x00\x00')  # reserved
        count = len(entries)
        packed = ((tile_num & 0x1FF) << 7) | (count & 0x7F)
        data.append(packed & 0xFF)
        data.append((packed >> 8) & 0xFF)

        for rec in entries:
            verts, _tn, is_wall, _owner = rec
            num_verts = len(verts)
            flags = ((num_verts - 2) & 0x7) | POLY_FLAG_AXIS_NONE
            if is_wall:
                flags |= POLY_FLAG_SWAPXY
            data.append(flags)
            for x, y, z, s, t in verts:
                data.extend([x & 0xFF, y & 0xFF, z & 0xFF, s & 0xFF, t & 0xFF])

    return data

def convert_bsp_to_bin(bsp):
    """Convert D1 BSP map to D2 .bin format using fresh tile-grid BSP."""

    # Step 1: Generate geometry from tile grid
    polys, lines = generate_geometry(bsp)

    # Step 2: Build BSP tree — tile-centric, crossings-minimization.
    # See docs/d2-rpg/BSP_COMPILER_NOTES.md for the algorithm.
    open_tiles = {(c, r)
                  for r in range(MAP_SIZE)
                  for c in range(MAP_SIZE)
                  if not is_solid(bsp.block_map, c, r)}
    nodes = []
    build_bsp(open_tiles, 0, 0, MAP_SIZE, MAP_SIZE, 0, nodes)
    attach_polys_lines_to_leaves(nodes, polys, lines)

    # Per-leaf constraint checks (engine format limits).
    leaf_count = sum(1 for n in nodes if n.is_leaf)
    assert leaf_count <= 256, f"BSP has {leaf_count} leaves (engine limit 256)"
    for i, n in enumerate(nodes):
        if n.is_leaf:
            assert len(n.line_indices) <= 63, (
                f"leaf {i} has {len(n.line_indices)} collision lines (max 63)"
            )

    # Step 3: Collect lines in leaf order (contiguous ranges)
    all_lines_ordered = []
    leaf_line_start = {}
    leaf_line_count = {}
    for i, node in enumerate(nodes):
        if node.is_leaf:
            leaf_line_start[i] = len(all_lines_ordered)
            leaf_line_count[i] = len(node.line_indices)
            assert leaf_line_start[i] <= 0x3FF, (
                f"leaf {i} lineStart={leaf_line_start[i]} exceeds 10-bit field (max 1023)"
            )
            for li in node.line_indices:
                all_lines_ordered.append(lines[li])

    # Step 4: Pack polygon data per leaf
    leaf_poly_data = {}
    all_poly_data = bytearray()
    for i, node in enumerate(nodes):
        if node.is_leaf:
            offset = len(all_poly_data)
            leaf_poly_data[i] = offset
            all_poly_data.extend(pack_polys(polys, node.poly_indices))

    # Step 5: Normals — negative, matching real D2 maps (not BSPCompiler source)
    # nodeClassifyPoint: (viewX * (-16384) >> 14) + offset = -viewX + offset
    # Result >= 0 when viewX <= offset → child1 (left) first
    normals = [(-16384, 0, 0), (0, -16384, 0)]

    # Step 6: Build output arrays
    num_nodes = len(nodes)
    num_lines = len(all_lines_ordered)
    num_normals = len(normals)
    data_size_polys = len(all_poly_data)
    assert data_size_polys <= 0xFFFF, (
        f"dataSizePolys={data_size_polys} exceeds uint16 (max 65535)"
    )

    node_offsets = []
    node_normal_idxs = []
    node_child1 = []
    node_child2 = []
    node_bounds = []

    for i, node in enumerate(nodes):
        node_bounds.extend([node.min_x & 0xFF, node.min_y & 0xFF,
                            node.max_x & 0xFF, node.max_y & 0xFF])
        if node.is_leaf:
            node_offsets.append(0xFFFF)
            node_normal_idxs.append(0)
            node_child1.append(leaf_poly_data.get(i, 0) & 0xFFFF)
            ls = leaf_line_start.get(i, 0)
            lc = leaf_line_count.get(i, 0)
            node_child2.append((ls & 0x3FF) | ((lc & 0x3F) << 10))
        else:
            node_normal_idxs.append(node.axis)
            # classifyPoint: (-viewX * 16384 >> 14) + offset = -viewX + offset
            # viewX is in render space = world * 16
            # offset = splitCoord_render = splitCoord_world * 16
            offset = (node.split_coord * 16) & 0xFFFF
            node_offsets.append(offset)
            node_child1.append(node.left & 0xFFFF)
            node_child2.append(node.right & 0xFFFF)

    # Line arrays
    line_flags_arr = bytearray((num_lines + 1) // 2)
    line_xs = bytearray(num_lines * 2)
    line_ys = bytearray(num_lines * 2)
    for i, (x1, x2, y1, y2, flags, _owner) in enumerate(all_lines_ordered):
        line_xs[i*2+0] = x1 & 0xFF
        line_xs[i*2+1] = x2 & 0xFF
        line_ys[i*2+0] = y1 & 0xFF
        line_ys[i*2+1] = y2 & 0xFF
        nibble = flags & 0x0F
        if i & 1:
            line_flags_arr[i >> 1] |= (nibble << 4)
        else:
            line_flags_arr[i >> 1] |= nibble

    # Height map
    height_map = bytearray([FLOOR_HEIGHT] * 1024)

    # Map flags (D1 blockmap → D2 nibble-packed)
    map_flags_512 = bytearray(512)
    for i in range(512):
        ta, tb = i * 2, i * 2 + 1
        fa = bsp.block_map[ta] & 0x0F if ta < 1024 else 0
        fb = bsp.block_map[tb] & 0x0F if tb < 1024 else 0
        map_flags_512[i] = (fa & 0x0F) | ((fb & 0x0F) << 4)

    # Clear solid flag on door tiles
    for v1x, v1y, v2x, v2y, tex, flags in bsp.lines:
        if tex not in D1_DOOR_TEXTURES:
            continue
        mx, my = (v1x + v2x) // 2, (v1y + v2y) // 2
        tx, ty = mx // 64, my // 64
        tidx = ty * 32 + tx
        if 0 <= tidx < 1024:
            byte_idx = tidx // 2
            if tidx & 1:
                map_flags_512[byte_idx] &= 0x0F
            else:
                map_flags_512[byte_idx] &= 0xF0

    # Media indices
    media_indices = sorted(set([D2_WALL_TILE, D2_FLOOR_TILE, D2_CEIL_TILE]))

    # Spawn
    d2_spawn_dir = (bsp.spawn_dir // 32) & 7

    # ── Write .bin ──
    out = bytearray()

    # Header (42 bytes)
    out += write_byte(3)                          # version
    out += write_int(0x44314250)                  # mapCompileDate "D1BP"
    out += write_ushort(bsp.spawn_index)          # mapSpawnIndex
    out += write_byte(d2_spawn_dir)               # mapSpawnDir
    out += write_byte(0)                          # mapFlagsBitmask
    out += write_byte(0)                          # totalSecrets
    out += write_byte(0)                          # totalLoot
    out += write_ushort(num_nodes)                # numNodes
    out += write_ushort(data_size_polys)          # dataSizePolys
    out += write_ushort(num_lines)                # numLines
    out += write_ushort(num_normals)              # numNormals
    out += write_ushort(1)                        # numNormalSprites (1 dummy)
    out += write_short(0)                         # numZSprites
    out += write_short(0)                         # numTileEvents
    out += write_short(0)                         # mapByteCodeSize
    out += write_byte(0)                          # totalMayaCameras
    out += write_short(0)                         # totalMayaCameraKeys
    for _ in range(6):
        out += write_short(-1)                    # mayaTweenOffsets

    # 1. Media
    out += write_marker(MARKER_DEAD)
    out += write_ushort(len(media_indices))
    for idx in media_indices:
        out += write_ushort(idx)
    out += write_marker(MARKER_DEAD)

    # 2. Normals
    for nx, ny, nz in normals:
        out += write_short(nx)
        out += write_short(ny)
        out += write_short(nz)
    out += write_marker(MARKER_CAFE)

    # 3. Node Offsets
    for v in node_offsets:
        out += write_ushort(v)
    out += write_marker(MARKER_CAFE)

    # 4. Node Normal Indices
    for v in node_normal_idxs:
        out += write_byte(v)
    out += write_marker(MARKER_CAFE)

    # 5. Node Child Offsets
    for v in node_child1:
        out += write_ushort(v)
    for v in node_child2:
        out += write_ushort(v)
    out += write_marker(MARKER_CAFE)

    # 6. Node Bounds
    out += bytes(node_bounds)
    out += write_marker(MARKER_CAFE)

    # 7. Node Polygons
    out += bytes(all_poly_data)
    out += write_marker(MARKER_CAFE)

    # 8. Line Data
    out += bytes(line_flags_arr)
    out += bytes(line_xs)
    out += bytes(line_ys)
    out += write_marker(MARKER_CAFE)

    # 9. Height Map
    out += bytes(height_map)
    out += write_marker(MARKER_CAFE)

    # 10-14. Sprites (1 invisible dummy at spawn)
    spawn_tx = (bsp.spawn_index % 32) * 8 + 4
    spawn_ty = (bsp.spawn_index // 32) * 8 + 4
    out += bytes([spawn_tx])  # spriteX
    out += bytes([spawn_ty])  # spriteY
    out += bytes([0])         # spriteInfoLow
    out += write_marker(MARKER_CAFE)
    out += struct.pack('<H', 0x0001)  # spriteInfoHigh (invisible)
    out += write_marker(MARKER_CAFE)
    out += write_marker(MARKER_CAFE)  # Z-sprite heights (empty)
    out += write_marker(MARKER_CAFE)  # Z-sprite info mid (empty)

    # 15. Static Functions
    for _ in range(12):
        out += write_ushort(0xFFFF)
    out += write_marker(MARKER_CAFE)

    # 16. Tile Events (empty)
    out += write_marker(MARKER_CAFE)

    # 17. Bytecode (empty)
    out += write_marker(MARKER_CAFE)

    # 18. Maya Cameras (empty)
    out += write_marker(MARKER_DEAD)

    # 19. Map Flags
    out += bytes(map_flags_512)
    out += write_marker(MARKER_CAFE)

    return bytes(out), len(polys), len(lines), len(nodes)

# ─── Level YAML generation ───────────────────────────────────────────────────

def generate_level_yaml(bsp, output_path):
    lines = []
    lines.append(f"# Level configuration generated from {bsp.map_name}")
    lines.append(f"name: {bsp.map_name.lower().replace(' ', '_')}")
    lines.append("")

    spawn_x = bsp.spawn_index % 32
    spawn_y = bsp.spawn_index // 32
    d2_dir = (bsp.spawn_dir // 32) & 7
    dir_names = ["east", "northeast", "north", "northwest", "west", "southwest", "south", "southeast"]
    lines.append("player_spawn:")
    lines.append(f"  x: {spawn_x}")
    lines.append(f"  y: {spawn_y}")
    lines.append(f"  dir: {dir_names[d2_dir]}")
    lines.append("")

    lines.append("textures:")
    lines.append("  - texture_258")
    lines.append("  - texture_451")
    lines.append("  - texture_462")
    lines.append("  - door_unlocked")
    lines.append("")

    lines.append("height_map:")
    for row in range(32):
        lines.append(f"  - [{', '.join([str(FLOOR_HEIGHT)] * 32)}]")
    lines.append("")

    # Door sprites
    door_count = 0
    sprite_lines = []
    for v1x, v1y, v2x, v2y, tex, flags in bsp.lines:
        if tex not in D1_DOOR_TEXTURES:
            continue
        mid_x = (v1x + v2x) // 2
        mid_y = (v1y + v2y) // 2
        dx = abs(v2x - v1x)
        dy = abs(v2y - v1y)
        dir_flags = "north, south" if dx > dy else "east, west"
        sprite_lines.append(f"  - x: {mid_x}")
        sprite_lines.append(f"    y: {mid_y}")
        sprite_lines.append(f"    tile: door_unlocked")
        sprite_lines.append(f"    flags: [animation, {dir_flags}]")
        door_count += 1

    if sprite_lines:
        lines.append("sprites:")
        lines.extend(sprite_lines)
        lines.append("")

    yaml_path = output_path.replace('.bin', '_level.yaml')
    with open(yaml_path, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    return yaml_path, door_count

# ─── Main ────────────────────────────────────────────────────────────────────

def convert_file(input_path, output_path):
    with open(input_path, 'rb') as f:
        data = f.read()

    bsp = parse_bsp(data)
    bin_data, num_polys, num_lines, num_nodes = convert_bsp_to_bin(bsp)

    with open(output_path, 'wb') as f:
        f.write(bin_data)

    yaml_path, door_count = generate_level_yaml(bsp, output_path)

    print(f"  {os.path.basename(input_path):30s} -> {os.path.basename(output_path)}")
    print(f"    polys={num_polys:5d}  lines={num_lines:4d}  nodes={num_nodes:4d}  "
          f"doors={door_count:3d}  size={len(data)} -> {len(bin_data)}")
    print(f"    + {os.path.basename(yaml_path)}")


def main():
    parser = argparse.ArgumentParser(description="Convert Doom RPG .bsp maps to Doom II RPG .bin format")
    parser.add_argument("input", help="Path to .bsp file or directory containing .bsp files")
    parser.add_argument("output", nargs="?", default=None,
                        help="Output .bin file or directory (default: same dir with .bin extension)")
    args = parser.parse_args()

    input_path = os.path.abspath(args.input)

    if os.path.isdir(input_path):
        bsp_files = sorted(f for f in os.listdir(input_path) if f.endswith('.bsp'))
        if not bsp_files:
            print(f"No .bsp files found in {input_path}")
            sys.exit(1)
        out_dir = os.path.abspath(args.output) if args.output else input_path
        os.makedirs(out_dir, exist_ok=True)
        print(f"Converting {len(bsp_files)} BSP files...")
        for bsp_file in bsp_files:
            convert_file(os.path.join(input_path, bsp_file),
                         os.path.join(out_dir, bsp_file.replace('.bsp', '.bin')))
        print(f"\nDone. Output in {out_dir}")
    elif os.path.isfile(input_path):
        output_path = os.path.abspath(args.output) if args.output else input_path.replace('.bsp', '.bin')
        convert_file(input_path, output_path)
    else:
        print(f"ERROR: Input not found: {input_path}")
        sys.exit(1)


if __name__ == "__main__":
    main()
