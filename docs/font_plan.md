# Fonts — implementation plan

Status: **draft, not started** (written 2026-11-04, expanded 2026-11-05).

This document is self-contained: it states the goal, the vocabulary, every decision that
shapes the work, the target API and the order of the work. The *reasoning* behind the
design decisions (the survey of game font formats, the comparison with SDL_FontCache, the
colour and atlas trade-offs) is in [`render_model.md`](render_model.md) §9; this plan
quotes each decision in one line and points there. It supersedes the painter's single RM7
milestone, which now points here. Atlas vocabulary (`image`, `image_bank`, `image_atlas`,
`atlas_id`, `atlas_address`) is defined in
[`images_atlases_sprites.md`](images_atlases_sprites.md).

---

## 1. Why

Neutrino has no text. Every game that shows a string solves it on its own, each badly in a
different way:

- **KE** draws the BIOS 8×8 font from onyx_font **one `draw_point` per lit pixel**, with
  the advance hard-coded to 8 (`scenes/play_game_scene.cc` `draw_game_over`,
  `ke_collage.cc`, `scenes/sprite_gallery_scene.cc`). It works only because KE prints a
  few short strings; there is no measuring, wrapping or alignment.
- **brix** (not yet ported) uses tileset fonts: a glyph is `rects[ch - ' ']` in a picture,
  with case folding and six colour variants made by remapping the palette.
- **warlords** (external) loads onyx `bitmap_font`s from game data, bakes them into a
  white RGBA atlas for Nuklear, rebuilds them when the game switches asset sets, draws BIOS
  text one rectangle per lit pixel, and rasterises its intro text into a CPU buffer.

The pieces exist — onyx_font reads the standard font containers and rasterises glyphs,
neutrino has atlases and textured quads — but nothing connects them. The goal is one
neutrino type, `font`, behind which all of this happens:

```cpp
neutrino::font f = neutrino::bios_font(neutrino::bios_size::_8x8);
neutrino::draw_text({120, 96}, "GAME OVER", f, {.colour = colors::red, .align = text_align::center});
```

Games never include `onyx_font/…` — the same arrangement neutrino already has for
onyx_image, where games see `image` and `load_image` and onyx is an implementation detail
plus a plugin point.

### 1.1 Scope

In scope: loading fonts (standard containers, built-in BIOS fonts, game-specific glyph
formats), measuring and wrapping text, drawing it to the screen, rasterising it into CPU
images, and keeping glyph atlases alive across device resets and asset switches.

Out of scope here: the Nuklear binding (postponed, §5 Phase D), kerning, painting vector
glyphs as line geometry, rich text (mixed fonts / colours inside one string),
right-to-left and complex scripts, and `printf`-style formatting (callers use
`std::format`).

### 1.2 What exists today

| Piece | State |
|---|---|
| neutrino | links `neutrino::onyx_font`; no font type, no text API |
| onyx_font | `bitmap_font` (1-bit, `uint8_t` glyph codes in a `charset` — CP437 for BIOS, ABC spacing), `vector_font` (BGI, Microsoft vector), `ttf_font` (behind `NEUTRINO_ONYX_FONT_LOADER_TTF`, needs FreeType), `font_factory` (`analyze`, `load_bitmap` / `load_vector` / `load_ttf`, `load_all_*`), `font_registry` (decoder plugins), `font_source` (one source over every kind), `glyph_cache<Surface>` (rasterises and packs glyphs on demand), `text_renderer` (draw / aligned / wrapped / measure), `bios_font_8x8/14/16` |
| atlases | `image_atlas`, `build_atlas`, `register_atlas`, `atlas_address` (P3.6, P3.7 M0) — static only: no pictures can be added after registration |
| drawing | immediate quads from atlas pictures (`draw.cc`); the painter ([P3.8], `render_model.md`) is designed, not built |

Prerequisites from other plans that are **not** in the tree yet:

- `image_bank_decoder` and `with_palette` — card [P3.6b];
- per-atlas filter mode and atlas growth after registration — built here (FM0, FM4.1);
- the painter (RM1), replacement in place (`render_model.md` §8.2) and device-reset
  handling (RM6) — card [P3.8].

## 2. Vocabulary

One meaning per word, used throughout this plan and the code.

