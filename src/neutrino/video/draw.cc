//
// Created by igor on 03/07/2026.
//

#include <neutrino/video/draw.hh>
#include <failsafe/enforce.hh>
#include <sdlpp/video/renderer.hh>
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

#include "services/service_locator.hh"

namespace neutrino {
    namespace {
        [[nodiscard]] bool has_flip(sprite_flip flags, sprite_flip flag) noexcept {
            return static_cast <std::uint8_t>(flags & flag) != 0;
        }

        [[nodiscard]] bool has_diagonal_flip(sprite_flip flags) noexcept {
            return has_flip(flags, sprite_flip::diagonal);
        }

        [[nodiscard]] sdlpp::flip_mode to_sdl_flip(sprite_flip flags) noexcept {
            auto result = sdlpp::flip_mode::none;
            if (has_flip(flags, sprite_flip::horizontal)) {
                result |= sdlpp::flip_mode::horizontal;
            }
            if (has_flip(flags, sprite_flip::vertical)) {
                result |= sdlpp::flip_mode::vertical;
            }
            return result;
        }

        [[nodiscard]] point transformed_origin(const sprite_visual& visual, sprite_flip flip) noexcept {
            point origin = visual.origin;
            if (has_flip(flip, sprite_flip::horizontal)) {
                origin.x = visual.texture_rect.w - origin.x;
            }
            if (has_flip(flip, sprite_flip::vertical)) {
                origin.y = visual.texture_rect.h - origin.y;
            }
            return origin;
        }

        [[nodiscard]] int scaled_offset(int value, float scale) noexcept {
            return static_cast <int>(std::lround(static_cast <float>(value) * scale));
        }

        [[nodiscard]] int scaled_extent(int value, float scale) noexcept {
            return std::max(1, scaled_offset(value, scale));
        }

        struct local_point {
            float x{0};
            float y{0};
        };

        [[nodiscard]] local_point transform_local_tiled(local_point p, float width, float height, sprite_flip flip) noexcept {
            // Match Tiled orthogonal/isometric GID flags: diagonal first, then H/V.
            // The diagonal flag swaps bottom-left and top-right corners (x/y axis swap).
            if (has_flip(flip, sprite_flip::diagonal)) {
                std::swap(p.x, p.y);
                std::swap(width, height);
            }
            if (has_flip(flip, sprite_flip::horizontal)) {
                p.x = width - p.x;
            }
            if (has_flip(flip, sprite_flip::vertical)) {
                p.y = height - p.y;
            }
            return p;
        }

        [[nodiscard]] SDL_Vertex make_sprite_vertex(
            local_point local,
            local_point anchor,
            const point& position,
            float scale,
            SDL_FPoint tex_coord) noexcept {
            return SDL_Vertex{
                .position = {
                    static_cast <float>(position.x) + (local.x - anchor.x) * scale,
                    static_cast <float>(position.y) + (local.y - anchor.y) * scale
                },
                .color = {1.0f, 1.0f, 1.0f, 1.0f},
                .tex_coord = tex_coord
            };
        }

        sdlpp::expected <void, std::string> draw_sprite_diagonal(
            sdlpp::renderer& renderer,
            const sdlpp::texture& texture,
            const sprite_visual& visual,
            const point& position,
            sprite_flip flip,
            float scale) {
            const auto texture_size = texture.get_size();
            if (!texture_size) {
                return sdlpp::make_unexpectedf(texture_size.error());
            }

            const auto& src = visual.texture_rect;
            const float texture_width = static_cast <float>(texture_size->width);
            const float texture_height = static_cast <float>(texture_size->height);
            const float u0 = static_cast <float>(src.x) / texture_width;
            const float v0 = static_cast <float>(src.y) / texture_height;
            const float u1 = static_cast <float>(src.x + src.w) / texture_width;
            const float v1 = static_cast <float>(src.y + src.h) / texture_height;
            const float width = static_cast <float>(src.w);
            const float height = static_cast <float>(src.h);

            const auto anchor = transform_local_tiled(
                local_point{static_cast <float>(visual.origin.x), static_cast <float>(visual.origin.y)},
                width,
                height,
                flip);

            const std::array <SDL_Vertex, 4> vertices{
                make_sprite_vertex(transform_local_tiled({0.0f, 0.0f}, width, height, flip), anchor, position, scale, {u0, v0}),
                make_sprite_vertex(transform_local_tiled({width, 0.0f}, width, height, flip), anchor, position, scale, {u1, v0}),
                make_sprite_vertex(transform_local_tiled({width, height}, width, height, flip), anchor, position, scale, {u1, v1}),
                make_sprite_vertex(transform_local_tiled({0.0f, height}, width, height, flip), anchor, position, scale, {u0, v1})
            };
            constexpr std::array <int, 6> indices{0, 1, 2, 0, 2, 3};

            return renderer.render_geometry(texture.get(), vertices, indices);
        }
    }

