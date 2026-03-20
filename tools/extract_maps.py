#!/usr/bin/env python3
"""
extract_maps.py - Extract DoomIIRPG map/model geometry with texture references.

Parses map/model .bin files and newMappings.bin to produce JSON files
suitable for the map-viewer.html tool. Each JSON contains triangulated
geometry with tile→media texture mapping.

Usage:
    python3 extract_maps.py [path/to/Packages] [output_dir]

Output per map:
    map00.json — geometry + texture refs
    tile_mapping.json — global tile→media index table
"""

import struct
import sys
import os
import json
import glob

DEADBEEF = 0xDEADBEEF
CAFEBABE = 0xCAFEBABE

# Polygon flags from Enums.h
POLY_FLAG_VERTS_MASK = 7
POLY_FLAG_AXIS_X = 0
POLY_FLAG_AXIS_Y = 8
POLY_FLAG_AXIS_Z = 16
POLY_FLAG_AXIS_MASK = 24
POLY_FLAG_SWAPXY = 64
POLY_FLAG_UV_DELTAX = 128

MEDIA_MAX_MAPPINGS = 512
MEDIA_MAX_IMAGES = 1024
MEDIA_FLAG_REFERENCE = 0x80000000


class BinReader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def u8(self):
        v = self.data[self.pos]; self.pos += 1; return v

    def i8(self):
        v = struct.unpack_from('<b', self.data, self.pos)[0]; self.pos += 1; return v

    def u16(self):
        v = struct.unpack_from('<H', self.data, self.pos)[0]; self.pos += 2; return v

    def i16(self):
        v = struct.unpack_from('<h', self.data, self.pos)[0]; self.pos += 2; return v

    def u32(self):
        v = struct.unpack_from('<I', self.data, self.pos)[0]; self.pos += 4; return v

    def i32(self):
        v = struct.unpack_from('<i', self.data, self.pos)[0]; self.pos += 4; return v

    def skip(self, n):
        self.pos += n

    def read_bytes(self, n):
        r = self.data[self.pos:self.pos + n]; self.pos += n; return r

    def expect(self, marker):
        v = self.u32()
        if v != marker:
            raise ValueError(f"Expected {marker:#010x} at {self.pos - 4}, got {v:#010x}")


def load_tile_mapping(packages_dir):
    """Load newMappings.bin and build tile→media index table."""
    path = os.path.join(packages_dir, 'newMappings.bin')
    data = open(path, 'rb').read()

    # mediaMappings: int16[512]
    mappings = [struct.unpack_from('<h', data, i * 2)[0] for i in range(MEDIA_MAX_MAPPINGS)]

    # mediaDimensions: uint8[1024]
    dims = list(data[1024:1024 + MEDIA_MAX_IMAGES])

    # mediaBounds: int16[4096] (at offset 1024+1024 = 2048)
    bounds_offset = 1024 + MEDIA_MAX_IMAGES
    bounds = [struct.unpack_from('<h', data, bounds_offset + i * 2)[0]
              for i in range(MEDIA_MAX_IMAGES * 4)]

    # Build tile→media info
    tile_map = {}
    for t in range(MEDIA_MAX_MAPPINGS - 1):
        start = mappings[t]
        end = mappings[t + 1]
        if start >= 0 and end > start:
            media_id = start
            if media_id < MEDIA_MAX_IMAGES:
                d = dims[media_id]
                w = 1 << ((d >> 4) & 0xF)
                h = 1 << (d & 0xF)
                tile_map[t] = {
                    'media_id': media_id,
                    'frames': end - start,
                    'width': w,
                    'height': h,
                }

    return tile_map, mappings, dims