| Term | Meaning |
|---|---|
| **code point** | One Unicode character (`char32_t`) decoded from the UTF-8 input string. All text in the API is UTF-8. |
| **glyph** | One drawable shape of a font: a picture plus its metrics. Several code points may share one glyph. |
| **glyph id** | The font's own number for a glyph. For a glyph bank it is the `image_bank` entry id; for an onyx bitmap face it is the 8-bit code in the face's charset. |
| **mapping** | The function code point → glyph id (`glyph_mapping`). Many-to-one is allowed; "no glyph" falls back to the font's **default glyph**. Case folding, offset tables and code-page conversion are all mappings. |
| **face** | One typeface in a file (a FON or TTC file holds several). |
| **font** | One face at one pixel size, ready to use: glyph source + glyph atlas + layout rules. A cheap value handle over shared, immutable data. |
| **glyph source** | Where glyph pixels come from: an onyx face (bitmap, vector, TTF) or a game's glyph bank. Kept so the atlas can be rebuilt (device reset) and, for TTF, extended. |
| **glyph atlas** | The `image_atlas` holding a font's rasterised glyphs; each glyph is one picture, addressed by an `atlas_address`. |
| **glyph bank** | A font's glyph pictures plus their metrics: an `image_bank` (glyph ids as entry ids) and `glyph_metrics`, produced by a `glyph_bank_builder`. With a mapping it makes a `font`. The general path for any format onyx does not model, and the way onyx glyphs become pictures too. |
| **glyph_bank_builder** | The public builder of glyph banks (P5): `start_glyph(id, advance)`, pixels relative to the pen on the baseline, `end_glyph()`, `build()`. Glyph sizes and offsets are found from the pixels written, never given in advance. |
| **advance** | How far the pen moves after drawing a glyph. |
| **bearing** (`bearing_x`) | Horizontal distance from the pen to the glyph's left edge (onyx's ABC *a-space*); may be negative. |
| **y offset** | Vertical distance from the top of the line cell to the top of the glyph picture (formats that store short glyphs, e.g. Westwood v3). |
| **baseline** | Distance from the top of the line cell to the line text sits on. |
| **line height** | Distance between the tops of consecutive lines (before `line_spacing`). |
| **white-alpha glyph** | A glyph stored as RGBA with RGB white and coverage in alpha, so one atlas serves every colour: the colour is the quad's **tint** (multiply). |
| **coloured font** | A font whose glyphs carry authored colour (indexed-colour game fonts). Tint still multiplies; white leaves it unchanged. |
| **pixel font** | A bitmap font drawn pixel-exact: nearest filtering, integer scale, positions snapped to whole pixels. |
| **static / growing atlas** | Static: every glyph is baked at load (bitmap fonts, glyph banks). Growing: glyphs are rasterised and added on first use (TTF, vector at size). |
| **warm-up set** | The glyphs a growing font rasterises at creation, so the first frame showing them does not hitch, and the part of a device-reset rebuild that is done eagerly. |
| **layout** | Turning a string into positioned glyphs: decode, map, advance, wrap, align. Measuring, wrapping, caret queries, drawing and CPU rendering all run the same layout. |

## 3. Decisions

Status: *agreed* = decided with the user; *proposed* = this plan's suggestion, to be
confirmed before the milestone that depends on it.

### 3.1 Design decisions (from `render_model.md` §9.3, §9.7)

| # | Decision | Status |
|---|---|---|
| F1 | **Face × size.** A `font` is one face at one pixel size. Bitmap faces have their native size and scale only by integers. `font::at_size(px)` makes another `font` sharing the face data; each size has its own glyph atlas. | agreed |
| F2 | **Character sets.** Text is UTF-8. onyx bitmap faces are 8-bit (codes in a `charset`, CP437 for BIOS): the font maps UTF-8 → that charset and falls back to the face's default character. Glyph-bank fonts are not limited to 8 bits: their mapping takes any code point to a glyph id, many-to-one. | agreed |
| F3 | **Game formats: the glyph bank is the general path.** A glyph-bank decoder is an `image_bank_decoder` (glyph numbers as ids; native pixel format; INDEX8 with a colour key for transparency) that also returns `glyph_metrics` and a mapping. An onyx `bitmap_font`, re-exported as a plugin type, is the shortcut for formats onyx already models (1-bit, ≤ 256 glyphs, ABC spacing, no per-glyph Y offset). | agreed |
| F4 | **Coloured glyph fonts.** A glyph bank with colour pixels makes a coloured font; `tint` multiplies. Colour variants (palette remaps) are separate fonts made with `with_palette`; fonts are small, so a dozen variants is cheap. | agreed |
| F5 | **Location.** `include/neutrino/video/text/`, next to `video/image/`. | agreed |
| F6 | **Glyph metrics.** Per glyph: advance, x bearing, y offset. Per font: line height, baseline, space advance (for formats without a space glyph), letter spacing, default glyph. The superset the surveyed formats need; onyx `bitmap_font` metrics map onto it. | agreed |
| F7 | **Large static fonts.** A Unicode bitmap font with thousands of glyphs is still a static atlas, just multi-page. A lazily built variant only if size becomes a problem. | agreed |
| F8 | **Decoders by analogy with images.** onyx_font keeps the formats it models (TTF/OTF/TTC, FNT, FON, GEM, BGI). Every other format is a glyph-bank decoder in the game or a formats add-on; nothing waits on onyx_font growing an indexed-colour glyph type. | agreed |
| T1 | **Who packs TTF glyphs.** onyx_font's `glyph_cache` packs its own pages; the atlas learns to accept "pre-packed pages + rects" as pictures. (Alternative: onyx rasterises single glyphs and the atlas packer places them.) Since P5 the alternative is the natural fit — single glyphs go through the `glyph_bank_builder` — so this is to be re-decided at the start of FM4. | open |
| T2 | **The `font` type.** Settled by F1–F8. | agreed |
| T3 | **Pixel fonts stay pixel-exact:** nearest filtering, integer scale, positions snapped like every quad. TTF gets sub-pixel positioning only if a game asks. | agreed |
| T4 | **Kerning.** onyx's advances and bearings are kept; pair kerning for TTF only when a game needs it. | agreed |

### 3.2 Decisions taken in this plan

