//
// Created by igor on 03/07/2026.
//

#include <neutrino/video/draw.hh>
#include <sdlpp/video/renderer.hh>
#include <algorithm>
#include <cmath>

namespace neutrino {

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
