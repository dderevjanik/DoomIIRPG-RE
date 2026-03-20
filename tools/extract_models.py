#!/usr/bin/env python3
"""
extract_models.py - Extract 3D geometry from DoomIIRPG model/map .bin files to OBJ format.

Both model_XXXX.bin and mapXX.bin use the same BSP-based binary format.
This extracts the polygon geometry from the nodePolys section.

Usage:
    python3 extract_models.py [path/to/ipa_or_packages_dir] [output_dir]

    If no arguments given, looks for .ipa or Packages directory in standard locations.

Output:
    One .obj file per input .bin file (e.g. model_0000.obj, map00.obj)
    Optionally a .mtl file referencing texture IDs
"""

import struct
import sys
import os
import zipfile
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


class BinReader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def read_bytes(self, n):
        result = self.data[self.pos:self.pos + n]
        self.pos += n
        return result

    def read_ubyte(self):
        val = self.data[self.pos]
        self.pos += 1
        return val

    def read_byte(self):
        val = struct.unpack_from('<b', self.data, self.pos)[0]
        self.pos += 1
        return val

    def read_ushort(self):
        val = struct.unpack_from('<H', self.data, self.pos)[0]
        self.pos += 2
        return val

    def read_short(self):
        val = struct.unpack_from('<h', self.data, self.pos)[0]
        self.pos += 2
        return val

    def read_int(self):
        val = struct.unpack_from('<i', self.data, self.pos)[0]
        self.pos += 4
        return val

    def read_uint(self):
        val = struct.unpack_from('<I', self.data, self.pos)[0]
        self.pos += 4
        return val

    def read_marker(self):
        return self.read_uint()

    def expect_marker(self, expected=None):
        m = self.read_marker()
        if expected and m != expected:
            raise ValueError(f"Expected marker {expected:#010x} at offset {self.pos - 4}, got {m:#010x}")
        return m


def parse_level_bin(data):
    """Parse a model/map .bin file and extract polygon geometry."""
    r = BinReader(data)

    # Header (42 bytes)
    version = r.read_ubyte()
    if version != 3:
        raise ValueError(f"Bad version: {version}, expected 3")

    compile_date = r.read_uint()
    spawn_index = r.read_ushort()
    spawn_dir = r.read_ubyte()
    flags_bitmask = r.read_byte()
    total_secrets = r.read_byte()
    total_loot = r.read_ubyte()
    num_nodes = r.read_ushort()
    data_size_polys = r.read_ushort()
    num_lines = r.read_ushort()
    num_normals = r.read_ushort()
    num_normal_sprites = r.read_ushort()
    num_z_sprites = r.read_short()
    num_tile_events = r.read_short()
    map_bytecode_size = r.read_short()
    total_maya_cameras = r.read_ubyte()
    total_maya_camera_keys = r.read_short()
    maya_tween_offsets = [r.read_short() for _ in range(6)]

    num_map_sprites = num_normal_sprites + num_z_sprites

    print(f"  Nodes: {num_nodes}, PolyData: {data_size_polys}B, Lines: {num_lines}, Normals: {num_normals}")
    print(f"  Sprites: {num_normal_sprites}+{num_z_sprites}z, Events: {num_tile_events}")

    # Section 1: Media
    r.expect_marker(DEADBEEF)
    media_count = r.read_ushort()
    media_indices = [r.read_ushort() for _ in range(media_count)]
    r.expect_marker(DEADBEEF)

    # Section 2: Normals
    normals = []
    for _ in range(num_normals):
        nx = r.read_short()
        ny = r.read_short()
        nz = r.read_short()
        normals.append((nx, ny, nz))
    r.expect_marker(CAFEBABE)

    # Section 3: Node offsets
    node_offsets = [r.read_short() for _ in range(num_nodes)]
    r.expect_marker(CAFEBABE)

    # Section 4: Node normal indices
    node_normal_idxs = [r.read_ubyte() for _ in range(num_nodes)]
    r.expect_marker(CAFEBABE)

    # Section 5: Node child offsets
    node_child_offset1 = [r.read_short() for _ in range(num_nodes)]
    node_child_offset2 = [r.read_short() for _ in range(num_nodes)]
    r.expect_marker(CAFEBABE)

    # Section 6: Node bounds
    node_bounds = r.read_bytes(num_nodes * 4)
    r.expect_marker(CAFEBABE)

    # Section 7: Node polygons (the geometry data!)
    node_polys = r.read_bytes(data_size_polys)
    r.expect_marker(CAFEBABE)

    # Extract polygons from nodePolys
    all_polygons = extract_polygons(node_polys, node_offsets, node_child_offset1, media_indices)

    return {
        'polygons': all_polygons,
        'normals': normals,
        'media_indices': media_indices,
        'num_nodes': num_nodes,
    }