| # | Decision | Status |
|---|---|---|
| P1 | **Neutrino owns layout.** Measure, wrap, align and caret run in neutrino over the font's glyph table (P3), one code path for every font kind. Reason: glyph-bank fonts are not onyx fonts, so onyx's `text_renderer` (which works over its own `glyph_cache`) cannot lay them out. onyx stays the source of glyph pixels and metrics; its UTF-8 decoder (`utf8.hh`) and wrapping logic are reused where they fit. Refines `render_model.md` §9.2 ("onyx's `text_renderer` does the layout"). | proposed |
| P2 | **Fonts ship before the painter.** Phases A–B draw text through today's immediate draw path (one textured quad per glyph, `tint` = colour); Phase C moves the same calls onto the painter. The public API does not change between the two: `draw.hh` calls become painting when the painter's RM2 lands anyway. Reason: KE needs text now, and the painter is a large separate project. | proposed |
| P3 | **One internal glyph table.** Every font, whatever its source, is normalised at construction into one table: mapping (code point → glyph id), glyph id → `{atlas_address, glyph metrics}`, and font metrics. Layout, drawing, CPU rendering and the later Nuklear binding read only this table, so a new font source never touches them. | proposed |
| P5 | **Glyph pictures are made by a public `glyph_bank_builder`.** One builder for every source: onyx bitmap faces, onyx's TTF / vector rasteriser, and game decoders (brix, Westwood, …). A glyph is written pixel by pixel relative to the pen on the baseline, between `start_glyph(id, advance)` and `end_glyph()`; the builder finds the glyph's bounds from the pixels written, crops to them, and derives `bearing_x` / `y_offset` itself, so nobody sizes a glyph in advance and decoders only state the advance. The pixel kind is fixed per builder: coverage (white + alpha), indexed (palette + transparent index) or RGBA. `build()` returns a `glyph_bank` — exactly what `font::from_glyphs` takes. Refines F3: a glyph-bank decoder fills a builder rather than returning a bank and side tables. | agreed 2026-11-05 |
| P4 | **TTF is optional at build time.** `load_font` handles TTF/OTF/TTC only when `NEUTRINO_ONYX_FONT_LOADER_TTF` is on (it pulls in FreeType). Without it those formats are reported as unsupported; bitmap and vector fonts still work. Reason: FreeType's size and availability on web / Android builds. | proposed |

## 4. Target API (sketch)

Names follow the decisions above; signatures are indicative and are fixed in the milestone
that builds them. Headers: `include/neutrino/video/text/font.hh`, `font_loader.hh`,
`text_layout.hh`.

### 4.1 Making a font

```cpp
namespace neutrino {
    class font;                                     // value handle over shared immutable data

    // Built-in BIOS faces (CP437); the shared data is created once.
    enum class bios_size { _8x8, _8x14, _8x16 };
    font bios_font(bios_size);

    // Standard containers through onyx_font: FNT, FON, GEM, BGI, MS vector; TTF/OTF/TTC (P4).
    struct font_options {
        float        pixel_size{0};                 // 0 = native size (bitmap faces)
        std::size_t  face_index{0};                 // which face in a FON / TTC
        font_style   style{};                       // bold, italic, underline, strikethrough
        atlas_filter filter{atlas_filter::automatic};   // nearest for bitmap, linear for scalable
        std::u32string_view warm_up{};              // glyphs rasterised up front (growing fonts)
    };
    font load_font(const std::filesystem::path&, const font_options& = {});
    font load_font(std::span<const std::uint8_t>, const font_options& = {});
    std::vector<font_face_info> list_faces(std::span<const std::uint8_t>);

    // Glyph banks (F3, P5): pictures + metrics, built by glyph_bank_builder.
    struct glyph_metrics {
        struct glyph {
            std::uint32_t id;                       // image_bank entry id
            std::int16_t  advance;
            std::int16_t  bearing_x{0};
            std::int16_t  y_offset{0};
        };
        std::vector<glyph> glyphs;
        std::int16_t  line_height;
        std::int16_t  baseline;
        std::int16_t  space_advance;                // used when there is no space glyph
        std::int16_t  letter_spacing{0};            // implicit gap the format adds
        std::uint32_t default_glyph;                // for unmapped code points
    };
    struct glyph_bank {
        image_bank    pictures;                     // entry id = glyph id
        glyph_metrics metrics;
    };

    enum class bit_order { msb_first, lsb_first };   // neutrino's own; no onyx type in the API

    class glyph_bank_builder {
        public:
            // The pixel kind is fixed per builder.
            static glyph_bank_builder coverage();                                  // white + alpha
            static glyph_bank_builder indexed(const palette&, std::uint8_t transparent_index);
            static glyph_bank_builder rgba();

            // Font-wide metrics (F6); defaults: computed from the glyphs where possible.
            glyph_bank_builder& line_height(int), baseline(int), space_advance(int),
                                letter_spacing(int), default_glyph(std::uint32_t);

            // One glyph. Coordinates: origin = pen on the baseline, y down; negative allowed.
            void start_glyph(std::uint32_t id, float advance);
            void put_alpha(int x, int y, std::uint8_t alpha);   // coverage: max of writes
            void put_index(int x, int y, std::uint8_t index);   // indexed: last write wins
            void put_color(int x, int y, color c);              // rgba: last write wins
            // Bulk forms (same rules), one call per row:
            void put_alpha_span(int x, int y, std::span<const std::uint8_t>);
            void put_index_span(int x, int y, std::span<const std::uint8_t>);
            void put_color_span(int x, int y, std::span<const color>);
            void put_bits(int x, int y, std::span<const std::byte> row, int width,
                          bit_order, std::uint8_t on);          // 1-bit rows → alpha / index
            void end_glyph();                                   // crop; bearing / y_offset

            [[nodiscard]] glyph_bank build() &&;
    };

    using glyph_mapping = std::function<std::optional<std::uint32_t>(char32_t)>;
    // plus a constructor from a table for the common contiguous case

    class font {
        public:
            static font from_glyphs(glyph_bank glyphs, glyph_mapping);
            static font from_bitmap(const onyx_font::bitmap_font&);   // F3 shortcut, plugin type
            font at_size(float pixel_size) const;   // same face, another size (F1)
            const font_metrics& metrics() const;
    };

    void register_font_decoder(…);                  // plugin point over onyx font_registry
}
```

