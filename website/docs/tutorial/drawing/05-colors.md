---
sidebar_position: 5
title: "Part 5: Colors"
description: Mix, premultiply, grayscale, add and scale colours, and move between 8-bit and float colour, with a runnable demo of each operation.
---

# Part 5: Colors

Every part of this series has passed colours to draw calls. This part looks at the colours themselves: how to blend two of them, fade one out, take its brightness, and do colour maths in floats.

You will build a single screen with six rows, one per operation:

| Row | Operation | What you see |
|---|---|---|
| 1 | `mix()` / `lerp()` | A gradient from a FROM colour to a TO colour, and a swatch sliding along it |
| 2 | `premultiply()` | Straight alpha over a checkerboard, above the premultiplied colour |
| 3 | `luminance()` / `to_grayscale()` | Palette colours above their perceived-brightness grey |
| 4 | `operator+` | Red, green and blue light adding up to white, then a dim colour accumulating |
| 5 | `operator*` | A brightness ramp from ×0 to ×2 |
| 6 | `color` ↔ `fcolor` | An animation computed in floats, and the truncation the conversion performs |

The full source is `tutorials/drawing/05_colors/main.cc`. The reference for everything here is the [Colors section of the Drawing concept page](../../concepts/drawing.md#1-colors-neutrinovideocolorhh).

---

## The two colour types

```cpp
#include <neutrino/video/color.hh>

neutrino::color  c{255, 128, 0};          // 8-bit channels, alpha defaults to 255
neutrino::fcolor f{1.0f, 0.5f, 0.0f};     // float channels in 0..1
neutrino::color  named = neutrino::colors::cornflower_blue;
```

The draw API takes `color`. `fcolor` is for maths. The palette uses **X11** values, so `colors::gray` is 190, not CSS's 128, and `colors::green` is pure (0, 255, 0).

The demo keeps a small palette and two indices into it:

```cpp
constexpr std::array palette{
    neutrino::colors::crimson,  neutrino::colors::tomato,     neutrino::colors::orange,
    neutrino::colors::gold,     neutrino::colors::lime_green, neutrino::colors::forest_green,
    // ...
};

if (in.pressed(neutrino::key::right)) m_from = (m_from + 1) % palette.size();
if (in.pressed(neutrino::key::up))    m_to = (m_to + 1) % palette.size();
```

---

## Row 1: `mix()` and `lerp()`

`a.mix(b, t)` moves from `a` (at `t = 0`) to `b` (at `t = 1`) on every channel. `neutrino::lerp(a, b, t)` is the same as a free function.

```cpp
for (int i = 0; i < steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps - 1);
    swatch(left + i * cell, y, cell, row_height, from().mix(to(), t));
}

const float t = wave(4.0f);   // 0..1..0 every four seconds
swatch(x, y, w, row_height, neutrino::lerp(from(), to(), t));
```

Use it for fades, damage flashes, day/night tints and health bars that shift from green to red. The 8-bit result truncates: halfway between black and white is 127.

---

## Row 2: `premultiply()`

A colour with alpha below 255 only looks translucent if the renderer blends. Primitives start with blending off, so the demo turns it on for the top half of the row:

```cpp
neutrino::set_draw_blend_mode(neutrino::blend_mode::blend);
// ... translucent swatches over the checkerboard ...
neutrino::set_draw_blend_mode(neutrino::blend_mode::none);
```

`premultiply()` multiplies r, g and b by alpha and keeps alpha. Drawn opaque, the bottom half therefore shows what each straight-alpha swatch would look like over black:

```cpp
neutrino::color pre = c.premultiply();   // rgb scaled by alpha
pre.a = 255;
swatch(x, y + half, cell, half, pre);
```

Premultiplied colours are what premultiplied blend modes and many image pipelines expect.

---

## Row 3: `luminance()` and `to_grayscale()`

`luminance()` is the perceived brightness, using the BT.709 weights: green counts most, blue least. `to_grayscale()` writes it into all three channels and keeps alpha.

```cpp
swatch(x, y,        cell - 2, half, palette[i]);
swatch(x, y + half, cell - 2, half, palette[i].to_grayscale());

const int tick = palette[i].luminance() * (cell - 2) / 255;
```

Notice that gold and lime green come out far brighter than royal blue, even though all three look saturated. A common use: pick black or white text depending on whether the background's luminance is above 128.

---

## Row 4: `operator+`

`+` adds channel by channel and saturates at 255, the way light adds up:

```cpp
const std::array segments{red, red + green, red + green + blue, green + blue, blue};
```

Red plus green is yellow; all three are white. Alpha is added too, so adding to an opaque colour keeps it opaque.

The right half adds a dim colour to itself again and again. Each channel climbs until it clips at 255, so the hue drifts towards white: the same thing that happens when glow effects stack.

```cpp
const neutrino::color dim = from() * 0.25f;
neutrino::color acc = dim;
for (...) {
    swatch(x, y, cell - 2, row_height, acc);
    acc = acc + dim;
}
```

---

## Row 5: `operator*`

`c * f` scales r, g and b by `f`, clamps to 255 and leaves alpha alone. Below 1 it darkens, above 1 it brightens:

```cpp
for (int i = 0; i < steps; ++i) {
    const float factor = static_cast<float>(i) * 0.1f;   // 0.0 .. 2.0
    swatch(left + i * cell, y, cell, row_height, from() * factor);
}
```

The outlined cell is ×1.0, the original colour. Brightening saturates: once a channel reaches 255 it stops, so a bright colour washes out instead of getting more intense. Write `color * float`; `float * color` is not defined.

---

## Row 6: `color` and `fcolor`

When colour maths wants continuous values (square roots, smooth curves, gamma), compute in `fcolor` and convert once at the end. Both directions are explicit:

```cpp
const neutrino::fcolor a{from()};
const neutrino::fcolor b{to()};
const float s = 0.5f + 0.5f * std::sin(m_time * 3.0f);
const neutrino::fcolor pulse{
    a.r + (b.r - a.r) * s,
    std::sqrt(a.g * b.g),          // a float-only operation
    a.b * (1.0f - s) + b.b * s,
    1.0f,
};
swatch(left, y, 240, row_height, neutrino::color{pulse});
```

The middle pair of swatches shows the catch. Converting a float to a byte clamps and **truncates**, so `colors::f::gray` (0.5) becomes 127. Next to it, `colors::gray` from the X11 palette is 190: two "grays" that look nothing alike.

The right pair is a `color` → `fcolor` → `color` round trip, which always returns the original colour.

---

## Build and run

```bash
cmake --build build --target neutrino_tutorial_drawing_05_colors
./build/bin/neutrino_tutorial_drawing_05_colors
```

| Key | Action |
|---|---|
| Left / Right | Change the FROM colour |
| Up / Down | Change the TO colour |
| Space | Pause the animation |
| Escape | Quit |

---

## Summary

- `color` is what you draw with; `fcolor` is for maths; convert explicitly, once.
- `mix` / `lerp` blend, `+` adds light, `*` changes brightness, `premultiply` folds alpha into rgb, `luminance` / `to_grayscale` measure brightness.
- Translucent primitives need `set_draw_blend_mode(blend_mode::blend)`.
- The palette is X11: check the value when the exact shade matters.

Back to the **[Drawing Tutorial overview](./index.md)**, or on to the **[Drawing Concept Page](../../concepts/drawing.md)** for the full reference.
