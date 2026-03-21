# Doom II RPG String/Localization File Format

This document describes the binary format of the Doom II RPG string index and string chunk files based on reverse engineering of the game code.

## Overview

The localization system consists of an index file and multiple chunk files:

- **Index File**: `strings.idx`
- **Chunk Files**: `strings00.bin`, `strings01.bin`, `strings02.bin`
- **Location**: `Doom2rpg.app/Packages/`

Strings are organized into 15 text groups (indices 0-14), each supporting multiple languages. The index file maps each (language, group) pair to a chunk file, byte offset, and data size. The chunk files contain the raw null-terminated string data.

## Data Types

All multi-byte values are stored in **little-endian** format.

| Type | Size | Description |
|------|------|-------------|
| `ubyte` | 1 | Unsigned 8-bit integer |
| `short` | 2 | Signed 16-bit integer |
| `int` | 4 | Signed 32-bit integer |

## strings.idx Format

### Header (2 bytes)

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | short | count | Total number of index entries |

### Index Entry Array (5 bytes per entry)

Immediately follows the header. Repeated `count` times:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | ubyte | chunkId | Chunk file number (0 = `strings00.bin`, 1 = `strings01.bin`, etc.) |
| 1 | int | offset | Byte offset into the chunk file where this text group's data begins |

A special sentinel byte value of `0xFF` for `chunkId` marks a padding/skip entry that is not stored in the output array.

### Sentinel Entry (5 bytes)

After all `count` entries, one final 5-byte entry follows. Its `offset` field is used to compute the data size of the last real entry.

### Parsed Output

The index is parsed into a flat array of 3 integers per entry:

| Field | Description |
|-------|-------------|
| chunkId | Which chunk file to read from (0, 1, or 2) |
| offset | Byte offset within that chunk file |
| size | Number of bytes of string data (computed from the next entry's offset) |

The size of each entry is calculated as `nextOffset - currentOffset`, derived from the following entry's offset field.

### Index Addressing

Entries are addressed as `(language * MAXTEXT + textGroup) * 3`, where:
- `language` is the language index (0 = English)
- `MAXTEXT` is 15 (total text groups)
- The result indexes into the parsed 3-int-per-entry array

## String Chunk Files (strings00-02.bin)

Each chunk file contains raw string data: sequences of null-terminated strings packed contiguously.

When a text group is loaded, its byte range (offset + size from the index) is read from the appropriate chunk file into a buffer. A `textMap` array is then built by scanning for null terminators — each entry in `textMap` stores the offset of the start of each string within the buffer.

### String Counts Per Text Group

| Group | String Count | Description |
|-------|-------------|-------------|
| 0 | 253 | Core UI strings |
| 1 | 183 | Entity names, item names |
| 2 | 14 | Misc strings |
| 3 | 399 | Dialog/story text |
| 4 | 186 | Additional dialog |
| 5 | 155 | Additional dialog |
| 6 | 136 | Additional dialog |
| 7 | 158 | Additional dialog |
| 8 | 162 | Additional dialog |
| 9 | 100 | Additional dialog |
| 10 | 97 | Additional dialog |
| 11 | 40 | Additional dialog |
| 12 | 45 | Additional dialog |
| 13 | 15 | Additional dialog |
| 14 | 3 | Additional dialog |

Groups 0, 1, 3, and 14 are loaded at startup. Other groups are loaded on demand.

## String ID Encoding

String references throughout the codebase use a packed 16-bit format:

```
STRINGID(group, index) = (group << 10) | index
```

To decode:
- **group** = `(id >> 10) & 0x1F` (5 bits, range 0-31)
- **index** = `id & 0x3FF` (10 bits, range 0-1023)

## String Format

Strings support the following escape/formatting sequences:

| Sequence | Description |
|----------|-------------|
| `\%` | Literal `%` character |
| `\n` | Newline character |
| `%NN` | Argument substitution (2-digit, 1-indexed, e.g. `%01` = first arg) |
| `\|` | Pipe character (literal) |

### Special Characters

| Byte | Constant | Description |
|------|----------|-------------|
| `0x80` | C_LINE | Line separator |
| `0x84` | C_CURSOR2 | Cursor variant |
| `0x85` | C_ELLIPSES | Ellipsis character |
| `0x87` | C_CHECK | Checkmark |
| `0x88` | C_MINIDASH | Mini dash |
| `0x89` | C_GREYLINE | Grey line separator |
| `0x8A` | C_CURSOR | Cursor |
| `0x8B` | C_SHIELD | Shield icon |
| `0x8D` | C_HEART | Heart icon |
| `0x90` | C_POINTER | Pointer icon |
| `|` | NEWLINE | Line break in text rendering |

## Source References

This format was reverse-engineered from:
- `src/Resource.cpp` — `loadFileIndex()` index parsing
- `src/Text.cpp` — `Localization` class: `beginTextLoading()`, `loadTextFromIndex()`, `composeText()`
- `src/Text.h` — `Localization` and `Text` class definitions
- `src/Resource.h` — Resource file name constants
