//
// Created by igor on 03/07/2026.
//

#include <source_location>
#include <type_traits>
#include <neutrino/video/draw.hh>
#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>
#include <failsafe/exception.hh>
#include <sdlpp/video/renderer.hh>
#include <euler/complex/complex.hh>
#include <euler/angles/degree.hh>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <optional>

#include "services/service_access.hh"

namespace neutrino {
    namespace {
// Total draw failures since startup; read through draw_failure_count().
        std::uint64_t g_draw_failures = 0;

        // Turns an sdlpp result into the draw API's contract. Draws/setters: a failure is counted
        // and logged, rate-limited per call site (1st, 1001st, ...) so a broken renderer cannot
        // flood the log. Getters: a failure means the renderer itself is gone, so throw.
        // LINE is the DRAW_CHECK call site; two checks on one line share a counter.
        template<std::uint_least32_t LINE>
        struct call_site_checker {
            static inline std::uint32_t count = 0;

            static void run(const sdlpp::expected <void, std::string>& x) {
                if (x) [[likely]] {
                    return;
                }
                ++g_draw_failures;
                if (count++ % 1000 == 0) {
                    failsafe::logger::log_with_level <LOGGER_LEVEL_ERROR>(
                        LOGGER_DEFAULT_CATEGORY_STR, __FILE__, static_cast <int>(LINE), x.error());
                }
            }

            template<typename T>
            static T run(const sdlpp::expected <T, std::string>& x) {
                if (x) [[likely]] {
                    return *x;
                }
                THROW_DEFAULT(x.error());
            }
        };

#define DRAW_CHECK(...) call_site_checker <std::source_location::current().line()>::run(__VA_ARGS__)

        // Open the renderer, set the draw colour under a scope guard, and run `fn(renderer)`.
        // Collapses the repeated get_renderer() + draw_color_guard + forward boilerplate.
        template<class Fn>
        auto with_draw_color(const sdlpp::color& c, Fn&& fn) {
            auto& renderer = get_renderer();
            sdlpp::renderer::draw_color_guard guard(renderer, c);
            return fn(renderer);
        }

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

        // Sprite metrics are computed in float. Snapped (pixel_snap::logical) every value is rounded
        // to a whole pixel -- held exactly in a float, so output matches the old integer path pixel
        // for pixel. Unsnapped, the fraction reaches SDL.
        [[nodiscard]] bool snapping() noexcept {
            return get_pixel_snap() == pixel_snap::logical;
        }

        [[nodiscard]] float snap_coord(float v, bool snap) noexcept {
            return snap ? std::round(v) : v;
        }

        [[nodiscard]] render_pos snap_pos(render_pos p, bool snap) noexcept {
            return {snap_coord(p.x, snap), snap_coord(p.y, snap)};
        }

        [[nodiscard]] float scaled_offset(float value, float scale, bool snap) noexcept {
            return snap_coord(value * scale, snap);
        }

        [[nodiscard]] float scaled_extent(float value, float scale, bool snap) noexcept {
            return snap ? std::max(1.0f, std::round(value * scale)) : value * scale;
        }

        struct local_point {
            float x{0};
            float y{0};
        };

        [[nodiscard]] local_point transform_local_tiled(local_point p, float width, float height,
                                                        sprite_flip flip) noexcept {
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
            float pixel_width{1.0f};
            float pixel_height{1.0f};
        };

        [[nodiscard]] tiled_sprite_geometry build_tiled_geometry(
            const sprite_visual& visual,
            sprite_flip flip,
            float scale,
            bool snap) noexcept {
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
            result.pixel_width = scaled_extent(result.width, scale, snap);
            result.pixel_height = scaled_extent(result.height, scale, snap);
            return result;
        }

        [[nodiscard]] float scaled_coordinate(
            float coordinate,
            float extent,
            float pixel_extent,
            float scale,
            bool snap) noexcept {
            if (coordinate <= 0.0f) {
                return 0.0f;
            }
            if (coordinate >= extent) {
                return pixel_extent;
            }
            return scaled_offset(coordinate, scale, snap);
        }

