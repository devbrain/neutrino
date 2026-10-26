//
// Created by igor on 03/07/2026.
//

#pragma once

/**
 * @file draw.hh
 * @brief Immediate-mode 2D primitive drawing on the active renderer.
 *
 * Free functions that submit points, lines, rectangles, circles, arrows, crosses,
 * and sprites straight to @ref get_renderer() in call order -- no retained scene,
 * no batching. Coordinates are render-space pixels: integer @c point / @c rect
 * overloads, and float @c render_pos / @c render_rect overloads (see the section at the
 * end). Both reach SDL3 as floats; the pixel_snap policy (globals.hh) decides whether
 * fractions are rounded to whole logical pixels first.
 *
 * Drawing is fire-and-forget: draw and set functions return @c void. Mistakes in
 * the arguments (a non-finite or non-positive sprite scale, a non-finite rotation)
 * throw. A failure inside SDL itself (a lost renderer, out of memory while
 * queuing) can't be handled per call, so it is logged -- rate-limited per call
 * site -- and counted in @ref draw_failure_count(). Getters return the value
 * directly and throw if the renderer cannot report it. Per-function briefs below
 * do not restate this and instead describe *what* is drawn and what the
 * parameters mean.
 *
 * Overloads that take no @c sdlpp::color draw with the renderer's current draw
 * colour; overloads that take a trailing @c color set that colour for the single
 * call and restore the previous draw colour afterwards.
 */

#include <neutrino/video/globals.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprite/sprite_state.hh>
#include <neutrino/neutrino_export.h>

#include <concepts>
#include <cstdint>

namespace neutrino {
    /// @brief Set the renderer's current draw colour, used by all subsequent
    ///        colourless draw calls.
    NEUTRINO_EXPORT void set_draw_color(const sdlpp::color& c);
    /// @brief Read the renderer's current draw colour.
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::color get_draw_color();

    /// @brief Set the blend mode applied to subsequent draws (defaults to no blending).
    NEUTRINO_EXPORT void set_draw_blend_mode(sdlpp::blend_mode mode = sdlpp::blend_mode::none);
    /// @brief Read the renderer's current draw blend mode.
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::blend_mode get_draw_blend_mode();

    /// @brief Set the line style (dash pattern, cap, etc.) used by subsequent line draws.
    NEUTRINO_EXPORT void set_line_style(const sdlpp::line_style& style);
    /// @brief Read the renderer's current line style.
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::line_style get_line_style();

    /// @brief Total draw calls that SDL rejected since startup (monotonic). Tests and tools take
    ///        the difference across a span of draws; 0 new failures means everything was submitted.
    NEUTRINO_EXPORT [[nodiscard]] std::uint64_t draw_failure_count() noexcept;

    /// @brief Draw a single pixel at (@p x, @p y) in the current draw colour.
    NEUTRINO_EXPORT void draw_point(int x, int y);
    /// @brief Draw a single pixel at (@p x, @p y) in colour @p c.
    NEUTRINO_EXPORT void draw_point(int x, int y, const sdlpp::color& c);
    /// @brief Draw a single pixel at point @p p in the current draw colour.
    NEUTRINO_EXPORT void draw_point(const point& p);
    /// @brief Draw a single pixel at point @p p in colour @p c.
    NEUTRINO_EXPORT void draw_point(const point& p, const sdlpp::color& c);