### 4.2 Measuring, drawing, rendering

```cpp
enum class text_align { left, center, right };
struct text_params {
    color      colour{colors::white};   // tint; multiplies coloured fonts
    text_align align{text_align::left};
    float      scale{1.0f};             // integer for pixel fonts (T3)
    float      letter_spacing{0};
    float      line_spacing{0};
};

// Layout queries (text_layout.hh)
dim                       measure(std::string_view, const font&, const text_params& = {});
std::vector<line_range>   wrap(std::string_view, const font&, float width, const text_params& = {});
point                     offset_of(std::string_view, const font&, std::size_t index);   // caret position
std::size_t               index_at(std::string_view, const font&, point);                // hit test

// Drawing: one quad per glyph; returns the bounds actually drawn.
rect draw_text(point position, std::string_view, const font&, const text_params& = {});
rect draw_text(const rect& box, std::string_view, const font&, const text_params& = {});   // wrapped

// CPU rasterisation into an image, same layout as draw_text.
void render_text(image& target, point, std::string_view, const font&, const text_params& = {});
```

In Phase C, `paint_list::paint_text(…)` joins these with the same parameters plus a depth.

Hidden inside neutrino: `font_source`, `glyph_cache`, `text_renderer`, the
`raster_target` adaptor feeding a `glyph_bank_builder`, FreeType. The only onyx type a game may name is `onyx_font::bitmap_font`, and only to hand
one to `font::from_bitmap` (warlords already produces them).

### 4.3 Examples

KE's game-over text, today ~20 lines of per-pixel loops:

```cpp
font f = bios_font(bios_size::_8x8);
draw_text({160, 100}, "GAME OVER", f, {.colour = colors::red, .align = text_align::center, .scale = 2});
```

A game format decoded as a glyph bank (brix-style: glyph `i` is character `' ' + i`,
lower case folded to upper case):

```cpp
// The game's decoder writes each glyph's palette indices; sizes and offsets are derived.
glyph_bank decode_brix_font(std::span<const std::uint8_t> bytes, const palette& pal) {
    auto b = glyph_bank_builder::indexed(pal, 0);
    b.line_height(8).baseline(7);
    for (std::uint32_t g = 0; g < glyph_count(bytes); ++g) {
        b.start_glyph(g, 8);
        for (int y = 0; y < 8; ++y) {
            b.put_index_span(0, y - 7, glyph_row(bytes, g, y));   // rows above the baseline
        }
        b.end_glyph();
    }
    return std::move(b).build();
}

font f = font::from_glyphs(decode_brix_font(bytes, pal),
    [](char32_t c) -> std::optional<std::uint32_t> {
        if (c >= U'a' && c <= U'z') c -= U'a' - U'A';
        if (c < U' ' || c > U'Z') return std::nullopt;     // → default glyph
        return static_cast<std::uint32_t>(c - U' ');
    });
```

A TTF label at two sizes, with ASCII warmed up:

```cpp
font ui = load_font("ui.ttf", {.pixel_size = 16, .warm_up = U" !\"#…~"});
font title = ui.at_size(32);
rect r = draw_text(box, "Options", title, {.align = text_align::center});
```

## 5. How a string becomes pixels

```
UTF-8 text
   │ decode                      (onyx utf8.hh)
   ▼
code points ──mapping──► glyph ids ──glyph table (P3)──► {atlas_address, metrics}
                                                             │
   layout: pen += bearing / advance / letter_spacing; line breaks by wrap width;
           align per line; y from baseline and y_offset; scale; snap for pixel fonts
                                                             ▼
                                         positioned glyphs  (one list, three consumers)
                       ┌─────────────────────┬──────────────┴──────────────┐
                       ▼                     ▼                             ▼
             measure / wrap / caret   draw_text: quad per glyph,   render_text: blit glyph
             (no pixels touched)      tint = colour → screen       source pixels → image
```

For a growing font (TTF), a glyph missing from the atlas is rasterised and added during
layout, before its quad is drawn — so every consumer sees a complete table.

## 6. Phases and milestones

