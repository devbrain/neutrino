//
// Created by igor on 22/11/2026.
//

#include "video/composite_draw.hh"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

#include <failsafe/enforce.hh>

#include <neutrino/video/globals.hh>
#include <neutrino/video/texture.hh>

#include "video/blend_mode_sdl.hh"
#include "video/draw_failures.hh"
#include "video/flip_mode_sdl.hh"

namespace neutrino::detail {
    namespace {
        [[nodiscard]] std::uint8_t times(std::uint8_t v, float factor) noexcept {
            return static_cast <std::uint8_t>(std::lround(static_cast <float>(v) * factor));
        }

        [[nodiscard]] bool valid_scale(float v) noexcept {
            return std::isfinite(v) && v >= 0.0f;
        }

        // The pivot in the target's pixels: @p pivot, in render units from @p frame's top-left,
        // or @p frame's centre.
        [[nodiscard]] fpoint frame_point(const sdlpp::rect <float>& frame, // sdlpp-ok: internal
                                         const std::optional <point>& pivot, const pixel_mapping& px) noexcept {
            if (!pivot) {
                return {frame.x + frame.w / 2.0f, frame.y + frame.h / 2.0f};
            }
            const fpoint d = px.scaled(*pivot);
            return {frame.x + d.x, frame.y + d.y};
        }
    }

    blend_mode premultiplied(blend_mode mode) noexcept {
        switch (mode) {
            case blend_mode::blend: return blend_mode::blend_premultiplied;
            case blend_mode::add: return blend_mode::add_premultiplied;
            default: return mode;              // none, mul, mod and the premultiplied ones
        }
    }

    blend_mode composite_blend(blend_mode mode, float opacity) noexcept {
        // SDL's mod (src × dst) ignores alpha, so it cannot fade. Below full opacity a mod
        // layer is drawn with mul (src × dst + dst × (1 − srcA)): with colour and alpha
        // scaled by the opacity o, an opaque pixel c gives dst × (1 − o + c × o), the tint
        // faded by o. A mod layer is cleared to opaque white, so its pixels are opaque.
        if (mode == blend_mode::mod && std::clamp(opacity, 0.0f, 1.0f) < 1.0f) {
            return blend_mode::mul;
        }
        return premultiplied(mode);
    }

    bool apply_composite_modulation(texture& t, const layer_composite_settings& s) {
        auto& native = t.native();
        const float opacity = std::clamp(s.opacity, 0.0f, 1.0f);
        const color mod{times(s.tint.r, opacity), times(s.tint.g, opacity), times(s.tint.b, opacity), 255};
        return native.set_blend_mode(to_sdl_blend(composite_blend(s.blend, opacity))).has_value()
               && native.set_color_mod(mod).has_value()
               && native.set_alpha_mod(times(255, opacity)).has_value();
    }

    pixel_mapping pixel_mapping::of_bound_target() {
        pixel_mapping m;
        m.target = bound_target_geometry();
        m.area = {0.0f, 0.0f, static_cast <float>(m.target.size.width), static_cast <float>(m.target.size.height)};
        if (m.target.mode != sdlpp::logical_presentation::disabled && !m.target.logical.empty()) { // sdlpp-ok: internal
            if (const auto shown = get_renderer().get_logical_presentation_rect()) {
                m.area = sdlpp::rect <float>{shown->x, shown->y, shown->w, shown->h}; // sdlpp-ok: internal
                m.scale_x = shown->w / static_cast <float>(m.target.logical.width);
                m.scale_y = shown->h / static_cast <float>(m.target.logical.height);
            }
        }
        return m;
    }

    sdlpp::rect <float> pixel_mapping::to_pixels(const rect& r) const noexcept { // sdlpp-ok: internal
        return {area.x + static_cast <float>(r.x) * scale_x, area.y + static_cast <float>(r.y) * scale_y,
                static_cast <float>(r.w) * scale_x, static_cast <float>(r.h) * scale_y};
    }

    fpoint pixel_mapping::scaled(const point& d) const noexcept {
        return {static_cast <float>(d.x) * scale_x, static_cast <float>(d.y) * scale_y};
    }

    pixel_space::pixel_space()
        : pixel_space(pixel_mapping::of_bound_target()) {
    }

    pixel_space::pixel_space(const pixel_mapping& mapping)
        : m_mapping(mapping) {
        auto& r = get_renderer();
        if (const auto clip = r.get_clip_rect <rect>()) {
            m_clip = *clip;                    // in render units, under the presentation
        } else {
            count_draw_failure();
        }
        apply_presentation(target_geometry{m_mapping.target.size});   // off: the target's own pixels
        if (!r.set_clip_rect(std::nullopt)) {
            count_draw_failure();
        }
    }