    /// @brief Draw a line from (@p x1, @p y1) to (@p x2, @p y2) in the current draw colour.
    NEUTRINO_EXPORT void draw_line(int x1, int y1, int x2, int y2);
    /// @brief Draw a line from (@p x1, @p y1) to (@p x2, @p y2) in colour @p c.
    NEUTRINO_EXPORT void draw_line(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    /// @brief Draw a line between points @p p1 and @p p2 in the current draw colour.
    NEUTRINO_EXPORT void draw_line(const point& p1, const point& p2);
    /// @brief Draw a line between points @p p1 and @p p2 in colour @p c.
    NEUTRINO_EXPORT void draw_line(const point& p1, const point& p2, const sdlpp::color& c);
    /// @brief Draw line segment @p l (its endpoints) in the current draw colour.
    NEUTRINO_EXPORT void draw_line(const line& l);
    /// @brief Draw line segment @p l (its endpoints) in colour @p c.
    NEUTRINO_EXPORT void draw_line(const line& l, const sdlpp::color& c);

    /// @brief Draw a rectangle outline spanning the two corners (@p x1,@p y1) and
    ///        (@p x2,@p y2) (order-independent), in the current draw colour.
    NEUTRINO_EXPORT void draw_rect(int x1, int y1, int x2, int y2);
    /// @brief Draw a rectangle outline between corners (@p x1,@p y1) and (@p x2,@p y2) in colour @p c.
    NEUTRINO_EXPORT void draw_rect(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    /// @brief Draw the outline of rectangle @p r (x, y, w, h) in the current draw colour.
    NEUTRINO_EXPORT void draw_rect(const rect& r);
    /// @brief Draw the outline of rectangle @p r (x, y, w, h) in colour @p c.
    NEUTRINO_EXPORT void draw_rect(const rect& r, const sdlpp::color& c);

    /// @brief Draw a filled rectangle spanning the two corners (@p x1,@p y1) and
    ///        (@p x2,@p y2) (order-independent), in the current draw colour.
    NEUTRINO_EXPORT void draw_rect_fill(int x1, int y1, int x2, int y2);
    /// @brief Draw a filled rectangle between corners (@p x1,@p y1) and (@p x2,@p y2) in colour @p c.
    NEUTRINO_EXPORT void draw_rect_fill(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    /// @brief Draw filled rectangle @p r (x, y, w, h) in the current draw colour.
    NEUTRINO_EXPORT void draw_rect_fill(const rect& r);
    /// @brief Draw filled rectangle @p r (x, y, w, h) in colour @p c.
    NEUTRINO_EXPORT void draw_rect_fill(const rect& r, const sdlpp::color& c);

    /**
     * @brief Draw-time render transform shared by every draw_sprite overload.
     *
     * @ref scale is applied around the visual origin and must be finite and
     * greater than zero. @ref flip composes with a drawn appearance's intrinsic
     * flip by toggling flags (each flip is an involution), so drawing a
     * left-authored frame with a horizontal draw flip renders it facing right.
     * Diagonal flips use Tiled-compatible semantics. @ref rotation_degrees rotates
     * the drawn sprite clockwise (screen space) about the visual origin/anchor,
     * applied on top of scale and flip; it must be finite. 0 means no rotation and
     * renders byte-identically to omitting it (e.g. a TMX hex-120 tile is
     * rotation_degrees = 120 composed with the cell flip). The params never mutate
     * the sprite sheet, visual, appearance, or runtime sprite state.
     */
    struct sprite_draw_params {
        float scale{1.0f};
        sprite_flip flip{sprite_flip::none};
        float rotation_degrees{0.0f};
    };

    /**
     * @brief Draw a sheet visual at a caller-supplied position.
     *
     * @p position is the visual origin/anchor in render coordinates. The sprite
     * system does not store world position; callers provide it from their own
     * transform/scene/physics layer.
     */
    NEUTRINO_EXPORT void draw_sprite(
        const point& position,
        const sprite_sheet& sheet,
        sprite_visual_id visual,
        const sprite_draw_params& params = {});

    /**
     * @brief Draw a registered sprite visual at a caller-supplied position.
     */
    NEUTRINO_EXPORT void draw_sprite(
        const point& position,
        sprite_visual_ref visual,
        const sprite_draw_params& params = {});

    /**
     * @brief Draw a registered sprite visual stretched to fill an explicit
     *        destination rectangle, with optional H/V flip.
     *
     * Unlike the position+scale overloads (which compute position and size
     * independently, and round each separately under pixel_snap::logical), the
     * caller supplies the exact integer destination. This lets
     * a tile renderer derive @p dst from two rounded world corners so adjacent tiles
     * share edges exactly under fractional zoom (no 1px seams). No rotation and no
     * diagonal flip on this path; use the position overload for those. An invalid
     * visual or a non-positive @p dst is a no-op.
     */
    NEUTRINO_EXPORT void draw_sprite(
        const rect& dst,
        sprite_visual_ref visual,
        sprite_flip flip = sprite_flip::none);

    /**
     * @brief Draw a sprite appearance at a caller-supplied position.
     *
     * Invisible appearances and appearances without a valid visual are skipped.
     * @p params.flip composes with the appearance's own flip.
     */
    NEUTRINO_EXPORT void draw_sprite(
        const point& position,
        const sprite_appearance& appearance,
        const sprite_draw_params& params = {});

    /**
     * @brief Draw a runtime sprite state at a caller-supplied position.
     *
     * The current appearance is resolved from the internal sprite manager, so
     * animated states advance automatically with the application update loop.
     * @p params.flip composes with the resolved appearance's own flip.
     */
    NEUTRINO_EXPORT void draw_sprite(
        const point& position,
        sprite_state_id state,
        const sprite_draw_params& params = {});

    /// @brief Draw an anti-aliased line from (@p x1,@p y1) to (@p x2,@p y2) in the current draw colour.
    NEUTRINO_EXPORT void draw_line_aa(int x1, int y1, int x2, int y2);
    /// @brief Draw an anti-aliased line from (@p x1,@p y1) to (@p x2,@p y2) in colour @p c.
    NEUTRINO_EXPORT void draw_line_aa(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    /// @brief Draw an anti-aliased line between points @p p1 and @p p2 in the current draw colour.
    NEUTRINO_EXPORT void draw_line_aa(const point& p1, const point& p2);
    /// @brief Draw an anti-aliased line between points @p p1 and @p p2 in colour @p c.
    NEUTRINO_EXPORT void draw_line_aa(const point& p1, const point& p2, const sdlpp::color& c);
    /// @brief Draw anti-aliased line segment @p l in the current draw colour.
    NEUTRINO_EXPORT void draw_line_aa(const line& l);
    /// @brief Draw anti-aliased line segment @p l in colour @p c.
    NEUTRINO_EXPORT void draw_line_aa(const line& l, const sdlpp::color& c);

    /// @brief Draw a line from (@p x1,@p y1) to (@p x2,@p y2) with stroke width @p w
    ///        pixels, in the current draw colour.
    NEUTRINO_EXPORT void draw_line_thick(int x1, int y1, int x2, int y2, float w);
    /// @brief Draw a line of width @p w pixels between (@p x1,@p y1) and (@p x2,@p y2) in colour @p c.
    NEUTRINO_EXPORT void draw_line_thick(int x1, int y1, int x2, int y2, float w, const sdlpp::color& c);
    /// @brief Draw a line of width @p w pixels between points @p p1 and @p p2 in the current draw colour.
    NEUTRINO_EXPORT void draw_line_thick(const point& p1, const point& p2, float w);
    /// @brief Draw a line of width @p w pixels between points @p p1 and @p p2 in colour @p c.
    NEUTRINO_EXPORT void draw_line_thick(const point& p1, const point& p2, float w, const sdlpp::color& c);
    /// @brief Draw line segment @p l with stroke width @p w pixels in the current draw colour.
    NEUTRINO_EXPORT void draw_line_thick(const line& l, float w);
    /// @brief Draw line segment @p l with stroke width @p w pixels in colour @p c.
    NEUTRINO_EXPORT void draw_line_thick(const line& l, float w, const sdlpp::color& c);

    /// @brief Draw a dashed line from @p p1 to @p p2: @p dash-pixel marks separated by
    ///        @p gap-pixel gaps, in the current draw colour.
    NEUTRINO_EXPORT void draw_line_dashed(const point& p1, const point& p2, int dash, int gap);
    /// @brief Draw a dashed line from @p p1 to @p p2 (@p dash-px marks, @p gap-px gaps) in colour @p c.
    NEUTRINO_EXPORT void draw_line_dashed(const point& p1, const point& p2, int dash, int gap, const sdlpp::color& c);
    /// @brief Draw dashed line segment @p l (@p dash-px marks, @p gap-px gaps) in the current draw colour.
    NEUTRINO_EXPORT void draw_line_dashed(const line& l, int dash, int gap);
    /// @brief Draw dashed line segment @p l (@p dash-px marks, @p gap-px gaps) in colour @p c.
    NEUTRINO_EXPORT void draw_line_dashed(const line& l, int dash, int gap, const sdlpp::color& c);

    /// @brief Draw a dotted line from @p p1 to @p p2 with dots every @p spacing pixels,
    ///        in the current draw colour.
    NEUTRINO_EXPORT void draw_line_dotted(const point& p1, const point& p2, int spacing);
    /// @brief Draw a dotted line from @p p1 to @p p2 with dots every @p spacing pixels, in colour @p c.
    NEUTRINO_EXPORT void draw_line_dotted(const point& p1, const point& p2, int spacing, const sdlpp::color& c);
    /// @brief Draw dotted line segment @p l with dots every @p spacing pixels in the current draw colour.
    NEUTRINO_EXPORT void draw_line_dotted(const line& l, int spacing);
    /// @brief Draw dotted line segment @p l with dots every @p spacing pixels in colour @p c.
    NEUTRINO_EXPORT void draw_line_dotted(const line& l, int spacing, const sdlpp::color& c);

    /// @brief Draw a circle outline centred at (@p x1,@p y1) with the given @p radius,
    ///        in the current draw colour.
    NEUTRINO_EXPORT void draw_circle(int x1, int y1, int radius);
    /// @brief Draw a circle outline centred at (@p x1,@p y1) with the given @p radius, in colour @p c.
    NEUTRINO_EXPORT void draw_circle(int x1, int y1, int radius, const sdlpp::color& c);
    /// @brief Draw the outline of circle @p c (centre and radius) in the current draw colour.
    NEUTRINO_EXPORT void draw_circle(const circle& c);
    /// @brief Draw the outline of circle @p shape in colour @p c.
    NEUTRINO_EXPORT void draw_circle(const circle& shape, const sdlpp::color& c);

    /// @brief Draw a filled disc centred at (@p x1,@p y1) with the given @p radius,
    ///        in the current draw colour.
    NEUTRINO_EXPORT void draw_circle_fill(int x1, int y1, int radius);
    /// @brief Draw a filled disc centred at (@p x1,@p y1) with the given @p radius, in colour @p c.
    NEUTRINO_EXPORT void draw_circle_fill(int x1, int y1, int radius, const sdlpp::color& c);
    /// @brief Draw filled disc @p c (centre and radius) in the current draw colour.
    NEUTRINO_EXPORT void draw_circle_fill(const circle& c);
    /// @brief Draw filled disc @p shape in colour @p c.
    NEUTRINO_EXPORT void draw_circle_fill(const circle& shape, const sdlpp::color& c);

    /// @brief Draw an arrow from @p from to @p to in the current draw colour.
    ///        @p head_size is the arrowhead length in pixels, @p head_angle its
    ///        half-angle in degrees, @p thickness the stroke width in pixels.
    NEUTRINO_EXPORT void draw_arrow(const point& from, const point& to, int head_size = 8, float head_angle = 30.0f, float thickness = 1.0f);
    /// @brief Draw an arrow from @p from to @p to in colour @p c (@p head_size px head length,
    ///        @p head_angle deg half-angle; default thickness).
    NEUTRINO_EXPORT void draw_arrow(const point& from, const point& to, int head_size, float head_angle, const sdlpp::color& c);
    /// @brief Draw an arrow from @p from to @p to in colour @p c with an explicit @p thickness (px).
    NEUTRINO_EXPORT void draw_arrow(const point& from, const point& to, int head_size, float head_angle, float thickness, const sdlpp::color& c);
    /// @brief Draw an arrow along segment @p l (start -> end) in the current draw colour;
    ///        @p head_size / @p head_angle / @p thickness as above.
    NEUTRINO_EXPORT void draw_arrow(const line& l, int head_size = 8, float head_angle = 30.0f, float thickness = 1.0f);
    /// @brief Draw an arrow along segment @p l in colour @p c (default thickness).
    NEUTRINO_EXPORT void draw_arrow(const line& l, int head_size, float head_angle, const sdlpp::color& c);
    /// @brief Draw an arrow along segment @p l in colour @p c with an explicit @p thickness (px).
    NEUTRINO_EXPORT void draw_arrow(const line& l, int head_size, float head_angle, float thickness, const sdlpp::color& c);

    /// @brief Draw a small plus/cross centred at @p center; @p size is each arm's
    ///        half-length in pixels, @p thickness the stroke width; current draw colour.
    NEUTRINO_EXPORT void draw_cross(const point& center, int size = 5, float thickness = 1.0f);
    /// @brief Draw a cross centred at @p center with arm half-length @p size (px), in colour @p c.
    NEUTRINO_EXPORT void draw_cross(const point& center, int size, const sdlpp::color& c);
    /// @brief Draw a cross centred at @p center (arm half-length @p size px, stroke @p thickness px) in colour @p c.
    NEUTRINO_EXPORT void draw_cross(const point& center, int size, float thickness, const sdlpp::color& c);

    // ---- float render-space overloads -----------------------------------------------------------
    // The float forms are the real implementations; every integer overload above forwards to them.
    // Coordinates pass to SDL3 as floats unless the pixel_snap policy (globals.hh) rounds them.
    //
    // Constrained TEMPLATES rather than plain overloads, as in globals.hh: a braced `{x, y}`
    // initializes both `point` and `render_pos`, so a plain render_pos overload would make the
    // long-legal `draw_point({x, y})` ambiguous. A braced list is a non-deduced context, so these
    // templates are never candidates for it and `{x, y}` keeps selecting the integer overload.
    // Spell the float form `render_pos{x, y}` / `render_rect{x, y, w, h}`.

    namespace detail {
        NEUTRINO_EXPORT void draw_point(render_pos p);
        NEUTRINO_EXPORT void draw_point(render_pos p, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_line(render_pos a, render_pos b);
        NEUTRINO_EXPORT void draw_line(render_pos a, render_pos b, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_line_aa(render_pos a, render_pos b);
        NEUTRINO_EXPORT void draw_line_aa(render_pos a, render_pos b, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_line_thick(render_pos a, render_pos b, float w);
        NEUTRINO_EXPORT void draw_line_thick(render_pos a, render_pos b, float w, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_line_dashed(render_pos a, render_pos b, float dash, float gap);
        NEUTRINO_EXPORT void draw_line_dashed(render_pos a, render_pos b, float dash, float gap,
                                              const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_line_dotted(render_pos a, render_pos b, float spacing);
        NEUTRINO_EXPORT void draw_line_dotted(render_pos a, render_pos b, float spacing, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_arrow(render_pos from, render_pos to, float head_size, float head_angle,
                                        float thickness);
        NEUTRINO_EXPORT void draw_arrow(render_pos from, render_pos to, float head_size, float head_angle,
                                        float thickness, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_cross(render_pos centre, float size, float thickness);
        NEUTRINO_EXPORT void draw_cross(render_pos centre, float size, float thickness, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_rect(const render_rect& r);
        NEUTRINO_EXPORT void draw_rect(const render_rect& r, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_rect_fill(const render_rect& r);
        NEUTRINO_EXPORT void draw_rect_fill(const render_rect& r, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_circle(render_pos centre, float radius);
        NEUTRINO_EXPORT void draw_circle(render_pos centre, float radius, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_circle_fill(render_pos centre, float radius);
        NEUTRINO_EXPORT void draw_circle_fill(render_pos centre, float radius, const sdlpp::color& c);
        NEUTRINO_EXPORT void draw_sprite(render_pos position, const sprite_sheet& sheet,
                                         sprite_visual_id visual, const sprite_draw_params& params);
        NEUTRINO_EXPORT void draw_sprite(render_pos position, sprite_visual_ref visual,
                                         const sprite_draw_params& params);
        NEUTRINO_EXPORT void draw_sprite(render_pos position, const sprite_appearance& appearance,
                                         const sprite_draw_params& params);
        NEUTRINO_EXPORT void draw_sprite(render_pos position, sprite_state_id state,
                                         const sprite_draw_params& params);
    }

    /// @brief Draw a single pixel at render position @p p in the current draw colour.
    template<std::same_as <render_pos> P>
    void draw_point(P p) { detail::draw_point(p); }
    /// @brief Draw a single pixel at render position @p p in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_point(P p, const sdlpp::color& c) { detail::draw_point(p, c); }

    /// @brief Draw a line between render positions @p a and @p b in the current draw colour.
    template<std::same_as <render_pos> P>
    void draw_line(P a, P b) { detail::draw_line(a, b); }
    /// @brief Draw a line between render positions @p a and @p b in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_line(P a, P b, const sdlpp::color& c) { detail::draw_line(a, b, c); }

    /// @brief Draw an anti-aliased line between render positions @p a and @p b in the current draw colour.
    template<std::same_as <render_pos> P>
    void draw_line_aa(P a, P b) { detail::draw_line_aa(a, b); }
    /// @brief Draw an anti-aliased line between render positions @p a and @p b in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_line_aa(P a, P b, const sdlpp::color& c) { detail::draw_line_aa(a, b, c); }

    /// @brief Draw a line of width @p w between render positions @p a and @p b in the current draw colour.
    template<std::same_as <render_pos> P>
    void draw_line_thick(P a, P b, float w) { detail::draw_line_thick(a, b, w); }
    /// @brief Draw a line of width @p w between render positions @p a and @p b in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_line_thick(P a, P b, float w, const sdlpp::color& c) { detail::draw_line_thick(a, b, w, c); }

    /// @brief Draw a dashed line (@p dash on, @p gap off) between render positions in the current draw colour.
    template<std::same_as <render_pos> P>
    void draw_line_dashed(P a, P b, float dash, float gap) { detail::draw_line_dashed(a, b, dash, gap); }
    /// @brief Draw a dashed line (@p dash on, @p gap off) between render positions in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_line_dashed(P a, P b, float dash, float gap, const sdlpp::color& c) {
        detail::draw_line_dashed(a, b, dash, gap, c);
    }

    /// @brief Draw a dotted line (dots @p spacing apart) between render positions in the current draw colour.
    template<std::same_as <render_pos> P>
    void draw_line_dotted(P a, P b, float spacing) { detail::draw_line_dotted(a, b, spacing); }
    /// @brief Draw a dotted line (dots @p spacing apart) between render positions in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_line_dotted(P a, P b, float spacing, const sdlpp::color& c) {
        detail::draw_line_dotted(a, b, spacing, c);
    }

    /// @brief Draw an arrow from @p from to @p to (head length @p head_size, half-angle @p head_angle degrees)
    ///        in the current draw colour.
    template<std::same_as <render_pos> P>
    void draw_arrow(P from, P to, float head_size = 8.0f, float head_angle = 30.0f, float thickness = 1.0f) {
        detail::draw_arrow(from, to, head_size, head_angle, thickness);
    }
    /// @brief Draw a 1px arrow from @p from to @p to in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_arrow(P from, P to, float head_size, float head_angle, const sdlpp::color& c) {
        detail::draw_arrow(from, to, head_size, head_angle, 1.0f, c);
    }
    /// @brief Draw an arrow from @p from to @p to with stroke @p thickness in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_arrow(P from, P to, float head_size, float head_angle, float thickness, const sdlpp::color& c) {
        detail::draw_arrow(from, to, head_size, head_angle, thickness, c);
    }

    /// @brief Draw a cross centred at @p centre (arm half-length @p size) in the current draw colour.
    template<std::same_as <render_pos> P>
    void draw_cross(P centre, float size = 5.0f, float thickness = 1.0f) {
        detail::draw_cross(centre, size, thickness);
    }
    /// @brief Draw a 1px cross centred at @p centre in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_cross(P centre, float size, const sdlpp::color& c) { detail::draw_cross(centre, size, 1.0f, c); }
    /// @brief Draw a cross centred at @p centre with stroke @p thickness in colour @p c.
    template<std::same_as <render_pos> P>
    void draw_cross(P centre, float size, float thickness, const sdlpp::color& c) {
        detail::draw_cross(centre, size, thickness, c);
    }

    /// @brief Draw the outline of render rectangle @p r in the current draw colour.
    template<std::same_as <render_rect> R>
    void draw_rect(const R& r) { detail::draw_rect(r); }
    /// @brief Draw the outline of render rectangle @p r in colour @p c.
    template<std::same_as <render_rect> R>
    void draw_rect(const R& r, const sdlpp::color& c) { detail::draw_rect(r, c); }

    /// @brief Fill render rectangle @p r with the current draw colour.
    template<std::same_as <render_rect> R>
    void draw_rect_fill(const R& r) { detail::draw_rect_fill(r); }
    /// @brief Fill render rectangle @p r with colour @p c.
    template<std::same_as <render_rect> R>
    void draw_rect_fill(const R& r, const sdlpp::color& c) { detail::draw_rect_fill(r, c); }

    /// @brief Draw a circle outline. The circle rasterizer is integer-only, so @p centre and
    ///        @p radius are rounded to whole pixels regardless of the pixel_snap policy.
    template<std::same_as <render_pos> P>
    void draw_circle(P centre, float radius) { detail::draw_circle(centre, radius); }
    /// @copydoc draw_circle(P, float)
    template<std::same_as <render_pos> P>
    void draw_circle(P centre, float radius, const sdlpp::color& c) { detail::draw_circle(centre, radius, c); }

    /// @brief Draw a filled circle. Rounded to whole pixels like draw_circle(P, float).
    template<std::same_as <render_pos> P>
    void draw_circle_fill(P centre, float radius) { detail::draw_circle_fill(centre, radius); }
    /// @copydoc draw_circle_fill(P, float)
    template<std::same_as <render_pos> P>
    void draw_circle_fill(P centre, float radius, const sdlpp::color& c) {
        detail::draw_circle_fill(centre, radius, c);
    }

    /// @brief Draw a sheet visual with its origin/anchor at render position @p position.
    ///        Under pixel_snap::off the origin and size are not rounded, so a fractional
    ///        @c params.scale yields fractional extents.
    template<std::same_as <render_pos> P>
    void draw_sprite(P position, const sprite_sheet& sheet, sprite_visual_id visual,
                     const sprite_draw_params& params = {}) {
        detail::draw_sprite(position, sheet, visual, params);
    }
    /// @brief Draw a registered sprite visual at render position @p position.
    template<std::same_as <render_pos> P>
    void draw_sprite(P position, sprite_visual_ref visual, const sprite_draw_params& params = {}) {
        detail::draw_sprite(position, visual, params);
    }
    /// @brief Draw a sprite appearance at render position @p position (skipped if invisible).
    template<std::same_as <render_pos> P>
    void draw_sprite(P position, const sprite_appearance& appearance, const sprite_draw_params& params = {}) {
        detail::draw_sprite(position, appearance, params);
    }
    /// @brief Draw a runtime sprite state at render position @p position.
    template<std::same_as <render_pos> P>
    void draw_sprite(P position, sprite_state_id state, const sprite_draw_params& params = {}) {
        detail::draw_sprite(position, state, params);
    }
}