    sdlpp::expected<void, std::string> set_draw_color(const sdlpp::color& c) {
        return get_renderer().set_draw_color(c);
    }

    sdlpp::expected<sdlpp::color, std::string> get_draw_color() {
        return get_renderer().get_draw_color();
    }

    sdlpp::expected<void, std::string> set_draw_blend_mode(sdlpp::blend_mode mode) {
        return get_renderer().set_draw_blend_mode(mode);
    }

    sdlpp::expected<sdlpp::blend_mode, std::string> get_draw_blend_mode() {
        return get_renderer().get_draw_blend_mode();
    }

    sdlpp::expected<void, std::string> set_line_style(const sdlpp::line_style& style) {
        return get_renderer().set_line_style(style);
    }

    sdlpp::expected<sdlpp::line_style, std::string> get_line_style() {
        return get_renderer().get_line_style();
    }

    // --- Points ---

    sdlpp::expected<void, std::string> draw_point(int x, int y) {
        return get_renderer().draw_point(x, y);
    }

    sdlpp::expected<void, std::string> draw_point(int x, int y, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_point(x, y);
    }

    sdlpp::expected<void, std::string> draw_point(const point& p) {
        return get_renderer().draw_point(p);
    }

    sdlpp::expected<void, std::string> draw_point(const point& p, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_point(p);
    }

    // --- Lines ---

    sdlpp::expected<void, std::string> draw_line(int x1, int y1, int x2, int y2) {
        return get_renderer().draw_line(x1, y1, x2, y2);
    }

    sdlpp::expected<void, std::string> draw_line(int x1, int y1, int x2, int y2, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_line(x1, y1, x2, y2);
    }

    sdlpp::expected<void, std::string> draw_line(const point& p1, const point& p2) {
        return get_renderer().draw_line(p1, p2);
    }

    sdlpp::expected<void, std::string> draw_line(const point& p1, const point& p2, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_line(p1, p2);
    }

    sdlpp::expected<void, std::string> draw_line(const line& l) {
        return get_renderer().draw_line(l.start(), l.end());
    }

    sdlpp::expected<void, std::string> draw_line(const line& l, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_line(l.start(), l.end());
    }

    // --- Rectangles ---

    sdlpp::expected<void, std::string> draw_rect(int x1, int y1, int x2, int y2) {
        int rx = std::min(x1, x2);
        int ry = std::min(y1, y2);
        int rw = std::abs(x2 - x1);
        int rh = std::abs(y2 - y1);
        return get_renderer().draw_rect(rect{rx, ry, rw, rh});
    }

    sdlpp::expected<void, std::string> draw_rect(int x1, int y1, int x2, int y2, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return draw_rect(x1, y1, x2, y2);
    }

    sdlpp::expected<void, std::string> draw_rect(const rect& r) {
        return get_renderer().draw_rect(r);
    }