        [[nodiscard]] SDL_Vertex make_sprite_vertex(
            local_point local,
            const tiled_sprite_geometry& geometry,
            const render_rect& dst,
            float scale,
            bool snap,
            SDL_FPoint tex_coord) noexcept {
            const auto x = scaled_coordinate(local.x - geometry.min.x, geometry.width, geometry.pixel_width, scale,
                                             snap);
            const auto y = scaled_coordinate(local.y - geometry.min.y, geometry.height, geometry.pixel_height, scale,
                                             snap);
            return SDL_Vertex{
                .position = {dst.x + x, dst.y + y},
                .color = {1.0f, 1.0f, 1.0f, 1.0f},
                .tex_coord = tex_coord
            };
        }

        sdlpp::expected <void, std::string> draw_sprite_diagonal(
            sdlpp::renderer& renderer,
            const gpu_texture_atlas& atlas,
            const sprite_visual& visual,
            render_pos position,
            sprite_flip flip,
            float scale,
            float rotation,
            bool snap) {
            const auto& src = visual.texture_rect;
            const auto texture_width = static_cast <float>(atlas.width);
            const auto texture_height = static_cast <float>(atlas.height);
            const float u0 = static_cast <float>(src.x) / texture_width;
            const float v0 = static_cast <float>(src.y) / texture_height;
            const float u1 = static_cast <float>(src.x + src.w) / texture_width;
            const float v1 = static_cast <float>(src.y + src.h) / texture_height;

            const auto geometry = build_tiled_geometry(visual, flip, scale, snap);
            const auto anchor_x = scaled_coordinate(
                geometry.anchor.x - geometry.min.x,
                geometry.width,
                geometry.pixel_width,
                scale,
                snap);
            const auto anchor_y = scaled_coordinate(
                geometry.anchor.y - geometry.min.y,
                geometry.height,
                geometry.pixel_height,
                scale,
                snap);
            const render_rect dst{
                position.x - anchor_x,
                position.y - anchor_y,
                geometry.pixel_width,
                geometry.pixel_height
            };

            std::array <SDL_Vertex, 4> vertices{
                make_sprite_vertex(geometry.corners[0], geometry, dst, scale, snap, {u0, v0}),
                make_sprite_vertex(geometry.corners[1], geometry, dst, scale, snap, {u1, v0}),
                make_sprite_vertex(geometry.corners[2], geometry, dst, scale, snap, {u1, v1}),
                make_sprite_vertex(geometry.corners[3], geometry, dst, scale, snap, {u0, v1})
            };
            if (rotation != 0.0f) {
                // A 2D rotation is a unit complex: multiplying (dx,dy) by
                // polar(1, θ) gives (dx·cosθ - dy·sinθ, dx·sinθ + dy·cosθ) —
                // clockwise in screen space (y down), matching copy_ex on the
                // other path. Rotate each vertex about the anchor `position`.
                const auto rotor = euler::complex <float>::polar(1.0f, euler::degree <float>(rotation));
                const auto cx = position.x;
                const auto cy = position.y;
                for (auto& v : vertices) {
                    const auto rotated = euler::complex <float>(v.position.x - cx, v.position.y - cy) * rotor;
                    v.position.x = cx + rotated.real();
                    v.position.y = cy + rotated.imag();
                }
            }
            constexpr std::array <int, 6> indices{0, 1, 2, 0, 2, 3};

            return renderer.render_geometry(atlas.texture.native().get(), vertices, indices);
        }

