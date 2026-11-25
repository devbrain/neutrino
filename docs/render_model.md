# Painter — how neutrino draws

Status: **draft for discussion.** Written 2026-11-04 as a deferred design that recorded and
sorted paint commands; **rewritten 2026-11-09** around an immediate painter that draws into
declared layer textures; **2026-11-10** adds the compositor (§5.4), palette animation
(§6.2), scenes that own their layers and draw onto a canvas (§5.4, §5.6, §5.7), and the
public contract — defaults that need no porting, every setting exposed (§3.7);
**2026-11-17** makes the painter one application-wide object, with each scene owning its
layers, canvas and compositor (R37, §5.5). §11
separates what is agreed from what is only proposed and what is still open; the discussion
is not finished. Nothing here is implemented beyond what §10 lists as existing. The
agreed design, without the reasoning trail, is the implementation guide
[`painter.md`](painter.md). Companion documents: [`images_atlases_sprites.md`](images_atlases_sprites.md)
(atlas vocabulary), [`sprite_layer_plan.md`](sprite_layer_plan.md) (P3.7 — sprites, a
*producer* that draws through the painter) and [`font_plan.md`](font_plan.md) (text).

---

## 0. Vocabulary

| Term | Meaning |
|---|---|
| **painter** | the only way to draw in neutrino: every draw call (sprites, text, lines, curves, tiles, meshes) goes to the painter's current layer |
| **scene context** | every scene has one: its layers, compositor, canvas, painter state and save stack. The one painter uses it while the scene is current — during every call the scene manager makes into it — so the free functions of `draw.hh` draw into it with its state (§5.6, R37, R48) |
| **layer** | a texture the painter draws into, declared ahead of time by the scene; layers are combined in declaration order (§5) |
| **canvas** | a scene's result, not a layer. Without declared layers draw calls go straight onto it; with layers the compositor draws them onto it. A scene's canvas is what the window shows of that scene (§5.4) |
| **current layer** | the layer selected by `push_layer`; draw calls go there |
| **redraw mode** | `every_frame` (default): a layer or canvas is cleared at its first push in each frame; `on_demand`: its contents persist until cleared or redrawn (§5.1, R33). An *on-demand layer* has `on_demand` |
| **combining** | the end-of-frame step that draws a scene's layers onto its canvas; the scene's compositor does it (§5.4) |
| **compositor** | the function that combines. Every scene starts with `default_compositor` (each layer with content, in declaration order, with its style); `set_compositor` replaces it with the game's own (§5.4) |
| **layer style** | a layer's `layer_style` — blend, opacity, tint, transform, effects — read when the layer is combined, changed through `style_of(id)` every frame if need be (§5.4.2, R45; was `layer_composite_settings`) |
| **quad** | a picture from an atlas placed on a layer (§3.2) |
| **geometry** | triangles, optionally textured |
| **mesh** | a subdivided textured quad deformed by effects (§6) |
| **producer** | anything above the painter that draws: tilemap, sprites, text, primitives, beams, meshes, particles |

"Renderer" stays reserved for SDL_Renderer (`sdlpp::renderer`), which only the painter's
internals see.

## 1. Why

Drawing today has three ordering models: `draw.hh` draws immediately in call order with
global colour and blend state, `sprite_batch` sorts by `(draw_layer, depth, insertion
order)`, and `world_compositor` orders its slots, with `texture_layer` drawing around the
batch. Effects that act on a whole layer or on the finished picture — brix's board
rotation, a Doom-style screen melt — have no place to live: once something is in the
backbuffer it cannot be rotated on its own. This document pins down one painter that every
producer draws through, with layers that are combined, and transformed, at the end of the
frame.

## 2. Levels

| Level | Concepts | Knows about |
|---|---|---|
| **Game** | hero, gem, laser gun, board rotation | game state; picks what each producer shows |
| **Producers** | tilemap, `sprite` + `animation`, text, primitive helpers, beams, meshes + effects, particles | time and appearance; each one only issues draw calls |
| **Painter** (public) | the one painter, scene layers, `push_layer` / `pop_layer`, painter state, draw calls, compositors | *what* to draw and on which layer |
| **Internals** (private) | layer targets, combining, render state, atlas resolution, backend | *how* it reaches the GPU |

Rules:

- **The painter never knows about animation.** An animation is a producer choosing which
  `atlas_address` to draw this frame. A tile, a character frame and a glyph are the same
  thing to the painter: a quad cut from an atlas picture.
- **The painter is the only interface for drawing** (R1). The world, simplex, the demos,
  the tutorials and the games all draw through it. `get_renderer()`, public today
  (`application.hh`, `draw.hh`, `globals.hh`), becomes hidden (RM2), so nothing outside
  neutrino calls SDL_Renderer. There is no escape hatch: what SDL_Renderer can do is
  reachable through the painter (§3.7). Outside `src/` only four test files call
  `get_renderer()`; tests link `neutrino_objects` and include an internal header instead.

## 3. The painter — public contract

```cpp
// sketch — names provisional
class play_scene : public base_scene {
    // the scene's layers: z-order is declaration order (§5.1); the canvas is the result (§5.4)
    const layer_id m_world = declare_layer({.space = layer_space::camera});
    const layer_id m_hud   = declare_layer({.redraw = redraw_mode::on_demand});

    void render() override {
        push_layer(m_world);                           // free functions on the one painter (R37)
            tilemap.draw();                            // producers draw on the current layer
            sprites.draw();
        pop_layer();
        if (score_changed || needs_redraw(m_hud)) {
            push_layer(m_hud);
            clear();
            set_draw_color(colors::yellow);            // painter state (§3.3)
            draw_text({4, 4}, score_text, font);
            pop_layer();
        }
    }
    // after render() (engine): world, then hud, combined onto the canvas (§5.4);
    // the scenes' canvases combined into the window in stack order (§5.6)
};
```

### 3.1 Drawing is immediate

A draw call renders into the current layer's texture when it is called. Nothing is
recorded, sorted or replayed (R2). The 2026-11-04 draft recorded commands; every reason for
that is gone:

| Why recording was planned | Now |
|---|---|
| fewer draw calls | SDL 3.4 already merges consecutive draws with the same texture, blend and scale mode into one GPU draw call on every hardware backend (GL, GLES2, D3D9/11/12, Vulkan, GPU — `gpu/SDL_render_gpu.c:1287`). Those backends queue copies as geometry (`SDL_render.c:4047`) and turn a texture's colour mod into vertex colour, so differently tinted quads still merge |
| order across producers | layers are combined in declaration order, whatever order the code draws them in (§3.4) |
| state that cannot leak | each draw call sets the render state it needs, as `draw.cc` does today; `pop_layer` restores the painter state (§3.3) |
| effects on a whole layer | a layer is a texture, complete at the end of the frame (§5.4) |
| errors at the caller's line | immediate drawing reports there already (§3.5) |
| baking and readback as "render boundaries" | nothing is pending; `compose()` works as today, and `screenshot()` draws the scenes again to read them (R46) |
| textures freed between record and render | nothing is pending on our side, and SDL flushes its own queue before a texture in use is changed or destroyed (`FlushRenderCommandsIfTextureNeeded`, `SDL_render.c:340`) |

Our own batching into one `render_geometry` call is not wanted either: on the software
renderer (the tests, machines without a GPU) SDL splits it back into per-quad copies
(`SDL_SW_RenderGeometryRaw`, `SDL_render.c:4958`); rotated quads then go through the
triangle rasteriser instead of `copy_ex`'s rotation, so their pixels differ; and that path
leaves the page's colour and alpha mod set to the last vertex colour (`SDL_render.c:5203`).

### 3.2 Quads — placing a picture

A quad is a picture from an atlas placed at a position. Its parameters:

```cpp
// flip_mode (flip_mode.hh): bit flags none, horizontal, vertical, diagonal (tiles) (R35)

struct quad_params {                  // today's sprite_draw_params + tint and crop
    float     scale{1.0f};
    flip_mode flip{};
    float     rotation_degrees{0.0f};
    color     tint{colors::white};    // colour + alpha modulation, this call only
    insets    crop{};                 // source inset in picture pixels
};
enum class fit : std::uint8_t { stretch, aspect, centre };

void draw_sprite(point position, atlas_address picture, const quad_params& = {});
void draw_picture(const rect& dst, atlas_address picture, flip_mode = flip_mode::none, fit = fit::stretch);
```

The blend mode comes from the painter state (§3.3; a per-call parameter is O8).

Placement rules (moved here from the sprite plan's D11; all exist in `draw.cc` today and
are preserved):

- `position` is the picture's **pivot** (D9: per-picture default, per-frame override),
  snapped to whole pixels.
- Scale and rotation are about the pivot; flip mirrors the pivot with the picture.
- Diagonal flip (tiles) goes through textured geometry.
- Rotation is byte-identical to the unrotated path at 0°.
- `tint` modulates colour and alpha for this call only.
- `crop` insets the source; the destination size follows the cropped source (brix's six
  `draw_tile_*` variants).
