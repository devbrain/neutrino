//
// Created by igor on 03/07/2026.
//

#include <neutrino/video/draw.hh>
#include <failsafe/enforce.hh>
#include <sdlpp/video/renderer.hh>
#include <euler/complex/complex.hh>
#include <euler/angles/degree.hh>
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

        [[nodiscard]] int scaled_offset(float value, float scale) noexcept {
            return static_cast <int>(std::lround(value * scale));
        }

        [[nodiscard]] int scaled_offset(int value, float scale) noexcept {
            return scaled_offset(static_cast <float>(value), scale);
        }

        [[nodiscard]] int scaled_extent(float value, float scale) noexcept {
            return std::max(1, scaled_offset(value, scale));
        }

        [[nodiscard]] int scaled_extent(int value, float scale) noexcept {
            return scaled_extent(static_cast <float>(value), scale);
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

        struct tiled_sprite_geometry {
            std::array <local_point, 4> corners;
            local_point anchor;
            local_point min;
            float width{0.0f};
            float height{0.0f};
            int pixel_width{1};
            int pixel_height{1};
        };

        [[nodiscard]] tiled_sprite_geometry build_tiled_geometry(
            const sprite_visual& visual,
            sprite_flip flip,
            float scale) noexcept {
            const auto& src = visual.texture_rect;
            const float width = static_cast <float>(src.w);
            const float height = static_cast <float>(src.h);

            tiled_sprite_geometry result;
            result.corners = {
                transform_local_tiled({0.0f, 0.0f}, width, height, flip),
                transform_local_tiled({width, 0.0f}, width, height, flip),
                transform_local_tiled({width, height}, width, height, flip),
                transform_local_tiled({0.0f, height}, width, height, flip)
            };
            result.anchor = transform_local_tiled(
                local_point{static_cast <float>(visual.origin.x), static_cast <float>(visual.origin.y)},
                width,
                height,
                flip);

            auto min_x = result.corners[0].x;
            auto max_x = result.corners[0].x;
            auto min_y = result.corners[0].y;
            auto max_y = result.corners[0].y;
            for (const auto& corner : result.corners) {
                min_x = std::min(min_x, corner.x);
                max_x = std::max(max_x, corner.x);
                min_y = std::min(min_y, corner.y);
                max_y = std::max(max_y, corner.y);
            }

            result.min = {min_x, min_y};
            result.width = max_x - min_x;
            result.height = max_y - min_y;
            result.pixel_width = scaled_extent(result.width, scale);
            result.pixel_height = scaled_extent(result.height, scale);
            return result;
        }

        [[nodiscard]] int scaled_coordinate(
            float coordinate,
            float extent,
            int pixel_extent,
            float scale) noexcept {
            if (coordinate <= 0.0f) {
                return 0;
            }
            if (coordinate >= extent) {
                return pixel_extent;
            }
            return scaled_offset(coordinate, scale);
        }

        [[nodiscard]] SDL_Vertex make_sprite_vertex(
            local_point local,
            const tiled_sprite_geometry& geometry,
            const rect& dst,
            float scale,
            SDL_FPoint tex_coord) noexcept {
            const auto x = scaled_coordinate(local.x - geometry.min.x, geometry.width, geometry.pixel_width, scale);
            const auto y = scaled_coordinate(local.y - geometry.min.y, geometry.height, geometry.pixel_height, scale);
            return SDL_Vertex{
                .position = {
                    static_cast <float>(dst.x + x),
                    static_cast <float>(dst.y + y)
                },
                .color = {1.0f, 1.0f, 1.0f, 1.0f},
                .tex_coord = tex_coord
            };
        }

        sdlpp::expected <void, std::string> draw_sprite_diagonal(
            sdlpp::renderer& renderer,
            const gpu_texture_atlas& atlas,
            const sprite_visual& visual,
            const point& position,
            sprite_flip flip,
            float scale,
            float rotation) {
            const auto& src = visual.texture_rect;
            const float texture_width = static_cast <float>(atlas.width);
            const float texture_height = static_cast <float>(atlas.height);
            const float u0 = static_cast <float>(src.x) / texture_width;
            const float v0 = static_cast <float>(src.y) / texture_height;
            const float u1 = static_cast <float>(src.x + src.w) / texture_width;
            const float v1 = static_cast <float>(src.y + src.h) / texture_height;

            const auto geometry = build_tiled_geometry(visual, flip, scale);
            const auto anchor_x = scaled_coordinate(
                geometry.anchor.x - geometry.min.x,
                geometry.width,
                geometry.pixel_width,
                scale);
            const auto anchor_y = scaled_coordinate(
                geometry.anchor.y - geometry.min.y,
                geometry.height,
                geometry.pixel_height,
                scale);
            const rect dst{
                position.x - anchor_x,
                position.y - anchor_y,
                geometry.pixel_width,
                geometry.pixel_height
            };

            std::array <SDL_Vertex, 4> vertices{
                make_sprite_vertex(geometry.corners[0], geometry, dst, scale, {u0, v0}),
                make_sprite_vertex(geometry.corners[1], geometry, dst, scale, {u1, v0}),
                make_sprite_vertex(geometry.corners[2], geometry, dst, scale, {u1, v1}),
                make_sprite_vertex(geometry.corners[3], geometry, dst, scale, {u0, v1})
            };
            if (rotation != 0.0f) {
                // A 2D rotation is a unit complex: multiplying (dx,dy) by
                // polar(1, θ) gives (dx·cosθ - dy·sinθ, dx·sinθ + dy·cosθ) —
                // clockwise in screen space (y down), matching copy_ex on the
                // other path. Rotate each vertex about the anchor `position`.
                const auto rotor = euler::complex<float>::polar(1.0f, euler::degree<float>(rotation));
                const float cx = static_cast <float>(position.x);
                const float cy = static_cast <float>(position.y);
                for (auto& v : vertices) {
                    const auto rotated = euler::complex<float>(v.position.x - cx, v.position.y - cy) * rotor;
                    v.position.x = cx + rotated.real();
                    v.position.y = cy + rotated.imag();
                }
            }
            constexpr std::array <int, 6> indices{0, 1, 2, 0, 2, 3};

            return renderer.render_geometry(atlas.texture.get(), vertices, indices);
        }

        [[nodiscard]] texture_registry& require_texture_registry() {
            auto* registry = service_locator::instance().get_texture_registry();
            ENFORCE(registry != nullptr);
            return *registry;
        }

        [[nodiscard]] sprites_manager& require_sprites_manager() {
            auto* manager = service_locator::instance().get_sprites_manager();
            ENFORCE(manager != nullptr);
            return *manager;
        }

        sdlpp::expected <void, std::string> draw_visual_impl(
            const texture_registry& registry,
            const sprite_sheet& sheet,
            sprite_visual_id visual_id,
            const point& position,
            sprite_flip flip,
            float scale,
            float rotation) {
            ENFORCE(std::isfinite(scale) && scale > 0.0f)("sprite scale must be finite and greater than zero");
            ENFORCE(std::isfinite(rotation))("sprite rotation must be finite");

            const auto& visual = sheet.visual(visual_id);
            const auto& atlas = registry.get(sheet.atlas());
            auto& renderer = get_renderer();

            if (has_diagonal_flip(flip)) {
                return draw_sprite_diagonal(renderer, atlas, visual, position, flip, scale, rotation);
            }

            const auto dst_w = scaled_extent(visual.texture_rect.w, scale);
            const auto dst_h = scaled_extent(visual.texture_rect.h, scale);
            auto origin_x = scaled_offset(visual.origin.x, scale);
            auto origin_y = scaled_offset(visual.origin.y, scale);
            if (has_flip(flip, sprite_flip::horizontal)) {
                origin_x = dst_w - origin_x;
            }
            if (has_flip(flip, sprite_flip::vertical)) {
                origin_y = dst_h - origin_y;
            }

            const rect dst{
                position.x - origin_x,
                position.y - origin_y,
                dst_w,
                dst_h
            };

            const bool rotate = rotation != 0.0f;
            return renderer.copy_ex(
                atlas.texture,
                std::optional <rect>{visual.texture_rect},
                std::optional <rect>{dst},
                static_cast <double>(rotation),
                // Pivot around the (flip-adjusted) visual origin; at 0 keep the
                // original nullopt center so no-rotation is byte-identical.
                rotate ? std::optional <point>{point{origin_x, origin_y}} : std::optional <point>{},
                to_sdl_flip(flip));
        }

        sdlpp::expected <void, std::string> draw_appearance_impl(
            const sprites_manager& manager,
            const point& position,
            const sprite_appearance& appearance,
            const sprite_draw_params& params) {
            if (!appearance.visible || !appearance.visual.valid()) {
                return {};
            }

            return draw_visual_impl(
                require_texture_registry(),
                manager.get(appearance.visual.sheet),
                appearance.visual.visual,
                position,
                appearance.flip ^ params.flip,
                params.scale,
                params.rotation_degrees);
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
        const point& position,
        const sprite_sheet& sheet,
        sprite_visual_id visual,
        const sprite_draw_params& params) {
        return draw_visual_impl(require_texture_registry(), sheet, visual, position, params.flip, params.scale,
                                params.rotation_degrees);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        sprite_visual_ref visual,
        const sprite_draw_params& params) {
        const auto& manager = require_sprites_manager();
        return draw_visual_impl(
            require_texture_registry(),
            manager.get(visual.sheet),
            visual.visual,
            position,
            params.flip,
            params.scale,
            params.rotation_degrees);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const rect& dst,
        sprite_visual_ref visual,
        sprite_flip flip) {
        if (!visual.valid() || dst.w <= 0 || dst.h <= 0) {
            return {}; // nothing to draw
        }
        const auto& manager = require_sprites_manager();
        const auto& sheet = manager.get(visual.sheet);
        const auto& sprite = sheet.visual(visual.visual);
        const auto& atlas = require_texture_registry().get(sheet.atlas());
        // Straight blit into the explicit destination: the caller has already rounded
        // both corners, so no origin/scale rounding here. Diagonal flip / rotation are
        // not supported on this path (H/V only).
        return get_renderer().copy_ex(
            atlas.texture,
            std::optional <rect>{sprite.texture_rect},
            std::optional <rect>{dst},
            0.0,
            std::optional <point>{},
            to_sdl_flip(flip));
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        const sprite_appearance& appearance,
        const sprite_draw_params& params) {
        if (!appearance.visible || !appearance.visual.valid()) {
            return {};
        }

        return draw_appearance_impl(require_sprites_manager(), position, appearance, params);
    }

    sdlpp::expected <void, std::string> draw_sprite(
        const point& position,
        sprite_state_id state,
        const sprite_draw_params& params) {
        const auto& manager = require_sprites_manager();
        return draw_appearance_impl(manager, position, manager.appearance(state), params);
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
