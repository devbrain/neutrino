//
// Created by igor on 03/07/2026.
//

#pragma once
#include <neutrino/video/globals.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprite/sprite_state.hh>
#include <neutrino/neutrino_export.h>

#include <string>

namespace neutrino {
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> set_draw_color(const sdlpp::color& c);
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::expected <sdlpp::color, std::string> get_draw_color();

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> set_draw_blend_mode(sdlpp::blend_mode mode = sdlpp::blend_mode::none);
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::expected <sdlpp::blend_mode, std::string> get_draw_blend_mode();

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> set_line_style(const sdlpp::line_style& style);
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::expected <sdlpp::line_style, std::string> get_line_style();

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_point(int x, int y);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_point(int x, int y, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_point(const point& p);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_point(const point& p, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(int x1, int y1, int x2, int y2);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(const point& p1, const point& p2);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(const point& p1, const point& p2, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(const line& l);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line(const line& l, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect(int x1, int y1, int x2, int y2);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect(const rect& r);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect(const rect& r, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect_fill(int x1, int y1, int x2, int y2);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect_fill(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect_fill(const rect& r);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_rect_fill(const rect& r, const sdlpp::color& c);

    /**
     * @brief Draw-time render transform shared by every draw_sprite overload.
     *
     * @ref scale is applied around the visual origin and must be finite and
     * greater than zero. @ref flip composes with a drawn appearance's intrinsic
     * flip by toggling flags (each flip is an involution), so drawing a
     * left-authored frame with a horizontal draw flip renders it facing right.
     * Diagonal flips use Tiled-compatible semantics. The params never mutate the
     * sprite sheet, visual, appearance, or runtime sprite state.
     */
    struct sprite_draw_params {
        float scale{1.0f};
        sprite_flip flip{sprite_flip::none};
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

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(int x1, int y1, int x2, int y2);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(int x1, int y1, int x2, int y2, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(const point& p1, const point& p2);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(const point& p1, const point& p2, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(const line& l);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_aa(const line& l, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(int x1, int y1, int x2, int y2, float w);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(int x1, int y1, int x2, int y2, float w, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(const point& p1, const point& p2, float w);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(const point& p1, const point& p2, float w, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(const line& l, float w);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_thick(const line& l, float w, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dashed(const point& p1, const point& p2, int dash, int gap);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dashed(const point& p1, const point& p2, int dash, int gap, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dashed(const line& l, int dash, int gap);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dashed(const line& l, int dash, int gap, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dotted(const point& p1, const point& p2, int spacing);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dotted(const point& p1, const point& p2, int spacing, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dotted(const line& l, int spacing);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_line_dotted(const line& l, int spacing, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle(int x1, int y1, int radius);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle(int x1, int y1, int radius, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle(const circle& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle(const circle& shape, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle_fill(int x1, int y1, int radius);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle_fill(int x1, int y1, int radius, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle_fill(const circle& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_circle_fill(const circle& shape, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const point& from, const point& to, int head_size = 8, float head_angle = 30.0f, float thickness = 1.0f);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const point& from, const point& to, int head_size, float head_angle, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const point& from, const point& to, int head_size, float head_angle, float thickness, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const line& l, int head_size = 8, float head_angle = 30.0f, float thickness = 1.0f);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const line& l, int head_size, float head_angle, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_arrow(const line& l, int head_size, float head_angle, float thickness, const sdlpp::color& c);

    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_cross(const point& center, int size = 5, float thickness = 1.0f);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_cross(const point& center, int size, const sdlpp::color& c);
    NEUTRINO_EXPORT sdlpp::expected <void, std::string> draw_cross(const point& center, int size, float thickness, const sdlpp::color& c);
}