def parse_map(data, tile_mappings):
    """Parse a map/model .bin file and extract geometry grouped by tile number."""
    r = BinReader(data)

    # Header (42 bytes)
    version = r.u8()
    if version != 3:
        raise ValueError(f"Bad version: {version}")
    compile_date = r.u32()
    spawn_index = r.u16()
    spawn_dir = r.u8()
    flags_bitmask = r.i8()
    total_secrets = r.i8()
    total_loot = r.u8()
    num_nodes = r.u16()
    data_size_polys = r.u16()
    num_lines = r.u16()
    num_normals = r.u16()
    num_normal_sprites = r.u16()
    num_z_sprites = r.i16()
    num_tile_events = r.i16()
    map_bytecode_size = r.i16()
    total_maya_cameras = r.u8()
    total_maya_camera_keys = r.i16()
    for _ in range(6):
        r.i16()

    num_map_sprites = num_normal_sprites + num_z_sprites

    # Section 1: Media (tile numbers used by this map)
    r.expect(DEADBEEF)
    media_count = r.u16()
    map_media_indices = [r.u16() for _ in range(media_count)]
    r.expect(DEADBEEF)

    # Section 2: Normals
    normals = []
    for _ in range(num_normals):
        normals.append((r.i16(), r.i16(), r.i16()))
    r.expect(CAFEBABE)

    # Section 3: Node offsets
    node_offsets = [r.i16() for _ in range(num_nodes)]
    r.expect(CAFEBABE)

    # Section 4: Node normal indices
    node_normal_idxs = [r.u8() for _ in range(num_nodes)]
    r.expect(CAFEBABE)

    # Section 5: Node child offsets
    node_child1 = [r.i16() for _ in range(num_nodes)]
    node_child2 = [r.i16() for _ in range(num_nodes)]
    r.expect(CAFEBABE)

    # Section 6: Node bounds
    node_bounds_data = r.read_bytes(num_nodes * 4)
    r.expect(CAFEBABE)

    # Section 7: Node polygons
    node_polys = r.read_bytes(data_size_polys)
    r.expect(CAFEBABE)

    # Section 8: Lines
    line_flags = r.read_bytes((num_lines + 1) // 2)
    line_xs = r.read_bytes(num_lines * 2)
    line_ys = r.read_bytes(num_lines * 2)
    r.expect(CAFEBABE)

    # Section 9: Height map
    height_map = list(r.read_bytes(1024))
    r.expect(CAFEBABE)

    # Section 10-14: Sprites
    sprite_x = [r.i8() for _ in range(num_map_sprites)]
    sprite_y = [r.i8() for _ in range(num_map_sprites)]
    sprite_info_low = [r.u8() for _ in range(num_map_sprites)]
    r.expect(CAFEBABE)
    sprite_info_high = [r.u16() for _ in range(num_map_sprites)]
    r.expect(CAFEBABE)
    if num_z_sprites > 0:
        sprite_z = [r.u8() for _ in range(num_z_sprites)]
        r.expect(CAFEBABE)
        sprite_info_mid = [r.u8() for _ in range(num_z_sprites)]
        r.expect(CAFEBABE)

    # Extract geometry
    geometry = extract_geometry(node_polys, node_offsets, node_child1, tile_mappings)

    # Build sprites list
    sprites = []
    for i in range(num_map_sprites):
        info = sprite_info_low[i] | (sprite_info_high[i] << 16)
        tile_num = info & 0xFF
        if info & 0x00400000:  # extended tile
            tile_num += 257
        sprites.append({
            'x': sprite_x[i] * 8,
            'y': sprite_y[i] * 8,
            'tile': tile_num,
            'flags': sprite_info_high[i],
        })

    return {
        'spawn_index': spawn_index,
        'spawn_dir': spawn_dir,
        'num_nodes': num_nodes,
        'height_map': height_map,
        'map_media': map_media_indices,
        'geometry': geometry,
        'sprites': sprites,
        'num_lines': num_lines,
    }


def extract_geometry(node_polys, node_offsets, node_child1, tile_mappings):
    """Extract polygons grouped by tile number with positions and UVs."""
    scale = 1.0 / 128.0
    tile_groups = {}  # tileNum -> { verts: [], uvs: [], indices: [] }

    for n in range(len(node_offsets)):
        if (node_offsets[n] & 0xFFFF) != 0xFFFF:
            continue
        offset = node_child1[n] & 0xFFFF
        if offset >= len(node_polys):
            continue

        mesh_count = node_polys[offset]
        offset += 1

        for _ in range(mesh_count):
            if offset + 6 > len(node_polys):
                break

            packed = node_polys[offset + 4] | (node_polys[offset + 5] << 8)
            offset += 6
            tile_num = packed >> 7
            poly_count = packed & 127

            if tile_num not in tile_groups:
                tile_groups[tile_num] = {'verts': [], 'uvs': [], 'indices': []}
            group = tile_groups[tile_num]

            for _ in range(poly_count):
                if offset >= len(node_polys):
                    break

                poly_flags = node_polys[offset]
                offset += 1
                num_v = (poly_flags & POLY_FLAG_VERTS_MASK) + 2
                swap_xy = bool(poly_flags & POLY_FLAG_SWAPXY)

                pverts = []
                for _ in range(num_v):
                    if offset + 5 > len(node_polys):
                        break
                    x = (node_polys[offset] & 0xFF) << 7
                    y = (node_polys[offset + 1] & 0xFF) << 7
                    z = (node_polys[offset + 2] & 0xFF) << 7
                    s = struct.unpack_from('b', node_polys, offset + 3)[0] << 6
                    t = struct.unpack_from('b', node_polys, offset + 4)[0] << 6
                    offset += 5
                    pverts.append({'x': x, 'y': y, 'z': z, 's': s, 't': t})

                # Expand 2-vert to quad
                if len(pverts) == 2:
                    v0, v1 = pverts[0].copy(), pverts[1].copy()
                    v2 = v0.copy()
                    v3 = v0.copy()

                    axis = poly_flags & POLY_FLAG_AXIS_MASK
                    if axis == POLY_FLAG_AXIS_X:
                        v2['x'] = v1['x']
                        v3['x'] = v0['x']
                    elif axis == POLY_FLAG_AXIS_Y:
                        v2['y'] = v1['y']
                        v3['y'] = v0['y']
                    elif axis == POLY_FLAG_AXIS_Z:
                        v2['z'] = v1['z']
                        v3['z'] = v0['z']

                    if poly_flags & POLY_FLAG_UV_DELTAX:
                        v2['s'] = v1['s']
                        v3['s'] = v0['s']
                    else:
                        v2['t'] = v1['t']
                        v3['t'] = v0['t']

                    pverts = [v0, v1, v2, v3]

                # Add vertices
                base = len(group['verts']) // 3
                for v in pverts:
                    if swap_xy:
                        group['verts'].extend([v['y'] * scale, v['z'] * scale, v['x'] * scale])
                    else:
                        group['verts'].extend([v['x'] * scale, v['z'] * scale, v['y'] * scale])
                    # UV: normalize to 0..1 range
                    # Texture coords are in fixed-point (<<6), texture size varies
                    # We store raw coords; the viewer normalizes per-texture
                    group['uvs'].extend([v['s'] / 4096.0, v['t'] / 4096.0])

                # Fan triangulation
                for i in range(1, len(pverts) - 1):
                    group['indices'].extend([base, base + i, base + i + 1])

    return tile_groups


def find_packages(arg=None):
    if arg and os.path.isdir(arg):
        return arg
    candidates = [
        'Doom 2 RPG/Payload/Doom2rpg.app/Packages',
        '../Doom 2 RPG/Payload/Doom2rpg.app/Packages',
    ]
    for c in candidates:
        if os.path.isdir(c):
            return c
    return None


def find_texture_files(tex_dir):
    """Build media_id → filename mapping from extracted textures."""
    result = {}
    if not os.path.isdir(tex_dir):
        return result
    for f in os.listdir(tex_dir):
        if not f.endswith('.png'):
            continue
        try:
            media_id = int(f[:4])
            result[media_id] = f
        except ValueError:
            pass
    return result


def main():
    packages = find_packages(sys.argv[1] if len(sys.argv) > 1 else None)
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'mapdata'
    tex_dir = sys.argv[3] if len(sys.argv) > 3 else 'textures/media'

    if not packages:
        print("Usage: python3 extract_maps.py [Packages_dir] [output_dir] [textures_dir]")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)
    print(f"Packages: {packages}")
    print(f"Output: {output_dir}")
    print(f"Textures: {tex_dir}")

    # Load tile mapping
    tile_map, mappings, dims = load_tile_mapping(packages)
    print(f"Tile mappings: {len(tile_map)} tiles with textures")

    # Find texture files
    tex_files = find_texture_files(tex_dir)
    print(f"Texture PNGs found: {len(tex_files)}")

    # Build tile→texture filename mapping
    tile_textures = {}
    for tile_num, info in tile_map.items():
        media_id = info['media_id']
        if media_id in tex_files:
            tile_textures[tile_num] = {
                'file': tex_files[media_id],
                'media_id': media_id,
                'width': info['width'],
                'height': info['height'],
                'frames': info['frames'],
            }

    # Save tile mapping
    mapping_path = os.path.join(output_dir, 'tile_mapping.json')
    with open(mapping_path, 'w') as f:
        json.dump({
            'tiles': {str(k): v for k, v in tile_textures.items()},
            'texture_dir': tex_dir,
        }, f, indent=2)
    print(f"Written: {mapping_path} ({len(tile_textures)} tile textures)")

    # Process all map and model files
    bin_files = {}
    for path in sorted(glob.glob(os.path.join(packages, '*.bin'))):
        base = os.path.basename(path)
        if base.startswith('map') or base.startswith('model_'):
            with open(path, 'rb') as f:
                bin_files[base] = f.read()

    print(f"\nProcessing {len(bin_files)} files...\n")

    for name, data in sorted(bin_files.items()):
        base = os.path.splitext(name)[0]
        print(f"  {name} ({len(data)} bytes)...", end=' ')

        try:
            result = parse_map(data, tile_map)
            geo = result['geometry']

            # Convert tile_groups keys to strings for JSON
            geo_json = {}
            total_tris = 0
            total_verts = 0
            for tile_num, group in geo.items():
                total_tris += len(group['indices']) // 3
                total_verts += len(group['verts']) // 3
                geo_json[str(tile_num)] = group

            out_path = os.path.join(output_dir, base + '.json')
            output = {
                'name': base,
                'spawn_index': result['spawn_index'],
                'spawn_dir': result['spawn_dir'],
                'height_map': result['height_map'],
                'map_media': result['map_media'],
                'geometry': geo_json,
                'sprites': result['sprites'],
                'stats': {
                    'triangles': total_tris,
                    'vertices': total_verts,
                    'tile_groups': len(geo_json),
                },
            }

            with open(out_path, 'w') as f:
                json.dump(output, f)

            print(f"{total_tris} tris, {total_verts} verts, {len(geo_json)} tile groups")
        except Exception as e:
            print(f"ERROR: {e}")

    print("\nDone!")


if __name__ == '__main__':
    main()
