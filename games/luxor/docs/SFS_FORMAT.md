# SFS (Single File System) File Format Documentation

## Overview
The `.sfs` format found in *Luxor Adventures* (`L2/resources.sfs`) is an **AidAim Single File System (SFS)** container (version 2.60).
It is a virtual filesystem container originally created by AidAim Software for Delphi / C++ Builder applications to pack entire directory hierarchies and assets into a single OS file.

In `resources.sfs`, it contains **6,751 entries** (6,385 files and 366 directories) totaling ~214 MB. All files are stored uncompressed in 4 KB chunk pages.

---

## File Structure

### Global Header (File Offset 0x00 - 0x14C)

| Offset | Size | Type | Description |
|---|---|---|---|
| `0x000` | 8 | ASCII | Magic signature: `"AAMVHFSS"` |
| `0x008` | 284 | Bytes | Reserved / volume header metadata |
| `0x124` | 4 | Float32 | SFS Version (e.g. `2.60`, stored as IEEE-754 float `0x40266666`) |
| `0x128` | 4 | UInt32 | Chunk size in bytes (always `4096` / `0x1000`) |
| `0x12C` | 20 | Bytes | Reserved |
| `0x140` | 4 | UInt32 | Root directory tree chunk index (e.g. `4`) |
| `0x144` | 4 | UInt32 | Total number of tree entries (e.g. `6751`) |
| `0x148` | 4 | UInt32 | Total chunk count in container (e.g. `54561`) |

All multi-byte integers are stored in little-endian format.

---

## Chunk Organization

The container is partitioned into contiguous chunks of fixed size (`chunk_size = 4096` bytes).
A chunk index $C$ begins at byte offset:
$$\text{offset} = C \times 4096$$

### Chunk Header Layout (32 bytes at offset 0x118 inside each chunk)
Unlike file formats that place headers at offset 0, SFS places a 32-byte chunk management header in the interior of the chunk between `0x118` and `0x138`:

| Offset in Chunk | Size | Type | Description |
|---|---|---|---|
| `0x118` | 4 | UInt32 | Next chunk index in linked list (`0` or terminal value if end) |
| `0x11C` | 4 | UInt32 | Block checksum / magic |
| `0x120` | 24 | Bytes | Block flags and internal allocation metadata |

### Chunk Payload Layout
- **First Data Chunk of a File:**
  - `0x000 .. 0x137` (312 bytes): Unused / reserved header space.
  - `0x138 .. 0x1000` (3,784 bytes): Data payload begins here.
- **Subsequent Data Chunks:**
  - `0x000 .. 0x117` (280 bytes): First part of data payload.
  - `0x118 .. 0x137` (32 bytes): Internal chunk header (skipped during data reading).
  - `0x138 .. 0x1000` (3,784 bytes): Second part of data payload.
  - Total payload per continuation chunk: $280 + 3784 = 4064$ bytes ($\text{chunk\_size} - 32$).

---

## Directory Tree

The directory tree starts at `tree_address` (chunk 4) and is chained across multiple chunks using the `next_chunk` pointer at offset `0x118`.

Each tree chunk stores up to **7 entries** of 512 bytes (`0x200`) each:
$$\text{Entry } j \text{ starts at } 0\text{x}138 + j \times 0\text{x}200 \quad (j = 0 \dots 6)$$

The total number of chunks used by the directory tree is:
$$\text{tree\_chunks} = \lceil \text{n\_tree\_items} / 7 \rceil \quad (\text{e.g. } \lceil 6751 / 7 \rceil = 965)$$

### Directory Entry Structure (512 bytes)

| Offset | Size | Type | Description |
|---|---|---|---|
| `0x000` | 4 | Int32 | Pointer table chunk index (`-1` for directory or 0-byte file) |
| `0x004` | 8 | UInt64 | Uncompressed file size in bytes |
| `0x00C` | 8 | UInt64 | File creation time (Windows `FILETIME`) |
| `0x014` | 8 | UInt64 | File modification time (Windows `FILETIME`) |
| `0x01C` | 8 | UInt64 | File access time (Windows `FILETIME`) |
| `0x024` | 4 | UInt32 | Permissions / file attributes |
| `0x028` | 4 | Int32 | Parent directory index (`-1` for root directory items) |
| `0x02C` | 176 | Bytes | Reserved |
| `0x0DC` | 1 | Bool | `is_directory` (`1` = directory, `0` = file) |
| `0x0DD` | 3 | Bytes | Padding |
| `0x0E0` | 256 | String | Entry name (null-terminated ASCII / Latin-1 string) |
| `0x1E0` | 32 | Bytes | Reserved |

---

## File Data Retrieval

To read a regular file with `size > 0`:

1. **Read Chunk Pointer Table:**
   - Seek to chunk `ptr_to_ptr` at offset `ptr_to_ptr * 4096 + 0x138`.
   - The required number of data chunks is:
     - If $\text{size} \le 3784$: $1$ chunk.
     - If $\text{size} > 3784$: $1 + \lceil (\text{size} - 3784) / 4064 \rceil$ chunks.
   - Read $N$ little-endian `uint32` chunk indices from this pointer table.

2. **Assemble Payload:**
   - For chunk 0:
     $$\text{payload}_0 = \text{chunk}[0\text{x}138 : 0\text{x}1000] \quad (\le 3784 \text{ bytes})$$
   - For chunk $k \ge 1$:
     $$\text{payload}_k = \text{chunk}[0\text{x}000 : 0\text{x}118] + \text{chunk}[0\text{x}138 : 0\text{x}1000] \quad (4064 \text{ bytes})$$
   - Concatenate all payloads and truncate to `file_size` bytes.

---

## File Types in Luxor Adventures L2 resources.sfs

The archive contains 6,385 game assets:
- **.png** (3,995 files): UI graphics, sprites, hidden object items
- **.jp2** (1,004 files): JPEG 2000 high-resolution background artwork
- **.ani** (872 files): Animation script/definition files
- **.ogg** (266 files): Music tracks and sound effects (Ogg Vorbis)
- **.loc** (130 files): Localization strings
- **.cfg** (58 files): XML/text configuration definitions
- **.win** (34 files): XML UI window definitions
- **.fnt** (22 files): Font descriptors
