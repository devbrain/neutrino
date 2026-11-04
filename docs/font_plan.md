# Fonts — implementation plan

Status: **draft, not started** (written 2026-11-04). The design is in
[`render_model.md`](render_model.md) §9 (text) — the facade over onyx_font (§9.3, decisions
F1–F8), colour (§9.4), atlas requirements (§9.5), painting text (§9.6), open decisions
T1–T4 (§9.7). This document is the *how* and the *order*; it supersedes the painter's
single RM7 milestone, which now points here. Atlas vocabulary:
[`images_atlases_sprites.md`](images_atlases_sprites.md).

---

## 1. Goal and scope

Games get one neutrino type, `font`, and never include `onyx_font/…` — the arrangement
neutrino already has for onyx_image. A `font` is glyph source + glyph atlas + layout rules
(§9.2), loaded from a standard container through onyx, built in (BIOS), or built from a
game's glyph bank. Text is measured, wrapped, painted, and rasterised into CPU images
through it.

In scope: everything in §9 except what §8 lists as postponed.

## 2. Where things are today

| Site | What it does |
|---|---|
| KE `scenes/play_game_scene.cc` (`draw_game_over`), `ke_collage.cc`, `scenes/sprite_gallery_scene.cc` | `onyx_font::bios_font_8x8()` drawn **one `draw_point` per lit pixel**, advance hard-coded to 8 |
| brix (`video/video_system.cc`, not yet ported) | tileset fonts, `rects[ch - ' ']`, case folding, 6 palette-ramped colour variants |
| warlords (`bane/ui/font_service.cc`, `nuklear/backend/nk_onyx_font.cc`, external) | onyx `bitmap_font`s from game data, baked to a white RGBA atlas for Nuklear; rebuilt on asset switch; `bios_text` draws one rect per lit pixel; intro text rasterised into a CPU buffer |
| neutrino | links `neutrino::onyx_font`; no font type, no text API |
| onyx_font | `bitmap_font` (1-bit, `uint8_t` glyphs, ABC spacing), `vector_font`, `ttf_font` (behind `NEUTRINO_ONYX_FONT_LOADER_TTF`, FreeType), `font_factory` (`analyze`, `load_bitmap` / `load_vector` / `load_ttf`, `load_all_*`), `font_registry` (decoder plugins), `font_source` (unified source), `glyph_cache<Surface>`, `text_renderer` (draw / aligned / wrapped / measure), `bios_font_8x8/14/16` |

Prerequisites from other plans that are **not** in the tree yet: `image_bank_decoder` and
`with_palette` (images doc, "planned"); per-atlas filter mode; atlas growth after
registration; the painter (RM1), replacement in place (§8.2) and device-reset handling
(RM6).

## 3. Decisions taken here

The design decisions are F1–F8 and T1–T4 in `render_model.md`. This plan adds:

| # | Decision | Status |
|---|---|---|
| P1 | **neutrino owns layout.** Measure, wrap, align, caret and bounds run in neutrino over the font's `glyph_metrics` (§9.3, F6), one code path for every font kind. Glyph-bank fonts are not onyx fonts, so onyx's `text_renderer` (which works over a `glyph_cache`) cannot serve them; onyx stays the source of glyph pixels and metrics, and its UTF-8 decoding (`utf8.hh`) and line-wrapping logic are reused where they fit. This refines §9.2's "onyx_font's `text_renderer` does the layout". | proposed |
| P2 | **Fonts ship before the painter.** Phases A–B draw text through the existing immediate draw path (glyph quads from the atlas with `tint`); Phase C moves `paint_text` onto the painter. The public API does not change between the two — `draw.hh` calls become painting in the painter's RM2 anyway. | proposed |
| P3 | **One internal glyph table.** Every font, whatever its source, is normalised at construction into the same table: code point → glyph id (mapping), glyph id → `{atlas_address, glyph metrics}`, plus font metrics. Layout, painting, CPU rendering and the Nuklear binding (postponed) read only this table. | proposed |
| P4 | **TTF is optional at build time.** `load_font` for TTF/OTF/TTC exists only when `NEUTRINO_ONYX_FONT_LOADER_TTF` is on (it pulls FreeType); without it, those formats are reported as unsupported, bitmap and vector fonts still work. | proposed |

## 4. Phases and milestones

```
Phase A — bitmap fonts, immediate path      FM0 → FM1 → FM2 → FM3
Phase B — scalable fonts                    FM3 → FM4 (needs atlas growth)
Phase C — painter and lifetime              FM5 (needs painter RM1) → FM6 (needs RM6, §8.2)
Phase D — later, on demand                  FM7…
```

Every milestone ends with the whole tree building and `neutrino_tests`,
`simplex_entry_tests`, `luxor_unittest`, `game_format_tests` green.

### Phase A — Bitmap fonts on the immediate path