```
Phase A — bitmap fonts, immediate path      FM0 → FM1 → FM2 → FM3
Phase B — scalable fonts                    FM3 → FM4 (needs atlas growth)
Phase C — painter and lifetime              FM5 (needs painter RM1) → FM6 (needs RM6, §8.2)
Phase D — later, on demand                  FM7…
```

Every milestone ends with the whole tree building and `neutrino_tests`,
`simplex_entry_tests`, `luxor_unittest`, `game_format_tests` green.

### Phase A — Bitmap fonts on the immediate path

After Phase A every game's current text needs are met: KE's BIOS text, brix-style tileset
fonts and warlords's bitmap faces can all be loaded, measured, drawn and rasterised.

#### FM0 — Atlas prerequisites

Why: a glyph atlas is an ordinary `image_atlas`, but text asks two things of atlases
that sprites have not asked yet:

- **control over filtering:** a pixel font drawn at scale 2 must stay crisp, while a TTF
  font rasterised at size wants smoothing;
- **authored colour with transparency:** game fonts are indexed-colour images with a
  transparent index, and must reach the GPU with that index transparent.

FM0 adds the missing pieces to the atlas layer and tests them in isolation. No font code
yet; FM1 is the first user. (Making glyph pictures — white + alpha for monochrome fonts —
is the `glyph_bank_builder`, FM1.)

**What exists today** (checked 2026-11-05):

- `build_atlas(bank, format, max_page)` (`src/neutrino/video/atlas/image_atlas.cc`) keeps
  an INDEX8 bank whose entries share one palette and colour key as **INDEX8 pages**
  (keyed pixels keep the page's key fill); asked for another format, it converts each
  entry first, which turns the colour key into alpha 0. Pictures get a 1-pixel gutter
  with edge extrusion.
- `register_atlas(atlas, atlas_texture_format)` (`src/neutrino/video/sprite/texture_registry.cc`)
  uploads each page with `SDL_CreateTextureFromSurface` (format `automatic`) or after a
  conversion, and wraps it with `texture::adopt`, which deliberately applies **no** engine
  defaults. `SDL_CreateTextureFromSurface` promotes a colour key to an alpha format (SDL
  source: "No alpha, but a colorkey => promote to alpha").
- **Atlas pages are therefore linear-filtered today.** SDL gives a new texture the
  renderer's default scale mode, which is `SDL_SCALEMODE_LINEAR`, and `adopt` does not
  change it — unlike `texture::from_image` / `texture::create`, which apply neutrino's
  nearest default. Any atlas picture drawn scaled, rotated or at a fractional position
  (sprites, tiles) is smoothed. This is the open filtering question of card #19.
- Tint exists on the immediate path: `draw_texture` sets the texture's colour / alpha mod
  for the call (`tint_scope`), `draw_texture_quad` uses vertex colour. The sprite-visual
  path builds white vertices and has no tint.

1. **Filter mode per atlas.**
   - The filter is a property of the GPU pages, so it is given to `register_atlas` and
     stored in the registry entry (`gpu_texture_atlas`), not on the CPU `image_atlas`:
     every page uploaded now, added later by growth (FM4) or rebuilt after a device reset
     (FM6) gets it from there.
   - Type: reuse `texture_filter { nearest, linear, pixelart }` from `texture.hh` rather
     than a new `atlas_filter`; the "automatic" choice (nearest for bitmap fonts, linear
     for scalable ones) belongs to `font_options`, which resolves it before registering.
     *(Proposed; `render_model.md` §9.3 sketches a separate `atlas_filter` with
     `automatic`. Either works; one enum is less vocabulary.)*
   - Default: **`linear`** (decided 2026-11-05) — what atlas pages always had, so no
     existing atlas changes its look. Standalone textures keep their `nearest` default.
     Pixel-exact users (pixel fonts, coloured glyph banks) pass `nearest` explicitly.
     This answers card #19's open filtering question for atlases.
   - Optionally `set_atlas_filter(atlas_id, texture_filter)` for changing it after
     upload — only if a user appears.
2. **Indexed-colour glyph banks.** The CPU side already works (see above); FM0 settles the
   path and pins it with tests:
   - keep the bank's INDEX8 pages in the CPU atlas (that is what `with_palette` will
     recolour later, [P3.6b]);
   - upload them as RGBA with the key as alpha 0 — `automatic` already does this through
     SDL's colour-key promotion; make it explicit for glyph atlases so it does not depend
     on which formats a renderer reports;
   - tint multiplies the authored colours, white leaves them unchanged (F4);
   - coloured fonts are pixel fonts, so they use `nearest`: with linear filtering the
     RGB of the transparent index would bleed into glyph edges.
   - Only if a test shows a gap (for example a key not surviving the gutter extrusion or
     the upload) does `build_atlas` / `register_atlas` change.

**Tests** (all headless, dummy video driver, software renderer):

- each `texture_filter` passed to `register_atlas` is the scale mode of every page
  texture (read through `require_texture_registry()`);
- a two-colour picture drawn scaled into a target texture contains only the two
  original colours with `nearest`, and blended colours with `linear`;
- an INDEX8 bank with a colour key: the CPU atlas stays INDEX8; after upload, keyed
  pixels are transparent (the background shows through), other pixels keep their palette
  colour, and a tint multiplies them;
- the key survives the gutter: a picture whose border pixels are keyed draws no fringe
  at ×2 with `nearest`.

