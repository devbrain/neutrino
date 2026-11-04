# Images, atlases and sprites — the vocabulary and the pipeline

This document is the reference for everything between "bytes of a picture file" and
"`draw_sprite` puts it on screen": pixels, palettes, decoders, atlases, GPU upload, sprite
sheets, animations, the asset cache and the world tile path. It fixes **one meaning per
word** and says which type owns what.

It describes the **target** vocabulary agreed during P3.6 (see
`neutrino_ergonomics_roadmap.md` §3.6). Where the code still uses an older name, the
[rename table](#7-renames-current--target) lists it; until those land, read the old name
through that table.

The **sprite layer** (everything after GPU upload) is being redesigned; the decisions so
far and the open points are in [§9](#9-sprite-layer-redesign). Sections 3.5–3.9 describe
the *current* sprite types, which §9 replaces.

---

## 1. The pipeline at a glance

```
                      file bytes
                          │
     ┌────────────────────┴─────────────────────┐
     │ one picture                              │ many pictures
     │ onyx codecs: load_image / decode_image   │ image_bank_decoder
     │ or a game parser using image_builder     │ (game parser using image_bank_builder)
     ▼                                          ▼
   image ──── crop / slice ────────────────► image_bank {entries: image + id}
                                                │
                                                │ build_atlas          (CPU, load time)
                                                ▼
                                          image_atlas {atlas_id, pictures 0..n-1}
                                                (storage: pages + one placement
                                                 {page, rect} per picture)
                                                │            ▲
        LOAD: the game keeps atlas_addresses    │            │ with_palette (recolour)
        {atlas, picture_idx} and defines        │
        sprites from them                       │ register_atlas(std::move(atlas))  -- consumes it
                                                ▼
 ───── registry: atlas_id → {page textures, placements} ─────  GPU, registry-owned
                                                │
                     sprite = frames {address, duration, flip} (+ loop); a still = 1 frame
                                                │
                     sprite_player (per actor playhead) ──► draw_sprite(position, …)
                                       address → registry.get(atlas, picture_idx) → {texture, rect}
```

An atlas is a **list of pictures**: picture `i` is bank entry `i`. Pages are how the
pictures are stored, not how they are named. Loading never touches the GPU: an address
`{atlas, picture_idx}` is valid from the moment `build_atlas` returns, because the atlas's
`atlas_id` is assigned then. `register_atlas` later consumes the CPU atlas and keeps its
page textures and placements under that same id, so every address taken during loading
becomes drawable.

The sprite half of the diagram is the **target** of [§9](#9-sprite-layer-redesign). Today
the path from upload to screen still goes through `sprite_sheet` / `sprite_visual` /
`sprite_animation` / `sprite_state`, with two front doors that build it for you:

- **Sprites**: `sprite_def` → `build_sprite_set` → `sprite_set`, cached by `sprite_cache`.
- **Tiles**: `world_tileset` → `build_bundle` → `tileset_bundle`, cached by
  `resource_cache`.

Both are facades over one `render_bundle` that owns the registered ids.

---

## 2. Reserved words

Each word below means exactly one thing. A new type that does not match the meaning must
pick another word. *Sheet, visual, appearance, clip/animation, state, instance, def* and
*set* name the current sprite types; [§9](#9-sprite-layer-redesign) retires most of them.

| Word | Means | Never means |
|---|---|---|
| **image** | CPU pixels of any format (`neutrino::image` = `sdlpp::surface`), however they were loaded. INDEX8 images carry a palette and a colour key. | a file, a source descriptor |
| **builder** | A convenience for writing pixels (`image_builder`) or authoring a def (`sprite_def_builder`). Not a type of pixels. | — |
| **bank** | Many images, each with the game's id (`image_bank`). | registered resources |
| **palette** | Up to 256 colours for indexed images; refcounted by SDL, so many images share one. Recolouring = swapping the palette. | — |
| **decoder** | A function from bytes to pixels: `image_decoder` (onyx) → `image`, `image_bank_decoder` → `image_bank`. | — |
| **picture** | One entry of an atlas, numbered by position: picture `i` is bank entry `i`. `picture_count()`, `picture_size(i)`. | a page; a sprite |
| **address** | Which picture: `atlas_address {atlas, picture_idx}`. The handle the game keeps; valid from load time, drawable once its atlas is registered. | a rect; a page |
| **region** | An address plus the game's id (`atlas_region`), as returned by `region(i)` / `find_by_id`. | an animation step |
| **placement** | Where a picture is stored: `{page, rect}`. Internal to the atlas and the registry; the game never holds one. | an address |
| **atlas** | An indexed list of pictures (`image_atlas`), identified by an `atlas_id` assigned when it is built. Short-lived on the CPU: load, define sprites, upload. | one page; a GPU object; a JSON descriptor |
| **page** | One image of an atlas = one GPU texture. Storage, not addressing: an index inside a placement. | — |
| **texture** | GPU pixels: `texture` (render targets, streaming) or a registry-owned atlas page (reached through an address). | CPU pixels |
| **sheet** | `sprite_sheet`: named/indexed visuals over one uploaded texture. | a CPU image |
| **visual** | One drawable picture: texture rect + pivot + trim (`sprite_visual`). | — |
| **appearance** | A visual + flip + visibility: *what* to draw, statically. | — |
| **sprite** *(target)* | A sequence of frames plus a loop flag. A still picture is a sprite with one frame; there is no separate animation type. | a playhead; a texture |
| **player** *(target)* | A per-actor playhead over a sprite (`sprite_player`). | the sprite itself |
| **frame** | **One timed step of a sprite** (`sprite_animation_frame`, `sprite_frame_def`, `world_image_frame`; target: `{region, duration, flip}`). | a region, a bank entry, a visual |
| **clip** / **animation** | A sequence of frames. *Clip* is the authored, named form (`sprite_clip_def`); *animation* the registered one (`sprite_animation_id`). | — |
| **state** | A runtime playhead (`sprite_state_id`): a fixed appearance or a cursor in an animation. | — |
| **instance** | A per-entity state plus a lease on its set (`sprite_instance`). | — |
| **def** | A pure, hashable, GPU-free description (`sprite_def`). | — |
| **set** / **bundle** | Built, registered resources owned as a unit (`sprite_set`, `tileset_bundle`, base `render_bundle`). | a bank of CPU images |
| **id** | The game's key for a picture (`image_bank::entry::id`, `atlas_region::id`): opaque to the engine, may repeat. Engine handles are strong types named `*_id` (`atlas_id`, …) and never mix with it. | — |

---

## 3. Layer by layer

### 3.1 Pixels — `video/image/image.hh`, `video/image/image_builder.hh`, `video/image/image_bank.hh`, `video/palette.hh`

| Name | What |
|---|---|
| `image` | `sdlpp::surface`. Any pixel format; the one CPU pixel type. |
| `pixel_format` | `sdlpp::pixel_format_enum`. |
| `palette` | Up to 256 colours. `load_palette(bytes, palette_encoding::{rgb8, vga6})`. |
| `image_builder` | Writes pixels without locking, pitch or format code. |

```cpp
class image_builder {
    static image_builder indexed(dim, const palette&, std::optional<uint8_t> transparent_index = {});
    static image_builder rgba(dim);
    void set(int x, int y, uint8_t index);      // indexed
    void set(int x, int y, color c);            // rgba
    std::span<uint8_t> row(int y);              // fast path for planar / RLE decoders
    image build() &&;
};
```

An indexed builder produces an INDEX8 image that references the palette and carries
`transparent_index` as the surface's colour key. An RGBA builder produces RGBA32.

**There is no second pixel type.** An image decoded by onyx and one built by a game parser
are the same thing; only the way they were loaded differs. Convenience for parsers is the
builder's job.

### 3.2 Decoding — `video/image/image_loader.hh`

The only split is **one picture vs. many**:

| Shape | API | Result |
|---|---|---|
| **One picture**, standard or magic-detected format (PNG, BMP, PCX, …) | `load_image(path / bytes)`, `decode_image(bytes, image_decoder)`, `register_image_decoder` | `image` |
| **One picture**, game format | a game function using `image_builder` | `image` |
| **Many numbered pictures** in one file (KE BOBs, brix blocks and font, CC tiles) | `image_bank_decoder` = `std::function<image_bank(span<const uint8_t>, const image_decode_options&)>` | `image_bank` |

```cpp
// image/image_bank.hh
struct image_bank {
    struct entry { image pixels; uint32_t id = 0; };    // id = the game's key (usually the file's number)
    std::vector<entry> entries;
};
```

`image_decoder`, `image_sink`, `image_decode_options` and `image_decode_result` are onyx
types re-exported for sniffable single-image formats; game code rarely touches them.

Rules:

- A decoder **returns a value and throws on corrupt input**. There is no sink protocol and
  no streaming.
- It produces images in the file's **native** format: indexed files give INDEX8 images
  sharing one palette.
- **Transparency** has exactly two sources: alpha 0 (RGBA), or the colour key of an INDEX8
  image. A file with a separate mask plane folds it in: it reserves a free index as the
  key, or builds RGBA images when the palette is full.
- An entry carries **pixels and the game's id only**. Pivots, hit boxes and names are
  side tables keyed by id, returned by the decoder next to the bank and read by the layer
  that needs them (the sprite layer).
- **The id is the game's key.** Every entry gets the id its decoder passes
  (`image_bank_builder::add(img, id)` has no default): a block number, a character code,
  a cell index. Ids may be sparse and may repeat; the engine never interprets them. There
  are no empty placeholder entries: a file with blocks 1, 2, 5 gives three entries.

`load_atlas(bytes, decoder, opts)` = decode + `build_atlas`.

### 3.3 Atlas — `video/atlas/`

```cpp
class atlas_id;                   // atlas/atlas_id.hh: strong type, minted by build_atlas

struct atlas_address {            // which picture -- known at load time, before any GPU
    atlas_id    atlas;
    std::size_t picture_idx;      // == bank position
};

struct atlas_region : atlas_address {
    uint32_t id;                  // the game's id, carried from the bank entry
};

class image_atlas {
    atlas_id    id() const;                                     // assigned by build_atlas, kept on move
    std::size_t picture_count() const;                          // == bank size
    dim         picture_size(std::size_t i) const;
    const atlas_region&         region(std::size_t i) const;
    std::optional<atlas_region> find_by_id(uint32_t id) const;  // first match
  private:                        // storage, read only by build_atlas and the upload
    std::vector<image>           pages;
    std::vector<atlas_placement> placements;                    // {page, rect}, placements[i] == picture i
};

image_atlas build_atlas(const image_bank&, std::optional<pixel_format> = {},
                        std::optional<dim> max_page = {});
image_atlas build_atlas(const image&, std::optional<pixel_format> = {});   // one picture, one page
image_atlas with_palette(const image_atlas&, const palette&);
```

| Header | Contents |
|---|---|
| `bitmask.hh` | `bitmask`: 1 bit per pixel, `overlaps(other, dx, dy)`; `make_bitmask(image, rect)` builds one on demand. |
| `atlas_id.hh` | `atlas_id`. |
| `image_atlas.hh` | `atlas_address`, `atlas_region`, `image_atlas`, `build_atlas`. |
| `atlas_builder.hh` | `build_atlas`, `with_palette`. |

Rules:

- **An atlas is a list of pictures; pages are storage.** The packer cuts the pictures into
  pages that fit the GPU's texture limit, but nobody addresses by page: a different page
  limit would move pictures between pages without changing any address.
- **The address is `{atlas, picture_idx}`.** The game stores it from load time on and
  draws it after upload. The rect is not in the address: it lives in the atlas's
  placement table (and, after upload, in the registry), which can bounds-check a bad or
  stale address. Picture `i` is bank entry `i`, whatever page it landed on.
- **Placements are one flat table.** `placements[i] = {page, rect}` for every picture, so
  a lookup is O(1) however many pictures each page holds.
- **Sizes.** `picture_size(i)` reads the placement; it needs no renderer, so sprite
  definitions and headless tools use it before upload.
- **The id is the game's key.** `find_by_id(id)` returns the region carrying it, or
  nullopt; ids are opaque and may repeat, in which case the first region (lowest
  position) wins. Formats whose numbers are their positions (brix tiles) use
  `region(i)` directly.
- **The CPU atlas is short-lived.** Load, define sprites from its addresses, upload. The
  upload consumes it, so its rects are never kept twice.
- **The atlas id is minted at build time** from an engine-wide counter (no renderer
  needed) and is preserved by moves. It is a `details::id_strong_type`, like every other
  engine handle.
- **Output format.** `indexed8` keeps indices and requires every entry to be INDEX8 with
  the same palette (the same refcounted SDL palette, or equal colours), else throws.
  `rgba` expands. `automatic` picks indexed when the bank qualifies, RGBA otherwise.
- **Compositing is a blit.** Each page is first filled with the colour key (indexed) or
  alpha 0 (RGBA), so keyed pixels stay transparent.
- **No collision masks.** The atlas stages pixels for upload; nothing reads a mask from it,
  and the CPU atlas is dropped after upload. A game that wants pixel-perfect collision
  calls `make_bitmask(image, rect)` on its bank images and keeps the masks in its own
  side table, keyed by id.
- **Placement is private** to `build_atlas`: shelf packing (sort by height, fill rows),
  page width ≈ √(total area), clamped to [widest picture, max texture size]; a new page
  starts at the height cap; each page is trimmed to its used extent. Pictures of one size
  keep their input order. A picture larger than a page throws. There is no public packer
  and no layout parameter.
- **Recolouring.** `with_palette` swaps the palette of every indexed page: one brix font
  sheet + six palettes, not six sheets.
- **Slicing is not decoding.** One image with an externally known layout (a uniform grid)
  is sliced by `sprite_grid`/explicit rects in the sprite layer, without repacking.

### 3.4 GPU upload — `video/sprite/texture_atlas.hh`

| Name | What |
|---|---|
| `register_atlas(image_atlas&&, atlas_texture_format = automatic)` | **Consumes** the CPU atlas: uploads every page and moves the placement table into the registry, **under the atlas's existing `atlas_id`** (returned for convenience). Every address taken from the atlas during loading becomes drawable; nothing is repacked. |
| `unregister_atlas(atlas_id)` | Releases the atlas's textures and placements. |
| `picture_size(atlas_id, i)` | A picture's size after upload (before upload: `image_atlas::picture_size`). |
| `draw_sprite(pos, atlas_address)` *(planned)* | `registry.get(atlas, picture_idx)` → `{page texture, rect}`: one hash lookup by atlas id, then two indexed reads. |

- **One handle per atlas, pages internal.** The registry is
  `unordered_map<atlas_id, gpu_texture_atlas>`; a `gpu_texture_atlas` holds the page
  textures and the placement table. The id is minted by `build_atlas`, not by a registry
  slot.
- **The registry owns the geometry.** Without rects a texture registry could not draw
  anything, so the rects live there for the life of the atlas, and one query returns
  texture and rect together. Because the address is an index, the registry rejects an
  out-of-range or stale address, and an atlas could be reloaded under the same id.
- Drawing an address whose atlas is not registered (yet, or any more) fails through
  `DRAW_CHECK`, like any other bad draw *(planned)*.

### 3.5 Sprite sheet and visuals — `video/sprite/sprite_sheet.hh` (current; replaced by §9)

| Name | What |
|---|---|
| `sprite_visual` | `texture_rect` + `origin` (pivot, packed space) + `source_size` / `trim_offset` (untrimmed geometry). `metrics()` resolves them into `sprite_metrics`. |
| `sprite_sheet` | Named/indexed visuals over **one** `atlas_texture_id`. Built by hand or from one atlas page. |
| `sprite_sheet_id` | Registered sheet (`register_sprite_sheet`). |
| `sprite_visual_id` | A visual inside one sheet. |
| `sprite_visual_ref` | `{sheet, visual}`: a visual addressable anywhere. What everything above this layer stores. |
| `find_visual(ref)` | The geometry of a registered visual. |

**Pivots live here.** A decoder never supplies an origin; the sprite layer does, by
id (`sprite_def_builder::add_visual(name, rect, origin)` or a side table keyed by the
region's id).

### 3.6 Appearance, animation, state — `sprite_appearance.hh`, `sprite_animation.hh`, `sprite_state.hh` (current; replaced by §9)

| Name | What | Owns time? |
|---|---|---|
| `sprite_flip` | Horizontal / vertical / diagonal flags; compose with `^`. | no |
| `sprite_appearance` | Visual + flip + visible. | no |
| `sprite_animation_frame` | Appearance + duration. **The** frame. | no |
| `sprite_animation` / `sprite_animation_id` | Ordered frames + loop flag; registered, immutable. | no |
| `sprite_state_id` | Playhead: a fixed appearance or a cursor in an animation. The application advances it each update. | **yes** |

`create_sprite_state`, `switch_sprite_animation` (keeps elapsed time if already playing),
`restart_sprite_animation` (always from zero), `sprite_state_finished`.

### 3.7 The sprite asset path — `sprite_def.hh`, `sprite_def_builder.hh`, `sprite_set.hh`, `sprite_cache.hh` (current; replaced by §9)

| Name | What |
|---|---|
| `sprite_def` | Pure description: a `world_image` source + optional `sprite_grid` + named `sprite_visual_def`s + `sprite_clip_def`s. Hashable (`key_for`). |
| `sprite_grid` | Uniform slicing into visuals named `"0".."N-1"`, with an origin rule. |
| `sprite_visual_def` | Name + source rect + pivot (+ trim). |
| `sprite_clip_def` / `sprite_frame_def` | Named animation / one timed step referencing a visual by name. |
| `sprite_def_builder` | Fluent authoring; also `from_aseprite_json`. |
| `build_sprite_set(def)` → `sprite_set` | Uploads the image, makes one sheet, registers one animation per clip. Lookups by name or index. |
| `sprite_cache` / `sprite_set_handle` | Content-keyed, refcounted cache; RAII lease. |
| `sprite_instance` | Per-entity state + lease (`handle.spawn("walk")`). |

### 3.8 The tile path — `video/world/tileset_bundle.hh`, `resource_cache.hh`

`build_bundle(world_tileset)` crops each tile out of the tileset image(s) into an
`image_bank`, calls `build_atlas`, registers every page, makes one sheet per page, and
maps each `world_local_tile_id` to a `sprite_visual_ref` (plus a shared state for animated
tiles). `resource_cache` / `bundle_handle` cache it by content.

### 3.9 Ownership — `render_bundle`

`render_bundle { atlases[], sheets[], animations[], states[] }` owns registered ids and
releases them in dependency order: **states → animations → sheets → textures**.
`sprite_set` (by name) and `tileset_bundle` (by tile id) are lookup facades over it.

---

## 4. Which path do I use?

| I have… | Use |
|---|---|
| One PNG with a uniform grid of sprites | `sprite_def_builder().from_file(…).with_grid(…)` → `acquire_sprite` |
| A PNG + Aseprite JSON | `sprite_def_builder::from_aseprite_json(…)` → `acquire_sprite` |
| A custom file with one picture | a parser using `image_builder` → `image` → `from_surface` in a `sprite_def` |
| A custom file with many pictures | an `image_bank_decoder` → `load_atlas` → `register_sprite_sheet` per page; pivots from the decoder's side table |
| An indexed sheet in several colourways | `build_atlas(bank, indexed8)` once, `with_palette` per colourway |
| A Tiled map | the world path; `world_renderer` builds bundles for you |
| A procedural picture | `image_builder` (or any `image`) → `image_from_surface` in a `sprite_def` |
| Something drawn every frame | a `texture` render target / a `render_layer`, not an atlas |

---

## 5. Identity: how a picture is named

| Key | Scope | Who assigns | Example |
|---|---|---|---|
| **`id`** | one bank / one atlas | the game (usually the file's number); opaque, may repeat | brix sprite number 0x42 |
| **`atlas_address`** | the whole program | `build_atlas` (atlas id + picture position) | what sprites, side tables and the draw call hold |
| **position** `i` | one bank / one atlas | order of bank entries; it is the address's `picture_idx` | `region(i)` for formats numbered by position |
| **name** | one `sprite_def` / `sprite_set` (current) | the author | `"walk_3"` |

The game connects its ids to addresses at load time (`find_by_id`, or `region(i)`); the
engine connects addresses to GPU textures and rects at upload (`atlas_id` → pages +
placements). Neither side needs the other's key.

---

## 6. Transparency and masks

| Source | Where it lives | Gives |
|---|---|---|
| RGBA alpha | pixel alpha = 0 | transparency + exact bitmask |
| Indexed colour key | INDEX8 image's colour key (`image_builder::indexed(…, transparent_index)`) | transparency + exact bitmask; survives palette swaps |
| Colour key on an RGB image | `world_image::transparent_color` | transparency + bitmask |

Masks are not stored anywhere by the engine: `make_bitmask(image, rect)` computes one from
either transparency source when a game asks for it.

---

## 7. Renames (current → target)

Status: **done** = in the working tree, **planned** = agreed, not yet applied,
**proposed** = needs a decision.

| Current | Target | Status |
|---|---|---|
| `video/sprite/{bitmask,atlas_packer,cpu_texture_atlas}.hh` | `video/atlas/{bitmask,atlas_packer,image_atlas}.hh` | done (uncommitted) |
| `cpu_texture_atlas` → `image_atlas` (one page) | `image_atlas`: a list of pictures over **all pages** | done (uncommitted) |
| `packed_atlas`, `frame_ref`, `locate` | merged into `image_atlas` (`placements[i].page`, private) | done (uncommitted) |
| `atlas_frame` (`atlas_rect`, `mask`) | `atlas_address {atlas, picture_idx}` + `atlas_region` (address + `id`); rect in the private `atlas_placement {page, rect}` — no mask | done (uncommitted) |
| `image_atlas::surface()`, `frame()`, `frame_count()`, `add_frame()` | `region(i)`, `picture_count()`, `picture_size(i)`; pages private | done (uncommitted) |
| `image_atlas_old` | removed | done (uncommitted) |
| `atlas_frame::evaluate_bitmask`, `atlas_mask_options` | `make_bitmask(image, rect)` in `bitmask.hh`, on demand | planned |
| `atlas_packer.hh`, `pack_*`, stb_rect_pack | removed; placement private to `build_atlas` | files deleted (uncommitted; `image_loader.hh`, `tileset_bundle` still to port); stb_rect_pack planned |
| frame-image draft (`indexed_image`, `rgba_image`, `frame<T>`, `*_frames`, `decoded_frames`, `to_*_image`, `frame_image.hh`) | `image` + `image_builder` + `image_bank` | planned |
| `atlas_decoder` | `image_bank_decoder` | planned |
| `load_image_atlas` | `load_atlas` | planned |
| `atlas_output` (draft) | `atlas_texture_format` | planned |
| `gpu_texture_atlas_id` (one page) | `atlas_id`: one per atlas, minted by `build_atlas`; the per-page `atlas_texture_id` is dropped | done in the registry (uncommitted); sprite layer still to port |
| `register_atlas(const image_atlas_old&, format)` → id | `register_atlas(image_atlas&&, format)`: consumes the atlas, keeps textures + placements under its id | done (uncommitted) |
| `gpu_texture_atlas {texture, format, width, height}` | `gpu_texture_atlas {page textures, placements}`; `texture_registry::get(atlas, picture_idx)` → `{texture, rect}` | done (uncommitted) |
| `image_atlas::find_by_id` → `optional<size_t>` | → `optional<atlas_region>` | done (uncommitted) |
| `video/sprite/detail/id_strong_type.hh` | `neutrino/detail/id_strong_type.hh` (atlas sits below sprite); `operator<<` for every id | done (uncommitted) |
| `image_bank_builder::add_empty`, default `id = 0` on `add` | removed; `add(img, id)` requires the id | done (uncommitted) |
| `atlas_sink`, `frame_info`, `frame_format`, `atlas_layout`, `decode_image_atlas` (draft in `image_loader.hh`) | removed | planned |
| sdlpp `surface`: no colour key | `set_color_key` / `color_key` | planned |
| `load_aseprite_atlas`, `sprite/atlas_loader.hh` | `load_aseprite_sprite_def`, `sprite/aseprite_loader.hh` | proposed |
| `sprite_set::frame_rect`, `require_frame_rect` (and on `sprite_set_handle`) | `visual_rect`, `require_visual_rect` — it is a visual's rect, not an animation frame | proposed |
| `sprites.hh` "Vocabulary" section | rewritten to match this document | planned |

---

## 8. Open questions

1. ~~**`id` on bank entries and regions vs the `*_id` handles.**~~ Decided: `id` is the
   game's opaque key; engine handles are `*_id` strong types (`atlas_id`), and the region
   field holding one is named after it (`atlas`), so they never read alike.
2. **`world_image`** is a *source descriptor* (disk / memory / surface + declared size +
   colour key), not pixels. It lives in `world/` and is shared by sprites. Rename to
   `image_source`, or leave it as world vocabulary?
3. **Tile vs sprite cache naming.** `resource_cache` / `bundle_handle` / `tileset_bundle`
   vs `sprite_cache` / `sprite_set_handle` / `sprite_set`. `tile_cache` /
   `tileset_handle` would make the parallel obvious; low priority.
4. **Do the onyx decoder types stay in `image_loader.hh`?** Moving `image_decoder`,
   `image_sink`, `image_decode_result` and `register_image_decoder` to a separate
   `video/codec.hh` would leave `image_loader.hh` as `load_image`, `image_bank_decoder`,
   `load_atlas`.

---

## 9. Sprite layer redesign

Status: **under discussion**. Decisions below are agreed; open points are listed at the end.

### 9.1 What is wrong today

A review of `video/sprite/` against its real users (KE, Luxor, `sprite_demo`):

- **~30 public nouns** for one job, with synonyms: *frame* means an atlas rect
  (`frame_rect`), an animation step and a grid cell; *visual / appearance / metrics* are
  one picture; *clip / animation* are one sequence; *origin / pivot / logical_pivot* one
  anchor; *sheet / set / bundle / def* one group of pictures.
- **Two parallel worlds**: registered ids (`sprite_sheet_id`, `sprite_visual_id` with a
  hidden owner, `sprite_visual_ref`, `sprite_animation_id`, `sprite_state_id`) and
  string-keyed defs (`sprite_def`, `sprite_visual_def`, `sprite_clip_def`,
  `sprite_frame_def`, `sprite_grid`), plus a third layer of handles (`sprite_cache`,
  `sprite_set_handle`, `sprite_instance`).
- **Animations reference uploaded visuals**, so they cannot be defined before upload.
- **One pre-packed image per def** (`sprite_def::image`); a bank of decoded pictures does not
  fit. KE packs itself, wraps the result as `image_from_surface` and names visuals
  `to_string(i)` to look them up by index.
- **Playheads are not RAII** on the common path: KE creates and unregisters raw
  `sprite_state_id`s by hand.
- **Unused in games**: Aseprite loading, trim, `bounding_size`, `sprite_instance` (demo only),
  content-key dedup; `sprite_set` has 17 lookups, all repeated on `sprite_set_handle`.

brix (`~/proj/ares/brix`) is the model to follow: a tileset (texture + rects, index =
position), an `object_templates` table of tile ranges (pure data), a value `animation`
playhead owned by the actor and advanced with `update(dt)`, and `draw_tile(pos, index)`.

### 9.2 Decisions

1. **The pipeline is load → define sprites → upload → draw.** Loading and defining are
   CPU-only and need no renderer; upload happens afterwards; drawing uses what was
   defined during loading.
2. **Sprite and animation are one concept.** A `sprite` is a sequence of frames
   `{region, duration, flip}` plus a loop flag; a still picture is a sprite with one frame.
   `sprite_animation`, `sprite_appearance`, `sprite_visual`, `sprite_clip_def` and
   `sprite_frame_def` go. *Frame* means an animation step only.
3. ~~**The per-actor playhead is a value** (`sprite_player`), owned by the actor like brix's
   `animation` -- no registry, no unregister.~~ Superseded by 11 and 13: the instance is a
   `sprite` handle whose state the engine owns and advances. Which sprite an actor plays (brix's
   `sprite`: a list of animations + the active one) is the game's state machine.
4. **The game's id is opaque.** `image_bank::entry::id` is whatever the game uses (block
   number, character code, cell index); it may be sparse and may repeat. The engine never
   requires uniqueness; `find_by_id` returns the first match. `image_bank_builder::add`
   takes the id with no default, and there are no placeholder entries (`add_empty` removed).
5. **An atlas has an identity from build time.** `build_atlas` mints an `atlas_id` from an
   engine-wide counter; no GPU is involved.
6. **An atlas is a list of pictures; the address is `{atlas, picture_idx}`** (decided
   2026-11-03, replacing "a region carries `{atlas, page, dest, id}`"). Pages are storage;
   each picture has a placement `{page, rect}` in one flat, private table. `find_by_id`
   returns `optional<atlas_region>` (the address + the game's id); `region(i)` returns the
   same thing by position. Sprites, side tables and draw calls hold addresses.
7. **Upload consumes the atlas.** `register_atlas(image_atlas&&)` keeps the page textures
   and moves the placement table into the registry under the atlas's `atlas_id`; one
   handle per atlas, pages internal. The CPU atlas is short-lived (load → define → upload),
   so rects are not kept twice. The per-page `atlas_texture_id` (earlier "option 1") and
   `gpu_texture_atlas_id` are dropped.
8. **Drawing resolves the address**: `registry.get(atlas, picture_idx)` → `{page texture,
   rect}`, one query. An address of an unregistered atlas, or out of range, fails through
   `DRAW_CHECK`.
9. **Strong types reuse `details::id_strong_type`** + `details::id_access` (the existing
   pattern of `gpu_texture_atlas_id`, `sprite_sheet_id`, …). The header moves from
   `video/sprite/detail/` to `neutrino/detail/`, next to `hash.hh`, because `atlas/` sits
   below `sprite/`. A hidden-friend `operator<<` on the base prints every id (for
   `ENFORCE` messages) without exposing the raw value.
10. **Picture sizes come from either side.** `image_atlas::picture_size(i)` before upload
    (no renderer needed, so headless code keeps the atlas alive), `picture_size(atlas, i)`
    from the registry after. One name, `picture_size` / `picture_count`, at every level.

### 9.2a Decisions of 2026-11-03 (semantics, scenes, timing)

The implementation plan, with milestones, is [`sprite_layer_plan.md`](sprite_layer_plan.md);
its §2 is the full decision table. In short:

11. **`sprite_def` is the class, `sprite` is the instance.** A `sprite_def` holds **one**
    animation sequence (frames + loop mode); it is immutable and shared (revised
    2026-11-04, see 18).
    This supersedes decision 3's value `sprite_player`: the instance is an RAII handle
    whose state the engine owns.
12. **A `sprite` only animates;** the game positions and draws it.
13. **The engine advances sprites.** Each sprite belongs to the scene that spawned it and
    advances with that scene's `fixed_update`, with the same `dt`.
14. **`scene_flags { hides_below, pauses_below }` replaces `is_opaque()`.** A pause menu
    sets `pauses_below`, so the scenes below, and their sprites, freeze.
15. **An application always has at least one scene;** `simplex` gets a single built-in scene.
16. **Events: polling (`finished`, `looped`, `frame_changed`) plus optional callbacks;** a
    looping animation never finishes. The playhead carries the leftover time.
17. *Proposed:* per-picture pivot (from the bank entry) with a per-frame override; animated
    tiles evaluated with the pure `frame_at` against the scene clock + a per-cell phase;
    tint, source inset and fit modes in the draw API; refcounted `atlas_handle`; an
    `atlas_cache` + PNG loader as the asset layer.
18. *(2026-11-04)* **One sequence per `sprite_def`; switching is `sprite::play(other_def)`.**
    A survey of KE, Luxor, the world renderer, warlords and brix found no user that needs
    named multi-clip defs (brix's definition is one frame range; its instance switches
    between definitions). Grouping is a plain game struct of `sprite_def` members; names
    exist only in loaders. This removes animation names, ids and enum keys from the engine.

### 9.3 How brix looks with it

```cpp
// LOAD (no GPU): blocks are headerless 16x16 INDEX8 tiles; tile N is picture N
image_bank load_blocks(std::span<const uint8_t> bytes, const palette& pal) {
    auto bank = image_bank_builder::indexed(pal, /*transparent=*/0);
    for (std::size_t off = 0, n = 0; off + 256 <= bytes.size(); off += 256, ++n) {
        auto tile = bank.next({16, 16});
        for (int y = 0; y < 16; ++y)
            std::memcpy(tile.row(y).data(), bytes.data() + off + y * 16, 16);
        bank.add(std::move(tile), static_cast<uint32_t>(n));
    }
    return std::move(bank).build();
}
image_atlas blocks = build_atlas(load_blocks(dm.load(BLOCKS), pal));

// DEFINE SPRITES (no GPU): object_templates becomes a table of sprites over addresses
const sprite GEM1          = still(blocks.region(1));
const sprite GEM_EXPLOSION = sequence(blocks, 14, 19, configured::gem_explosion_duration(),
                                      /*loop=*/false);

// UPLOAD
register_atlas(std::move(blocks));

// USE: the actor owns its playhead
m_player.play(GEM_EXPLOSION);
if (m_player.update(dt_ms)) { /* one-shot ended */ }
draw_sprite(m_position, m_player);
```

Gone from brix: the hand-built `SDL_Surface`, `video_assets::texture_atlas`, the
`std::vector<SDL_Rect>` lookups, the `object_templates` singleton and its `animation` class.

### 9.4 Open points

1. **Pivot.** Move the hotspot into the bank entry and the placement (`point pivot`), so
   the registry returns everything `draw_sprite` needs -- or keep it in the game's side
   table, or in the sprite's frames?
2. **Playhead advance.** `update(dt)` returning "finished" (brix), or a clock-based player
   (start time + the app clock) with `finished()`?
3. **Fate of the asset path.** `sprite_def` / `sprite_def_builder` / `build_sprite_set` /
   `sprite_cache` / `sprite_set_handle` / `sprite_instance` / `render_bundle`: rebuild on
   atlas + sprites, move to an optional asset layer, or drop until a game needs them.
   Aseprite and grid slicing become producers of a bank / atlas + sprites.
4. **Draw effects for brix.** `draw_tile_shrinked`, `draw_tile_sunken`, `draw_tile_offset_*`
   need a destination size and a source sub-rect in `sprite_draw_params`.
5. **Recolouring.** brix's 6 font colourways and KE's per-level palettes: `with_palette`
   per colourway gives a new atlas (new `atlas_id`, new addresses) -- or a palette swap on
   a registered atlas that keeps the addresses valid?
6. **Does `atlas_region` keep the game's id?** The id is only a lookup key; once found,
   the game keeps the address. `find_by_id` could return a bare `atlas_address`, leaving
   the id in the bank and the atlas's sorted index only.
7. **The world path.** `tileset_bundle`, `render_bundle` and `world_image` share these
   types; review the world subsystem before porting it.
