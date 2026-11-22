# Painter — implementation guide

The painter is neutrino's only drawing interface. This guide is the reference for building
it: the model, the public contract, the internals and the milestones, each with its meaning
and the reason for it. How the design was reached — alternatives, history, open questions —
is in [`render_model.md`](render_model.md).

## Contents

1. [Model](#1-model)
   - [1.1 The painter in brief](#11-the-painter-in-brief)
   - [1.2 Vocabulary](#12-vocabulary)
   - [1.3 Levels](#13-levels)
2. [Public contract](#2-public-contract)
   - [2.1 Two rules](#21-two-rules)
   - [2.2 What stays as it is](#22-what-stays-as-it-is)
   - [2.3 Defaults](#23-defaults)
   - [2.4 Painter state](#24-painter-state)
   - [2.5 Layers](#25-layers)
   - [2.6 The canvas](#26-the-canvas)
   - [2.7 Composite settings](#27-composite-settings)
   - [2.8 Alpha and blend modes](#28-alpha-and-blend-modes)
   - [2.9 The compositor](#29-the-compositor)
   - [2.10 Scenes](#210-scenes)
   - [2.11 The pinned scene](#211-the-pinned-scene)
   - [2.12 Rendering into a texture](#212-rendering-into-a-texture)
   - [2.13 Quads](#213-quads)
   - [2.14 Inspection](#214-inspection)
   - [2.15 Errors](#215-errors)
   - [2.16 Below the painter](#216-below-the-painter)
3. [Internals](#3-internals)
   - [3.1 Data](#31-data)
   - [3.2 A frame](#32-a-frame)
   - [3.3 Targets and the texture allocator](#33-targets-and-the-texture-allocator)
   - [3.4 The layer stack](#34-the-layer-stack)
   - [3.5 Combining](#35-combining)
   - [3.6 Render state](#36-render-state)
   - [3.7 Backend](#37-backend)
   - [3.8 Cost](#38-cost)
   - [3.9 Device reset](#39-device-reset)
   - [3.10 From today's code](#310-from-todays-code)
4. [Effects](#4-effects)
   - [4.1 Kinds of effect](#41-kinds-of-effect)
   - [4.2 Meshes](#42-meshes)
5. [Milestones](#5-milestones)
6. [References](#6-references)

---

## 1. Model

### 1.1 The painter in brief

- **One interface.** Everything neutrino draws — sprites, tiles, text, lines, curves,
  meshes — goes through the painter; the world, simplex, the demos, the tutorials and the
  games use nothing else. *Why:* order, layers and effects have one place to live.
- **Drawing is immediate.** A draw call renders when it is called; nothing is recorded,
  sorted or replayed. *Why:* SDL 3.4 already merges consecutive draws that share a texture
  and blend mode into one GPU draw call on every hardware backend, so recording would add
  machinery and buy nothing.
- **Layers are textures.** A scene declares its layers ahead of time; each one is a render
  target texture. `push_layer` / `pop_layer` choose where drawing goes. *Why:* a finished
  layer can be moved, faded, rotated or warped as a whole, and the z-order no longer
  depends on the order the code draws in.
- **The compositor combines the layers onto the canvas** — by default in declaration order,
  each with its composite settings; a scene can replace it with its own. *Why:* the
  settings cover the common effects; anything else is ordinary drawing code.
- **The canvas is the result.** Without layers, drawing goes straight onto it; with
  layers, drawing outside every layer goes to the base layer, the bottom one. A scene's
  canvas is what the window shows of that scene.
- **One painter; scenes own their layers.** There is one painter, application-wide: one
  painter state and one stack of saves. Each scene owns its layers, its compositor, its
  canvas and its saved state — its scene context — which the painter uses while the scene
  is rendered; the scene manager draws the canvases into the window in stack order. *Why
  one:* drawing is immediate and single-threaded and scenes render one after another, so
  only one painter is ever in use (R37).
- **Defaults change nothing; everything is exposed.** Code that draws today keeps working
  unchanged, and every setting is public.

### 1.2 Vocabulary

| Term | Meaning |
|---|---|
| **painter** | the drawing interface, one for the application: the painter state, the stack of saves, and the scene context in use; the free functions of `draw.hh` draw with it |
| **scene context** | what a scene draws into: its layers, compositor and canvas, and the painter state saved between its frames. Owned by the scene; in use while the scene is rendered |
| **pinned scene** | the one scene in the slot above the stack; push, pop and replace never touch it |
| **layer** | a render target texture a scene declares; drawing goes to it while it is pushed |
| **current layer** | the layer most recently pushed and not yet popped |
| **base layer** | in a scene with layers, the bottom layer, made with the first `declare_layer`: drawing with no layer pushed goes to it |
| **redraw mode** | how a layer or canvas is kept: `every_frame` (the default) clears it at its first push in each frame, so it shows only what that frame drew; `on_demand` keeps its contents between frames and shows them in frames that do not draw it. An *on-demand layer* has `on_demand`; an *every-frame layer* the default |
| **canvas** | a scene's result; not a layer |
| **composite settings** | how a layer — or a canvas — is drawn when combined: blend, opacity, tint, transform, effects |
| **compositor** | the function that draws a scene's layers onto its canvas |
| **composition** | what a compositor receives: the layers, their textures, the canvases below |
| **combining** | running the compositor, then drawing the canvases into the window |
| **painter state** | what every draw call reads: colour, blend mode, line style, clip, view |
| **producer** | anything above the painter that draws: tilemap, sprites, text, primitives, meshes |
| **quad** | a picture from an atlas placed at a position |
| **mesh** | a subdivided textured quad deformed by effects |
| **needs redraw** | an on-demand layer or canvas whose texture holds nothing its owner drew — it is new, taken from the pool, resized or reset — so the owner must draw it in full |
| **texture allocator** | the internal owner of every layer target and canvas texture: hands them out, takes them back, keeps unused ones for reuse |
| **texture handle** | a layer's or a canvas's hold on a texture from the allocator; gives it back when destroyed |
| **pool** | the allocator's unused textures, kept for reuse |
| **layer stack** | the internal owner of one scene's layers: their settings, their targets, the frame's flags and the record of the layer pushes (§3.4) |
| **save stack** | the painter's stack of saved states: one entry per `push_state`, the painter state to restore (§3.1). Layer pushes do not save state (R39) |

"Renderer" means SDL_Renderer (`sdlpp::renderer`), which only the painter's internals see.

### 1.3 Levels

| Level | Concepts | Knows about |
|---|---|---|
| **Game** | hero, gem, laser gun, board rotation | game state; picks what each producer shows |
| **Producers** | tilemap, sprites and animations, text, primitives, meshes and effects | time and appearance; they only issue draw calls |
| **Painter** (public) | the painter, layers, canvas, compositor, painter state, draw calls | *what* is drawn and where |
| **Internals** (private) | the texture allocator, combining, render state, backend | *how* it reaches the GPU |

- **The painter knows nothing about animation.** An animation is a producer choosing which
  picture to draw this frame; a tile, a character frame and a glyph are the same thing to
  the painter.
- **`get_renderer()` is hidden**, with no escape hatch. Everything SDL_Renderer can do is
  reachable through the painter. Today only four test files call it outside `src/`; tests
  link `neutrino_objects` and include an internal header instead.

---

## 2. Public contract

### 2.1 Two rules

1. **It works out of the box.** Code that draws today draws unchanged with the defaults;
   nothing is ported.
2. **Everything is exposed.** Every setting the painter has is public, and its default is
   today's behaviour.

The contract comes in three tiers: what stays as it is (§2.2), layers (§2.5–§2.9), and the
nuts and bolts — full state, inspection, rendering into a texture (§2.4, §2.12, §2.14).

### 2.2 What stays as it is

| Today | Contract |
|---|---|
| `draw.hh` free functions, all overloads (`int` and `render_pos`), and their names | same signatures and meaning; they draw with the painter |
| `set_draw_color` / `set_draw_blend_mode` / `set_line_style` and their getters | same calls; the values are the painter's state (§2.4) |
| `texture::create`, `from_image`, `adopt` | unchanged; what they make belongs to the caller, not to the texture allocator (§3.3); `adopt` rejects a texture made on another renderer |
| `texture::bind()`, `renderer_target_scope` (was `target_scope`), `compose()`, `draw_texture`, `draw_texture_quad` | unchanged but for the scope's name; inside a layer, `bind()` redirects drawing into the texture, and the layer is current again when the scope ends |
| `capture_frame()` | unchanged; it combines first, so it sees what will be shown |
| `render_size()`, `pixel_snap`, `draw_failure_count()` | unchanged |
| `base_scene::render()` | draws onto the scene's canvas, which is drawn into the window |
| `sprite_batch` | unchanged; flushes onto the current layer, or the canvas |
| simplex | unchanged; its canvas is the simplex scene's canvas |

### 2.3 Defaults

| Setting | Default | Why it matches today |
|---|---|---|
| scene context in use | the scene's during its `render()`; none outside scenes, where drawing goes to the bound target | tests and tools draw on the bound target as now |
| layers | none: drawing goes onto the canvas | one picture per scene |
| canvas | the pixel size and logical presentation of the target it is drawn into (R42), cleared every frame | drawing onto it lands on the same pixels as drawing into the window |
| canvas blend | `none` for a scene with `hides_below`, `blend` otherwise — and `blend` for every scene in a transparent window; set when the scene is pushed, before `on_enter()` | a full-screen scene is a plain copy, pixel-identical; an overlay shows the scenes below; a transparent window stays opaque where nothing was drawn |
| compositor | `default_compositor` | — |
| painter state | colour, blend and line style as SDL's defaults today; saved per scene between its frames | simplex relies on its draw colour persisting |
| view | identity: screen space | — |
| filtering | nearest | pixel art |

Four things change underneath, all intended:

- **Draw state is saved per scene.** A colour set by one scene no longer leaks into the
  scene drawn after it.
- **Overlay scenes may differ by ±1 on semi-transparent pixels**, because they pass through
  a canvas. Full-screen scenes are identical.
- **An overlay that draws translucent colour with blend `none`**, SDL's default, shows it
  blended over the scenes below instead of overwriting them. Drawn into the window, `none`
  replaced the pixels: `(0, 0, 255, 128)` over red came out solid blue. On the overlay's
  own canvas the alpha is kept, and the copy blends it: `(127, 0, 255)`. Full-screen
  scenes do not change, because the window under them is black. An overlay that wants to
  cover the scenes below draws opaque colour.
- **Each visible scene costs one copy into the window**, except the bottom full-screen
  scene when its canvas would only copy it: it draws straight into the window, as today
  (R44).

### 2.4 Painter state

```cpp
struct painter_state {                       // everything a draw call reads
    color               draw_color;          // set_draw_color
    blend_mode          blend;               // set_draw_blend_mode: a preset or custom_blend(...)
    line_style          line;                // set_line_style
    std::optional<rect> clip;                // nothing outside it is drawn
    view                view;                // camera transform; identity until RM2
};

[[nodiscard]] painter_state get_painter_state();          // a copy: inspect it, or change it and push it
void push_state();                                         // save
void push_state(const painter_state& s);                   // save, then replace with s
void pop_state();
[[nodiscard]] painter_state_scope scoped_state();          // push_state() now, pop_state() at scope end
[[nodiscard]] painter_state_scope scoped_state(const painter_state& s);
[[nodiscard]] blend_mode custom_blend(/* colour and alpha factors and operations */);
```

All of these are free functions on the one painter, as `set_draw_color` is.

- **One state, saved per scene.** The scene manager restores a scene's saved state before
  its `render()` and saves it again after; a new scene starts from the defaults. Outside
  scenes the state carries on from call to call, as today. *Why:* nothing leaks between
  scenes, and persistent drawing (simplex) keeps its colour.
- **Every draw call reads the state when it is made** and sets the SDL render state it
  needs; nothing depends on what an earlier call left behind.
- **`push_state` saves the state and `pop_state` restores it.** *Why:* a helper that
  changes the colour cannot leave it changed for its caller. `scoped_state` pops with
  RAII, so an exception cannot leave a save behind.
- **A layer push does not touch the state** (R39): the state says how to draw, a layer
  where. A helper that pushes a layer and wants its colour changes undone wraps them in
  `scoped_state()` too.
- **`push_state()` saves and changes nothing; `push_state(s)` saves and replaces the
  whole state with `s`.** The first is the common case — save, change a field or two,
  draw, restore — as `SaveDC`, `cairo_save`, Canvas 2D's `save()` and Processing's
  `push()` are. The second gives a helper a known state whatever its caller set:
  `scoped_state(painter_state{})` draws with the defaults.
- **There is no `set_painter_state`.** Replacing the whole state with nothing to restore
  is what the save stack exists to prevent; the setters (`set_draw_color` and the others)
  change single fields, and `push_state(s)` replaces it all. Only the scene manager
  replaces the state outright, when it restores a scene's saved state, and that is
  internal.
- **State pushes and layer pushes are independent.** Each restores only its own — the
  state, or the target — so a `pop_state` between a `push_layer` and its `pop_layer`
  leaves both right.
- **`clip`** limits drawing to a rectangle. *Why:* UI panels, and Nuklear, which draws with
  scissor rects. It belongs to the painter, not to a target: SDL keeps one per target, so
  the painter applies its clip to whichever target is bound — after a layer push or pop,
  or a `texture::bind()`, too. A clip set straight on the renderer, by a UI backend, is
  kept while the painter has none (R39, amended).
- **`blend_mode` is neutrino's own enum** (`video/blend_mode.hh`): none, blend, add, mul,
  mod and the two premultiplied forms. Its values are neutrino's; the engine maps them to
  SDL's in one internal header (`video/blend_mode_sdl.hh`), so nothing else relies on SDL's
  numbers (R34). One choice per draw, not flags.
- **`custom_blend`** composes a blend mode from factors and operations, encoded by
  neutrino and turned into SDL's at the mapping (`SDL_ComposeCustomBlendMode`, through
  sdlpp). Only the seven presets are guaranteed on every backend; the software renderer —
  the tests — supports no custom blend, and a draw SDL rejects counts as a failed draw.

### 2.5 Layers

```cpp
enum class redraw_mode : std::uint8_t {
    every_frame,                             // cleared at its first push in each frame (default)
    on_demand,                               // kept between frames; its owner redraws it (below)
};

struct layer_settings {                      // every field has a default
    std::optional<dim> size;                 // default: the target's pixel size (R42)
    redraw_mode        redraw{redraw_mode::every_frame};
    layer_composite_settings composite{};    // §2.7
};

// base_scene members: the scene's layers and their settings
layer_id declare_layer(const layer_settings& = {});
layer_composite_settings& composite(layer_id);
[[nodiscard]] layer_id base_layer() const;          // the bottom layer (below)

// free functions on the painter: during the scene's render()
void push_layer(layer_id);
void pop_layer();
[[nodiscard]] layer_scope scoped_layer(layer_id);   // pushes now, pops at the end of the scope
void clear();
[[nodiscard]] bool needs_redraw(layer_id);
```

*Why the split:* a scene declares its layers in its constructor, before it is ever
rendered, so declaring belongs to the scene. Everything else happens while the scene is
rendered and goes through the painter, so a helper given a `layer_id` can push it without
holding the scene. The id carries its layer stack's serial, so a layer of a scene that is
not being rendered is rejected.

**Declaring.** A scene declares its layers, normally in its constructor. `declare_layer`
appends to the scene's ordered list; that order is the default z-order.
Declaring creates no texture: a layer takes its target at its first push (§3.4).
*Why ahead of time:* the game knows its layers, and a fixed list makes the z-order a
property of the scene instead of the call order.

**The base layer.** The first `declare_layer` also makes the scene's base layer, before
the declared one: it is always the bottom of the default z-order.

- **Drawing with no layer pushed goes to it.** It is current at the start of every frame,
  under every push.
- **It is an ordinary layer**, at the scene's size and redrawn every frame.
  `base_layer()` gives its id: `composite(base_layer())` shakes or fades it, and a
  compositor may draw it anywhere, twice, or not at all.
- **It is pushed every frame,** so it is cleared at the start of each frame and always
  combined.
- **Pushing it** makes it current again over the layers pushed above it. It cannot be
  popped: `pop_layer` with only the base layer current is `pop_layer` with nothing pushed.
- *Why:* declaring a layer only adds one. A scene that declares a HUD keeps its other
  drawing as it is, and adopts layers one at a time. *Why a layer and not the canvas:* the
  canvas belongs to the compositor, and content drawn outside every layer can still be
  moved or faded as a whole.
- *Cost:* a scene with layers pays for its base layer — one clear a frame — whether it
  draws on it or not. While its composite settings are the identity its texture is the
  canvas, so it adds no combine (R41, §3.8).

**Pushing.**

- `push_layer(id)` makes the layer current; `pop_layer()` makes the previous one current.
  Neither touches the painter state (R39). `scoped_layer` does both with RAII, so an
  exception cannot leave a layer pushed.
- **The first push of a layer in a frame clears it**, unless it is redrawn on demand.
  Pushing it again later in the frame continues drawing on it.
- **Pushes nest:** a helper may push the HUD while the world is current. Nesting changes
  which target is current, never the order.
- **Pushing never changes the z-order.** *Why:* code may draw the HUD before the world, and
  the HUD is still on top.
- **Once a scene has declared layers, drawing goes only to layers**: with none pushed, to
  the base layer (above). *Why:* the canvas then belongs to the compositor.
- **One push per layer per frame** is the rule of thumb: every switch of target flushes
  SDL's queue (§3.7).

**Order.**

| Between | Order |
|---|---|
| layers | the compositor's; by default declaration order |
| draws within a layer | call order — the painter's algorithm; in 2D nearly everything is blended, so back to front is required |
| sprites by depth | `sprite_batch`: `(draw_layer, depth, insertion order)`, stable; anything that must sort with sprites goes through the batch |

**`redraw`** decides what a layer holds in a frame that does not draw it:

| | `every_frame` (default) | `on_demand` |
|---|---|---|
| first push in a frame | clears the layer | keeps last frame's contents; draws add to them |
| a frame that does not push it | not combined: shows nothing | combined with its old contents |
| emptied by | every frame | `clear()` only |

Uses of `on_demand`: a HUD redrawn only when the score changes; a background drawn once;
accumulation effects such as trails. *Why an enum, not a `retained` flag* (the name until
2026-11-16): "retained" reads as retained-mode drawing — a recorded scene replayed every
frame — which the painter is not (§1.1); and an enum has room for another policy, such as
clearing once. Caveats:

- **It can need drawing in full again**: `needs_redraw(id)` says when (below).
- **It holds its texture every frame**, drawn or not.
- **A camera layer is rarely redrawn on demand**: once the camera moves, the old contents
  are stale.

**`needs_redraw(id)`** says that an on-demand layer holds nothing its owner drew, so the
owner must draw all of it:

```cpp
if (m_score_changed || needs_redraw(m_hud)) {
    auto s = scoped_layer(m_hud);
    clear();
    draw_score();
}
```

- **It is true** in the layer's first frame, after a resize if the layer follows the
  scene's size, and after SDL resets the render targets or the device (§3.9).
- **A layer that needs a redraw holds only its clear colour**: transparent, or white for
  `mod`. Until it is pushed it is not combined, so an owner that does not draw it shows
  nothing — never stale pixels, and never another scene's.
- **It turns false at the end of a frame that pushed the layer**, so it gives one answer
  for the whole frame. *Why not at `pop_layer`:* two helpers that each draw part of the
  layer must both redraw; marked at the first one's pop, the second would see `false`,
  take its part to be still there, and leave it missing.
- **Redraw everything when it is true.** A partial update on such a layer leaves the rest
  empty.
- **It never changes during a frame.** SDL reports resets as events
  (`SDL_main_callbacks.c:32` does not dispatch them immediately), and events are handled
  between frames.
- **For an every-frame layer it is always false**: such a layer keeps nothing, and
  its owner draws it every frame anyway.

**`clear()`** clears the current layer — the base layer when none is pushed, the canvas in
a scene without layers, the bound target outside scenes — to transparent; a `mod` layer to
opaque white (§2.8).

**`size`** gives a layer its own resolution; it is scaled to the canvas when combined.

### 2.6 The canvas

```cpp
// base_scene members
layer_composite_settings& canvas_composite();  // how the scene's canvas is drawn into the window
void set_canvas_redraw(redraw_mode);            // on_demand: the canvas persists between frames
[[nodiscard]] bool canvas_needs_redraw() const; // needs_redraw(id) for an on-demand canvas
[[nodiscard]] const texture& canvas() const;    // the result
```

- **The canvas is the scene's result, not a layer.** It has the pixel size and the logical
  presentation of the target the scene manager draws into, the window or a bound texture,
  so drawing onto it lands on the same pixels as drawing into that target (R42).
- **Without declared layers, drawing goes straight onto the canvas**, as today; there is
  nothing to combine and the compositor is not called.
- **With declared layers, the canvas belongs to the compositor.** It is cleared when the
  compositor starts, unless it is redrawn on demand, and the compositor draws the layers
  onto it. Drawing with no layer pushed goes to the base layer (§2.5), not to the canvas.
- **Its texture is taken when something is drawn onto it** (R41): at the first frame of a
  scene without layers; at combining in a scene with layers, and only when the base
  layer's own texture cannot serve as the result (§3.5). A scene hidden under one with
  `hides_below` gives the textures of its every-frame canvas and layers back to the pool,
  and takes them again when it is next drawn.
- **The canvas's composite settings** say how a scene's canvas is drawn into the window over
  the scenes below, every flat setting included, as for a layer that mirrors the canvas
  (§2.7, R43). Fading a whole pause menu in is `canvas_composite().opacity = t`; a
  transition can shrink or turn a whole scene. The default blend is `none` with
  `hides_below` and `blend` without.
- **`set_canvas_redraw(redraw_mode::on_demand)`** keeps the canvas between frames:
  simplex's persistent canvas.
- **`canvas_needs_redraw()`** is `needs_redraw(id)` for an on-demand canvas, with the same
  rules: true in the first frame and after a resize, a reset or a change of the logical
  presentation at the same pixel size, false at the end of a frame
  that drew on the canvas, always false for an every-frame canvas. Simplex records
  nothing, so its canvas after a reset stays empty.
- *Why the canvas is not a layer:* every layer stays an ordinary layer that a compositor
  can draw in any order, any number of times, or not at all; the result has no special
  rules to remember.

| Concept | Job |
|---|---|
| `declare_layer` | which layers exist, and their default order |
| `push_layer` / `pop_layer` | where draw calls go now |
| compositor | how the layers become the canvas: the final z-order |

### 2.7 Composite settings

```cpp
struct layer_composite_settings {
    blend_mode blend{blend_mode::blend};   // none, blend, add, mul, mod (§2.8)
    float      opacity{1.0f};
    color      tint{colors::white};        // fade to black, damage flash, night
    point      offset{};                   // screen shake; render units
    layer_scale scale{1.0f};               // per axis; one float scales both
    float      rotation_degrees{0.0f};
    std::optional<point> pivot;            // centre of scale and rotation; unset: the middle
    flip_mode  flip{};                     // flip_mode.hh; diagonal: refused (R35)
    std::optional<rect> source;            // only part of it: render units, or its pixels
    std::optional<rect> destination;       // where it lands, in render units; unset: in place
    std::vector<mesh_effect> effects;      // ordered: rotate_3d, wave, perspective… (§4)
};

p.composite(m_world).offset  = m_shake.offset();
p.composite(m_hud).opacity   = m_fade.value();
p.composite(m_board).effects = {rotate_3d{.yaw = m_angle}};
```

- **Coordinate types follow docs/coordinates.md (R40):** `offset` and `pivot` become
  `render_delta`s, `destination` a `render_rect`, and `source` a `render_rect` for a layer
  that mirrors the canvas and for a canvas, a texel rectangle for a layer with its own
  `size`; the struct above shows today's types.
- **Live:** change them at any time; the compositor reads them when it runs. Animating
  them each frame on the scene's clock is how layer effects move.
- **Flat transforms** draw the layer as one quad (R43). The rectangle it is drawn onto is
  `destination`, or, unset, the layer in place: a layer that mirrors the canvas lands on
  the same pixels, a layer with its own `size` is stretched over the render area (not the
  letterbox bars). A `source` part stays where it was, or is stretched onto `destination`;
  without a `source`, a layer that mirrors the canvas gives `destination` its picture, the
  render area, not the bars.
  Then the layer is flipped, scaled and rotated about the pivot (unset: the rectangle's
  centre), and moved by the offset. A scale of 0 hides it.
- **Brix's plasma** (render_model.md §7.1) as a layer: one texel per 4×4 cell,
  `declare_layer({.size = {w / 4, h / 4}, .composite = {.destination = board}})`, with
  `board` rounded down to whole cells.
- **Effects** draw the layer through a subdivided mesh (§4.2).
- The same struct describes a scene's canvas (`canvas_composite()`).

### 2.8 Alpha and blend modes

Layer targets and canvases are ARGB8888, cleared to transparent, so wherever nothing was
drawn, what is below shows through. Blended drawing into a transparent target leaves
premultiplied colour in it (colour × alpha), so each layer is combined with the
premultiplied form of its blend:

| `blend` | Combined with | Cleared to | Use |
|---|---|---|---|
| `none` | `none` | transparent | a layer that covers everything: a plain copy, the cheapest. On a canvas: an opaque scene |
| `blend` (default) | `blend_premultiplied` | transparent | normal layers |
| `add` | `add_premultiplied` | transparent | glow, light, fire |
| `mul` | `mul` | transparent | darkness, shadows. `src × dst + dst × (1 − srcA)` is already right for premultiplied content; a transparent pixel leaves what is below unchanged |
| `mod` | `mod`; `mul` below full opacity | opaque white | tinting everything below. `src × dst` ignores alpha, so a transparent pixel would turn what is below black, and it cannot fade: below full opacity the layer is drawn with `mul`, which for its opaque pixels gives `dst × (1 − o + c × o)`, the tint faded by the opacity `o` |

- **Opacity scales colour as well as alpha.** In the premultiplied modes SDL's alpha
  modulation scales only alpha, so colour is modulated by `tint × opacity`. A `mod` layer
  below full opacity is drawn with `mul` (table), where the same modulation fades it.
- **`none` is opaque by contract:** pixels a `none` layer does not cover come out black.
- **A layer is isolated.** `mul` or `mod` draws inside a layer see only that layer; a
  darkness overlay is a layer whose blend is `mul`.
- **A scene's layers are combined onto its own canvas**, not onto the scenes below. To dim
  the game under a pause menu, the pause scene draws a translucent black rectangle first.
- **±1 on semi-transparent pixels** compared with drawing straight into the window; opaque
  pixels are identical.

### 2.9 The compositor

```cpp
using compositor = std::function<void(composition&)>;

// base_scene members
void set_compositor(compositor);
[[nodiscard]] const compositor& get_compositor() const;

void default_compositor(composition& c) {
    for (const layer_id id : c.layers()) {    // declaration order
        if (c.has_content(id)) {              // pushed this frame, or on demand and not needing a redraw
            c.draw_layer(id);                 // onto the canvas, with its composite settings
        }
    }
}
```

Every scene starts with `default_compositor`. It is a public function: a compositor of
the game's own can call it as a building block, and `set_compositor(default_compositor)`
puts it back.

| `composition` call | Does |
|---|---|
| `layers()` | the base layer, then the declared layers, in declaration order |
| `has_content(id)` | pushed this frame, or on demand and not needing a redraw |
| `draw_layer(id)` | draws the layer onto the canvas with its composite settings |
| `draw_layer(id, settings)` | draws it with other settings: a second copy, a part, another blend |
| `draw_layers(first, last)` | `draw_layer(id)` for each layer with content from `first` to `last` |
| `texture(id)` | a layer's texture, for `draw_texture`, `draw_mesh` or quads with source rects |
| `below()` | the canvases of the visible scenes below, bottom to top, read-only (§2.10) |

- **The free functions draw onto the canvas** while a compositor runs: a vignette, a melt,
  a debug overlay.
- **Pushing a layer inside a compositor is an error**: the layers are finished.
- **A layer the compositor does not draw is not shown** — debug views for free.
- **When it runs** (R43): right after the scene's `render()`, onto the canvas, cleared
  unless it is redrawn on demand; then the layers pushed this frame and the canvas are
  marked drawn. Each scene is combined before the scene above it renders, so `below()` is
  ready. The compositor starts with the default painter state, whatever `render()` left. A
  compositor that throws is treated like a `render()` that throws: the scene is popped.
- **`draw_layer` draws in the canvas's pixels**, with the presentation and the clip off,
  so a layer that mirrors the canvas is copied exactly; the clip does not apply to it.
- **`below()`** is the canvases of the visible scenes below, bottom to top, without those
  with nothing drawn: `std::span<const texture* const>`, valid during the call.
- `set_compositor` fails on an empty compositor, and inside the scene's own compositor.

```cpp
// a water reflection: the world drawn a second time, mirrored and faded, under the HUD
set_compositor([&](composition& c) {
    c.draw_layers(sky, world);
    c.draw_layer(world, {.opacity = 0.3f, .offset = {0, water_y}, .flip = flip_mode::vertical});
    c.draw_layer(hud);
});
```

**Wrapping.** A transition does not need to know how the game composes its frame: it
wraps the current compositor and puts it back when it ends.

```cpp
const compositor scene = next.get_compositor();      // next: the scene coming in
next.set_compositor([scene, &melt](composition& c) {
    scene(c);           // the new scene, composed as the game composes it
    melt.draw();        // columns of the old scene's canvas falling over it
});
// when the melt ends:
next.set_compositor(scene);
```

*Why two levels:* settings cover screen shake, fades, brix's board rotation and night
overlays without code; a compositor covers the rest — a layer drawn twice, an overlay
between layers, a transition, split screen — without inventing an effects language.

### 2.10 Scenes

```cpp
class play_scene : public neutrino::base_scene {
    const layer_id m_world = declare_layer();
    const layer_id m_hud   = declare_layer({.redraw = redraw_mode::on_demand});

    void render() override {
        {
            auto s = scoped_layer(m_world);
            m_map.draw();
            m_batch.flush();
        }
        if (m_score_changed || needs_redraw(m_hud)) {
            auto s = scoped_layer(m_hud);
            clear();
            draw_text({4, 4}, m_score_text, m_font);
        }
    }
};
```

- **Every scene owns a scene context:** its layers, its compositor, its canvas and the
  painter state saved between its frames. The painter uses it while the scene is
  rendered, so the free functions draw into the scene's layers and canvas. *Why per
  scene:* a menu, gameplay and a loading screen need different layers, and the scene's
  lifetime manages their textures. *Why not a painter per scene:* only one scene draws at
  a time (R37).
- **A frame:** the scene manager walks the visible scenes bottom to top, from the topmost
  one with `hides_below`. For each it makes the scene's context the painter's, restoring
  its saved state, calls `render()`, and, if the scene declared layers, runs its
  compositor; the pinned scene comes last (§2.11). Then the canvases are drawn into the
  window in that order, each with its canvas composite settings.
- **Overlays** are scenes whose canvas is blended over the scenes below.
- **Layers are private to their scene.** Pushing another scene's layer is an error. A
  compositor reads other scenes only through `below()`: their canvases, never their
  layers. A pause menu that wants the game blurred or warped draws those canvases itself,
  as its own bottom content.
- **Transitions need no capture.** The old scene's canvas is its last frame; the scene
  manager keeps it until the transition ends, and a wrapping compositor draws it with the
  new scene (§2.9).
- **Simplex is a scene.** `application_adapter::create_initial_scene()` installs a
  `simplex_scene`; its persistent canvas becomes that scene's on-demand canvas.
- **A scene rendered outside the scene manager** — a test calling `scene.render()`
  directly — draws with no scene context: onto the bound target, as today. That works for
  scenes without declared layers; pushing a layer there fails ENFORCE, so a layered scene
  is rendered through the manager (tests get a helper).

### 2.11 The pinned scene

```cpp
void pin_scene(std::unique_ptr<base_scene> scene);   // replaces the pinned scene, if any
void unpin_scene();                                  // exits and destroys it; none pinned: no-op
```

One slot above the stack, for what outlives the scenes below it: an FPS counter, a debug
console.

- **Both are queued** like `push_scene`. `pin_scene` gives a previously pinned scene
  `on_exit` and the new one `on_enter`; `unpin_scene` gives it `on_exit`.
- **`push_scene`, `pop_scene` and `replace_scene` never touch it**; they act on the stack
  below it. *Why a slot:* a scene meant to stay above everything is not covered by the next
  push, and needs no ids or ordering rules to remove.
- **Otherwise it is the topmost scene**, with all its flags: `hides_below` hides the whole
  stack, `pauses_below` pauses it, and its canvas's default blend follows `hides_below`. An
  FPS counter overrides `flags()` like any overlay.
- **Resize notices** go to the top stack scene and to the pinned scene; a scene that becomes
  the top of the stack is sized even with a pinned scene above it.
- **With nothing below it**, `pop_scene` and `replace_scene` are errors (§2.15).
- **A failed `on_enter` rolls the pin back:** the scene gets `on_exit` and is dropped, a stack
  it paused is resumed, and nothing stays pinned.
- **At shutdown** `finish()` exits it before the stack and clears the slot.
- **One slot:** a console that also shows the FPS, or one debug scene hosting both.

**Pausing follows changes of state.** After every change of the stack, of the slot or of a
scene's flags, the scene manager works out which scenes are updated: a scene gets
`on_pause` when it stops being updated and `on_resume` when it starts again. *Why:* scenes
are pushed and popped underneath a pinned scene that pauses, so "the scene directly below"
is not stable; state changes are balanced by construction. So `flags()` may change at any
time; the manager reads it every frame. Under a pause menu, a busy spinner and the game
below it both get `on_pause`. (Not implemented yet — RM10. Until then only `passes_input`
may change while a scene is on the stack.)

**`passes_input`**, a scene flag:

```cpp
enum class scene_flags : std::uint8_t {
    none         = 0,
    hides_below  = 1,
    pauses_below = 2,
    passes_input = 4   ///< sees input and events, and passes them on to the scene below
};
```

Input and events go down from the top, the pinned scene first, and stop at the first scene
without `passes_input`; the scenes passed on the way see them too. The scenes below that
one get the neutral snapshot and no events, as covered scenes do today. Input never reaches
a scene paused by `pauses_below`. The flags are read before an event is delivered, so a
`handle_action` that changes them routes the next event. *Why "sees and passes on":* a
closed console must see its toggle key.

| Scene | Flags |
|---|---|
| FPS counter | `passes_input` |
| console, closed | `passes_input` |
| console, open | `pauses_below` (freezes the game) or `none` (the game runs on) |
| pause menu, busy spinner | as today |

**A Quake-style console:**

```cpp
class console_scene : public neutrino::base_scene {
    bool  m_open  = false;
    float m_slide = 0.0f;                                // 0: hidden, 1: fully down

    scene_flags flags() const override {
        return m_open ? scene_flags::pauses_below : scene_flags::passes_input;
    }
    void handle_action(const event& e) override {
        if (is_key_down(e, key::grave)) { m_open = !m_open; return; }   // seen while closed too
        if (m_open) m_line.edit(e);                      // text, history, completion
    }
    void fixed_update(sim_duration dt, const input_snapshot&) override {
        m_slide = approach(m_slide, m_open ? 1.0f : 0.0f, dt.count() / 0.25f);   // always updated
    }
    void render() override {
        if (m_slide == 0.0f) return;                     // nothing drawn: nothing copied (§3.2)
        canvas_composite().offset = {0, int((m_slide - 1.0f) * m_height)};
        draw_rect_fill({0, 0, render_size().width, m_height}, color{0, 0, 0, 200});
        draw_log_lines();
        draw_prompt();
        draw_fps();
    }
};

pin_scene(std::make_unique<console_scene>());            // once, at startup
```

The slide is the canvas's composite `offset`; the translucency is the canvas's blend.
Beyond the scene manager and the painter, the console needs text input (sdlpp's
`text_input_session`, not exposed by neutrino yet), `draw_text` (FM3), a log backend
(failsafe's `logger::set_backend`) and a command registry.

**The mouse cursor is not a scene.** It is SDL's hardware cursor, drawn by the OS: no layer
and no lag. sdlpp wraps `SDL_CreateColorCursor`; SDL 3.4 adds `SDL_CreateAnimatedCursor`.
A pixel-art cursor must be scaled to window pixels. neutrino has no cursor API yet.

### 2.12 Rendering into a texture

There are no explicit painters (R37). The one painter draws into a texture with
`texture::compose()` or `bind()`, as today:

```cpp
auto board = texture::create({256, 256}, texture_access::target);
board->compose([&] { draw_board(); });             // the free functions draw into board
draw_mesh(*board, rotate_3d{.yaw = angle});        // onto the scene's current layer (RM4)
```

| Use | How |
|---|---|
| baking (KE's backdrop) | `compose()` once |
| brix's board rotation | a layer with `rotate_3d` in its composite effects (RM5), or a texture made with `compose()` and drawn through a mesh: no readback either way |
| one effect over several layers (O3) | a compositor that `compose()`s those layers' textures into one texture and draws it through the effect |
| a picture used many times, or in several scenes | a texture made once with `compose()` |

- **The texture belongs to its owner**, not to the texture allocator: made with
  `texture::create`, kept as long as the owner likes, rebuilt by the owner after a reset
  (§3.9).
- **Inside `compose()` the painter state is the caller's.** A helper that changes the
  colour while drawing into the texture changes it for its caller too, unless it uses
  `scoped_state()` — the same rule as for layers.
- **A layered render-to-texture** — a texture with its own layers and compositor — is not
  supported. Nothing needs it yet; if something does, the painter can hold a stack of
  scene contexts and a scope can push one (R37).

A HUD needs no texture of its own (an on-demand layer does it), nor does a transition (the
old scene's canvas does it).

### 2.13 Quads

```cpp
// flip_mode (flip_mode.hh): bit flags none, horizontal, vertical, diagonal (tiles)
struct quad_params {                         // today's sprite_draw_params plus tint and crop
    float     scale{1.0f};
    flip_mode flip{};
    float     rotation_degrees{0.0f};
    color     tint{colors::white};           // colour and alpha, this call only
    insets    crop{};                        // source inset, in picture pixels
};
enum class fit : std::uint8_t { stretch, aspect, centre };

void draw_sprite(point position, atlas_address picture, const quad_params& = {});
void draw_picture(const rect& dst, atlas_address picture, flip_mode = flip_mode::none, fit = fit::stretch);
```

Placement rules — all exist in `draw.cc` today and are kept:

- `position` is the picture's **pivot**, snapped to whole pixels.
- Scale and rotation are about the pivot; flip mirrors the pivot with the picture.
- A diagonal flip (tiles) goes through textured geometry.
- At 0° rotation the result is byte-identical to the unrotated path.
- `tint` modulates colour and alpha for this call only.
- `crop` insets the source; the destination size follows the cropped source.
- `draw_picture` fills a rectangle: `stretch`; `aspect` (fit inside, keep the ratio);
  `centre` (no scaling). The exact-integer `stretch` path is the seam-free one tilemaps use.
- A quad's source may also be a texture and a source rect — a layer, a canvas — not only an
  atlas picture (the Doom melt, §4.1).
- The blend mode comes from the painter state.

### 2.14 Inspection

```cpp
// base_scene members
[[nodiscard]] std::span<const layer_id> layers() const;       // the base layer, then declaration order
[[nodiscard]] layer_info info(layer_id) const;                // size, redraw mode, pushed this frame, needs redraw
[[nodiscard]] const texture& layer_texture(layer_id) const;   // read-only: draw it, mesh it, dump it

// free functions
[[nodiscard]] painter_stats get_painter_stats();  // last frame: draw calls, target switches,
                                                  // layers combined, failed draws
[[nodiscard]] pool_stats texture_pool_stats();    // held and pooled, bytes; created, reused, destroyed
void trim_texture_pool();                         // destroys every pooled texture
```

A draw call can be stepped into and its effect inspected on the layer texture; layers and
canvases can be dumped with `texture::to_image()`.

### 2.15 Errors

- **ENFORCE at the caller's line:**
  - an unknown atlas or picture index;
  - `push_layer` with a layer of a scene that is not being rendered, or `layer_id{}`;
  - `push_layer` outside a scene's `render()`;
  - `pop_layer` with nothing pushed;
  - `pop_layer` when the bound target is not the layer's: a `texture::bind()` scope
    outlived the layer;
  - a frame ending with a layer still pushed (the message names it);
  - drawing a layer's or a canvas's own texture into itself;
  - `base_layer()` in a scene that declared no layers;
  - pushing a layer inside a compositor;
  - a scene's frame, or a compositor, that ends with a `push_state()` not popped (the push
    is undone first);
  - `set_compositor` with an empty compositor, or inside the scene's own compositor;
  - a diagonal flip, a non-finite rotation or opacity, or a negative or non-finite scale
    in the composite settings of a layer being drawn, or of a canvas at the end of its
    scene's frame (a canvas whose settings go bad later is skipped and counted instead);
  - `pop_scene` or `replace_scene` with no scene below the pinned scene.
- **Silent no-op:** an invalid address ("no picture").
- **Wrong arguments throw**, as today: a non-finite or non-positive scale, a non-finite
  rotation.
- **What only SDL can reject** — a failed call, a target that cannot be created — is logged,
  rate-limited per call site, and counted in `draw_failure_count()` and `painter_stats`.

### 2.16 Below the painter

Still public, built on the same painter:

- **`draw_geometry`** — textured triangles with per-vertex colour; every other draw can be
  expressed with it (RM3).
- **Meshes and effects** — `draw_mesh` on atlas pictures, layer textures and canvases (§4.2,
  RM4).
- **Text** — `draw_text(point, std::string_view, const font&, const text_params& = {})` and
  the wrapped `rect` form, drawn immediately on the current layer; planned in
  [`font_plan.md`](font_plan.md) (RM7).

---

## 3. Internals

Everything below lives in `src/` and is not installed.

### 3.1 Data

A layer is more than a render target: it is an entry in an ordered list. `push_layer` /
`pop_layer` only choose the current entry; the default compositor follows the list.

```cpp
struct layer_entry {                 // one per layer: the base layer first
    layer_settings  settings;        // size, redraw mode, composite (live; read when combining)
    texture_handle  target;          // from the texture allocator; taken at the first push
    bool            pushed{false};   // pushed this frame
};

struct scene_context {                       // one per scene, owned by base_scene
    detail::layer_stack        layers;          // the base layer, then declaration order (§3.4)
    detail::canvas             canvas;          // the result: texture, composite, redraw, drawn
    compositor                 compositor{default_compositor};
    painter_state              saved;           // the painter state between the scene's frames
};

class painter {                              // one: application_scoped<painter>
    painter_state              m_state;         // what every draw call reads; the clip included
    std::vector<painter_state> m_saves;         // the save stack: one per push_state
    scene_context*             m_context{nullptr}; // the scene being rendered; none outside scenes
};
```

- **The painter is `application_scoped<painter>`** (`services/application_scoped.hh`):
  made on first use, destroyed when the application quits, before SDL goes. The free
  functions of `draw.hh` reach it through `instance()`; anything that may run during or
  after quit uses `maybe()`, which is null then.
- **`application_scoped<T>`** is the engine's lifetime for objects that live as long as the
  application (R38). `on_ready()` opens the application scope; `on_quit()` closes it right
  after the scenes and `teardown()`, before the application's own services and SDL, and
  destroys every such object, newest first — so one made by another's constructor is
  destroyed after it. Outside an open scope `instance()` fails ENFORCE, so nothing is made
  after SDL is gone, and the next application gets new objects. Registering for the quit
  is checked under the same lock as closing, so an object made on another thread while
  the scope closes is refused, not left for the next application. Only what benefits uses
  it; the services the application owns directly stay as they are.
- **A scene's context belongs to `base_scene`**, so `declare_layer` works in the scene's
  member initialisers, where layers are declared (§2.10), and the scene's lifetime is its
  layers' and canvas's lifetime.
- **`m_context` is set by the scene manager around each `render()`** and is null
  otherwise; drawing then goes to the bound target, as today.
- **`detail::canvas`** (`src/neutrino/video/canvas.hh`) owns a scene's canvas: its
  texture from the allocator, its composite settings, its redraw mode and the frame's
  `drawn` flag. It knows its size and the allocator, not scenes, so it is tested alone.
  It is made with its scene; its texture is taken when first needed, and for a frame it
  may show the base layer's texture instead of its own (R41, §3.5). Its frame:
  - `begin_frame(geometry)` takes the geometry of the target the scene manager draws into
    (`detail::target_geometry`: its pixel size and logical presentation, R42), resizes a
    texture it holds, and resets the frame's flags;
  - `bind()` takes the texture at its first bind, sets the target's logical presentation
    on it, and makes it the target while `render()` runs, or the compositor. An
    every-frame canvas is cleared at its first bind in the frame;
  - `note_drawn()` is called by whoever draws on it: `scene_frame` calls it for a scene
    without layers whose `render()` returned;
  - `end_frame()` marks the texture drawn if something was, so an on-demand canvas no
    longer needs a redraw;
  - `release()` gives the texture back: a hidden scene's every-frame canvas, at the start
    of the frame, so the scenes drawn can reuse it.

  `draw_canvases()` copies the canvases into the target inside a `detail::pixel_space`
  (§3.4): its logical presentation and clip off, each canvas with every flat composite
  setting, as a layer that mirrors the target, so in place it lands 1:1 (R43).
  `has_content()` says whether a canvas is copied, and whether the scenes above see it in
  `below()`.
- **`detail::scene_frame`** (`src/neutrino/scene/scene_frame.hh`) runs one scene's frame
  around its `render()` and combines its layers in `finish()` (§3.2, R43). The scene
  manager gives it the canvases drawn so far, for `below()`.
- **A `layer_id` carries its layer stack's serial** as well as its index (§3.4), so a
  layer of a scene that is not being rendered is rejected.
- **The save stack holds the state pushes only.** The layer pushes are recorded by the
  scene's layer stack (§3.4): a layer push changes where draws go, not the state (R39).
  Both are empty between frames: a scene's frame that ends with a layer or a state push
  left fails, after undoing it, and so does a compositor that leaves a state push.

### 3.2 A frame

`application::on_render` (`src/neutrino/application.cc`) calls `render_scene()` and
presents. `scenes_manager::render()` (`src/neutrino/scene/scenes_manager.cc`) plans the
frame (`plan()`: the bottom visible scene, whether it draws straight into the window,
whether its canvas covers it), clears the window to black unless that canvas will cover
it (`covers_target()`, R44) — late, if it ends with nothing to show, or before the bottom
scene's `render()` if its own target could not be made, so it draws into the window — and
walks the visible scenes bottom to top. With the painter, after the allocator's
`begin_frame()` (§3.3):

1. Every scene hidden under one with `hides_below` gives back the textures of its
   every-frame canvas and layers (R41), first, so the scenes drawn this frame can reuse
   them.
2. For each visible scene:
   1. start the canvas's frame: resize its texture if the render size changed, and reset
      its `drawn` flag;
   2. the painter takes the scene's context and restores the context's saved state;
   3. a scene without layers binds its canvas, taking its texture at the first frame
      (§3.3) and clearing it unless it is redrawn on demand — or, the bottom full-screen
      scene whose canvas would only copy it, binds nothing and draws straight into the
      window (R44). In a scene with layers,
      `begin_frame()` on its layer stack resizes the layers if the render size changed,
      resets their `pushed` flags and pushes the base layer; a layer takes its target at
      its first push (§3.4);
   4. call `render()`; if it throws, `abort_frame()` ends every layer push and every
      `push_state()` left, and the canvas's binding ends, so the target and state from
      before the scene are back. The canvases of the scenes finished below it are still
      drawn into the window, as in step 4, before the exception goes on, so they show;
   5. `end_frame()` checks that only the base layer is left pushed and pops it, then gives
      back the targets of the every-frame layers that were not pushed;
   6. if the scene declared layers, combine them (R43): choose the canvas's texture
      (R41) — the base layer's, when it can serve as the result, or the canvas's own —
      bind it, cleared unless it is redrawn on demand, and run the compositor with the
      default painter state and the canvases of the scenes drawn below (`below()`). On
      the base layer's texture, the default compositor does not draw the base layer. A
      compositor that throws is treated like a `render()` that throws; a canvas that
      cannot be made skips the compositor;
   7. `mark_drawn()` every handle drawn on this frame: each layer pushed, and the canvas if
      anything was drawn on it;
   8. the canvas's binding ends; the painter saves its state into the context and lets go
      of it.
3. Do the same for the pinned scene, last.
4. Clear the window now if it was not cleared because the bottom canvas would cover it,
   and that canvas has nothing to show. Draw the canvases into the window in that order,
   each with its canvas composite settings. An every-frame canvas that had nothing drawn
   on it this frame is not copied, so a hidden pinned scene costs nothing.
5. Present.

`capture_frame()` runs the combining first, so it reads what will be shown.

### 3.3 Targets and the texture allocator

The texture allocator is the internal owner of the painter's render targets: every layer
target and every scene's canvas. It creates them, hands
them out as handles, takes them back when the handles die and keeps them for reuse. It also
keeps each handle's "needs a redraw" flag true to what the texture holds, through resizes,
resets and low memory. *Why one class:* clearing, reuse, loss, resize and device reset are
decided in one place and tested without a scene manager.

**What it owns, and what it does not.** The public ways to make a texture stay (§2.2):
`texture::create`, `from_image` and `adopt`. What they make is not the allocator's.

| Texture | Made by | Owned by | Pooled | After a reset (§3.9) |
|---|---|---|---|---|
| layer targets, scene canvases | `acquire` | the scene context, through a handle | yes | the allocator clears or recreates it and marks it for redraw |
| baked results (`compose()`), streaming textures, any other game texture | `texture::create`, `from_image` | the game | no | its owner re-bakes or re-creates it |
| sprite and glyph atlas pages | the texture registry (`adopt`, `from_image`) | the registry | no | re-uploaded or rebuilt |

The simplex canvas is a game texture today. It becomes the simplex scene's on-demand canvas
([`render_model.md`](render_model.md) §5.6), and then comes from the allocator.

**The renderer.** Every texture, the allocator's included, is made on the application's
renderer. The allocator creates through `texture::create`, which uses it, and takes no
renderer of its own.

- *Why not another renderer:* SDL binds and draws a texture only with the renderer that
  created it, and `bind()`, `compose()`, `to_image()` and `draw_texture()` all use the
  application's. A target made on another renderer would fail at its first use.
- *Why not a renderer per call:* one pool would then hold several renderers' textures and
  could hand one renderer's texture to another; resizes and resets are per renderer too.
- **`texture::adopt` checks it:** a texture created on another renderer fails ENFORCE.
- If neutrino ever drives several renderers, `texture` learns its renderer and each
  renderer gets an allocator of its own. The rules below stay as they are.

```cpp
namespace neutrino::detail {                     // src/, not installed
    struct texture_spec {
        dim   size;
        color clear_color;                       // opaque white for a mod layer, else transparent
    };                                           // ARGB8888, target access, nearest: fixed

    class texture_handle {                       // move-only; a moved-from handle is empty
        public:
            texture_handle() = default;                         // empty
            texture_handle(texture_handle&&) noexcept;
            texture_handle& operator=(texture_handle&&) noexcept; // gives its own back first
            ~texture_handle();                                  // gives the texture back

            [[nodiscard]] const texture& get() const;  // live only, else ENFORCE
            [[nodiscard]] texture& get();              // to draw into it (bind()); live only
            [[nodiscard]] dim size() const;            // {0, 0} when empty or detached
            [[nodiscard]] bool needs_redraw() const;   // false only when live and drawn
            void mark_drawn();                         // its owner has drawn it
            void clear();                              // to its clear colour; flag unchanged
            explicit operator bool() const;            // true only when live
    };

    class texture_allocator {                    // one per renderer, owned by the application
        public:
            [[nodiscard]] texture_handle acquire(const texture_spec&);
            void resize(texture_handle&, dim);
            void drop_pooled(dim);               // destroys the pooled textures of this size
            void trim();                         // destroys every pooled texture
            static void request_trim() noexcept; // any thread: trim at the next begin_frame()
            void begin_frame();                  // runs a requested trim
            [[nodiscard]] pool_stats stats() const;
            void on_targets_reset();
            void on_device_reset();
    };
}

namespace neutrino {
    struct pool_stats {                          // public: texture_pool_stats() (§2.14)
        std::size_t held{0};                     // textures in live handles
        std::size_t pooled{0};                   // textures waiting for reuse
        std::size_t held_bytes{0};               // width × height × 4 per texture
        std::size_t pooled_bytes{0};
        std::size_t created{0};                  // since the application started
        std::size_t reused{0};                   // acquires served from the pool
        std::size_t destroyed{0};
    };
}
```

**Terms.**

- A **held** texture belongs to a live handle. A **pooled** texture was given back and
  waits for reuse, keyed by its size; nothing ever shows a pooled texture, so its contents
  never matter.
- A **slot** is the allocator's record of one handle: its spec, its texture if it has one,
  and its redraw flag.

**Handle states.**

| State | Reached by | `bool` | `size()` | `needs_redraw()` | `get()` |
|---|---|---|---|---|---|
| empty | default construction; being moved from | false | `{0, 0}` | true | ENFORCE |
| live | `acquire`, `resize` or `on_device_reset()` gave it a texture | true | its texture's | its flag | the texture |
| failed | its texture could not be made (*Failure*, below) | false | the size asked for | true | ENFORCE |
| detached | the allocator was destroyed while the handle was live or failed | false | `{0, 0}` | true | ENFORCE |

What each call does in each state:

| Call | empty | live | failed | detached |
|---|---|---|---|---|
| `mark_drawn()` | nothing | clears the flag | nothing: it still needs a redraw | nothing |
| `clear()` | nothing | clears to the spec's colour; the flag is unchanged | nothing | nothing |
| `resize(h, size)` | nothing | a new texture (*Resize*) | tries again at the new size | — (no allocator) |
| destroyed, or assigned over | nothing | its texture goes to the pool | its slot is freed | nothing |
| `on_targets_reset()` | — | cleared, marked for redraw | unchanged | — |
| `on_device_reset()` | — | recreated, cleared, marked for redraw | tried again | — |

*Why only `get()` enforces:* every other call has a harmless answer for a handle without a
texture; `get()` has nothing to return. Callers test the handle first, as the painter does
before drawing into a layer or combining it.

**The spec.**

- **A spec is a size and a clear colour.** Format (ARGB8888), access (target) and filtering
  (nearest) are fixed, so the pool key is the size alone.
- **The clear colour belongs to the handle, not to the texture.** `acquire` clears a reused
  texture to the new handle's colour; `clear()` and the resets use the handle's.
- **A `mod` layer asks for opaque white** (§3.5); every other target for transparent.

**Acquiring.** `acquire(spec, clear = true)`:

1. A size with a zero or negative side gives a failed handle.
2. Otherwise it takes a pooled texture of that size — the one given back last — and counts
   it in `reused`. With none pooled it creates one and counts it in `created`; if SDL
   cannot create it, the handle is failed.
3. It clears the texture to the spec's colour, and the handle needs a redraw. *Why always:*
   a pooled texture holds another scene's pixels, and a new one holds whatever SDL left in
   it; neither may ever be shown. A caller that binds the texture at once may pass
   `clear = false` and clear it as it binds it: `bind_pooled()`, which canvases and layers
   use, does, so a texture is bound once, not twice — each bind flushes SDL's command
   queue.

- **A canvas is acquired at its scene's first frame.** A scene that was never drawn holds
  nothing.
- **A layer takes its target at its first push**, not at `declare_layer`; a layer never
  pushed holds nothing (§3.4).

**Giving back.**

- **Destroying a live handle gives its texture to the pool**, under its size and uncleared:
  `acquire` clears. Assigning over a handle gives its own texture back first.
- **Moving a handle moves its texture.** The moved-from handle is empty and gives nothing
  back.
- **A failed handle frees its slot; an empty or detached one does nothing.**
- **A scene's handles die with its scene context** when the scene leaves the stack; a
  covered scene keeps them. Pushing and popping a pause menu allocates nothing after the
  first time.
- **An every-frame layer gives its target back** at the end of a frame that did
  not push it, and takes one again at its next push (§3.4).
- **A transition keeps the old scene's canvas** by moving its handle out of the context;
  the texture goes back to the pool when the transition ends.

**`needs_redraw()` and `mark_drawn()`.**

- **A handle that needs a redraw holds only its clear colour, or no texture at all.** The
  allocator keeps this true: it clears the texture everywhere it sets the flag — `acquire`,
  `resize`, `on_targets_reset()`, `on_device_reset()`.
- **Only the allocator sets `needs_redraw`; only `mark_drawn()` clears it, and only on a
  live handle.** An empty, failed or detached handle always needs a redraw.
- **Only the painter calls `mark_drawn()`**, at the end of a scene's frame (§3.2, step
  1.7), on each handle drawn on in that frame: every layer pushed, and the canvas if
  anything was drawn on it. *Why at the end of the frame:* `needs_redraw(id)` then gives
  one answer for the whole frame (§2.5).
- **The painter reports it** through `needs_redraw(id)` and `canvas_needs_redraw()`, for
  on-demand layers and canvases; for the others it answers `false`.
- **`has_content(id)` is `pushed || (on_demand && !needs_redraw())`**: an on-demand layer
  that needs a redraw and was not pushed is not combined.
- **`clear()` leaves `needs_redraw()` as it is.** A cleared layer counts as drawn when the
  frame that pushed it ends, like any other.

**Resize.**

- **When the render size changes, the painter resizes its canvas and every layer without a
  `size` of its own**, at its next frame (§3.4). A layer with its own size keeps its
  texture and its contents.
- **`resize(handle, size)` on a live handle:**
  - at the same size it does nothing; the texture keeps its contents and its flag;
  - at another size it destroys the old texture — not pooled — before taking the new one
    the way `acquire` does: pooled at the new size, or created. The new texture is cleared
    and the handle needs a redraw. *Why destroy first:* the old pixels are lost either way,
    and the peak is one texture instead of two;
  - if the new texture cannot be made, the handle is failed at the new size.
- **On a failed handle it tries again at the new size; on an empty one it does nothing.** A
  layer that was never drawn takes the new size when the painter acquires it.
- **Why the old texture is not pooled:** pooling it would keep a set of textures for every
  size the window has had.
- **The pooled textures of the old render size are destroyed too**
  (`drop_pooled(old_size)`): those of a pause menu closed before the resize, for example.
  *Why:* they would be reused only if the window came back to exactly that size. Pooled
  textures of other sizes stay.

**Resets** (§3.9).

- **`on_targets_reset()`** clears every live handle's texture and marks it as needing a
  redraw. Failed handles already need one. The pool is left alone: pooled contents never
  matter.
- **`on_device_reset()`** destroys the pool first, then recreates every live and failed
  handle's texture in place, at its size, cleared and needing a redraw; a texture that
  cannot be made leaves its handle failed. *Why in place:* the scene contexts' handles
  stay valid, so nothing has to find and replace them.

**Leaving the pool.** A pooled texture is reused by `acquire` or `resize` for its size, or
destroyed:

- when the render size changes and it has the old render size (above);
- when the system is low on memory (below);
- on `trim_texture_pool()`;
- on a device reset;
- when the allocator is destroyed.

Nothing else evicts it: there is no age or budget policy (O15). Using the pool does not
make it grow. A texture is created only when none of its size is pooled, so the textures
of one size, held and pooled, never outnumber the most of that size that were held at
once. Only many different sizes make it grow: the render size is handled above, and layers
with a `size` of their own are the game's choice.

**Low memory.**

- **On `SDL_EVENT_LOW_MEMORY` the pool is trimmed.** iOS sends it from
  `applicationDidReceiveMemoryWarning`, Android from `onTrimMemory` (`SDL_events.h:96`).
  *Why only the pool:* held textures are in use; pooled ones can be given back at the cost
  of creating a texture at a later push.
- **The trim waits for the next frame.** SDL dispatches this event at once
  (`SDL_main_callbacks.c:32`), from whatever thread pushed it (`SDL_main.h:404`), and
  `SDL_DestroyTexture` may only be called on the main thread (`SDL_render.h:2623`). The
  event handler calls `request_trim()`, which only sets an atomic flag; `begin_frame()`,
  at the start of `application::on_render`, runs the trim.
- **The flag lives outside the allocator** — `request_trim()` is static. *Why:* the event
  can arrive while `on_quit()` is destroying the allocator; a handler that looked the
  allocator up and called it could reach an object being destroyed. Setting a flag that
  outlives every allocator touches nothing that goes away. A new allocator clears it: a
  request from before it, or from the last application's shutdown, is not about its pool.
- **`begin_frame()` does nothing else.** Evicting by age (O15) would count frames there.

**Failure.** A texture cannot be made when its size has a zero or negative side, or when
SDL refuses to create it. `acquire`, `resize` and `on_device_reset()` then log it, count
it in `draw_failure_count()` (§2.15) and leave the handle failed. A failed handle always
needs a redraw; drawing into it and combining it do nothing; the next `resize` or device
reset tries again.

**Threads.** Everything runs on the main thread except `request_trim()`, which only sets an
atomic flag outside the allocator. Nothing else locks.

**Ownership and lifetime.**

- **The allocator is `application_scoped`** (§3.1, R38), reached internally with
  `require_texture_allocator()` and `maybe_texture_allocator()`. It is never a plain
  static, whose destruction order nobody controls.
- **It is made in `on_ready()`**, once the renderer is published and before the first
  scene enters — early, so it is destroyed after anything made later that may hold its
  handles.
- **It is destroyed in `on_quit()`, when the application scope closes**: after `finish()`
  has emptied the scene stack and `teardown()` has run, and before the resource caches and
  `game_application::on_quit()`. *Why there, not in the application's destructor:* sdlpp's
  quit callback calls `SDL_Quit` right after `on_quit()`. That destroys the window and its
  renderer, and with the renderer every texture made on it, long before the application's
  members are destroyed.
- **Destroying the allocator frees every texture**, pooled or held, and detaches the live
  and failed handles: a detached handle is empty and its destructor does nothing. *Why:* a
  scene that outlives the application, or a handle in a static, is harmless.
- **A handle holds a slot number and looks the allocator up** with
  `maybe_texture_allocator()`, which reads null once the allocator is gone, so a handle
  finds that out without touching freed memory.

**Invariants.** The allocator's tests (RM1) check them after every operation:

- every texture the allocator made is in exactly one place: a live handle or the pool;
- `created − destroyed = held + pooled`;
- a live handle's texture has the handle's size;
- a live handle that needs a redraw holds only its clear colour;
- the textures of one size, held and pooled, never outnumber the most of that size that
  were held at once.

**Statistics.** `texture_pool_stats()` and `trim_texture_pool()` (§2.14) forward to the
application's allocator. The byte counts assume four bytes a pixel; the driver may pad.

### 3.4 The layer stack

One scene's layers live in an internal `detail::layer_stack`, part of its scene context:
their settings, their targets from the texture allocator, the frame's flags and the
record of the layer pushes. A layer push changes where draws go, not how: it does not
touch the painter state, whose saves are the painter's own (§2.4, R39).

```cpp
namespace neutrino::detail {
    class layer_stack {
        public:
            explicit layer_stack(target_geometry);       // what its layers mirror (R42)

            // the scene's layer calls (§2.5)
            layer_id declare(const layer_settings& = {}); // the first also makes the base layer
            layer_id base_layer() const;
            layer_composite_settings& composite(layer_id);
            bool needs_redraw(layer_id) const;           // false for an every-frame layer

            // push_layer, pop_layer and clear() on the scene being rendered
            void push(layer_id);                         // take the target, bind it, clear at the first push
            void pop();
            const texture* target(layer_id) const;       // the layer's texture; null if it has none
            void clear(layer_id);                        // to the layer's clear colour

            // around render() (§3.2)
            void begin_frame(const target_geometry&);    // resize, reset the flags, push the base layer
            void end_frame();                            // pop the base layer, give unused targets back
            void abort_frame() noexcept;                 // render() threw: end every push

            // combining (§2.9, R43): scene_frame runs it, composition reaches it
            void begin_combine();                        // pushing and declaring fail until end_combine()
            void end_combine() noexcept;
            bool combining() const noexcept;
            std::span<const layer_id> layers() const;    // the base layer, then declaration order
            layer_composite_settings& composite(layer_id);
            bool has_content(layer_id) const;            // a texture, and pushed || (on_demand && !needs_redraw)
            void draw_layer(layer_id, const layer_composite_settings&);
            void draw_layers(layer_id first, layer_id last);
            void mark_drawn();                           // every layer pushed this frame, after combining

            // inspection (§2.14)
            layer_info info(layer_id) const;
        private:
            struct push_entry {
                std::size_t                          layer;   // its index in m_layers
                std::optional<renderer_target_scope> target;  // ending it binds the previous target; none for a failed layer
                bool                                 frame;   // begin_frame()'s push of the base layer
            };
            std::vector<layer_entry> m_layers;           // the base layer, then declaration order (§3.1)
            std::vector<layer_id>    m_ids;              // for layers()
            std::vector<push_entry>  m_stack;            // back() is the current layer
            target_geometry          m_geometry;         // what the layers without a size mirror
            std::uint32_t            m_serial;           // carried by every layer_id
            bool                     m_combining;        // between begin_combine() and end_combine()
    };
}
```

**In a scene without layers every hook does nothing**: no base layer is pushed, nothing is
combined, and drawing goes onto the canvas as today.

**Identity.** A `layer_id` holds the serial of the stack that declared it and the layer's
index, which is also its place in the declaration order. Each stack takes the next value
of a global counter when it is made, and every call checks both. *Why a serial, not a
pointer to the stack:* a destroyed scene's memory can be reused by a new one, and an old
id would then match. `layer_id{}` names no layer.

**Declaring.** The first `declare` makes the base layer, at index 0, before the declared
one: at the stack's size, redrawn every frame, with the default composite settings.
`declare` appends and takes no texture. It is allowed at any time except while combining.

**Pushing.** `push(id)`:

1. checks that `id` is a layer of this stack;
2. takes a target from the allocator if the layer holds none, at its own `size` or the
   stack's — the render size until the stack has one — cleared to the layer's clear
   colour: opaque white for a `mod` layer, transparent for any other (§2.8). A held
   texture takes the clear colour of the layer's blend now, for its later refills: the
   blend is live and may have become `mod` or stopped being it. A failed handle is not
   empty and is not retried here (§3.3). Steps 2 and 3 are `detail::bind_pooled()`, which
   a canvas's bind uses too;
3. binds it with `texture::bind()`, clearing it again if this is its first push in the
   frame and its redraw mode is `every_frame`, unless the texture was taken in step 2,
   which came cleared. A failed layer binds nothing;
4. marks the layer pushed this frame and appends a push entry.

`pop()`:

1. checks that something is pushed — `begin_frame()`'s push of the base layer does not
   count — and that the bound target is still the layer's texture: a `texture::bind()`
   scope of the game's that outlives the layer fails here, instead of leaving the wrong
   target bound;
2. ends the entry's binding, which binds the previous target again: the layer below it,
   or the canvas.

- **Pushing the base layer** makes it current again over the layers pushed above it; it
  was cleared at its first push, at the start of the frame.
- **The painter state is not touched** (R39). A helper that changes the colour inside a
  layer and wants the change undone wraps it in `scoped_state()`. A layer push and a
  state push may be popped in any order relative to each other: each stack restores only
  its own — the state, or the target.
- **The clip follows the painter.** SDL keeps a clip per target, so the painter applies
  its clip to whichever target is bound: after every `texture::bind()` and at the end of
  every `renderer_target_scope`, and so after layer pushes and pops. Without a painter
  clip (R39, amended 2026-11-22):
  - a texture just bound gets none, so a pooled one never keeps its last owner's;
  - a target bound again when a scope ends keeps its own clip — one a UI backend set
    straight on the renderer — unless the painter had a clip when the scope began, which
    the target held and which is now gone, so it is cleared;
  - `push_state` / `pop_state` touch the target's clip only when the painter's changes;
  - a `pixel_space` (combining, the copy into the window) puts the target's clip back as
    it was.
- **Coordinates.** SDL keeps the viewport and scale per target, and logical presentation
  applies only to the window, so inside a layer the coordinates are its own pixels: render
  coordinates for a layer at the scene's size.
- **A failed layer** — its target could not be made (§3.3) — is pushed like any other,
  with nothing bound. Until draw calls go through the painter (RM1 step 4) they land on
  the target below it; from then on, drawing into it does nothing.

**The frame**, around `render()` (§3.2):

- **`begin_frame(size)`** resizes every layer without a `size` of its own if `size` is
  not the stack's (`resize`, §3.3), resets every layer's `pushed` flag, and pushes the
  base layer, cleared, as the frame's own entry. ENFORCE: no frame is open.
- **`end_frame()`** checks that only the frame's entry is left — a frame ending with
  another layer pushed ends every push as `abort_frame()` does and fails ENFORCE, naming
  it — pops it, and gives back the target of each every-frame layer that was not pushed.
  An on-demand layer keeps its target; so does a failed one.
- **`abort_frame()`**, when `render()` throws, ends every push from the back, so the
  target bound before the frame is bound again before the scene is popped. It checks
  nothing and never throws.
- **Combining** is done by `scene_frame::finish()`, after `end_frame()` (R43):
  `begin_combine()`, the canvas bound, the scene's compositor run with a `composition`
  over the stack and the default painter state, `end_combine()` however it ends, then
  `mark_drawn()`. Marking after the compositor keeps `has_content()` and `needs_redraw()`
  one answer for the frame.
- **Phases.** The stack is idle, drawing (from `begin_frame()` to `end_frame()`) or
  combining. Pushing or declaring while combining fails ENFORCE; so does `draw_layer`
  outside combining.

**Resizing waits for the next frame.** `begin_frame()` compares sizes, so a scene's layers
are resized when it is next drawn. *Why:* the scene manager's resize notice reaches only
the top scene and the pinned scene; a scene further down, hidden or not, takes the new
size when it is drawn again. The pooled textures of the old render size are dropped once,
by the application (`drop_pooled`, §3.3), not by each stack.

**When a layer holds a texture.**

- **It takes one at its first push**, not at `declare`. A layer never pushed holds
  nothing.
- **An on-demand layer keeps it** until the stack is destroyed; a resize or a device reset
  replaces it in place (§3.3). A change of the logical presentation at the same pixel size
  clears it and marks it for a redraw (`texture_handle::invalidate()`): what it held was
  drawn under another mapping of render space.
- **An every-frame layer gives it back** at the end of a frame that did not push it, and
  takes one again at its next push — usually the same texture, from the pool. A layer
  pushed every frame keeps it. *Why:* an occasional layer — a damage flash — costs nothing
  between uses, and a low-memory trim can free its texture.
- **The base layer** is pushed every frame, so it keeps its texture.
- **A scene hidden under one with `hides_below`** gives back the textures of its
  every-frame layers, the base layer's included, at the start of each frame it is not
  drawn, as it does its every-frame canvas's (R41). Its on-demand layers keep theirs.
- **Destroying the stack** gives every texture back.

**Combining a layer.** `draw_layer(id, settings)` does nothing for a layer without
content; otherwise it is one texture copy onto the bound canvas
(`detail::draw_composited`, `src/neutrino/video/composite_draw.hh`), inside a
`detail::pixel_space`: the canvas's logical presentation and clip are off while it lasts,
and it maps render units onto the canvas's pixels (R43).

| Setting | Becomes |
|---|---|
| `source` | the source rectangle: in render units mapped to pixels for a layer that mirrors the canvas, in its own pixels for one with a `size`; the whole layer when unset |
| `destination` | the rectangle drawn onto, in render units mapped to pixels; unset: the layer in place — 1:1 for a layer that mirrors the canvas, stretched over the render area for one with its own `size` |
| `scale`, `pivot` | that rectangle scaled per axis about the pivot (unset: its centre); 0 draws nothing |
| `rotation_degrees`, `pivot`, `flip` | `copy_ex`'s rotation, centre and flip; `diagonal` fails ENFORCE (R35) |
| `offset` | added last, in render units mapped to pixels |
| `tint`, `opacity` | colour modulation `tint × opacity`, alpha modulation `opacity` (§2.8) |
| `blend` | its premultiplied form (§2.8), set on the texture at every combine |

- *Why the blend mode is set every time:* the texture comes from the pool, and its
  previous owner may have left another one on it.
- `effects` come with RM4.

**What is checked** (ENFORCE, §2.15): pushing a layer of another stack, or `layer_id{}`;
`pop` with nothing pushed, or when the bound target is not the layer's; a frame ending
with a layer still pushed; `begin_frame()` with a frame open; pushing or declaring while
combining; `draw_layer` outside combining; a diagonal flip, a non-finite rotation, or a
scale that is not finite or is negative; `base_layer()` before any layer is declared.
`push_layer` with no scene being rendered is the painter's check (`layer_ops.cc`).

### 3.5 Combining

For each layer the compositor draws, and for each canvas drawn into the window:

- the SDL blend mode is the premultiplied form of the composite `blend` (§2.8);
- colour modulation is `tint × opacity`, alpha modulation is `opacity`;
- flat transforms place the texture as one quad with the placement rules of §2.13;
- effects draw it through a mesh (§4.2);
- the texture is scaled from its size to the destination's.

Both go through `src/neutrino/video/composite_draw.hh`: `pixel_space`, and
`draw_composited()` with every flat setting and the alpha rules (`premultiplied()`,
`apply_composite_modulation()`). A canvas is drawn as a layer that mirrors the target.

A `mod` layer is cleared to opaque white at its first push; every other layer and every
canvas to transparent. For a layer, `layer_stack::draw_layer` does all of this (§3.4).

**The base layer's texture as the canvas** (R41, built 2026-11-22). Combining a scene
with layers first chooses the canvas's texture for the frame. The base layer's texture
serves when the compositor is the default one, the base layer's composite settings are
the identity (opacity 1, a white tint, no offset, scale, rotation, flip, `source` or
`destination`; a blend of `none` or `blend`, not `add`, which keeps the target's alpha),
and the canvas is redrawn every frame. Drawing the base layer onto a
cleared transparent canvas would then copy it exactly, so the copy is skipped: the
declared layers are drawn straight onto the base layer's texture, which the canvas shows
until the scene's next frame (`canvas::share()`, `layer_stack::bind_base()` and
`draw_declared()`). Otherwise the canvas takes its own texture from the pool, cleared, and
gives it back at the first frame where sharing is possible again. A test switch
(`detail::shortcuts().base_layer_as_canvas`) turns sharing off, so the tests check that
both paths give the same pixels.

**The bottom full-screen scene straight into the window** (R44). When the lowest visible
scene has `hides_below`, no layers, an every-frame canvas with identity settings, and no
scene above runs a compositor of its own, `scenes_manager::plan()` has it drawn straight
into the window (`scene_frame` with `frame_target::bound_target`): its canvas gives its
texture back and shows nothing. Otherwise a full-screen scene's canvas, copied with
`none` in place, covers the window, so the scene manager does not clear it
(`covers_target()`), unless the canvas ends the frame with nothing to show, or the scene's
own target could not be made and its `render()` draws into the window.
`detail::shortcuts().bottom_scene_into_target` turns the first off. With the default
compositor the layers are drawn with the presentation and clip turned off once for all of
them (`layer_stack::draw_all()`, `draw_declared()`).

### 3.6 Render state

- **Each draw call sets the SDL state it needs** — target, blend mode, colour and alpha
  modulation, clip — from the painter state, and assumes nothing about what the previous
  call left. *Why:* no state leaks between helpers, layers or scenes.
- **A texture's colour and alpha modulation are restored after the call** (as `tint_scope`
  does in `draw.cc` today), so a texture always rests at white and opaque.
- **`texture::bind()` inside a layer** pushes the texture as a temporary target; when its
  scope ends, the layer's target is bound again.
- **Outside scenes** the painter keeps its state in the same `painter_state`, so
  `get_draw_color()` outside a scene behaves as today.
- **The core is today's `draw.cc`:** `resolve_visual` (address → page and rectangle), quad
  placement including the diagonal path, line and rectangle tessellation, pixel snapping.

### 3.7 Backend

- **SDL_Renderer** through `sdlpp::renderer`. Moving to SDL_Renderer on the GPU backend or
  to raw SDL_GPU changes only this level.
- **No batching of our own.** SDL already merges consecutive same-texture draws on every
  hardware backend (`gpu/SDL_render_gpu.c:1287` and the same loop in the GL, GLES2, D3D,
  D3D11, D3D12 and Vulkan backends); copies are queued as geometry (`SDL_render.c:4047`)
  and a texture's colour modulation becomes vertex colour, so differently tinted quads still
  merge. Merging into one `render_geometry` call ourselves would hurt the software renderer:
  it splits geometry back into per-quad copies (`SDL_render.c:4958`), sends rotated quads
  through the triangle rasteriser instead of `copy_ex`'s rotation, and leaves the page's
  colour and alpha modulation at the last vertex colour (`SDL_render.c:5203`).
- **Target switches flush.** `SDL_SetRenderTarget` sends SDL's queued commands to the GPU
  (`SDL_render.c:2797`); on mobile GPUs it also writes the previous target's tiles to
  memory.
- **Textures in use stay safe.** SDL flushes its queue before a texture that queued
  commands use is changed or destroyed (`SDL_render.c:340`).

### 3.8 Cost

One layer costs one clear and one full-size blend per frame, and two target switches,
each of which flushes SDL's command queue; one visible scene costs one copy of its canvas
into the window. A scene with layers also pays for its base layer, drawn on or not
(§2.5); while the base layer is shown as it is, its texture is the canvas, so the scene
pays for no canvas texture and no extra copy (R41). The bottom full-screen scene without
layers costs what it did before canvases: it draws straight into the window (R44). A
full-screen scene that goes through its canvas covers the window, so the window's clear
is skipped. A scene hidden under one with `hides_below` holds no every-frame texture.

| Size | GPU memory per layer | Software renderer, extra per layer per frame |
|---|---|---|
| 320×200 (KE) | 250 KB | 0.2 ms |
| 640×360 | 0.9 MB | 0.6–0.8 ms |
| 1280×720 | 3.7 MB | 2.5–3.0 ms |
| 1920×1080 | 8.3 MB | 5.7–6.8 ms |

On a desktop GPU this is negligible: five 1080p layers at 60 fps move about 10 GB/s, a few
percent of a discrete GPU's bandwidth. On the software renderer at output resolution it is
not (five 1080p layers ≈ 30 ms); the tests use small sizes.

### 3.9 Device reset

| SDL event | Meaning | Response |
|---|---|---|
| `SDL_EVENT_RENDER_TARGETS_RESET` | render targets lost their contents | `on_targets_reset()` (§3.3): every layer and canvas cleared and marked for redraw, so on-demand layers and on-demand canvases report it; layers drawn every frame are redrawn anyway; baked textures (`compose()`): their owners re-bake |
| `SDL_EVENT_RENDER_DEVICE_RESET` | every texture must be recreated | re-upload every registered atlas; `on_device_reset()` (§3.3): the pool destroyed, every layer and canvas texture recreated in place, cleared and marked for redraw; owners of game textures re-create them |
| `SDL_EVENT_RENDER_DEVICE_LOST` | the device is gone | report and shut down cleanly |

- **The atlas registry keeps each atlas's CPU pages after upload**, so re-upload needs no
  loader. Glyph atlases are rebuilt from their glyph source instead.
- **The painter records nothing, so it cannot redraw lost contents itself.** The owner of a
  layer that needs a redraw draws it on the next frame.
- **Game-owned textures** (baked `compose()` results, streaming textures) are rebuilt by
  their owners on a scene-level notification (`on_textures_lost()`); textures refilled
  every frame need nothing.

### 3.10 From today's code

| Today | Becomes |
|---|---|
| `draw.hh` / `draw.cc`: immediate drawing; colour and blend are global SDL_Renderer state | the same calls on the one painter; its state saved per scene |
| `application::on_render`: clear → `render_scene()` → present | each scene's context in use during its `render()`, its compositor after it, canvases into the window |
| `scenes_manager`: one stack, the pinned scene and `passes_input` (RM10, partly done); `on_pause` only to the scene directly below | a scene context per scene; pausing by change of state |
| simplex adapter: a persistent canvas texture bound around callbacks | the simplex scene's on-demand canvas |
| `texture` (`target` access), `bind()` / `compose()`, `renderer_target_scope` | unchanged; `bind()` cooperates with the layer stack |
| render targets created wherever they are needed; nothing reuses them | layer targets and canvases from the internal `texture_allocator` (§3.3) |
| `blend_mode`: neutrino's own enum, mapped to SDL's internally (R34); the presets | painter state; premultiplied forms when combining; `custom_blend` |
| `get_renderer()` public (`application.hh`, `draw.hh`, `globals.hh`) | hidden |
| `sprite_draw_params`: no tint, crop or fit | `quad_params` and `draw_picture` |
| `world_compositor`, `render_layer`, `layer_view`, `world_renderer`, `sprite_batch` camera mode | redesigned on the painter in RM2 |
| `register_atlas` / `unregister_atlas` public; CPU pages dropped after upload | internal registration; CPU pages kept (RM6) |
| no handler for the three render events | §3.9 (RM6) |
| lib_sdlpp `simplex::sprite_mesh` and `mesh_effect`s | `mesh` and effects in neutrino (RM4) |

---

## 4. Effects

### 4.1 Kinds of effect

"Effect" is not a painter concept. Every effect is one of these:

| Kind | Example | Expressed as |
|---|---|---|
| primitive with animated parameters | brix's laser and cursor | a producer drawing geometry |
| deformed picture | a waving flag, a 3-D card flip | `draw_mesh` on an atlas picture |
| whole-layer transform or deformation | brix's board rotation, an underwater wobble, screen shake | the layer's composite settings, or a texture made with `compose()` drawn through a mesh |
| part of a layer | heat haze over a fire, a ripple | a mesh over part of a layer's texture, drawn by a compositor |
| transition over the finished picture | Doom melt, crossfade, wipe | the old scene's canvas, kept until the transition ends, drawn with the new scene by a wrapping compositor |
| many short-lived quads | particles, sparks | a producer drawing quads |
| per-pixel shader effect | distortion, bloom, lighting | needs a GPU backend; not covered |

- **The Doom melt** cuts the old frame into columns (2 px wide at 320) that start falling
  after random, neighbour-correlated delays; each frame, each column is a quad from the old
  canvas moved down. It cannot be one grid mesh — neighbouring columns would share vertices
  and shear instead of separating — so it is N quads with a source rect each, which is why
  a quad's source may be a texture and a rect.
- **brix's board rotation** today reads the board back from the GPU and builds a new
  texture before projecting it. Here the board is a layer (or a texture made with
  `compose()`) drawn through `rotate_3d`, with no CPU round trip.
- **Effects that run over many frames** (a melt, a transition) keep their own state on the
  scene's clock; the painter only draws their current geometry.

### 4.2 Meshes

Meshes come from lib_sdlpp's `simplex` (`include/simplex/mesh.hh`, `effects.hh`):

- **`sprite_mesh`**, renamed **`mesh`** — a quad subdivided into `cols × rows` cells
  (position, colour, uv) laid out over a destination rect and a source region.
- **`mesh_effect`s** — `wave_horizontal` / `wave_vertical`, `skew_horizontal` /
  `skew_vertical`, `perspective`, `pinch_punch`, `rotate_3d` (pitch, yaw, roll, pivot,
  perspective factor).

A mesh is a producer of textured geometry: `draw_mesh(mesh, source, …)` draws it on the
current layer. The source is an `atlas_address` (resolved at the call to page and
placement) or a texture — a layer, a canvas — so the same effects work on one picture, on a
layer when it is combined, and on a whole frame.

Changes in the port:

- **Subdivide.** `SDL_RenderGeometry` interpolates uv affinely per triangle; a 1×1 quad
  under `rotate_3d` or `perspective` shows the diagonal seam. A grid (e.g. 8×8) hides it.
  Plain 2-D rotation, scale and flip stay single quads.
- **Draw through the painter**, not `sprite_mesh::render(renderer, texture)`.
- **Resolve against the page.** `layout_rect(atlas, frame)` normalises uv against the whole
  texture; with multi-page atlases it must use the resolved page and placement.
- **Vertex colour from tint and alpha.** It is hard-coded white in `layout_rect_impl`;
  fades and pulses need it.
- **Effect order is part of the API** (an ordered list per mesh): effects deform positions
  in sequence, and "wave then rotate" differs from "rotate then wave".
- **Effects run on the scene's clock**, like animations.

---

## 5. Milestones

Each milestone lands green; each numbered step inside RM1 is committed on its own. The RM
numbers match `render_model.md`, `font_plan.md` and `sprite_layer_plan.md`.

```
RM1 ──► RM2
 │
 ├────► RM3 ──► RM4 ──► RM5
 │       │
 └────► RM6 ──► RM7 ──► RM8 (postponed; also needs RM3)

RM10: independent (scene manager only)
```

### RM1 — The painter

The public contract of §2, without effects. Each step is committed on its own.

*Order* (changed 2026-11-16; was painter core, then scene painters and canvases, then
layers): layers come before painter state. They only add to what exists — a scene without
layers draws exactly as today — they hold most of the questions still open, and they give
the texture allocator its first user. Painter state is a refactor of `draw.hh` with
nothing new to show, so it follows the canvases. Since R37 (2026-11-17) there is one
painter, so there are no scene painters to build.

1. **The texture allocator** (§3.3): `texture_allocator` and `texture_handle`, tested
   alone, with the `SDL_EVENT_LOW_MEMORY` and render-reset handlers; failures counted in
   `draw_failure_count()`; `texture_pool_stats()` and `trim_texture_pool()`. *Done
   2026-11-16:* `src/neutrino/video/texture_allocator.*`, `texture_handle.*`,
   `texture_table.*`, `texture_spec.hh`; tests in `test/video/test_texture_allocator.cc`
   (suites `neutrino::detail::texture_allocator` and `neutrino::detail::texture_table`),
   which check the invariants after every step.
2. **Layers.** `detail::layer_stack` (§3.4), tested alone; a scene context owned by
   `base_scene`, with `declare_layer`, `composite(id)` and `base_layer()` as scene
   members; the one painter, owned by the application, with its save stack and the free
   `push_layer` / `pop_layer` / `scoped_layer`, `push_state` / `pop_state` /
   `scoped_state`, `clear()` and `needs_redraw()`; the base layer; on-demand layers; the
   frame steps of §3.2 in the scene manager, with `abort_frame()` when `render()` throws;
   the default compositor; composite settings without effects (§2.7) and the alpha rules
   of §2.8; inspection (§2.14). Until step 3 the compositor draws into the window. Until
   step 4 the state saves hold the renderer's draw colour, blend mode and line style, and
   drawing into a failed layer lands on the target below it. *Done so far (2026-11-18):*
   the layer stack's declare, the base layer, push, pop, begin_frame, end_frame,
   abort_frame and target; layer pushes independent of the painter state, the clip
   following the painter (R39); tests in `test/video/test_layer_stack.cc`. *Exit:* scenes
   without layers draw exactly as today; a layered test scene renders as specified.
3. **Canvases.** Each scene's canvas from the allocator, drawn into the window in stack
   order with the canvas composite settings and the default blend from `hides_below`; the
   compositor draws onto the canvas; `set_canvas_redraw` and `canvas_needs_redraw()`;
   the simplex canvas as the simplex scene's canvas; the pooled textures of the old render
   size dropped on a resize; `below()`; `capture_frame()` combines first; R41: the
   canvas's texture taken when first needed, the base layer's texture as the canvas, with
   a test switch, and the every-frame canvases of hidden scenes given back. *Done so far
   (2026-11-22):* `detail::canvas` (§3.1), owned by `base_scene` and run by
   `scene_frame`; a scene without layers draws onto it, and the scene manager copies the
   canvases into the bound target with every flat composite setting; R42's mirrored
   geometry for canvases and layers; hidden scenes give back the textures of their
   every-frame canvases and layers.
   The compositor (R43): `composition`, `default_compositor`, `set_compositor` /
   `get_compositor`, `below()`, every flat composite setting with `layer_scale` and
   `destination`. Tests in `test/video/test_canvas.cc`, `test/scene/test_scene_frame.cc`,
   `test/scene/test_compositor.cc` and the layer presentation case in
   `test/video/test_layer_stack.cc`. The base layer's texture as the canvas (R41), the
   bottom full-screen scene straight into the window (R44), the default canvas blend from
   `hides_below`, the window's clear skipped under a covering canvas. Not yet: the public
   canvas and layer members (`canvas_composite`, `composite(id)`, …), the simplex canvas
   as the simplex scene's, and `capture_frame()` combining first. *Exit:*
   tutorials, demos, simplex and the games unchanged (±1 on blended pixels in overlays).
4. **Painter state.** The painter's own `painter_state` (colour, blend, line style, clip),
   saved into each scene's context between its frames; `get_painter_state`,
   `push_state(s)` / `scoped_state(s)`; `draw.hh` reads it and sets the SDL state each
   call needs; the saves hold it instead of the renderer's; drawing into a failed layer
   does nothing; `custom_blend`. *Exit:* every existing test pixel-identical.
5. **Custom compositors.** `set_compositor` / `get_compositor` on the scene,
   `composition`, wrapping.
6. **Quads.** `quad_params` with tint and crop; `draw_picture` with the fit modes; every
   placement rule of §2.13. Independent of the painter; may go first.

*Tests:*

- z-order is independent of call order;
- drawing with no layer pushed lands on the base layer, under every declared layer;
  `composite(base_layer())` moves it;
- a layer is cleared at its first push; an on-demand layer survives a frame and is combined
  in a frame that does not push it; an every-frame layer that is not pushed shows nothing
  and gives its texture back; a layer never pushed never takes one;
- premultiplied combining leaves no dark fringe; half opacity halves colour as well as
  alpha; a `mul` layer sees only itself; a `mod` layer leaves uncovered pixels unchanged;
- the canvas's opacity fades the whole scene; an overlay over a full-screen scene;
  `below()` returns the lower scenes' canvases;
- a custom compositor draws one layer twice, mirrored, and hides another; a wrapping
  compositor calls the one it wraps;
- the allocator alone, with its invariants (§3.3) checked after every step (done, step 1);
- `needs_redraw(id)` gives one answer for a whole frame and turns false after a frame that
  pushes the layer; an on-demand layer that needs a redraw and is not pushed shows nothing;
  an on-demand layer that takes a pooled texture never shows its previous owner's pixels; a
  layer with its own `size` keeps its contents through a resize; a scene below the top
  takes the new render size when it is next drawn;
- a `render()` that throws with layers pushed leaves the target it started with bound; a
  `texture::bind()` scope that outlives its layer fails at `pop_layer`;
- a scene popped and pushed again reuses its textures from the pool;
- with sharing on and off, a layered scene gives the same pixels; a scene that shakes its
  base layer takes a canvas texture and gives it back when the shake ends; a scene hidden
  under one with `hides_below` gives its every-frame canvas's texture back, and an
  on-demand canvas keeps it;
- the clip rect clips; a draw colour set in one scene does not leak into the next;
- each ENFORCE of §2.15;
- pixel tests for tint, crop and each fit mode.

*Exit:* everything that draws today draws unchanged on its canvas; a layered test scene
renders as specified.

### RM10 — The pinned scene

`pin_scene` / `unpin_scene` and the slot in `scenes_manager`; pausing by change of state,
with `flags()` read every frame; `passes_input`; resize notices to the pinned scene
(§2.11).

*Done (2026-11-14):* `pin_scene` / `unpin_scene` (`scene_transitions.hh`), the slot,
`passes_input`, resize notices, rollback of a failed pin, `finish()` clearing the slot.
The pinned scene is the last element of `m_stack`, marked by `bool m_pinned`, so
rendering, updating and the input chain need no change; `active_count()` and
`active_top()` give the part of the stack push, pop and replace act on. Tests:
`test/scene/test_scenes_manager.cc`, suites `passes_input` and `pinned scene`.

*Remaining:*

- pausing by change of state, with `flags()` read every frame;
- the safety net: when a frame throws, `application` pops "the top scene", which with a
  pinned scene is the scene below it — it needs to know which scene threw, or the pinned
  scene's calls need their own guard;
- the exit demo.

*Tests still to add:*

- pushing and popping under a pausing pinned scene keeps `on_pause` / `on_resume` balanced;
- under a pause menu, a busy spinner and the game below it both get `on_pause`;
- a `pauses_below` changed at run time takes effect at the next frame.

*Exit:* a console-style scene pinned over KE that slides down on `~`, takes the keyboard
while open and freezes the game. Text input, a log backend and a command registry are
follow-ups outside RM10.

### RM2 — World and sprites on the painter

Designed when RM1 exists. To settle first:

- where the camera transform is applied — the painter applying a view, or producers calling
  `to_screen` as today;
- float positions for world coordinates in draw calls;
- text on a zoomed camera layer: scaled, or kept at its pixel size;
- map layer opacity and tint — documented on `world_layer_header` but never applied by
  `world_renderer` today;
- what becomes of `world_compositor`, `render_layer`, `layer_view` and `texture_layer`;
- y-sorting map objects together with actors.

Then: producers draw on the current layer; KE and Luxor on the painter; `get_renderer()`
hidden, tests on an internal header. *Exit:* KE and Luxor frames identical except ±1 on
blended pixels; nothing outside `src/` calls SDL_Renderer.

### RM3 — Geometry and primitives

Public `draw_geometry` (textured triangles, per-vertex colour); thick and gradient lines,
curves. *Exit:* a brix-style beam (one strip with per-vertex alpha instead of five lines)
and cursor demo.

### RM4 — Meshes and effects

`mesh` and the `mesh_effect`s ported from lib_sdlpp with the changes of §4.2; `draw_mesh` on
atlas pictures, layer textures and canvases. *Exit:* every effect has a pixel test; a
subdivided `rotate_3d` shows no seam.

### RM5 — Layer effects and transitions

Composite `effects` applied when combining; effects on part of a layer; transitions as
wrapping compositors over the old scene's canvas, kept by the scene manager until the
transition ends. There are no explicit painters (R37): rendering into a texture is
`compose()` (§2.12). *Exit:* a brix-style board rotation with no readback; a Doom-style
melt between two scenes.

### RM6 — Atlas lifetime and device reset

`register_atlas` internal; CPU pages kept; handlers for the three render events (§3.9);
atlases re-uploaded; the events call the allocator's `on_targets_reset()` /
`on_device_reset()`, so on-demand layers and on-demand canvases need a redraw;
`on_textures_lost()` for game-owned textures. *Exit:* a forced device reset re-renders a
KE frame pixel-identically.

### RM7 — Text

Planned in [`font_plan.md`](font_plan.md). `draw_text` draws through `draw.hh`, so it lands
on the current layer once RM1 exists, with no change to the call.

### RM8 — Nuklear (postponed)

A Nuklear backend drawing `nk_convert` output as geometry with atlas addresses (glyph
atlases included), clipped with the painter's clip rect, and an engine `nk_user_font` over
`font`. *Exit:* a warlords-style UI panel on its own layer over the world.

---

## 6. References

- [`render_model.md`](render_model.md) — the design record behind this guide.
- [`font_plan.md`](font_plan.md) — text (RM7);
  [`sprite_layer_plan.md`](sprite_layer_plan.md) — sprites, a producer; its M3 needs RM1
  (tint, crop); [`images_atlases_sprites.md`](images_atlases_sprites.md) — atlas
  vocabulary.
- SDL 3.4 source (`cmake-build-*/_deps/sdl3-src/src/`):
  - `render/SDL_render.c` — `FlushRenderCommandsIfTextureNeeded` (340); `IsSupportedBlendMode`
    (1403); the flush in `SDL_SetRenderTarget` (2797); copies queued as geometry (4047);
    the software renderer's geometry path (4958) and its colour modulation (5203);
  - `render/gpu/SDL_render_gpu.c:1287` — merging consecutive draws (the same loop exists in
    every hardware backend);
  - `video/SDL_blit_slow.c:200–213` — alpha modulation in the premultiplied modes.
- lib_sdlpp: `include/simplex/mesh.hh`, `include/simplex/effects.hh`.
- J. Gregory, *Game Engine Architecture*, 3rd ed.: §11.2.7.3 (translucency forces
  back-to-front order).