        sdlpp::expected <void, std::string> draw_visual_impl(
            const texture_registry& registry,
            const sprite_sheet& sheet,
            sprite_visual_id visual_id,
            render_pos position,
            sprite_flip flip,
            float scale,
            float rotation) {
            ENFORCE(std::isfinite(scale) && scale > 0.0f)("sprite scale must be finite and greater than zero");
            ENFORCE(std::isfinite(rotation))("sprite rotation must be finite");

            const auto& visual = sheet.visual(visual_id);
            const auto& atlas = registry.get(sheet.atlas());
            auto& renderer = get_renderer();
            const bool snap = snapping();
            position = snap_pos(position, snap);

            if (has_diagonal_flip(flip)) {
                return draw_sprite_diagonal(renderer, atlas, visual, position, flip, scale, rotation, snap);
            }

            const auto dst_w = scaled_extent(static_cast <float>(visual.texture_rect.w), scale, snap);
            const auto dst_h = scaled_extent(static_cast <float>(visual.texture_rect.h), scale, snap);
            auto origin_x = scaled_offset(static_cast <float>(visual.origin.x), scale, snap);
            auto origin_y = scaled_offset(static_cast <float>(visual.origin.y), scale, snap);
            if (has_flip(flip, sprite_flip::horizontal)) {
                origin_x = dst_w - origin_x;
            }
            if (has_flip(flip, sprite_flip::vertical)) {
                origin_y = dst_h - origin_y;
            }

            const sdlpp::rect <float> dst{
                position.x - origin_x,
                position.y - origin_y,
                dst_w,
                dst_h
            };

            const bool rotate = rotation != 0.0f;
            return renderer.copy_ex(
                atlas.texture.native(),
                std::optional <rect>{visual.texture_rect},
                std::optional <sdlpp::rect <float>>{dst},
                static_cast <double>(rotation),
                // Pivot around the (flip-adjusted) visual origin; at 0 keep the
                // original nullopt center so no-rotation is byte-identical.
                rotate ? std::optional <sdlpp::point <float>>{sdlpp::point <float>{origin_x, origin_y}}
                       : std::optional <sdlpp::point <float>>{},
                to_sdl_flip(flip));
        }

        sdlpp::expected <void, std::string> draw_appearance_impl(
            const sprites_manager& manager,
            render_pos position,
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

        [[nodiscard]] render_pos as_render(const point& p) noexcept {
            return {static_cast <float>(p.x), static_cast <float>(p.y)};
        }

        [[nodiscard]] render_rect as_render(const rect& r) noexcept {
            return {static_cast <float>(r.x), static_cast <float>(r.y), static_cast <float>(r.w),
                    static_cast <float>(r.h)};
        }

        // Normalizes a corner-pair rectangle (either diagonal) to x/y/w/h.
        [[nodiscard]] render_rect from_corners(int x1, int y1, int x2, int y2) noexcept {
            return {static_cast <float>(std::min(x1, x2)), static_cast <float>(std::min(y1, y2)),
                    static_cast <float>(std::abs(x2 - x1)), static_cast <float>(std::abs(y2 - y1))};
        }

        // Snaps the EDGES, not x/y and w/h separately, so rectangles sharing an edge still meet.
        [[nodiscard]] sdlpp::rect <float> to_sdl(const render_rect& r, bool snap) noexcept {
            if (!snap) {
                return {r.x, r.y, r.w, r.h};
            }
            const float x0 = std::round(r.x);
            const float y0 = std::round(r.y);
            return {x0, y0, std::round(r.x + r.w) - x0, std::round(r.y + r.h) - y0};
        }

        // The circle rasterizer is integer-only, so circles always round (documented in draw.hh).
        [[nodiscard]] int whole(float v) noexcept {
            return static_cast <int>(std::lround(v));
        }
    }

    void set_draw_color(const sdlpp::color& c) {
        DRAW_CHECK(get_renderer().set_draw_color(c));
    }

    sdlpp::color get_draw_color() {
        return DRAW_CHECK(get_renderer().get_draw_color());
    }

    void set_draw_blend_mode(sdlpp::blend_mode mode) {
        DRAW_CHECK(get_renderer().set_draw_blend_mode(mode));
    }

    sdlpp::blend_mode get_draw_blend_mode() {
        return DRAW_CHECK(get_renderer().get_draw_blend_mode());
    }

    void set_line_style(const sdlpp::line_style& style) {
        DRAW_CHECK(get_renderer().set_line_style(style));
    }

    sdlpp::line_style get_line_style() {
        return DRAW_CHECK(get_renderer().get_line_style());
    }

    // --- Float render-space implementations -------------------------------------------------------
    // Every primitive below is implemented once, here, on floats; the integer overloads forward.

