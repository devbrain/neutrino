# SPR (MumboJumbo Sprite) File Format Documentation

## Overview
In the MumboJumbo game engine (used across the *Luxor* series, *Luxor: Amun Rising*, *Luxor 2*, *Luxor HD*, *Luxor Evolved*, and *Luxor Mahjong*), `.spr` files define 2D sprites, animated sprite sheets, UI buttons, visual particles, and map overlay masks.

Rather than storing raw pixel data inside the `.spr` file, the file acts as a sprite descriptor that references external image files (such as `.jpg`, `.tga`, `.png`, `.dds`, or `.pvr`) and maps sprite dimensions, animation sequences, frame UV rectangles, and button interaction states.

Over the evolution of the engine, the `.spr` format progressed through **three generations**:

1. **Classic Line-Based ASCII Format** (*Luxor*, *Luxor: Amun Rising*, *Luxor Mahjong*, UI assets)
2. **Declarative Text Block Format** (*Luxor HD*, *Luxor Amun Rising HD*, *Luxor Evolved*)
3. **Compiled Binary Format (`GVF002`)** (*Luxor 2*, *Luxor 2 HD*)

---

## 1. Classic Line-Based ASCII Format

This is the primary format used in *Luxor* (2005) and *Luxor: Amun Rising* (2006). It is a simple, line-delimited plain text file parsed sequentially by `gfx2d.dll` (`gfxSprite::load`).

### File Layout

```text
Line 1: <DiffuseBitmapPath>
Line 2: <AlphaBitmapPath>
Line 3: <FrameWidth> <FrameHeight>
Line 4: <AnimCount>
[Followed by AnimCount animation blocks]
```

### Field Definitions

| Line | Field | Type | Description |
|---|---|---|---|
| **1** | `DiffuseBitmapPath` | String | Relative path to the base diffuse/color texture (e.g. `data/bitmaps/game/ball_1.jpg`). |
| **2** | `AlphaBitmapPath` | String | Relative path to the 8-bit grayscale alpha mask texture (e.g. `data/bitmaps/game/ball_alpha.tga`), or the literal word `none` if no alpha channel is used. |
| **3** | `FrameWidth FrameHeight` | Integer Pair | Width and height of an individual sprite frame in pixels (e.g. `32 32` or `100 27`). |
| **4** | `AnimCount` | Integer | Total number of animation states or button interaction states defined in this file. |

### Animation Block Definitions

Following Line 4, there are `<AnimCount>` animation sequence descriptors. Each animation block starts with a frame count:

```text
Line A: <FrameCount> [ModeFlag]
Line B: <StartX> <StartY>
```

#### Mode 1: Implicit Horizontal Layout (Standard)
When no mode flag is present, only a single coordinate pair `StartX StartY` is provided:
- Frame 0 is located at pixel `(StartX, StartY)`.
- Subsequent frames $i \in [1, \text{FrameCount}-1]$ are arranged consecutively horizontally across the sprite sheet:
  $$\text{Frame}_i.X = \text{StartX} + (i \times \text{FrameWidth})$$
  $$\text{Frame}_i.Y = \text{StartY}$$

#### Mode 2: Explicit Multi-Frame Layout (`X` Flag)
When the mode flag `X` follows `FrameCount` (e.g. `4 X`), each frame's coordinate is specified explicitly on its own subsequent line:
```text
<FrameCount> X
<X_0> <Y_0>
<X_1> <Y_1>
...
<X_N-1> <Y_N-1>
```

---

### Common Classic Patterns

#### Pattern A: Animated Particles / Rotating Spheres (6 Lines)
A rotating sphere or multi-frame effect stored as a horizontal strip on a texture:

```text
data/bitmaps/game/ball_1.jpg
data/bitmaps/game/ball_alpha.tga
32 32
1
32
0 0
```
- **Image Size**: $1024 \times 32$ pixels.
- **Frames**: 32 frames of $32 \times 32$, starting at $(0, 0)$ and stepping horizontally by $+32\text{px}$ each frame.

#### Pattern B: UI Buttons (12 Lines)
UI buttons define 4 distinct interaction animations/states (`AnimCount = 4`), with 1 frame each:
- `Anim 0`: Normal / Idle state
- `Anim 1`: Hover / Mouse-Over state
- `Anim 2`: Pressed / Down state
- `Anim 3`: Disabled state