#### FM0 — Atlas prerequisites

1. **Per-atlas filter mode:** `atlas_filter { automatic, nearest, linear, pixelart }`
   on `build_atlas` / the registered atlas, mapped to the existing `texture_filter` on its
   page textures. Closes the filtering question left open by card #19 for atlases.
2. **White-alpha glyph images:** a helper building an RGBA8888 image (RGB white, coverage
   or bit → alpha) from an onyx `bitmap_view` / coverage buffer.
3. **INDEX8 with colour key → RGBA** in `build_atlas` for coloured glyph banks (check what
   `build_atlas` already does for the tileset path; add only the missing case).

Exit: tests for each filter mode reaching the page textures; white-alpha conversion pixel
tests; an INDEX8 + key bank packs to RGBA with the key transparent.

#### FM1 — `font` core and onyx bitmap fonts

1. `include/neutrino/video/text/font.hh`: `font` as a value handle over shared immutable
   data (F1); `font_metrics` (line height, baseline, space advance, letter spacing,
   default glyph); the internal glyph table (P3).
2. `font::from_bitmap(const onyx_font::bitmap_font&)` (F3 shortcut): bakes the face's
   whole range into a static white glyph atlas (`atlas_filter::nearest`), glyph metrics
   from the ABC spacing (`a_space` → bearing, advance), mapping = UTF-8 → the face's
   `charset` with fallback to its default character (F2).
3. `bios_font(bios_size::_8x8 | _8x14 | _8x16)` over onyx's built-ins, created once.
4. Layout (P1) in `text_layout.hh`: `measure(text, font)`, `wrap(text, font, width)`,
   `offset_of(text, font, index)`, `index_at(text, font, point)`; UTF-8 decoding; `\n`;
   tab width.
5. Tests: charset mapping and fallback (CP437 for BIOS); advances and bearings from an
   onyx face; measure / wrap edge cases (empty, single long word, trailing spaces,
   newlines); caret round-trip `index_at(offset_of(i)) == i`.

Exit: a BIOS font and an onyx FNT face measure and wrap correctly; no `onyx_font` include
in the public headers except the re-exported plugin type.

#### FM2 — Glyph-bank fonts

1. `glyph_metrics` (F6) and `glyph_mapping` (code point → glyph id, many-to-one; a
   function, plus a table constructor for the common case) in `font.hh`.
2. `font::from_glyphs(image_bank, glyph_metrics, glyph_mapping)` (F3 general path):
   glyph ids are the bank's ids; per-glyph `y_offset` and `bearing_x` honoured; a missing
   space glyph uses `space_advance`; unmapped code points use `default_glyph`.
3. **Coloured fonts (F4):** a bank with colour pixels makes a coloured font; `tint`
   multiplies. Colour variants via `with_palette` once the images layer has it (until
   then, a variant is a second bank).
4. A glyph-bank decoder signature = `image_bank_decoder` + side tables
   (`glyph_metrics`, mapping), following the images doc's rules.
5. Tests with synthetic data: a Westwood-v3-style bank (4-bit indexed, per-glyph Y offset
   and height, shared glyph data), a Jazz-style mapping (0–25 = A–Z, unsupported → 126,
   no space glyph), an RA2-style Unicode mapping (many code points → one glyph), a
   brix-style mapping (case folding, `' '` offset).

Exit: all four synthetic fonts lay out and rasterise correctly.

#### FM3 — Drawing text and CPU rendering; port KE

1. `text_params { colour, align, scale, letter_spacing, line_spacing }`, `text_align`
   (§9.6).
2. `draw_text(position, text, font, params) → rect` and
   `draw_text(box, text, font, params) → rect` (wrapped), on the immediate draw path
   (P2): one glyph quad per glyph from the atlas, `tint` = colour, returns the bounds.
   Pixel fonts: integer `scale`, positions snapped (T3).
3. `render_text(image&, point, text, font, params)`: the same layout rasterised into a CPU
   image from the glyph table's source pixels (warlords's intro case).
4. **Port KE:** `draw_game_over`, `ke_collage.cc` and the sprite gallery use
   `bios_font(_8x8)` + `draw_text`; their per-pixel loops and `onyx_font` includes go.
5. Tests: pixel tests for left / centre / right and wrapped boxes; tint on white and on
   coloured fonts; returned bounds; `render_text` matches `draw_text` pixel for pixel on a
   cleared target.

Exit: KE shows identical text through `draw_text`; no file under `games/` includes
`onyx_font/…`.

### Phase B — Scalable fonts

#### FM4 — TTF / OTF / vector fonts

1. **Atlas growth (engine, §9.5):** a dynamic atlas accepts new pictures after
   registration — written into its page with a sub-rect texture upload (static texture,
   `SDL_UpdateTexture`, never a render target); a full page adds a page; existing
   addresses never move.