    namespace detail {
        void draw_point(render_pos p) {
            p = snap_pos(p, snapping());
            DRAW_CHECK(get_renderer().draw_point(p.x, p.y));
        }

        void draw_point(render_pos p, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_point(p); });
        }

        void draw_line(render_pos a, render_pos b) {
            const bool snap = snapping();
            a = snap_pos(a, snap);
            b = snap_pos(b, snap);
            DRAW_CHECK(get_renderer().draw_line(a.x, a.y, b.x, b.y));
        }

        void draw_line(render_pos a, render_pos b, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_line(a, b); });
        }

        void draw_line_aa(render_pos a, render_pos b) {
            const bool snap = snapping();
            a = snap_pos(a, snap);
            b = snap_pos(b, snap);
            DRAW_CHECK(get_renderer().draw_line_aa(a.x, a.y, b.x, b.y));
        }

        void draw_line_aa(render_pos a, render_pos b, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_line_aa(a, b); });
        }

        void draw_line_thick(render_pos a, render_pos b, float w) {
            const bool snap = snapping();
            a = snap_pos(a, snap);
            b = snap_pos(b, snap);
            DRAW_CHECK(get_renderer().draw_line_thick(a.x, a.y, b.x, b.y, w));
        }

        void draw_line_thick(render_pos a, render_pos b, float w, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_line_thick(a, b, w); });
        }

        void draw_line_dashed(render_pos a, render_pos b, float dash, float gap) {
            const bool snap = snapping();
            a = snap_pos(a, snap);
            b = snap_pos(b, snap);
            DRAW_CHECK(get_renderer().draw_line_dashed(a.x, a.y, b.x, b.y, dash, gap));
        }

        void draw_line_dashed(render_pos a, render_pos b, float dash, float gap, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_line_dashed(a, b, dash, gap); });
        }

        void draw_line_dotted(render_pos a, render_pos b, float spacing) {
            const bool snap = snapping();
            a = snap_pos(a, snap);
            b = snap_pos(b, snap);
            DRAW_CHECK(get_renderer().draw_line_dotted(a.x, a.y, b.x, b.y, spacing));
        }

        void draw_line_dotted(render_pos a, render_pos b, float spacing, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_line_dotted(a, b, spacing); });
        }

        void draw_arrow(render_pos from, render_pos to, float head_size, float head_angle, float thickness) {
            const bool snap = snapping();
            from = snap_pos(from, snap);
            to = snap_pos(to, snap);
            DRAW_CHECK(get_renderer().draw_arrow(from.x, from.y, to.x, to.y, head_size, head_angle, thickness));
        }

        void draw_arrow(render_pos from, render_pos to, float head_size, float head_angle, float thickness,
                        const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_arrow(from, to, head_size, head_angle, thickness); });
        }

        void draw_cross(render_pos centre, float size, float thickness) {
            centre = snap_pos(centre, snapping());
            DRAW_CHECK(get_renderer().draw_cross(centre.x, centre.y, size, thickness));
        }

        void draw_cross(render_pos centre, float size, float thickness, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_cross(centre, size, thickness); });
        }

        void draw_rect(const render_rect& r) {
            DRAW_CHECK(get_renderer().draw_rect(to_sdl(r, snapping())));
        }

        void draw_rect(const render_rect& r, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_rect(r); });
        }

        void draw_rect_fill(const render_rect& r) {
            DRAW_CHECK(get_renderer().fill_rect(to_sdl(r, snapping())));
        }

        void draw_rect_fill(const render_rect& r, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_rect_fill(r); });
        }

        void draw_circle(render_pos centre, float radius) {
            DRAW_CHECK(get_renderer().draw_circle(whole(centre.x), whole(centre.y), whole(radius)));
        }

        void draw_circle(render_pos centre, float radius, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_circle(centre, radius); });
        }

        void draw_circle_fill(render_pos centre, float radius) {
            DRAW_CHECK(get_renderer().fill_circle(whole(centre.x), whole(centre.y), whole(radius)));
        }

        void draw_circle_fill(render_pos centre, float radius, const sdlpp::color& c) {
            with_draw_color(c, [&](auto&) { draw_circle_fill(centre, radius); });
        }

        void draw_sprite(render_pos position, const sprite_sheet& sheet, sprite_visual_id visual,
                         const sprite_draw_params& params) {
            DRAW_CHECK(draw_visual_impl(require_texture_registry(), sheet, visual, position, params.flip,
                                        params.scale, params.rotation_degrees));
        }

        void draw_sprite(render_pos position, sprite_visual_ref visual, const sprite_draw_params& params) {
            const auto& manager = require_sprites_manager();
            DRAW_CHECK(draw_visual_impl(require_texture_registry(), manager.get(visual.sheet), visual.visual,
                                        position, params.flip, params.scale, params.rotation_degrees));
        }

        void draw_sprite(render_pos position, const sprite_appearance& appearance,
                         const sprite_draw_params& params) {
            if (!appearance.visible || !appearance.visual.valid()) {
                return;
            }
            DRAW_CHECK(draw_appearance_impl(require_sprites_manager(), position, appearance, params));
        }

        void draw_sprite(render_pos position, sprite_state_id state, const sprite_draw_params& params) {
            const auto& manager = require_sprites_manager();
            DRAW_CHECK(draw_appearance_impl(manager, position, manager.appearance(state), params));
        }
    }

    // --- Points ---

    void draw_point(int x, int y) {
        detail::draw_point(as_render(point{x, y}));
    }

    void draw_point(int x, int y, const sdlpp::color& c) {
        detail::draw_point(as_render(point{x, y}), c);
    }

    void draw_point(const point& p) {
        detail::draw_point(as_render(p));
    }

    void draw_point(const point& p, const sdlpp::color& c) {
        detail::draw_point(as_render(p), c);
    }

    // --- Lines ---

    void draw_line(int x1, int y1, int x2, int y2) {
        detail::draw_line(as_render(point{x1, y1}), as_render(point{x2, y2}));
    }

    void draw_line(int x1, int y1, int x2, int y2, const sdlpp::color& c) {
        detail::draw_line(as_render(point{x1, y1}), as_render(point{x2, y2}), c);
    }

    void draw_line(const point& p1, const point& p2) {
        detail::draw_line(as_render(p1), as_render(p2));
    }

    void draw_line(const point& p1, const point& p2, const sdlpp::color& c) {
        detail::draw_line(as_render(p1), as_render(p2), c);
    }

    void draw_line(const line& l) {
        detail::draw_line(as_render(l.start()), as_render(l.end()));
    }

    void draw_line(const line& l, const sdlpp::color& c) {
        detail::draw_line(as_render(l.start()), as_render(l.end()), c);
    }

    // --- Rectangles ---

    void draw_rect(int x1, int y1, int x2, int y2) {
        detail::draw_rect(from_corners(x1, y1, x2, y2));
    }

    void draw_rect(int x1, int y1, int x2, int y2, const sdlpp::color& c) {
        detail::draw_rect(from_corners(x1, y1, x2, y2), c);
    }

    void draw_rect(const rect& r) {
        detail::draw_rect(as_render(r));
    }

    void draw_rect(const rect& r, const sdlpp::color& c) {
        detail::draw_rect(as_render(r), c);
    }

    void draw_rect_fill(int x1, int y1, int x2, int y2) {
        detail::draw_rect_fill(from_corners(x1, y1, x2, y2));
    }

    void draw_rect_fill(int x1, int y1, int x2, int y2, const sdlpp::color& c) {
        detail::draw_rect_fill(from_corners(x1, y1, x2, y2), c);
    }

    void draw_rect_fill(const rect& r) {
        detail::draw_rect_fill(as_render(r));
    }

    void draw_rect_fill(const rect& r, const sdlpp::color& c) {
        detail::draw_rect_fill(as_render(r), c);
    }

    // --- Sprites ---

    void draw_sprite(
        const point& position,
        const sprite_sheet& sheet,
        sprite_visual_id visual,
        const sprite_draw_params& params) {
        detail::draw_sprite(as_render(position), sheet, visual, params);
    }

    void draw_sprite(
        const point& position,
        sprite_visual_ref visual,
        const sprite_draw_params& params) {
        detail::draw_sprite(as_render(position), visual, params);
    }

    void draw_sprite(
        const rect& dst,
        sprite_visual_ref visual,
        sprite_flip flip) {
        if (!visual.valid() || dst.w <= 0 || dst.h <= 0) {
            return; // nothing to draw
        }
        const auto& manager = require_sprites_manager();
        const auto& sheet = manager.get(visual.sheet);
        const auto& sprite = sheet.visual(visual.visual);
        const auto& atlas = require_texture_registry().get(sheet.atlas());
        // Straight blit into the explicit destination: the caller has already rounded
        // both corners, so no origin/scale rounding here. Diagonal flip / rotation are
        // not supported on this path (H/V only).
        DRAW_CHECK(get_renderer().copy_ex(
            atlas.texture.native(),
            std::optional <rect>{sprite.texture_rect},
            std::optional <rect>{dst},
            0.0,
            std::optional <point>{},
            to_sdl_flip(flip)));
    }

    void draw_sprite(
        const point& position,
        const sprite_appearance& appearance,
        const sprite_draw_params& params) {
        detail::draw_sprite(as_render(position), appearance, params);
    }

    void draw_sprite(
        const point& position,
        sprite_state_id state,
        const sprite_draw_params& params) {
        detail::draw_sprite(as_render(position), state, params);
    }

    // --- Anti-aliased Lines ---

    void draw_line_aa(int x1, int y1, int x2, int y2) {
        detail::draw_line_aa(as_render(point{x1, y1}), as_render(point{x2, y2}));
    }

    void draw_line_aa(int x1, int y1, int x2, int y2, const sdlpp::color& c) {
        detail::draw_line_aa(as_render(point{x1, y1}), as_render(point{x2, y2}), c);
    }

    void draw_line_aa(const point& p1, const point& p2) {
        detail::draw_line_aa(as_render(p1), as_render(p2));
    }

    void draw_line_aa(const point& p1, const point& p2, const sdlpp::color& c) {
        detail::draw_line_aa(as_render(p1), as_render(p2), c);
    }

    void draw_line_aa(const line& l) {
        detail::draw_line_aa(as_render(l.start()), as_render(l.end()));
    }

    void draw_line_aa(const line& l, const sdlpp::color& c) {
        detail::draw_line_aa(as_render(l.start()), as_render(l.end()), c);
    }

    // --- Thick Lines ---

    void draw_line_thick(int x1, int y1, int x2, int y2, float w) {
        detail::draw_line_thick(as_render(point{x1, y1}), as_render(point{x2, y2}), w);
    }

    void draw_line_thick(int x1, int y1, int x2, int y2, float w, const sdlpp::color& c) {
        detail::draw_line_thick(as_render(point{x1, y1}), as_render(point{x2, y2}), w, c);
    }

    void draw_line_thick(const point& p1, const point& p2, float w) {
        detail::draw_line_thick(as_render(p1), as_render(p2), w);
    }

    void draw_line_thick(const point& p1, const point& p2, float w, const sdlpp::color& c) {
        detail::draw_line_thick(as_render(p1), as_render(p2), w, c);
    }

    void draw_line_thick(const line& l, float w) {
        detail::draw_line_thick(as_render(l.start()), as_render(l.end()), w);
    }

    void draw_line_thick(const line& l, float w, const sdlpp::color& c) {
        detail::draw_line_thick(as_render(l.start()), as_render(l.end()), w, c);
    }

    // --- Dashed Lines ---

    void draw_line_dashed(const point& p1, const point& p2, int dash, int gap) {
        detail::draw_line_dashed(as_render(p1), as_render(p2), static_cast <float>(dash), static_cast <float>(gap));
    }

    void draw_line_dashed(const point& p1, const point& p2, int dash, int gap, const sdlpp::color& c) {
        detail::draw_line_dashed(as_render(p1), as_render(p2), static_cast <float>(dash), static_cast <float>(gap),
                                 c);
    }

    void draw_line_dashed(const line& l, int dash, int gap) {
        draw_line_dashed(l.start(), l.end(), dash, gap);
    }

    void draw_line_dashed(const line& l, int dash, int gap, const sdlpp::color& c) {
        draw_line_dashed(l.start(), l.end(), dash, gap, c);
    }

    // --- Dotted Lines ---

    void draw_line_dotted(const point& p1, const point& p2, int spacing) {
        detail::draw_line_dotted(as_render(p1), as_render(p2), static_cast <float>(spacing));
    }

    void draw_line_dotted(const point& p1, const point& p2, int spacing, const sdlpp::color& c) {
        detail::draw_line_dotted(as_render(p1), as_render(p2), static_cast <float>(spacing), c);
    }

    void draw_line_dotted(const line& l, int spacing) {
        draw_line_dotted(l.start(), l.end(), spacing);
    }

    void draw_line_dotted(const line& l, int spacing, const sdlpp::color& c) {
        draw_line_dotted(l.start(), l.end(), spacing, c);
    }

    // --- Circles ---

    void draw_circle(int x1, int y1, int radius) {
        detail::draw_circle(as_render(point{x1, y1}), static_cast <float>(radius));
    }

    void draw_circle(int x1, int y1, int radius, const sdlpp::color& c) {
        detail::draw_circle(as_render(point{x1, y1}), static_cast <float>(radius), c);
    }

    void draw_circle(const circle& c) {
        detail::draw_circle(as_render(point{c.x, c.y}), static_cast <float>(c.radius));
    }

    void draw_circle(const circle& shape, const sdlpp::color& c) {
        detail::draw_circle(as_render(point{shape.x, shape.y}), static_cast <float>(shape.radius), c);
    }

    // --- Filled Circles ---

    void draw_circle_fill(int x1, int y1, int radius) {
        detail::draw_circle_fill(as_render(point{x1, y1}), static_cast <float>(radius));
    }

    void draw_circle_fill(int x1, int y1, int radius, const sdlpp::color& c) {
        detail::draw_circle_fill(as_render(point{x1, y1}), static_cast <float>(radius), c);
    }

    void draw_circle_fill(const circle& c) {
        detail::draw_circle_fill(as_render(point{c.x, c.y}), static_cast <float>(c.radius));
    }

    void draw_circle_fill(const circle& shape, const sdlpp::color& c) {
        detail::draw_circle_fill(as_render(point{shape.x, shape.y}), static_cast <float>(shape.radius), c);
    }

    // --- Arrows ---

    void draw_arrow(const point& from, const point& to, int head_size, float head_angle, float thickness) {
        detail::draw_arrow(as_render(from), as_render(to), static_cast <float>(head_size), head_angle, thickness);
    }

    void draw_arrow(const point& from, const point& to, int head_size, float head_angle, const sdlpp::color& c) {
        detail::draw_arrow(as_render(from), as_render(to), static_cast <float>(head_size), head_angle, 1.0f, c);
    }

    void draw_arrow(const point& from, const point& to, int head_size, float head_angle, float thickness,
                    const sdlpp::color& c) {
        detail::draw_arrow(as_render(from), as_render(to), static_cast <float>(head_size), head_angle, thickness, c);
    }

    void draw_arrow(const line& l, int head_size, float head_angle, float thickness) {
        draw_arrow(l.start(), l.end(), head_size, head_angle, thickness);
    }

    void draw_arrow(const line& l, int head_size, float head_angle, const sdlpp::color& c) {
        draw_arrow(l.start(), l.end(), head_size, head_angle, c);
    }

    void draw_arrow(const line& l, int head_size, float head_angle, float thickness, const sdlpp::color& c) {
        draw_arrow(l.start(), l.end(), head_size, head_angle, thickness, c);
    }

    // --- Crosses ---

    void draw_cross(const point& center, int size, float thickness) {
        detail::draw_cross(as_render(center), static_cast <float>(size), thickness);
    }

    void draw_cross(const point& center, int size, const sdlpp::color& c) {
        detail::draw_cross(as_render(center), static_cast <float>(size), 1.0f, c);
    }

    void draw_cross(const point& center, int size, float thickness, const sdlpp::color& c) {
        detail::draw_cross(as_render(center), static_cast <float>(size), thickness, c);
    }

    // --- Textures ---

    namespace {
        // Tint is per-texture state in SDL: apply it around one draw and put the neutral value
        // back, so a texture always rests at white / opaque. A white tint touches nothing.
        class tint_scope {
            public:
                tint_scope(sdlpp::texture& tex, const color& tint) noexcept
                    : m_texture(tex), m_active(tint != color{255, 255, 255, 255}) {
                    if (m_active) {
                        (void) m_texture.set_color_mod(tint);
                        (void) m_texture.set_alpha_mod(tint.a);
                    }
                }

                ~tint_scope() {
                    if (m_active) {
                        (void) m_texture.set_color_mod(color{255, 255, 255, 255});
                        (void) m_texture.set_alpha_mod(255);
                    }
                }

                tint_scope(const tint_scope&) = delete;
                tint_scope& operator=(const tint_scope&) = delete;

            private:
                sdlpp::texture& m_texture;
                bool m_active;
        };

        // Drawing is logically const: the only texture state touched (tint) is restored.
        [[nodiscard]] sdlpp::texture& drawable(const texture& t) noexcept {
            return const_cast <sdlpp::texture&>(t.native());
        }
    }

    namespace detail {
        void draw_texture(const texture& t, const render_rect& dst, const texture_draw_params& params) {
            ENFORCE(!has_diagonal_flip(params.flip))("draw_texture: diagonal flip is not supported; "
                                                     "use draw_texture_quad");
            ENFORCE(std::isfinite(params.rotation_degrees))("texture rotation must be finite");

            auto& native = drawable(t);
            const tint_scope tint(native, params.tint);
            const std::optional <sdlpp::rect <float>> target{to_sdl(dst, snapping())};
            if (params.rotation_degrees == 0.0f && params.flip == sprite_flip::none) {
                DRAW_CHECK(get_renderer().copy(native, params.src, target));
                return;
            }
            const std::optional <sdlpp::point <float>> pivot =
                params.pivot ? std::optional <sdlpp::point <float>>{sdlpp::point <float>{params.pivot->x, params.pivot->y}}
                             : std::nullopt;
            DRAW_CHECK(get_renderer().copy_ex(native, params.src, target, static_cast <double>(params.rotation_degrees),
                                              pivot, to_sdl_flip(params.flip)));
        }
    }

    void draw_texture(const texture& t, const rect& dst, const texture_draw_params& params) {
        detail::draw_texture(t, as_render(dst), params);
    }

    void draw_texture(const texture& t) {
        const dim size = t.size();
        detail::draw_texture(t, render_rect{0.0f, 0.0f, static_cast <float>(size.width),
                                            static_cast <float>(size.height)}, {});
    }

    void draw_texture_quad(const texture& t, const std::array <render_pos, 4>& corners, std::optional <rect> src,
                           const color& tint) {
        const dim size = t.size();
        const rect s = src.value_or(rect{0, 0, size.width, size.height});
        const float w = static_cast <float>(size.width);
        const float h = static_cast <float>(size.height);
        const float u0 = static_cast <float>(s.x) / w;
        const float v0 = static_cast <float>(s.y) / h;
        const float u1 = static_cast <float>(s.x + s.w) / w;
        const float v1 = static_cast <float>(s.y + s.h) / h;
        const SDL_FColor c{static_cast <float>(tint.r) / 255.0f, static_cast <float>(tint.g) / 255.0f,
                           static_cast <float>(tint.b) / 255.0f, static_cast <float>(tint.a) / 255.0f};
        const std::array <SDL_FPoint, 4> uv{{{u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}}};

        const bool snap = snapping();
        std::array <SDL_Vertex, 4> vertices{};
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const render_pos p = snap_pos(corners[i], snap);
            vertices[i] = SDL_Vertex{.position = {p.x, p.y}, .color = c, .tex_coord = uv[i]};
        }
        constexpr std::array <int, 6> indices{0, 1, 2, 0, 2, 3};
        DRAW_CHECK(get_renderer().render_geometry(drawable(t).get(), vertices, indices));
    }

    std::uint64_t draw_failure_count() noexcept {
        return g_draw_failures;
    }
}

#undef DRAW_CHECK