Example (`assets/button_moregames.spr`):
```text
assets/button_moregames.jpg
none
233 51
4
1
0 0
1
0 51
1
0 102
1
0 153
```
- **Image Size**: $233 \times 204$ pixels ($51 \times 4 = 204$).
- The 4 states are stacked vertically at $Y = 0$, $Y = 51$, $Y = 102$, and $Y = 153$.

#### Pattern C: Static Map Masks / Backgrounds (6 Lines)
```text
data/maps/TheWrathOfSet/tunnel1.jpg
data/maps/TheWrathOfSet/tunnel1_alpha.tga
64 64
1
1
0 0
```
- A single static $64 \times 64$ overlay sprite at $(0, 0)$.

---

## 2. Declarative Text Block Format

Introduced in *Luxor HD*, *Luxor: Amun Rising HD*, and *Luxor Evolved*, this format replaces fixed line offsets with a structured, nested hierarchical grammar.

### Syntax Structure

```text
Material_0 = ~<TexturePath>[*<AlphaPath>]

Anim <AnimationName>
{
    SubAnim <SubAnimationName>
    {
        Frame
        {
            Material = <MaterialIndex>
            Rect = [ <U_Left>, <V_Top>, <U_Right>, <V_Bottom> ]
        }
        ...
    }
}
```

### Syntax Details

- **Material Definition**:
  - `Material_0 = ~data/textures/game/vise.png`: Relative texture file path (prefixed with `~`).
  - `Material_0 = ~diffuse.jpg*alpha.tga`: An asterisk `*` combines a RGB JPEG with a separate grayscale alpha texture.
- **Normalized Texture Coordinates (UV)**:
  - `Rect = [ U_min, V_min, U_max, V_max ]` specifies normalized floating-point UV coordinates in the range $[0.0, 1.0]$.
- **Hierarchical Structure**:
  - `Anim`: High-level state (e.g. `Idle`, `Hover`, `Walk`, `HyperSpace`).
  - `SubAnim`: Sub-state, direction, or loop mode (e.g. `Stop`, `Normal`, `Implicit`, `Center`).
  - `Frame`: Individual rendering frame referencing a `Material` index and UV `Rect`.

### Example (`Luxor Evolved` shooter sprite):
```text
Material_0 = ~data/textures/game/shooter/normal/shooter.png
Anim normal
{
    SubAnim idle
    {
        Frame
        {
            Material = 0
            Rect = [ 0.250000, 0.666667, 0.500000, 1.000000 ]
        }
    }
}
Anim hyperspace
{
    SubAnim idle
    {
        Frame
        {
            Material = 0
            Rect = [ 0.500000, 0.333333, 0.750000, 0.666667 ]
        }
    }
}
```

---

## 3. Compiled Binary Format (`GVF002`)

Used in *Luxor 2* and *Luxor 2 HD*, `GVF002` (**Game Visual Format v2**) is a pre-compiled binary tokenized AST of the Declarative Block Format.

### Binary Header (8 bytes)
```
Offset  Size  Description
0x00    1     Marker byte (0x01)
0x01    6     Magic signature ASCII: "GVF002"
0x07    1     Marker byte (0x01)
```

### String Dictionary Table
Immediately follows the header at offset `0x08`:
- `uint32`: Number of strings in dictionary ($N$).
- Repeated $N$ times:
  - `uint32`: String length $L$.
  - `char[L]`: ASCII string (e.g. `Rect`, `SubAnim`, `Frame`, `Material_0`, `Anim`, texture paths).

### Data Stream
- **Separator**: 8 bytes of `0xFF` (`FFFFFFFF FFFFFFFF`).
- **Bytecode Stream**: A sequential bytecode stream with 3 opcodes representing the AST:

#### Opcodes

| Opcode | Name | Operands | Description |
|---|---|---|---|
| `0x11` | `BLOCK_START` | `int32 name_id`<br>`int32 type_id` | Opens a block `{`. If `type_id != -1`, emits `<type_str> <name_str> {`. If `type_id == -1`, emits `<name_str> {` (e.g. `Frame {`). |
| `0x12` | `BLOCK_END` | *None* | Closes a block `}`. |
| `0x13` | `PROPERTY` | `int32 key_id`<br>`uint8 val_type`<br>`uint8 val_subtype`<br>`data...` | Assigns property `<key_str> = <value>`. |

