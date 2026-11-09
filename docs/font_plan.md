# Fonts — implementation plan

Status: **in progress** — FM0 done, FM1 mostly built (written 2026-11-04, expanded 2026-11-05,
updated 2026-11-09).

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
| neutrino | links `neutrino::onyx_font`; `glyph_atlas` and `glyph_atlas_builder`, `font_id` with `register_font` / `unregister_font`, onyx bitmap fonts through decoders and `register_bios_font`, a code point → glyph lookup (FM1, 2026-11-08); layout queries in `text_layout.hh` (FM1, 2026-11-09); no drawing yet |
| onyx_font | `bitmap_font` (1-bit, `uint8_t` glyph codes in a `charset` — CP437 for BIOS, ABC spacing), `vector_font` (BGI, Microsoft vector), `ttf_font` (behind `NEUTRINO_ONYX_FONT_LOADER_TTF`, needs FreeType), `font_factory` (`analyze`, `load_bitmap` / `load_vector` / `load_ttf`, `load_all_*`), `font_registry` (decoder plugins), `font_source` (one source over every kind), `glyph_cache<Surface>` (rasterises and packs glyphs on demand), `text_renderer` (draw / aligned / wrapped / measure), `bios_font_8x8/14/16` |
| atlases | `image_atlas`, `build_atlas`, `register_atlas`, `atlas_address` (P3.6, P3.7 M0) — static only: no pictures can be added after registration |
| drawing | immediate quads from atlas pictures (`draw.cc`); the painter ([P3.8], `render_model.md`) is designed, not built |

Prerequisites from other plans that are **not** in the tree yet:

- `image_bank_decoder` and `with_palette` — card [P3.6b];
- per-atlas filter mode and atlas growth after registration — built here (FM0; growth in FM4 step 2, P8);
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
| **glyph bank** | A font's glyph pictures plus their metrics, as a game format stores them. In neutrino it becomes a `glyph_atlas`: the packed `image_atlas` (glyph *i* = picture *i*, entry id = glyph id), one `glyph_properties` per glyph and the font's `font_properties`. With a mapping it makes a `font`. The general path for any format onyx does not model, and the way onyx glyphs become pictures too. |
| **glyph_atlas_builder** | The public builder of glyph atlases (P5): `start_glyph(id, props)`, pixels relative to the pen on the baseline, `end_glyph()`, `build()`. Glyph sizes and offsets are found from the pixels written, never given in advance. |
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
| F1 | **Face × size.** A `font` is one face at one pixel size. Bitmap faces have their native size and scale only by integers. `font::at_size(px)` makes another `font` sharing the face data; each size has its own glyph atlas. Since 2026-11-08 a font is a `font_id`; `font_at_size(id, px)` gives the other size, over a hidden shared face (P7). | agreed |
| F2 | **Character sets.** Text is UTF-8. onyx bitmap faces are 8-bit (codes in a `charset`, CP437 for BIOS): the font maps UTF-8 → that charset and falls back to the face's default character. Glyph-bank fonts are not limited to 8 bits: their mapping takes any code point to a glyph id, many-to-one. | agreed |
| F3 | **Game formats: the glyph bank is the general path.** A glyph-bank decoder is an `image_bank_decoder` (glyph numbers as ids; native pixel format; INDEX8 with a colour key for transparency) that also returns glyph metrics and a mapping. An onyx `bitmap_font`, re-exported as a plugin type, is the shortcut for formats onyx already models (1-bit, ≤ 256 glyphs, ABC spacing, no per-glyph Y offset). | agreed |
| F4 | **Coloured glyph fonts.** A glyph bank with colour pixels makes a coloured font; `tint` multiplies. Colour variants (palette remaps) are separate fonts made with `with_palette`; fonts are small, so a dozen variants is cheap. | agreed |
| F5 | **Location.** `include/neutrino/video/text/`, next to `video/image/`. | agreed |
| F6 | **Glyph metrics.** Per glyph: advance, x bearing, y offset. Per font: line height, baseline, space advance (for formats without a space glyph), letter spacing, default glyph. The superset the surveyed formats need; onyx `bitmap_font` metrics map onto it. | agreed |
| F7 | **Large static fonts.** A Unicode bitmap font with thousands of glyphs is still a static atlas, just multi-page. A lazily built variant only if size becomes a problem. | agreed |
| F8 | **Decoders by analogy with images.** onyx_font keeps the formats it models (TTF/OTF/TTC, FNT, FON, GEM, BGI). Every other format is a glyph-bank decoder in the game or a formats add-on; nothing waits on onyx_font growing an indexed-colour glyph type. | agreed |
| T1 | **Who packs TTF glyphs.** onyx_font's `glyph_cache` packs its own pages; the atlas learns to accept "pre-packed pages + rects" as pictures. (Alternative: onyx rasterises single glyphs and the atlas packer places them.) Since P5 the alternative is the natural fit — single glyphs go through the `glyph_atlas_builder` — so this is to be re-decided at the start of FM4. **Settled 2026-11-08:** onyx rasterises single glyphs into the `glyph_atlas_builder`, our packer places them; onyx's `glyph_cache` pages are not used (they would bypass the builder and the code point table). | agreed 2026-11-08 |
| T2 | **The `font` type.** Settled by F1–F8. | agreed |
| T3 | **Pixel fonts stay pixel-exact:** nearest filtering, integer scale, positions snapped like every quad. TTF gets sub-pixel positioning only if a game asks. | agreed |
| T4 | **Kerning.** onyx's advances and bearings are kept; pair kerning for TTF only when a game needs it. | agreed |