    sdlpp::expected<void, std::string> draw_rect(const rect& r, const sdlpp::color& c) {
        auto& r_dev = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r_dev, c);
        return r_dev.draw_rect(r);
    }

    sdlpp::expected<void, std::string> draw_rect_fill(int x1, int y1, int x2, int y2) {
        int rx = std::min(x1, x2);
        int ry = std::min(y1, y2);
        int rw = std::abs(x2 - x1);
        int rh = std::abs(y2 - y1);
        return get_renderer().fill_rect(rect{rx, ry, rw, rh});
    }

    sdlpp::expected<void, std::string> draw_rect_fill(int x1, int y1, int x2, int y2, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return draw_rect_fill(x1, y1, x2, y2);
    }

    sdlpp::expected<void, std::string> draw_rect_fill(const rect& r) {
        return get_renderer().fill_rect(r);
    }

    sdlpp::expected<void, std::string> draw_rect_fill(const rect& r, const sdlpp::color& c) {
        auto& r_dev = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r_dev, c);
        return r_dev.fill_rect(r);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const sprite_sheet& sheet,
        sprite_visual_id visual_id,
        const point& position,
        sprite_flip flip) {
        return draw_sprite(sheet, visual_id, position, flip, 1.0f);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const sprite_sheet& sheet,
        sprite_visual_id visual_id,
        const point& position,
        sprite_flip flip,
        float scale) {
        ENFORCE(std::isfinite(scale) && scale > 0.0f)("sprite scale must be finite and greater than zero");

        auto* registry = service_locator::instance().get_texture_registry();
        ENFORCE(registry != nullptr);

        const auto& visual = sheet.visual(visual_id);
        const auto& atlas = registry->get(sheet.atlas());
        auto& renderer = get_renderer();

        if (has_diagonal_flip(flip)) {
            return draw_sprite_diagonal(renderer, atlas.texture, visual, position, flip, scale);
        }

        const auto origin = transformed_origin(visual, flip);
        const rect dst{
            position.x - scaled_offset(origin.x, scale),
            position.y - scaled_offset(origin.y, scale),
            scaled_extent(visual.texture_rect.w, scale),
            scaled_extent(visual.texture_rect.h, scale)
        };

        return renderer.copy_ex(
            atlas.texture,
            std::optional <rect>{visual.texture_rect},
            std::optional <rect>{dst},
            0.0,
            std::optional <point>{},
            to_sdl_flip(flip));
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        const sprite_sheet& sheet,
        sprite_visual_id visual,
        sprite_flip flip) {
        return draw_sprite(sheet, visual, position, flip);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        const sprite_sheet& sheet,
        sprite_visual_id visual,
        float scale) {
        return draw_sprite(sheet, visual, position, sprite_flip::none, scale);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        const sprite_sheet& sheet,
        sprite_visual_id visual,
        sprite_flip flip,
        float scale) {
        return draw_sprite(sheet, visual, position, flip, scale);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        sprite_visual_ref visual,
        sprite_flip flip) {
        return draw_sprite(position, visual, flip, 1.0f);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        sprite_visual_ref visual,
        float scale) {
        return draw_sprite(position, visual, sprite_flip::none, scale);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        sprite_visual_ref visual,
        sprite_flip flip,
        float scale) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);

        return draw_sprite(manager->get(visual.sheet), visual.visual, position, flip, scale);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        const sprite_appearance& appearance) {
        return draw_sprite(position, appearance, 1.0f);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        const sprite_appearance& appearance,
        float scale) {
        if (!appearance.visible) {
            return {};
        }

        return draw_sprite(position, appearance.visual, appearance.flip, scale);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        sprite_state_id state) {
        return draw_sprite(position, state, 1.0f);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        sprite_state_id state,
        float scale) {
        return draw_sprite(position, sprite_state_appearance(state), scale);
    }

    // --- Anti-aliased Lines ---

    sdlpp::expected<void, std::string> draw_line_aa(int x1, int y1, int x2, int y2) {
        return get_renderer().draw_line_aa(static_cast<float>(x1), static_cast<float>(y1),
                                           static_cast<float>(x2), static_cast<float>(y2));
    }

    sdlpp::expected<void, std::string> draw_line_aa(int x1, int y1, int x2, int y2, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return draw_line_aa(x1, y1, x2, y2);
    }

    sdlpp::expected<void, std::string> draw_line_aa(const point& p1, const point& p2) {
        return get_renderer().draw_line_aa(p1, p2);
    }

    sdlpp::expected<void, std::string> draw_line_aa(const point& p1, const point& p2, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_line_aa(p1, p2);
    }

    sdlpp::expected<void, std::string> draw_line_aa(const line& l) {
        return get_renderer().draw_line_aa(l.start(), l.end());
    }

    sdlpp::expected<void, std::string> draw_line_aa(const line& l, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_line_aa(l.start(), l.end());
    }

    // --- Thick Lines ---

    sdlpp::expected<void, std::string> draw_line_thick(int x1, int y1, int x2, int y2, float w) {
        return get_renderer().draw_line_thick(static_cast<float>(x1), static_cast<float>(y1),
                                              static_cast<float>(x2), static_cast<float>(y2), w);
    }

    sdlpp::expected<void, std::string> draw_line_thick(int x1, int y1, int x2, int y2, float w, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return draw_line_thick(x1, y1, x2, y2, w);
    }

    sdlpp::expected<void, std::string> draw_line_thick(const point& p1, const point& p2, float w) {
        return get_renderer().draw_line_thick(p1, p2, w);
    }

    sdlpp::expected<void, std::string> draw_line_thick(const point& p1, const point& p2, float w, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_line_thick(p1, p2, w);
    }

    sdlpp::expected<void, std::string> draw_line_thick(const line& l, float w) {
        return get_renderer().draw_line_thick(l.start(), l.end(), w);
    }

    sdlpp::expected<void, std::string> draw_line_thick(const line& l, float w, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_line_thick(l.start(), l.end(), w);
    }

    // --- Dashed Lines ---

    sdlpp::expected<void, std::string> draw_line_dashed(const point& p1, const point& p2, int dash, int gap) {
        return get_renderer().draw_line_dashed(p1, p2, static_cast<float>(dash), static_cast<float>(gap));
    }

    sdlpp::expected<void, std::string> draw_line_dashed(const point& p1, const point& p2, int dash, int gap, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return draw_line_dashed(p1, p2, dash, gap);
    }

    sdlpp::expected<void, std::string> draw_line_dashed(const line& l, int dash, int gap) {
        return get_renderer().draw_line_dashed(l.start(), l.end(), static_cast<float>(dash), static_cast<float>(gap));
    }

    sdlpp::expected<void, std::string> draw_line_dashed(const line& l, int dash, int gap, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_line_dashed(l.start(), l.end(), static_cast<float>(dash), static_cast<float>(gap));
    }

    // --- Dotted Lines ---

    sdlpp::expected<void, std::string> draw_line_dotted(const point& p1, const point& p2, int spacing) {
        return get_renderer().draw_line_dotted(p1, p2, static_cast<float>(spacing));
    }

    sdlpp::expected<void, std::string> draw_line_dotted(const point& p1, const point& p2, int spacing, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return draw_line_dotted(p1, p2, spacing);
    }

    sdlpp::expected<void, std::string> draw_line_dotted(const line& l, int spacing) {
        return get_renderer().draw_line_dotted(l.start(), l.end(), static_cast<float>(spacing));
    }

    sdlpp::expected<void, std::string> draw_line_dotted(const line& l, int spacing, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_line_dotted(l.start(), l.end(), static_cast<float>(spacing));
    }

    // --- Circles ---

    sdlpp::expected<void, std::string> draw_circle(int x1, int y1, int radius) {
        return get_renderer().draw_circle(x1, y1, radius);
    }

    sdlpp::expected<void, std::string> draw_circle(int x1, int y1, int radius, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_circle(x1, y1, radius);
    }

    sdlpp::expected<void, std::string> draw_circle(const circle& c) {
        return get_renderer().draw_circle(point{c.x, c.y}, c.radius);
    }

    sdlpp::expected<void, std::string> draw_circle(const circle& shape, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_circle(point{shape.x, shape.y}, shape.radius);
    }

    // --- Filled Circles ---

    sdlpp::expected<void, std::string> draw_circle_fill(int x1, int y1, int radius) {
        return get_renderer().fill_circle(x1, y1, radius);
    }

    sdlpp::expected<void, std::string> draw_circle_fill(int x1, int y1, int radius, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.fill_circle(x1, y1, radius);
    }

    sdlpp::expected<void, std::string> draw_circle_fill(const circle& c) {
        return get_renderer().fill_circle(point{c.x, c.y}, c.radius);
    }

    sdlpp::expected<void, std::string> draw_circle_fill(const circle& shape, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.fill_circle(point{shape.x, shape.y}, shape.radius);
    }

    // --- Arrows ---

    sdlpp::expected<void, std::string> draw_arrow(const point& from, const point& to, int head_size, float head_angle, float thickness) {
        return get_renderer().draw_arrow(from, to, static_cast<float>(head_size), head_angle, thickness);
    }

    sdlpp::expected<void, std::string> draw_arrow(const point& from, const point& to, int head_size, float head_angle, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_arrow(from, to, static_cast<float>(head_size), head_angle);
    }

    sdlpp::expected<void, std::string> draw_arrow(const point& from, const point& to, int head_size, float head_angle, float thickness, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_arrow(from, to, static_cast<float>(head_size), head_angle, thickness);
    }

    sdlpp::expected<void, std::string> draw_arrow(const line& l, int head_size, float head_angle, float thickness) {
        return get_renderer().draw_arrow(l.start(), l.end(), static_cast<float>(head_size), head_angle, thickness);
    }

    sdlpp::expected<void, std::string> draw_arrow(const line& l, int head_size, float head_angle, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_arrow(l.start(), l.end(), static_cast<float>(head_size), head_angle);
    }

    sdlpp::expected<void, std::string> draw_arrow(const line& l, int head_size, float head_angle, float thickness, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_arrow(l.start(), l.end(), static_cast<float>(head_size), head_angle, thickness);
    }

    // --- Crosses ---

    sdlpp::expected<void, std::string> draw_cross(const point& center, int size, float thickness) {
        return get_renderer().draw_cross(center, static_cast<float>(size), thickness);
    }

    sdlpp::expected<void, std::string> draw_cross(const point& center, int size, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_cross(center, static_cast<float>(size));
    }

    sdlpp::expected<void, std::string> draw_cross(const point& center, int size, float thickness, const sdlpp::color& c) {
        auto& r = get_renderer();
        sdlpp::renderer::draw_color_guard guard(r, c);
        return r.draw_cross(center, static_cast<float>(size), thickness);
    }
}