Exit: the tests above pass; no public API besides the `register_atlas` filter parameter.

**Done 2026-11-05.**
- Item 1: `register_atlas(atlas, format, filter)` (default `linear`) applies the filter to
  every page and `gpu_texture_atlas` keeps it for FM4 / FM6;
  `test/video/test_register_atlas.cc` covers the default, every filter on a two-page atlas,
  and nearest vs linear sampling of a picture drawn at ×10.
- Item 2: no code change was needed. The same file's "indexed banks" suite checks that a
  keyed INDEX8 bank stays INDEX8 on the CPU and, uploaded `automatic` or `rgba` with
  `nearest`, draws at ×2 with the key transparent (border and surroundings show the
  background, no fringe), the palette colours intact, and a tint multiplying them.
  Dropping the colour key makes it fail (144 assertions), so it guards the path.

#### FM1 — `font` core and onyx bitmap fonts

Why: the type, the glyph table, the glyph builder and the layout engine are the core every
later milestone builds on; onyx bitmap faces (including BIOS) are the simplest source to
prove them.

1. `include/neutrino/video/text/font.hh`: `font` as a value handle over shared immutable
   data (F1); `font_metrics` (line height, baseline, space advance, letter spacing,
   default glyph); the internal glyph table (P3).
2. **`glyph_bank_builder`** (P5), public, `include/neutrino/video/text/glyph_bank_builder.hh`.
   All three pixel kinds land here, because the builder is one piece; FM1 uses
   `coverage`, FM2 `indexed` / `rgba`.
   - **Coordinates.** Origin = the pen on the baseline, x right, y down; negative
     coordinates are normal (a negative bearing, everything above the baseline).
   - **Ink.** A write of alpha 0, of the transparent index, or of a colour with alpha 0 is
     not ink: it neither extends the glyph's bounds nor changes a pixel. A glyph with no
     ink is **blank**: an empty picture with an advance (space).
   - **Overlap.** Coverage keeps the maximum alpha (vector strokes overlap at their joints;
     onyx's `grayscale_max_target` does the same); indexed and RGBA: last write wins.
   - **`end_glyph()`** crops to the ink bounds and records `bearing_x` = left edge and
     `y_offset` = top edge relative to the cell top (`baseline` above the origin), so the
     decoder states only the advance. A glyph id may be started once; `build()` checks.
   - **Pictures.** Coverage → RGBA32 with RGB white on **every** pixel, transparent ones
     included, and straight alpha. SDL_Renderer has no portable single-channel texture,
     so coverage lives in alpha; tint `c` then draws exactly `c`, one atlas serves every
     colour, and linear filtering never mixes black into an edge (a fresh image is
     transparent *black*, so the builder fills white first). Indexed → INDEX8 with the
     builder's palette and the transparent index as colour key (what `build_atlas` and
     `with_palette` expect). RGBA → RGBA32.
   - **Font-wide metrics** are set on the builder; unset ones are derived (line height =
     tallest ink extent, space advance = advance of glyph `' '` if present).
   - **Performance: the per-pixel call.** Glyph data is small — BIOS 8×8 is 16 K pixels
     for 256 glyphs, ASCII TTF at 32 px about 40 K, a 20 000-glyph Unicode bitmap font at
     16×16 about 5 M, all once at load or first use — but the hot path is still kept to a
     compare and a store:
     - `put_*` are **inline** in the header and write into a scratch buffer held
       directly in the builder (not behind a pimpl), so no call crosses the shared-library
       boundary per pixel; only buffer growth is out of line;
     - the scratch buffer is one canvas reused for every glyph (cleared at
       `start_glyph`); a write outside it grows it geometrically and re-centres, so after
       the first few glyphs nothing is allocated;
     - **row forms** cover the bulk producers with one call per row:
       `put_*_span` (game decoders that produce rows; onyx's TTF path through the
       `raster_target_with_span` concept's `put_span`) and `put_bits` (1-bit rows, e.g.
       `bitmap_view::row(y)` with its bit order — a BIOS glyph is 8 calls);
     - the pixel-kind check (calling `put_index` on a coverage builder) is one predictable
       branch, then an ENFORCE.
     FM1 measures it: building BIOS 8×16 and a synthetic 20 000-glyph font, reported in
     the test log; if per-pixel cost shows up, the row forms are the answer, not a
     different API.
   - Tests: coordinates and crop (negative bearing, ink above and below the baseline,
     derived `bearing_x` / `y_offset`); non-ink writes ignored; overlap rules per kind;
     blank glyphs give an empty picture and picture *i* = glyph *i*; every coverage pixel
     is RGB white; `put_bits` for both bit orders; row forms equal per-pixel writes;
     wrong-kind write throws; drawn with tint `(200, 40, 40)` over black a full-coverage
     pixel reads `(200, 40, 40)`, and at ×3 with `linear` no edge pixel is darker than the
     tint scaled by its coverage.
3. `font::from_bitmap(const onyx_font::bitmap_font&)` (F3 shortcut): feeds every glyph of
   the face into a coverage builder (`put_bits` per row of `bitmap_view`; advance from the
   ABC spacing, `a_space` as the pen offset of the first column), builds a static glyph
   atlas with `nearest` filtering; mapping = UTF-8 → the face's `charset`, falling back to
   its default character (F2).