#### Value Types for Opcode `0x13`:

| `(val_type, val_subtype)` | Target Type | Value Encoding | Typical Properties |
|---|---|---|---|
| `(8, 1)` | String | `uint32 length` followed by ASCII bytes | `Material_0`, `Material_1`, etc. |
| `(2, 1)` | 32-bit Int | `int32` little-endian | `Material` index |
| `(0, 1)` | Boolean / Byte | `uint8` (`0` or `1`) | `Filter` |
| `(3, 4)` | 4D Float Vector | `float[4]` little-endian (16 bytes) | `Rect = [ U_min, V_min, U_max, V_max ]` |

---

### Decompiler Tool (`gvf_decompile`)

A C++ decompiler tool `gvf_decompile` is provided to convert `GVF002` binary files back into readable Generation 2 text format:

```bash
# Decompile single file to stdout:
./gvf_decompile pyramid.spr

# Decompile to target output file:
./gvf_decompile pyramid.spr pyramid_decompiled.spr

# Batch decompile an entire extracted directory in-place:
./gvf_decompile -r ./extracted_sprites --in-place
```

---

## 4. Implementation Reference

### Classic Format Parser (C++17)

```cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

struct SpriteFrame {
    int x;
    int y;
};

struct SpriteAnim {
    std::vector<SpriteFrame> frames;
};

struct ClassicSprite {
    std::string diffuse_texture;
    std::string alpha_texture;
    bool has_alpha = false;
    int frame_width = 0;
    int frame_height = 0;
    std::vector<SpriteAnim> anims;

    static bool load(std::istream& in, ClassicSprite& spr) {
        std::string line;
        if (!std::getline(in, spr.diffuse_texture)) return false;
        if (!std::getline(in, spr.alpha_texture)) return false;

        // Strip carriage returns
        if (!spr.diffuse_texture.empty() && spr.diffuse_texture.back() == '\r') spr.diffuse_texture.pop_back();
        if (!spr.alpha_texture.empty() && spr.alpha_texture.back() == '\r') spr.alpha_texture.pop_back();
        spr.has_alpha = (spr.alpha_texture != "none" && !spr.alpha_texture.empty());

        if (!(in >> spr.frame_width >> spr.frame_height)) return false;
        int anim_count = 0;
        if (!(in >> anim_count)) return false;

        spr.anims.resize(anim_count);
        for (int a = 0; a < anim_count; ++a) {
            int frame_count = 0;
            std::string extra;
            in >> frame_count;
            std::getline(in, extra); // check for 'X' flag

            bool is_explicit = (extra.find('X') != std::string::npos || extra.find('x') != std::string::npos);
            spr.anims[a].frames.resize(frame_count);

            if (!is_explicit) {
                int start_x = 0, start_y = 0;
                in >> start_x >> start_y;
                for (int f = 0; f < frame_count; ++f) {
                    spr.anims[a].frames[f].x = start_x + (f * spr.frame_width);
                    spr.anims[a].frames[f].y = start_y;
                }
            } else {
                for (int f = 0; f < frame_count; ++f) {
                    in >> spr.anims[a].frames[f].x >> spr.anims[a].frames[f].y;
                }
            }
        }
        return true;
    }
};
```

---

## 5. Summary Matrix

| Engine Feature | Generation 1 (Classic) | Generation 2 (`GVF002`) | Generation 3 (Declarative) |
|---|---|---|---|
| **Encoding** | ASCII (Line-delimited) | Binary (Bytecode) | ASCII (Block-syntax `{}`) |
| **Coordinate System** | Integer Pixels | Normalized Floats `[0..1]` | Normalized Floats `[0..1]` |
| **Alpha Handling** | Line 2 path or `"none"` | `diffuse*alpha` syntax | `diffuse*alpha` syntax |
| **Supported Games** | Luxor 1, Luxor AR, Mahjong | Luxor 2, Luxor 2 HD | Luxor HD, AR HD, Luxor Evolved |