    pixel_space::~pixel_space() {
        try {
            apply_presentation(m_mapping.target);
            auto& r = get_renderer();
            const auto put = m_clip ? r.set_clip_rect(*m_clip) : r.set_clip_rect(std::nullopt);
            if (!put) {
                count_draw_failure();
            }
        } catch (...) {
            count_draw_failure();
        }
    }

    const pixel_mapping& pixel_space::mapping() const noexcept {
        return m_mapping;
    }

    bool is_identity_composite(const layer_composite_settings& s) noexcept {
        // Not add: SDL's add keeps the target's alpha, so onto a transparent target it leaves
        // alpha 0 where the texture had some, which is not the texture.
        const bool copies = s.blend == blend_mode::none || s.blend == blend_mode::blend;
        return copies && s.opacity >= 1.0f && s.tint == colors::white && s.offset == point{0, 0}
               && s.scale == layer_scale{1.0f} && s.rotation_degrees == 0.0f && s.flip == flip_mode::none
               && !s.source && !s.destination;
    }

    void validate_composite(const layer_composite_settings& s) {
        ENFORCE(!has_diagonal_flip(s.flip), "a layer or canvas cannot be combined with a diagonal flip");
        ENFORCE(std::isfinite(s.rotation_degrees), "a layer's or canvas's rotation must be finite");
        ENFORCE(valid_scale(s.scale.x) && valid_scale(s.scale.y),
                "a layer's or canvas's scale must be finite and not negative:", s.scale.x, s.scale.y);
        ENFORCE(std::isfinite(s.opacity), "a layer's or canvas's opacity must be finite");
    }

    void draw_composited(texture& t, const layer_composite_settings& s, in_place where,
                         const pixel_space& space) {
        const pixel_mapping& px = space.mapping();
        validate_composite(s);

        const dim size = t.size();
        if (size.empty()) {
            return;
        }
        using frect = sdlpp::rect <float>; // sdlpp-ok: internal
        // The part drawn, in the texture's pixels. A texture that mirrors the target takes its
        // source in render units, like everything else drawn into it; one with a size of its
        // own in its own pixels (R43).
        std::optional <frect> src;
        if (s.source) {
            const rect& r = *s.source;
            src = where == in_place::mirrored
                      ? px.to_pixels(r)
                      : frect{static_cast <float>(r.x), static_cast <float>(r.y),
                              static_cast <float>(r.w), static_cast <float>(r.h)};
        } else if (s.destination && where == in_place::mirrored) {
            // Stretched onto a destination: the picture, not the letterbox bars around it.
            src = px.area;
        }
        const frect part = src.value_or(frect{0.0f, 0.0f, static_cast <float>(size.width),
                                              static_cast <float>(size.height)});

        // The rectangle the texture is drawn onto, in pixels.
        frect dst;
        if (s.destination) {
            dst = px.to_pixels(*s.destination);
        } else if (where == in_place::mirrored) {
            dst = part;                        // the same pixels: 1:1
        } else {
            const auto& area = px.area;
            const float kx = area.w / static_cast <float>(size.width);
            const float ky = area.h / static_cast <float>(size.height);
            dst = {area.x + part.x * kx, area.y + part.y * ky, part.w * kx, part.h * ky};
        }

        // The rectangle the pivot is measured from: the one the layer is drawn onto, except
        // for a texture that mirrors the target drawn whole, which is copied whole, bars
        // included, so the copy stays exact, but placed as the render area it shows.
        const frect frame = !s.destination && !s.source && where == in_place::mirrored ? px.area : dst;
        fpoint pivot = frame_point(frame, s.pivot, px);

        // Scaled about the pivot, which stays where it is; then moved by the offset.
        dst.x = pivot.x + (dst.x - pivot.x) * s.scale.x;
        dst.y = pivot.y + (dst.y - pivot.y) * s.scale.y;
        dst.w *= s.scale.x;
        dst.h *= s.scale.y;
        const fpoint offset = px.scaled(s.offset);
        dst.x += offset.x;
        dst.y += offset.y;
        if (dst.w <= 0.0f || dst.h <= 0.0f) {
            return;
        }
        // copy_ex's centre is measured from the destination's top-left.
        pivot.x += offset.x - dst.x;
        pivot.y += offset.y - dst.y;

        if (!apply_composite_modulation(t, s)) {
            count_draw_failure();
            return;
        }
        auto& r = get_renderer();
        const auto& native = t.native();
        const bool ok = (s.rotation_degrees == 0.0f && s.flip == flip_mode::none)
                            ? r.copy(native, src, dst).has_value()
                            : r.copy_ex(native, src, dst, static_cast <double>(s.rotation_degrees),
                                        sdlpp::point <float>{pivot.x, pivot.y}, // sdlpp-ok: internal
                                        to_sdl_flip(s.flip)).has_value();
        if (!ok) {
            count_draw_failure();
        }
    }
}
