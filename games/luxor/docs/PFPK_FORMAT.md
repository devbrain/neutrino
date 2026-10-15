# PFPK / PFP (Package File) File Format Documentation

## Overview

The `.pfp` format found in *Luxor: 5th Passage* (`assets/assets.pfp`) is a custom flat package archive identified by the ASCII magic signature **`PFPK`**.

It serves as the main resource container for the game, packing audio files (`.ogg`), textures (`.png`), level definitions (`.xml`), shaders, and interface definitions.

In `assets.pfp`, it contains **1,968 files** totaling **70,880,580 bytes (~70.8 MB)**. All files are stored uncompressed in sequential, contiguous order.

---

## File Layout

The archive consists of three contiguous segments:
1. **Header (8 bytes)**: Magic identifier and total file count.
2. **Table of Contents (TOC)**: Sequential variable-length directory entries.
3. **Data Payload**: Raw file data packed back-to-back without padding.

```
+-------------------------------------------------------------+
| Header (8 bytes)                                            |
|  - Magic: "PFPK" (4 bytes)                                  |
|  - File Count: uint32_t (4 bytes)                           |
+-------------------------------------------------------------+
| Table of Contents (Variable Length, 8 .. Offset 74,997)      |
|  - Entry 0: [name_len: 1B] [name: LB] [offset: 4B] [size: 4B] |
|  - Entry 1: [name_len: 1B] [name: LB] [offset: 4B] [size: 4B] |
|  ...                                                        |
|  - Entry N-1                                                |
+-------------------------------------------------------------+
| Data Payload (Offset 74,997 .. EOF)                         |
|  - Raw File 0 data (size bytes)                             |
|  - Raw File 1 data (size bytes)                             |
|  ...                                                        |
|  - Raw File N-1 data (size bytes)                           |
+-------------------------------------------------------------+
```

---

## Header Structure

The header starts at byte offset `0x00`:

| Offset | Size | Type | Description |
|---|---|---|---|
| `0x00` | 4 | ASCII | Magic signature: `"PFPK"` (`0x50 0x46 0x50 0x4B`) |
| `0x04` | 4 | UInt32 (LE) | Number of files in the archive (e.g. `1968` / `0x000007B0`) |

All multi-byte integers are stored in **little-endian** byte order.

---

## Table of Contents (TOC)

The Table of Contents begins immediately after the header at offset `0x08`. It contains exactly `num_files` sequentially packed entries.

Each entry has a variable size determined by its filename length:

| Field | Size | Type | Description |
|---|---|---|---|
| `name_len` | 1 | UInt8 | Length $L$ of the filename path string in bytes |
| `name` | $L$ | ASCII / UTF-8 | Relative file path (no null terminator, forward slashes `/`) |
| `offset` | 4 | UInt32 (LE) | Absolute byte offset of file data from the start of the archive |
| `size` | 4 | UInt32 (LE) | Size of the file in bytes |

### Entry Characteristics
- **Length Prefix**: Filename length is a single unsigned 8-bit integer ($0 < L \le 255$).
- **No Padding**: Fields are packed tightly with no alignment padding between entries.
- **Directory Hierarchy**: Directory paths are encoded directly into the filename string using forward slashes (e.g., `audio/music/blast_mode.ogg`, `blast_mode_levels/baran/easy.xml`).
- **No Directory Entries**: The TOC only stores file leaves; intermediate directories are implicit.

---

## Data Payload Section

The file data section begins immediately following the last entry of the TOC:
- In `assets.pfp`, the TOC occupies bytes `0x08` through `74,996` (totaling 74,989 bytes).
- The first file (`audio/music/blast_mode.ogg`) begins at offset `74,997`.
- Files are placed contiguously: for any file $i$, $\text{offset}_{i+1} = \text{offset}_i + \text{size}_i$.
- The final file ends exactly at the total archive file size (`70,880,580` bytes).

### Compression
- Files are **uncompressed** (`entry_type::Uncompressed`).
- Data bytes can be directly streamed or mapped from the container without decompression buffers.

---

## Comparison with Other Luxor Archive Formats

| Attribute | PFPK (`.pfp`) | NHP (`.nhp`) | NPK (`.npk`) | MJZ (`.mjz`) | SFS (`.sfs`) |
|---|---|---|---|---|---|
| **Used In** | *Luxor 5* | *Luxor 3*, *Adventures L1* | *Luxor 4* | *Luxor 1, 2, AR, HD, Evolved* | *Luxor Adventures L2* |
| **Magic** | `"PFPK"` | `"NovaHackPack"` | `"NovaPack"` | `"MJZ0"` | `"AAMVHFSS"` |
| **TOC Location** | Header (offset 8) | End of file | Offset 8 | Header / End | Linked 4KB chunk tree |
| **Compression** | Uncompressed | Uncompressed | LZMA / Uncompressed | Deflate (zlib) | Uncompressed (chunked) |
| **Directory Marker** | None | `"DictionaryBegin"` | None | None | Node chunk type |
| **Name Encoding** | 1-byte length prefix | 2-byte length prefix | 1-byte length prefix | Null-terminated string | Pascal string in tree |

---

## Reference Implementation

### Probing
```cpp
bool probe_pfp(std::istream& is, uint64_t file_size) {
    if (file_size < 8) return false;
    const auto pos = is.tellg();
    char magic[4]{};
    bool ok = false;
    if (is.read(magic, 4) && std::memcmp(magic, "PFPK", 4) == 0) {
        uint32_t num_files = 0;
        if (is.read(reinterpret_cast<char*>(&num_files), 4)) {
            // Sanity check: num_files > 0 and fits minimum TOC size (9 bytes per entry)
            ok = (num_files > 0 && 8 + static_cast<uint64_t>(num_files) * 9 <= file_size);
        }
    }
    is.clear();
    is.seekg(pos, std::ios::beg);
    return ok;
}
```

### Loading
```cpp
std::vector<entry> load_pfp(std::istream& is, uint64_t file_size) {
    char magic[4]{};
    if (!is.read(magic, 4) || std::memcmp(magic, "PFPK", 4) != 0) {
        throw std::runtime_error("Invalid PFP header magic (expected PFPK)");
    }

    uint32_t num_files = 0;
    if (!is.read(reinterpret_cast<char*>(&num_files), 4)) {
        throw std::runtime_error("Failed to read PFP file count");
    }

    std::vector<entry> entries;
    entries.reserve(num_files);

    for (uint32_t i = 0; i < num_files; ++i) {
        uint8_t name_len = 0;
        is.read(reinterpret_cast<char*>(&name_len), 1);

        std::string name(name_len, '\0');
        is.read(name.data(), name_len);

        uint32_t offset = 0;
        uint32_t size = 0;
        is.read(reinterpret_cast<char*>(&offset), 4);
        is.read(reinterpret_cast<char*>(&size), 4);

        entries.push_back(entry{
            .name = sanitize_path(name),
            .type = entry_type::Uncompressed,
            .offset = offset,
            .size = size,
            .zsize = size
        });
    }

    return entries;
}
```
