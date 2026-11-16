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
   - [2.12 Explicit painters](#212-explicit-painters)
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
- **Scenes own their painters.** Each scene has its layers, its compositor and its canvas;
  the scene manager draws the canvases into the window in stack order.
- **Defaults change nothing; everything is exposed.** Code that draws today keeps working
  unchanged, and every setting is public.

### 1.2 Vocabulary

| Term | Meaning |
|---|---|
| **painter** | the drawing interface: state, layers, compositor and canvas |
| **scene painter** | the painter every scene has; current while the scene is rendered |
| **pinned scene** | the one scene in the slot above the stack; push, pop and replace never touch it |
| **explicit painter** | a painter the game creates with a size; its canvas is a texture to draw elsewhere |
| **direct painter** | the painter outside any scene (tests, tools): it draws on whatever target is bound, as today |
| **current painter** | the painter the free functions of `draw.hh` draw with |
| **layer** | a render target texture a scene declares; drawing goes to it while it is pushed |
| **current layer** | the layer on top of the push stack |
| **base layer** | in a painter with layers, the bottom layer, made with the first `declare_layer`: drawing with no layer pushed goes to it |
| **retained** | a layer (or canvas) that is not cleared at the start of a frame |
| **canvas** | a painter's result; not a layer |
| **composite settings** | how a layer — or a canvas — is drawn when combined: blend, opacity, tint, transform, effects |
| **compositor** | the function that draws a painter's layers onto its canvas |
| **composition** | what a compositor receives: the layers, their textures, the canvases below |
| **combining** | running the compositor, then drawing the canvases into the window |
| **painter state** | what every draw call reads: colour, blend mode, line style, clip, view |
| **producer** | anything above the painter that draws: tilemap, sprites, text, primitives, meshes |
| **quad** | a picture from an atlas placed at a position |
| **mesh** | a subdivided textured quad deformed by effects |
| **needs redraw** | a retained layer or canvas whose texture holds nothing its owner drew — it is new, taken from the pool, resized or reset — so the owner must draw it in full |
| **texture allocator** | the internal owner of every layer target and canvas texture: hands them out, takes them back, keeps unused ones for reuse |
| **texture handle** | a layer's or a canvas's hold on a texture from the allocator; gives it back when destroyed |
| **pool** | the allocator's unused textures, kept for reuse |
| **layer stack** | the internal owner of one painter's layers: their settings, their targets, the push stack (§3.4) |

"Renderer" means SDL_Renderer (`sdlpp::renderer`), which only the painter's internals see.

### 1.3 Levels

| Level | Concepts | Knows about |
|---|---|---|
| **Game** | hero, gem, laser gun, board rotation | game state; picks what each producer shows |
| **Producers** | tilemap, sprites and animations, text, primitives, meshes and effects | time and appearance; they only issue draw calls |
| **Painter** (public) | painters, layers, canvas, compositor, painter state, draw calls | *what* is drawn and where |
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
nuts and bolts — full state, inspection, explicit painters (§2.4, §2.12, §2.14).

### 2.2 What stays as it is

| Today | Contract |
|---|---|
| `draw.hh` free functions, all overloads (`int` and `render_pos`), and their names | same signatures and meaning; they draw with the current painter |
| `set_draw_color` / `set_draw_blend_mode` / `set_line_style` and their getters | same calls; the values are the current painter's state (§2.4) |
| `texture::create`, `from_image`, `adopt` | unchanged; what they make belongs to the caller, not to the texture allocator (§3.3); `adopt` rejects a texture made on another renderer |
| `texture::bind()`, `target_scope`, `compose()`, `draw_texture`, `draw_texture_quad` | unchanged; inside a layer, `bind()` redirects drawing into the texture, and the layer is current again when the scope ends |
| `capture_frame()` | unchanged; it combines first, so it sees what will be shown |
| `render_size()`, `pixel_snap`, `draw_failure_count()` | unchanged |
| `base_scene::render()` | draws onto the scene's canvas, which is drawn into the window |
| `sprite_batch` | unchanged; flushes onto the current layer, or the canvas |
| simplex | unchanged; its canvas is the simplex scene's canvas |

### 2.3 Defaults

| Setting | Default | Why it matches today |
|---|---|---|
| current painter | the scene's painter during `render()`; the direct painter outside scenes | tests and tools draw on the bound target as now |
| layers | none: drawing goes onto the canvas | one picture per scene |
| canvas | the render size, cleared every frame | — |
| canvas blend | `none` for a scene with `hides_below`, `blend` otherwise | a full-screen scene is a plain copy, pixel-identical; an overlay shows the scenes below |
| compositor | `default_compositor` | — |
| painter state | colour, blend and line style as SDL's defaults today; kept between frames | simplex relies on its draw colour persisting |
| view | identity: screen space | — |
| filtering | nearest | pixel art |

Three things change underneath, all intended:

- **Draw state belongs to each painter.** A colour set by one scene no longer leaks into the
  scene drawn after it.
- **Overlay scenes may differ by ±1 on semi-transparent pixels**, because they pass through
  a canvas. Full-screen scenes are identical.
- **Each visible scene costs one copy into the window.**

### 2.4 Painter state

```cpp
struct painter_state {                       // everything a draw call reads
    color               draw_color;          // set_draw_color
    blend_mode          blend;               // set_draw_blend_mode: a preset or custom_blend(...)
    line_style          line;                // set_line_style
    std::optional<rect> clip;                // nothing outside it is drawn
    view                view;                // camera transform; identity until RM2
};

painter_state& state();
void push_state();
void pop_state();
[[nodiscard]] blend_mode custom_blend(/* colour and alpha factors and operations */);
```

- **Each painter has its own state, kept between frames.** *Why:* nothing leaks between
  scenes, and persistent drawing (simplex) keeps its colour.
- **Every draw call reads the state when it is made** and sets the SDL render state it
  needs; nothing depends on what an earlier call left behind.
- **`push_layer` saves the state and `pop_layer` restores it; `push_state` / `pop_state` do
  the same without changing the layer.** *Why:* a helper that changes the colour cannot
  leave it changed for its caller.
- **`clip`** limits drawing to a rectangle. *Why:* UI panels, and Nuklear, which draws with
  scissor rects.
- **`custom_blend`** composes a blend mode from factors and operations
  (`SDL_ComposeCustomBlendMode`). Only the seven presets are guaranteed on every backend;
  the software renderer — the tests — supports no custom blend, and a draw SDL rejects
  counts as a failed draw.

### 2.5 Layers

```cpp
struct layer_settings {                      // every field has a default
    std::optional<dim> size;                 // default: the painter's size
    bool               retained{false};
    layer_composite    composite{};          // §2.7
};

layer_id declare_layer(const layer_settings& = {});
void push_layer(layer_id);
void pop_layer();
[[nodiscard]] layer_scope scoped_layer(layer_id);   // pushes now, pops at the end of the scope
void clear();
layer_composite& composite(layer_id);
[[nodiscard]] bool needs_redraw(layer_id) const;
[[nodiscard]] layer_id base_layer() const;          // the bottom layer (below)
```

**Declaring.** A scene declares its layers on its painter, normally in its constructor.
`declare_layer` appends to the painter's ordered list; that order is the default z-order.
Declaring creates no texture: a layer takes its target at its first push (§3.4).
*Why ahead of time:* the game knows its layers, and a fixed list makes the z-order a
property of the scene instead of the call order.

**The base layer.** The first `declare_layer` also makes the painter's base layer, before
the declared one: it is always the bottom of the default z-order.

- **Drawing with no layer pushed goes to it.** It is current at the start of every frame,
  under every push.
- **It is an ordinary layer**, at the painter's size and not retained. `base_layer()`
  gives its id: `composite(base_layer())` shakes or fades it, and a compositor may draw it
  anywhere, twice, or not at all.
- **It is pushed every frame,** so it is cleared at the start of each frame and always
  combined.
- **Pushing it** makes it current again over the layers pushed above it. It cannot be
  popped: `pop_layer` with only the base layer current is `pop_layer` with nothing pushed.
- *Why:* declaring a layer only adds one. A scene that declares a HUD keeps its other
  drawing as it is, and adopts layers one at a time. *Why a layer and not the canvas:* the
  canvas belongs to the compositor, and content drawn outside every layer can still be
  moved or faded as a whole.
- *Cost:* a scene with layers pays for its base layer — one clear and one combine a frame
  — whether it draws on it or not (§3.8).

**Pushing.**

- `push_layer(id)` makes the layer current and saves the painter state; `pop_layer()`
  makes the previous one current and restores the state. `scoped_layer` does both with
  RAII, so an exception cannot leave a layer pushed.
- **The first push of a layer in a frame clears it**, unless it is retained. Pushing it
  again later in the frame continues drawing on it.
- **Pushes nest:** a helper may push the HUD while the world is current. Nesting changes
  which target is current, never the order.
- **Pushing never changes the z-order.** *Why:* code may draw the HUD before the world, and
  the HUD is still on top.
- **Once a painter has declared layers, drawing goes only to layers**: with none pushed,
  to the base layer (above). *Why:* the canvas then belongs to the compositor.
- **One push per layer per frame** is the rule of thumb: every switch of target flushes
  SDL's queue (§3.7).

**Order.**

| Between | Order |
|---|---|
| layers | the compositor's; by default declaration order |
| draws within a layer | call order — the painter's algorithm; in 2D nearly everything is blended, so back to front is required |
| sprites by depth | `sprite_batch`: `(draw_layer, depth, insertion order)`, stable; anything that must sort with sprites goes through the batch |

**`retained`** decides what a layer holds in a frame that does not draw it:

| | Not retained (default) | Retained |
|---|---|---|
| first push in a frame | clears the layer | keeps last frame's contents; draws add to them |
| a frame that does not push it | not combined: shows nothing | combined with its old contents |
| emptied by | every frame | `clear()` only |

Uses: a HUD redrawn only when the score changes; a background drawn once; accumulation
effects such as trails. Caveats:

- **It can need drawing in full again**: `needs_redraw(id)` says when (below).
- **It holds its texture every frame**, drawn or not.
- **A camera layer is rarely retained**: once the camera moves, the old contents are stale.

**`needs_redraw(id)`** says that a retained layer holds nothing its owner drew, so the
owner must draw all of it:

```cpp
if (m_score_changed || p.needs_redraw(m_hud)) {
    auto s = p.scoped_layer(m_hud);
    p.clear();
    draw_score();
}
```

- **It is true** in the layer's first frame, after a resize if the layer follows the
  painter's size, and after SDL resets the render targets or the device (§3.9).
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
- **For a layer that is not retained it is always false**: such a layer keeps nothing, and
  its owner draws it every frame anyway.

**`clear()`** clears the current layer — the base layer when none is pushed, or the canvas
in a painter without layers — to transparent; a `mod` layer to opaque white (§2.8).

**`size`** gives a layer its own resolution; it is scaled to the canvas when combined.

### 2.6 The canvas

```cpp
layer_composite& canvas_composite();            // how a scene's canvas is drawn into the window
void set_canvas_retained(bool);                 // the canvas persists between frames
[[nodiscard]] bool canvas_needs_redraw() const; // needs_redraw(id) for a retained canvas
[[nodiscard]] const texture& canvas() const;    // the result
```

- **The canvas is the painter's result, not a layer.** It has the painter's size — the
  render size for a scene.
- **Without declared layers, drawing goes straight onto the canvas**, as today; there is
  nothing to combine and the compositor is not called.
- **With declared layers, the canvas belongs to the compositor.** It is cleared at the
  start of the frame unless retained, and the compositor draws the layers onto it. Drawing
  with no layer pushed goes to the base layer (§2.5), not to the canvas.
- **The canvas's composite settings** say how a scene's canvas is drawn into the window over
  the scenes below. Fading a whole pause menu in is `p.canvas_composite().opacity = t`. The
  default blend is `none` with `hides_below` and `blend` without.
- **`set_canvas_retained(true)`** keeps the canvas between frames: simplex's persistent
  canvas.
- **`canvas_needs_redraw()`** is `needs_redraw(id)` for a retained canvas, with the same
  rules: true in the first frame and after a resize or a reset, false at the end of a frame
  that drew on the canvas, always false for a canvas that is not retained. Simplex records
  nothing, so its canvas after a reset stays empty; an explicit painter's owner draws it
  again (§2.12).
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
struct layer_composite {
    blend_mode blend{blend_mode::blend};   // none, blend, add, mul, mod (§2.8)
    float      opacity{1.0f};
    color      tint{colors::white};        // fade to black, damage flash, night
    point      offset{};                   // screen shake
    float      scale{1.0f};
    float      rotation_degrees{0.0f};
    point      pivot{};                    // centre of scale and rotation
    flip       mirror{};
    std::optional<rect> source;            // only part of the layer
    std::vector<mesh_effect> effects;      // ordered: rotate_3d, wave, perspective… (§4)
};

p.composite(m_world).offset  = m_shake.offset();
p.composite(m_hud).opacity   = m_fade.value();
p.composite(m_board).effects = {rotate_3d{.yaw = m_angle}};
```

- **Live:** change them at any time; the compositor reads them when it runs. Animating
  them each frame on the scene's clock is how layer effects move.
- **Flat transforms** — offset, scale, rotation about the pivot, mirror, `source` — draw
  the layer as one quad with the placement rules of §2.13.
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
| `mod` | `mod` | opaque white | tinting everything below. `src × dst` ignores alpha, so a transparent pixel would turn what is below black |

- **Opacity scales colour as well as alpha.** In the premultiplied modes SDL's alpha
  modulation scales only alpha, so colour is modulated by `tint × opacity`.
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

void set_compositor(compositor);
[[nodiscard]] const compositor& get_compositor() const;

void default_compositor(composition& c) {
    for (const layer_id id : c.layers()) {    // declaration order
        if (c.has_content(id)) {              // pushed this frame, or retained and not needing a redraw
            c.draw_layer(id);                 // onto the canvas, with its composite settings
        }
    }
}
```

Every painter starts with `default_compositor`. It is a public function: a compositor of
the game's own can call it as a building block, and `set_compositor(default_compositor)`
puts it back.

| `composition` call | Does |
|---|---|
| `layers()` | the base layer, then the declared layers, in declaration order |
| `has_content(id)` | pushed this frame, or retained and not needing a redraw |
| `draw_layer(id)` | draws the layer onto the canvas with its composite settings |
| `draw_layer(id, settings)` | draws it with other settings: a second copy, a part, another blend |
| `draw_layers(first, last)` | `draw_layer(id)` for each layer with content from `first` to `last` |
| `texture(id)` | a layer's texture, for `draw_texture`, `draw_mesh` or quads with source rects |
| `below()` | the canvases of the visible scenes below, bottom to top, read-only (§2.10) |

- **The free functions draw onto the canvas** while a compositor runs: a vignette, a melt,
  a debug overlay.
- **Pushing a layer inside a compositor is an error**: the layers are finished.
- **A layer the compositor does not draw is not shown** — debug views for free.

```cpp
// a water reflection: the world drawn a second time, mirrored and faded, under the HUD
p.set_compositor([&](composition& c) {
    c.draw_layers(sky, world);
    c.draw_layer(world, {.opacity = 0.3f, .offset = {0, water_y}, .mirror = flip::vertical});
    c.draw_layer(hud);
});
```

**Wrapping.** A transition does not need to know how the game composes its frame: it
wraps the current compositor and puts it back when it ends.

```cpp
const compositor scene = p.get_compositor();
p.set_compositor([scene, &melt](composition& c) {
    scene(c);           // the new scene, composed as the game composes it
    melt.draw();        // columns of the old scene's canvas falling over it
});
// when the melt ends:
p.set_compositor(scene);
```

*Why two levels:* settings cover screen shake, fades, brix's board rotation and night
overlays without code; a compositor covers the rest — a layer drawn twice, an overlay
between layers, a transition, split screen — without inventing an effects language.

### 2.10 Scenes

```cpp
class play_scene : public neutrino::base_scene {
    painter&       m_p     = get_painter();
    const layer_id m_world = m_p.declare_layer();
    const layer_id m_hud   = m_p.declare_layer({.retained = true});

    void render() override {
        {
            auto s = m_p.scoped_layer(m_world);
            m_map.draw();
            m_batch.flush();
        }
        if (m_score_changed || m_p.needs_redraw(m_hud)) {
            auto s = m_p.scoped_layer(m_hud);
            m_p.clear();
            draw_text({4, 4}, m_score_text, m_font);
        }
    }
};
```

- **Every scene has a painter**, reached with `get_painter()`; it is current while the
  scene is rendered, so the free functions draw with it. *Why per scene:* a menu, gameplay
  and a loading screen need different layers, and the scene's lifetime manages their
  textures.
- **A frame:** the scene manager walks the visible scenes bottom to top, from the topmost
  one with `hides_below`. For each it makes the scene's painter current, calls `render()`,
  and, if the scene declared layers, runs its compositor; the pinned scene comes last
  (§2.11). Then the canvases are drawn into the window in that order, each with its canvas
  composite settings.
- **Overlays** are scenes whose canvas is blended over the scenes below.
- **Layers are private to their scene.** Pushing another scene's layer is an error. A
  compositor reads other scenes only through `below()`: their canvases, never their
  layers. A pause menu that wants the game blurred or warped draws those canvases itself,
  as its own bottom content.
- **Transitions need no capture.** The old scene's canvas is its last frame; the scene
  manager keeps it until the transition ends, and a wrapping compositor draws it with the
  new scene (§2.9).
- **Simplex is a scene.** `application_adapter::create_initial_scene()` installs a
  `simplex_scene`; its persistent canvas becomes that scene's retained canvas.
- **A scene rendered outside the scene manager** — a test calling `scene.render()`
  directly — draws with the direct painter. That works for scenes without declared layers;
  a layered scene is rendered through the manager (tests get a helper).

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
        get_painter().canvas_composite().offset = {0, int((m_slide - 1.0f) * m_height)};
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

### 2.12 Explicit painters

```cpp
explicit painter(dim size);
[[nodiscard]] painter& current_painter();
class painter_scope;                         // makes an explicit painter current for a scope

painter board{{256, 256}};
{
    painter_scope use{board};                // the free functions now draw into board
    draw_board();
}
draw_mesh(board.canvas(), rotate_3d{.yaw = angle});   // on the scene's current layer
```

An explicit painter has everything a scene painter has — state, layers, compositor,
canvas — and its canvas is a texture that any painter can draw, any number of times,
through any mesh. Producer code written with free functions draws into it unchanged.

| Use | How |
|---|---|
| baking (`compose()`, KE's backdrop) | an explicit painter drawn once |
| brix's board rotation | the board drawn into an explicit painter, then drawn through a `rotate_3d` mesh: no readback |
| a picture used many times or in several scenes | its canvas drawn wherever it is needed |

- **Its textures come from the texture allocator** (§3.3) when it is first made current,
  and go back to the pool when it is destroyed.
- **`board.canvas_needs_redraw()` tells its owner to draw it again**: true until it is
  first drawn, and after a reset; it turns false when its canvas is final (RM5 settles
  when that is).

A HUD needs no explicit painter (a retained layer does it), nor does a transition (the old
scene's canvas does it).

### 2.13 Quads

```cpp
enum class flip : std::uint8_t {             // bit flags, as today's sprite_flip
    none = 0, horizontal = 1u << 0u, vertical = 1u << 1u, diagonal = 1u << 2u /* tiles */
};
struct quad_params {                         // today's sprite_draw_params plus tint and crop
    float  scale{1.0f};
    flip   mirror{};
    float  rotation_degrees{0.0f};
    color  tint{colors::white};              // colour and alpha, this call only
    insets crop{};                           // source inset, in picture pixels
};
enum class fit : std::uint8_t { stretch, aspect, centre };

void draw_sprite(point position, atlas_address picture, const quad_params& = {});
void draw_picture(const rect& dst, atlas_address picture, flip = flip::none, fit = fit::stretch);
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
[[nodiscard]] std::span<const layer_id> layers() const;       // the base layer, then declaration order
[[nodiscard]] layer_info info(layer_id) const;                // size, retained, pushed this frame, needs redraw
[[nodiscard]] const texture& layer_texture(layer_id) const;   // read-only: draw it, mesh it, dump it
[[nodiscard]] painter_stats stats() const;                    // last frame: draw calls, target switches,
                                                              // layers combined, failed draws
[[nodiscard]] pool_stats texture_pool_stats();  // held and pooled, bytes; created, reused, destroyed
void trim_texture_pool();                       // destroys every pooled texture
```

A draw call can be stepped into and its effect inspected on the layer texture; layers and
canvases can be dumped with `texture::to_image()`.

### 2.15 Errors

- **ENFORCE at the caller's line:**
  - an unknown atlas or picture index;
  - `push_layer` with a layer the painter did not declare, another scene's layer included;
  - `push_layer` outside its painter's frame — for a scene, outside `render()`;
  - `pop_layer` with nothing pushed;
  - `pop_layer` when the bound target is not the layer's: a `texture::bind()` scope
    outlived the layer;
  - a frame ending with a layer still pushed (the message names it);
  - drawing a layer's or a canvas's own texture into itself;
  - `base_layer()` in a painter that declared no layers;
  - pushing a layer inside a compositor;
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
    layer_settings  settings;        // size, retained, composite (live; read when combining)
    texture_handle  target;          // from the texture allocator; taken at the first push
    bool            pushed{false};   // pushed this frame
};

class painter {
    detail::layer_stack        m_layers;            // the layers and the push stack (§3.4)
    painter_state              m_state;
    std::vector<painter_state> m_states;            // saved by push_state
    texture_handle             m_canvas;            // from the texture allocator
    bool                       m_canvas_drawn{false};
    layer_composite            m_canvas_composite;
    bool                       m_canvas_retained{false};
    compositor                 m_compositor{default_compositor};
};
```

A scene's painter belongs to `base_scene`, so `get_painter()` works in the scene's member
initialisers, where layers are declared (§2.10). A `layer_id` carries its layer stack's
serial as well as its index (§3.4), so a layer of another painter is rejected. The current
painter is a pointer the scene manager sets around each `render()`
and `painter_scope` sets for its scope; otherwise it is the direct painter.

### 3.2 A frame

`application::on_render` (`src/neutrino/application.cc:326`) clears the window to black,
calls `render_scene()` and presents. `scenes_manager::render()`
(`src/neutrino/scene/scenes_manager.cc:230`) walks the visible scenes bottom to top. With
the painter, after the allocator's `begin_frame()` (§3.3):

1. For each visible scene:
   1. at the scene's first frame, acquire its canvas from the texture allocator (§3.3); a
      layer takes its target at its first push (§3.4);
   2. clear the canvas unless it is retained, and reset its `drawn` flag;
   3. make the scene's painter current, with the canvas bound; `begin_frame()` on its
      layer stack resizes the layers if the render size changed, resets their `pushed`
      flags and, in a painter with layers, pushes the base layer;
   4. call `render()`; if it throws, `abort_frame()` ends every push, so the canvas is
      bound again before the scene is popped;
   5. `end_frame()`: check that no layer is left pushed, pop the base layer, and give back
      the targets of the layers that are not retained and were not pushed;
   6. if the scene declared layers, run its compositor with the canvas bound;
   7. `mark_drawn()` every handle drawn on this frame: each layer pushed, and the canvas if
      anything was drawn on it.
2. Do the same for the pinned scene, last.
3. Draw the canvases into the window in that order, each with its canvas composite
   settings. A canvas that is not retained and had nothing drawn on it this frame is not
   copied, so a hidden pinned scene costs nothing.
4. Present.

`capture_frame()` runs the combining first, so it reads what will be shown.

### 3.3 Targets and the texture allocator

The texture allocator is the internal owner of the painter's render targets: every layer
target and every canvas, of scene painters and of explicit painters. It creates them, hands
them out as handles, takes them back when the handles die and keeps them for reuse. It also
keeps each handle's "needs a redraw" flag true to what the texture holds, through resizes,
resets and low memory. *Why one class:* clearing, reuse, loss, resize and device reset are
decided in one place and tested without a scene manager.

**What it owns, and what it does not.** The public ways to make a texture stay (§2.2):
`texture::create`, `from_image` and `adopt`. What they make is not the allocator's.

| Texture | Made by | Owned by | Pooled | After a reset (§3.9) |
|---|---|---|---|---|
| layer targets, scene canvases, explicit painters' canvases | `acquire` | the painter, through a handle | yes | the allocator clears or recreates it and marks it for redraw |
| baked results (`compose()`), streaming textures, any other game texture | `texture::create`, `from_image` | the game | no | its owner re-bakes or re-creates it |
| sprite and glyph atlas pages | the texture registry (`adopt`, `from_image`) | the registry | no | re-uploaded or rebuilt |

The simplex canvas is a game texture today. It becomes the simplex scene's retained canvas
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
            void request_trim();                 // any thread: trim at the next begin_frame()
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

**Acquiring.** `acquire(spec)`:

1. A size with a zero or negative side gives a failed handle.
2. Otherwise it takes a pooled texture of that size — the one given back last — and counts
   it in `reused`. With none pooled it creates one and counts it in `created`; if SDL
   cannot create it, the handle is failed.
3. It clears the texture to the spec's colour, and the handle needs a redraw. *Why always:*
   a pooled texture holds another scene's pixels, and a new one holds whatever SDL left in
   it; neither may ever be shown.

- **A canvas is acquired when its painter is first drawn:** a scene painter's at its
  scene's first frame, an explicit painter's when it is first made current. A scene that
  was never drawn holds nothing.
- **A layer takes its target at its first push**, not at `declare_layer`; a layer never
  pushed holds nothing (§3.4).

**Giving back.**

- **Destroying a live handle gives its texture to the pool**, under its size and uncleared:
  `acquire` clears. Assigning over a handle gives its own texture back first.
- **Moving a handle moves its texture.** The moved-from handle is empty and gives nothing
  back.
- **A failed handle frees its slot; an empty or detached one does nothing.**
- **A scene's handles die with its painter** when the scene leaves the stack; a covered
  scene keeps them. Pushing and popping a pause menu allocates nothing after the first
  time.
- **A layer that is not retained gives its target back** at the end of a frame that did
  not push it, and takes one again at its next push (§3.4).
- **A transition keeps the old scene's canvas** by moving its handle out of the painter;
  the texture goes back to the pool when the transition ends.

**`needs_redraw()` and `mark_drawn()`.**

- **A handle that needs a redraw holds only its clear colour, or no texture at all.** The
  allocator keeps this true: it clears the texture everywhere it sets the flag — `acquire`,
  `resize`, `on_targets_reset()`, `on_device_reset()`.
- **Only the allocator sets `needs_redraw`; only `mark_drawn()` clears it, and only on a
  live handle.** An empty, failed or detached handle always needs a redraw.
- **Only the painter calls `mark_drawn()`**, at the end of its frame (§3.2, step 1.7), on
  each handle drawn on in that frame: every layer pushed, and the canvas if anything was
  drawn on it. An explicit painter marks its canvas when the canvas is final. *Why at the
  end of the frame:* `needs_redraw(id)` then gives one answer for the whole frame (§2.5).
- **The painter reports it** through `needs_redraw(id)` and `canvas_needs_redraw()`, for
  retained layers and canvases; for the others it answers `false`.
- **`has_content(id)` is `pushed || (retained && !needs_redraw())`**: a retained layer
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
  cannot be made leaves its handle failed. *Why in place:* the painters' handles stay
  valid, so nothing has to find and replace them.

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
- **`begin_frame()` does nothing else.** Evicting by age (O15) would count frames there.

**Failure.** A texture cannot be made when its size has a zero or negative side, or when
SDL refuses to create it. `acquire`, `resize` and `on_device_reset()` then log it, count
it in `draw_failure_count()` (§2.15) and leave the handle failed. A failed handle always
needs a redraw; drawing into it and combining it do nothing; the next `resize` or device
reset tries again.

**Threads.** Everything runs on the main thread except `request_trim()`, which only sets an
atomic flag. Nothing else locks.

**Ownership and lifetime.**

- **The application owns the allocator.** It is never a static, whose destruction order
  nobody controls.
- **It is created in `on_ready()`**, once the renderer is published and before the first
  scene enters.
- **It is released in `on_quit()`**: after `finish()` has emptied the scene stack,
  `teardown()` has run and the resource caches are gone, and before
  `game_application::on_quit()`. *Why there, not in the application's destructor:* sdlpp's
  quit callback calls `SDL_Quit` right after `on_quit()`. That destroys the window and its
  renderer, and with the renderer every texture made on it, long before the application's
  members are destroyed.
- **Destroying the allocator frees every texture**, pooled or held, and detaches the live
  and failed handles: a detached handle is empty and its destructor does nothing. *Why:* an
  explicit painter that outlives the application, or a handle in a static, is harmless.
- **How a handle reaches its slot is the implementation's choice** — a slot it shares with
  the allocator, or a slot number looked up through the application — with one
  requirement: once the allocator is gone, a handle finds that out without touching freed
  memory.

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

One painter's layers live in an internal `detail::layer_stack`: their settings, their
targets from the texture allocator, the push stack and the frame's flags. It knows its
size and the allocator, not scenes, so an explicit painter (§2.12) uses the same class.
The painter's layer calls forward to it; the scene manager calls its frame hooks around
`render()` (§3.2).

```cpp
namespace neutrino::detail {
    class layer_stack {
        public:
            explicit layer_stack(dim size);              // the painter's size: render_size() for a scene

            // the painter's layer calls (§2.5)
            layer_id declare(const layer_settings& = {}); // the first also makes the base layer
            layer_id base_layer() const;
            void push(layer_id);
            void pop();
            void clear();                                // the current layer
            layer_composite& composite(layer_id);
            bool needs_redraw(layer_id) const;           // false for a layer that is not retained

            // the scene manager, around render() (§3.2)
            void begin_frame(dim size);
            void end_frame();
            void abort_frame() noexcept;
            void combine();                              // the default compositor, then mark_drawn()

            // compositions (§2.9) and inspection (§2.14)
            std::span<const layer_id> layers() const;
            bool has_content(layer_id) const;            // pushed || (retained && !needs_redraw)
            const texture& layer_texture(layer_id) const;
            void draw_layer(layer_id, const layer_composite&);
            layer_info info(layer_id) const;
        private:
            struct push_entry {
                std::size_t  layer;                      // its index in m_layers
                target_scope target;                     // from texture::bind(); ending it binds the previous target
                saved_state  state;                      // restored by pop()
            };
            std::vector<layer_entry> m_layers;           // the base layer, then declaration order (§3.1)
            std::vector<push_entry>  m_stack;            // back() is the current layer
            dim                      m_size;
            std::uint32_t            m_serial;           // carried by every layer_id
            phase                    m_phase;            // idle, drawing or combining
    };
}
```

**In a painter without layers every hook does nothing**: no base layer is pushed, nothing
is combined, and drawing goes onto the canvas as today.

**Identity.** A `layer_id` holds the serial of the stack that declared it and the layer's
index, which is also its place in the declaration order. Each stack takes the next value
of a global counter when it is made, and every call checks both. *Why a serial, not a
pointer to the stack:* a destroyed scene's memory can be reused by a new one, and an old
id would then match. `layer_id{}` names no layer.

**Declaring.** The first `declare` makes the base layer, at index 0, before the declared
one. `declare` appends and takes no texture. It is allowed at any time except while
combining.

**Pushing.** `push(id)`:

1. takes a target from the allocator if the layer has none, at its own `size` or the
   stack's;
2. binds it with `texture::bind()`, clearing it to the layer's clear colour if this is its
   first push in the frame and it is not retained;
3. saves the state and appends a `push_entry`; the layer counts as pushed this frame.

`pop()`:

1. checks that the bound target is still the layer's texture: a `texture::bind()` scope of
   the game's that outlives the layer fails here, instead of leaving the wrong target
   bound;
2. ends the entry's `target_scope`, which binds the previous target again: the layer below
   it on the stack;
3. restores the saved state and removes the entry. The base layer is not popped: `pop()`
   with only the base layer on the stack fails ENFORCE.

- **Pushing the base layer** makes it current again over the layers pushed above it; it
  was cleared at its first push, in `begin_frame()`.
- **Coordinates.** SDL keeps the viewport and scale per target, and logical presentation
  applies only to the window, so inside a layer the coordinates are its own pixels: render
  coordinates for a layer at the painter's size.
- **The saved state**, until painter state exists (RM1 step 4): the draw colour, the blend
  mode and the line style, which are global today. Then it is the `painter_state` (§2.4).
- **A failed layer** — its target could not be made (§3.3) — is pushed like any other,
  with nothing bound. Until draw calls go through the painter (RM1 step 4) they land on
  the target below it; from then on, drawing into it does nothing.

**The frame.**

- **`begin_frame(size)`:**
  1. if `size` is not the stack's, resizes every layer without a `size` of its own
     (`resize`, §3.3);
  2. resets every layer's `pushed` flag;
  3. pushes the base layer, cleared.
- **`end_frame()`** checks that only the base layer is on the stack — a frame ending with
  a layer still pushed fails ENFORCE, naming it — pops the base layer, and gives back the
  target of every layer that is not retained and was not pushed.
- **`abort_frame()`**, when `render()` throws: ends every entry from the back, restoring
  the state, so the target bound before `begin_frame()` is bound again before the scene is
  popped. It checks nothing.
- **`combine()`** runs the default compositor onto the bound target — the canvas; until
  canvases exist, the window — then calls `mark_drawn()` on every layer pushed this frame.
- **Phases.** The stack is idle, drawing (from `begin_frame()` to `end_frame()`) or
  combining. Pushing outside a frame, and pushing or declaring while combining, fail
  ENFORCE.

**Resizing waits for the next frame.** `begin_frame()` compares sizes, so a painter
resizes its layers when it is next drawn. *Why:* the scene manager's resize notice reaches
only the top scene and the pinned scene; a scene further down, hidden or not, takes the
new size when it is drawn again. The pooled textures of the old render size are dropped
once, by the application (`drop_pooled`, §3.3), not by each stack.

**When a layer holds a texture.**

- **It takes one at its first push**, not at `declare`. A layer never pushed holds
  nothing.
- **A retained layer keeps it** until the stack is destroyed; a resize or a device reset
  replaces it in place (§3.3).
- **A layer that is not retained gives it back** at the end of a frame that did not push
  it, and takes one again at its next push — usually the same texture, from the pool. A
  layer pushed every frame keeps it. *Why:* an occasional layer — a damage flash — costs
  nothing between uses, and a low-memory trim can free its texture.
- **The base layer** is pushed every frame, so it keeps its texture.
- **Destroying the stack** gives every texture back.

**Combining a layer.** `draw_layer(id, settings)` is one `draw_texture` call onto the
bound target:

| Setting | Becomes |
|---|---|
| `source` | the source rectangle; the whole layer when unset |
| `offset`, `scale`, `pivot` | the destination: the stack's size, scaled about the pivot and moved by the offset |
| `rotation_degrees`, `pivot`, `mirror` | `draw_texture`'s rotation, pivot and flip |
| `tint`, `opacity` | colour modulation `tint × opacity`, alpha modulation `opacity` (§2.8) |
| `blend` | its premultiplied form (§2.8), set on the texture at every combine |

- A layer with its own `size` is scaled to the stack's size.
- *Why the blend mode is set every time:* the texture comes from the pool, and its
  previous owner may have left another one on it.
- `effects` come with RM4.

**What the stack checks** (ENFORCE, §2.15): a layer of another painter, or `layer_id{}`;
pushing outside a frame or while combining; declaring while combining; `pop()` with
nothing pushed; `pop()` when the bound target is not the layer's; a frame ending with a
layer still pushed; `base_layer()` before any layer is declared.

### 3.5 Combining

For each layer the compositor draws, and for each canvas drawn into the window:

- the SDL blend mode is the premultiplied form of the composite `blend` (§2.8);
- colour modulation is `tint × opacity`, alpha modulation is `opacity`;
- flat transforms place the texture as one quad with the placement rules of §2.13;
- effects draw it through a mesh (§4.2);
- the texture is scaled from its size to the destination's.

A `mod` layer is cleared to opaque white at its first push; every other layer and every
canvas to transparent. For a layer, `layer_stack::draw_layer` does all of this (§3.4).

### 3.6 Render state

- **Each draw call sets the SDL state it needs** — target, blend mode, colour and alpha
  modulation, clip — from the painter state, and assumes nothing about what the previous
  call left. *Why:* no state leaks between helpers, layers or scenes.
- **A texture's colour and alpha modulation are restored after the call** (as `tint_scope`
  does in `draw.cc` today), so a texture always rests at white and opaque.
- **`texture::bind()` inside a layer** pushes the texture as a temporary target; when its
  scope ends, the layer's target is bound again.
- **The direct painter** keeps its state in the same `painter_state`, so `get_draw_color()`
  outside a scene behaves as today.
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

One layer costs one clear and one full-size blend per frame; one visible scene costs one
copy of its canvas into the window. A scene with layers also pays for its base layer,
drawn on or not (§2.5).

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
| `SDL_EVENT_RENDER_TARGETS_RESET` | render targets lost their contents | `on_targets_reset()` (§3.3): every layer and canvas cleared and marked for redraw, so retained layers, retained canvases and explicit painters report it; layers drawn every frame are redrawn anyway; baked textures (`compose()`): their owners re-bake |
| `SDL_EVENT_RENDER_DEVICE_RESET` | every texture must be recreated | re-upload every registered atlas; `on_device_reset()` (§3.3): the pool destroyed, every layer and canvas texture recreated in place, cleared and marked for redraw; owners of game textures re-create them |
| `SDL_EVENT_RENDER_DEVICE_LOST` | the device is gone | report and shut down cleanly |

- **The atlas registry keeps each atlas's CPU pages after upload**, so re-upload needs no
  loader. Glyph atlases are rebuilt from their glyph source instead.
- **The painter records nothing, so it cannot redraw lost contents itself.** The owner of a
  layer or explicit painter that needs a redraw draws it on the next frame.
- **Game-owned textures** (baked `compose()` results, streaming textures) are rebuilt by
  their owners on a scene-level notification (`on_textures_lost()`); textures refilled
  every frame need nothing.

### 3.10 From today's code

| Today | Becomes |
|---|---|
| `draw.hh` / `draw.cc`: immediate drawing; colour and blend are global SDL_Renderer state | the same calls on the current painter; state per painter |
| `application::on_render`: clear → `render_scene()` → present | each scene's painter current during its `render()`, its compositor after it, canvases into the window |
| `scenes_manager`: one stack, the pinned scene and `passes_input` (RM10, partly done); `on_pause` only to the scene directly below | scene painters; pausing by change of state |
| simplex adapter: a persistent canvas texture bound around callbacks | the simplex scene's retained canvas |
| `texture` (`target` access), `bind()` / `compose()`, `target_scope` | unchanged; `bind()` cooperates with the layer stack |
| render targets created wherever they are needed; nothing reuses them | layer targets and canvases from the internal `texture_allocator` (§3.3) |
| `blend_mode` (sdlpp): the presets | painter state; premultiplied forms when combining; `custom_blend` |
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
| whole-layer transform or deformation | brix's board rotation, an underwater wobble, screen shake | the layer's composite settings, or an explicit painter drawn through a mesh |
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
  texture before projecting it. Here the board is an explicit painter (or a layer) drawn
  through `rotate_3d`, with no CPU round trip.
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
nothing new to show, so it follows the canvases.

1. **The texture allocator** (§3.3): `texture_allocator` and `texture_handle`, tested
   alone, with the `SDL_EVENT_LOW_MEMORY` and render-reset handlers; failures counted in
   `draw_failure_count()`; `texture_pool_stats()` and `trim_texture_pool()`. *Done
   2026-11-16:* `src/neutrino/video/texture_allocator.*`, `texture_handle.*`,
   `texture_table.*`, `texture_spec.hh`; tests in `test/video/test_texture_allocator.cc`
   (suites `neutrino::detail::texture_allocator` and `neutrino::detail::texture_table`),
   which check the invariants after every step.
2. **Layers.** `detail::layer_stack` (§3.4), tested alone; a `painter` with only its layer
   calls, owned by `base_scene` and reached with `get_painter()`; the base layer;
   `declare_layer`, `push_layer` / `pop_layer` / `scoped_layer`, `clear()`, retained
   layers and `needs_redraw()`; the scene manager's frame hooks, with `abort_frame()` when
   `render()` throws; the default compositor; composite settings without effects (§2.7)
   and the alpha rules of §2.8; inspection (§2.14). Until step 3 the compositor draws into
   the window. Until step 4 `push_layer` saves the global draw colour, blend mode and line
   style, and drawing into a failed layer lands on the target below it. *Exit:* scenes
   without layers draw exactly as today; a layered test scene renders as specified.
3. **Canvases.** Each scene's canvas from the allocator, drawn into the window in stack
   order with the canvas composite settings and the default blend from `hides_below`; the
   compositor draws onto the canvas; `set_canvas_retained` and `canvas_needs_redraw()`;
   the simplex canvas as the simplex scene's canvas; the pooled textures of the old render
   size dropped on a resize; `below()`; `capture_frame()` combines first. *Exit:*
   tutorials, demos, simplex and the games unchanged (±1 on blended pixels in overlays).
4. **Painter state.** `painter_state` (colour, blend, line style, clip), `push_state` /
   `pop_state`, `current_painter()`, the direct painter; `draw.hh` forwards to the current
   painter; `push_layer` saves the painter state; drawing into a failed layer does
   nothing; `custom_blend`. *Exit:* every existing test pixel-identical.
5. **Custom compositors.** `set_compositor` / `get_compositor`, `composition`, wrapping.
6. **Quads.** `quad_params` with tint and crop; `draw_picture` with the fit modes; every
   placement rule of §2.13. Independent of the painter; may go first.

*Tests:*

- z-order is independent of call order;
- drawing with no layer pushed lands on the base layer, under every declared layer;
  `composite(base_layer())` moves it;
- a layer is cleared at its first push; a retained layer survives a frame and is combined
  in a frame that does not push it; a non-retained layer that is not pushed shows nothing
  and gives its texture back; a layer never pushed never takes one;
- premultiplied combining leaves no dark fringe; half opacity halves colour as well as
  alpha; a `mul` layer sees only itself; a `mod` layer leaves uncovered pixels unchanged;
- the canvas's opacity fades the whole scene; an overlay over a full-screen scene;
  `below()` returns the lower scenes' canvases;
- a custom compositor draws one layer twice, mirrored, and hides another; a wrapping
  compositor calls the one it wraps;
- the allocator alone, with its invariants (§3.3) checked after every step (done, step 1);
- `needs_redraw(id)` gives one answer for a whole frame and turns false after a frame that
  pushes the layer; a retained layer that needs a redraw and is not pushed shows nothing;
  a retained layer that takes a pooled texture never shows its previous owner's pixels; a
  layer with its own `size` keeps its contents through a resize; a scene below the top
  takes the new render size when it is next drawn;
- a `render()` that throws with layers pushed leaves the target it started with bound; a
  `texture::bind()` scope that outlives its layer fails at `pop_layer`;
- a scene popped and pushed again reuses its textures from the pool;
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

### RM5 — Layer effects, explicit painters, transitions

Composite `effects` applied when combining; effects on part of a layer; explicit painters
and `painter_scope`; transitions as wrapping compositors over the old scene's canvas, kept
by the scene manager until the transition ends. To settle first: when an explicit
painter's canvas is final, how it follows a resize, and whether `compose()` becomes an
explicit painter. *Exit:* a brix-style board rotation with no readback; a Doom-style melt
between two scenes.

### RM6 — Atlas lifetime and device reset

`register_atlas` internal; CPU pages kept; handlers for the three render events (§3.9);
atlases re-uploaded; the events call the allocator's `on_targets_reset()` /
`on_device_reset()`, so retained layers, retained canvases and explicit painters need a
redraw; `on_textures_lost()` for game-owned textures. *Exit:* a forced device reset
re-renders a KE frame pixel-identically.

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
