#!/usr/bin/env python3
"""Generate standalone demo game assets for DoomIIRPG-RE engine.

Usage:
    python3 tools/generate_demo.py [output_dir]

Default output: games/demo/
Run the demo: ./build/DoomIIRPG --game demo
"""

import struct
import os
import sys
import math

OUTPUT_DIR = sys.argv[1] if len(sys.argv) > 1 else "games/demo"

# --- Constants ---
DEADBEEF = struct.pack("<I", 0xDEADBEEF)
CAFEBABE = struct.pack("<I", 0xCAFEBABE)
MAGENTA = (255, 0, 255)

# ============================================================================
# BMP Generation
# ============================================================================

def make_bmp_8bit(width, height, pixels, palette):
    """Create an 8-bit indexed BMP file.

    Args:
        width: Image width in pixels
        height: Image height in pixels
        pixels: List of palette indices, row-major top-to-bottom (width*height)
        palette: List of (R,G,B) tuples, up to 256 entries
    Returns:
        bytes: Complete BMP file data
    """
    num_colors = 256
    row_stride = (width + 3) & ~3  # Pad rows to 4-byte boundary
    pixel_data_size = row_stride * height
    palette_size = num_colors * 4
    header_size = 14 + 40  # BITMAPFILEHEADER + BITMAPINFOHEADER
    file_size = header_size + palette_size + pixel_data_size
    data_offset = header_size + palette_size

    # BITMAPFILEHEADER (14 bytes)
    bfh = struct.pack("<2sIHHI", b"BM", file_size, 0, 0, data_offset)

    # BITMAPINFOHEADER (40 bytes)
    bih = struct.pack("<IiiHHIIiiII",
        40,             # biSize
        width,          # biWidth
        height,         # biHeight (positive = bottom-up)
        1,              # biPlanes
        8,              # biBitCount
        0,              # biCompression (BI_RGB)
        pixel_data_size,# biSizeImage
        0, 0,           # biXPelsPerMeter, biYPelsPerMeter
        num_colors,     # biClrUsed
        0               # biClrImportant
    )

    # Color table: BGRA format
    pal_data = bytearray(palette_size)
    for i, (r, g, b) in enumerate(palette):
        if i >= num_colors:
            break
        pal_data[i * 4 + 0] = b
        pal_data[i * 4 + 1] = g
        pal_data[i * 4 + 2] = r
        pal_data[i * 4 + 3] = 0

    # Pixel data: bottom-up rows
    pix_data = bytearray(pixel_data_size)
    for y in range(height):
        src_row = y
        dst_row = height - 1 - y  # BMP is bottom-up
        for x in range(width):
            idx = src_row * width + x
            pix_data[dst_row * row_stride + x] = pixels[idx] if idx < len(pixels) else 0

    return bfh + bih + bytes(pal_data) + bytes(pix_data)


def make_solid_bmp(width, height, r, g, b):
    """Create a solid-color 8-bit BMP."""
    palette = [MAGENTA, (r, g, b)] + [(0, 0, 0)] * 254
    pixels = [1] * (width * height)
    return make_bmp_8bit(width, height, pixels, palette)