### 3.2 Decisions taken in this plan

| # | Decision | Status |
|---|---|---|
| P1 | **Neutrino owns layout.** Measure, wrap, align and caret run in neutrino over the font's glyph table (P3), one code path for every font kind. Reason: glyph-bank fonts are not onyx fonts, so onyx's `text_renderer` (which works over its own `glyph_cache`) cannot lay them out. onyx stays the source of glyph pixels and metrics; its UTF-8 decoder (`utf8.hh`) is reused. onyx's wrapping was checked (2026-11-09) and not reused: it is tied to its `glyph_cache` / `font_source`, breaks at any character, and has no caret, fit or pixel snapping. Refines `render_model.md` §9.2 ("onyx's `text_renderer` does the layout"). | agreed, built 2026-11-09 |
| P2 | **Fonts ship before the painter.** Phases A–B draw text through today's immediate draw path (one textured quad per glyph, `tint` = colour); Phase C moves the same calls onto the painter. The public API does not change between the two: `draw.hh` calls become painting when the painter's RM2 lands anyway. Reason: KE needs text now, and the painter is a large separate project. | proposed |
| P3 | **One internal glyph table.** Every font, whatever its source, is normalised at construction into one table: mapping (code point → glyph id), glyph id → `{atlas_address, glyph metrics}`, and font metrics. Layout, drawing, CPU rendering and the later Nuklear binding read only this table, so a new font source never touches them. | proposed |
| P5 | **Glyph pictures are made by a public `glyph_atlas_builder`.** One builder for every source: onyx bitmap faces, onyx's TTF / vector rasteriser, and game decoders (brix, Westwood, …). A glyph is written pixel by pixel relative to the pen on the baseline, between `start_glyph(id, props)` and `end_glyph()`; the builder finds the glyph's bounds from the pixels written, crops to them, and derives `bearing_x` / `y_offset` itself, so nobody sizes a glyph in advance and decoders only state the advance. The pixel kind is a template parameter, `glyph_atlas_builder<glyph_atlas_format>`: `alpha` (white + coverage), `indexed` (palette + transparent index, given to the constructor) or `rgba`; a pixel of the wrong kind does not compile. Font-wide `font_properties` are given to the constructor as they are. `build()` packs the pictures and returns a `glyph_atlas` — what `font::from_glyphs` takes. Refines F3: a glyph-bank decoder fills a builder rather than returning a bank and side tables. | agreed 2026-11-05, shape built 2026-11-08 |
| P6 | **An empty glyph is a glyph without pixels.** `end_glyph()` with nothing written (or `empty_glyph(id, props)`) adds an empty picture: glyph *i* stays picture *i*, its region is 0×0 and nothing is packed; it only advances the pen. There is no separate "ink" rule: every pixel written counts, and a later write to the same pixel replaces it (all kinds). | agreed 2026-11-08 |
| P4 | **TTF is built in by default.** onyx_font's `NEUTRINO_ONYX_FONT_LOADER_TTF` defaults to ON and fetches FreeType 2.13.3 from source with every optional dependency off (zlib, bzip2, png, HarfBuzz, brotli), so it builds wherever CMake builds C, web and Android included; the cost is binary size, not availability. neutrino gates nothing: `register_font(bytes)` finds TTF/OTF/TTC through onyx's outline decoders like any format. The opt-out stays onyx's (warlords-style size savings); then TTF data finds no decoder and the error says TTF support is not built in. TTF tests sit behind `ONYX_FONT_HAS_LOADER_TTF`. | agreed 2026-11-08 (revised; was "optional at build time") |
| P7 | **Scalable fonts: faces are hidden.** Users only see `font_id`. `register_font(bytes, face, font_options)` with a `pixel_size` makes one size; `font_at_size(id, px)` makes another. Internally `fonts_manager` keeps one shared face per source (the onyx `font_source`, which owns the TTF bytes or BGI strokes); each size's `font_id` references it and the last `unregister_font` frees it. The face is also what a device-reset rebuild (FM6) and later kerning (T4) use. (centurion's `font_bundle` shape, without public face ids.) | agreed 2026-11-08 |
| P8 | **Scalable fonts: static atlases first.** BGI / Microsoft vector fonts are 8-bit (≤ 256 glyphs, a `charset`): every glyph is rasterised at registration into a static atlas, the FON path. TTF rasterises a glyph set at registration — default `glyph_set::latin1` (U+0020–U+007E, U+00A0–U+00FF, 190 glyphs), or `basic_latin`, plus `extra_glyphs` (€, Cyrillic, curly quotes, …); other code points draw the default glyph. The growing atlas (glyphs added on first use, sub-rect uploads) comes later, when a game needs CJK or typed text; it is internal to the font — `font_id`, layout and drawing do not change. | agreed 2026-11-08 |
| P9 | **Advances in 26.6 fixed point.** `glyph_properties::advance` is in 1/64 px (FreeType's unit), so fractional TTF advances do not drift (ten glyphs of 6.4 px are 64 px, not 60). Layout carries the fractional pen and rounds each glyph's position to a whole pixel (T3's snapping). Bearings, offsets and font properties stay whole pixels. **As built (2026-11-09):** the public `glyph_properties::advance` is a `float` in pixels, rounded to 1/64 by `start_glyph` (NaN / infinity throw), so decoders write `8` or `6.4f`; the 26.6 type (`details::fixed26_6`, `src/neutrino/video/text/fixed26_6.hh`) is internal — a registered font keeps its advances in it and layout sums them. | agreed 2026-11-08, built 2026-11-09 |
| P10 | **No foreign glyph cache.** Checked 2026-11-09: onyx_font's `glyph_cache<Surface>` (its own pages, onyx fonts only — T1), sdlpp's `font_cache` (one texture per glyph, colour mod, string textures — the centurion design) and warlords' `onyx_font_binding` (one hand-made strip atlas per onyx `bitmap_font`, ≤ 256 glyphs, for Nuklear). None can hold glyph-bank fonts or feed neutrino atlases (texture registry, filter, device reset, painter batching), so neutrino keeps one store, `fonts_manager`. Reused from them: onyx's rasterisers, metrics, `charset` / `encode_char`, UTF-8 and later kerning; the named ranges (`store_basic_latin` / `store_latin1` = P8's glyph sets). warlords' binding uses the same recipe as neutrino's FON path (white + alpha, gutter, nearest, colour at draw time) and shrinks to `register_font` + a Nuklear adaptor over layout once `draw_text` exists (FM7). | proposed 2026-11-09 |

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
    // Revised 2026-11-08 (P7, P8): these become register_font(bytes, face, font_options) -> font_id
    // and font_at_size(font_id, px); the font value handle below is superseded by font_id.
    enum class glyph_set { basic_latin, latin1 };   // P8; latin1 is the default
    struct font_options {
        float        pixel_size{0};                 // required for scalable faces; 0 = native (bitmap)
        glyph_set    glyphs{glyph_set::latin1};     // TTF: rasterised at registration (P8)
        std::u32string_view extra_glyphs{};         // TTF: on top of the set (€, Cyrillic, ...)
        bool         monochrome{false};             // TTF hinted to 1-bit (the Windows 95 look)
        font_style   style{};                       // synthetic bold / italic (onyx render_style)
        atlas_filter filter{atlas_filter::automatic};   // nearest for 1-bit (bitmap, BGI, monochrome), linear for antialiased
    };
    font load_font(const std::filesystem::path&, const font_options& = {});
    font load_font(std::span<const std::uint8_t>, const font_options& = {});
    std::vector<font_face_info> list_faces(std::span<const std::uint8_t>);

    // Glyph atlases (F3, F6, P5, P6): built in include/neutrino/video/text/glyph_atlas.hh.
    struct glyph_properties {                       // per glyph
        std::uint32_t glyph = 0;                    // the game's id; also the bank entry id
        float         advance = 0;                  // pixels, stated by the decoder; kept to 1/64 px (P9)
        std::int16_t  bearing_x = 0;                // derived: pen -> left edge of the picture
        std::int16_t  y_offset = 0;                 // derived: cell top -> top edge of the picture
    };
    struct font_properties {                        // per font, given to the builder as they are
        std::int16_t  line_height = 0;
        std::int16_t  baseline = 0;                 // cell top -> baseline
        std::int16_t  space_advance = 0;            // used when there is no space glyph
        std::int16_t  letter_spacing = 0;           // implicit gap the format adds
        std::uint32_t default_glyph = 0;            // for unmapped code points
    };
    class glyph_atlas {                             // made only by glyph_atlas_builder
        public:
            std::size_t glyph_count() const noexcept;
            const glyph_properties& get_glyph_properties(std::size_t idx) const; // glyph i = picture i
            font_properties get_font_properties() const noexcept;
    };
    font_id register_font(glyph_atlas&&, atlas_texture_format, texture_filter);
    void unregister_font(font_id);

    enum class glyph_atlas_format { alpha, indexed, rgba };

    template<glyph_atlas_format F>                  // the pixel kind is part of the type
    class glyph_atlas_builder {
        public:
            explicit glyph_atlas_builder(font_properties);                       // alpha, rgba
            glyph_atlas_builder(font_properties, std::optional<std::uint8_t> key);          // indexed
            glyph_atlas_builder(font_properties, const palette&, std::optional<std::uint8_t> key);

            // One glyph. Coordinates: origin = pen on the baseline, y down; negative allowed.
            void start_glyph(std::uint32_t id, glyph_properties props = {});    // props.advance
            void put(int x, int y, pixel_type);     // alpha: coverage, indexed: index, rgba: color
            void end_glyph();                       // crop; bearing_x / y_offset; empty -> 0x0
            void empty_glyph(std::uint32_t id, glyph_properties props = {});

            [[nodiscard]] glyph_atlas build(std::optional<pixel_format> = {}, std::optional<dim> max_page = {}) &&;
    };

    // Built (FM1, 2026-11-09): bulk writes, stored and replayed as whole runs
    //   put_span(x, y, std::span<const pixel_type>);       a row of pixels, every one counts
    //   put_mask(x, y, const bitmask&, pixel_type on);     solid pixels of neutrino's bitmask

    using glyph_mapping = std::function<std::optional<std::uint32_t>(char32_t)>;
    // plus a constructor from a table for the common contiguous case

    class font {
        public:
            static font from_glyphs(glyph_atlas glyphs, glyph_mapping);
            static font from_bitmap(const onyx_font::bitmap_font&);   // built as register_font(bytes) / register_bios_font
            font at_size(float pixel_size) const;   // same face, another size (F1)
            const font_properties& metrics() const;
    };

    // Built (FM1): onyx bitmap fonts, the same arrangement as image decoders
    using bitmap_font_decoder = onyx_font::bitmap_font_decoder;
    void    register_font_decoder(std::unique_ptr<bitmap_font_decoder>);          // -> onyx font_registry
    font_id register_font(std::span<const std::uint8_t>, std::size_t face = 0, …);  // first decoder that sniffs it
    font_id register_font(std::span<const std::uint8_t>, const bitmap_font_decoder&, std::size_t face = 0, …);
    enum class bios_font { cga_8x8, ega_8x14, vga_8x16 };
    font_id register_bios_font(bios_font, …);
}
```

### 4.2 Measuring, drawing, rendering

```cpp
// Built (FM1, include/neutrino/video/text/text_layout.hh):
enum class text_align { left, center, right };
struct text_params {
    text_align align{text_align::left}; // within the wrap width, else the widest line
    float      scale{1.0f};             // must be > 0; integer for pixel fonts (T3)
    int        letter_spacing{0};       // on top of the font's
    int        line_spacing{0};         // on top of the font's line height
    int        tab_width{4};            // tab stops every tab_width space advances
    // To add (FM3):
    color      colour{colors::white};   // tint; multiplies coloured fonts
    // Proposed (2026-11-08, ideas from centurion / SDL_ttf), see FM3:
    std::optional<color> background{};  // "shaded": one opaque box per line behind the glyphs
    bool       underline{false};        // lines drawn by layout from the baseline, any font kind
    bool       strikethrough{false};
    std::optional<color> shadow{};      // the glyphs drawn again, offset, before the text
    point      shadow_offset{1, 1};
};

// Layout queries (text_layout.hh), built (FM1). Indices are byte offsets into the UTF-8 text.
struct line_range { std::size_t begin, end; int width; };   // without break spaces and '\n'
struct fit_result { std::size_t count; int extent; };       // bytes that fit, their width
dim                     measure(std::string_view, font_id, const text_params& = {}, std::optional<int> wrap_width = {});
std::vector<line_range> wrap(std::string_view, font_id, int width, const text_params& = {});
fit_result              fit(std::string_view, font_id, int width, const text_params& = {});  // first line only
point                   offset_of(std::string_view, font_id, std::size_t index, const text_params& = {},
                                  std::optional<int> wrap_width = {});                       // caret position
std::size_t             index_at(std::string_view, font_id, point, const text_params& = {},
                                 std::optional<int> wrap_width = {});                        // hit test
// Font queries (font.hh), built (FM1):
font_properties                 get_font_properties(font_id);
std::optional<glyph_properties> get_glyph_properties(font_id, char32_t);  // own glyph, else default
bool                            has_glyph(font_id, char32_t);             // own glyph only

// Drawing: one quad per glyph; returns the bounds actually drawn.
rect draw_text(point position, std::string_view, const font&, const text_params& = {});
rect draw_text(const rect& box, std::string_view, const font&, const text_params& = {});   // wrapped

// CPU rasterisation into an image, same layout as draw_text.
void render_text(image& target, point, std::string_view, const font&, const text_params& = {});
```

In Phase C, `paint_list::paint_text(…)` joins these with the same parameters plus a depth.

Hidden inside neutrino: `font_source`, `glyph_cache`, `text_renderer`, the
`raster_target` adaptor feeding a `glyph_atlas_builder`, FreeType. The only onyx type a game may name is `onyx_font::bitmap_font`, and only to hand
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
glyph_atlas decode_brix_font(std::span<const std::uint8_t> bytes, const palette& pal) {
    glyph_atlas_builder<glyph_atlas_format::indexed> b({.line_height = 8, .baseline = 7}, pal, 0);
    for (std::uint32_t g = 0; g < glyph_count(bytes); ++g) {
        b.start_glyph(g, {.advance = 8});
        for (int y = 0; y < 8; ++y) {
            b.put_span(0, y - 7, glyph_row(bytes, g, y));   // rows above the baseline (FM1 row form)
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

A TTF label at two sizes (Latin-1 rasterised at registration, P8):

```cpp
font_id ui = register_font(read_file("ui.ttf"), 0, {.pixel_size = 16, .extra_glyphs = U"€"});
font_id title = font_at_size(ui, 32);
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

For a growing font (TTF, after P8's static first step), a glyph missing from the atlas is
rasterised and added during layout, before its quad is drawn — so every consumer sees a
complete table. Until then a code point outside the font's glyph set draws the default glyph.

## 6. Phases and milestones

```
Phase A — bitmap fonts, immediate path      FM0 → FM1 → FM2 → FM3
Phase B — scalable fonts                    FM3 → FM4 (static atlases first; growth later, P8)
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
is the `glyph_atlas_builder`, FM1.)

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
   data (F1), using `font_properties` (line height, baseline, space advance, letter spacing,
   default glyph); the internal glyph table (P3).
2. **`glyph_atlas_builder`** (P5), public, `include/neutrino/video/text/glyph_atlas.hh`.
   All three pixel kinds land here, because the builder is one piece; FM1 uses
   `alpha`, FM2 `indexed` / `rgba`. *(The list below is the original plan; see "Progress"
   after this milestone for what is built and what changed.)*
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
   decoding; `\n`; tab width. *Proposed (from centurion's `measure_text`):* `fit(text,
   width) -> {count, extent}`, how many characters fit in a width, the step under wrapping,
   ellipsis truncation and text input; `has_glyph(cp)` without the default fallback, for
   fallback chains later.
6. Tests: charset mapping and fallback (CP437 for BIOS); advances and bearings from an
   onyx face; measure / wrap edge cases (empty, a single word longer than the width,
   trailing spaces, newlines); caret round-trip `index_at(offset_of(i)) == i`.

Exit: a BIOS font and an onyx FNT face measure and wrap correctly; no `onyx_font` include
in the public headers except the re-exported plugin type.

**Progress (2026-11-09).** Items 1–6 are built in their first form. Left: the two drawing
tests of item 2 and a layout test on a real FON face (the exit criterion).
- **Built:** `glyph_atlas_builder<alpha | indexed | rgba>` (P5) and `glyph_atlas` in
  `include/neutrino/video/text/glyph_atlas.hh`; `glyph_properties` / `font_properties` (F6);
  Pixels are recorded relative to the pen, cropped to their bounds, and replayed into an `image_builder`; `bearing_x` = left edge, `y_offset` =
  top edge + `baseline`. Alpha pictures are white with coverage as alpha on a
  transparent-white background; indexed pictures use the builder's palette and are filled
  with the transparent index; RGBA keeps transparent black. Empty glyphs follow P6; the bank
  entry id is the glyph id, so `find_by_id(glyph)` works. Out-of-order `start_glyph` /
  `end_glyph` / `build`, and glyphs further than an `int16_t` from the pen, throw.
- **Item 1, started:** `include/neutrino/video/text/font.hh` has `font_id` (a strong id like
  `atlas_id`), `register_font(glyph_atlas&&, format, filter)` and `unregister_font(font_id)`.
  Registering uploads the atlas to the texture registry and keeps the font and glyph
  properties in an internal `fonts_manager` (an application service, next to `sprite_cache`),
  keyed by `font_id`; a failure after the upload unregisters the atlas again.
  `unregister_font` ignores invalid ids, ids already removed, and calls after teardown.
  `glyph_atlas` exposes `get_glyph_properties(i)` and `get_font_properties()`.
- **Underneath:** `image_builder<image_kind>` / `image_bank_builder<image_kind>` are typed
  (palette and key only on indexed builders) and have `fill()`; `build_atlas` accepts INDEX8
  for a bank of only empty entries.
- **Changed from the list above:** no ink rule and no max-of-writes for coverage (P6:
  every write counts, last write wins); font-wide metrics are not derived; wrong-kind writes
  are compile errors, not an ENFORCE; one `put(x, y, pixel)` per kind instead of
  `put_alpha` / `put_index` / `put_color`.
- **Glyph lookup (item 1):** each registered font keeps a code point → glyph index map (many
  code points may share a glyph) and the index of `default_glyph`; `find_glyph(cp)` gives the
  glyph's own index, else the default's, else nothing. `register_font(glyph_atlas&&)` takes
  each glyph id as its code point; FM2's `glyph_mapping` adds a many-to-one overload.
  `build()` now rejects a glyph id used twice.
- **Items 3–4, built differently:** no `font::from_bitmap`; onyx bitmap fonts go through an
  internal converter (`src/neutrino/video/text/onyx_bitmap_font.cc`) to a `glyph_atlas` and
  then `register_font`. Public: `bitmap_font_decoder` (re-exported onyx type, like
  `image_decoder`), `register_font_decoder`, `register_font(bytes, face)` (first decoder in
  onyx's `font_registry` that sniffs the data: GEM, Windows FNT / FON, custom),
  `register_font(bytes, decoder, face)`, `register_bios_font(cga_8x8 | ega_8x14 | vga_8x16)`.
  Glyph ids are the font's bytes; the lookup is built by passing every BMP code point
  through `encode_char`, because CP437 maps two code points to one byte (☺ U+263A and
  U+0001 → byte 1), which decoding bytes would miss (about 30 ms for BIOS 8×16, debug build).
  Baseline = ascent, line height = pixel height + external leading, space advance = the break
  char's advance, advance = A + B + C; each glyph is copied into a `bitmask` and written with
  one `put_mask` at `(a, -ascent)`; the atlas filter is the caller's choice (default linear).
- **Changed in item 1:** a font is an id over an application registry (the
  `register_atlas` / `atlas_id` pattern), not the value handle over shared data that item 1
  and F1 describe. `font::at_size` (F1) has to fit this shape when scalable faces arrive.
- **Advances (P9), 2026-11-09:** `glyph_properties::advance` is a `float` rounded to 1/64 px by
  `start_glyph`; a registered font keeps its advances as internal `details::fixed26_6`.
- **Item 5, layout (2026-11-09):** `include/neutrino/video/text/text_layout.hh` (public:
  `text_align`, `text_params`, `line_range`, `fit_result`, `measure`, `wrap`, `fit`, `offset_of`,
  `index_at`) over an internal engine (`src/neutrino/video/text/text_layout_engine.*`) that
  lays out every character once (line, caret x, advance, glyph and picture position), so
  FM3's `draw_text` reads the same result. Rules:
  - greedy word wrap at spaces and tabs; the spaces at a break belong to no line and do not
    count in its width; a word wider than the box breaks between characters; every line
    holds at least one character;
  - `\n` ends a line (text ending in `\n` has an empty last line), `\r` takes no space,
    tabs go to the next multiple of `tab_width` × the space advance;
  - letter spacing (font + params) goes between characters, not before a line's first;
  - pens run in 26.6 at unscaled size; each position is scaled and rounded to a whole
    pixel (T3), so carets and pictures snap and long lines do not drift;
  - alignment is within the wrap width, or the widest line when not wrapping, never
    negative; height = (lines − 1) × line step + line height;
  - `offset_of` / `index_at` round-trip for every character; `index_at` picks the caret
    before the character whose left half holds x.
  `fit` works on the first line only and is not wrapped.
- **Item 1, font queries (2026-11-09):** `get_font_properties(font_id)`,
  `get_glyph_properties(font_id, cp)` (own glyph, else the default, else nullopt) and
  `has_glyph(font_id, cp)` (own glyph only, for fallback chains) in `font.hh`.
- **Item 2, bulk writes (2026-11-09):** `put_span(x, y, pixels)` and `put_mask(x, y, bitmask,
  on)` replace the planned `put_*_span` / `put_bits` / `bit_order`: 1-bit data uses the
  existing `neutrino::bitmask` (which gained `row(y)`), so there is no second bit-row type.
  The builder records every write as a horizontal run — a fill (`put`, each solid run of a
  mask) or a span whose pixels go to a per-glyph pool in one insert — and replays runs in
  order (last write wins) straight into the picture's rows with `fill_n` / `copy_n` /
  `transform`, without per-pixel bounds checks. Measured (release, 20 000 glyphs of 16×16):
  per-pixel `put` 34 ms, `put_mask` ~21 ms, `put_span` ~15 ms, packing 24–32 ms; BIOS 8×16
  conversion 4.4 ms, most of it the code point lookup. Debug build per pixel: 1652 ms before,
  ~500 ms after. The timings are printed by the cost test cases.
- **Still to do in item 2:** the tint-over-black and ×3 linear-edge drawing tests.
- **Still to do for the exit:** measure / wrap on a real proportional FON face (`HELVA.FON`).
- **Open:** `font.hh` includes `onyx_font/font_factory.hh` (for `font_entry` in decoders'
  `enumerate()`), more than the "plugin type only" exit rule allows.
- **Tests:** `test/video/test_glyph_atlas.cc` (24 cases: crop, bearing and offset, pictures per
  kind, empty glyphs, entry ids, font properties, misuse, duplicate ids, all-empty font;
  `put_span` incl. coverage 0, span / put order, `put_mask` against per-pixel writes, clear
  masks, indexed runs; three cost cases;
  `register_font` stores the atlas and properties, `unregister_font` removes both and
  tolerates repeats); `test/video/test_font.cc` (21 cases: lookup rules; ABC spacing on a
  synthetic font; every BIOS font incl. ░, é, ☺ / U+0001 and no €; CP437 and proportional FON
  files found by detection, from `test/video/data/fonts/`; custom decoder registered and found;
  explicit decoder; failures; every BIOS glyph pixel-exact against onyx; conversion cost; font
  queries); `test/video/test_bitmask.cc` adds `row`; `test/video/test_text_layout.cc` (23 cases on a synthetic
  monospace font: empty text, newlines, `\r\n`, tabs, letter and line spacing, scale, word
  wrap, long words, trailing spaces, alignment, fit, caret round trip, hit test, UTF-8,
  missing glyphs); the builder test adds advance rounding (6.4 → 410/64) and NaN.

#### FM2 — Glyph-bank fonts

Why: game formats are the reason neutrino needs its own font type rather than onyx's;
FM2 proves the general path against the hardest formats from the survey
(`render_model.md` §9.3.1).

1. `glyph_mapping` (a function, plus a table constructor for the common case) in
   `font.hh`; `glyph_properties` / `font_properties` (F6) and `glyph_atlas` exist since FM1.
2. `font::from_glyphs(glyph_atlas, glyph_mapping)` (F3 general path): glyph ids are the
   atlas's entry ids; per-glyph `y_offset` and `bearing_x` honoured; a missing space glyph
   uses `space_advance`; unmapped code points use `default_glyph`.
3. **Coloured fonts (F4):** a bank with colour pixels makes a coloured font; `tint`
   multiplies. Colour variants via `with_palette` once [P3.6b] has it (until then, a
   variant is a second bank).
4. **Decoders fill a builder (P5):** a glyph-bank decoder takes the bytes (and, for
   palette fonts, the palette), writes every glyph into a `glyph_atlas_builder` in the
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

1. `text_params` and `text_align` (§4.2). *Proposed (from centurion / SDL_ttf's solid /
   shaded / blended modes and styles):* `background` (a box per line, for terminals and
   debug overlays), `underline` / `strikethrough` drawn by layout for every font kind, and
   `shadow` / outline as the same glyphs drawn again offset — no new atlas.
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

Why: UI text at arbitrary sizes. Revised 2026-11-08 (P4, P7, P8, P9, T1 settled): scalable
fonts arrive in two steps, and the first needs nothing new from the engine.

**Step 1 — static atlases (no engine changes):**

1. **Shared faces (P7):** `fonts_manager` keeps one face per source (onyx `font_source`,
   owning the TTF bytes / BGI strokes), referenced by every size's `font_id`; the last
   `unregister_font` frees it.
2. **Registration:** `register_font(bytes, face, font_options)` tries onyx's bitmap, then
   vector (BGI, Microsoft vector FON), then outline (TTF / OTF / TTC) decoders;
   `pixel_size` is required for scalable faces. `font_at_size(font_id, px)` makes another
   size of the same face. `register_font_decoder` gains the vector and outline overloads.
   With TTF opted out in onyx, TTF data reports "TTF support is not built in" (P4).
3. **Rasterising into the builder (T1):** `font_source::rasterize_glyph(code_point, size,
   target, x, y)` (`x` = pen, `y` = baseline) writes 8-bit coverage through onyx's
   `raster_target` concept (`put_pixel`, `width()`, `height()`; optional `put_span`). A
   small internal adaptor forwards that to an `alpha` `glyph_atlas_builder`, with the pen
   at the origin of a large virtual canvas, so nothing clips and the builder's crop gives
   the exact bounds — also for synthetic bold / italic (`rasterize_styled_glyph`), which
   draw past the metrics box. onyx's `put_span` maps onto the builder's `put_span` row
   form when that exists. (`glyph_rasterizer.hh`'s free functions are internal; not used.)
4. **What is rasterised (P8):** BGI / vector faces: every glyph, through their `charset`
   (the FON lookup). TTF: `font_options::glyphs` (default `latin1`) plus `extra_glyphs`;
   other code points draw the default glyph.
5. **Metrics:** font properties from `get_scaled_metrics(size)` (baseline = round(ascent),
   line height = round(line_height)); advances in 26.6 from `get_glyph_metrics` (P9).
   Filter: nearest for 1-bit (BGI, `monochrome`), linear for antialiased TTF.
6. Tests: a BGI glyph and a TTF glyph land entirely inside their picture, with and without
   bold / italic; Latin-1 present, a code point outside the set draws the default glyph;
   `extra_glyphs` added; `font_at_size` shares the face (one face, two atlases), freed with
   the last size; fractional advances accumulate without drift; TTF tests behind
   `ONYX_FONT_HAS_LOADER_TTF`, with a freely licensed test font (not onyx's `ARIAL.TTF`).

Exit (step 1): a TTF label and a BGI label draw correctly at two sizes.

**Step 2 — growing atlas (when a game needs CJK or typed text):**

7. **Atlas growth (engine):** a growing atlas accepts new pictures after registration,
   written into its page with a sub-rect texture upload (`SDL_UpdateTexture` on a static
   texture, never a render target); a full page adds a page; existing addresses never
   move.
8. Glyphs outside the set are rasterised on first use, during layout, before their quad
   is drawn; `font_options::glyphs` becomes the warm-up set. *Proposed (from centurion's
   `store_basic_latin_glyphs`):* named ranges next to custom strings, as in step 1.
9. Tests: glyphs appear on first use without moving earlier addresses; a page overflow adds
   a page; warm-up glyphs exist before any draw.

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
| FM4 | FM3 (step 1 needs no engine changes); atlas growth for step 2 (FM4.7) |
| FM5 | FM3; painter RM1 |
| FM6 | FM4, FM5; painter RM6 and replacement in place (`render_model.md` §8.2) |
| FM7 | FM5; painter RM3 (geometry) |

Phase A depends on neither the painter nor the sprite plan, and can run in parallel with
both.

## 8. Risks and open points

| Risk / question | Plan |
|---|---|
| Neutrino-owned layout (P1) diverges from onyx's `text_renderer` | Built on its own rules (word wrap, caret, snapping) with onyx's UTF-8 decoding; onyx's wrap breaks at any character, so results differ by design. Kerning (T4) comes later from the shared face |
| `with_palette` and `image_bank_decoder` are not implemented yet ([P3.6b]) | FM2 works without them (variants as separate banks, decoders as plain functions); adopt them when they land |
| One quad per glyph on the immediate path is slow for long text | Acceptable for Phase A — KE draws far less than its current one-point-per-pixel loop; batching comes with the painter (FM5) |
| Pixel fonts blurred by non-integer scale or position | T3: nearest filter, integer scale, snapped positions; a pixel test at scale 2 |
| FreeType size (several MB) | P4: built from source with no optional deps, so available everywhere; size-sensitive games opt out in onyx_font |
| Atlas growth touches the atlas core shared with sprites | Deferred to FM4 step 2 (P8); append-only (addresses never move), tested in isolation |
| Unicode bitmap fonts with thousands of glyphs make large static atlases | Multi-page atlases already work (F7); lazy pages only if measured |
| Fixed glyph set misses characters a game needs (P8) | `extra_glyphs`; the default glyph shows the gap; step 2's growing atlas removes the limit |
| A method call per pixel in `glyph_atlas_builder` | Resolved in FM1: writes are runs replayed into rows, `put_span` / `put_mask` for bulk producers; 20 000 glyphs build in 15–35 ms (release) |
