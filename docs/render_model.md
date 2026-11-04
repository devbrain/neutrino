# Painter — what the engine can draw

Status: **draft for discussion** (written 2026-11-04; revised the same day: painting vs
rendering, record-time validation, meshes and effects, GPU options, the painter
vocabulary, picture placement moved here from the sprite plan's D11, atlas lifetime, device reset and text). Nothing here is
implemented beyond what §10 lists as existing. Companion documents:
[`images_atlases_sprites.md`](images_atlases_sprites.md) (atlas vocabulary) and
[`sprite_layer_plan.md`](sprite_layer_plan.md) (P3.7 — sprites, a *producer* that paints
through the painter).

---

## 0. Vocabulary

| Term | Meaning |
|---|---|
| **painter** | the engine subsystem that takes paint commands, orders them and gets them on screen |
| **painting** | the public, logical side: consumers issue paint commands onto layers |
| **rendering** | the private, physical side: the painter turns sorted commands into SDL_Renderer calls. "Renderer" stays reserved for SDL_Renderer (`sdlpp::renderer`) |
| **layer** | an ordered plane with a transform and an optional pass (§5) |
| **paint list** | the commands recorded for one layer in one frame (today's `sprite_batch`) |
| **quad** | a picture from an atlas placed on screen (§3.2) |
| **geometry** | triangles, optionally textured (§3.3) |
| **mesh** | a subdivided textured quad with deformation effects (§6) |
| **producer** | anything above the painter that issues paint commands: tilemap, sprites, text, primitives, beams, meshes, particles |

## 1. Why

The sprite redesign kept running into drawing questions: what a sprite *is* when drawn,
how primitives and effects interleave with tiles, where z-order lives, how a whole board
rotates. Today the answers are spread over `draw.hh` (immediate calls with global state),
`sprite_batch` (sorted, atlas visuals only), `render_layer` / `world_compositor` (layer
order) and `texture_layer` (draws around the batch). This document pins down the bottom of
the stack so every producer above it targets one contract.

## 2. Levels

| Level | Concepts | Knows about |
|---|---|---|
| **Game** | hero, gem, laser gun, board rotation | game state; picks what each producer shows |
| **Producers** | tilemap, `sprite` + `animation`, text, primitive helpers, beams, meshes + effects, particles | time and appearance; each one only issues paint commands |
| **Painting** (public) | layers, paint lists, depth, quads / geometry / meshes / primitives, handles | *what* to draw, where, in which order |
| **Rendering** (private) | sorting, batching, render state, targets, atlas resolution, backend | *how* it reaches the GPU |

Rules:

- **The painter never knows about animation.** An animation is a producer choosing which
  `atlas_address` to paint this frame. A tile, a character frame and a glyph are the same
  thing to the painter: a quad cut from an atlas picture.
- **Consumers (KE, brix, Luxor, simplex) only ever see painting.** Rendering is entirely
  private and may change backend without breaking them.

## 3. Painting — the public contract

Paint calls look immediate, but each one **records a command**; nothing touches the GPU
until the frame is rendered.

```cpp
// sketch — names provisional
paint_list& list = painter.layer(ui_layer);
list.paint(position, address, quad_params{.scale, .mirror, .rotation, .tint, .crop}, depth);
list.paint_picture(dst_rect, address, flip::none, fit::aspect, depth);
list.paint_geometry(vertices, indices, address_or_none, blend, depth);
list.paint_mesh(mesh, source, blend, depth);          // §6
list.paint_line(a, b, color, depth);                  // primitive helpers → geometry
list.paint_rect_outline(r, color, depth);
```

### 3.1 Commands carry their own state

A command captures colour, tint, blend and source **at record time**. There is no global
render state that a later command can inherit or leak (Gregory §11.2.7.2, "state leaks").
If `set_draw_color` survives as a convenience (R7), it only sets the default for
subsequent *recordings* and never touches rendering state.

### 3.2 Quads — placing a picture

A quad is a picture from an atlas placed at a position. Its parameters:

```cpp
enum class flip : std::uint8_t {      // bit flags, as today's sprite_flip
    none = 0, horizontal = 1u << 0u, vertical = 1u << 1u, diagonal = 1u << 2u /* tiles */
};

struct quad_params {
    float  scale{1.0f};
    flip   mirror{};
    float  rotation_degrees{0.0f};
    color  tint{colors::white};       // colour + alpha modulation
    insets crop{};                    // source inset in picture pixels
};
enum class fit : std::uint8_t { stretch, aspect, centre };
```

Placement rules (moved here from the sprite plan's D11; all exist in `draw.cc` today and
are preserved):

- `position` is the picture's **pivot** (D9: per-picture default, per-frame override),
  snapped to whole pixels.
- Scale and rotation are about the pivot; flip mirrors the pivot with the picture.
- Diagonal flip (tiles) goes through textured geometry.
- Rotation is byte-identical to the unrotated path at 0°.
- `tint` modulates colour and alpha for this command only.
- `crop` insets the source; the destination size follows the cropped source (brix's six
  `draw_tile_*` variants).
- `paint_picture(dst, address, flip, fit)` fills a destination rect: `stretch`, `aspect`
  (fit inside, keep ratio), `centre` (no scaling). The exact-integer `stretch` path is the
  seam-free one tilemaps use (warlords's portraits and shields need the fit modes).
- Every engine studied has tint; warlords needs colour/alpha mod and the fit modes.

### 3.3 Validation and resolution happen at the call site

Deferral moves *execution* to the frame, not *validation*:

- `paint(position, address, …)` resolves `atlas_address → {page, src rect, pivot}`
  immediately (the M0 `resolve_visual` step). An unknown atlas or picture index fails
  **there**, at the consumer's line, with ENFORCE — same as `draw.hh` today. An invalid
  address ("no picture") is a silent no-op, as today.
- The recorded command stores the *resolved* page and rect, not the address. Rendering
  does no registry lookups, and the sorter sees the real texture page it groups by.
- Index ranges, unknown layers, invalid parameters, painting into a target that is also
  sampled in the same pass — all checked on record.

What can only fail later is not a consumer error:

| Failure | Handling |
|---|---|
| Rendering: SDL rejects a call, device lost, target creation fails | the painter's own per-frame reporting (today `draw_stats.failed`, `DRAW_CHECK`) |
| Lifetime: a texture/atlas released between record and render | **prevented, not reported**: releases are deferred until after the frame that used the resource |

### 3.4 Sort key

`(layer, depth, seq)`, compared lexicographically, stable. `seq` is submission order.

- **In 2D, order is correctness, not performance.** Nearly everything is alpha-blended, so
  painter's order is mandatory (Gregory §11.2.7.3: translucency forces back-to-front).
  Grouping by texture is only allowed *within* equal keys.
- `depth` covers in-layer ordering: y-sort for top-down/isometric; "beam above tiles,
  cursor above beam" in brix.
- A consumer that never sets depth gets pure submission order — exactly immediate-mode
  behaviour.

### 3.5 Resources

Public handles only: `atlas_id`, `atlas_address`, `texture`. A handle used
by a command stays valid until that frame is rendered (§3.3). Vertex data passed to
`paint_geometry` / `paint_mesh` is copied into the frame's storage.

### 3.6 Debugging

Errors surface at the call (§3.3). For the frame as a whole, the painter can dump its
sorted command list (layer, depth, seq, kind, page, blend) — the deferred equivalent of
breaking on a draw call.

## 4. Rendering — private

Everything below painting lives in `src/` and is not installed:

- sorting and batching (consecutive commands with the same page and blend → one
  `render_geometry`);
- render state (blend, colour mod, bound target);
- layer passes: creating targets, rendering into them, compositing (§5.2);
- the per-frame vertex arena and deferred resource release;
- **the backend.** Today `sdlpp::renderer`. Moving to SDL_Renderer on the GPU backend, or
  to raw SDL_GPU (§12), changes only this level.

The existing helpers in `src/neutrino/video/draw.cc` are its seed: `resolve_visual`
(address → page + rect), quad vertex building for rotation/flip (the diagonal path at
`draw.cc:288`), line/rect tessellation, pixel snapping.

### 4.1 Relationship to `draw.hh`

`draw.hh` stays public with its current signatures, but its calls become **painting**:
each records into the current layer in submission order. Consumers that draw without
layers (simplex, tools, tutorials) see the same picture as today, because submission order
*is* immediate order. `draw.hh` and the layer API both sit on the same rendering level; the
painter does **not** render through `draw.hh` (its global state and one-call-per-draw unit
are exactly what rendering must avoid).

## 5. Layers

```text
layer     = { transform, optional pass }
pass      = { target, composite }
composite = { mesh + ordered effects, blend, colour/alpha mod }
```

### 5.1 Transform

Camera × parallax for world layers, identity for screen space (HUD, menus). This is today's
`layer_view` / `world_layer_header`.

### 5.2 Passes

A layer with a pass renders its commands into its own target texture; that texture is then
painted into the parent **through a mesh** (§6). Because the composite *is* a mesh, every
mesh effect applies to a whole layer: brix's board rotation is `rotate_3d` on the board
layer; an underwater wobble is a `wave` on the world layer; a screen transition is
`perspective` or `pinch`. No special list of composite ops is needed.

The painter orders passes so a target is complete before anything samples it.

### 5.3 Baking (`compose()`)

KE (`assets/backdrop.cc`) and tutorial `drawing/04_render_textures` bake static content
into a texture once with `texture::compose([&]{ … })`. That stays: **a `compose()` scope is
a render boundary** — commands painted inside it are rendered into the target when the
scope ends, so the texture is ready to use immediately after.

## 6. Meshes and effects

Rotations, warps and deformations are first-class. The model takes them from lib_sdlpp's
`simplex` (`include/simplex/mesh.hh`, `effects.hh`), which already implements them:

- **`sprite_mesh`** — a quad subdivided into `cols × rows` cells (`SDL_Vertex`: position,
  colour, uv) laid out over a destination rect and a source region; `transform_vertices`
  deforms positions.
- **`mesh_effect`** — deformations applied to a laid-out mesh: `wave_horizontal` /
  `wave_vertical`, `skew_horizontal` / `skew_vertical`, `perspective` (trapezoid taper),
  `pinch_punch`, `rotate_3d` (pitch/yaw/roll, pivot, perspective factor).

Here a mesh is a **producer of textured geometry**: `paint_mesh(mesh, source, …)` records
it like any other geometry command. The source is either an `atlas_address` (resolved at
record time to page + placement) or a layer's pass target (§5.2) — the same effects work
on one picture or on the whole screen.

Notes for the port into neutrino:

- **Subdivision is required for correctness.** `SDL_RenderGeometry` interpolates uv
  affinely per triangle; a 1×1 quad under `rotate_3d` or `perspective` shows the
  diagonal-seam warp. A grid (e.g. 8×8) hides it. Plain 2-D rotation/scale/flip needs no
  subdivision — that stays a quad.
- **Split painting from rendering.** `sprite_mesh::render(renderer, texture)` renders
  directly; in neutrino the mesh and effects are public and painting it records a command.
- **Resolve against the page.** `layout_rect(atlas, frame)` normalises uv against the
  whole texture; with multi-page atlases it must use the resolved page and placement.
- **Tint/alpha into vertices.** Vertex colour is hard-coded white in `layout_rect_impl`;
  fades and pulses need it.
- **Explicit effect order.** Effects deform positions sequentially and `layout_rect`
  resets them; "wave then rotate" differs from "rotate then wave". Make the order part of
  the API (an ordered effect list per mesh).
- **Timing.** `update(dt, speed)` puts an effect on the caller's clock — the same question
  the sprite plan answers for animations (the owning scene's time). One answer for both.
- **Name.** `sprite_mesh` → `mesh` (neutrino's sprites are animation playheads, not
  pictures).

### 6.1 Effects taxonomy

"Effect" is not a painter concept. Every effect found so far is one of:

| Kind | Example | Expressed as |
|---|---|---|
| primitive with animated parameters | brix laser, brix cursor | producer painting geometry |
| deformed picture | waving flag, 3-D card flip | `paint_mesh` on an atlas picture |
| whole-layer transform / deformation | brix board rotation, screen wobble, transitions | layer pass composited through a mesh (§5.2) |
| many short-lived quads | particles, sparks (none in tree yet) | producer painting quads |
| per-pixel shader effect | distortion, bloom, lighting | needs a GPU backend (§12) |

## 7. Case studies

### 7.1 brix (`~/proj/ares/brix/src/brix/game/view/`)

`view::draw_board` draws, in order, objects (atlas tiles) → lasers → cursor, into the
board's own render target (`game_board_painter::start_render` / `end_render`), which
`present()` composites onto the screen.

- **Laser** (`view::draw_laser`): five parallel 1-px lines, alpha `{128, 200, 255, 200,
  128}` pulsed by `beam_func(seq)`, axis-aligned only. Here: one geometry strip with
  per-vertex alpha (edge → centre → edge) — smoother, one command instead of five, and
  diagonal beams for free.
- **Cursor** (`game_board_painter::draw_cursor`): three rect outlines with phase-shifted
  pulsing alpha → three outline commands at the top depth.
- **Board rotation**: renders the board, `SDL_RenderReadPixels` → surface → new texture,
  then `rotor.cc`'s `rotate_texture` projects a 3-D-rotated quad. The board already is a
  target, so the readback is unnecessary. Here: the board layer has a pass; its composite
  mesh carries a `rotate_3d` effect. No CPU round-trip (cf. card #19).

Nothing in brix needs shaders or additive blending.

### 7.2 KE and Luxor

- **Tiles** — `world_renderer` draws each tile with `draw_sprite`, one call per tile. They
  become quads in their layer; atlas pages keep same-texture runs long.
- **Sprites in world layers** — `sprite_batch::add(pos, layer, depth, visual)` is already
  §3.4 restricted to quads; it becomes a paint list whose commands hold resolved
  `atlas_address`es.
- **Screen-space UI** (KE) — a layer with identity transform.
- **Backdrops** — `texture_layer` draws straight to the renderer, bypassing the batch;
  becomes a quad in the bottom layer. KE's baked backdrop is a `compose()` (§5.3).

### 7.3 Call sites that depend on immediacy

The audit for the migration (§4.1) — code whose behaviour depends on draws having already
happened:

| Site | What it does | Here |
|---|---|---|
| `texture::to_image()`, `capture_frame()` (`src/neutrino/video/texture.cc`) | GPU → CPU readback | painter operation that renders first (card #19 part 2) |
| KE `tests/enemies.cc:172` | `save_bmp(capture_frame())` | unchanged API, render-then-capture |
| `texture::compose()` / `bind()` (KE `assets/backdrop.cc`, tutorial `04_render_textures`) | bake into a target | render boundary (§5.3) |
| simplex adapter (`application_adapter.cc`, `canvas.bind()`) | persistent canvas target | simplex's single layer renders into the persistent canvas; contract unchanged |
| brix `view.cc:177` `SDL_RenderReadPixels`, `game_board_painter.cc` `SDL_SetRenderTarget` | raw SDL, outside neutrino | ported to a layer pass + mesh (§7.1) |

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
  painting from an atlas that no longer exists is an ENFORCE at the call site, never a
  dangling pointer.

### 8.2 Later: replacement and group release

Two mid-run cases are realistic; both are deferred until a user needs them, and neither
changes the painting API:

| Need | Cases | Mechanism |
|---|---|---|
| **replace in place** | glyph atlases re-rasterised for a new size (DPI change, UI scale, window resize with scaled text); graphics settings switching the art set (classic / upscaled); hot reload (`atlas_cache::reload`) | rebuild the pages under the **same** `atlas_id`; addresses stay valid as long as picture indices keep their meaning (or the producer, e.g. text, re-resolves them) |
| **group release** | content generated per game session (warlords player-colour armies, per-scenario minimap / terrain) in a "new game" loop; long-running tools (map viewer, editor) opening map after map; large HD art on memory-limited platforms | atlases registered into a group; an internal `release(group)` at a point where nothing uses it (between games, on scene switch); stale use is caught by §8.1's never-reused ids |

Deferred release of GPU objects until the frame has rendered (§3.3) is still needed for
textures a game owns and may destroy (KE's baked backdrop, render targets) and for the two
mechanisms above.

### 8.3 Device reset

SDL reports three render events (`SDL_events.h`):

| Event | Meaning | Painter response |
|---|---|---|
| `SDL_EVENT_RENDER_TARGETS_RESET` | render targets lost their *contents* | layer pass targets: nothing (re-rendered every frame); baked targets (`compose()`): owners must re-bake |
| `SDL_EVENT_RENDER_DEVICE_RESET` | **all textures must be recreated** | re-upload every registered atlas; recreate pass targets; owners of game textures must re-create / re-bake |
| `SDL_EVENT_RENDER_DEVICE_LOST` | device gone, unrecoverable | report and shut down cleanly |

Device reset happens with D3D device loss and can happen on mobile (warlords has an
Android build); neutrino handles none of these events today.

Consequences:

- **The registry keeps each atlas's CPU pages after upload**, so re-upload needs no
  loader. 2-D atlases at this scale are small; the same copy serves CPU-side reads
  (warlords's minimap, picture pixel queries, `make_bitmask`). An atlas may opt out with a
  rebuild function instead (large art), decided when one does.
- **Game-owned textures** (baked `compose()` results, streaming textures) cannot be
  rebuilt by the engine: the engine raises a scene-level notification (e.g.
  `on_textures_lost()`) and the owner re-bakes. Streaming textures refilled every frame
  need nothing.
- **Glyph atlases** are the exception: they are rebuilt from their glyph source, with no
  CPU copy (§9.5).
- Test: a forced device-reset path (re-upload everything, then pixel-compare a frame).

## 9. Text

Text is a **producer**: layout turns a string into glyph quads, painted like any other
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
`blit(atlas, src, x, y)` callback that maps directly onto painting a quad).

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
     painter: paint_text(…), measure / wrap / caret          CPU: render_text(image&, …)
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

// layout and painting (§9.6)
text_params, text_align, paint_text(…), measure(…), wrap(…), offset_of(…), index_at(…)
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
| vector strokes | BGI, Microsoft Vector | scalable line strokes | onyx `vector_font`, rasterised at size (scalable path); painting strokes as line geometry is a possible later option |
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
  leak its own comments call a bug. Per-command tint avoids it.)*
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

### 9.6 Painting text

```cpp
struct text_params {                  // (FC: FC_Effect — alignment, scale, colour)
    color      colour{colors::white}; // tint; ignored by baked-colour variants
    text_align align{text_align::left};
    float      scale{1.0f};           // integer for pixel fonts (T3)
    float      letter_spacing{0}, line_spacing{0};
};
rect paint_text(position, std::string_view, const font&, text_params = {}, depth = {});
rect paint_text(const rect& box, std::string_view, const font&, text_params = {}, depth = {});  // wrapped
```

- One quad per glyph through the layout callback; resolution happens at record like every
  other quad (§3.3), so a glyph rasterised during layout is registered before its command
  is recorded.
- **Returns the bounds** of what was painted — for UI layout, hit areas and dirty rects
  *(FC: every draw returns its rect)*.
- **Measurement and caret helpers** on `font`: `measure`, `wrap`, and index ↔ position
  (`offset_of(text, index)`, `index_at(text, point)`) for text input. onyx's layout has
  measure and wrap; the caret pair is new *(FC: `FC_GetCharacterOffset`,
  `FC_GetPositionFromOffset`)*.
- **No printf-style formatting** in the API *(FC has it on every call)*; callers use
  `std::format`.
- **Nuklear (postponed).** Nuklear's output is already a vertex / index list with a texture
  per command (`nk_convert`), so when it comes, a Nuklear backend is a producer painting
  `paint_geometry` with atlas addresses, glyph atlases included (RM8).
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
| sorted paint list | `sprite_batch` (`layer`, `depth`, stable) | quads from visuals only; no tint/blend; resolves at flush, not at record |
| quad placement | `draw.cc` (snapping, pivot, flip, diagonal, rotation) | no tint, crop or fit; parameters are `sprite_draw_params` |
| geometry | `draw.cc` uses `render_geometry` internally | not a command; primitives are immediate with global colour/blend state |
| meshes + effects | lib_sdlpp `simplex::sprite_mesh`, `mesh_effect`s | not in neutrino; renders directly; uv vs whole texture; white vertices |
| layers | `render_layer`, `world_compositor`, `layer_view` | no pass / composite |
| render targets | `texture` (`target` access), `bind()` / `compose()`, `target_scope` | not wired to layers |
| blend modes | `blend_mode` (sdlpp): none, blend, premultiplied, add, mod, mul | global state, not per command |
| deferred release | — | needed (§3.3, §8.2) |
| atlas lifetime | `register_atlas` / `unregister_atlas` public; CPU pages dropped after upload | internal registration; keep CPU pages (§8) |
| device reset | — (no handler for the three render events) | re-upload, notify owners (§8.3) |
| text | onyx_font `bitmap_font` / `glyph_cache` / `text_renderer`; each game bakes and draws its own (brix tilesets, warlords white atlas for Nuklear, KE BIOS glyphs) | `font` = source + atlas + layout; growing, pre-packed atlases; filter per atlas; `paint_text` (§9) |

## 11. Open decisions

- **R1 — paint list per layer or per frame.** Per layer keeps layers independent and maps
  directly onto passes. Proposed: per layer.
- **R2 — geometry storage.** The painter owns a per-frame vertex/index arena; commands
  hold ranges into it. Proposed: yes.
- **R3 — quad transform.** Explicit scale + rotation + flip vs a general 2×3 matrix.
  Anything beyond the explicit fields is a mesh. Proposed: explicit fields.
- **R4 — white texel.** One reserved white picture per atlas so untextured geometry shares
  a page with quads. Only if measurements show primitives breaking runs. Proposed: defer.
- **R5 — composite.** Composite = mesh + ordered effects + blend + colour/alpha mod
  (§5.2). Proposed: that, nothing more.
- **R6 — names.** Settled: `painter`, painting / rendering, `layer`, `paint_list`,
  `quad_params`, `flip`, `fit`, `mesh`. Open: the public entry point (`painter.layer(id)`
  vs a paint list handed to `render_layer::draw`), and whether `world_compositor` is folded
  into the painter.
- **R7 — `set_draw_color` and friends.** Keep as record-time defaults, or remove in favour
  of explicit arguments. Proposed: keep for `draw.hh` compatibility, document as defaults.

Text decisions T1–T4 are in §9.7; facade decisions F1–F8 in §9.3.

## 12. GPU backends (deferred)

SDL_Renderer gives textured triangles, per-vertex colour, blend modes (including additive
and custom) and render targets — enough for every case in §7 and every mesh effect in §6.
Per-pixel effects (distortion, bloom, lighting) need shaders. The build uses SDL 3.4.0,
which offers two routes; both change only rendering (§4):

| Route | Gains | Costs |
|---|---|---|
| SDL_Renderer on the GPU backend (`SDL_CreateGPURenderer`) + `SDL_GPURenderState` (custom fragment shader + uniforms per draw) | everything above, plus fragment shaders as an optional **material** on a command or composite | needs Vulkan / D3D12 / Metal; shaders shipped per backend (SPIR-V / DXIL / MSL via SDL_shadercross); fragment stage only; web (brix's Emscripten build) unverified |
| Raw SDL_GPU (pipelines, passes, compute) | full control: own vertex formats, instancing, GPU particles | batching, text and primitives written from scratch |

Proposed: no material field until a game needs a per-pixel effect; when one does, the
first route, behind the same painting contract.

## 13. Out of scope

- **Particle system** — a producer; designed when a game needs one.
- **Scene graph / transform hierarchy** — a gameplay convenience, not a painter need. Flat
  layers plus the tilemap's implicit spatial grid are enough for 2D (Gregory §11.2.7.5:
  choose from measured need).
- **Culling beyond the tilemap** — per-layer bounds checks until measured otherwise.

## 14. Milestones

- **RM1 — Painting quads.** Paint list per layer with `(layer, depth, seq)`; `paint`
  resolving `atlas_address` at record; `quad_params` with tint and crop; `paint_picture`
  with fit modes; per-command blend; rendering that batches same-page runs; deferred
  resource release. Preserve every placement rule of §3.2 (pixel snapping, pivot, flip,
  diagonal, rotation identical at 0°, `DRAW_CHECK`). `sprite_batch` and `world_renderer`
  move onto it. Tests: the relevant `test_draw.cc` cases on the new entry points; pixel
  tests for tint, crop and each fit mode. Exit: KE, Luxor, tutorials pixel-identical;
  `test_draw` / batch tests green.
- **RM2 — Painting `draw.hh`.** `draw.hh` calls record instead of drawing; `compose()` and
  `capture_frame()` / `to_image()` become render boundaries; record-time defaults for
  colour/blend (R7). Exit: simplex and all tutorials unchanged visually.
- **RM3 — Geometry and primitives.** `paint_geometry`; primitives tessellated into geometry
  so they sort with quads. Exit: a brix-style beam and cursor demo in one layer.
- **RM4 — Meshes and effects.** Port `sprite_mesh` (as `mesh`) + `mesh_effect`s from
  lib_sdlpp simplex with the changes in §6; `paint_mesh` on atlas pictures. Exit: each
  effect has a pixel test; subdivided `rotate_3d` shows no seam.
- **RM5 — Layer passes.** Target + composite mesh per layer; pass ordering. Exit: board
  rotation (brix-style) with no readback.
- **RM6 — Atlas lifetime and device reset.** `register_atlas` internal; CPU pages kept;
  handlers for the three render events; re-upload of atlases and pass targets;
  `on_textures_lost()` for game-owned textures. Exit: a forced device reset re-renders a
  KE frame pixel-identically.
- **RM7 — Text.** Planned in its own document, [`font_plan.md`](font_plan.md) (phases A–D,
  milestones FM0–FM7). Its original exit criteria: KE's BIOS text, a brix-style
  baked-colour font, an indexed-colour glyph-bank font with per-glyph Y offsets
  (Westwood-style, test data) and a TTF label paint through the painter; a forced device
  reset and a forced font replacement keep all of them; no game includes `onyx_font/…`.
- **RM8 — Nuklear as a producer (postponed).** A Nuklear backend that paints
  `nk_convert` output as geometry with atlas addresses (glyph atlases included), and an
  engine `nk_user_font` over `font`. Exit: a warlords-style UI panel sorts with world
  layers.

Dependencies: RM1 → RM2; RM1 → RM3 → RM4 → RM5; RM1 → RM6 → RM7 (text uses RM6's
device-reset handling and replacement; `font_plan.md` §5 has the detail — its Phase A
runs before the painter); RM3 + RM7 → RM8 (postponed). Sprite plan P3.7: its M3 (painting
sprites) needs RM1; its M1 / M2 do not depend on the painter.

## References

- J. Gregory, *Game Engine Architecture*, 3rd ed.: §11.2.7.2 primitive submission and
  state leaks; §11.2.7.3 geometry sorting (translucency forces back-to-front); §11.2.7.5
  choosing a scene graph; §11.4 effects and overlays; §12.1.1 cel / sprite animation.
- C. Ericson, "Order your graphics draw calls around!" (2008) — packed sort keys.
- SDL 3.4 headers: `SDL_gpu.h`; `SDL_render.h` (`SDL_CreateGPURenderer`,
  `SDL_GPURenderState`).
- lib_sdlpp: `include/simplex/mesh.hh`, `include/simplex/effects.hh`,
  `examples/simplex/example_animation.cc`.