def extract_polygons(node_polys, node_offsets, node_child_offset1, media_indices):
    """Extract polygon geometry from nodePolys data, mirroring drawNodeGeometry()."""
    all_polygons = []

    for n in range(len(node_offsets)):
        # Only process leaf nodes (offset == 0xFFFF means leaf with geometry)
        if (node_offsets[n] & 0xFFFF) != 0xFFFF:
            continue

        offset = node_child_offset1[n] & 0xFFFF
        if offset >= len(node_polys):
            continue

        mesh_count = node_polys[offset]
        offset += 1

        for _ in range(mesh_count):
            if offset + 6 > len(node_polys):
                break

            # Read mesh header (6 bytes)
            # Bytes 4-5: packed ushort with media ref (upper 9 bits) and poly count (lower 7 bits)
            header_bytes = node_polys[offset:offset + 6]
            packed = header_bytes[4] | (header_bytes[5] << 8)
            offset += 6

            media_ref = packed >> 7
            poly_count = packed & 127

            for _ in range(poly_count):
                if offset >= len(node_polys):
                    break

                poly_flags = node_polys[offset]
                offset += 1
                num_verts = (poly_flags & POLY_FLAG_VERTS_MASK) + 2

                verts = []
                for _ in range(num_verts):
                    if offset + 5 > len(node_polys):
                        break
                    x = (node_polys[offset + 0] & 0xFF) << 7
                    y = (node_polys[offset + 1] & 0xFF) << 7
                    z = (node_polys[offset + 2] & 0xFF) << 7
                    s = struct.unpack_from('b', node_polys, offset + 3)[0] << 6
                    t = struct.unpack_from('b', node_polys, offset + 4)[0] << 6
                    offset += 5
                    verts.append({'x': x, 'y': y, 'z': z, 's': s, 't': t})

                # Expand 2-vertex edges to quads (same logic as drawNodeGeometry)
                if len(verts) == 2:
                    v0, v1 = verts[0].copy(), verts[1].copy()
                    v2 = v0.copy()
                    v3 = v0.copy()

                    axis = poly_flags & POLY_FLAG_AXIS_MASK
                    if axis == POLY_FLAG_AXIS_X:
                        v2['x'] = v1['x']
                        v3['x'] = v0['x']
                        # v1 keeps its x, v3 gets v0's x
                        # Actually: mv[1].x = mv[2].x; mv[3].x = mv[0].x
                        # After the memcpy shuffle: v0, v2(=v1), then expand
                        pass
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

                    # Rebuild quad: the C++ code does a memcpy shuffle
                    # Original: [v0, v1] -> after shuffle: [v0, v1_copy, v0_copy, v0_copy]
                    # Then axis expansion modifies positions
                    # Final quad order: v0, v1, v2, v3
                    verts = [v0, v1, v2, v3]

                swap_xy = bool(poly_flags & POLY_FLAG_SWAPXY)

                all_polygons.append({
                    'verts': verts,
                    'media_ref': media_ref,
                    'flags': poly_flags,
                    'swap_xy': swap_xy,
                })

    return all_polygons


def write_obj(polygons, output_path, scale=1.0 / 128.0):
    """Write extracted polygons to OBJ format."""
    # Group polygons by media_ref for material assignment
    groups = {}
    for poly in polygons:
        ref = poly['media_ref']
        if ref not in groups:
            groups[ref] = []
        groups[ref].append(poly)

    mtl_name = os.path.splitext(os.path.basename(output_path))[0]
    mtl_path = os.path.splitext(output_path)[0] + '.mtl'

    verts = []
    uvs = []
    faces = []  # list of (group_name, face_vert_indices, face_uv_indices)

    vert_idx = 1  # OBJ is 1-indexed
    uv_idx = 1

    for ref in sorted(groups.keys()):
        group_name = f"media_{ref}"
        for poly in groups[ref]:
            face_v = []
            face_vt = []
            for v in poly['verts']:
                if poly['swap_xy']:
                    verts.append((v['y'] * scale, v['z'] * scale, v['x'] * scale))
                else:
                    verts.append((v['x'] * scale, v['z'] * scale, v['y'] * scale))
                # Normalize UVs (texture coords are in fixed-point << 6, texture is 64x64)
                uvs.append((v['s'] / 4096.0, v['t'] / 4096.0))
                face_v.append(vert_idx)
                face_vt.append(uv_idx)
                vert_idx += 1
                uv_idx += 1
            faces.append((group_name, face_v, face_vt))

    with open(output_path, 'w') as f:
        f.write(f"# DoomIIRPG Model Export\n")
        f.write(f"# Polygons: {len(polygons)}, Vertices: {len(verts)}\n")
        f.write(f"mtllib {mtl_name}.mtl\n\n")

        for v in verts:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        f.write("\n")

        for uv in uvs:
            f.write(f"vt {uv[0]:.6f} {uv[1]:.6f}\n")
        f.write("\n")

        current_group = None
        for group_name, face_v, face_vt in faces:
            if group_name != current_group:
                f.write(f"\ng {group_name}\n")
                f.write(f"usemtl {group_name}\n")
                current_group = group_name

            indices = " ".join(f"{vi}/{ti}" for vi, ti in zip(face_v, face_vt))
            f.write(f"f {indices}\n")

    # Write a basic MTL file
    with open(mtl_path, 'w') as f:
        f.write(f"# DoomIIRPG Materials\n")
        for ref in sorted(groups.keys()):
            # Assign distinct colors per material group
            r = ((ref * 37) % 256) / 255.0
            g = ((ref * 73 + 100) % 256) / 255.0
            b = ((ref * 113 + 50) % 256) / 255.0
            f.write(f"\nnewmtl media_{ref}\n")
            f.write(f"Kd {r:.3f} {g:.3f} {b:.3f}\n")
            f.write(f"Ka 0.1 0.1 0.1\n")

    print(f"  Written: {output_path} ({len(polygons)} polygons, {len(verts)} vertices)")
    print(f"  Written: {mtl_path} ({len(groups)} materials)")


