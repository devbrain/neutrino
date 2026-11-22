# Coordinate spaces — plan

*Status: plan, agreed 2026-11-21 (render_model.md R40). Phase 1 implemented in lib_sdlpp
(2026-11-21, not committed); phases 2 to 5 to do.*

Every coordinate in the public API of lib_sdlpp and neutrino gets a space and a kind, and
crossing from one space to another is a named call. This document defines the spaces,
the kinds and the rule that picks a type, and plans the migration of lib_sdlpp, neutrino
and lib_nuklearpp. It builds on window_presentation.md, which set up `render_size()` and
the window-to-render mapping.

## Contents

1. [Why](#1-why)
2. [Spaces](#2-spaces)
3. [Kinds and their algebra](#3-kinds-and-their-algebra)
4. [The rule](#4-the-rule)
5. [Conversions](#5-conversions)
6. [lib_sdlpp](#6-lib_sdlpp)
7. [neutrino](#7-neutrino)
8. [Other projects](#8-other-projects)
9. [Order](#9-order)
10. [Open items](#10-open-items)

## 1. Why

The same question comes back with every new API: `point` or `render_pos`, int or float, a
position or an offset. Each answer is local, so the next API asks again, and answers
differ.

- **World space is typed** (`coordinates.hh`): `world_pos`, `world_delta`,
  `world_velocity` and `world_bounds`, with an algebra that only allows the operations
  that mean something. It ended the position-versus-velocity bug class in KE.
- **Render and window space are half typed:** `render_pos`, `render_rect` and
  `window_pos` exist, with no delta, no size and no algebra.
- **Everything else is untyped:** `point`, `rect`, `dim`, `fpoint`, `world_point` and
  `world_rect` are bare sdlpp aliases. `point` alone appears in 41 public headers.
- **sdlpp delivers bare floats.** Its events carry `float x, y, xrel, yrel`, and its draw
  calls take anything with an `x` and a `y` (`point_like`). Neutrino attaches the space
  late, in `input_snapshot`, and nothing checks it at the draw calls. Window coordinates
  handed to a draw call — the KE mouse bug — still compile.

*The goal:* a window coordinate cannot reach a draw call, and a world coordinate cannot
reach the renderer, without a visible conversion; and the type of a new field is decided
by a rule (§4), not by a discussion.

## 2. Spaces

| Space | What it is | Values | Owner |
|---|---|---|---|
| **display** | where windows sit on the desktop: a window's position, a display's bounds, the global mouse | int | sdlpp |
| **window** | SDL's window coordinates ("points"): what mouse, pen and drop events carry. Equal to the window's pixels unless its pixel density is not 1 (HiDPI) | float | sdlpp |
| **render** | what draw calls take: the bound target's coordinates after logical presentation, the viewport and the scale. Inside a texture target, its pixels; inside a layer at the scene's size, render pixels | float | sdlpp |
| **output** | the renderer's real output in pixels, ignoring logical presentation: `SDL_GetRenderOutputSize`, `SDL_RenderReadPixels`, the logical presentation rectangle | int | sdlpp |
| **texel** | pixels inside a texture: source rectangles, atlas rectangles, texture sizes | int (SDL also takes float source rectangles) | sdlpp |
| **uv** | fractions of a texture, 0 to 1 across it (beyond that with the wrap address mode): the texture coordinates of geometry | float | sdlpp |
| **normalized** | fractions of the window, 0 to 1: touch finger positions | float | sdlpp |
| **world** | gameplay and tile pixels; the camera maps it to render | float | neutrino |

**Not typed**, so they stay plain numbers, documented where they are declared: mouse
wheel scroll amounts, joystick ball motion, gamepad touchpad positions (fractions of the
touchpad, not of a window), pen axes and tile grid indices (§10).

*Why texture coordinates became a space after all* (2026-11-22): sdlpp's geometry calls
took `SDL_Vertex`, a raw SDL type. Its replacement, `sdlpp::vertex`, holds a `render_pos`, so
its texture coordinates are typed as well: a `uv_pos`, which `texture::to_uv()` makes from
texels and `texture::to_texel()` turns back.

*Why sdlpp owns all but world:* SDL defines those spaces and the mappings between them —
logical presentation, pixel density, the bound target, `SDL_RenderCoordinatesFromWindow`.
Typing them in sdlpp attaches the space where SDL delivers a value and checks it where SDL
consumes one. World space is neutrino's: SDL knows nothing of cameras.

## 3. Kinds and their algebra

Four kinds, each a template over a space tag, in a new sdlpp header:

```cpp
namespace sdlpp {
    struct display_space; struct window_space; struct render_space; struct output_space;
    struct texel_space;   struct uv_space;      struct normalized_space;

    template <class Space, class T = float> struct pos    { T x{}, y{}; };   // a place
    template <class Space, class T = float> struct delta  { T x{}, y{}; };   // a displacement
    template <class Space, class T = float> struct extent { T width{}, height{}; }; // a size
    template <class Space, class T = float> struct area   { T x{}, y{}, w{}, h{}; }; // a rectangle

    using window_pos   = pos<window_space>;     using window_delta  = delta<window_space>;
    using render_pos   = pos<render_space>;     using render_delta  = delta<render_space>;
    using render_rect  = area<render_space>;    using render_extent = extent<render_space>;
    using display_pos  = pos<display_space, int>;
    using texel_rect   = area<texel_space, int>; using texel_extent = extent<texel_space, int>;
    // ... one alias per space and kind in use
}

namespace neutrino {
    struct world_space;                         // neutrino adds its own space
    using world_pos   = sdlpp::pos<world_space>;
    using world_delta = sdlpp::delta<world_space>;
}
```

The algebra is written once, for every space:

| Expression | Result |
|---|---|
| `pos - pos` | `delta` |
| `pos + delta`, `pos - delta` | `pos` |
| `delta ± delta`, `-delta`, `delta * s`, `s * delta` | `delta` |
| `extent * s` | `extent` |
| `area{pos, extent}`, `a.origin()`, `a.size()`, `a.contains(pos)`, `a.intersects(area)`, `a + delta` | as named |
| `pos + pos`, `pos * s`, anything mixing two spaces | does not compile |

- **Int to float in the same space is implicit;** it loses nothing. Int and float values
  of one space mix in an expression and give float. **Float to int is explicit**, through
  `round_to_int`, `floor_to_int` and `ceil_to_int`; a rounded rectangle rounds its
  corners, so neighbours that share an edge still share it.
- **`extent` uses `width` / `height`**, as `sdlpp::size` and SDL do; `area` uses `x`, `y`,
  `w`, `h`, as `SDL_Rect` does.
- **`contains` is half-open** for int and float alike: a rectangle holds its left and top
  edges, not its right and bottom ones.
- **The float types have the layout of `SDL_FPoint` / `SDL_FRect`**, checked with
  `static_assert`, so they pass to SDL without copying; the int ones that of `SDL_Point` /
  `SDL_Rect`.
- **World-only types stay neutrino's:** `world_velocity`, with `delta / seconds` and
  `velocity * seconds`, and `world_bounds` (two corners), built on `world_pos`.
- **Entering a space from untyped data is explicit:** `render_pos{v.x, v.y}`, or a
  helper that names the space, for a GLM vector or a parsed value. sdlpp's geometry
  concepts (`point_like` and the others) and `geometry_algorithms.hh` stay, for untyped
  geometry; the renderer, the windows and the events no longer take untyped values (§6).

## 4. The rule

For every coordinate in a public API:

1. **Its space** is where the value lives (§2).
2. **Its kind:**
   - a place is a `pos`;
   - a displacement is a `delta`, and so is a point measured from another point — a pivot
     inside a destination rectangle, an anchor inside a sprite, an offset;
   - a size is an `extent`;
   - a rectangle is an `area`.
3. **Float or int:**
   - **float** for anything placed, moved or animated, and for what SDL delivers as float;
     rounding is the `pixel_snap` policy's job at the draw call, not the type's;
   - **int** for whole pixels by nature: texture and layer sizes, texel rectangles,
     display positions, the output size, `render_size()` (a logical size is whole).
4. **Crossing spaces** is a named conversion (§5); nothing converts implicitly between
   two spaces.

Applied to the questions that started this:

| Field | Type | Why |
|---|---|---|
| `layer_style::offset` | `render_delta` | a displacement, animated |
| `layer_style::pivot` | `std::optional<render_delta>` | measured from the destination's top-left; unset is the centre (R43) |
| `layer_style::source` | `std::optional<render_rect>`, or `texel_rect` for a layer with its own `size` | part of the layer: render units like everything drawn into a layer that mirrors the canvas (R43) |
| `layer_style::destination` | `std::optional<render_rect>` | where the layer lands (R43) |
| `layer_settings::size` | `std::optional<texel_extent>` | a texture size |
| `texture_draw_params::src` | `std::optional<texel_rect>` | part of a texture |
| `texture_draw_params::pivot` | `std::optional<render_delta>` | destination-local today, so a delta, not the `render_pos` it is declared as |
| `input_snapshot` pointer | `window_pos`, then `render_pos` through the renderer | as SDL delivers it, then converted |

## 5. Conversions

| From | To | Through | Notes |
|---|---|---|---|
| `window_pos` | `render_pos` | the renderer (`SDL_RenderCoordinatesFromWindow`) | logical presentation, pixel density, viewport, scale |
| `render_pos` | `window_pos` | the renderer (`SDL_RenderCoordinatesToWindow`) | the inverse |
| `window_delta` | `render_delta` | the renderer | the scale only, no letterbox offset: the difference of two converted points |
| normalized `pos` / `delta` | `window_pos` / `window_delta` | the window's size | touch fingers |
| `display_pos` | `window_pos` | the window's position | the global mouse |
| `output` | `render` | the logical presentation rectangle | reading the window's pixels under logical presentation |
| `world_pos` | `render_pos` | the camera, per layer for parallax | neutrino; a `world_delta` by the zoom only |

`SDL_ConvertEventToRenderCoordinates` is not wrapped: it rewrites an event's fields into
another space in place, which typed fields cannot express. Code converts the fields it
needs, as SDL's own documentation suggests for relative motion.

## 6. lib_sdlpp

A clean break, not a deprecation: sdlpp's renderer and events have two users left
(neutrino and lib_nuklearpp, §8), so they move with it.

**Phase 1 — the types** (adds; done 2026-11-21). The space tags, the four templates, the
aliases, the algebra and the layout checks, in `sdlpp/utility/coordinate_spaces.hh`; the
conversions of §5 on `renderer` (`to_render`, `to_window`, `to_output`,
`get_logical_presentation_rect`) and `window` (`to_window`, `to_display`); tests in
`test/utility/test_coordinate_spaces.cc`, including the forbidden expressions of §3, and
`test/video/test_coordinate_conversions.cc`. `namespace window_pos` became
`window_placement` here, not in phase 2: the new `window_pos` type needs the name, and
nothing outside `window.hh` used the namespace.

**Phase 2 — events and windows** (breaking).

- **Mouse:** motion carries a `window_pos` and a `window_delta`; a button event a
  `window_pos`; a wheel event its scroll amounts as plain floats and the pointer as a
  `window_pos`.
- **Pen and drop events:** `window_pos`.
- **Touch fingers:** a normalized `pos` and `delta`.
- **Window events:** an accessor per kind instead of `data1` / `data2`: moved gives a
  `display_pos`, resized a window `extent`, pixel size changed an output `extent`.
- **Windows:** the position is a `display_pos`, the size a window `extent`, the size in
  pixels an output `extent`. `sdlpp::position<T>` and `window_position`, used nowhere
  else, are replaced by `display_pos`.
- **Mouse state:** the mouse in a `window_pos`, the global mouse in a `display_pos`,
  relative motion in a `window_delta`.

**Phase 3 — the renderer** (breaking).

- **Draw and fill calls** take render-space types only; the `point_like` / `rect_like`
  overloads go.
- **Copies:** the source is a texel area (int, or float as SDL allows), the destination a
  `render_rect`, the rotation centre a `render_delta` from the destination's top-left.
- **Geometry** (done 2026-11-22): `sdlpp::vertex` holds a `render_pos`, an `fcolor` and a
  `uv_pos`, with `SDL_Vertex`'s layout; `render_geometry` takes it and a `const texture&`, or
  no texture.
- **Viewport and clip rectangle:** render areas, the clip relative to the viewport (§10).
- **Read pixels, the output size and the logical presentation rectangle:** output space.
  The current output size (adjusted by logical presentation) and the logical size: render
  space.
- **Docs:** the geometry, renderer and window guides, and a breaking entry in the
  changelog.

*Consequence:* sdlpp's geometry proposal (`geometry_refactoring_proposal.md`) aimed to let
any library's points reach the renderer through the concepts. That ends for the renderer:
another library's vectors enter render space explicitly (§3). The concepts keep their
place in untyped geometry.

## 7. neutrino

**Phase 4.** First an inventory: every public use of `point`, `rect`, `dim`, `line`,
`circle`, `fpoint`, `world_point` and `world_rect`, each classified by space and kind
(§4), recorded here before code changes. Then:

- **`coordinates.hh`:** `world_space`, with `world_pos` and `world_delta` on the sdlpp
  templates; `world_velocity` and `world_bounds` stay neutrino's; `render_pos`,
  `render_rect` and `window_pos` become aliases of sdlpp's, marked `sdlpp-ok` for the
  public-header seal.
- **`geometry_types.hh`:** `point`, `rect` and `dim` become render-space int types where
  they mean render pixels — `draw.hh`'s integer overloads; every other use is retyped;
  `fpoint`, `world_point` and `world_rect` go.
- **`globals.hh`:** the `fpoint` forms of `to_render_coords` / `to_window_coords` go, the
  typed ones forward to sdlpp's conversions; `render_size()` returns a render `extent`.
- **Input:** `input_snapshot` and `mouse_click` take the typed positions from the events.
- **Drawing:** `draw.hh`'s float overloads take render types (as today), the integer ones
  render int types; `texture_draw_params` as in §4.
- **World drawing:** the camera, `world_renderer`, `sprite_batch` and text layout map
  world to render through the camera (§5).
- **Layers:** `layer_settings` and `layer_style` as in §4.
- **Docs:** painter.md's signatures, window_presentation.md's boundary functions, and the
  examples.

## 8. Other projects

| Project | sdlpp renderer / event use | In the migration |
|---|---|---|
| lib_nuklearpp | its sdlpp backend, 28 files | yes, phase 5: events and renderer calls |
| onyx_ui | 9 files | no: to be retired |
| warlords | 43 files | no: to be ported to neutrino and rewritten |

onyx_ui and warlords stop compiling against sdlpp from phase 2 on. To keep building
meanwhile they pin sdlpp at `b8f0f74`, the last commit before the migration.

## 9. Order

1. This plan.
2. sdlpp phase 1, committed and pushed on its own: it breaks nothing.
3. **The compositor resumes** (painter.md, RM1 step 2), using the phase 1 types through
   neutrino's aliases.
4. sdlpp phases 2 and 3, then neutrino's phase 4 and lib_nuklearpp's phase 5 against
   them. Each commit builds and passes its own tests; nothing is pushed until all three
   build, then sdlpp first.

## 10. Open items

- **Viewport and clip units.** SDL's documentation does not say whether the viewport is
  in render or output coordinates under logical presentation; a phase 1 test settles it
  before phase 3 types them.
- **Names** of the conversion and rounding functions: as phase 1 has them (§3, §6),
  unless its review changes them.
- **Tile grid indices** are not pixel coordinates: their own integer type, or plain
  ints, decided in phase 4's inventory.
