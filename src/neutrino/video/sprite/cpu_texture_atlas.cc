//
// Created by igor on 04/07/2026.
//

#include <utility>
#include <neutrino/video/sprite/cpu_texture_atlas.hh>
#include <failsafe/enforce.hh>

#include "video/sprite/surface_lock.hh"

namespace neutrino {
    namespace {
        using details::surface_pixel_lock;

        bool has_alpha_channel(sdlpp::pixel_format_enum format) {
            return SDL_ISPIXELFORMAT_ALPHA(static_cast <SDL_PixelFormat>(format));
        }

        bool same_rgb(const sdlpp::color& lhs, const sdlpp::color& rhs) {
            return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
        }
    }

    std::optional <bitmask> cpu_texture_atlas_frame::evaluate_bitmask(
        const sdlpp::surface& s,
        const rect& atlas_rect,
        const cpu_texture_atlas_mask_options& options) {
        if (!s || atlas_rect.x < 0 || atlas_rect.y < 0 || atlas_rect.w < 0 || atlas_rect.h < 0) {
            return std::nullopt;
        }

        const auto right = static_cast <std::size_t>(atlas_rect.x + atlas_rect.w);
        const auto bottom = static_cast <std::size_t>(atlas_rect.y + atlas_rect.h);
        if (right > s.width() || bottom > s.height()) {
            return std::nullopt;
        }

        const auto format = s.format();
        if (options.key_color_index && format != sdlpp::pixel_format_enum::INDEX8) {
            return std::nullopt;
        }

        surface_pixel_lock lock(s);
        if (!lock.ready()) {
            return std::nullopt;
        }

        const auto* raw_surface = s.get();
        const auto* indexed_pixels =
            options.key_color_index && raw_surface ? static_cast <const uint8_t*>(raw_surface->pixels) : nullptr;
        const int indexed_pitch = raw_surface ? raw_surface->pitch : 0;
        if (options.key_color_index && !indexed_pixels) {
            return std::nullopt;
        }

        const bool alpha_source = has_alpha_channel(format);
        const bool keyed_color_source = options.key_color.has_value();

        bitmask evaluated(atlas_rect.w, atlas_rect.h);
        for (int y = 0; y < atlas_rect.h; ++y) {
            for (int x = 0; x < atlas_rect.w; ++x) {
                const int sx = atlas_rect.x + x;
                const int sy = atlas_rect.y + y;

                bool solid = true;
                if (options.key_color_index) {
                    const uint8_t pixel_index = indexed_pixels[static_cast <std::size_t>(sy * indexed_pitch + sx)];
                    solid = pixel_index != *options.key_color_index;
                } else {
                    const auto color = s.get_pixel(sx, sy);
                    if (!color) {
                        return std::nullopt;
                    }

                    if (keyed_color_source && same_rgb(*color, *options.key_color)) {
                        solid = false;
                    } else if (alpha_source) {
                        solid = color->a != 0;
                    }
                }

                evaluated.set(x, y, solid);
            }
        }

        return evaluated;
    }

    cpu_texture_atlas::cpu_texture_atlas(sdlpp::surface&& s)
        : m_surface(std::move(s)) {
    }

    cpu_texture_atlas::cpu_texture_atlas(sdlpp::surface&& s, std::vector <cpu_texture_atlas_frame>&& frames)
        : m_surface(std::move(s)),
          m_frames(std::move(frames)) {
    }

    const sdlpp::surface& cpu_texture_atlas::surface() const noexcept {
        return m_surface;
    }

    std::size_t cpu_texture_atlas::frame_count() const noexcept {
        return m_frames.size();
    }

    const cpu_texture_atlas_frame& cpu_texture_atlas::frame(std::size_t index) const {
        ENFORCE(index < m_frames.size());
        return m_frames[index];
    }

    void cpu_texture_atlas::add_frame(const cpu_texture_atlas_frame& f) {
        m_frames.push_back(f);
    }
}