4. `bios_font(bios_size)` over onyx's built-ins, created once.
5. Layout (P1) in `text_layout.hh`: `measure`, `wrap`, `offset_of`, `index_at`; UTF-8
   decoding; `\n`; tab width.
6. Tests: charset mapping and fallback (CP437 for BIOS); advances and bearings from an
   onyx face; measure / wrap edge cases (empty, a single word longer than the width,
   trailing spaces, newlines); caret round-trip `index_at(offset_of(i)) == i`.

Exit: a BIOS font and an onyx FNT face measure and wrap correctly; no `onyx_font` include
in the public headers except the re-exported plugin type.

#### FM2 — Glyph-bank fonts

Why: game formats are the reason neutrino needs its own font type rather than onyx's;
FM2 proves the general path against the hardest formats from the survey
(`render_model.md` §9.3.1).

1. `glyph_metrics` (F6), `glyph_bank` and `glyph_mapping` (a function, plus a table
   constructor for the common case) in `font.hh`.
2. `font::from_glyphs(glyph_bank, glyph_mapping)` (F3 general path): glyph ids are the
   bank's entry ids; per-glyph `y_offset` and `bearing_x` honoured; a missing space glyph
   uses `space_advance`; unmapped code points use `default_glyph`.
3. **Coloured fonts (F4):** a bank with colour pixels makes a coloured font; `tint`
   multiplies. Colour variants via `with_palette` once [P3.6b] has it (until then, a
   variant is a second bank).
4. **Decoders fill a builder (P5):** a glyph-bank decoder takes the bytes (and, for
   palette fonts, the palette), writes every glyph into a `glyph_bank_builder` in the
   format's native kind (`indexed` for palette fonts, so `with_palette` can recolour
   them; `coverage` for 1-bit formats; `rgba` only for true-colour data), and returns
   `build()`. Storage quirks (RLE, interleaving, glyph data shared between codes) stay in
   the decoder; shared data is simply written once per glyph id that uses it.
5. Tests with synthetic data, one per hard case, each written through the builder:
   - Westwood-v3-style: 4-bit indexed, per-glyph Y offset and height, glyph data shared
     between codes;
   - Jazz-style mapping: 0–25 = A–Z, unsupported → 126, no space glyph;
   - RA2-style Unicode mapping: many code points → one glyph, implicit +1 px spacing;
   - brix-style mapping: case folding, `' '` offset.

Exit: all four synthetic fonts lay out and rasterise correctly.

#### FM3 — Drawing text and CPU rendering; port KE

Why: the first visible result, and the first real user — KE's per-pixel loops go.

1. `text_params` and `text_align` (§4.2).
2. `draw_text(point, …)` and `draw_text(box, …)` (wrapped) on the immediate draw path
   (P2): one glyph quad per glyph from the atlas, `tint` = colour, returns the drawn
   bounds. Pixel fonts: integer `scale`, snapped positions (T3).