def write_json(polygons, output_path, scale=1.0 / 128.0):
    """Write extracted polygons to a simple JSON format for the HTML viewer."""
    import json

    meshes = {}
    for poly in polygons:
        ref = poly['media_ref']
        if ref not in meshes:
            meshes[ref] = {'vertices': [], 'uvs': [], 'indices': []}
        mesh = meshes[ref]
        base = len(mesh['vertices']) // 3

        for v in poly['verts']:
            if poly['swap_xy']:
                mesh['vertices'].extend([v['y'] * scale, v['z'] * scale, v['x'] * scale])
            else:
                mesh['vertices'].extend([v['x'] * scale, v['z'] * scale, v['y'] * scale])
            mesh['uvs'].extend([v['s'] / 4096.0, v['t'] / 4096.0])

        # Triangulate polygon (fan from first vertex)
        n = len(poly['verts'])
        for i in range(1, n - 1):
            mesh['indices'].extend([base, base + i, base + i + 1])

    output = {
        'meshes': {str(k): v for k, v in meshes.items()},
        'polygon_count': len(polygons),
        'vertex_count': sum(len(m['vertices']) // 3 for m in meshes.values()),
    }

    with open(output_path, 'w') as f:
        json.dump(output, f)

    print(f"  Written: {output_path} (JSON, {len(meshes)} meshes)")


def find_packages_dir(arg=None):
    """Find the Packages directory from CLI arg, IPA, or default locations."""
    if arg and os.path.isdir(arg):
        return arg

    if arg and arg.endswith('.ipa') and os.path.isfile(arg):
        return arg  # Will be handled as zip

    # Default search paths
    candidates = [
        'Doom 2 RPG/Payload/Doom2rpg.app/Packages',
        '../Doom 2 RPG/Payload/Doom2rpg.app/Packages',
    ]
    for c in candidates:
        if os.path.isdir(c):
            return c

    return None


def load_bin_files(source):
    """Load all model_XXXX.bin and mapXX.bin files from source."""
    files = {}

    if source and source.endswith('.ipa') and os.path.isfile(source):
        with zipfile.ZipFile(source) as zf:
            for name in zf.namelist():
                base = os.path.basename(name)
                if base.startswith('model_') and base.endswith('.bin'):
                    files[base] = zf.read(name)
                elif base.startswith('map') and base.endswith('.bin'):
                    files[base] = zf.read(name)
    elif source and os.path.isdir(source):
        for path in sorted(glob.glob(os.path.join(source, '*.bin'))):
            base = os.path.basename(path)
            if base.startswith('model_') or base.startswith('map'):
                with open(path, 'rb') as f:
                    files[base] = f.read()

    return files


def main():
    source = sys.argv[1] if len(sys.argv) > 1 else None
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'models'

    pkg_dir = find_packages_dir(source)
    if not pkg_dir:
        print("Usage: python3 extract_models.py [path/to/ipa_or_packages_dir] [output_dir]")
        print("\nCould not find Packages directory. Provide path to:")
        print("  - The .ipa file")
        print("  - The Packages directory")
        sys.exit(1)

    print(f"Source: {pkg_dir}")
    print(f"Output: {output_dir}")

    os.makedirs(output_dir, exist_ok=True)

    files = load_bin_files(pkg_dir)
    if not files:
        print("No model/map .bin files found!")
        sys.exit(1)

    print(f"\nFound {len(files)} files\n")

    for name, data in sorted(files.items()):
        base = os.path.splitext(name)[0]
        print(f"Processing {name} ({len(data)} bytes)...")

        try:
            result = parse_level_bin(data)
            polygons = result['polygons']

            if polygons:
                obj_path = os.path.join(output_dir, base + '.obj')
                json_path = os.path.join(output_dir, base + '.json')
                write_obj(polygons, obj_path)
                write_json(polygons, json_path)
            else:
                print(f"  No polygons extracted")
        except Exception as e:
            print(f"  ERROR: {e}")

        print()


if __name__ == '__main__':
    main()
