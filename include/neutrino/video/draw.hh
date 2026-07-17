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
 * no batching. Coordinates are integer render-space pixels (see
 * @ref geometry_types.hh).
 *
 * Every draw/get/set function returns @c sdlpp::expected<...,std::string>: on
 * success the primitive was submitted (or the state applied / value read); on
 * failure the expected carries the SDL error string. Per-function briefs below
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

#include <string>

namespace neutrino {
    /// @brief Set the renderer's current draw colour, used by all subsequent
    ///        colourless draw calls.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> set_draw_color(const sdlpp::color& c);
    /// @brief Read the renderer's current draw colour.
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::expected <sdlpp::color, std::string> get_draw_color();

    /// @brief Set the blend mode applied to subsequent draws (defaults to no blending).
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> set_draw_blend_mode(sdlpp::blend_mode mode = sdlpp::blend_mode::none);
    /// @brief Read the renderer's current draw blend mode.
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::expected <sdlpp::blend_mode, std::string> get_draw_blend_mode();

    /// @brief Set the line style (dash pattern, cap, etc.) used by subsequent line draws.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> set_line_style(const sdlpp::line_style& style);
    /// @brief Read the renderer's current line style.
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::expected <sdlpp::line_style, std::string> get_line_style();

    /// @brief Draw a single pixel at (@p x, @p y) in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_point(int x, int y);
    /// @brief Draw a single pixel at (@p x, @p y) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_point(int x, int y, const sdlpp::color& c);
    /// @brief Draw a single pixel at point @p p in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_point(const point& p);
    /// @brief Draw a single pixel at point @p p in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_point(const point& p, const sdlpp::color& c);

    /// @brief Draw a line from (@p x1, @p y1) to (@p x2, @p y2) in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(int x1, int y1, int x2, int y2);
    /// @brief Draw a line from (@p x1, @p y1) to (@p x2, @p y2) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    /// @brief Draw a line between points @p p1 and @p p2 in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(const point& p1, const point& p2);
    /// @brief Draw a line between points @p p1 and @p p2 in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(const point& p1, const point& p2, const sdlpp::color& c);
    /// @brief Draw line segment @p l (its endpoints) in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(const line& l);
    /// @brief Draw line segment @p l (its endpoints) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(const line& l, const sdlpp::color& c);

    /// @brief Draw a rectangle outline spanning the two corners (@p x1,@p y1) and
    ///        (@p x2,@p y2) (order-independent), in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect(int x1, int y1, int x2, int y2);
    /// @brief Draw a rectangle outline between corners (@p x1,@p y1) and (@p x2,@p y2) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    /// @brief Draw the outline of rectangle @p r (x, y, w, h) in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect(const rect& r);
    /// @brief Draw the outline of rectangle @p r (x, y, w, h) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect(const rect& r, const sdlpp::color& c);

