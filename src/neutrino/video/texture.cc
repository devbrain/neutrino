//
// See texture.hh.
//

#include <neutrino/video/texture.hh>
#include <neutrino/video/globals.hh>

#include "video/painter_context.hh"
#include "video/target_geometry.hh"


#include <failsafe/enforce.hh>
#include <failsafe/exception.hh>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace neutrino {
    namespace {
        [[nodiscard]] sdlpp::scale_mode to_sdl(texture_filter filter) noexcept {
            switch (filter) {
                case texture_filter::linear: return sdlpp::scale_mode::linear;
                case texture_filter::pixelart: return sdlpp::scale_mode::pixelart;
                case texture_filter::nearest: break;
            }
            return sdlpp::scale_mode::nearest;
        }

        [[nodiscard]] sdlpp::texture_access to_sdl(texture_access access) noexcept {
            switch (access) {
                case texture_access::streaming: return sdlpp::texture_access::streaming;
                case texture_access::target: return sdlpp::texture_access::target;
                case texture_access::fixed: break;
            }
            return sdlpp::texture_access::static_access;
        }

        [[nodiscard]] texture_access from_sdl(sdlpp::texture_access access) noexcept {
            switch (access) {
                case sdlpp::texture_access::streaming: return texture_access::streaming;
                case sdlpp::texture_access::target: return texture_access::target;
                case sdlpp::texture_access::static_access: break;
            }
            return texture_access::fixed;
        }

        // Engine defaults for every texture: nearest-filtered (pixel art stays crisp). No blend:
        // every draw sets the painter's on its source (R49).
        void apply_defaults(sdlpp::texture& tex) {
            (void) tex.set_scale_mode(sdlpp::scale_mode::nearest);
        }

        [[nodiscard]] image read_current_target() {
            auto pixels = get_renderer().read_pixels();
            if (!pixels) {
                THROW_RUNTIME("reading the render target back failed:", pixels.error());
            }
            return std::move(*pixels);
        }

        // One premultiplied channel as straight colour: divided by @p a, rounded, at most 255.
        [[nodiscard]] std::uint32_t straight_channel(std::uint32_t c, std::uint32_t a) noexcept {
            return std::min <std::uint32_t>(255u, (c * 255u + a / 2u) / a);
        }

        // @p img, premultiplied, as straight colour (R49): each pixel's colour divided by its
        // alpha. A pixel with alpha 0 has no straight colour; it becomes transparent black.
        // Worked on in ARGB8888, and handed back in the format it came in.
        [[nodiscard]] image straight_from_premultiplied(image img) {
            const pixel_format original = img.format();
            if (original != pixel_format::ARGB8888) {
                auto converted = img.convert(pixel_format::ARGB8888);
                if (!converted) {
                    THROW_RUNTIME("converting a read-back texture failed:", converted.error());
                }
                img = std::move(*converted);
            }
            if (const auto locked = img.lock(); !locked) {
                THROW_RUNTIME("locking a read-back texture failed:", locked.error());
            }
            const std::span <std::uint8_t> bytes = img.pixels();
            const std::size_t pitch = img.pitch();
            const auto width = static_cast <std::size_t>(img.width());
            const auto height = static_cast <std::size_t>(img.height());
            for (std::size_t y = 0; y < height; ++y) {
                auto* row = reinterpret_cast <std::uint32_t*>(bytes.data() + y * pitch);
                for (std::size_t x = 0; x < width; ++x) {
                    const std::uint32_t p = row[x];
                    const std::uint32_t a = p >> 24;
                    if (a == 255u) {
                        continue;
                    }
                    if (a == 0u) {
                        row[x] = 0u;
                        continue;
                    }
                    row[x] = (a << 24) | (straight_channel((p >> 16) & 0xFFu, a) << 16)
                             | (straight_channel((p >> 8) & 0xFFu, a) << 8) | straight_channel(p & 0xFFu, a);
                }
            }
            img.unlock();
            if (original != pixel_format::ARGB8888) {
                auto back = img.convert(original);
                if (!back) {
                    THROW_RUNTIME("converting a read-back texture failed:", back.error());
                }
                return std::move(*back);
            }
            return img;
        }

        // Make @p tex the render target; a failure is an error, not a no-op.
        void set_render_target(const sdlpp::texture& tex) {
            const auto bound = get_renderer().set_target(tex);
            detail::count_target_switch();        // counted even on failure: the target may have changed
            if (!bound) {
                THROW_RUNTIME("binding the render target failed:", bound.error());
            }
        }
    }

    // ---- renderer_target_scope ------------------------------------------------------------------

    // The guard restores on destruction and a moved-from guard restores nothing, so moving and
    // resetting the optional is all the bookkeeping a scope needs.
    renderer_target_scope::renderer_target_scope(sdlpp::renderer& r) // sdlpp-ok: private
        : m_before(detail::save_target()) {
        m_guard.emplace(r);
    }

    renderer_target_scope::renderer_target_scope(renderer_target_scope&& other) noexcept
        : m_guard(std::move(other.m_guard)), m_before(other.m_before) {
        other.m_guard.reset();
    }

    renderer_target_scope& renderer_target_scope::operator=(renderer_target_scope&& other) noexcept {
        if (this != &other) {
            end();
            m_guard = std::move(other.m_guard);
            m_before = other.m_before;
            other.m_guard.reset();
        }
        return *this;
    }

    renderer_target_scope::~renderer_target_scope() {
        end();
    }

    void renderer_target_scope::end() noexcept {
        if (m_guard) {
            m_guard.reset();                      // the previous target is bound again
            detail::count_target_switch();
            detail::restore_target(m_before);     // it discards as before; its clip
        }
    }

    // ---- pixel_lock -----------------------------------------------------------------------------

    pixel_lock::pixel_lock(pixel_lock&& other) noexcept
        : m_texture(std::exchange(other.m_texture, nullptr)), m_pixels(other.m_pixels),
          m_pitch(other.m_pitch), m_width(other.m_width), m_height(other.m_height) {
    }

    pixel_lock::~pixel_lock() {
        if (m_texture) {
            m_texture->unlock();
        }
    }

    std::span <std::uint32_t> pixel_lock::row(int y) const noexcept {
        auto* first = reinterpret_cast <std::uint32_t*>(m_pixels + static_cast <std::ptrdiff_t>(y) * m_pitch);
        return {first, static_cast <std::size_t>(m_width)};
    }

    // ---- texture --------------------------------------------------------------------------------

    std::optional <texture> texture::from_image(const image& img) {
        auto tex = sdlpp::texture::create(get_renderer(), img);
        if (!tex) {
            return std::nullopt;
        }
        apply_defaults(*tex);
        return texture(std::move(*tex), dim{static_cast <int>(img.width()), static_cast <int>(img.height())}, texture_access::fixed);
    }

    std::optional <texture> texture::create(dim size, texture_access access, pixel_format format) {
        if (size.width <= 0 || size.height <= 0) {
            return std::nullopt;
        }
        ENFORCE(access != texture_access::streaming || sdlpp::bytes_per_pixel(format) == 4,
            "streaming textures must use a 32-bit pixel format");
        auto tex = sdlpp::texture::create(get_renderer(), format, to_sdl(access), size.width, size.height);
        if (!tex) {
            return std::nullopt;
        }
        apply_defaults(*tex);
        return texture(std::move(*tex), size, access);
    }

    texture texture::adopt(sdlpp::texture raw) {
        // SDL can bind and draw a texture only with the renderer that created it; neutrino uses
        // the application's for both, so a texture from another one would fail at first use.
        const auto owner = raw.get_renderer();
        ENFORCE(owner.has_value() && *owner == get_renderer().get(),
                "adopting a texture created on another renderer");
        const auto size = raw.get_size();
        ENFORCE(size.has_value(), "adopting a texture whose size cannot be queried");
        const auto access = raw.get_access();
        ENFORCE(access.has_value(), "adopting a texture whose access cannot be queried");
        return {std::move(raw), dim{size->width, size->height}, from_sdl(*access)};
    }

    void texture::set_filter(texture_filter filter) {
        (void) m_texture.set_scale_mode(to_sdl(filter));
    }

    void texture::update(std::span <const std::uint32_t> pixels) {
        ENFORCE(m_access == texture_access::streaming, "update() needs a streaming texture");
        ENFORCE(pixels.size() == static_cast <std::size_t>(m_size.width) * static_cast <std::size_t>(m_size.height),
            "update() needs exactly width * height pixels");
        const auto result = m_texture.update(pixels.data(), m_size.width * static_cast <int>(sizeof(std::uint32_t)));
        ENFORCE(result.has_value(), "texture update failed");
    }

    pixel_lock texture::lock() {
        ENFORCE(m_access == texture_access::streaming, "lock() needs a streaming texture");
        auto locked = m_texture.lock();
        ENFORCE(locked.has_value(), "texture lock failed");
        return {&m_texture, locked->first, locked->second, m_size.width, m_size.height};
    }

    renderer_target_scope texture::bind(std::optional <color> clear) {
        ENFORCE(m_access == texture_access::target, "bind() needs a target texture");
        auto& r = get_renderer();
        renderer_target_scope scope(r);
        set_render_target(m_texture);
        detail::texture_bound();                  // drawn into; the painter's clip, or none (R39)
        if (clear) {
            // Clearing uses the renderer's draw colour, which is scratch: every draw call sets
            // the colour it uses (R48).
            (void) r.set_draw_color(*clear);
            (void) r.clear();
        }
        return scope;
    }

    image texture::to_image() const {
        ENFORCE(m_access == texture_access::target, "to_image() needs a target texture");
        renderer_target_scope restore(get_renderer());
        set_render_target(m_texture);
        image img = read_current_target();
        if (m_alpha == neutrino::alpha_mode::premultiplied) {
            return straight_from_premultiplied(std::move(img));
        }
        return img;
    }
}
