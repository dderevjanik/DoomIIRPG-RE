#!/usr/bin/env python3
"""
Extract tables.bin from DoomIIRPG .ipa and generate human-readable tables.ini.

Usage:
    python3 tools/extract_tables.py ["Doom 2 RPG.ipa"] [tables.ini]

The script reads tables.bin (gzipped) from the .ipa zip archive, decodes all
14 game data tables, and writes a human-readable INI file.

Binary format:
    Header: 20 x int32 (big-endian) table end-offsets
    Then 14 tables, each starting with int32 count followed by typed data:
        Table  0: MonsterAttacks  - short[count] (stride 9 shorts per monster, 3 parms x 3 fields)
        Table  1: WeaponInfo      - byte[count]  (stride 6 bytes per weapon)
        Table  2: Weapons         - byte[count]  (stride 9 bytes per weapon)
        Table  3: MonsterStats    - byte[count]  (flat array)
        Table  4: CombatMasks     - int32[count] (one per entry)
        Table  5: KeysNumeric     - byte[count]  (flat array)
        Table  6: OSCCycle        - byte[count]  (flat array)
        Table  7: LevelNames      - short[count] (one per level)
        Table  8: MonsterColors   - byte[count]  (stride 3 bytes RGB per monster)
        Table  9: SinTable        - int32[count] (flat array)
        Table 10: EnergyDrinkData - short[count] (flat array)
        Table 11: MonsterWeakness - byte[count]  (one per monster variant)
        Table 12: MovieEffects    - int32[count] (flat array)
        Table 13: MonsterSounds   - byte[count]  (stride 8 bytes per monster)
"""

import gzip
import struct
import sys
import zipfile

# --- Weapon names (indices 0-31) ---
WEAPON_NAMES = [
    "assault_rifle", "chainsaw", "holy_water_pistol", "shooting_sentry_bot",
    "exploding_sentry_bot", "red_shooting_sentry_bot", "red_exploding_sentry_bot",
    "super_shotgun", "chaingun", "assault_rifle_with_scope", "plasma_gun",
    "rocket_launcher", "bfg", "soul_cube", "item",
    "m_bite", "m_claw", "m_punch", "m_charge", "m_flesh_throw",
    "m_fireball", "m_plasma", "m_floor_strike", "m_fire", "m_machine_gun",
    "m_chain_gun", "m_rockets", "m_acid_spit", "m_plasma_gun",
    "m_vios_plasma", "m_vios_lightning", "m_vios_poison",
]

# --- Projectile type names ---
PROJ_TYPE_NAMES = {
    -1: "none", 0: "bullet", 1: "melee", 2: "water", 3: "plasma",
    4: "rocket", 5: "bfg", 6: "flesh", 7: "fire", 8: "caco_plasma",
    9: "thorns", 10: "acid", 11: "electric", 12: "soul_cube", 13: "item",
}

# --- Monster names (indices 0-17) ---
MONSTER_NAMES = [
    "zombie", "zombie_commando", "lost_soul", "imp", "sawcubus", "pinky",
    "cacodemon", "sentinel", "mancubus", "revenant", "arch_vile", "sentry_bot",
    "cyberdemon", "mastermind", "phantom", "archvile_ghost",
    "belphegor", "apollyon",
]


def weapon_name(idx):
    if 0 <= idx < len(WEAPON_NAMES):
        return WEAPON_NAMES[idx]
    return str(idx)


def proj_type_name(val):
    # proj_type is stored as int8_t, so -1 = 0xFF = 255 as unsigned
    signed = val if val < 128 else val - 256
    return PROJ_TYPE_NAMES.get(signed, str(signed))


def monster_name(idx):
    if 0 <= idx < len(MONSTER_NAMES):
        return MONSTER_NAMES[idx]
    return f"monster_{idx}"


class TableReader:
    """Reads big-endian typed data from a byte buffer."""

    def __init__(self, data, offset=0):
        self.data = data
        self.pos = offset

    def read_byte(self):
        val = struct.unpack_from(">b", self.data, self.pos)[0]
        self.pos += 1
        return val

    def read_ubyte(self):
        val = self.data[self.pos]
        self.pos += 1
        return val

    def read_short(self):
        val = struct.unpack_from(">h", self.data, self.pos)[0]
        self.pos += 2
        return val

    def read_int(self):
        val = struct.unpack_from(">i", self.data, self.pos)[0]
        self.pos += 4
        return val