def make_labeled_bmp(width, height, r, g, b, label=None, scale=1):
    """Create a solid-color 8-bit BMP with a text label rendered on it.

    The label is drawn centered using FONT_5X7 glyphs.
    scale: integer multiplier for glyph pixels (1=5x7, 2=10x14, etc.)
    """
    if label is None or width < 10 or height < 10:
        return make_solid_bmp(width, height, r, g, b)

    # Palette: 0=magenta(transparent), 1=bg color, 2=text color (brighter/contrasting)
    # Make text color: brighten the bg by ~100 or use white if already bright
    tr = min(255, r + 120)
    tg = min(255, g + 120)
    tb = min(255, b + 120)
    palette = [MAGENTA, (r, g, b), (tr, tg, tb)] + [(0, 0, 0)] * 253
    pixels = [1] * (width * height)  # fill with bg color (index 1)

    glyph_w, glyph_h = 5 * scale, 7 * scale

    # Word-wrap: split label into lines that fit within width
    max_chars_per_line = max(1, (width - 4) // (glyph_w + scale))
    words = label.replace('_', ' ').replace('-', ' ').split()
    lines = []
    current_line = ""
    for word in words:
        if not current_line:
            current_line = word
        elif len(current_line) + 1 + len(word) <= max_chars_per_line:
            current_line += " " + word
        else:
            lines.append(current_line)
            current_line = word
    if current_line:
        lines.append(current_line)

    # Truncate lines that still don't fit
    for i in range(len(lines)):
        if len(lines[i]) > max_chars_per_line:
            lines[i] = lines[i][:max_chars_per_line]

    line_spacing = glyph_h + scale
    total_text_h = len(lines) * line_spacing - scale
    start_y = max(1, (height - total_text_h) // 2)

    for line_idx, line in enumerate(lines):
        text_w = len(line) * (glyph_w + scale) - scale
        start_x = max(1, (width - text_w) // 2)
        ly = start_y + line_idx * line_spacing

        for ci, ch in enumerate(line):
            glyph = FONT_5X7.get(ch, FONT_5X7.get(' ', [0] * 7))
            cx = start_x + ci * (glyph_w + scale)

            for gy in range(7):
                row_bits = glyph[gy]
                for gx in range(5):
                    if row_bits & (1 << (4 - gx)):
                        # Draw scaled pixel
                        for sy in range(scale):
                            for sx in range(scale):
                                px = cx + gx * scale + sx
                                py = ly + gy * scale + sy
                                if 0 <= px < width and 0 <= py < height:
                                    pixels[py * width + px] = 2

    return make_bmp_8bit(width, height, pixels, palette)


def make_transparent_bmp(width, height):
    """Create a fully transparent 8-bit BMP (all magenta)."""
    palette = [MAGENTA] + [(0, 0, 0)] * 255
    pixels = [0] * (width * height)
    return make_bmp_8bit(width, height, pixels, palette)


# ============================================================================
# Font BMP Generation
# ============================================================================

# Minimal 5x7 pixel font for ASCII 32-127
FONT_5X7 = {}

def _init_font():
    """Initialize a basic 5x7 pixel font for printable ASCII."""
    # Each glyph is 5 columns x 7 rows, stored as list of 7 ints (each 5 bits wide)
    # Bit 4=leftmost pixel, bit 0=rightmost
    glyphs = {
        ' ': [0b00000]*7,
        '!': [0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100],
        '"': [0b01010, 0b01010, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000],
        '#': [0b01010, 0b11111, 0b01010, 0b01010, 0b11111, 0b01010, 0b00000],
        '$': [0b00100, 0b01111, 0b10100, 0b01110, 0b00101, 0b11110, 0b00100],
        '%': [0b11001, 0b11010, 0b00100, 0b00100, 0b01000, 0b10110, 0b10011],
        '&': [0b01100, 0b10010, 0b01100, 0b01001, 0b10110, 0b10010, 0b01101],
        "'": [0b00100, 0b00100, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000],
        '(': [0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010],
        ')': [0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000],
        '*': [0b00000, 0b00100, 0b10101, 0b01110, 0b10101, 0b00100, 0b00000],
        '+': [0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000],
        ',': [0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b01000],
        '-': [0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000],
        '.': [0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100],
        '/': [0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000],
        '0': [0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110],
        '1': [0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110],
        '2': [0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111],
        '3': [0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110],
        '4': [0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010],
        '5': [0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110],
        '6': [0b01110, 0b10000, 0b11110, 0b10001, 0b10001, 0b10001, 0b01110],
        '7': [0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000],
        '8': [0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110],
        '9': [0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110],
        ':': [0b00000, 0b00100, 0b00000, 0b00000, 0b00000, 0b00100, 0b00000],
        ';': [0b00000, 0b00100, 0b00000, 0b00000, 0b00000, 0b00100, 0b01000],
        '<': [0b00010, 0b00100, 0b01000, 0b10000, 0b01000, 0b00100, 0b00010],
        '=': [0b00000, 0b00000, 0b11111, 0b00000, 0b11111, 0b00000, 0b00000],
        '>': [0b10000, 0b01000, 0b00100, 0b00010, 0b00100, 0b01000, 0b10000],
        '?': [0b01110, 0b10001, 0b00001, 0b00110, 0b00100, 0b00000, 0b00100],
        '@': [0b01110, 0b10001, 0b10111, 0b10101, 0b10111, 0b10000, 0b01110],
    }
    # A-Z
    alpha = [
        [0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001],  # A
        [0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110],  # B
        [0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110],  # C
        [0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110],  # D
        [0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111],  # E
        [0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000],  # F
        [0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110],  # G
        [0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001],  # H
        [0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110],  # I
        [0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100],  # J
        [0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001],  # K
        [0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111],  # L
        [0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001],  # M
        [0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001],  # N
        [0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110],  # O
        [0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000],  # P
        [0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101],  # Q
        [0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001],  # R
        [0b01110, 0b10001, 0b10000, 0b01110, 0b00001, 0b10001, 0b01110],  # S
        [0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100],  # T
        [0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110],  # U
        [0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b01010, 0b00100],  # V
        [0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001],  # W
        [0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b01010, 0b10001],  # X
        [0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100],  # Y
        [0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111],  # Z
    ]
    for i, g in enumerate(alpha):
        glyphs[chr(ord('A') + i)] = g

    # a-z (same as uppercase for simplicity)
    for i in range(26):
        glyphs[chr(ord('a') + i)] = alpha[i]

    # Fill remaining printable chars with empty
    for c in range(32, 128):
        ch = chr(c)
        if ch not in glyphs:
            glyphs[ch] = [0b00000] * 7

    FONT_5X7.update(glyphs)

_init_font()


def generate_hud_numbers_bmp():
    """Generate Hud_Numbers.bmp — vertical strip of digits 9..0, each cell 10x20.

    The engine indexes as: drawRegion(img, 0, (9-digit)*20, 10, 20, ...)
    So row 0 = digit 9, row 1 = digit 8, ..., row 9 = digit 0.
    """
    cell_w, cell_h = 10, 20
    sheet_w = cell_w
    sheet_h = cell_h * 10

    fg = (220, 220, 220)
    palette = [MAGENTA, fg] + [(0, 0, 0)] * 254
    pixels = [0] * (sheet_w * sheet_h)

    pad_x = max(1, (cell_w - 5) // 2)
    pad_y = max(1, (cell_h - 7) // 2)

    for row in range(10):
        digit = 9 - row
        glyph = FONT_5X7.get(str(digit), [0] * 7)
        base_y = row * cell_h

        for gy in range(7):
            row_bits = glyph[gy]
            for gx in range(5):
                if row_bits & (1 << (4 - gx)):
                    px = pad_x + gx
                    py = base_y + pad_y + gy
                    if px < sheet_w and py < sheet_h:
                        pixels[py * sheet_w + px] = 1

    return make_bmp_8bit(sheet_w, sheet_h, pixels, palette)


def generate_font_bmp(cell_w, cell_h, fg_rgb, bg_rgb=MAGENTA):
    """Generate a font sprite sheet BMP.

    The engine's drawChar() indexes into a 16-column grid:
      texX = (index & 15) * cell_w
      texY = index & 0xF0  (for 16px-tall rows, this equals (index // 16) * 16)
    where index = char - '!'.

    For the standard Font.bmp: cell_w=12, cell_h=16, so the sheet is 192 x 160.
    Max index ~143 → 9 rows.
    """
    num_cols = 16
    num_rows = 10  # indices 0..159, covers all needed chars
    sheet_w = cell_w * num_cols
    sheet_h = cell_h * num_rows

    palette = [bg_rgb, fg_rgb] + [(0, 0, 0)] * 254
    pixels = [0] * (sheet_w * sheet_h)

    glyph_w, glyph_h = 5, 7
    pad_x = max(1, (cell_w - glyph_w) // 2)
    pad_y = max(1, (cell_h - glyph_h) // 2)

    for char_idx in range(32, 128):
        index1 = char_idx - ord('!')
        if index1 < 0 or index1 >= num_cols * num_rows:
            continue

        col = index1 % num_cols
        row = index1 // num_cols
        base_x = col * cell_w
        base_y = row * cell_h

        ch = chr(char_idx)
        glyph = FONT_5X7.get(ch, [0] * 7)

        for gy in range(glyph_h):
            if pad_y + gy >= cell_h:
                break
            row_bits = glyph[gy]
            for gx in range(glyph_w):
                if pad_x + gx >= cell_w:
                    break
                if row_bits & (1 << (4 - gx)):
                    px = base_x + pad_x + gx
                    py = base_y + pad_y + gy
                    if px < sheet_w and py < sheet_h:
                        pixels[py * sheet_w + px] = 1

    return make_bmp_8bit(sheet_w, sheet_h, pixels, palette)


# ============================================================================
# All BMP files
# ============================================================================

def generate_all_bmps(out_dir):
    """Generate all BMP files required by the engine."""
    print("  Generating BMP images...")

    def save(name, data):
        path = os.path.join(out_dir, name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(data)

    def lbmp(name, w, h, r, g, b, label=None, scale=None):
        """Save a labeled BMP. Auto-derives label from filename and picks scale."""
        if label is None:
            label = name.replace(".bmp", "").split("/")[-1]
        if scale is None:
            # Pick scale based on smallest dimension
            mindim = min(w, h)
            if mindim >= 200:
                scale = 3
            elif mindim >= 80:
                scale = 2
            else:
                scale = 1
        save(name, make_labeled_bmp(w, h, r, g, b, label, scale))

    # --- Fonts (these are sprite sheets, not labeled) ---
    save("Font.bmp", generate_font_bmp(12, 16, (200, 200, 200)))
    save("Font_16p_Light.bmp", generate_font_bmp(12, 16, (220, 220, 220)))
    save("Font_16p_Dark.bmp", generate_font_bmp(12, 16, (60, 60, 60)))
    save("Font_18p_Light.bmp", generate_font_bmp(13, 18, (220, 220, 220)))
    save("WarFont.bmp", generate_font_bmp(22, 25, (255, 50, 50)))
    save("Hud_Numbers.bmp", generate_hud_numbers_bmp())

    # --- Canvas startup ---
    lbmp("DialogScroll.bmp", 480, 40, 30, 30, 60)
    lbmp("FabricBG.bmp", 480, 320, 70, 60, 50)
    lbmp("endOfLevelStatsBG.bmp", 480, 320, 60, 60, 60)
    lbmp("gameHelpBG.bmp", 480, 320, 50, 50, 70)
    save("Icons_Buffs.bmp", make_transparent_bmp(128, 64))
    lbmp("inventoryBG.bmp", 480, 320, 45, 45, 65)
    lbmp("loadingFire.bmp", 32, 128, 200, 100, 20, "fire")
    save("cockpit.bmp", make_transparent_bmp(480, 320))

    # --- Arrow buttons ---
    arrow_files = [
        "arrow-up.bmp", "greenArrow_up.bmp", "arrow-up_pressed.bmp", "greenArrow_up-pressed.bmp",
        "arrow-down.bmp", "greenArrow_down.bmp", "arrow-down_pressed.bmp", "greenArrow_down-pressed.bmp",
        "arrow-left.bmp", "greenArrow_left.bmp", "arrow-left_pressed.bmp", "greenArrow_left-pressed.bmp",
        "arrow-right.bmp", "greenArrow_right.bmp", "arrow-right_pressed.bmp", "greenArrow_right-pressed.bmp",
    ]
    for f in arrow_files:
        is_green = f.startswith("green")
        is_pressed = "pressed" in f
        if is_pressed:
            c = (80, 200, 80) if is_green else (150, 150, 150)
        else:
            c = (50, 150, 50) if is_green else (100, 100, 100)
        lbmp(f, 48, 48, *c)

    # --- D-pad ---
    lbmp("dpad_default.bmp", 120, 120, 60, 60, 60, "dpad")
    for d in ["up", "down", "left", "right"]:
        lbmp(f"dpad_{d}-press.bmp", 120, 120, 80, 80, 80, f"dpad {d}")

    # --- Page icons ---
    lbmp("pageUP_Icon.bmp", 32, 32, 100, 100, 100, "UP")
    lbmp("pageDOWN_Icon.bmp", 32, 32, 100, 100, 100, "DN")
    lbmp("pageOK_Icon.bmp", 32, 32, 100, 150, 100, "OK")

    # --- Sniper scope ---
    lbmp("SniperScope_Dial.bmp", 64, 64, 50, 50, 50)
    lbmp("SniperScope_Knob.bmp", 16, 16, 100, 100, 100, "K")

    # --- HUD (Hud.cpp) ---
    save("hud_ammo_icons.bmp", make_transparent_bmp(128, 32))
    save("Hud_Attack_Arrows.bmp", make_transparent_bmp(64, 64))
    save("Hud_Portrait_Small.bmp", make_transparent_bmp(64, 64))
    lbmp("HUD_Panel_top.bmp", 480, 32, 40, 40, 40)
    lbmp("HUD_Panel_top_sentrybot.bmp", 480, 32, 40, 50, 40)
    lbmp("HUD_Shield_Normal.bmp", 48, 48, 60, 60, 100, "Shld")
    lbmp("Hud_Shield_Button_Active.bmp", 48, 48, 80, 80, 150, "Shld!")
    lbmp("Hud_Key_Normal.bmp", 48, 48, 100, 100, 60, "Key")
    lbmp("Hud_Key_Active.bmp", 48, 48, 150, 150, 80, "Key!")
    lbmp("HUD_Health_Normal.bmp", 48, 48, 100, 60, 60, "HP")
    lbmp("Hud_Health_Button_Active.bmp", 48, 48, 150, 80, 80, "HP!")
    lbmp("Switch_Right_Normal.bmp", 32, 32, 80, 80, 80, "R")
    lbmp("Switch_Right_Active.bmp", 32, 32, 120, 120, 120, "R!")
    lbmp("Switch_Left_Normal.bmp", 32, 32, 80, 80, 80, "L")
    lbmp("Switch_Left_Active.bmp", 32, 32, 120, 120, 120, "L!")
    lbmp("vending_softkey_pressed.bmp", 64, 32, 100, 80, 40)
    lbmp("vending_softkey_normal.bmp", 64, 32, 80, 60, 30)
    lbmp("inGame_menu_softkey.bmp", 64, 32, 60, 60, 80)
    lbmp("Hud_Weapon_Normal.bmp", 48, 48, 70, 70, 70, "Wpn")
    lbmp("HUD_Weapon_Active.bmp", 48, 48, 120, 120, 120, "Wpn!")
    lbmp("Hud_Player.bmp", 64, 64, 80, 60, 50, "Player")
    lbmp("HUD_Player_Active.bmp", 64, 64, 120, 90, 70, "Player!")
    lbmp("HUD_Player_frame_Normal.bmp", 72, 72, 50, 50, 50, "Frame")
    lbmp("HUD_Player_frame_Active.bmp", 72, 72, 80, 80, 80, "Frame!")
    lbmp("Hud_PlayerDoom.bmp", 64, 64, 60, 80, 50, "Major")
    lbmp("HUD_PlayerDoom_Active.bmp", 64, 64, 90, 120, 70, "Major!")
    lbmp("Hud_PlayerScientist.bmp", 64, 64, 50, 60, 80, "Riley")
    lbmp("HUD_PlayerScientist_Active.bmp", 64, 64, 70, 90, 120, "Riley!")
    lbmp("Hud_Sentry.bmp", 64, 64, 70, 70, 50, "Sentry")
    lbmp("HUD_sentry_active.bmp", 64, 64, 110, 110, 70, "Sentry!")
    save("Hud_Test.bmp", make_transparent_bmp(64, 64))
    lbmp("Hud_Fill.bmp", 480, 32, 30, 30, 30)
    save("Hud_Actions.bmp", make_transparent_bmp(256, 64))

    # --- Menu system (MenuSystem.cpp) ---
    lbmp("vending_button_large.bmp", 128, 40, 80, 60, 40)
    lbmp("inGame_menu_option_button.bmp", 200, 36, 50, 50, 70)
    lbmp("menu_button_background.bmp", 400, 40, 40, 40, 60)
    lbmp("menu_button_background_on.bmp", 400, 40, 60, 60, 100)
    lbmp("menu_arrow_down.bmp", 32, 32, 100, 100, 100, "v")
    lbmp("menu_arrow_up.bmp", 32, 32, 100, 100, 100, "^")
    lbmp("vending_arrow_up_glow.bmp", 32, 32, 120, 100, 50, "^")
    lbmp("vending_arrow_down_glow.bmp", 32, 32, 120, 100, 50, "v")
    lbmp("Menu_Dial.bmp", 64, 64, 60, 60, 60)
    lbmp("Menu_Dial_Knob.bmp", 16, 16, 100, 100, 100, "o")
    lbmp("Menu_Main_BOX.bmp", 400, 280, 50, 50, 80)
    save("Main_Menu_OverLay.bmp", make_transparent_bmp(480, 320))
    save("Main_Help_OverLay.bmp", make_transparent_bmp(480, 320))
    save("Main_About_OverLay.bmp", make_transparent_bmp(480, 320))
    lbmp("Menu_YesNo_BOX.bmp", 300, 180, 40, 40, 60)
    lbmp("Menu_ChooseDIFF_BOX.bmp", 300, 220, 40, 40, 60)
    lbmp("Menu_Language_BOX.bmp", 300, 200, 40, 40, 60)
    lbmp("Menu_Option_BOX3.bmp", 400, 240, 35, 35, 55)
    lbmp("Menu_Option_BOX4.bmp", 400, 280, 35, 35, 55)
    lbmp("Menu_Option_SliderBar.bmp", 200, 20, 50, 50, 50)
    lbmp("Menu_Option_SliderOFF.bmp", 24, 24, 80, 80, 80, "O")
    lbmp("Menu_Option_SliderON.bmp", 24, 24, 120, 120, 60, "I")
    lbmp("gameMenu_Panel_bottom.bmp", 480, 40, 35, 35, 35)
    lbmp("gameMenu_Panel_bottom_sentrybot.bmp", 480, 40, 35, 45, 35)
    lbmp("gameMenu_Health.bmp", 32, 32, 150, 50, 50, "HP")
    lbmp("gameMenu_Shield.bmp", 32, 32, 50, 50, 150, "AR")
    lbmp("gameMenu_infoButton_Pressed.bmp", 32, 32, 100, 100, 150, "i!")
    lbmp("gameMenu_infoButton_Normal.bmp", 32, 32, 60, 60, 100, "i")
    lbmp("vending_button_help.bmp", 64, 32, 80, 80, 40)
    lbmp("gameMenu_TornPage.bmp", 400, 280, 200, 190, 170)
    lbmp("gameMenu_Background.bmp", 480, 320, 45, 45, 65)
    lbmp("Main_Menu_DialA_anim.bmp", 128, 64, 60, 60, 60)
    lbmp("Main_Menu_DialC_anim.bmp", 128, 64, 60, 60, 60)
    lbmp("Main_Menu_Slide_anim.bmp", 256, 32, 50, 50, 50)
    lbmp("gameMenu_ScrollBar.bmp", 16, 200, 50, 50, 50, "|", scale=1)
    lbmp("gameMenu_topSlider.bmp", 16, 16, 80, 80, 80, "^")
    lbmp("gameMenu_midSlider.bmp", 16, 16, 80, 80, 80, "=")
    lbmp("gameMenu_bottomSlider.bmp", 16, 16, 80, 80, 80, "v")
    lbmp("vending_scroll_bar.bmp", 16, 200, 60, 50, 30, "|", scale=1)
    lbmp("vending_scroll_button_top.bmp", 16, 16, 100, 80, 40, "^")
    lbmp("vending_scroll_button_middle.bmp", 16, 16, 100, 80, 40, "=")
    lbmp("vending_scroll_button_bottom.bmp", 16, 16, 100, 80, 40, "v")
    lbmp("logo2.bmp", 480, 60, 50, 50, 80, "DOOM II RPG DEMO")

    # --- Boot/Logo ---
    lbmp("l2.bmp", 480, 320, 40, 40, 80, "DEMO ENGINE")
    lbmp("bootL.bmp", 240, 320, 50, 50, 90, "bootL")
    lbmp("bootR.bmp", 240, 320, 50, 50, 90, "bootR")
    lbmp("logo.bmp", 480, 320, 60, 60, 100, "DEMO GAME")

    # --- Character selection ---
    lbmp("prolog.bmp", 480, 320, 30, 30, 50, "Prolog")
    lbmp("Character_select_stat_bar.bmp", 200, 20, 60, 80, 60)
    lbmp("Character_select_stat_header.bmp", 200, 24, 50, 50, 70)
    lbmp("Character_select_top_bar.bmp", 480, 32, 40, 40, 50)
    lbmp("character_upperbar.bmp", 480, 32, 40, 40, 50)
    save("charSelect.bmp", make_transparent_bmp(256, 128))
    lbmp("charSelectionBG.bmp", 480, 320, 45, 45, 65, "Char Select BG")
    lbmp("Major_legs.bmp", 64, 128, 60, 50, 40, "Major legs")
    lbmp("Major_torso.bmp", 64, 128, 70, 60, 50, "Major torso")
    lbmp("Riley_legs.bmp", 64, 128, 40, 50, 60, "Riley legs")
    lbmp("Riley_torso.bmp", 64, 128, 50, 60, 70, "Riley torso")
    lbmp("Sarge_legs.bmp", 64, 128, 50, 60, 40, "Sarge legs")
    lbmp("Sarge_torso.bmp", 64, 128, 60, 70, 50, "Sarge torso")

    # --- Minigame images ---
    lbmp("hackerBG.bmp", 480, 320, 10, 40, 10, "Hacker BG")
    lbmp("blockGameColors.bmp", 128, 128, 40, 40, 40, "blockGame Colors")
    lbmp("matrixSkip_BG.bmp", 480, 320, 10, 30, 10, "matrixSkip BG")
    lbmp("sentryGame.bmp", 480, 320, 40, 40, 20, "Sentry Game")
    lbmp("matrixSkip_grid_pressed.bmp", 48, 48, 80, 120, 80, "1")
    lbmp("matrixSkip_grid_pressed_2.bmp", 48, 48, 120, 80, 80, "2")
    lbmp("matrixSkip_grid_pressed_3.bmp", 48, 48, 80, 80, 120, "3")
    lbmp("matrixSkip_button.bmp", 64, 32, 60, 80, 60, "Skip")
    lbmp("vendingGame.bmp", 480, 320, 40, 35, 20, "Vending Game")
    lbmp("vending_BG.bmp", 480, 320, 45, 40, 25, "vending BG")
    lbmp("vendingBG.bmp", 480, 320, 45, 40, 25, "vendingBG")
    lbmp("vending_submit.bmp", 80, 32, 80, 60, 40, "Submit")
    lbmp("vending_submit_pressed.bmp", 80, 32, 120, 90, 60, "Submit!")
    lbmp("vending_arrow_up.bmp", 32, 32, 80, 60, 40, "^")
    lbmp("vending_arrow_up_pressed.bmp", 32, 32, 120, 90, 60, "^!")
    lbmp("vending_arrow_down.bmp", 32, 32, 80, 60, 40, "v")
    lbmp("vending_arrow_down_pressed.bmp", 32, 32, 120, 90, 60, "v!")
    lbmp("vending_button_small.bmp", 48, 24, 80, 60, 40, "btn")

    # --- Particles ---
    lbmp("Skeletongibs_24.bmp", 24, 24, 200, 200, 180, "Sk")
    lbmp("Stonegibs_24.bmp", 24, 24, 120, 120, 120, "St")
    lbmp("woodgibs_24.bmp", 24, 24, 140, 100, 60, "Wd")

    # --- Render ---
    lbmp("portal_image2.bmp", 64, 64, 100, 50, 150, "Portal2")
    lbmp("portal_image.bmp", 64, 64, 80, 40, 120, "Portal")

    # --- Runtime images ---
    lbmp("Automap_Cursor.bmp", 16, 16, 200, 200, 50, "+")
    save("ui_images.bmp", make_transparent_bmp(256, 128))
    save("damage.bmp", make_transparent_bmp(480, 320))
    save("damage_bot.bmp", make_transparent_bmp(480, 320))
    save("scope.bmp", make_transparent_bmp(480, 320))

    # --- Travel map (on-demand but loaded when changing levels) ---
    lbmp("highlight.bmp", 32, 32, 200, 200, 100, "HL")
    save("magnifyingGlass.bmp", make_transparent_bmp(32, 32))
    save("spaceShip.bmp", make_transparent_bmp(32, 32))
    lbmp("TM_Levels1.bmp", 480, 320, 30, 30, 50, "TM Levels1")
    lbmp("TM_Levels2.bmp", 480, 320, 30, 30, 50, "TM Levels2")
    lbmp("TM_Levels3.bmp", 480, 320, 30, 30, 50, "TM Levels3")
    lbmp("TM_Levels4.bmp", 480, 320, 30, 30, 50, "TM Levels4")
    save("toMoon.bmp", make_transparent_bmp(480, 320))
    save("toEarth.bmp", make_transparent_bmp(480, 320))
    save("toHell.bmp", make_transparent_bmp(480, 320))
    lbmp("TravelMap.bmp", 480, 320, 20, 20, 40, "Travel Map")
    save("travelMapHorzGrid.bmp", make_transparent_bmp(480, 320))
    save("travelMapVertGrid.bmp", make_transparent_bmp(480, 320))

    # --- Comic book (on-demand) ---
    lbmp("ComicBook/Cover.bmp", 480, 320, 50, 30, 30, "Comic Cover")
    for i in range(1, 17):
        lbmp(f"ComicBook/page_{i:02d}.bmp", 480, 320, 40, 40, 40, f"Comic Page {i}")
    # iPhone comic frames
    os.makedirs(os.path.join(out_dir, "ComicBook", "frames"), exist_ok=True)
    iphone_pages = [
        "iPhone cover", "iPhone page 1a", "iPhone page 1b",
        "iPhone page 2a", "iPhone page 2b", "iPhone page 2c",
        "iPhone page 3a", "iPhone page 3b",
        "iPhone page 4a", "iPhone page 4b", "iPhone page 4c",
        "iPhone page 5a", "iPhone page 5b", "iPhone page 5c",
        "iPhone page 6a", "iPhone page 6b", "iPhone page 6c",
        "iPhone page 7a", "iPhone page 7b",
        "iPhone page 8a", "iPhone page 8b", "iPhone page 8c",
        "iPhone page 9",
        "iPhone page 10a", "iPhone page 10b", "iPhone page 10c",
        "iPhone page 11a", "iPhone page 11b",
        "iPhone page 12a", "iPhone page 12b",
        "iPhone page 13a", "iPhone page 13b",
        "iPhone page 14a", "iPhone page 14b",
        "iPhone page 15a", "iPhone page 15b", "iPhone page 15c",
        "iPhone page 16a", "iPhone page 16b",
    ]
    for name in iphone_pages:
        lbmp(f"ComicBook/frames/{name}.bmp", 480, 320, 40, 40, 40, name)

    print(f"  Generated BMP images")


# ============================================================================
# YAML Generators
# ============================================================================

def generate_game_yaml(out_dir):
    content = """\
# Demo Game - Generated placeholder
game:
  name: "DoomIIRPG Demo"
  description: "Standalone demo with generated placeholder assets"
  version: "1.0"
  window_title: "Doom II RPG - Demo"
  save_dir: "DoomIIRPG_Demo.app"
  entry_map: "map00"
  no_fog_maps: [0]
"""
    with open(os.path.join(out_dir, "game.yaml"), "w") as f:
        f.write(content)


def generate_characters_yaml(out_dir):
    content = """\
characters:
  - name: Major
    defense: 8
    strength: 9
    accuracy: 97
    agility: 12
    iq: 110
    credits: 30

  - name: Sarge
    defense: 12
    strength: 14
    accuracy: 92
    agility: 6
    iq: 100
    credits: 10

  - name: Scientist
    defense: 8
    strength: 8
    accuracy: 87
    agility: 6
    iq: 150
    credits: 80
"""
    with open(os.path.join(out_dir, "characters.yaml"), "w") as f:
        f.write(content)


def make_silent_wav(duration_ms=100, sample_rate=22050):
    """Create a minimal silent WAV file (PCM 16-bit mono)."""
    num_samples = (sample_rate * duration_ms) // 1000
    bits_per_sample = 16
    num_channels = 1
    byte_rate = sample_rate * num_channels * bits_per_sample // 8
    block_align = num_channels * bits_per_sample // 8
    data_size = num_samples * block_align

    buf = bytearray()
    # RIFF header
    buf += b"RIFF"
    buf += struct.pack("<I", 36 + data_size)  # file size - 8
    buf += b"WAVE"
    # fmt chunk
    buf += b"fmt "
    buf += struct.pack("<I", 16)  # chunk size
    buf += struct.pack("<H", 1)   # PCM format
    buf += struct.pack("<H", num_channels)
    buf += struct.pack("<I", sample_rate)
    buf += struct.pack("<I", byte_rate)
    buf += struct.pack("<H", block_align)
    buf += struct.pack("<H", bits_per_sample)
    # data chunk
    buf += b"data"
    buf += struct.pack("<I", data_size)
    buf += bytes(data_size)  # silence (all zeros)
    return bytes(buf)


# The 144 default sound filenames from Sounds.h
SOUND_FILE_NAMES = [
    "Arachnotron_active.wav", "Arachnotron_death.wav", "Arachnotron_sight.wav",
    "Arachnotron_walk.wav", "Archvile_active.wav", "Archvile_attack.wav",
    "Archvile_death.wav", "Archvile_pain.wav", "Archvile_sight.wav", "BFG.wav",
    "block_drop.wav", "block_pick.wav", "Cacodemon_death.wav", "Cacodemon_sight.wav",
    "chaingun.wav", "chainsaw.wav", "ChainsawGoblin_attack.wav",
    "ChainsawGoblin_death.wav", "ChainsawGoblin_pain.wav", "ChainsawGoblin_sight.wav",
    "chime.wav", "claw.wav", "Cyberdemon_death.wav", "Cyberdemon_sight.wav",
    "demon_active.wav", "demon_pain.wav", "demon_small_active.wav", "Dialog_Help.wav",
    "door_close.wav", "door_close_slow.wav", "door_open.wav", "door_open_slow.wav",
    "explosion.wav", "fireball.wav", "fireball_impact.wav", "footstep1.wav",
    "footstep2.wav", "gib.wav", "glass.wav", "hack_clear.wav", "hack_fail.wav",
    "hack_failed.wav", "hack_submit.wav", "hack_success.wav", "hack_successful.wav",
    "HolyWaterPistol.wav", "HolyWaterPistol_refill.wav", "hoof.wav", "impClaw.wav",
    "Imp_active.wav", "Imp_death1.wav", "Imp_death2.wav", "Imp_sight1.wav",
    "Imp_sight2.wav", "item_pickup.wav", "loot.wav", "LostSoul_attack.wav",
    "Maglev_arrive.wav", "Maglev_depart.wav", "malfunction.wav", "Mancubus_attack.wav",
    "Mancubus_death.wav", "Mancubus_pain.wav", "Mancubus_sight.wav", "menu_open.wav",
    "menu_scroll.wav", "monster_pain.wav", "Music_Boss.wav", "Music_General.wav",
    "Music_Level_End.wav", "Music_LevelUp.wav", "Music_Title.wav", "no_use.wav",
    "no_use2.wav", "Pinkinator_attack.wav", "Pinkinator_death.wav",
    "Pinkinator_pain.wav", "Pinkinator_sight.wav", "Pinkinator_spawn.wav",
    "Pinky_attack.wav", "Pinky_death.wav", "Pinky_sight.wav", "Pinky_small_attack.wav",
    "Pinky_small_death.wav", "Pinky_small_pain.wav", "Pinky_small_sight.wav",
    "pistol.wav", "plasma.wav", "platform_start.wav", "platform_stop.wav",
    "player_death1.wav", "player_death2.wav", "player_death3.wav", "player_pain1.wav",
    "player_pain2.wav", "Revenant_active.wav", "Revenant_attack.wav",
    "Revenant_death.wav", "Revenant_punch.wav", "Revenant_sight.wav",
    "Revenant_swing.wav", "rocketLauncher.wav", "rocket_explode.wav", "secret.wav",
    "Sentinel_attack.wav", "Sentinel_death.wav", "Sentinel_pain.wav",
    "Sentinel_sight.wav", "SentryBot_activate.wav", "SentryBot_death.wav",
    "SentryBot_discard.wav", "SentryBot_pain.wav", "SentryBot_return.wav",
    "slider.wav", "Soulcube.wav", "SpiderMastermind_death.wav",
    "SpiderMastermind_sight.wav", "supershotgun.wav", "supershotgun_close.wav",
    "supershotgun_load.wav", "supershotgun_open.wav", "switch.wav", "switch_exit.wav",
    "teleport.wav", "use_item.wav", "vending_sale.wav", "vent.wav", "VIOS_attack1.wav",
    "VIOS_attack2.wav", "VIOS_death.wav", "VIOS_pain.wav", "VIOS_sight.wav",
    "weapon_pickup.wav", "Weapon_Sniper_Scope.wav", "Weapon_Toilet_Pull.wav",
    "Weapon_Toilet_Smash.wav", "Weapon_Toilet_Throw.wav", "Weapon_Toilet_Throw2.wav",
    "zombie_active.wav", "zombie_death1.wav", "zombie_death2.wav", "zombie_death3.wav",
    "zombie_sight1.wav", "zombie_sight2.wav", "zombie_sight3.wav",
]


def generate_sounds_yaml(out_dir):
    lines = ["# Sound definitions (144 entries matching engine defaults)\n\nsounds:\n"]
    for name in SOUND_FILE_NAMES:
        lines.append(f"  - {name}\n")
    with open(os.path.join(out_dir, "sounds.yaml"), "w") as f:
        f.writelines(lines)


def generate_sound_files(out_dir):
    """Generate silent WAV files for all 144 sounds."""
    sounds_dir = os.path.join(out_dir, "sounds2")
    os.makedirs(sounds_dir, exist_ok=True)
    wav_data = make_silent_wav()
    for name in SOUND_FILE_NAMES:
        with open(os.path.join(sounds_dir, name), "wb") as f:
            f.write(wav_data)


def generate_strings_yaml(out_dir):
    """Generate strings.yaml with all required groups and placeholder text."""
    # String counts per group (from examining the original)
    # Groups 0-14 for language 0 (English)
    # The engine references strings by STRINGID(group, index) = (group << 10) | index
    group_counts = [200, 39, 29, 290, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 3]

    lines = ["# Demo game string tables\n", "strings:\n"]

    for group in range(15):
        lines.append(f"  - language: 0\n")
        lines.append(f"    group: {group}\n")
        lines.append(f"    values:\n")
        count = group_counts[group]
        for s in range(count):
            # Provide meaningful strings for key entries
            text = get_demo_string(group, s)
            # Escape for YAML
            text = text.replace("\\", "\\\\").replace('"', '\\"')
            lines.append(f'      - "{text}"\n')

    with open(os.path.join(out_dir, "strings.yaml"), "w") as f:
        f.writelines(lines)


def get_demo_string(group, index):
    """Return demo string for given group and index."""
    # Group 0 = CodeStrings (UI labels, help texts)
    if group == 0:
        code_strings = {
            20: "I want my mommy!",
            30: "Exit",
            31: "Play",
            37: "An error has occurred.",
            38: "Game Saved",
            39: "Game Loaded",
            40: "Skip",
            41: "Loading...",
            42: "Enter",
            43: "Continue",
            44: "Door is Locked",
            45: "Use",
            46: "Talk",
            47: "Open",
            48: "Search",
            49: "Attack",
            50: "Interact",
        }
        code_strings_high = {
            133: "Welcome to the DoomIIRPG Engine Demo.\\n\\nThis is a standalone demo game with placeholder assets.\\nNo original game files required.\\n\\nPress any key to continue.",
        }
        return code_strings.get(index, code_strings_high.get(index, f"str_{index}"))

    # Group 1 = EntityStrings (entity names, descriptions)
    if group == 1:
        entity_strings = {
            0: "Zombie",
            1: "Imp",
            2: "Pinky",
            3: "Cacodemon",
            4: "Revenant",
            5: "Mancubus",
            6: "Arch-Vile",
            7: "Health Pack",
            8: "Armor",
            9: "Bullets",
            10: "Shells",
            11: "Cells",
            12: "Rockets",
            13: "Key Card",
            14: "Assault Rifle",
            15: "Shotgun",
            16: "Chaingun",
            17: "Plasma Gun",
            18: "Rocket Launcher",
            19: "BFG",
        }
        return entity_strings.get(index, f"entity_{index}")

    # Group 3 = MenuStrings
    if group == 3:
        menu_strings = {
            0: "New Game",
            1: "Continue",
            2: "Options",
            3: "Help",
            4: "About",
            5: "Credits",
            6: "Exit",
            7: "Resume",
            8: "Save Game",
            9: "Load Game",
            10: "Easy",
            11: "Normal",
            12: "Hard",
            13: "Nightmare",
            14: "Yes",
            15: "No",
            16: "OK",
            17: "Cancel",
            18: "Back",
            19: "Sounds",
            20: "Music",
            21: "On",
            22: "Off",
            23: "Enable Sounds?",
            24: "Select Difficulty",
            25: "Save & Exit",
            26: "Inventory",
            27: "PDA",
            28: "Automap",
            29: "Settings",
            30: "Return To Player",
            31: "Select Character",
        }
        return menu_strings.get(index, f"menu_{index}")

    # Group 14 = Translations (language names)
    if group == 14:
        return ["English", "Francais", "Deutsch"][index] if index < 3 else f"lang_{index}"

    # Groups 4-13 = Map strings (M_01 through M_09, M_TEST)
    return f"s{group}_{index}"


def generate_entities_yaml(out_dir):
    """Generate entities.yaml with 190 entity definitions."""
    lines = ["# Entity definitions for demo game\n\nentities:\n"]

    # Generate 190 entities matching original tile indices
    for i in range(190):
        etype = "corpse"
        subtype = 0
        parm = 0
        name = 0
        long_name = 0
        desc = 0
        tile_index = 0

        # Assign meaningful types for key entities
        if i < 10:
            etype = "corpse"
            subtype = 15
            parm = i % 5
        elif i < 20:
            etype = "corpse"
            subtype = 0
            parm = i - 10
        elif 20 <= i < 30:
            etype = "monster"
            subtype = i - 20
            tile_index = 18 + (i - 20) * 3
            name = 1024 + (i - 20)  # Group 1 strings
        elif 30 <= i < 50:
            etype = "item"
            subtype = i - 30
            tile_index = 80 + (i - 30)
        elif 50 <= i < 60:
            etype = "door"
            subtype = i - 50
            tile_index = 60 + (i - 50)
        elif 60 <= i < 80:
            etype = "decor"
            subtype = i - 60
            tile_index = 100 + (i - 60)
        elif 80 <= i < 90:
            etype = "npc"
            subtype = i - 80
            tile_index = 120 + (i - 80)
        else:
            etype = "world"
            subtype = 0
            tile_index = 0

        lines.append(f"  - tile_index: {tile_index}\n")
        lines.append(f"    type: {etype}\n")
        lines.append(f"    subtype: {subtype}\n")
        lines.append(f"    parm: {parm}\n")
        lines.append(f"    name: {name}\n")
        lines.append(f"    long_name: {long_name}\n")
        lines.append(f"    description: {desc}\n")

    with open(os.path.join(out_dir, "entities.yaml"), "w") as f:
        f.writelines(lines)


def generate_menus_yaml(out_dir):
    """Generate menus.yaml with all required menu definitions."""
    # Menu IDs from Menus.h enum:
    #  3=MAIN, 4=HELP, 8=ABOUT, 13=EXIT, 14=CONFIRMNEW, 15=CONFIRMNEW2,
    # 16=DIFFICULTY, 17=OPTIONS, 23=SELECT_LANGUAGE, 25=ENABLE_SOUNDS,
    # 29=INGAME, 42=INGAME_EXIT
    # String IDs: STRINGID(group=3, index) = 3072 + index
    # Group 3 strings: 0=New Game, 1=Continue, 2=Options, 3=Help, 4=About,
    #   5=Credits, 6=Exit, 7=Resume, 8=Save Game, 9=Load Game,
    #  10=Easy, 11=Normal, 12=Hard, 13=Nightmare, 14=Yes, 15=No,
    #  16=OK, 17=Cancel, 18=Back, 19=Sounds, 20=Music, 21=On, 22=Off,
    #  23=Enable Sounds?, 24=Select Difficulty, 25=Save & Exit,
    #  26=Inventory, 27=PDA, 28=Automap, 29=Settings, 30=Return To Player,
    #  31=Select Character

    content = """\
# Menu definitions for demo game
# Menu IDs must match Menus.h enum values exactly
# Menu types: default=0, list=1, confirm=2, confirm2=3, main=4,
#             help=5, vcenter=6, notebook=7, main_list=8, vending_machine=9

menus:
  # MENU_MAIN (3) - Main menu
  - menu_id: 3
    type: main
    items:
      - string_id: 3072
        flags: align_center
        action: newgame
      - string_id: 3073
        flags: align_center,disabled
        action: load
      - string_id: 3074
        flags: align_center
        action: goto
        param: 17
      - string_id: 3078
        flags: align_center
        action: exit

  # MENU_MAIN_HELP (4)
  - menu_id: 4
    type: help
    items:
      - string_id: 3090
        flags: align_center
        action: back

  # MENU_MAIN_ABOUT (8)
  - menu_id: 8
    type: vcenter
    items:
      - string_id: 3090
        flags: align_center
        action: back

  # MENU_MAIN_EXIT (13) - Confirm exit
  - menu_id: 13
    type: confirm
    items:
      - string_id: 3078
        flags: noselect,align_center
      - string_id: 3086
        flags: align_center
        action: exit
      - string_id: 3087
        flags: align_center
        action: back

  # MENU_MAIN_CONFIRMNEW (14)
  - menu_id: 14
    type: confirm
    items:
      - string_id: 3072
        flags: noselect,align_center
      - string_id: 3086
        flags: align_center
        action: goto
        param: 16
      - string_id: 3087
        flags: align_center
        action: back

  # MENU_MAIN_DIFFICULTY (16) - Difficulty selection
  - menu_id: 16
    type: vcenter
    items:
      - string_id: 3082
        flags: align_center
        action: difficulty
        param: 0
      - string_id: 3083
        flags: align_center
        action: difficulty
        param: 1
      - string_id: 3084
        flags: align_center
        action: difficulty
        param: 2
      - string_id: 3085
        flags: align_center
        action: difficulty
        param: 3

  # MENU_MAIN_OPTIONS (17)
  - menu_id: 17
    type: vcenter
    items:
      - string_id: 3090
        flags: align_center
        action: back

  # MENU_SELECT_LANGUAGE (23)
  - menu_id: 23
    type: vcenter
    items:
      - string_id: 3090
        flags: align_center
        action: back

  # MENU_ENABLE_SOUNDS (25) - Sound enable prompt on first launch
  # Note: initMenu handles this specially with SetYESNO(), but we still
  # need the menu_id present so loadMenuItems doesn't error
  - menu_id: 25
    type: confirm
    items:
      - string_id: 3095
        flags: noselect,align_center
      - string_id: 3086
        flags: align_center
        action: togsound
        param: 1
      - string_id: 3087
        flags: align_center
        action: togsound
        param: 0

  # MENU_INGAME (29) - In-game pause menu
  - menu_id: 29
    type: default
    items:
      - string_id: 3079
        flags: align_center
        action: back
      - string_id: 3080
        flags: align_center
        action: save
      - string_id: 3097
        flags: align_center
        action: exit

  # MENU_INGAME_EXIT (42)
  - menu_id: 42
    type: confirm
    items:
      - string_id: 3078
        flags: noselect,align_center
      - string_id: 3086
        flags: align_center
        action: backtomain
      - string_id: 3087
        flags: align_center
        action: back

  # MENU_INGAME_OPTIONS (35)
  - menu_id: 35
    type: vcenter
    items:
      - string_id: 3090
        flags: align_center
        action: back

  # MENU_INGAME_SAVE (48)
  - menu_id: 48
    type: confirm
    items:
      - string_id: 3080
        flags: noselect,align_center
      - string_id: 3086
        flags: align_center
        action: save
      - string_id: 3087
        flags: align_center
        action: back

  # MENU_INGAME_DEAD (51) - Death menu
  - menu_id: 51
    type: confirm
    items:
      - string_id: 3072
        flags: noselect,align_center
      - string_id: 3086
        flags: align_center
        action: load
      - string_id: 3087
        flags: align_center
        action: backtomain
"""
    with open(os.path.join(out_dir, "menus.yaml"), "w") as f:
        f.write(content)


def generate_tables_yaml(out_dir):
    """Generate tables.yaml with all 14 game tables."""
    lines = ["# Game tables for demo\n\n"]

    # --- Weapons (32 entries, stride 9 bytes) ---
    lines.append("weapons:\n")
    weapons = [
        # Player weapons 0-14
        {"name": "assault_rifle", "str_min": 8, "str_max": 10, "range_min": 0, "range_max": 5, "ammo_type": 1, "ammo_usage": 1, "proj_type": "bullet", "num_shots": 2, "shot_hold": 50},
        {"name": "chainsaw", "str_min": 18, "str_max": 22, "range_min": 0, "range_max": 1, "ammo_type": 0, "ammo_usage": 0, "proj_type": "none", "num_shots": 1, "shot_hold": 100},
        {"name": "holy_water_pistol", "str_min": 5, "str_max": 8, "range_min": 0, "range_max": 3, "ammo_type": 3, "ammo_usage": 2, "proj_type": "water", "num_shots": 2, "shot_hold": 50},
    ]
    # Fill remaining with generic melee
    for i in range(len(weapons), 32):
        weapons.append({"name": f"weapon_{i}", "str_min": 5, "str_max": 8, "range_min": 1, "range_max": 1, "ammo_type": 0, "ammo_usage": 0, "proj_type": "melee", "num_shots": 1, "shot_hold": 20})
    for w in weapons:
        lines.append(f"  - str_min: {w['str_min']}\n")
        lines.append(f"    str_max: {w['str_max']}\n")
        lines.append(f"    range_min: {w['range_min']}\n")
        lines.append(f"    range_max: {w['range_max']}\n")
        lines.append(f"    ammo_type: {w['ammo_type']}\n")
        lines.append(f"    ammo_usage: {w['ammo_usage']}\n")
        lines.append(f"    proj_type: {w['proj_type']}\n")
        lines.append(f"    num_shots: {w['num_shots']}\n")
        lines.append(f"    shot_hold: {w['shot_hold']}\n")

    # --- WeaponInfo (32 entries, stride 6 bytes) ---
    lines.append("\nweapon_info:\n")
    for i in range(32):
        lines.append(f"  - idle_x: 0\n    idle_y: 0\n    attack_x: 0\n    attack_y: -5\n    flash_x: 0\n    flash_y: -10\n")

    # --- MonsterAttacks (18 entries) ---
    lines.append("\nmonster_attacks:\n")
    for i in range(18):
        lines.append(f"  - parm0_attack1: bite\n    parm0_attack2: '0'\n    parm0_chance: 100\n")
        lines.append(f"    parm1_attack1: bite\n    parm1_attack2: '0'\n    parm1_chance: 100\n")
        lines.append(f"    parm2_attack1: bite\n    parm2_attack2: '0'\n    parm2_chance: 100\n")

    # --- MonsterStats ---
    lines.append("\nmonster_stats:\n  data: [")
    # 18 monsters * ~10 stats each = 180 bytes (approximate)
    stats = [10, 5, 3, 1, 50, 0, 0, 0, 0, 0] * 18
    lines.append(", ".join(str(s) for s in stats))
    lines.append("]\n")

    # --- CombatMasks ---
    lines.append("\ncombat_masks: ['0x0', '0x0', '0x0', '0x0', '0x0', '0x0', '0x0', '0x0']\n")

    # --- KeysNumeric ---
    lines.append("\nkeys_numeric:\n  data: [")
    lines.append(", ".join(["0"] * 40))
    lines.append("]\n")

    # --- OSCCycle ---
    lines.append("\nosc_cycle:\n  data: [")
    osc = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
    lines.append(", ".join(str(v) for v in osc))
    lines.append("]\n")

    # --- LevelNames ---
    lines.append("\nlevel_names: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]\n")

    # --- MonsterColors ---
    lines.append("\nmonster_colors:\n")
    lines.append("  zombie: [100, 80, 60]\n")
    lines.append("  zombie_commando: [80, 100, 60]\n")
    lines.append("  lost_soul: [200, 150, 50]\n")
    lines.append("  imp: [150, 100, 50]\n")
    lines.append("  sawcubus: [120, 80, 60]\n")
    lines.append("  pinky: [180, 100, 100]\n")

    # --- SinTable (1025 entries) ---
    lines.append("\nsin_table:\n  data: [")
    sin_values = []
    for i in range(1025):
        val = round(65536 * math.sin(i * 2 * math.pi / 1024))
        # Clamp to 32-bit signed int range
        sin_values.append(str(val))
    lines.append(", ".join(sin_values))
    lines.append("]\n")

    # --- EnergyDrinkData ---
    lines.append("\nenergy_drink_data:\n  data: [")
    # 11 drinks * 4 stats = 44 shorts
    drink_data = [10, 0, 0, 0] * 11
    lines.append(", ".join(str(d) for d in drink_data))
    lines.append("]\n")

    # --- MonsterWeakness ---
    lines.append("\nmonster_weakness:\n")
    monster_names = ["zombie", "zombie_commando", "lost_soul", "imp", "sawcubus", "pinky",
                     "cacodemon", "sentinel", "mancubus", "revenant", "arch_vile", "sentry_bot",
                     "cyberdemon", "mastermind", "phantom", "archvile_ghost", "belphegor", "apollyon"]
    for name in monster_names:
        lines.append(f"  {name}: 0\n")

    # --- MovieEffects ---
    lines.append("\nmovie_effects:\n  data: [0, 0, 0, 0, 0, 0, 0, 0]\n")

    # --- MonsterSounds ---
    lines.append("\nmonster_sounds:\n")
    for i in range(18):
        lines.append(f"  - alert1: none\n    alert2: none\n    alert3: none\n")
        lines.append(f"    attack1: none\n    attack2: none\n    idle: none\n    pain: none\n    death: none\n")

    with open(os.path.join(out_dir, "tables.yaml"), "w") as f:
        f.writelines(lines)


# ============================================================================
# Binary: tables.bin (needed for sky data at tables 16-19)
# ============================================================================

def generate_tables_bin(out_dir):
    """Generate tables.bin with 20 table offsets and minimal sky data.

    Tables 0-13 are loaded from tables.yaml and their data here is placeholder.
    Tables 14-19 are loaded from tables.bin at runtime (sky/camera data).

    Each table entry in the data section: [int32 count/size] [data bytes]
    For byte tables (loadByteTable): count = number of bytes
    For short tables (loadShortTable/loadUShortTable): count = number of shorts
    """
    data = bytearray()

    # Build table data sequentially, tracking end offsets
    offsets = []

    # Tables 0-15: minimal placeholder (4 bytes each = int32 size of 0)
    for i in range(16):
        data += struct.pack("<i", 0)  # size/count = 0
        offsets.append(len(data))

    # Table 16: sky palette (ushort table) - 16 gray colors
    num_colors = 16
    data += struct.pack("<i", num_colors)  # count of shorts
    for c in range(num_colors):
        # RGB565 grayscale
        gray5 = (c * 2) & 0x1F
        gray6 = (c * 4) & 0x3F
        rgb565 = (gray5 << 11) | (gray6 << 5) | gray5
        data += struct.pack("<H", rgb565)
    offsets.append(len(data))

    # Table 17: sky texels (ubyte table) - 64x32 simple gradient
    sky_w, sky_h = 64, 32
    num_texels = sky_w * sky_h
    data += struct.pack("<i", num_texels)  # count of bytes
    for y in range(sky_h):
        for x in range(sky_w):
            data += struct.pack("B", (y * 15) // sky_h)  # gradient
    offsets.append(len(data))

    # Table 18: sky palette 2 (same as 16)
    data += struct.pack("<i", num_colors)
    for c in range(num_colors):
        gray5 = (c * 2) & 0x1F
        gray6 = (c * 4) & 0x3F
        rgb565 = (gray5 << 11) | (gray6 << 5) | gray5
        data += struct.pack("<H", rgb565)
    offsets.append(len(data))

    # Table 19: sky texels 2 (same as 17)
    data += struct.pack("<i", num_texels)
    for y in range(sky_h):
        for x in range(sky_w):
            data += struct.pack("B", (y * 15) // sky_h)
    offsets.append(len(data))

    # Write header (20 int32 offsets) + data
    header = bytearray()
    for off in offsets:
        header += struct.pack("<i", off)

    with open(os.path.join(out_dir, "tables.bin"), "wb") as f:
        f.write(header)
        f.write(data)


# ============================================================================
# Binary: map00.bin
# ============================================================================

def generate_map00(out_dir):
    """Generate a minimal playable map.

    Layout (32x32 grid):
      Room 1: rows 10-20, cols 5-15 (10x10 room)
      Hallway: row 15, cols 15-20 (connects rooms)
      Room 2: rows 10-20, cols 20-30 (10x10 room)
      Player spawn: (15, 10) facing East
    """
    # Map dimensions
    MAP_W, MAP_H = 32, 32

    # Build collision map (1 = wall/blocked, 0 = open)
    collision = [[1] * MAP_W for _ in range(MAP_H)]

    # Carve Room 1
    for y in range(11, 20):
        for x in range(6, 15):
            collision[y][x] = 0

    # Carve hallway
    for x in range(15, 21):
        for y in range(14, 17):
            collision[y][x] = 0

    # Carve Room 2
    for y in range(11, 20):
        for x in range(21, 30):
            collision[y][x] = 0

    # Player spawn tile
    spawn_x, spawn_y = 10, 15
    spawn_index = spawn_y * MAP_W + spawn_x
    spawn_dir = 2  # East (0=N, 2=E, 4=S, 6=W)

    # Header values
    num_nodes = 0
    data_size_polys = 0
    num_lines = 0
    num_normals = 0
    num_normal_sprites = 0
    num_z_sprites = 0
    num_tile_events = 0
    map_bytecode_size = 0
    total_maya_cameras = 0
    total_maya_camera_keys = 0

    buf = bytearray()

    # --- Header (42 bytes) ---
    buf += struct.pack("B", 3)                    # version
    buf += struct.pack("<i", 0)                    # mapCompileDate
    buf += struct.pack("<H", spawn_index)          # mapSpawnIndex
    buf += struct.pack("B", spawn_dir)             # mapSpawnDir
    buf += struct.pack("b", 0)                     # mapFlagsBitmask
    buf += struct.pack("b", 0)                     # totalSecrets
    buf += struct.pack("B", 0)                     # totalLoot
    buf += struct.pack("<H", num_nodes)            # numNodes
    buf += struct.pack("<H", data_size_polys)      # dataSizePolys
    buf += struct.pack("<H", num_lines)            # numLines
    buf += struct.pack("<H", num_normals)          # numNormals
    buf += struct.pack("<H", num_normal_sprites)   # numNormalSprites
    buf += struct.pack("<h", num_z_sprites)        # numZSprites
    buf += struct.pack("<h", num_tile_events)      # numTileEvents
    buf += struct.pack("<h", map_bytecode_size)    # mapByteCodeSize
    buf += struct.pack("B", total_maya_cameras)    # totalMayaCameras
    buf += struct.pack("<h", total_maya_camera_keys) # totalMayaCameraKeys
    # mayaTweenOffsets (6 shorts)
    for _ in range(6):
        buf += struct.pack("<h", 0)

    assert len(buf) == 42, f"Header should be 42 bytes, got {len(buf)}"

    # --- Media section ---
    buf += DEADBEEF
    media_count = 0
    buf += struct.pack("<H", media_count)
    buf += DEADBEEF

    # --- Normals (0 entries) ---
    # (no data for 0 normals)
    buf += CAFEBABE

    # --- Node offsets (0 entries) ---
    buf += CAFEBABE

    # --- Node normal indices (0 entries) ---
    buf += CAFEBABE

    # --- Node child offsets (0+0 entries) ---
    buf += CAFEBABE

    # --- Node bounds (0 entries) ---
    buf += CAFEBABE

    # --- Node polys (0 bytes) ---
    buf += CAFEBABE

    # --- Line data (0 entries) ---
    # lineFlags: 0 bytes, lineXs: 0 bytes, lineYs: 0 bytes
    buf += CAFEBABE

    # --- Height map (1024 bytes) ---
    buf += bytes([0] * 1024)
    buf += CAFEBABE

    # --- Sprite coords (0 sprites) ---
    # spriteX, spriteY: 0 bytes each
    # spriteInfoLow: 0 bytes
    buf += CAFEBABE

    # --- Sprite info high (0 sprites) ---
    buf += CAFEBABE

    # --- Sprite Z (0 z-sprites) ---
    buf += CAFEBABE

    # --- Sprite info mid (0 z-sprites) ---
    buf += CAFEBABE

    # --- Static functions (12 ushorts) ---
    for _ in range(12):
        buf += struct.pack("<H", 0)
    buf += CAFEBABE

    # --- Tile events (0 entries) ---
    buf += CAFEBABE

    # --- Map bytecode (0 bytes) ---
    buf += CAFEBABE

    # --- Maya cameras (0 cameras, no data) ---
    # loadMayaCameras reads nothing when totalMayaCameras=0

    # --- Marker after maya cameras ---
    buf += CAFEBABE

    # --- Map flags (512 bytes, packed nibbles) ---
    # Each byte encodes flags for 2 tiles: low nibble = tile N, high nibble = tile N+1
    # Bit 0 of flags = solid/blocking
    flag_bytes = bytearray(512)
    for i in range(0, 1024, 2):
        y0, x0 = divmod(i, MAP_W)
        y1, x1 = divmod(i + 1, MAP_W)
        f0 = collision[y0][x0] & 0xF  # 1 = blocked
        f1 = collision[y1][x1] & 0xF
        flag_bytes[i // 2] = (f1 << 4) | f0
    buf += bytes(flag_bytes)
    buf += CAFEBABE

    with open(os.path.join(out_dir, "map00.bin"), "wb") as f:
        f.write(buf)

    print(f"  Generated map00.bin ({len(buf)} bytes)")


# ============================================================================
# Binary: Texture files
# ============================================================================

def generate_textures(out_dir):
    """Generate newMappings.bin, newPalettes.bin, newTexels000.bin."""

    # --- newMappings.bin (19,456 bytes) ---
    # mediaMappings: short[512]
    # mediaDimensions: ubyte[1024]
    # mediaBounds: short[4096]
    # mediaPalColors: int[1024]
    # mediaTexelSizes: int[1024]

    mappings = struct.pack("<" + "h" * 512, *([0] * 512))

    dimensions = bytearray(1024)
    # Image 0: 64x64 -> width nibble=6 (2^6=64), height nibble=6
    dimensions[0] = 0x66
    dimensions = bytes(dimensions)

    bounds = struct.pack("<" + "h" * 4096, *([0] * 4096))

    pal_colors = [0] * 1024
    pal_colors[0] = 16  # 16 colors in palette for image 0
    pal_colors = struct.pack("<" + "i" * 1024, *pal_colors)

    texel_sizes = [0] * 1024
    texel_sizes[0] = 64 * 64 - 1  # texel size = (value & 0x3FFFFFFF) + 1
    texel_sizes = struct.pack("<" + "i" * 1024, *texel_sizes)

    with open(os.path.join(out_dir, "newMappings.bin"), "wb") as f:
        f.write(mappings + dimensions + bounds + pal_colors + texel_sizes)

    # --- newPalettes.bin ---
    pal_data = bytearray()
    # Image 0 palette: 16 grayscale RGB565 colors
    for c in range(16):
        gray5 = (c * 2) & 0x1F
        gray6 = (c * 4) & 0x3F
        rgb565 = (gray5 << 11) | (gray6 << 5) | gray5
        pal_data += struct.pack("<H", rgb565)
    pal_data += DEADBEEF

    with open(os.path.join(out_dir, "newPalettes.bin"), "wb") as f:
        f.write(pal_data)

    # --- newTexels000.bin ---
    texel_data = bytearray()
    # Image 0: 64x64 checkerboard pattern
    for y in range(64):
        for x in range(64):
            if (x // 8 + y // 8) % 2 == 0:
                texel_data.append(4)   # medium gray
            else:
                texel_data.append(12)  # light gray
    texel_data += DEADBEEF

    with open(os.path.join(out_dir, "newTexels000.bin"), "wb") as f:
        f.write(texel_data)

    print("  Generated texture files")


# ============================================================================
# Main
# ============================================================================

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print(f"Generating demo game in {OUTPUT_DIR}/")

    # YAML configs
    print("  Generating YAML configs...")
    generate_game_yaml(OUTPUT_DIR)
    generate_characters_yaml(OUTPUT_DIR)
    generate_sounds_yaml(OUTPUT_DIR)
    generate_strings_yaml(OUTPUT_DIR)
    generate_entities_yaml(OUTPUT_DIR)
    generate_menus_yaml(OUTPUT_DIR)
    generate_tables_yaml(OUTPUT_DIR)

    # Binary assets
    print("  Generating binary assets...")
    generate_tables_bin(OUTPUT_DIR)
    generate_map00(OUTPUT_DIR)
    generate_textures(OUTPUT_DIR)

    # Sound files (silent WAVs)
    print("  Generating sound files...")
    generate_sound_files(OUTPUT_DIR)

    # BMP images
    generate_all_bmps(OUTPUT_DIR)

    # Count files
    total = sum(len(files) for _, _, files in os.walk(OUTPUT_DIR))
    print(f"\nDone! Generated {total} files in {OUTPUT_DIR}/")
    print(f"Run with: ./build/DoomIIRPG --game demo")


if __name__ == "__main__":
    main()