    /// @brief Draw a filled rectangle spanning the two corners (@p x1,@p y1) and
    ///        (@p x2,@p y2) (order-independent), in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect_fill(int x1, int y1, int x2, int y2);
    /// @brief Draw a filled rectangle between corners (@p x1,@p y1) and (@p x2,@p y2) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect_fill(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    /// @brief Draw filled rectangle @p r (x, y, w, h) in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect_fill(const rect& r);
    /// @brief Draw filled rectangle @p r (x, y, w, h) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect_fill(const rect& r, const sdlpp::color& c);

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
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        const sprite_sheet& sheet,
        sprite_visual_id visual,
        const sprite_draw_params& params = {});

    /**
     * @brief Draw a registered sprite visual at a caller-supplied position.
     */
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        sprite_visual_ref visual,
        const sprite_draw_params& params = {});

    /**
     * @brief Draw a registered sprite visual stretched to fill an explicit
     *        destination rectangle, with optional H/V flip.
     *
     * Unlike the position+scale overloads (which round position and size
     * independently), the caller supplies the exact integer destination. This lets
     * a tile renderer derive @p dst from two rounded world corners so adjacent tiles
     * share edges exactly under fractional zoom (no 1px seams). No rotation and no
     * diagonal flip on this path; use the position overload for those. An invalid
     * visual or a non-positive @p dst is a no-op returning success.
     */
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_sprite(
        const rect& dst,
        sprite_visual_ref visual,
        sprite_flip flip = sprite_flip::none);

    /**
     * @brief Draw a sprite appearance at a caller-supplied position.
     *
     * Invisible appearances and appearances without a valid visual are skipped
     * and return success. @p params.flip composes with the appearance's own flip.
     */
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_sprite(
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
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        sprite_state_id state,
        const sprite_draw_params& params = {});

    /// @brief Draw an anti-aliased line from (@p x1,@p y1) to (@p x2,@p y2) in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(int x1, int y1, int x2, int y2);
    /// @brief Draw an anti-aliased line from (@p x1,@p y1) to (@p x2,@p y2) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    /// @brief Draw an anti-aliased line between points @p p1 and @p p2 in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(const point& p1, const point& p2);
    /// @brief Draw an anti-aliased line between points @p p1 and @p p2 in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(const point& p1, const point& p2, const sdlpp::color& c);
    /// @brief Draw anti-aliased line segment @p l in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(const line& l);
    /// @brief Draw anti-aliased line segment @p l in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(const line& l, const sdlpp::color& c);

    /// @brief Draw a line from (@p x1,@p y1) to (@p x2,@p y2) with stroke width @p w
    ///        pixels, in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(int x1, int y1, int x2, int y2, float w);
    /// @brief Draw a line of width @p w pixels between (@p x1,@p y1) and (@p x2,@p y2) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(int x1, int y1, int x2, int y2, float w, const sdlpp::color& c);
    /// @brief Draw a line of width @p w pixels between points @p p1 and @p p2 in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(const point& p1, const point& p2, float w);
    /// @brief Draw a line of width @p w pixels between points @p p1 and @p p2 in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(const point& p1, const point& p2, float w, const sdlpp::color& c);
    /// @brief Draw line segment @p l with stroke width @p w pixels in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(const line& l, float w);
    /// @brief Draw line segment @p l with stroke width @p w pixels in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(const line& l, float w, const sdlpp::color& c);

    /// @brief Draw a dashed line from @p p1 to @p p2: @p dash-pixel marks separated by
    ///        @p gap-pixel gaps, in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dashed(const point& p1, const point& p2, int dash, int gap);
    /// @brief Draw a dashed line from @p p1 to @p p2 (@p dash-px marks, @p gap-px gaps) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dashed(const point& p1, const point& p2, int dash, int gap, const sdlpp::color& c);
    /// @brief Draw dashed line segment @p l (@p dash-px marks, @p gap-px gaps) in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dashed(const line& l, int dash, int gap);
    /// @brief Draw dashed line segment @p l (@p dash-px marks, @p gap-px gaps) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dashed(const line& l, int dash, int gap, const sdlpp::color& c);

    /// @brief Draw a dotted line from @p p1 to @p p2 with dots every @p spacing pixels,
    ///        in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dotted(const point& p1, const point& p2, int spacing);
    /// @brief Draw a dotted line from @p p1 to @p p2 with dots every @p spacing pixels, in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dotted(const point& p1, const point& p2, int spacing, const sdlpp::color& c);
    /// @brief Draw dotted line segment @p l with dots every @p spacing pixels in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dotted(const line& l, int spacing);
    /// @brief Draw dotted line segment @p l with dots every @p spacing pixels in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dotted(const line& l, int spacing, const sdlpp::color& c);

    /// @brief Draw a circle outline centred at (@p x1,@p y1) with the given @p radius,
    ///        in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle(int x1, int y1, int radius);
    /// @brief Draw a circle outline centred at (@p x1,@p y1) with the given @p radius, in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle(int x1, int y1, int radius, const sdlpp::color& c);
    /// @brief Draw the outline of circle @p c (centre and radius) in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle(const circle& c);
    /// @brief Draw the outline of circle @p shape in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle(const circle& shape, const sdlpp::color& c);

    /// @brief Draw a filled disc centred at (@p x1,@p y1) with the given @p radius,
    ///        in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle_fill(int x1, int y1, int radius);
    /// @brief Draw a filled disc centred at (@p x1,@p y1) with the given @p radius, in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle_fill(int x1, int y1, int radius, const sdlpp::color& c);
    /// @brief Draw filled disc @p c (centre and radius) in the current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle_fill(const circle& c);
    /// @brief Draw filled disc @p shape in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle_fill(const circle& shape, const sdlpp::color& c);

    /// @brief Draw an arrow from @p from to @p to in the current draw colour.
    ///        @p head_size is the arrowhead length in pixels, @p head_angle its
    ///        half-angle in degrees, @p thickness the stroke width in pixels.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const point& from, const point& to, int head_size = 8, float head_angle = 30.0f, float thickness = 1.0f);
    /// @brief Draw an arrow from @p from to @p to in colour @p c (@p head_size px head length,
    ///        @p head_angle deg half-angle; default thickness).
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const point& from, const point& to, int head_size, float head_angle, const sdlpp::color& c);
    /// @brief Draw an arrow from @p from to @p to in colour @p c with an explicit @p thickness (px).
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const point& from, const point& to, int head_size, float head_angle, float thickness, const sdlpp::color& c);
    /// @brief Draw an arrow along segment @p l (start -> end) in the current draw colour;
    ///        @p head_size / @p head_angle / @p thickness as above.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const line& l, int head_size = 8, float head_angle = 30.0f, float thickness = 1.0f);
    /// @brief Draw an arrow along segment @p l in colour @p c (default thickness).
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const line& l, int head_size, float head_angle, const sdlpp::color& c);
    /// @brief Draw an arrow along segment @p l in colour @p c with an explicit @p thickness (px).
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const line& l, int head_size, float head_angle, float thickness, const sdlpp::color& c);

    /// @brief Draw a small plus/cross centred at @p center; @p size is each arm's
    ///        half-length in pixels, @p thickness the stroke width; current draw colour.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_cross(const point& center, int size = 5, float thickness = 1.0f);
    /// @brief Draw a cross centred at @p center with arm half-length @p size (px), in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_cross(const point& center, int size, const sdlpp::color& c);
    /// @brief Draw a cross centred at @p center (arm half-length @p size px, stroke @p thickness px) in colour @p c.
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_cross(const point& center, int size, float thickness, const sdlpp::color& c);
}