3. `render_text(image&, …)`: the same layout rasterised into a CPU image from the glyph
   source pixels (warlords's intro case).
4. **Port KE:** `draw_game_over`, `ke_collage.cc` and the sprite gallery use
   `bios_font(_8x8)` + `draw_text`; their per-pixel loops and `onyx_font` includes go.
5. Tests: pixel tests for left / centre / right and wrapped boxes; tint on white and on
   coloured fonts; returned bounds; `render_text` matches `draw_text` pixel for pixel on a
   cleared target.

Exit: KE shows identical text through `draw_text`; no file under `games/` includes
`onyx_font/…`.

### Phase B — Scalable fonts

#### FM4 — TTF / OTF / vector fonts

Why: UI text at arbitrary sizes and DPI. Scalable fonts cannot be baked whole, so this is
where atlases must learn to grow.

1. **Atlas growth (engine):** a growing atlas accepts new pictures after registration,
   written into its page with a sub-rect texture upload (`SDL_UpdateTexture` on a static
   texture, never a render target); a full page adds a page; existing addresses never
   move.
2. **Rasterising into the builder:** `font_source::rasterize_glyph(code_point, size,
   target, x, y)` (`x` = pen, `y` = baseline) writes 8-bit coverage through onyx's
   `raster_target` concept (`put_pixel`, `width()`, `height()`; optional `put_span`). A
   small internal adaptor forwards that to a coverage `glyph_bank_builder` at origin
   (0, 0), so the builder's crop gives the exact bounds — no metrics box, no margin, no
   measuring pass; bold / italic (`rasterize_styled_glyph`) drawing past the metrics box is
   no longer a problem. To check: onyx passes the target's `width()` / `height()` to its
   vector rasteriser, so the adaptor must report a size that never clips.
   (`glyph_rasterizer.hh`'s free functions are `onyx_font::internal`; not used.)
3. **Packing (T1):** rasterising one glyph at a time into the builder is T1's alternative
   (onyx rasterises single glyphs, the atlas places them); using onyx's own packed
   `glyph_cache` pages would bypass the builder. FM4 starts by deciding T1; the builder
   path is the one this plan now assumes.
4. `load_font(path | bytes, font_options)` and `list_faces(bytes)` over
   `font_factory::analyze` / `load_*` (P4 for TTF); vector faces (BGI, Microsoft vector)
   rasterised at `pixel_size` through the same path.
5. **Warm-up set:** rasterised at creation; any other glyph on first use, added to the
   atlas before its quad is drawn.
6. `font::at_size(px)`: shares the face data, own glyph atlas (F1). `filter` defaults to
   linear for scalable fonts.
7. `register_font_decoder(…)`: the plugin point over `font_registry` (F8).
8. Tests: a BGI glyph and a TTF glyph (when built in) land entirely inside their picture,
   with and without bold / italic; glyphs appear on first use without moving earlier addresses; a page overflow
   adds a page; warm-up glyphs exist before any draw; `at_size` shares the face; a build
   with `NEUTRINO_ONYX_FONT_LOADER_TTF=OFF` reports TTF as unsupported and passes the rest.

Exit: a TTF label and a BGI label draw correctly at two sizes.

### Phase C — Painter integration and lifetime

#### FM5 — `paint_text` on the painter (needs painter RM1)

Why: on the immediate path text is always drawn where it is called; on the painter it
sorts by depth with sprites and geometry and is batched.

1. `paint_list::paint_text(…)` with the FM3 signatures plus depth; glyph quads become
   paint commands resolved at record time, sorted with everything else.
2. The immediate `draw_text` stays as the `draw.hh` entry point (it records once the
   painter's RM2 lands).
3. Tests: text interleaves with quads and geometry by depth; a glyph rasterised during
   layout is registered before its command is recorded.

Exit: KE text sorts with its sprites in one layer.

#### FM6 — Replacement and device reset (needs painter RM6 and `render_model.md` §8.2)

Why: glyph atlases must survive the GPU losing its textures and the game swapping data
sets, without games noticing.

1. **Replacement in place:** rebuild a font's glyph atlas under the same `atlas_id` — on a
   style change (`glyph_cache::clear()`), a new pixel size for an existing face slot, or a
   game swapping its data set (warlords's `on_assets_changed()`).
2. **Device reset: rebuild from the glyph source, no CPU copy.** Bitmap and glyph-bank
   fonts rebake eagerly (they are small); TTF clears its cache and re-rasterises lazily,
   eagerly only for its warm-up set. Glyph atlases are therefore the exception to the
   painter's "keep CPU pages" rule (`render_model.md` §8.3).
3. Tests: a forced device reset keeps BIOS, glyph-bank and TTF text pixel-identical; a
   forced replacement keeps addresses valid for code points that still exist.

Exit: RM7's original exit criteria (`render_model.md` §14) are met.

### Phase D — Later, on demand

Each item waits for a game that needs it.

- **FM7 — Nuklear** (postponed, painter RM8): an engine `nk_user_font` over `font`
  (width = `measure`, glyph query = the glyph table) and Nuklear as a geometry producer.
- **Kerning** (T4): pair kerning for TTF.
- **Vector strokes as geometry:** BGI / Microsoft vector glyphs painted as lines at any
  scale, without an atlas.
- **Lazy large static fonts** (F7): build pages of a big Unicode bitmap font on first use.
- **Format decoders:** Westwood, Dynamix, Jazz Jackrabbit, … as glyph-bank decoders in the
  game or a formats add-on (F8).

## 7. Dependencies

| Milestone | Needs |
|---|---|
| FM0 | — |
| FM1 | FM0 |
| FM2 | FM1; `with_palette` ([P3.6b]) for palette variants, optional until then |
| FM3 | FM2 |
| FM4 | FM3; atlas growth (built in FM4.1) |
| FM5 | FM3; painter RM1 |
| FM6 | FM4, FM5; painter RM6 and replacement in place (`render_model.md` §8.2) |
| FM7 | FM5; painter RM3 (geometry) |

Phase A depends on neither the painter nor the sprite plan, and can run in parallel with
both.

## 8. Risks and open points

| Risk / question | Plan |
|---|---|
| Neutrino-owned layout (P1) diverges from onyx's `text_renderer` | Reuse onyx's UTF-8 decoding and wrap logic where possible; cross-check measure / wrap against `text_renderer` on onyx faces in tests |
| `with_palette` and `image_bank_decoder` are not implemented yet ([P3.6b]) | FM2 works without them (variants as separate banks, decoders as plain functions); adopt them when they land |
| One quad per glyph on the immediate path is slow for long text | Acceptable for Phase A — KE draws far less than its current one-point-per-pixel loop; batching comes with the painter (FM5) |
| Pixel fonts blurred by non-integer scale or position | T3: nearest filter, integer scale, snapped positions; a pixel test at scale 2 |
| FreeType size and availability (web, Android) | P4: TTF optional at build time |
| Atlas growth touches the atlas core shared with sprites | Growth is append-only (addresses never move); tested in isolation in FM4.1 |
| Unicode bitmap fonts with thousands of glyphs make large static atlases | Multi-page atlases already work (F7); lazy pages only if measured |
| T1 is still open (P5 favours rasterising single glyphs) | Decide at the start of FM4; only FM4.3 depends on it |
| A method call per pixel in `glyph_bank_builder` | Inline hot path into an in-class buffer, reused scratch canvas, row forms (`put_*_span`, `put_bits`); measured in FM1 on BIOS 8×16 and a 20 000-glyph synthetic font |