- `draw_picture(dst, address, flip, fit)` fills a destination rect: `stretch`, `aspect`
  (fit inside, keep ratio), `centre` (no scaling). The exact-integer `stretch` path is the
  seam-free one tilemaps use (warlords's portraits and shields need the fit modes).
- Every engine studied has tint; warlords needs colour/alpha mod and the fit modes.

A quad's source may also be a texture plus a source rect — a layer or an explicit
painter's result — not only an atlas picture (the Doom melt, §6.1).

### 3.3 Painter state

- The painter keeps a drawing state, `painter_state`: draw colour, blend mode, line style,
  clip rect and, from RM2, the view (§3.7). Today's `set_draw_color` /
  `set_draw_blend_mode` / `set_line_style` set renderer state (the line style lives in
  sdlpp's renderer wrapper, R36); they become painter state (R4). Each scene owns its
  state and its save stack, current during every call the scene manager makes into it,
  and painter calls with no scene current fail (R48): a colour set by one scene no longer
  leaks into another, and simplex keeps relying on its draw colour persisting. The blend
  applies to every draw, textures included; a texture's alpha mode picks SDL's form (R49).
- `push_layer(id)` selects the layer and `pop_layer()` the previous one; neither touches
  the painter state (R39): the state says how to draw, a layer where. A helper that wants
  its colour changes undone wraps them in `scoped_state()`.
- Each draw call reads the state at the call and sets the SDL render state it uses itself
  (R48); nothing depends on what the previous call left in SDL_Renderer.
- An RAII guard beside the pair (`auto s = scoped_layer(world);`), as `std::lock_guard`
  sits beside `lock` / `unlock`, so an exception cannot leave a layer pushed (R10). Agreed
  (R24, was O2): `push_state` / `pop_state`, and `scoped_state()`, for helpers that change
  the state without selecting a layer. State pushes and layer pushes are independent:
  each restores only its own, so they may be popped in any order relative to each other
  (R39).

### 3.4 Order

- **Across layers: declaration order**, applied when the layers are combined. Code may draw
  the HUD before the world; the HUD is still on top.
- **Within a layer: call order** — the painter's algorithm, as `draw.hh` today. In 2D,
  order is correctness: nearly everything is alpha-blended, so back-to-front is mandatory
  (Gregory §11.2.7.3).
- **Depth sorting is a producer's job.** The only user today is y-sorting sprites, which
  `sprite_batch` already does (`(draw_layer, depth, insertion order)`, stable). It stays as
  the one small recorder, local to sprites (R12). Anything that must sort against sprites by
  depth goes through the batch or onto its own layer.

### 3.5 Errors at the call site

- Unknown atlas or picture index: ENFORCE at the caller's line, as `draw.hh` today. An
  invalid address ("no picture") is a silent no-op.
- `push_layer` with a layer of a scene that is not being rendered, `push_layer` outside a
  scene's `render()`, `pop_layer` with nothing pushed, `pop_layer` when the layer's target
  is no longer bound, a frame ending with a layer still pushed (the message names it),
  drawing a layer's or a canvas's own texture into itself, `style_of` in a scene without
  layers or on the base layer, pushing a layer inside a compositor: all ENFORCE. Drawing
  with no layer pushed goes to the base layer (R31).
- What only SDL can reject (a failed call, target creation) stays in the painter's
  per-frame reporting (`draw_stats.failed`, `DRAW_CHECK`).

### 3.6 Resources and debugging

Public handles only: `atlas_id`, `atlas_address`, `texture` and `layer_id`.
Because drawing is immediate, a handle only has to be valid during the call.

A draw call can be stepped into and its effect inspected on the layer texture; layers can be
dumped with `texture::to_image()`.

### 3.7 The public contract: defaults, and nuts and bolts

Agreed 2026-11-10 (R27), two rules:

1. **It works out of the box.** Code that draws today draws unchanged with the defaults;
   nothing is ported.
2. **Everything is exposed.** Every setting the painter has is public, and its default is
   today's behaviour.

**Tier 0 — as today.**

| Today | Contract |
|---|---|
| `draw.hh` free functions, all overloads (`int` and `render_pos`) | same signatures and meaning; they draw with the one painter (R37) |
| `set_draw_color` / `set_draw_blend_mode` / `set_line_style` and their getters | the same calls; the values are the painter's state (§3.3) |
| `texture::bind()`, `renderer_target_scope` (was `target_scope`), `compose()`, `draw_texture*` | unchanged but for the scope's name; inside a layer, `bind()` redirects drawing into the texture and the layer is current again when the scope ends |
| `render_size()`, `pixel_snap`, `draw_failure_count()` | unchanged; `capture_frame()` is replaced by `screenshot()` (R46) |
| `base_scene::render()` | draws onto the scene's canvas, which is drawn into the window |
| simplex | unchanged; its picture stays its own texture, copied onto the simplex scene's canvas (R47) |

The defaults that make it true:

| Setting | Default | Why it matches today |
|---|---|---|
| scene context in use | the current scene's, during every call the scene manager makes into it; outside a scene none, and painter calls fail (R48) | every game runs inside a scene |
| layers | none: draw calls go onto the canvas | one picture per scene |
| canvas | the render size, cleared each frame; blend `none` with `hides_below`, `blend` without | a full-screen scene is a plain copy, pixel-identical; an overlay shows the scenes below |
| compositor | `default_compositor` | — |
| state | opaque black, blend `blend`, solid lines, no clip; each scene's own (R48, R49) | simplex relies on the colour persisting |
| view | identity (screen space) | — |
| filtering | nearest | pixel art |

Behind the user's back, five things change, all accepted: each scene has its own draw
state (a colour no longer leaks from one scene into the next, R48); drawing outside a
scene fails (R48); the default blend is `blend`, for textures too (R49); overlay scenes
get ±1 on semi-transparent pixels; each visible scene costs one copy into the window.

**Tier 1 — layers.** On the scene: `declare_layer`, `style_of(id)`, `set_compositor`.
Free functions on the painter: `push_layer` / `pop_layer` / `scoped_layer`, `clear()`,
`needs_redraw(id)` (§5).

**Tier 2 — nuts and bolts.**

```cpp
struct painter_state {                       // everything a draw call reads
    color               draw_color;          // set_draw_color
    blend_mode          blend;               // set_draw_blend_mode: a preset or a custom blend
    line_style          line;                // set_line_style
    std::optional<rect> clip;                // UI panels; Nuklear's scissor rects (R25)
    view                view;                // RM2: identity by default
};

enum class redraw_mode : std::uint8_t { every_frame, on_demand };   // §5.1 (R33)

struct layer_settings {                      // every field has today's default
    std::optional<dim> size;                 // default: the scene's size (O7)
    redraw_mode        redraw{redraw_mode::every_frame};   // §5.1
    layer_style        style{};              // §5.4.2 (R45)
};

class base_scene {                           // the scene's layers and compositor (R37, R45)
public:
    // compositor
    void set_compositor(compositor);
    [[nodiscard]] const compositor& get_compositor() const;
protected:
    // layers: the scene's own; the base layer is internal (R45)
    layer_id declare_layer(const layer_settings& = {});
    layer_style& style_of(layer_id);         // live: how the layer is drawn when combined

    // no canvas members: a scene's picture is copied as it is, with the blend its flags give

    // inspection
    [[nodiscard]] std::span<const layer_id> layers() const;       // declaration order
    [[nodiscard]] layer_info info(layer_id) const;                // size, redraw mode, drawn this frame, needs redraw
    [[nodiscard]] const texture& layer_texture(layer_id) const;   // read-only: draw it, mesh it, dump it
    // ... and today's members
};

// free functions on the one painter
void push_layer(layer_id);  void pop_layer();
[[nodiscard]] layer_scope scoped_layer(layer_id);
void clear();                                       // the current layer, or the canvas without layers
[[nodiscard]] bool needs_redraw(layer_id);
[[nodiscard]] painter_state get_painter_state();
void push_state();  void pop_state();               // R24
void push_state(const painter_state&);              // save, then replace the whole state (R37)
[[nodiscard]] painter_state_scope scoped_state();
[[nodiscard]] painter_state_scope scoped_state(const painter_state&);
[[nodiscard]] painter_stats get_painter_stats();    // last frame: draw calls, target switches,
                                                    // layers combined, failures
[[nodiscard]] pool_stats texture_pool_stats();     // the texture allocator's pool (§5.6)
void trim_texture_pool();
// custom_blend(/* factors and operations */): R26, deferred until a game needs one (R48)
```

- **Custom blend modes** (R26) wrap `SDL_ComposeCustomBlendMode`; neutrino encodes them in
  its own `blend_mode` and maps them to SDL's internally, like the presets (R34). Only the
  seven presets are required of every backend (`IsSupportedBlendMode`,
  `SDL_render.c:1403`); the software renderer — the tests — supports no custom blend, so a
  rejected one counts as a failed draw.
- **Below the painter, still public:** `draw_geometry` (RM3: textured triangles with
  per-vertex colour, the primitive every other draw can be expressed with), meshes and
  effects (RM4), `palette` (RM9), and the `composition` calls including `below()` (§5.4.4).

## 4. Internals — private

Everything below the painter lives in `src/` and is not installed:

- **the texture allocator** (R20): every layer target and canvas is a handle from it. A
  canvas is acquired at its scene's first frame, a layer's target at the layer's first
  push (R32); both are given back when the scene is destroyed, and an every-frame layer
  gives its target back after a frame that did not push it. Resized with the render size,
  at the scene's next frame, unless a layer has its own size; cleared at the first push of
  a frame unless it is redrawn on demand (§5.3, §5.6);
- **the layer stack** (R32): one scene's layers, their targets and the frame's flags,
  part of the scene's context; the painter's save stack does the pushing (R37,
  [`painter.md`](painter.md) §3.4);
- **combining** (§5.4) and the frame boundary. `application::on_render`
  (`src/neutrino/application.cc:326`) clears, calls `render_scene()` and presents. In
  `render_scene()` the scene manager renders each visible scene with its context in use,
  runs its compositor, and draws the scenes' canvases into the window before `present()`
  (§5.6), the pinned scene's last (§5.7). An every-frame canvas that had nothing
  drawn on it this frame is not copied;
- **render state** per draw call (blend, colour mod, clip, bound target);
- **the backend.** Today `sdlpp::renderer`. Moving to SDL_Renderer on the GPU backend, or
  to raw SDL_GPU (§12), changes only this level.

**The data.** A layer is more than a render target: it is an entry in an ordered list.
`push_layer` / `pop_layer` only choose the current entry; the list's order is what the
default compositor follows. Each scene holds its context, its painter state and saves
included (R48); one painter context per application says what is current
([`painter.md`](painter.md) §3.1):

```cpp
struct layer_entry {
    layer_settings  settings;        // size, redraw mode, composite (live; read when combining)
    texture_handle  target;          // from the texture allocator; taken at the first push
    bool            pushed{false};   // pushed this frame
};
struct scene_context {               // one per scene, owned by base_scene
    detail::layer_stack      layers;            // the base layer, then declaration order (R32)
    texture_handle           canvas;            // the result, from the texture allocator
    blend_mode               canvas_blend;      // from the scene's flags, before each copy (R45)
    compositor               compositor{default_compositor};
    painter_state            saved;             // the painter state between the scene's frames
};
struct painter_context {             // one: application_scoped<painter_context> (R38)
    painter_state_stack*     scene{nullptr};    // the current scene's state and saves (R48)
    scene_frame*             frame{nullptr};    // the frame open now
    bool                     discards{false};   // the bound target could not be made
};
```

The helpers in `src/neutrino/video/draw.cc` are its core: `resolve_visual` (address →
page + rect), quad placement including the diagonal path, line / rect tessellation, pixel
snapping.

**Target switches cost a flush.** `SDL_SetRenderTarget` sends SDL's queued commands to the
GPU (`SDL_render.c:2797`); on mobile GPUs it also writes the previous target's tiles to
memory. Pushing a layer again later in the frame is allowed but costs another switch; the
rule of thumb is one push per layer per frame.

### 4.1 Relationship to `draw.hh`

`draw.hh` keeps its free functions and their names (R11, proposed): each draws with the
one painter (R37), on the current layer of the scene being rendered.

**A scene with no declared layers draws straight onto its canvas** (R23). Tutorials, demos
and simplex keep their drawing code; each scene costs one copy into the window (§5.6).

**Outside any scene** — the pixel tests, tools, `on_create` — no scene context is in use:
draw calls go straight to the bound target or the window, as today, so `test_draw` and the
other tests that read the window back stay unchanged.

## 5. Layers

### 5.1 Declared ahead of time

Each scene knows which layers it uses (R18, §5.6): `declare_layer(settings)`, a
`base_scene` member, returns a `layer_id`, and declaration order is the default z-order
(R3). The canvas is not a layer (§5.4). Proposed settings (R9):

| Setting | Meaning |
|---|---|
| `space` | screen (identity) or camera (camera × parallax × zoom) — today's `layer_view` / `world_layer_header`; where the transform is applied is O4 |
| `resolution` | logical size (default) or output size (O7) |
| `redraw` | `every_frame` (default), or `on_demand`: contents persist between frames (below) |
| `style` | the initial `layer_style` — how the layer is combined over the layers below it: blend, opacity, tint, transform, effects (§5.4.2); unlike the settings above, it can change every frame through `style_of(id)` (R45) |

**`redraw`** decides what a layer holds in a frame that does not draw it:

| | `every_frame` (default) | `on_demand` |
|---|---|---|
| first push in a frame | clears the layer | keeps last frame's contents; draw calls add to them |
| a frame that does not push it | not combined: the layer shows nothing | combined with its old contents |
| emptied by | every frame | `clear()` only |

Uses: a HUD redrawn only when the score changes; a background drawn once; accumulation
effects (trails: each frame draws a translucent rectangle, then the new positions); a
picture that persists (an on-demand layer, R45). Costs and caveats:

- **It can need drawing in full again.** `needs_redraw(id)` tells the owner to draw all of
  it (R13): `if (changed || p.needs_redraw(hud))`. It is true in the layer's first frame,
  after a resize (if the layer follows the painter's size) and after a render-target or
  device reset (§8.3). Such a layer holds only its clear colour and is not combined until
  pushed, so it shows nothing rather than stale pixels. It turns false at the end of a
  frame that pushed the layer, so it gives one answer for the whole frame: two helpers
  that check it both draw. For an every-frame layer it is always false. The rules
  are in [`painter.md`](painter.md) §2.5 and §3.3.
- **It holds its texture every frame**, drawn or not.
- **A camera layer is rarely redrawn on demand.** When the camera moves, its old contents
  are stale; only screen-space layers or a still camera benefit.

Layers are declared per scene, not once per application (R18; was O1).

### 5.2 Every layer is a texture

Agreed (R3): each layer is a render target texture, always. That gives one code path and one
meaning: a layer is a separate image combined over the ones below it.

- **Premultiplied alpha.** Alpha-blended drawing into a cleared, transparent layer leaves
  premultiplied colour in it, so layers are combined with the premultiplied blend modes
  (§5.4.1). Combining with plain blending would apply alpha twice: dark fringes on
  anti-aliased text and soft edges.
- **A layer is isolated.** Draw calls with blend modes that read the destination (mod, mul)
  see only their own layer, not the layers below. A darkness or lighting overlay is a layer
  whose `blend` is mul — Photoshop layer semantics — not mul-blended draw calls.
- **±1 on blended pixels.** A semi-transparent pixel that passes through a layer can differ
  by one from drawing straight into the window; opaque pixels are identical. Exit criteria
  that compare frames allow for it.
- **Output size, through each target's view** (R42; was "logical size for free"). SDL 3.4
  keeps the logical presentation per target. Layers and canvases take the output size and
  the window's logical presentation, so drawing into them lands on the same pixels as
  drawing into the window, and combining is a 1:1 copy.

Cost per layer per frame: one clear and one full-size blend.

| Size | GPU memory per layer | Software renderer, extra per layer per frame (measured 2026-11-09) |
|---|---|---|
| 320×200 (KE) | 250 KB | 0.2 ms |
| 640×360 | 0.9 MB | 0.6–0.8 ms |
| 1280×720 | 3.7 MB | 2.5–3.0 ms |
| 1920×1080 | 8.3 MB | 5.7–6.8 ms |

On a desktop GPU this is negligible: five 1080p layers at 60 fps move about 10 GB/s, a few
percent of a discrete GPU's bandwidth; integrated GPUs feel it, but it fits. On the
software renderer at output resolution it is not (five 1080p layers ≈ 30 ms per frame); the
tests use small sizes, so only machines without a GPU would notice. If that ever matters, a
layer with no settings can be drawn straight into the frame — for normal and additive
blending the picture is the same, since blending "over" is associative. That is an
optimisation, not part of the model.

### 5.3 `push_layer` / `pop_layer`

- `push_layer(id)` makes the layer current and binds its target; `pop_layer()` returns to
  the previous one and restores the painter state (§3.3).
- The first push of a layer in a frame clears it — to transparent, or to white for a `mod`
  layer (§5.4.1) — unless its redraw mode is `on_demand`; `clear()` clears the current
  layer on request.
- Pushing a layer again later in the same frame continues drawing on it (one more target
  switch, §4).
- Pushes nest: a helper may push the HUD while the world is current. Nesting changes only
  the current target, never the z-order.
- In a scene with declared layers, `render()` starts with the base layer current:
  drawing with no layer pushed goes to it, under every declared layer (R31). Without
  declared layers there is nothing to push: draw calls go onto the canvas.
- A frame that ends with a layer pushed is an error (§3.5).

### 5.4 Combining — the compositor

Drawing is immediate; combining is the one step that waits for the end of the frame. A
painter combines its layers onto its **canvas** by calling its **compositor** (R14). For a
scene this happens right after its `render()` (§5.6).

**The canvas is the result, not a layer** (R23). Three concepts, one job each:

| Concept | Job |
|---|---|
| `declare_layer` | which layers exist; appends to the painter's list, whose order is the default z-order |
| `push_layer` / `pop_layer` | where draw calls go now; never changes the order |
| compositor | how the layers become the canvas — the final z-order; `default_compositor` follows declaration order |

- **No declared layers:** draw calls go straight onto the canvas, as today. There is
  nothing to combine, and the compositor is not called.
- **Declared layers:** draw calls go only to layers; with none pushed, to the base layer
  (R31). The canvas belongs to the compositor: it is cleared at the start of the
  frame unless it is redrawn on demand, and the compositor draws the layers onto it.
- **The canvas is copied into the window as it is**, with the blend the scene's flags
  give: `none` for a scene with `hides_below` (a full-screen scene is a plain copy) and
  `blend` otherwise (an overlay shows what is below) (R27, R45). Fading a pause menu in is
  its own layers' business: `style_of(m_menu).opacity = t`.

Every layer is an ordinary layer, the base layer included: a custom compositor may draw
any of them in any order, twice, mirrored, or not at all.

An earlier version of this section (also 2026-11-10) made the bottom layer the result, to
save a texture. It needed four rules no other layer had — not a source, never kept between
frames under other layers, fixed at the output size, its settings meaning the scene's —
and a custom compositor could not put anything under it. The split costs a canvas texture
and one pass for scenes that declare layers (§5.6) and removes all four rules.

The game controls combining at two levels:

1. **Composite settings per layer** (§5.4.2), read by the default compositor (§5.4.3).
   Most games need only these: screen shake, fades, brix's board rotation and a night
   overlay are all settings.
2. **A compositor of the game's own** (§5.4.4), for what the settings cannot say: a layer
   drawn twice, an overlay between layers, a transition, split screen.

Either way each layer is scaled from its resolution to the result's. `screenshot()` draws
every visible scene again, so it sees what is shown (R46).

#### 5.4.1 Alpha and blend modes

Every layer target is ARGB8888 and is cleared at its first push in a frame (§5.3), so
wherever nothing was drawn the layers below show through. Alpha-blended drawing into a
cleared target leaves premultiplied colour in it (colour × alpha), so the compositor
combines each layer with the premultiplied form of its `blend`:

| Layer `blend` | Combined with | Cleared to | Use |
|---|---|---|---|
| `none` | `none` | transparent | a layer that covers the whole result: a plain copy, the cheapest. On the canvas it means the scene is opaque: a full-screen scene is copied into the window without blending |
| `blend` (default) | `blend_premultiplied` | transparent | normal layers |
| `add` | `add_premultiplied` | transparent | glow, light, fire |
| `mul` | `mul` | transparent | darkness, shadows. SDL's `mul` is `src × dst + dst × (1 − srcA)`, already right for premultiplied content; a transparent pixel leaves the picture below unchanged |
| `mod` | `mod`; `mul` below full opacity | opaque white | tinting everything below. `mod` is `src × dst` and ignores alpha, so a transparent pixel would turn the picture below black, and it cannot fade: below full opacity the layer is drawn with `mul`, which for its opaque pixels gives `dst × (1 − o + c × o)`, the tint faded by the opacity `o` (R43) |

- **Opacity scales colour too.** In the premultiplied modes SDL's alpha modulation scales
  only alpha (`SDL_blit_slow.c:200–213`), so the compositor modulates colour with
  `tint × opacity` and alpha with `opacity`. A `mod` layer below full opacity is drawn
  with `mul`, where the same modulation fades it.
- **`none` is opaque by contract.** Pixels a `none` layer does not cover come out black.
- **A layer is isolated** (§5.2): mul or mod draw calls inside a layer see only that layer.

#### 5.4.2 Layer style

Each layer has a `layer_style` (R45; was `layer_composite_settings`), set at declaration
(§5.1) and changed through `style_of(id)` at any time; the compositor reads it when it
runs.

```cpp
struct layer_style {
    blend_mode blend{blend_mode::blend};   // none, blend, add, mul, mod (§5.4.1)
    float      opacity{1.0f};
    color      tint{colors::white};        // fade to black, damage flash, night
    point      offset{};                   // screen shake
    layer_scale scale{1.0f};               // per axis; one float scales both
    float      rotation_degrees{0.0f};
    std::optional<point> pivot;            // centre of scale and rotation; unset: the middle
    flip_mode  flip{};                     // flip_mode.hh; diagonal: see R35
    std::optional<rect> source;            // only part of the layer
    std::optional<rect> destination;       // where it lands, in render units; unset: in place
    std::vector<mesh_effect> effects;      // ordered: rotate_3d, wave, perspective… (§6)
};

style_of(world).offset  = shake.offset();
style_of(hud).opacity   = fade.value();
style_of(board).effects = {rotate_3d{.yaw = angle}};   // brix (§7.1)
```

- **Flat transforms** — offset, scale, rotation about the pivot, flip, `source`,
  `destination` — draw the layer as one quad (RM1), placed as R43 says.
- **Effects** need a subdivided mesh (§6; RM4, RM5).
- **Animation** is changing the settings each frame on the scene's clock.

The field list was O9, resolved by R43.

#### 5.4.3 The default compositor

Every scene starts with `default_compositor`. It is a public function, not hidden
behaviour: a game's compositor can call it as a building block, and
`set_compositor(default_compositor)` puts it back.

```cpp
using compositor = std::function<void(composition&)>;

void default_compositor(composition& c) {
    for (const layer_id id : c.layers()) {    // declaration order
        if (c.has_content(id)) {              // drawn this frame, or on demand and not needing a redraw
            c.draw_layer(id);                 // onto the canvas, with its style
        }
    }
}
```

An every-frame layer not drawn this frame costs nothing. A scene without declared layers
draws onto its canvas directly, so its compositor is not called.

#### 5.4.4 A compositor of the game's own

`set_compositor(f)`, a `base_scene` member, replaces the scene's compositor;
`get_compositor()` returns the current one. `f` receives a `composition`:

| Call | Does |
|---|---|
| `layers()` | the declared layers, in declaration order |
| `has_content(id)` | drawn this frame, or on demand and not needing a redraw |
| `draw_layer(id)` | draws the layer onto the canvas with its style, as the default compositor does |
| `draw_layer(id, settings)` | draws it with other settings: a second copy, a part, another blend |
| `draw_layers(first, last)` | `draw_layer(id)` for each layer with content from `first` to `last`, in declaration order |
| `texture(id)` | a layer's texture, for `draw_texture`, `draw_mesh` or quads with source rects |
| `below()` | the canvases of the visible scenes below this one, bottom to top, read-only (§5.6) |

- **The free functions draw onto the canvas** while a compositor runs, so any producer can
  draw there: a vignette, a melt, a debug overlay.
- **Pushing a layer inside a compositor is an ENFORCE error.** The layers are finished.
- **A layer the compositor does not draw is not shown.** That gives debug views (only the
  collision layer) for nothing.

```cpp
// a water reflection: the world drawn a second time, mirrored and faded, under the HUD
set_compositor([&](composition& c) {
    c.draw_layers(sky, world);
    c.draw_layer(world, {.opacity = 0.3f, .offset = {0, water_y}, .flip = flip_mode::vertical});
    c.draw_layer(hud);
});
```

**Wrapping the current compositor.** A transition does not need to know how the game
composes its frame. It wraps whatever compositor is current, the default or the game's own,
and puts it back when it ends:

```cpp
// a Doom melt (§6.1): the new scene composed as usual, columns of the old scene's
// canvas (kept until the melt ends, §5.6) falling over it
const compositor scene = next.get_compositor();      // next: the scene coming in
next.set_compositor([scene, &melt](composition& c) {
    scene(c);
    melt.draw();
});
// when the melt ends:
next.set_compositor(scene);
```

The compositor belongs to the scene, with its layers (R18; was O10). Today's
`world_compositor` (the world's layer order) becomes declared layers in RM2 (§10), so the
name stops meaning two things.

### 5.5 One painter

Agreed (R37, 2026-11-17; replaces R5's implicit and explicit painters): there is one
painter, application-wide. Drawing is immediate and single-threaded and scenes render one
after another, so only one painter is ever in use. What differs between scenes — layers,
compositor, canvas, the state kept between frames — is the scene's context (§5.6), which
the painter uses while the scene is rendered.

Rendering into a texture is `texture::compose()` / `bind()`, as today:

| Use | How |
|---|---|
| baking (`texture::compose()`, KE's backdrop) | `compose()` once |
| brix board rotation | a layer with a `rotate_3d` effect, or a `compose()`d texture drawn through a mesh; no readback (§7.1) |
| a picture used many times or in several scenes | a texture made once with `compose()`; it can be drawn anywhere, any number of times, through any mesh |

A HUD needs no texture of its own: it is an on-demand layer of its scene. A scene
transition needs none either: the old scene's canvas is its last frame (§5.6).

```cpp
auto board = texture::create({256, 256}, texture_access::target);
board->compose([&] { draw_board(); });               // free functions draw into board
draw_mesh(*board, rotate_3d{.yaw = angle});          // on the scene's current layer
```

Earlier (R5, R7, O5) the game could create explicit painters, each with its own state,
layers, compositor and canvas, made current with a `painter_scope`. Nothing needed layers
inside a texture that `compose()` could not do, and per-painter state left `push_state`
and `push_layer` on separate stacks. If a game ever needs a layered texture, the painter
can hold a stack of scene contexts and a scope can push one.

### 5.6 Scenes own their layers

Agreed (R18, amended by R37): layers belong to the scene, and so does the compositor.
Each scene has a context: its layers, its compositor, its canvas, its painter state and
its save stack (R48).

**A frame.** `scenes_manager::render()` (`src/neutrino/scene/scenes_manager.cc:230`)
already walks the visible scenes bottom to top, from the topmost one with `hides_below`.
For each scene it:

1. gives the painter the scene's context and restores the state saved in it;
2. calls `render()`;
3. if the scene declared layers, runs its compositor, which combines them onto the canvas;
4. saves the painter's state into the context.

Then the scenes' canvases are drawn into the window in stack order, each as it is, with
the blend its flags give (§5.4, R45). The order of scenes is the z-order between scenes;
nothing else is needed.

**What this gives:**

- **Each scene has the layers it needs.** A menu needs a background and UI; gameplay needs
  sky, tiles, sprites, effects and HUD; a loading screen needs none.
- **Overlays come for free.** A pause menu is combined over the gameplay below it. Dimming
  the game is a translucent black rectangle drawn first by the pause scene. (A `mul` layer
  cannot do it: layers are combined onto the scene's own canvas, which is transparent, not
  onto the scenes below.)
- **Encapsulation.** Layer ids belong to their scene. Pushing another scene's layer is an
  ENFORCE error: the painter knows which scene is being rendered.
- **The compositor goes with the layers.** The scene that declared the water layer is the
  one that knows to draw the reflection.
- **Transitions need no capture** (was O6). The old scene's canvas is its last frame; the
  scene manager keeps it until the transition ends, and a wrapping compositor (§5.4.4) draws
  it with the new scene.
- **A scene with no declared layers** draws straight onto its canvas (R23). Existing
  scenes draw unchanged.
- **Simplex is not a special case.** `application_adapter::create_initial_scene()` already
  installs a `simplex_scene`: a scene with no layers whose `render()` copies the adapter's
  persistent picture, an opaque texture, onto the scene's canvas. The picture stays the
  adapter's, and simplex programs have no layers (R47); the simplex API does not change.

**Effects on the scenes below.** A scene's compositor cannot reach the layers of other
scenes; they are private. It can read their results: `below()` returns the canvases of the
visible scenes below, bottom to top (R22). A pause menu that wants the game blurred or
warped draws them itself, as its own bottom content, covering the plain copy already in the
window. Only that case pays for the extra pass.

**The texture allocator** (R20, amended 2026-11-15). Every layer target and every scene's
canvas is a `texture_handle` from one internal `texture_allocator`, application-scoped
(R38) and made in `on_ready()` next to the renderer ([`painter.md`](painter.md) §3.3). It
owns only those. Textures made with `texture::create`, `from_image` or `adopt` — baked
results, streaming textures, atlas pages — belong to whoever made them, are not pooled,
and are rebuilt by their owners after a reset (§8.3). Every texture, the allocator's
included, is made on the application's renderer; `adopt` rejects one made on another.

- **`acquire` takes a pooled texture of the same size or creates one, and always clears
  it**; the handle starts needing a redraw. A pooled texture's old pixels and a new one's
  undefined contents are never shown.
- **Destroying a handle gives the texture back to the pool.** A scene's handles die with
  its context when it leaves the stack; pushing and popping a pause menu allocates nothing
  after the first time. A transition keeps the old canvas by moving its handle out.
- **`needs_redraw()` / `mark_drawn()`.** A handle that needs a redraw holds only its clear
  colour. The allocator sets the flag (acquire, resize, resets); the painter calls
  `mark_drawn()` at the end of a frame on every handle drawn on in it. `needs_redraw(id)`
  reports the flag.
- **Resize destroys the old texture instead of pooling it, and destroys the pooled
  textures of the old render size**, so the pool does not keep a set of textures for every
  size the window has had.
- **Low memory trims the pool.** `SDL_EVENT_LOW_MEMORY` can arrive on any thread, so its
  handler only calls `request_trim()`; the allocator trims in `begin_frame()`, at the start
  of the next frame, on the main thread.
- **Resets** go through it: `on_targets_reset()` and `on_device_reset()` (§8.3).
- **Otherwise a pooled texture stays** until `acquire` reuses it, `trim_texture_pool()`
  empties the pool, the device is reset or the application ends. There is no age or budget
  policy (O15): the textures of one size never outnumber the most held at once, so only
  many different sizes make the pool grow.

**Cost.** Each visible scene's canvas is drawn into the window once per frame: negligible
on a GPU, about 0.2 ms on the software renderer at KE's 320×200 (§5.2). Usually one or two
scenes are visible. A full-screen scene's canvas (blend `none`) is a plain copy. A scene
that declares layers also pays a canvas texture and one combine per drawn layer.

Open: whether a paused scene may skip `render()` and reuse last frame's textures (O13).

### 5.7 The pinned scene

An FPS counter and a debug console outlive the scenes below them. Today the stack is
strict: only `m_stack.back()` gets live input, raw events and resize notices
(`scenes_manager.cc:201–227`, `handle_action`, `notify_top_resized`), and `push_scene` puts
the new scene on top — a scene meant to stay above everything would be covered by the next
push and would take input from the game.

Agreed 2026-11-14 (R28): one slot above the stack. An earlier version (2026-11-10) used a
`stays_on_top` flag; it needed an active scene and an input scene, a removal call with ids
and a ban on pausing, and was dropped for this.

```cpp
void pin_scene(std::unique_ptr<base_scene> scene);   // replaces the pinned scene, if any
void unpin_scene();                                  // exits and destroys it; none pinned: no-op
```

- **Both are queued** like `push_scene`. `pin_scene` gives a previously pinned scene
  `on_exit` and the new one `on_enter`; `unpin_scene` gives it `on_exit`.
- **`push_scene`, `pop_scene` and `replace_scene` never touch it**; they act on the stack
  below it.
- **Otherwise it is the topmost scene.** Rendering, updating, pausing and input treat it as
  the last scene of the stack, with all its flags: `hides_below` hides the whole stack,
  `pauses_below` pauses it, and its canvas's default blend follows `hides_below` (§5.4). An
  FPS counter overrides `flags()` like any overlay does today.
- **Resize notices** go to the top stack scene and to the pinned scene; a scene that becomes
  the top of the stack is sized even with a pinned scene above it.
- **With nothing below it**, `pop_scene` and `replace_scene` are ENFORCE errors, as they are
  on an empty stack today.
- **A failed `on_enter` rolls the pin back:** the scene gets `on_exit` and is dropped, a stack
  it paused is resumed, and nothing stays pinned.
- **At shutdown** `finish()` exits it before the stack and clears the slot.
- **One slot.** A console that also shows the FPS, or one debug scene hosting both.

**Pausing follows changes of state** (R29). Today `on_pause` goes to the scene directly
below a pushed `pauses_below` scene, and `on_resume` when that scene is popped. With a
pinned scene that pauses, scenes are pushed and popped underneath it, so "directly below"
is not stable and the pairs would break. Instead, after every change of the stack, of the
slot or of a scene's flags, the manager works out which scenes are updated: a scene gets
`on_pause` when it stops being updated and `on_resume` when it starts again. That is
balanced by construction, so `flags()` may change at any time; the manager reads it every
frame (today `base_scene.hh` requires the value to stay fixed while the scene is on the
stack). The cases of the existing tests (`test_scenes_manager.cc:314–411`) keep their
results. One case changes, as a fix: with game → busy spinner (no flags) → pause menu,
today only the spinner gets `on_pause` and the game stops updating unannounced; now both
get `on_pause`.

**`passes_input`** (R30), a new scene flag:

```cpp
enum class scene_flags : std::uint8_t {
    none         = 0,
    hides_below  = 1,
    pauses_below = 2,
    passes_input = 4   ///< sees input and events, and passes them on to the scene below
};
```

- Input and events go down from the top, the pinned scene first, and stop at the first
  scene without `passes_input`; the scenes passed on the way see them too. The scenes below
  that one get the neutral snapshot and no events, as covered scenes do today.
- It never reaches a scene paused by `pauses_below`: a paused scene gets no input and no
  events.
- The flags are read before an event is delivered, so a `handle_event` that changes them
  routes the next event, not the current one.
- It means "sees it and passes it on", not "gets nothing", so that a closed console sees
  its toggle key.

| Scene | Flags |
|---|---|
| FPS counter | `passes_input` |
| console, closed | `passes_input` |
| console, open | `pauses_below` (freezes the game) or `none` (the game runs on, as in Quake's multiplayer) |
| pause menu, busy spinner | as today |

**A Quake-style console** is one pinned scene:

```cpp
class console_scene : public neutrino::base_scene {
    bool  m_open  = false;
    float m_slide = 0.0f;                                // 0: hidden, 1: fully down
    layer_id m_panel{declare_layer()};                   // what slides (R45)

    scene_flags flags() const override {
        return m_open ? scene_flags::pauses_below : scene_flags::passes_input;
    }
    void handle_event(const event& e) override {
        if (is_key_down(e, key::grave)) { m_open = !m_open; return; }   // seen while closed too
        if (m_open) m_line.edit(e);                      // text, history, completion
    }
    void fixed_update(sim_duration dt, const input_snapshot&) override {
        m_slide = approach(m_slide, m_open ? 1.0f : 0.0f, dt.count() / 0.25f);   // always updated
    }
    void render() override {
        draw_fps();                                      // on the base layer: never slides
        if (m_slide == 0.0f) return;                     // the panel not pushed: not shown
        style_of(m_panel).offset = {0, int((m_slide - 1.0f) * m_height)};   // R45
        auto panel = scoped_layer(m_panel);
        set_draw_blend_mode(blend_mode::blend);
        draw_rect_fill({0, 0, render_size().width, m_height}, color{0, 0, 0, 200});
        draw_log_lines();
        draw_prompt();
    }
};

pin_scene(std::make_unique<console_scene>());            // once, at startup
```

The slide is the panel layer's `offset`; the translucency is the overlay's blend, from its
flags. What the console needs beyond the scene manager and the painter:

| Piece | Status |
|---|---|
| text input | sdlpp has an RAII `text_input_session` (`SDL_StartTextInput`); neutrino does not expose it. The console opens one while open; UI text fields need the same |
| drawing text | `draw_text` is FM3 in `font_plan.md` |
| log lines | failsafe's `logger::set_backend` lets the console collect lines; it has no getter, so the console's backend forwards to the cerr backend itself |
| commands | a registry (name → function, completion); not designed |

The scene manager change is independent of the painter (RM10).

**Implemented 2026-11-14** (`src/neutrino/scene/scenes_manager.cc`; tests in
`test/scene/test_scenes_manager.cc`, suites `passes_input` and `pinned scene`):
`pin_scene` / `unpin_scene` in `scene_transitions.hh`; `passes_input`; the rules above. The
pinned scene is the last element of `m_stack`, marked by a `bool m_pinned`, so rendering,
updating and the input chain treat it as the topmost scene unchanged; `active_count()` and
`active_top()` give the part of the stack push, pop and replace act on. Not yet
implemented: pausing by change of state (R29). Until it is, only `passes_input` may change
while a scene is on the stack (`base_scene.hh`), and a pinned scene with `pauses_below` can
still unbalance `on_pause` / `on_resume` when scenes are pushed or popped beneath it.

**The mouse cursor is not a scene**, and not a painter matter. SDL's hardware cursor is
drawn by the OS: no layer and no lag. sdlpp wraps `SDL_CreateColorCursor`
(`sdlpp/input/mouse.hh`), and SDL 3.4 adds `SDL_CreateAnimatedCursor`. The cursor is in
window pixels, so a pixel-art cursor must be scaled to the window. neutrino has no cursor
API yet (§10).

## 6. Meshes and effects

Rotations, warps and deformations are first-class. The model takes them from lib_sdlpp's
`simplex` (`include/simplex/mesh.hh`, `effects.hh`), which already implements them:

- **`sprite_mesh`** — a quad subdivided into `cols × rows` cells (`sdlpp::vertex`: position,
  colour, uv) laid out over a destination rect and a source region; `transform_vertices`
  deforms positions.
- **`mesh_effect`** — deformations applied to a laid-out mesh: `wave_horizontal` /
  `wave_vertical`, `skew_horizontal` / `skew_vertical`, `perspective` (trapezoid taper),
  `pinch_punch`, `rotate_3d` (pitch/yaw/roll, pivot, perspective factor).

Here a mesh is a **producer of textured geometry**: `draw_mesh(mesh, source, …)` draws it
on the current layer like any other geometry. The source is an `atlas_address` (resolved at
the call to page + placement) or a texture — a layer, a `compose()`d texture — so the
same effects work on one picture, on a layer when it is combined (§5.4) and on a whole
frame.

Notes for the port into neutrino:

- **Subdivision is required for correctness.** `SDL_RenderGeometry` interpolates uv
  affinely per triangle; a 1×1 quad under `rotate_3d` or `perspective` shows the
  diagonal-seam warp. A grid (e.g. 8×8) hides it. Plain 2-D rotation/scale/flip needs no
  subdivision — that stays a quad.
- **Draw through the painter.** `sprite_mesh::render(renderer, texture)` renders directly;
  in neutrino the mesh and effects are public and drawing goes to the current layer.
- **Resolve against the page.** `layout_rect(atlas, frame)` normalises uv against the
  whole texture; with multi-page atlases it must use the resolved page and placement.
- **Tint/alpha into vertices.** Vertex colour is hard-coded white in `layout_rect_impl`;
  fades and pulses need it.
- **Explicit effect order.** Effects deform positions sequentially and `layout_rect`
  resets them; "wave then rotate" differs from "rotate then wave". Make the order part of
  the API (an ordered effect list per mesh).
- **Timing.** `update(dt, speed)` puts an effect on the caller's clock — the same question
  the sprite plan answers for animations (the owning scene's time). One answer for both.
  Effects that run over many frames (a melt, a transition) keep their own state; the painter
  only draws their current geometry.
- **Name.** `sprite_mesh` → `mesh` (neutrino's sprites are animation playheads, not
  pictures).

### 6.1 Effects taxonomy

"Effect" is not a painter concept. Every effect found so far is one of:

| Kind | Example | Expressed as |
|---|---|---|
| primitive with animated parameters | brix laser, brix cursor | producer drawing geometry |
| deformed picture | waving flag, 3-D card flip | `draw_mesh` on an atlas picture |
| whole-layer transform / deformation | brix board rotation, underwater wobble, screen shake | the layer's style (§5.4.2), or a `compose()`d texture drawn through a mesh |
| part of a layer | heat haze over a fire, a ripple | a mesh over a sub-rect of the layer's texture, drawn by a compositor (§5.4.4) |
| transition over the finished picture | Doom melt, crossfade, wipe | the old scene's canvas, kept until the transition ends, drawn with the new scene by a wrapping compositor (§5.4.4, §5.6) |
| colour cycling | water, lava, a fade of indexed art | a palette shared by indexed atlas pages (§6.2) |
| many short-lived quads | particles, sparks (none in tree yet) | producer drawing quads |
| per-pixel shader effect | distortion, bloom, lighting | needs a GPU backend (§12) |

**The Doom melt** cuts the old frame into columns (2 px wide at 320) that start falling
after random, neighbour-correlated delays; each frame, each column is a quad from the old
frame's texture moved down. It cannot be one grid mesh, because neighbouring columns would
share vertices and shear instead of separating. It is N quads with a source rect each,
which is why a quad's source may be a texture and a rect (§3.2).

One effect over several layers — wobble every world layer but not the HUD — is open (O3).

### 6.2 Palette animation

SDL 3.4 added palettized textures. An `SDL_Palette` is attached with
`SDL_SetTexturePalette`, and one palette can be shared by any number of textures
(`SDL_render.h:1018`).

- **The lookup happens when the texture is drawn.** The GPU renderer does it in the shader
  (`TEXTURETYPE_PALETTE_NEAREST` / `_LINEAR` / `_PIXELART`, `gpu/SDL_render_gpu.c:72`), the
  software renderer in the blitter. Backends without native support convert the texture
  again when its palette changes (`UpdateTexturePalette`, `SDL_render.c:706`).
- **Draws before a palette change keep the old colours.** Changing the palette flushes the
  queued draws first (`FlushRenderCommandsIfPaletteNeeded`, `SDL_render.c:350`), so palette
  changes fit immediate drawing.

So colour cycling — water, lava, a fade of indexed art — is changing a few entries of a
palette shared by indexed atlas pages. Every picture on those pages changes, for the cost of
uploading 256 colours.

**Limits.**

- **Pictures, not layers.** Layer targets are RGBA (SDL forbids indexed render targets); a
  picture's colours are fixed when it is drawn into a layer. Layers redrawn every frame
  follow the palette; an on-demand layer must be redrawn when its palette changes.
- **Fades of the whole screen** — RGBA art and text included — are the layer's `tint` and
  `opacity` (§5.4.2), not the palette.

**What neutrino lacks today.**

- **The palette is baked in at upload.** `atlas_texture_format::indexed8` exists
  (`texture_registry.cc:20`), but pages are uploaded through `SDL_CreateTextureFromSurface`.
  That picks an RGBA texture whenever the surface has a colour key or a palette entry with
  alpha (`SDL_render.c:1832–1848`). Indexed art always has a transparent index, so the
  palette is always baked in.
- **The fix:**
  - create the page texture as INDEX8 explicitly (`SDL_CreateTexture` +
    `SDL_UpdateTexture`);
  - attach a palette whose transparent entry has alpha 0, instead of a colour key;
  - set the texture's blend mode to blend. SDL gives INDEX8 textures `none`, because the
    format has no alpha channel (`SDL_render.c:1537`).

  The software blitter honours palette alpha (`Blit1toNAlpha`, `SDL_blit_1.c:426`).
- **A palette handle for the game**, owned by an atlas or shared by several, with
  operations such as set an entry, cycle a range, fade. Its shape is open (O11).
- **Pixel tests on the software renderer:**
  - the same picture drawn before and after a palette change shows both colours;
  - the transparent index shows the layer below.

`demos/demoscene/vga_canvas` looks up its palette on the CPU every frame. It could upload
its 8-bit pixels to an INDEX8 streaming texture with an `SDL_Palette` and let SDL do the
lookup.

## 7. Case studies

### 7.1 brix (`~/proj/ares/brix/src/brix/game/view/`)

`view::draw_board` draws, in order, objects (atlas tiles) → lasers → cursor, into the
board's own render target (`game_board_painter::start_render` / `end_render`), which
`present()` composites onto the screen.

- **Laser** (`view::draw_laser`): five parallel 1-px lines, alpha `{128, 200, 255, 200,
  128}` pulsed by `beam_func(seq)`, axis-aligned only. Here: one geometry strip with
  per-vertex alpha (edge → centre → edge) — smoother, one call instead of five, and
  diagonal beams for free (RM3).
- **Cursor** (`game_board_painter::draw_cursor`): three rect outlines with phase-shifted
  pulsing alpha → three outline calls after the lasers.
- **Board rotation**: renders the board, `SDL_RenderReadPixels` → surface → new texture,
  then `rotor.cc`'s `rotate_texture` projects a 3-D-rotated quad. The board already is a
  target, so the readback is unnecessary. Here: the board is a layer with a `rotate_3d`
  effect, or a `compose()`d texture drawn through a `rotate_3d` mesh. No CPU round-trip
  (cf. card #19).

- **Plasma behind the options menu** (`scenes/level/options_scene.cc:139`,
  `draw_plasma`). The options scene is an overlay (`is_opaque() == false`) over the board.
  Each frame it computes a grey plasma value per 4×4-pixel cell of the board rect (four
  sines of distances, two of them moving with `time`) and draws the cell as a blended
  filled rectangle in `{c, c, c, max(200, c)}`, so the board shows faintly through; the
  dialog is drawn after it. A 640×480 board is 160×120 = 19,200 filled rectangles per frame,
  each in its own colour, through the raw SDL renderer.

  Here it works out of the box: the options scene has `pauses_below` and no `hides_below`,
  so its canvas is blended over the game (§5.4); the plasma is `set_draw_blend_mode(blend)`
  and `draw_rect_fill(cell, colour)` on the canvas, and call order puts the dialog on top —
  no layers. Blending onto the transparent canvas leaves premultiplied colour, and the
  canvas is combined with premultiplied blending, so the pixels equal drawing straight over
  the game (±1). Moving to neutrino changes two things: the raw `get_renderer()` calls
  become `draw.hh` calls (R1), and the function-local `static float time` advances in
  `fixed_update` on the scene's clock, since `render()` gets no time argument.

  What the model offers beyond that:

  | Option | How | Gain |
  |---|---|---|
  | a small streaming texture | compute the 160×120 values into a texture through `pixel_lock` (exists today), `draw_texture` it scaled ×4 with nearest filtering | one draw call and a 77 KB upload instead of 19,200 draws; on the software renderer one scaled copy instead of 19,200 fills |
  | a low-resolution layer | `declare_layer({.size = {w / 4, h / 4}, .style = {.destination = board}})`, one pixel per cell, stretched onto the board when combined (R43; `board` rounded down to whole cells) | a use case for per-layer resolution (O7); still one draw per cell, so the texture is better |
  | palette cycling (§6.2) | a classic plasma field computed once into an INDEX8 texture, the palette rotated each frame | no per-pixel work per frame; not identical — brix moves the field itself (`x + time`, `y + time / 7`), a palette cycle only makes the colours flow |
  | fading in | the overlay's layers with `style_of(id).opacity` — the whole overlay on one layer, or only the plasma on its own (R45) | — |
  | the paused game below | the board under the menu is paused, so its picture does not change, yet it is rendered every frame | the case for O13: reuse the game's last canvas; only the plasma and the dialog redraw |

  Only a per-pixel plasma — smooth, at full resolution — falls outside the model: it needs a
  fragment shader (§12).

Nothing in brix needs shaders or additive blending; the plasma would only want one at full
resolution.

### 7.2 KE and Luxor

- **Tiles** — `world_renderer` draws each tile with `draw_sprite`, one call per tile; they
  draw on the world layers, and SDL merges the same-page runs.
- **Sprites in world layers** — `sprite_batch` keeps sorting by `(band, depth, order)` and
  draws its sorted entries on the current layer when flushed (§3.4).
- **Screen-space UI** (KE) — a screen-space layer declared last.
- **Backdrops** — `texture_layer` draws on the bottom layer; KE's baked backdrop is
  `compose()`d, as today.
- `world_compositor`'s slots become declared layers; where the camera transform is applied
  is O4.

### 7.3 Call sites that depend on immediacy

Drawing stays immediate, so the audit of the deferred draft shrinks to readback and raw SDL:

| Site | What it does | Here |
|---|---|---|
| `texture::to_image()` (`src/neutrino/video/texture.cc`), `screenshot()` | GPU → CPU readback | `to_image()` unchanged; `screenshot()` replaces `capture_frame()` (R46) |
| `texture::compose()` / `bind()` (KE `assets/backdrop.cc`, tutorial `04_render_textures`) | bake into a target | unchanged (R37: no explicit painters) |
| simplex adapter (`application_adapter.cc`, `canvas.bind()`) | persistent canvas target | unchanged (R47) |
| brix `view.cc:177` `SDL_RenderReadPixels`, `game_board_painter.cc` `SDL_SetRenderTarget` | raw SDL, outside neutrino | a layer or a `compose()`d texture, and a mesh (§7.1) |

No game in this repository calls the SDL renderer directly for drawing.

## 8. Atlas lifetime and device reset

### 8.1 Default: atlases live for the application

The use case is *load, run, exit*. No current user releases an atlas mid-run (KE's
`unregister_*` calls release sprite *states*; warlords's `texture_cache::invalidate` /
`clear` are never called). So:

- `register_atlas` is **internal**: loaders, `tileset_bundle`, `atlas_cache` and the text
  system call it. Games see only `atlas_id` / `atlas_address`.
- Registered atlases stay until the application shuts down; the registry is cleared as a
  whole then (and between test cases).
- No handles, no refcounts, no public `unregister_atlas`.
- `atlas_id`s come from a monotonic counter and are never reused (`atlas_id_gen.cc`), so
  drawing from an atlas that no longer exists is an ENFORCE at the call site, never a
  dangling pointer.

### 8.2 Later: replacement and group release

Two mid-run cases are realistic; both are deferred until a user needs them, and neither
changes the drawing API:

| Need | Cases | Mechanism |
|---|---|---|
| **replace in place** | glyph atlases re-rasterised for a new size (DPI change, UI scale, window resize with scaled text); graphics settings switching the art set (classic / upscaled); hot reload (`atlas_cache::reload`) | rebuild the pages under the **same** `atlas_id`; addresses stay valid as long as picture indices keep their meaning (or the producer, e.g. text, re-resolves them) |
| **group release** | content generated per game session (warlords player-colour armies, per-scenario minimap / terrain) in a "new game" loop; long-running tools (map viewer, editor) opening map after map; large HD art on memory-limited platforms | atlases registered into a group; an internal `release(group)` at a point where nothing uses it (between games, on scene switch); stale use is caught by §8.1's never-reused ids |

Drawing is immediate, so nothing on our side outlives a draw call and no deferred-release
machinery is needed: SDL flushes its own queue before a texture in use is destroyed (§3.1).

### 8.3 Device reset

SDL reports three render events (`SDL_events.h`):

| Event | Meaning | Painter response |
|---|---|---|
| `SDL_EVENT_RENDER_TARGETS_RESET` | render targets lost their *contents* | the texture allocator's `on_targets_reset()`: every layer and canvas cleared and marked for redraw, so on-demand layers and on-demand canvases report it; layers drawn every frame are redrawn anyway; baked targets (`compose()`): owners must re-bake |
| `SDL_EVENT_RENDER_DEVICE_RESET` | **all textures must be recreated** | re-upload every registered atlas; the texture allocator's `on_device_reset()` (§5.6): the pool destroyed, every layer and canvas texture recreated in place, cleared and marked for redraw; owners of game textures must re-create / re-bake |
| `SDL_EVENT_RENDER_DEVICE_LOST` | device gone, unrecoverable | report and shut down cleanly |

Device reset happens with D3D device loss and can happen on mobile (warlords has an
Android build); neutrino handles none of these events today.

Consequences:

- **The registry keeps each atlas's CPU pages after upload**, so re-upload needs no
  loader. 2-D atlases at this scale are small; the same copy serves CPU-side reads
  (warlords's minimap, picture pixel queries, `make_bitmask`). An atlas may opt out with a
  rebuild function instead (large art), decided when one does.
- **The painter records nothing, so it cannot redraw lost contents itself** (R13,
  proposed). An on-demand layer that needs a redraw is drawn by its owner on the next
  frame: `if (changed || needs_redraw(hud))`.
- **Game-owned textures** (baked `compose()` results, streaming textures) cannot be
  rebuilt by the engine: the engine raises a scene-level notification (e.g.
  `on_textures_lost()`) and the owner re-bakes. Streaming textures refilled every frame
  need nothing.
- **Glyph atlases** are the exception to keeping CPU pages: they are rebuilt from their
  glyph source (§9.5).
- Test: a forced device-reset path (re-upload everything, then pixel-compare a frame).

## 9. Text

Text is a **producer**: layout turns a string into glyph quads, drawn like any other
quad.

### 9.1 What the games do today

| | brix | warlords | KE |
|---|---|---|---|
| where | `video/video_system.cc`, `video_assets.hh` | `bane/ui/font_service.cc`, `bane/ui/nuklear/backend/nk_onyx_font.cc` | `scenes/play_game_scene.cc`, `ke_collage.cc` |
| faces | one tileset per `font_t`; the 6 colour variants are separate fonts, coloured **per row by a palette ramp** | game faces (body / title / sign / heading) are **proportional** `onyx_font::bitmap_font`s from the original game data; mono = BIOS 8×16 | onyx `bios_font_8x8` |
| atlas | `texture_atlas` per font, glyph rects from `' '` | `onyx_font_binding` bakes the 256-glyph range once into a **white RGBA** atlas (1-px gutter), `scale_mode::nearest` | none: glyphs drawn from the font's bits |
| glyph lookup | lowercase folded to uppercase, `rects[ch - ' ']` | 256-entry table | 256-entry table |
| advance | rect width, no kerning | `a_space` + `advance` per glyph | fixed cell |
| colour | baked variants | `set_color_mod` / `set_alpha_mod` per draw | — |
| scaling | 1:1 | design pixels × integer UI scale, nearest | 1:1 |
| consumer | `SDL_RenderTexture` per glyph | **Nuklear** (`nk_user_font`) | its own blits |
| lifetime | load → exit | game faces **dropped and rebuilt on `on_assets_changed()`** (switching game data sets) | load → exit |

Two more warlords paths: `bios_text` draws every lit BIOS pixel as a filled rect (one
`SDL_FRect` per pixel), and `intro_script_decoder` rasterises text into a CPU
`rgba_buffer` — text into an image, no GPU.

Scalable (TTF / vector) fonts have no user yet; onyx_font supports them through
`glyph_cache` (rasterises on demand into alpha atlas surfaces, adds a page when one is
full) and `text_renderer` (`draw`, `draw_aligned`, `draw_wrapped`, `measure`, through a
`blit(atlas, src, x, y)` callback that maps directly onto drawing a quad).

[SDL_FontCache](https://github.com/grimfang4/SDL_FontCache) (C, SDL2, SDL_ttf) was read for
comparison; what it contributes is noted below as *(FC)*.

### 9.2 One model: font = glyph source + glyph atlas + layout rules

Every case above is the same model with different parameters:

| Aspect | brix | warlords | KE / BIOS | TTF |
|---|---|---|---|---|
| glyph source | tileset pictures | onyx `bitmap_font` (game data) | onyx BIOS font | onyx `glyph_cache` |
| atlas | static | static, **replaced** on asset change | static | **dynamic**, grows |
| codepoint → glyph | `ch - ' '`, case folding | 256-range lookup | 256-range lookup | rasterise on first use |
| advance | rect width | `a_space` + `advance` | fixed cell | bearings + advance (+ kerning) |
| colour | baked variants | tint | tint | tint |
| scaling | 1:1 | integer, nearest | integer, nearest | rasterised at size, linear |
| consumer | painter | Nuklear (postponed, §9.3) | painter | painter |

- **Glyph source** — owns the codepoint → glyph mapping (including brix's offset and case
  folding), the glyph pixels and the per-glyph metrics. onyx_font's `bitmap_font` and
  `glyph_cache` are sources; so is a tileset with a mapping rule.
- **Glyph atlas** — the glyphs' pictures in an atlas, addressed like any other picture.
  Static for bitmap fonts (whole range baked at load), dynamic for TTF.
- **Layout rules** — advance, bearings, kerning, line height, letter / line spacing, tab
  width; onyx_font's `text_renderer` does the layout.
- The font abstraction **must not require a GPU atlas**: warlords's intro rasterises text
  into a CPU image straight from the glyph source.

### 9.3 The font facade over onyx_font

Games never use onyx_font directly — the same arrangement as onyx_image
(`images_atlases_sprites.md` §1, §3.2): neutrino owns the vocabulary, onyx types appear only
as a plugin point, and the output flows into the engine pipeline.

```
                     font bytes / built-in / game glyphs
                                   │
  ┌────────────────────────────────┼─────────────────────────────────┐
  │ standard containers            │ built-in                        │ game formats
  │ TTF/OTF/TTC, FNT, FON, BGI, GEM│ bios_font(8x8 | 8x14 | 8x16)    │ glyph bank: image_bank with
  │ load_font(path|bytes, options) │                                 │ ids = glyph numbers
  │ (onyx font_factory / registry) │                                 │ + glyph_metrics + mapping
  └────────────────────────────────┴───────────────┬─────────────────┘
                                                   ▼
                       neutrino::font   (glyph source + glyph atlas + layout)
                     ┌─────────────────────────────┴─────────────────────────────┐
                     ▼                                                           ▼
     painter: draw_text(…), measure / wrap / caret           CPU: render_text(image&, …)
                                                             (warlords intro)
```

Public (`include/neutrino/video/text/`: `font.hh`, `font_loader.hh`, `text_layout.hh`):

```cpp
class font;                                    // value handle over shared data (like animation, D16)
struct font_options {
    float        pixel_size{0};                // 0 = native (bitmap faces)
    std::size_t  face_index{0};                // FON / TTC hold several faces
    font_style   style{};                      // onyx text_style: bold, italic, underline, strikethrough
    atlas_filter filter{atlas_filter::automatic};   // nearest for bitmap, linear for scalable
    std::u32string_view warm_up{};             // glyphs rasterised up front (§9.5)
};
font load_font(const std::filesystem::path&, const font_options& = {});
font load_font(std::span<const std::uint8_t>, const font_options& = {});
std::vector<font_face_info> list_faces(std::span<const std::uint8_t>);
font bios_font(bios_size);                     // 8x8, 8x14, 8x16
font font::from_glyphs(image_bank glyphs, glyph_metrics metrics, glyph_mapping mapping);   // F3, general path
font font::from_bitmap(const onyx_font::bitmap_font&);    // plugin type, re-exported (F3)

struct glyph_metrics {                         // the glyph bank's side table (F6)
    struct glyph {
        std::uint32_t id;                      // image_bank entry id
        std::int16_t  advance;                 // pen step
        std::int16_t  bearing_x{0};            // pen → left edge (onyx's a-space)
        std::int16_t  y_offset{0};             // cell top → glyph top (Westwood v3)
    };
    std::vector<glyph> glyphs;
    std::int16_t  line_height;
    std::int16_t  baseline;                    // cell top → baseline (Dynamix)
    std::int16_t  space_advance;               // when there is no space glyph (Jazz)
    std::int16_t  letter_spacing{0};           // implicit gap the game adds (RA2: +1)
    std::uint32_t default_glyph;               // for unmapped code points
};
using glyph_mapping = std::function<std::optional<std::uint32_t>(char32_t)>;  // code point → glyph id,
                                               // many-to-one allowed; or built from a table
font font::at_size(float pixel_size) const;    // another font sharing the face data (F1)

// layout and drawing (§9.6)
text_params, text_align, draw_text(…), measure(…), wrap(…), offset_of(…), index_at(…)
void render_text(image& target, point, std::string_view, const font&, const text_params&);

// plugin point only
void register_font_decoder(…);                 // onyx font_registry
```

Hidden inside neutrino: `font_source`, `glyph_cache<Surface>` and the `atlas_surface`
concept, `text_renderer`, raster targets, FreeType. No game includes `onyx_font/…`.

Decisions (agreed 2026-11-04):

- **F1 — face × size.** A `font` is one face at one pixel size. Bitmap faces have their
  native size and only scale by integers (T3). `at_size(px)` gives another `font` sharing
  the face data; each size has its own glyph atlas.
- **F2 — character sets.** Text is UTF-8. **onyx bitmap faces** are 8-bit (`uint8_t`
  glyph indices with a `charset`, CP437 for BIOS): the font maps UTF-8 → that charset and
  falls back to the face's default character. **Glyph-bank fonts** are not limited to 8
  bits: their `glyph_mapping` takes any Unicode code point to a glyph id, many-to-one
  (Westwood's RA2 Unicode BitFont maps 65,536 code points onto shared images). brix's case
  folding and `' '` offset, and Jazz Jackrabbit's table (0–25 = A–Z, unsupported → 126),
  are mappings.
- **F3 — game formats: the glyph bank is the general path.** A glyph-bank decoder is an
  `image_bank_decoder` (glyph numbers as ids, the images doc's rules: native format, INDEX8
  with a colour key for transparency) returning `glyph_metrics` and a mapping as side tables,
  like pivots. It covers every bitmap format, including colour and Unicode ones. An onyx
  `bitmap_font`, re-exported as a plugin type (the equivalent of `image_decoder`), is the
  shortcut for formats onyx already models (1-bit, ≤ 256 glyphs, ABC spacing, no per-glyph
  Y offset). warlords already produces `bitmap_font`s (and serialises them in
  `citadel/s11n/onyx_font`).
- **F4 — coloured glyph fonts.** A glyph bank with colour pixels (brix, Westwood v3/v4,
  Dynamix, Jazz Jackrabbit) makes a coloured font: `tint` multiplies, white leaves it
  unchanged. Colour variants (Westwood-style text colours are palette remaps) are separate
  fonts produced with `with_palette`, like any recoloured atlas; fonts are small, so a dozen
  variants is cheap.
- **F5 — location.** `include/neutrino/video/text/`, next to `video/image/`.
- **F6 — glyph metrics.** Per glyph: advance, x bearing, Y offset. Per font: line height,
  baseline, space advance (for formats without a space glyph), letter spacing, default
  glyph. This is the superset the formats in §9.3.1 need; onyx `bitmap_font` metrics map
  onto it.
- **F7 — large static fonts.** A Unicode bitmap font may hold thousands of glyphs (Japanese
  builds); it is still a static atlas, just multi-page. A lazily built variant only if
  size becomes a problem.
- **F8 — decoders live by analogy with images.** onyx_font keeps the formats it models
  (TTF/OTF/TTC, FNT, FON, GEM, BGI). Every other format is a glyph-bank decoder in the
  game or a formats add-on. Nothing depends on onyx_font growing an indexed-colour glyph
  type; if it does, decoders can move into it.
- **Postponed — Nuklear.** An engine-provided `nk_user_font` binding and Nuklear as a
  painter producer (RM8) wait until a Nuklear user moves onto neutrino.

#### 9.3.1 Evidence: the classic game font formats

The [ModdingWiki font category](https://moddingwiki.shikadi.net/wiki/Category:All_font_formats)
(25 formats) was checked against the facade; the format pages that stress it most were read
(Westwood v3 and Unicode BitFont, Jazz Jackrabbit, Dynamix v6).

| Group | Formats | What is special | Path |
|---|---|---|---|
| 1-bit bitmap, ≤ 256 glyphs | Microsoft Bitmap (FNT/FON), GEM, Westwood BitFont, most small game fonts | monochrome, proportional | onyx `bitmap_font` (F3 shortcut); white atlas + tint |
| vector strokes | BGI, Microsoft Vector | scalable line strokes | onyx `vector_font`, rasterised at size (scalable path); drawing strokes as line geometry is a possible later option |
| indexed colour bitmaps | Westwood v3 (4-bit), v4 (8-bit), Dynamix v1–v6 (v6 8-bit), Jazz Jackrabbit (8-bit; transparent index 0 or 254) | colour pixels; per-glyph Y offset (Westwood); baseline (Dynamix); space width only in the header (Jazz); RLE / interleaved storage; glyph data shared between codes; unreliable glyph counts | glyph bank (F3) → coloured font (F4) |
| Unicode bitmap | Westwood Unicode BitFont (RA2) | 65,536-entry code-point table, many-to-one; 1-bit; implicit +1 px spacing | glyph bank with a Unicode mapping (F2) |

Storage quirks (compression, interleaving, shared glyph data, unreliable counts) are the
decoder's business; they never reach the facade.

### 9.4 Colour

Two mechanisms, both already in the model:

- **Tint** for white glyphs (warlords, BIOS, TTF): glyphs are uploaded as white RGBA with
  coverage in alpha (SDL_Renderer has no portable A8 texture format) and the colour is the
  quad's `tint` (§3.2). One glyph atlas serves every colour. *(FC: same — glyphs rendered
  white, coloured at draw time; but FC sets the colour on the shared page texture, a state
  leak its own comments call a bug. Per-call tint avoids it.)*
- **Baked variants** for authored colour that tint cannot express (brix's per-row palette
  gradients): a variant is another picture range in the same atlas, or another atlas. The
  font variant selects it.

### 9.5 Atlas requirements

Beyond §8.1:

- **Static glyph atlases** are ordinary atlases built at load.
- **Growing atlases** (TTF): a new glyph is a new picture, written into the GPU page with a
  **sub-rect texture upload** (`SDL_UpdateTexture` on a static texture); a full page adds a
  page; existing addresses never move. *(FC adds glyphs by drawing them onto pages that are
  render targets, which forces it to save and restore clip / viewport / scale / logical size
  around every glyph and exposes the pages to `TARGETS_RESET`. Not borrowed.)*
- **Pre-packed pages.** onyx_font packs its own pages; the atlas accepts externally packed
  pages + rects as pictures (T1).
- **Per-atlas filter mode.** Pixel fonts need nearest, TTF rasterised at size wants linear
  *(FC: filter per font)*. This is also the atlas-filtering question left open by card #19:
  a filter mode on the atlas, not a global hint.
- **Replacement in place** (§8.2), first concrete user: warlords rebuilding its game faces
  on `on_assets_changed()`. Also TTF at a new pixel size (DPI, UI scale) and
  `glyph_cache::clear()` after a style change.
- **Device reset: rebuild from the source, no CPU copy.** Every glyph atlas can be
  regenerated from its glyph source — bitmap fonts rebake their (small) range eagerly, TTF
  clears its cache and re-rasterises lazily, eagerly only for its warm-up set. So glyph
  atlases are the exception to §8.3's "keep CPU pages". *(FC: `FC_ResetFontFromRendererReset`
  re-rasterises from the TTF source.)*
- **Warm-up set.** A font is created with an explicit set of glyphs to rasterise up front —
  ASCII, or every glyph used by the game's localised strings — so the first frame showing
  new text does not hitch, and the eager part of a device-reset rebuild is known.
  *(FC: the "loading string"; onyx's `pre_cache_ascii` is the fixed version.)* Bitmap fonts
  bake their whole range, so the set only matters for TTF.

### 9.6 Drawing text

```cpp
struct text_params {                  // (FC: FC_Effect — alignment, scale, colour)
    color      colour{colors::white}; // tint; ignored by baked-colour variants
    text_align align{text_align::left};
    float      scale{1.0f};           // integer for pixel fonts (T3)
    float      letter_spacing{0}, line_spacing{0};
};
rect draw_text(point position, std::string_view, const font&, const text_params& = {});
rect draw_text(const rect& box, std::string_view, const font&, const text_params& = {});   // wrapped
```

- One quad per glyph through the layout callback, drawn immediately on the current layer
  (§3.1); a glyph rasterised during layout is registered before it is drawn.
- **Returns the bounds** of what was drawn — for UI layout, hit areas and dirty rects
  *(FC: every draw returns its rect)*.
- **Measurement and caret helpers** on `font`: `measure`, `wrap`, and index ↔ position
  (`offset_of(text, index)`, `index_at(text, point)`) for text input. onyx's layout has
  measure and wrap; the caret pair is new *(FC: `FC_GetCharacterOffset`,
  `FC_GetPositionFromOffset`)*.
- **No printf-style formatting** in the API *(FC has it on every call)*; callers use
  `std::format`.
- **Nuklear (postponed).** Nuklear's output is already a vertex / index list with a texture
  per command (`nk_convert`), so when it comes, a Nuklear backend is a producer drawing
  `draw_geometry` with atlas addresses, glyph atlases included (RM8).
- **BIOS pixel-rect text** (warlords `bios_text`) becomes glyph quads from a static BIOS
  atlas.

### 9.7 Open decisions

- **T1 — who packs TTF glyphs.** onyx_font's packer (pages handed to the atlas as-is) or the
  atlas packer (onyx_font rasterises single glyphs). Proposed: onyx_font packs; the atlas
  learns "pre-packed pages + rects".
- **T2 — `font`.** Settled by the facade (§9.3, F1–F8).
- **T3 — pixel fonts and snapping.** Bitmap fonts stay pixel-exact: nearest filtering,
  integer scale, positions snapped like every quad. TTF is positioned at sub-pixel accuracy
  only if a game asks.
- **T4 — kerning.** onyx's advances and bearings are kept; kerning pairs for TTF when a
  game needs them *(FC has none)*.

## 10. What exists, what is missing

| Model piece | In the tree today | Gap |
|---|---|---|
| atlas + `atlas_address` | `image_atlas`, `texture_registry` (M0, `00c2639`) | — |
| immediate drawing | `draw.hh` / `draw.cc`: quads (snapping, pivot, flip, diagonal, rotation), primitives, `render_geometry` internally; `texture_draw_params` has a tint for whole-texture draws | colour and blend are global SDL_Renderer state; sprite draws have no tint, crop or fit (`sprite_draw_params`) |
| painter, layers, compositor | — | §3–§5 (RM1) |
| sorted sprites | `sprite_batch` (`draw_layer` band, `depth`, stable; screen-space and camera constructors) | stays; draws on the current layer when flushed (§3.4) |
| world layer order | `world_compositor`, `render_layer`, `layer_view` | becomes declared layers (RM2) |
| render targets | `texture` (`target` access), `bind()` / `compose()`, `renderer_target_scope` | not tied to layers; how the game renders into a texture (R37) |
| frame boundary | `application::on_render`: clear → `render_scene()` → present | each scene's context in use during its `render()`, its compositor after it, canvases into the window (RM1) |
| scene stack | `scenes_manager`: one stack; `hides_below`, `pauses_below`, `passes_input`; the pinned scene (`pin_scene` / `unpin_scene`); `on_pause` only to the scene directly below | a context per scene (RM1); pausing by change of state (§5.7, RM10) |
| mouse cursor | — (sdlpp wraps `SDL_CreateColorCursor`) | a cursor API over SDL's hardware cursor, scaled to the window (§5.7) |
| blend modes | `blend_mode`: neutrino's own enum (none, blend, add, mul, mod, `blend_premultiplied`, `add_premultiplied`), mapped to SDL's in `video/blend_mode_sdl.hh` (R34) | painter state; layer blend when combined |
| meshes + effects | lib_sdlpp `simplex::sprite_mesh`, `mesh_effect`s | not in neutrino; renders directly; uv vs whole texture; white vertices |
| raw renderer access | `get_renderer()` public (`application.hh`, `draw.hh`, `globals.hh`); used outside `src/` only by four test files | hidden, no escape hatch (R1, RM2) |
| painter state | colour and blend are global SDL_Renderer state, the line style global state of sdlpp's renderer wrapper; no clip rect in `draw.hh`; `blend_mode` has only the presets | `painter_state` per painter with clip (R25); `push_state` (R24); `custom_blend` (R26) |
| atlas lifetime | `register_atlas` / `unregister_atlas` public; CPU pages dropped after upload | internal registration; keep CPU pages (§8) |
| device reset | — (no handler for the three render events) | re-upload, redraw flags, notify owners (§8.3) |
| palette animation | `atlas_texture_format::indexed8`; pages uploaded through `SDL_CreateTextureFromSurface`, which bakes the palette in; `vga_canvas` looks colours up on the CPU | INDEX8 page textures with a live, shared palette; a palette handle (§6.2, RM9) |
| text | `font_id` registry, glyph atlases, `glyph_mapping`, layout (`font_plan.md` FM1–FM2, 2026-11-09); games still draw their own | `draw_text` (FM3); scalable faces (FM4); device reset (FM6) |

## 11. Decisions

The 2026-11-04 draft's R1–R7 (paint list per layer or per frame, geometry arena, quad
matrix, white texel, composite, names, `set_draw_color`) and the recorded "RM1a" proposed
earlier on 2026-11-09 are superseded by the immediate model. The numbers are reused below.

**Agreed (2026-11-09):**

- **R1 — One interface.** The painter is the only way to draw in neutrino. The world,
  simplex, the demos, the tutorials and the games draw through it; `get_renderer()` becomes
  hidden, with no escape hatch (amended 2026-11-10) (§2).
- **R2 — Immediate drawing, no recording.** A draw call renders into the current layer's
  texture when it is called (§3.1).
- **R3 — Declared layers, each a texture.** Each scene declares its layers ahead of time
  (R18); each is always a render target texture; layers are combined at the end of the
  frame, by default in declaration order (§5.1, §5.2, §5.4).
- **R4 — Painter state with `push_layer` / `pop_layer`.** Draw calls draw on the current
  layer, with the state's colour and blend (§3.3, §5.3). Since R39 a layer push does not
  save or restore the state.
- **R5** — superseded by R37 (one painter; no explicit painters). It had the free
  functions draw with the implicit painter and let the game create explicit painters whose
  result is a texture (§5.5).
- **R6 — Effects** on a layer, on part of a layer and on the finished picture (brix board
  rotation, Doom melt) (§6).

**Agreed (2026-11-10):**

- **R14 — The compositor.** A scene's layers are combined by calling its compositor.
  Every scene starts with `default_compositor` (each layer with content, in declaration
  order, with its style); `set_compositor` replaces it with the game's own
  (§5.4.3, §5.4.4).
- **R18 — Scenes own their layers and their compositor** (resolves O1, O10; amended by
  R37). Each scene has a context — layers, compositor, canvas, saved state — that the one
  painter uses while the scene is rendered; the scene manager runs its compositor after
  `render()` and draws the scenes' canvases into the window in stack order (§5.6).
- **R19** — superseded by R23. It made the bottom layer the result, which needed four
  special rules (§5.4).
- **R20 — The texture allocator** (amended 2026-11-15; was "the dead layers pool"). One
  internal `texture_allocator`, owned by the application, hands out every layer target and
  scene canvas as a move-only `texture_handle`, and keeps
  the textures given back for reuse by size. It owns only those: textures from the public
  `texture::create` / `from_image` / `adopt` stay with their owners. `acquire` always clears; the handle carries
  `needs_redraw()`; a resize destroys the old texture instead of pooling it, and the pooled
  textures of the old render size; `SDL_EVENT_LOW_MEMORY` trims the pool at the start of
  the next frame; the render resets go through `on_targets_reset()` /
  `on_device_reset()`. `texture_pool_stats()` / `trim_texture_pool()` are the public
  surface (§5.6).
- **R21** — superseded by R28 (top-level scenes through a `stays_on_top` flag). The mouse
  cursor is not a scene: SDL's hardware cursor (§5.7).
- **R22 — `below()`.** A compositor reads the canvases of the scenes below, never their
  layers; the old scene's canvas serves transitions (resolves O6) (§5.6).
- **R23 — The canvas is the result, not a layer.** `declare_layer` says which layers exist
  and their default order; `push_layer` / `pop_layer` say where drawing goes; the
  compositor decides the final z-order. Without declared layers draw calls go onto the
  canvas; with layers they go only to layers (§5.4). Amended 2026-11-16 by R31: drawing
  with none pushed goes to the base layer; it was an ENFORCE error, with no default layer.
- **R24 — `push_state` / `pop_state`** (resolves O2) (§3.3).
- **R25 — A clip rect in the painter state** (§3.7).
- **R26 — Custom blend modes** through `SDL_ComposeCustomBlendMode` (§3.7), encoded by
  neutrino (R34).
- **R27 — The public contract.** Code that draws today works unchanged with the defaults;
  every setting is public, with today's behaviour as its default. The canvas's default
  blend follows `hides_below` (§3.7, §5.4).

**Agreed (2026-11-14):**

- **R28 — The pinned scene** (implemented 2026-11-14). `pin_scene` / `unpin_scene`: one slot above the stack that
  `push_scene`, `pop_scene` and `replace_scene` never touch; otherwise the topmost scene,
  with all its flags (§5.7).
- **R29 — Pausing follows changes of state.** `on_pause` when a scene stops being updated,
  `on_resume` when it starts again; `flags()` may change at any time (§5.7).
- **R30 — `passes_input`** (implemented 2026-11-14). A scene with it sees input and events and passes them on to
  the scene below; input stops at the first scene without it (§5.7).

**Agreed (2026-11-16):**

- **R31 — The base layer** (amends R23; amended by R45: internal, no `base_layer()`, drawn
  as it is). The first `declare_layer` also makes a base
  layer, the bottom of the declaration order; drawing with no layer pushed goes to it.
  `base_layer()` gives its id. It is an ordinary every-frame layer at the painter's
  size, pushed every frame, so a compositor can move, fade or skip it like any other.
  Declaring only adds a layer, so a scene adopts layers one at a time. Considered: R23 as
  it was (drawing with none pushed an error: adopting layers is all or nothing), and such
  draws going onto the canvas under the layers (free, but the compositor cannot move that
  content). Not R19 again: the canvas stays the result, and the base layer has no special
  rules. Cost: one clear and one combine a frame per scene with layers.
- **R32 — The layer stack** (amended by R37: the push stack moved to the painter). One
  scene's layers live in an internal `layer_stack` ([`painter.md`](painter.md) §3.4):
  settings, targets and the frame's flags. A layer takes its target at its first push, not
  when its scene is first drawn; an every-frame layer gives it back after a frame that did
  not push it; a scene's layers are resized at its next frame, so scenes below the top
  follow the render size; a `layer_id` carries its stack's serial, so another scene's
  layer is rejected; `pop_layer` checks that the layer's target is still bound. RM1's
  order changes with it: layers before canvases and painter state (§14).
- **R33 — `redraw_mode` replaces `retained`** (the canvas's redraw mode dropped by R45).
  `layer_settings::redraw` is `redraw_mode::every_frame` (default) or
  `redraw_mode::on_demand`; `set_canvas_retained(bool)` becomes
  `set_canvas_redraw(redraw_mode)`. The meaning is unchanged (§5.1). *Why:* "retained"
  reads as retained-mode drawing — a recorded scene replayed every frame — which the
  painter is not (R2), and the sprite plan already uses "retained-mode" in that sense; an
  enum also has room for another policy, such as clearing once. Considered: `persistent`,
  `preserved` (WebGL's `preserveDrawingBuffer`), `keep_contents`, an inverted
  `auto_clear`, `cached`.
- **R34 — `blend_mode` is neutrino's own** (implemented 2026-11-16). An
  `enum class blend_mode : std::uint32_t` in `video/blend_mode.hh` with neutrino's values
  (none, blend, add, mul, mod, `blend_premultiplied`, `add_premultiplied`) replaces the
  alias of `sdlpp::blend_mode`; `video/blend_mode_sdl.hh` maps it to SDL's and back, the
  only code that knows both. A value outside the enum maps to SDL's invalid mode, so the
  draw fails and is counted. *Why:* layer composites and draw calls share one type, the
  public headers need no sdlpp, and nothing depends on SDL's numbers — which look like
  flags but are not (SDL reads any non-preset value as a packed custom blend). No bit
  operators: a blend mode is one choice. The 32-bit underlying type leaves room for
  custom blends (R26), which neutrino will encode itself when `custom_blend` is built.
  Considered: keeping the alias (the seal exception stays), the same enum with SDL's
  values (a `static_cast` instead of a mapping, but neutrino would rely on SDL's numbers),
  and a narrower `layer_blend` for composites (a second type for little gain).
- **R35 — One flip enum, `flip_mode`** (implemented 2026-11-16). `sprite_flip` is renamed
  `flip_mode` and moved to `video/flip_mode.hh`: bit flags none, horizontal, vertical,
  diagonal, promoted with `d_NEUTRINO_PROMOTE_ENUM`, so two flips compose with `^`.
  Sprites, `draw_texture`, quads and layer composites share it; the separate `flip` this
  document used to plan is dropped. Members and parameters keep the name `flip`
  (`flip_mode flip{}`), which a type named `flip` would have ruled out. No alias: every
  use was in this repository. Open: `diagonal` in a layer's composite. `draw_texture`
  cannot draw it, and for a layer that is not square it swaps width and height; either
  reject it (ENFORCE) or combine such a layer through textured geometry. Decide in RM1
  step 2.
- **R36 — `line_style` is neutrino's own** (implemented 2026-11-17). A struct in
  `video/line_style.hh` — `line_pattern` (solid, dashed, dotted) plus the dash, gap and dot
  spacing lengths — replaces the alias of `sdlpp::line_style`, which `painter_state.hh`
  could only see through `draw.hh`. `video/line_style_sdl.hh` maps it to sdlpp's and back.
  A struct, not an enum: the lengths matter (pixel art wants 2-pixel dashes, not sdlpp's
  default 8). SDL has no line styles; sdlpp keeps the style in its renderer wrapper and
  dashes lines itself, so once the painter holds its own state (RM1 step 4) the mapping may
  only be needed to call sdlpp's dashed and dotted lines.

**Agreed (2026-11-17):**

- **R37 — One painter** (supersedes R5, R7 and O5; amends R18 and R32; amended by R48:
  each scene owns its painter state and save stack). The painter is one
  application-wide object, owned by the application like the texture allocator: one
  `painter_state` and one save stack. Each scene owns a scene context — its layer stack,
  canvas, compositor and the painter state saved between its frames — which the scene
  manager gives the painter around the scene's `render()`. Declaring and settings are
  `base_scene` members (`declare_layer`, `style_of(id)`, `set_compositor`; the canvas and
  base-layer members dropped by R45); everything done during the
  frame is a free function on the painter (`push_layer`, `pop_layer`, `scoped_layer`,
  `clear`, `needs_redraw`, `push_state`, `pop_state`, `scoped_state`, `get_painter_state`,
  `get_painter_stats`). `push_state()` saves and changes nothing, as `SaveDC`,
  `cairo_save` and Canvas 2D's `save()` do; `push_state(s)` saves and then replaces the
  whole state with `s`. There is no public `set_painter_state`: replacing the whole state
  with nothing to restore is what the save stack exists to prevent, and only the scene
  manager does it, internally. (`push_layer` and `push_state` sharing the save stack was
  replaced by R39.) There are no explicit painters, `painter_scope` or
  `current_painter()`: rendering into a texture is `compose()`. *Why:* drawing is
  immediate and single-threaded and scenes render one after another, so only one painter
  is ever in use; per-painter state put `push_state` and `push_layer` on separate stacks
  that could interleave unnoticed; and nothing in the case studies needed layers inside a
  texture (§5.5). If something does, the painter can hold a stack of scene contexts.
- **R38 — `application_scoped<T>`** (implemented 2026-11-17). An internal lifetime for
  objects that live as long as the application, borrowed from bsw's `singleton` /
  `register_at_exit` (`~/proj/bsw`) and changed for neutrino. `instance()` makes the
  object on first use, from any thread, exactly once; `maybe()` returns it or null.
  `on_ready()` opens the application scope; `on_quit()` closes it right after the scenes
  and `teardown()`, before the application's own services and SDL, destroying the objects
  newest first. Outside an open scope `instance()` fails ENFORCE. Changes from bsw: run at
  the application's quit, not from `std::atexit` (textures and sounds must go while SDL is
  up, and a test runs many applications in one process); newest first, not oldest first (a
  later object usually depends on an earlier one); after quit the pointer reads null and
  the next application gets a new object, instead of a deleted pointer and a null queue;
  registration is locked; a throwing destructor is logged, not swallowed. Users: the
  painter context — the current scene, frame and target (2026-11-23; the painter's state
  stack before R48 made it per scene) — and the texture allocator (R20), made early in
  `on_ready()` so it outlives anything made later. Not a port of everything: the services the
  application owns directly stay as they are.
- **R39 — The painter state and the layers are independent** (2026-11-18; replaces R37's
  shared save stack). A layer push changes where draws go, not how: `push_layer` /
  `pop_layer` neither save nor restore the painter state, and `push_state` / `pop_state`
  do not touch the target. The layer pushes are recorded by the scene's layer stack, the
  state pushes by the painter's save stack, and each restores only its own, so the two
  may be popped in any order relative to each other. *Why:* both kinds of save captured
  the same state, so one shared stack would have had to forbid interleaving; with the
  state left to `push_state` there is nothing to keep in step, and a layer push costs no
  state read or write. The clip follows the painter, not the target: SDL keeps one per
  target, so the painter applies its clip after every `texture::bind()` and at the end of
  every `renderer_target_scope`, layer pushes and pops included. Cost: a helper that
  pushes a layer and wants its colour changes undone wraps them in `scoped_state()`.
  *Amended 2026-11-22:* a clip set straight on the renderer — a UI backend's: warlords,
  onyx_ui, lib_nuklearpp's sdlpp backend — survives neutrino's target switches. With no
  painter clip, a scope's end leaves the target bound again with its own clip, unless the
  painter had a clip when the scope began (the target held it, and it is gone); a texture
  just bound still loses the clip it held, so a pooled one never keeps its last owner's;
  `push_state` / `pop_state` touch the clip only when the painter's changes; and the copy
  of canvases into the window puts the window's clip back as it was.
- **R40 — Typed coordinate spaces** (2026-11-21; planned, docs/coordinates.md). Every
  coordinate in the public API of lib_sdlpp and neutrino has a space — display, window,
  render, output, texel, normalized, or neutrino's world — and a kind: `pos`, `delta`,
  `extent` or `area`, templates over a space tag with one algebra. sdlpp owns every space
  but world, since SDL defines them and their mappings; its events carry typed values and
  its draw calls take render types only, a clean break migrated with neutrino and
  lib_nuklearpp. A rule picks each type: a point measured from another is a `delta`; float
  for what is placed, moved or animated, int for whole pixels by nature. *Why:* the same
  type questions came back with every API, and window coordinates could still reach a draw
  call. It decides the composite settings' types: `offset` and `pivot` are
  `render_delta`s, `source` a texel rectangle.
- **R41 — When a canvas takes its texture; the base layer's texture as the canvas**
  (2026-11-21; amends the costs of R23 and R31). A scene's `detail::canvas` is made with
  the scene and takes its texture only when something is drawn onto it:
  - **a scene without layers** takes it at its first frame: `render()` draws onto it;
  - **a scene with layers** takes it only when combining needs one. During `render()` the
    base layer is always bound, so nothing draws onto the canvas before the compositor.

  Combining decides, frame by frame, whether the canvas shows the base layer's texture.
  It does when three things hold:
  - the compositor is the default one;
  - the base layer's composite settings are the identity: opacity 1, a white tint, no
    offset, scale, rotation, flip, `source` or `destination`, and a blend of `none` or
    `blend`, which leave it unchanged when drawn onto a transparent target (not `add`,
    which keeps the target's alpha: amended 2026-11-22);
  - the canvas is redrawn every frame.

  Then the declared layers are drawn straight onto the base layer's texture, and that
  texture is the scene's result for the frame: no canvas texture, no copy. Otherwise the
  canvas takes its own texture from the pool, cleared, and the base layer is drawn onto it
  like any other layer; the texture goes back once sharing is possible again.

  - **Invisible:** shared or not, the pixels are the same; a test switch turns sharing off
    to show it.
  - **Lifetime:** the base layer's texture is cleared or resized only at the scene's next
    frame, after the copy into the window. A transition that keeps an old scene's last
    frame keeps the scene, not only its canvas.
  - **A scene not rendered in a frame**, hidden under one with `hides_below`, gives its
    every-frame canvas's texture back to the pool at the end of that frame and takes one
    again when it is next drawn. An on-demand canvas keeps its texture: its contents are
    the point.

  *Why:* R19's saving without R19's rules: the base layer stays an ordinary layer and the
  canvas the result, and a scene that does not move or fade its base layer pays for no
  canvas. Considered: a separate canvas always (R23 as written: one texture and one
  full-screen copy per scene with layers, every frame); the base layer as the result (R19:
  four special rules); hidden scenes keeping their canvases (instant to show again, at the
  cost of a full-size texture each).
- **R42 — Canvases and layers mirror the target they are drawn into** (2026-11-21; amends
  R23's "the canvas is at the render size" and §5's "logical size for free"; settles O7
  for layers without a size of their own). A scene's canvas and its layers without a
  `size` take the pixel size of the target the scene manager draws into — the window's
  output size, or a texture a test binds — and that target's logical presentation, set on
  their own view at every bind (SDL 3.4 keeps the logical presentation per target).
  Drawing into them maps render space onto their pixels exactly as drawing into the
  target does, so a full-screen scene is pixel-identical to drawing straight into the
  window, at any window size and on HiDPI screens; `render_size()` inside them is the
  same as outside.
  - **The copy is 1:1:** the target's logical presentation and clip are turned off while
    the canvases are copied into it, and the canvas's composite `offset`, in render units,
    is scaled to its pixels.
  - **A layer with its own `size`** keeps its own pixels, as before (O7 stays open for
    it).
  - **The target's size** is read with its presentation off: under a logical
    presentation SDL's current output size is the presentation rect's, smaller than the
    target when there are letterbox bars (fixed 2026-11-22; canvases were made at that
    size and drawn into the bar).
  - **Cost:** output-size textures: 8.3 MB each at 1080p, where the logical size would
    have cost 1 MB at 640×400.

  *Why:* everything is addressed in render space; only the resolution of the result was
  in question, and drawing through a canvas should look as it does today. Considered: the
  logical size (small textures and an exact upscale for pixel art, but lines, rotated and
  smoothed sprites coarser than today on big and HiDPI windows); a per-application choice
  (more to build before anyone needs it).
- **R43 — How the compositor runs** (2026-11-22; resolves O9; amends §5.4.2 and R40's
  `pivot`).
  - **When:** in `scene_frame::finish()`, after the layer frame ends. The canvas is bound,
    cleared unless it is redrawn on demand; the compositor runs; the canvas is unbound;
    the layers pushed this frame and the canvas are marked drawn. Each scene is combined
    before the scene above it renders, so `below()` is ready. A compositor that throws is
    treated like a `render()` that throws: the scene is popped. A canvas that cannot be
    made skips the compositor, and the scene shows nothing in that frame.
  - **Painter state:** the compositor starts with the default state
    (`scoped_state(painter_state{})`), whatever `render()` left.
  - **`draw_layer` draws in the canvas's pixels**, with the canvas's logical presentation
    and the painter's clip off, as the copy of canvases into the window does (R42). A
    layer that mirrors the canvas is copied 1:1; `offset`, `pivot` and `destination`, in
    render units, are scaled by the presentation. The clip does not apply to
    `draw_layer`.
  - **Placement:** `destination` (new), an optional rectangle in render units.
    - *Unset:* in place. A layer that mirrors the canvas lands on the same pixels; a layer
      with its own `size` is stretched over the render area (the presentation rect, not
      the letterbox bars; the whole canvas without a presentation). A `source` part stays
      where it was in the layer.
    - *Set:* the layer, or its `source` part, is stretched onto it, as `draw_texture(t,
      dst, params)` does. A layer that mirrors the canvas, without a `source`, gives its
      picture, the render area, not the letterbox bars around it. Brix's plasma behind the options menu is a layer one texel per
      4×4 cell, with `destination` the board rectangle (§7.1).
  - **Transform order:** flip, then scale and rotation about the pivot, then the offset.
  - **Fields in RM1:** blend, opacity, tint, offset, scale, rotation, pivot, flip, source,
    destination; effects in RM4. `scale` is a factor per axis (`layer_scale`, from one
    `float` for a uniform scale): a TV-off transition collapses the picture to a line, and
    turning a `float` into a pair later would break code.
  - **`pivot`** is optional; unset is the centre of the rectangle the layer is drawn onto,
    as `draw_texture`'s pivot is, and a pivot is measured from that rectangle's top-left.
    A layer that mirrors the canvas, drawn whole in place, is copied whole, bars included,
    so the copy stays exact, but its pivot is measured from the render area it shows.
    Offset and pivot stay `point` until R40's `render_delta` exists.
  - **A `mod` layer below full opacity** is drawn with `mul`: SDL's `mod` ignores alpha
    and cannot fade, and `mul` with the usual modulation fades the tint exactly for the
    layer's opaque pixels (§5.4.1).
  - **A scene that throws** in `render()` or in its compositor still shows the scenes
    finished below it in that frame: their canvases are drawn before the exception goes on.
  - **Bad settings** (a diagonal flip; a non-finite rotation, scale or opacity; a
    negative scale) fail ENFORCE. A canvas's are checked at the end of its scene's frame,
    so the scene with them fails; one changed after that is skipped and counted when the
    canvases are drawn into the window, so it does not fail the frame of every scene.
  - **A compositor reads the canvas's mapping once:** the geometry and presentation rect
    are read at the first `draw_layer` onto a target, not at every one.
  - **Scope in RM1 step 3:** the public `composition`, `default_compositor`, `below()`,
    `set_compositor` / `get_compositor`. Wrapping and its tests stay in step 5.
  - **`below()`** is the canvas textures of the visible scenes below, bottom to top, as
    `const texture*`, without the canvases with nothing drawn. The scene manager hands
    them to each scene's frame.
  - **`source` is in render units** for a layer that mirrors the canvas, and for a canvas,
    like everything drawn into them: split screen's left half is `{0, 0, w / 2, h}` in
    render units, bars or not. A layer with its own `size` takes it in its own pixels.
  - **A canvas is drawn into the window with every flat setting**, as a layer that mirrors
    the canvas (was: blend, opacity, tint and offset only): a transition can shrink or
    turn a whole scene.
  - **A hidden scene** (under one with `hides_below`) gives back the textures of its
    every-frame layers, the base layer's included, as it does its every-frame canvas's
    (amends R41). Its on-demand layers keep theirs.

  *Why:* the default compositor and every custom one share one placement rule, and the
  copies stay exact for the layers that mirror the canvas. Considered: `draw_layer` in
  render space with the presentation on (the clip would apply, but a letterbox at a
  non-integer scale has a fractional presentation rect, which shifts or blurs a layer by
  part of a pixel); stretching an own-size layer over the whole canvas (brix's plasma
  would cover the screen instead of the board); one uniform `scale`.

- **R44 — The bottom full-screen scene draws straight into the window** (2026-11-22;
  extends R41). When the lowest visible scene has `hides_below`, declares no layers, has
  an every-frame canvas whose composite settings are the identity
  (`is_identity_style`, as for R41), and no scene above it runs a compositor of its
  own — the only thing that
  can read `below()` — its canvas would only copy it 1:1. It then draws straight into the
  window, as before canvases: no canvas texture, no clear, no copy, two target switches
  fewer, and pixels identical to drawing into the window, not ±1. Its every-frame canvas
  gives its texture back, shows nothing and is not in `below()`. When any condition
  fails, the scene goes through its canvas as R23 says. A test switch turns it off
  (`detail::shortcuts()`), as R41's does.
  - **The window's clear goes with the copy that covers it.** A full-screen scene's canvas
    is copied with `none` (§5.4, the default from `hides_below`), so in place, unmoved and
    unscaled, it writes every pixel of the window: the scene manager, which clears the
    window, then does not (`scenes_manager::covers_target()`), unless that canvas ends the
    frame with nothing to show, or the scene has no layers, its canvas could not be made
    and its `render()` draws into the window — until the copies, nothing else is drawn
    there (amended 2026-11-23: a failed base layer discards its drawing, painter.md §3.6).
    Where nothing was drawn on such a canvas the window gets a transparent pixel, which
    shows black; `screenshot()` returns it opaque (R46). In a window made
    with `window_flags::transparent`, where alpha shows, full-screen canvases blend over
    the cleared window instead (their default blend is `blend` there), so nothing becomes
    see-through that was opaque before.
  - **Fewer target switches:** a texture taken from the pool is cleared as it is first
    bound (`bind_pooled`), not bound once more by the allocator to clear it; and a logical
    presentation already on a target is not set again.

  *Why:* every existing game is one full-screen scene without layers, and drew straight
  into the window; through a canvas it paid a full-screen clear, a full-screen copy, an
  output-size texture and two flushes of SDL's command queue every frame for the same
  pixels. Considered: the overlays too (their canvas isolates them: drawing them straight
  into the window would change how `none` draws look whenever a fade starts); keeping the
  window's clear (one more full-screen pass when the bottom scene goes through its
  canvas).

- **R45 — Scenes are logical; everything visual is a layer's** (2026-11-22; amends R16,
  R23, R31, R41, R43, R44). A scene is a logical part of the game — brix's main menu,
  play scene, game-over scene — and its flags say how it relates to the scenes below.
  Fades, shakes, turns and a picture kept between frames are done with its layers.
  - **A scene has no style and no redraw mode.** Its picture (the internal canvas) is
    copied over the scenes below as it is, 1:1, with the blend its flags give: `none` with
    `hides_below`, `blend` without, `blend` for every scene in a transparent window. The
    flags are the only source; nothing the game writes changes it. Planned
    `canvas_composite()`, `set_canvas_redraw()`, `canvas_needs_redraw()` and `canvas()`
    are dropped; a scene that keeps its picture declares an on-demand layer (simplex
    keeps its own texture: R47).
  - **The base layer is internal** (amends R31): drawing with no layer pushed goes to it,
    it is drawn as it is, and there is no `base_layer()`. A compositor of the scene's own
    reaches it as `layers().front()`; drawing that must move or fade goes on a declared
    layer.
  - **Names:** `layer_composite_settings` is `layer_style`, how a layer looks when
    combined; `composite(id)` is `style_of(id)` (a member named `layer_style` would hide
    the type inside every scene); `layer_settings::composite` is `layer_settings::style`.
  - **The public members:** `declare_layer` and `style_of(id)`, protected members of
    `base_scene`; `set_compositor` / `get_compositor`, public; and, during `render()`, the
    free `push_layer`, `pop_layer`, `scoped_layer(id)` (RAII, movable, not assignable),
    `clear()` (the current layer to its clear colour, or the scene's picture) and
    `needs_redraw(id)` (for an on-demand layer).
  - **Consequences:** the canvas copy into the window is a plain copy again (R43's every
    flat setting for canvases is undone); R44's direct path no longer checks canvas
    settings; R41's sharing always holds with the default compositor and an every-frame
    canvas. An overlay can no longer multiply or add over the scenes below; dimming them
    is a translucent rectangle, as §5.4.1 says.

  *Why:* a scene that styles itself is a visual helper in a logical unit's clothing; the
  visual tools already exist one level down, where they compose. Considered: public
  `scene_style()` / `canvas_composite()` for transitions (transitions are RM5's wrapping
  compositors over the old scene's last frame, not styled scenes); a `blend` the game may
  set (the flags would stop being the truth).

- **R46 — `screenshot()` replaces `capture_frame()`** (2026-11-22). `capture_frame()` read
  whatever target was bound: inside a scene's `render()` the scene's own unfinished layer,
  outside it the window after the last present, which SDL leaves undefined; the promise
  that it "combines first" had no user. `screenshot()` (`<neutrino/video/screenshot.hh>`)
  returns what the window shows: every visible scene, the pinned one included, drawn again
  into a texture of the window's size and logical presentation, read back opaque. It is
  called from anywhere a scene is not being drawn — `fixed_update()`, an event handler, a
  test — with the window bound (ENFORCE otherwise), costs one full extra draw only when
  called, and relies on `render()` not advancing state. Reading a texture stays
  `texture::to_image()`; KE's test, whose scene is not on the stack, draws it into a
  texture with `compose()`.

  *Why:* the game wants the picture on the screen, from wherever it handles a key; one
  call that works anywhere beats a readback whose meaning depends on where it runs.
  Considered: keeping `capture_frame()` as a low-level readback beside it (two ways to
  read pixels, one of them a trap); a screenshot request answered at the end of the next
  frame (no extra draw, but a callback where a value will do).

- **R47 — Simplex keeps its own picture; simplex programs have no layers** (2026-11-23;
  amends R45 and §5.6). The simplex picture stays the adapter's texture, at the program's
  own pixel size, drawn in `on_create` and every fixed tick; the simplex scene's
  `render()` copies it onto the scene's canvas, as it does today. RM1 step 3's "the
  simplex scene's persistent picture on an on-demand layer" is dropped. Simplex exposes no
  layers to its programs. If it ever offers more, it is at most the default compositor's
  settings, for fades and the like; that is not planned.

  *Why:* simplex is for rapid development, and layers are machinery its programs do not
  need. Moving the picture onto an on-demand layer would also mean drawing in `render()`
  instead of each fixed tick, or pushing a layer outside `render()`, where pushes are
  refused. Either would change simplex or the layer rules for no gain to its programs.
  After a device reset the picture is lost like any game texture (RM6), as it would be on
  a layer, which simplex could not redraw either. Considered: an internal on-demand layer
  at the program's size, so a later fade would be its `layer_style`.

- **R48 — Each scene owns its painter state; painter calls need a scene** (2026-11-23;
  amends R24, R37 and §3.3). Each scene's context holds its `painter_state` and its save
  stack. A scene is current during every call the scene manager makes into it — enter
  and exit, `fixed_update`, events, `render()`, its compositor, pause and resume, resize
  — and the painter functions act on the current scene's state. A new scene starts from
  the defaults; what it sets stays until it changes it. A draw or a state function with
  no scene current fails ENFORCE: in `ready()`, `teardown()`, a scene's constructor.
  Texture operations (`create`, `fill`, `bind`, `compose`, `to_image`) work anywhere; a
  bake outside a scene fails at its first draw. Each draw call sets the SDL state it
  uses, at every call: colour and blend for primitives, the line style for `draw_line`
  too, the blend in its source's form for textured draws (R49). The renderer's own
  colour, blend and line style are scratch: state set on it through `native()` does not
  reach neutrino's draws, and a save no longer keeps an SDL blend with no neutrino name.
  Each call into a scene pops the state it pushes. Drawing into a layer or canvas whose
  texture cannot be made does nothing: the painter knows the bound target discards, and
  each draw call checks it; the bottom scene without layers draws into the window instead
  (amended 2026-11-23, painter.md §3.6). Simplex: the
  adapter creates its scene before `on_create` and runs `on_create` as that scene, still
  in `ready()`, so a failed `on_create` still fails startup. Engine tests run inside the
  harness's scene. `custom_blend` is deferred until a game needs one.

  *Why:* a scene is a part of the game — the main menu, a level — and every game runs
  inside one, so drawing outside a scene has no meaning, and a scene's state is its own
  wherever its code runs: nothing leaks between scenes, a push one scene forgets cannot
  reach another, and nothing is copied when scenes switch. Setting what a call uses at
  every call means nothing depends on an earlier one. *Amended 2026-11-23:* a 1×1 sink
  target was chosen first, bound for a failed layer or canvas, because it discards direct
  SDL draws too, with no check in each draw call. The flag replaced it: every draw goes
  through the painter, nothing draws through `native()` today, and the sink needed its
  own lifetime, reset handling and a fallback for when it could not be made, while SDL
  still processed every draw into it. Considered: one state saved into each scene
  around its `render()` only (the updates and event handlers of all scenes would share a
  state); every `render()` from the defaults with nothing kept; an application-level
  stack for code outside scenes; a public "run as scene" helper for games' tests (a back
  door around the rule); caching the SDL state last set (anything that touches the
  renderer directly would make it wrong); the sink (above); an empty clip on the target
  below.

- **R49 — One blend for every draw; textures know their alpha** (2026-11-23; amends R34,
  §3.3 and painter.md §2.8). The painter's blend applies to every draw — primitives,
  sprites, text, quads, meshes, `draw_texture` — and a texture keeps no blend of its own:
  `texture::set_blend_mode` is removed. The default blend is `blend`. `blend_mode` names
  the operation only — none, blend, add, mul, mod; `blend_premultiplied` and
  `add_premultiplied` leave the public enum and stay in the engine's mapping. A
  texture's `alpha_mode` — straight or premultiplied, `texture::set_alpha_mode()` — says
  what its pixels hold: target textures are premultiplied by default, fixed and streaming
  ones straight, and `adopt()` follows the access it reads. The engine picks SDL's form
  from the two: `blend` and `add` take their premultiplied forms for a premultiplied
  source; `none`, `mul` and `mod` are the same in both. Primitives are straight.
  `to_image()` returns straight colour. Tint on a premultiplied source scales colour as
  well as alpha (RM1 step 6). Engine internals — combining layers, copying canvases, the
  simplex copy — set SDL directly. Pixels change on purpose: translucent primitives drawn
  with the default blend, and translucent pixels of target textures drawn again or read
  back.

  *Why:* one source of truth: the game says how to combine, the texture what it holds.
  Sprites can be drawn additively — impossible before, since atlas pages are internal
  and their blend could not be set — and a texture made with `compose()` is drawn again
  without dark edges. The `none` default was a trap for translucent primitives drawn
  into a layer (painter.md §2.8). Considered: textures keeping their own blend (two
  sources); the painter's blend with a "the texture's own" value; leaving premultiplied
  sources to the game, with `blend_premultiplied` by hand; tracking what was drawn into
  each target; the alpha mode fixed at creation; `to_image()` returning premultiplied
  pixels (a saved PNG would have dark edges, and `from_image(t.to_image())` would be
  wrong).

**Proposed, not yet agreed:**

- **R7** — superseded by R37: the free functions draw with the one painter; there is no
  `current_painter()` or `painter_scope`.
- **R8** — superseded by R23 (a scene with no declared layers draws onto its canvas).
  Outside any scene, draw calls go straight to the bound target or the window, as today
  (§4.1).
- **R9** — layer settings: space, resolution, redraw mode, and the initial composite
  settings (§5.1).
- **R10** — an RAII `layer_scope` beside `push_layer` / `pop_layer`; unbalanced pushes are
  ENFORCE errors (§3.3, §3.5).
- **R11** — keep the `draw_*` names. The `paint_*` prefix of the old draft signalled
  deferral, which is gone (§4.1).
- **R12** — order within a layer is call order; `sprite_batch` keeps y-sorting sprites
  (§3.4).
- **R13** — after a device reset, on-demand layers and canvases need a redraw and their
  owners draw them again (§8.3). Elaborated 2026-11-15 with R20: a texture that
  needs a redraw holds only its clear colour and is not combined until pushed;
  `needs_redraw(id)` is also true in a layer's first frame and after a resize; the painter
  marks handles drawn at the end of the frame, so `needs_redraw(id)` gives one answer for
  the whole frame; for an every-frame layer it is always false; `has_content(id)`
  is false for an on-demand layer that needs a redraw and was not pushed;
  `canvas_needs_redraw()` does the same for on-demand canvases
  ([`painter.md`](painter.md) §2.5, §3.3). Renamed 2026-11-15 from `lost(id)` /
  `mark_valid()`: the flag is also set where nothing was lost (the first frame, a texture
  from the pool), and SDL's `SDL_EVENT_RENDER_DEVICE_LOST` means something else.
- **R15** — alpha rules for combining: ARGB8888 targets cleared to transparent (`mod`
  layers to opaque white); each layer combined with the premultiplied form of its blend;
  opacity scales colour as well as alpha (§5.4.1).
- **R16** — `layer_composite_settings` as live settings, changed through
  `p.composite(id)`; the `composition` calls of §5.4.4; `get_compositor()` so a transition
  can wrap the current compositor (§5.4.2, §5.4.4).
- **R17** — palette animation through INDEX8 page textures sharing an `SDL_Palette`, with
  palette alpha instead of a colour key (§6.2).

**Open:**

- **O1 — Layers per scene or per application.** Resolved 2026-11-10: per scene (R18).
- **O2 — `push_state` / `pop_state`.** Resolved 2026-11-10: yes (R24).
- **O3 — One effect over several layers** (wobble the world, not the HUD): the same effect
  on each layer, layer groups, or a compositor that `compose()`s those layers' textures
  into one texture and draws it through the effect.
- **O4 — Where a layer's transform is applied:** the painter applies camera × parallax ×
  zoom to every call on a camera layer, or producers keep applying it (`world_renderer`,
  `sprite_batch`, as today).
- **O5 — Explicit painters.** Closed 2026-11-17 by R37: there are none; rendering into a
  texture is `compose()`.
- **O6 — Transitions.** Resolved 2026-11-10: the old scene's canvas is its last frame
  (R22).
- **O7 — Layer resolution:** settled for layers without a size of their own and for the
  canvas (R42: the target's pixel size and logical presentation). Open for a layer with
  its own `size`: whether render space maps onto its pixels, and how pixel snapping
  (`pixel_snap::logical`) applies inside it. A use case: brix's plasma at a quarter of the
  board's resolution (§7.1).
- **O8 — Blend per call:** a field of `quad_params`, or painter state only.
- **O9 — Composite fields.** Resolved 2026-11-22: the flat ones in RM1, effects in RM4;
  `scale` per axis; a `destination` rectangle (R43).
- **O10 — Whose compositor.** Resolved 2026-11-10: the scene's (R18).
- **O11 — The palette handle:** owned by an atlas or free-standing and shared by several;
  its operations (set an entry, cycle a range, fade toward a target palette); whether
  `with_palette` recolouring (`images_atlases_sprites.md`) becomes a palette swap on the
  GPU.
- **O12 — Declaring a scene's layers.** Resolved 2026-11-16 (R32), amended 2026-11-17
  (R37): in the constructor, normally in member initialisers, with `declare_layer`, a
  `base_scene` member — the scene owns its context, so it works before the scene is ever
  rendered. Considered: `on_enter`, and a `declare_layers(...)` hook the scene manager
  calls on push.
- **O13 — Paused scenes:** a scene under a `pauses_below` scene gets no `fixed_update`, so
  its picture does not change; the scene manager could skip its `render()` and reuse last
  frame's canvas (brix's options menu over the board, §7.1). An optimisation, not part of
  the model.
- **O14 — The exception safety net and the pinned scene.** When a frame's update or render
  throws, `application` calls `pop_scene()` to drop "the top scene". With a pinned scene,
  `pop_scene` acts below it, so a pinned scene that throws drops the active scene instead
  and keeps throwing. The safety net needs to know which scene threw (and unpin it), or the
  pinned scene's calls need their own guard.
- **O15 — Evicting pooled textures by age.** A pooled texture stays until it is reused or
  trimmed (R20). The textures of one size never outnumber the most held at once, so only
  many different sizes make the pool grow; the render size is handled, layers with a
  `size` of their own are not. If a game sizes layers from data (a minimap layer per
  level), the simplest rule is to destroy a pooled texture unused for N frames: the
  allocator counts frames in `begin_frame()` and records the frame each texture was given
  back. Decide when measured.

Text decisions T1–T4 are in §9.7; facade decisions F1–F8 in §9.3.

## 12. GPU backends (deferred)

SDL_Renderer gives textured triangles, per-vertex colour, blend modes (including additive
and custom) and render targets — enough for every case in §7 and every mesh effect in §6.
Per-pixel effects (distortion, bloom, lighting) need shaders. The build uses SDL 3.4.0,
which offers two routes; both change only the internals (§4):

| Route | Gains | Costs |
|---|---|---|
| SDL_Renderer on the GPU backend (`SDL_CreateGPURenderer`) + `SDL_GPURenderState` (custom fragment shader + uniforms per draw) | everything above, plus fragment shaders as an optional **material** on a draw call or a layer | needs Vulkan / D3D12 / Metal; shaders shipped per backend (SPIR-V / DXIL / MSL via SDL_shadercross); fragment stage only; web (brix's Emscripten build) unverified |
| Raw SDL_GPU (pipelines, passes, compute) | full control: own vertex formats, instancing, GPU particles | batching, text and primitives written from scratch — SDL_Renderer's merging of same-texture draws (§3.1) would have to be rebuilt inside the internals |

Proposed: no material until a game needs a per-pixel effect; when one does, the first
route, behind the same painter.

## 13. Out of scope

- **Command recording and frame-wide sorting** — not needed (R2, §3.1).
- **Particle system** — a producer; designed when a game needs one.
- **Scene graph / transform hierarchy** — a gameplay convenience, not a painter need. Flat
  layers plus the tilemap's implicit spatial grid are enough for 2D (Gregory §11.2.7.5:
  choose from measured need).
- **Culling beyond the tilemap** — per-layer bounds checks until measured otherwise.

## 14. Milestones

Rewritten 2026-11-09 for the immediate model. The numbers keep their old subjects where one
survives (RM1 the painter and quads, RM3 geometry, RM4 meshes, RM6 device reset, RM7 text,
RM8 Nuklear), so references from `font_plan.md` and `sprite_layer_plan.md` stay meaningful.

- **RM1 — The painter.** The public contract of §3.7. Order of work, each step green and
  committed on its own (reordered 2026-11-16, R32: layers first, because they only add to
  what exists and hold most of the open questions; painter state, a refactor with nothing
  new to show, last):
  1. *The texture allocator* — `texture_allocator` and `texture_handle`, tested alone,
     with the `SDL_EVENT_LOW_MEMORY` and render-reset handlers (R20). Done 2026-11-16.
  2. *Layers* — the `layer_stack` (R32) in a scene context owned by `base_scene`, with
     `declare_layer` / `style_of(id)` as scene members (R45); the one painter
     (R37) with its save stack and the free `push_layer` / `pop_layer` / `scoped_layer`,
     `push_state` / `pop_state` / `scoped_state`, `clear()` and `needs_redraw()`; the base
     layer (R31); clear on first push, on-demand layers; the frame steps; the default
     compositor, drawing into the window until step 3; layer styles without effects
     (blend, opacity, tint, flat transform, source), the alpha rules of §5.4.1, inspection
     (`layers()`, `info()`, `layer_texture()`, `get_painter_stats()`).
  3. *Scenes' pictures* — each scene's canvas (internal, R45) from the allocator, copied
     into the window in stack order with the blend its flags give; `below()`;
     `screenshot()` (R46).
  4. *Painter state* — each scene's own `painter_state` and save stack, current during
     every call into the scene; painter calls outside a scene fail; `draw.hh` sets the
     SDL state each call uses (R48); one blend for every draw and textures' alpha modes
     (R49); drawing into a failed layer or canvas does nothing. `custom_blend` deferred. Every
     existing test pixel-identical, except where R49 changes pixels on purpose.
  5. *Custom compositors* — `set_compositor` / `get_compositor`, `composition`, wrapping.
  6. *Quads* — tint, crop and `draw_picture` with the fit modes; independent of the
     painter, so it may go first. Preserve every placement rule of §3.2.

  Tests: [`painter.md`](painter.md) §5, RM1. Exit: tutorials, demos, simplex and the games
  draw unchanged on their canvases (±1 on blended pixels in overlays, one copy per visible
  scene); a layered test scene renders as specified.
- **RM2 — Producers on layers.** `world_compositor`'s slots become declared layers;
  `world_renderer`, `sprite_batch` and `texture_layer` draw on the current layer; KE and
  Luxor declare their layers; `get_renderer()` becomes hidden (tests move to an internal
  header). Exit: KE and Luxor frames identical except ±1 on blended pixels; nothing
  outside `src/` calls SDL_Renderer.
- **RM3 — Geometry and primitives.** Public `draw_geometry` (textured triangles,
  per-vertex colour); thick and gradient lines, curves. Exit: a brix-style beam and cursor
  demo.
- **RM4 — Meshes and effects.** Port `sprite_mesh` (as `mesh`) + `mesh_effect`s from
  lib_sdlpp simplex with the changes in §6; `draw_mesh` on atlas pictures and textures.
  Exit: each effect has a pixel test; subdivided `rotate_3d` shows no seam.
- **RM5 — Layer effects and transitions.** Composite `effects` applied when combined;
  effects on part of a layer; transitions as wrapping compositors over the old scene's
  canvas, which the scene manager keeps until the transition ends. Exit: brix-style board
  rotation with no readback; a Doom-style melt between two scenes.
- **RM6 — Atlas lifetime and device reset.** `register_atlas` internal; CPU pages kept;
  handlers for the three render events; re-upload of atlases; the events call the texture
  allocator's `on_targets_reset()` / `on_device_reset()`, so on-demand layers and canvases
  need a redraw; `on_textures_lost()` for game-owned
  textures. Exit: a forced device reset re-renders a KE frame pixel-identically.
- **RM7 — Text.** Planned in its own document, [`font_plan.md`](font_plan.md) (phases A–D,
  milestones FM0–FM7). Drawing stays immediate, so decision P2 there holds: FM3's
  `draw_text` draws through `draw.hh` now and lands on the current layer once RM1 exists,
  with no change to the call. Its original exit criteria: KE's BIOS text, a brix-style
  baked-colour font, an indexed-colour glyph-bank font with per-glyph Y offsets
  (Westwood-style, test data) and a TTF label draw through the painter; a forced device
  reset and a forced font replacement keep all of them; no game includes `onyx_font/…`.
- **RM8 — Nuklear as a producer (postponed).** A Nuklear backend that draws `nk_convert`
  output as geometry with atlas addresses (glyph atlases included), and an engine
  `nk_user_font` over `font`. Exit: a warlords-style UI panel on its own layer over the
  world.
- **RM9 — Palette animation.** `indexed8` atlas pages uploaded as INDEX8 textures with
  palette alpha instead of a colour key and blend mode blend; a palette handle (O11);
  `vga_canvas` on an INDEX8 streaming texture. Tests on the software renderer: one picture
  drawn before and after a palette change shows both colours; the transparent index shows
  what is below; a texture sharing the palette changes with it. Exit: a colour-cycling
  demo (water, lava) and `vga_canvas` without the CPU lookup.
- **RM10 — The pinned scene.** `pin_scene` / `unpin_scene` and the slot in
  `scenes_manager`; pausing by change of state, with `flags()` read every frame;
  `passes_input`; resize notices to the pinned scene (§5.7). *Status 2026-11-14:* done —
  `pin_scene` / `unpin_scene`, the slot, `passes_input`, resize notices, rollback, `finish()`,
  with tests; remaining — pausing by change of state and its tests (the balanced pairs
  under a pausing pinned scene, the busy-spinner case, flags changed at run time), O14, the
  exit demo. Tests: the pinned scene is
  drawn above every stack scene; push, pop and replace act below it; its `pauses_below`
  freezes the stack and unpinning resumes it; pushing and popping under a pausing pinned
  scene keeps `on_pause` / `on_resume` balanced; under a pause menu, a busy spinner and the
  game below it both get `on_pause`; a `passes_input` scene sees events and the scene below
  gets them too; a flag changed at run time takes effect at the next frame; pinning
  replaces the previous scene (`on_exit`); unpinning with none pinned is a no-op. Exit: a
  console-style scene pinned over KE that slides down on `~`, takes the keyboard while
  open and freezes the game. Follow-ups, outside RM10: text input, a log backend, a command
  registry (§5.7).

Dependencies: RM1 → RM2; RM1 → RM3 → RM4 → RM5; RM1 → RM6 → RM7 (text uses RM6's
device-reset handling and replacement; `font_plan.md` §5 has the detail — its Phases A–B
run before the painter); RM3 + RM7 → RM8 (postponed). RM9 needs only the atlases (M0) and
can run before the painter; device reset (RM6) must keep page palettes. RM10 changes only
the scene manager and can run before or after RM1. Sprite plan P3.7: its M3 (drawing
sprites) needs RM1 (tint, crop); its M1 / M2 do not depend on the painter.

## References

- J. Gregory, *Game Engine Architecture*, 3rd ed.: §11.2.7.2 primitive submission and
  state leaks; §11.2.7.3 geometry sorting (translucency forces back-to-front); §11.2.7.5
  choosing a scene graph; §11.4 effects and overlays; §12.1.1 cel / sprite animation.
- SDL 3.4 source (`cmake-build-*/_deps/sdl3-src/src/render/`): `SDL_render.c` — copies as
  geometry (4047), the software renderer's quad reinterpretation (4958, colour mod at
  5203), `FlushRenderCommandsIfTextureNeeded` (340), the flush in `SDL_SetRenderTarget`
  (2797); `gpu/SDL_render_gpu.c:1287` and the same merge loop in the GL, GLES2, D3D,
  D3D11, D3D12 and Vulkan backends. Palettes: `SDL_render.c` — `FlushRenderCommandsIfPaletteNeeded`
  (350), `UpdateTexturePalette` (706), INDEX8 default blend mode (1537), format choice in
  `SDL_CreateTextureFromSurface` (1832–1848); `gpu/SDL_render_gpu.c:72` (palette texture
  types); `video/SDL_blit_1.c:426` (`Blit1toNAlpha`); `video/SDL_blit_slow.c:200–213`
  (modulation in the premultiplied modes).
- SDL 3.4 headers: `SDL_gpu.h`; `SDL_render.h` (`SDL_CreateGPURenderer`,
  `SDL_GPURenderState`).
- lib_sdlpp: `include/simplex/mesh.hh`, `include/simplex/effects.hh`,
  `examples/simplex/example_animation.cc`.