def extract_tables(ipa_path, output_path):
    """Extract tables.bin from .ipa and write tables.ini."""
    prefix = "Payload/Doom2rpg.app/Packages/"

    with zipfile.ZipFile(ipa_path) as ipa:
        raw = ipa.read(prefix + "tables.bin")
        data = gzip.decompress(raw)

    print(f"tables.bin: {len(raw)} compressed, {len(data)} decompressed")

    # Parse 20 x int32 offsets header
    offsets = []
    for i in range(20):
        offsets.append(struct.unpack_from(">i", data, i * 4)[0])
    print(f"Table offsets: {offsets[:14]}")

    # Data starts after the 80-byte header
    data_start = 80

    def table_range(index):
        """Return (start, end) byte offsets relative to data_start for table index."""
        start = 0 if index == 0 else offsets[index - 1]
        end = offsets[index]
        return start, end

    def get_num_bytes(index):
        start, end = table_range(index)
        return end - start - 4  # subtract 4 for the count int

    lines = []

    def w(s=""):
        lines.append(s)

    # === Table 2: Weapons (byte table, stride 9) ===
    w("; Weapon definitions: str_min, str_max, range_min, range_max, ammo_type, ammo_usage, proj_type, num_shots, shot_hold")
    w("[Weapons]")
    start, end = table_range(2)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    num_weapons = count // 9
    w(f"count = {num_weapons}")
    w()
    for i in range(num_weapons):
        w(f"; {weapon_name(i)}")
        w(f"[Weapon_{i}]")
        str_min = r.read_byte()
        str_max = r.read_byte()
        range_min = r.read_byte()
        range_max = r.read_byte()
        ammo_type = r.read_byte()
        ammo_usage = r.read_byte()
        pt = r.read_byte()
        num_shots = r.read_byte()
        shot_hold = r.read_byte()
        w(f"str_min = {str_min}")
        w(f"str_max = {str_max}")
        w(f"range_min = {range_min}")
        w(f"range_max = {range_max}")
        w(f"ammo_type = {ammo_type}")
        w(f"ammo_usage = {ammo_usage}")
        pt_signed = pt if pt < 128 else pt - 256
        w(f"proj_type = {PROJ_TYPE_NAMES.get(pt_signed, str(pt_signed))}")
        w(f"num_shots = {num_shots}")
        w(f"shot_hold = {shot_hold}")
        w()

    # === Table 1: WeaponInfo (byte table, stride 6) ===
    w("; Weapon display positions: idle_x, idle_y, attack_x, attack_y, flash_x, flash_y")
    w("[WeaponInfo]")
    start, end = table_range(1)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    num_wpinfo = count // 6
    w(f"count = {num_wpinfo}")
    w()
    for i in range(num_wpinfo):
        w(f"; {weapon_name(i)}")
        w(f"[WeaponInfo_{i}]")
        w(f"idle_x = {r.read_byte()}")
        w(f"idle_y = {r.read_byte()}")
        w(f"attack_x = {r.read_byte()}")
        w(f"attack_y = {r.read_byte()}")
        w(f"flash_x = {r.read_byte()}")
        w(f"flash_y = {r.read_byte()}")
        w()

    # === Table 0: MonsterAttacks (short table, stride 9) ===
    w("; Monster attack definitions: for each parm variant: attack1_weapon, attack2_weapon, attack2_chance")
    w("[MonsterAttacks]")
    start, end = table_range(0)
    r = TableReader(data, data_start + start)
    count = r.read_int()  # count of shorts
    num_monster_attacks = count // 9
    w(f"count = {num_monster_attacks}")
    w()
    for i in range(num_monster_attacks):
        w(f"; {monster_name(i)}")
        w(f"[MonsterAttack_{i}]")
        for p in range(3):
            atk1 = r.read_short()
            atk2 = r.read_short()
            chance = r.read_short()
            w(f"parm{p}_attack1 = {weapon_name(atk1)}")
            w(f"parm{p}_attack2 = {weapon_name(atk2)}")
            w(f"parm{p}_chance = {chance}")
        w()

    # === Table 3: MonsterStats (byte table, flat) ===
    w("[MonsterStats]")
    start, end = table_range(3)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    w(f"count = {count}")
    vals = [str(r.read_byte()) for _ in range(count)]
    w(f"data = {','.join(vals)}")
    w()

    # === Table 4: CombatMasks (int table) ===
    w("; Combat rendering masks per weapon")
    w("[CombatMasks]")
    start, end = table_range(4)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    w(f"count = {count}")
    for i in range(count):
        val = r.read_int()
        w(f"; {weapon_name(i)}")
        w(f"mask_{i} = 0x{val & 0xFFFFFFFF:08X}")
    w()

    # === Table 11: MonsterWeakness (byte table) ===
    w("; Monster weakness flags per monster subtype")
    w("[MonsterWeakness]")
    start, end = table_range(11)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    w(f"count = {count}")
    for i in range(count):
        val = r.read_byte()
        key = monster_name(i) if i < len(MONSTER_NAMES) else f"monster_{i}"
        w(f"{key} = {val}")
    w()

    # === Table 13: MonsterSounds (byte table, stride 8) ===
    w("[MonsterSounds]")
    start, end = table_range(13)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    num_monster_sounds = count // 8
    w(f"count = {num_monster_sounds}")
    w()
    sound_fields = ["alert1", "alert2", "alert3", "attack1", "attack2", "idle", "pain", "death"]
    for i in range(num_monster_sounds):
        w(f"; {monster_name(i)}")
        w(f"[MonsterSound_{i}]")
        for f_idx, field in enumerate(sound_fields):
            val = r.read_ubyte()
            w(f"{field} = {'none' if val == 255 else str(val)}")
        w()

    # === Table 8: MonsterColors (byte table, stride 3) ===
    w("[MonsterColors]")
    start, end = table_range(8)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    num_colors = count // 3
    w(f"count = {count}")
    for i in range(num_colors):
        cr = r.read_ubyte()
        cg = r.read_ubyte()
        cb = r.read_ubyte()
        key = monster_name(i) if i < len(MONSTER_NAMES) else f"monster_{i}"
        w(f"{key} = {cr},{cg},{cb}")
    w()

    # === Table 10: EnergyDrinkData (short table, flat) ===
    w("; Vending machine energy drink data")
    w("[EnergyDrinkData]")
    start, end = table_range(10)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    w(f"count = {count}")
    vals = [str(r.read_short()) for _ in range(count)]
    w(f"data = {','.join(vals)}")
    w()

    # === Table 7: LevelNames (short table) ===
    w("; Level name string indices")
    w("[LevelNames]")
    start, end = table_range(7)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    w(f"count = {count}")
    for i in range(count):
        w(f"level_{i} = {r.read_short()}")
    w()

    # === Table 5: KeysNumeric (byte table, flat) ===
    w("; Key numeric mappings")
    w("[KeysNumeric]")
    start, end = table_range(5)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    w(f"count = {count}")
    vals = [str(r.read_byte()) for _ in range(count)]
    w(f"data = {','.join(vals)}")
    w()

    # === Table 6: OSCCycle (byte table, flat) ===
    w("; On-screen control cycle")
    w("[OSCCycle]")
    start, end = table_range(6)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    w(f"count = {count}")
    vals = [str(r.read_byte()) for _ in range(count)]
    w(f"data = {','.join(vals)}")
    w()

    # === Table 9: SinTable (int table, flat) ===
    w("; Fixed-point sine lookup table")
    w("[SinTable]")
    start, end = table_range(9)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    w(f"count = {count}")
    vals = [str(r.read_int()) for _ in range(count)]
    w(f"data = {','.join(vals)}")
    w()

    # === Table 12: MovieEffects (int table, flat) ===
    w("; Cinematic effect data")
    w("[MovieEffects]")
    start, end = table_range(12)
    r = TableReader(data, data_start + start)
    count = r.read_int()
    w(f"count = {count}")
    vals = [str(r.read_int()) for _ in range(count)]
    w(f"data = {','.join(vals)}")
    w()

    # Write to file
    with open(output_path, "w") as f:
        f.write("\n".join(lines))

    total_lines = len(lines)
    print(f"Written {output_path}: {total_lines} lines, 14 tables")


if __name__ == "__main__":
    ipa_path = sys.argv[1] if len(sys.argv) > 1 else "Doom 2 RPG.ipa"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "tables.ini"
    extract_tables(ipa_path, output_path)
