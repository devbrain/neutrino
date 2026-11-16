//
// See texture.hh.
//

#include <neutrino/video/texture.hh>
#include <neutrino/video/globals.hh>

#include "video/blend_mode_sdl.hh"

#include <failsafe/enforce.hh>
#include <failsafe/exception.hh>

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

        // Engine defaults for every texture: alpha-blended (a texture with transparent regions
        // composites over what it is drawn onto) and nearest-filtered (pixel art stays crisp).
        void apply_defaults(sdlpp::texture& tex) {
            (void) tex.set_blend_mode(sdlpp::blend_mode::blend);
            (void) tex.set_scale_mode(sdlpp::scale_mode::nearest);
        }

        [[nodiscard]] image read_current_target() {
            auto pixels = get_renderer().read_pixels();
            if (!pixels) {
                THROW_RUNTIME("reading the render target back failed:", pixels.error());
            }
            return std::move(*pixels);
        }

        // Make @p tex the render target; a failure is an error, not a no-op.
        void set_render_target(const sdlpp::texture& tex) {
            if (const auto bound = get_renderer().set_target(tex); !bound) {
                THROW_RUNTIME("binding the render target failed:", bound.error());
            }
        }
    }

    // ---- target_scope ---------------------------------------------------------------------------

    // The guard restores on destruction and a moved-from guard restores nothing, so moving and
    // resetting the optional is all the bookkeeping a scope needs.
    target_scope::target_scope(target_scope&& other) noexcept
        : m_guard(std::move(other.m_guard)) {
        other.m_guard.reset();
    }

    target_scope& target_scope::operator=(target_scope&& other) noexcept {
        if (this != &other) {
            end();
            m_guard = std::move(other.m_guard);
            other.m_guard.reset();
        }
        return *this;
    }

    target_scope::~target_scope() {
        end();
    }

    void target_scope::end() noexcept {
        m_guard.reset();
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

    void texture::set_blend_mode(blend_mode mode) {
        (void) m_texture.set_blend_mode(detail::to_sdl_blend(mode));
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

    target_scope texture::bind(std::optional <color> clear) {
        ENFORCE(m_access == texture_access::target, "bind() needs a target texture");
        auto& r = get_renderer();
        target_scope scope(r);
        set_render_target(m_texture);
        if (clear) {
            // Clearing uses the renderer's draw colour; restore it so a bind leaves it untouched.
            const auto previous = r.get_draw_color();
            (void) r.set_draw_color(*clear);
            (void) r.clear();
            if (previous) {
                (void) r.set_draw_color(*previous);
            }
        }
        return scope;
    }

    image texture::to_image() const {
        ENFORCE(m_access == texture_access::target, "to_image() needs a target texture");
        target_scope restore(get_renderer());
        set_render_target(m_texture);
        return read_current_target();
    }

    image capture_frame() {
        return read_current_target();
    }
}