2. **Pre-packed pages (T1):** onyx_font's `glyph_cache` packs; the atlas accepts its pages
   + rects as pictures. (If T1 is decided the other way, onyx rasterises single glyphs and
   the atlas packer places them — the rest of FM4 is unchanged.)
3. `load_font(path | bytes, font_options)` and `list_faces(bytes)` over
   `font_factory::analyze` / `load_*` (P4 for TTF); `font_options { pixel_size,
   face_index, style, filter, warm_up }`; vector faces (BGI, Microsoft vector) rasterised
   at `pixel_size` through the same path.
4. **Warm-up set:** rasterised at creation; other glyphs on first use, registered before
   the glyph's quad is drawn (§9.6).
5. `font::at_size(px)`: shares the face data, own glyph atlas (F1). `filter` defaults to
   linear.
6. `register_font_decoder(…)`: the plugin point over `font_registry` (F8).
7. Tests: glyphs appear on first use without moving earlier addresses; a page overflow
   adds a page; warm-up glyphs exist before any draw; `at_size` shares the face; a build
   with `NEUTRINO_ONYX_FONT_LOADER_TTF=OFF` reports TTF as unsupported and passes the rest.

Exit: a TTF label and a BGI label draw correctly at two sizes.

### Phase C — Painter integration and lifetime

#### FM5 — `paint_text` on the painter (needs painter RM1)

1. `paint_list::paint_text(…)` with the FM3 signatures; glyph quads become paint commands
   resolved at record (§3.3), sorted with everything else.
2. The immediate `draw_text` stays as the `draw.hh` entry point (it records once the
   painter's RM2 lands).
3. Tests: text interleaves with quads and geometry by depth; a glyph rasterised during
   layout is registered before its command is recorded.

Exit: KE text sorts with its sprites in one layer.

#### FM6 — Replacement and device reset (needs painter RM6 and §8.2)

1. **Replacement in place:** rebuilding a font's glyph atlas under the same `atlas_id` —
   style change (`glyph_cache::clear()`), new pixel size for an existing face slot, a
   game swapping its data set (warlords's `on_assets_changed()`).
2. **Device reset: rebuild from the source, no CPU copy** (§9.5): bitmap and glyph-bank
   fonts rebake eagerly; TTF clears its cache and re-rasterises lazily, eagerly only for
   its warm-up set.
3. Tests: forced device reset keeps BIOS, glyph-bank and TTF text pixel-identical; forced
   replacement keeps addresses valid for code points that still exist.

Exit: RM7's original exit criteria (`render_model.md` §14) are met.

### Phase D — Later, on demand

- **FM7 — Nuklear** (postponed, painter RM8): an engine `nk_user_font` over `font`
  (width = `measure`, glyph query = the glyph table) and Nuklear as a geometry producer.
- **Kerning** (T4): pair kerning for TTF when a game needs it.
- **Vector strokes as geometry:** BGI / Microsoft vector glyphs painted as line geometry
  at any scale, without an atlas.
- **Lazy large static fonts** (F7): build pages of a big Unicode bitmap font on first use.
- **Format decoders:** Westwood, Dynamix, Jazz Jackrabbit, … as glyph-bank decoders in the
  game or a formats add-on (F8), driven by a game that needs them.

## 5. Dependencies

| Milestone | Needs |
|---|---|
| FM0 | — |
| FM1 | FM0 |
| FM2 | FM1; `with_palette` for palette variants (optional until then) |
| FM3 | FM2 |
| FM4 | FM3; atlas growth (built in FM4.1) |
| FM5 | FM3; painter RM1 |
| FM6 | FM4, FM5; painter RM6 and replacement in place (§8.2) |
| FM7 | FM5; painter RM3 (geometry) |

Phase A does not depend on the painter or on the sprite plan, and can run in parallel with
both.

## 6. Risks and open points

| Risk / question | Plan |
|---|---|
| Neutrino-owned layout (P1) diverges from onyx's `text_renderer` | Reuse onyx's UTF-8 decoding and wrap logic where possible; cross-check measure / wrap against `text_renderer` on onyx faces in tests |
| `with_palette` and `image_bank_decoder` are not implemented yet | FM2 works without them (variants as separate banks, decoders as plain functions); adopt them when the images layer lands them |
| One quad per glyph on the immediate path is slow for long text | Acceptable for Phase A (KE draws far less than the current one-point-per-pixel loop); batching comes with the painter (FM5) |
| Pixel fonts blurred by non-integer scale or position | T3: nearest filter, integer scale, snapped positions; a pixel test at scale 2 |
| FreeType size and availability (web, Android) | P4: TTF optional at build time |
| Atlas growth touches the atlas core shared with sprites | Growth is append-only (addresses never move); test it in isolation in FM4.1 |
| Unicode bitmap fonts with thousands of glyphs make large static atlases | Multi-page atlases already work (F7); lazy pages only if measured |
