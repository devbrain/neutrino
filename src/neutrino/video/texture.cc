//
// See texture.hh.
//

#include <neutrino/video/texture.hh>
#include <neutrino/video/globals.hh>

#include <failsafe/enforce.hh>
#include <SDL3/SDL.h>

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

        // Engine defaults for every texture: alpha-blended (a texture with transparent regions
        // composites over what it is drawn onto) and nearest-filtered (pixel art stays crisp).
        void apply_defaults(sdlpp::texture& tex) {
            (void) tex.set_blend_mode(sdlpp::blend_mode::blend);
            (void) tex.set_scale_mode(sdlpp::scale_mode::nearest);
        }

        [[nodiscard]] image read_current_target() {
            SDL_Surface* raw = SDL_RenderReadPixels(get_renderer().get(), nullptr);
            ENFORCE(raw != nullptr)("reading the render target back failed: ", SDL_GetError());
            return image(raw);
        }
    }

    // ---- target_scope ---------------------------------------------------------------------------

    target_scope::target_scope(target_scope&& other) noexcept
        : m_previous(other.m_previous), m_active(std::exchange(other.m_active, false)) {
    }

    target_scope& target_scope::operator=(target_scope&& other) noexcept {
        if (this != &other) {
            end();
            m_previous = other.m_previous;
            m_active = std::exchange(other.m_active, false);
        }
        return *this;
    }

    target_scope::~target_scope() {
        end();
    }

    void target_scope::end() noexcept {
        if (m_active) {
            SDL_SetRenderTarget(get_renderer().get(), m_previous);
            m_active = false;
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
        return texture(std::move(*tex), dim{img.width(), img.height()}, texture_access::fixed);
    }

    std::optional <texture> texture::create(dim size, texture_access access, pixel_format format) {
        if (size.width <= 0 || size.height <= 0) {
            return std::nullopt;
        }
        ENFORCE(access != texture_access::streaming
                || SDL_BYTESPERPIXEL(static_cast <SDL_PixelFormat>(format)) == 4)
            ("streaming textures must use a 32-bit pixel format");
        auto tex = sdlpp::texture::create(get_renderer(), format, to_sdl(access), size.width, size.height);
        if (!tex) {
            return std::nullopt;
        }
        apply_defaults(*tex);
        return texture(std::move(*tex), size, access);
    }

    texture texture::adopt(sdlpp::texture raw, texture_access access) {
        const auto size = raw.get_size();
        ENFORCE(size.has_value())("adopting a texture whose size cannot be queried");
        return texture(std::move(raw), dim{size->width, size->height}, access);
    }

    void texture::set_filter(texture_filter filter) {
        (void) m_texture.set_scale_mode(to_sdl(filter));
    }

    void texture::set_blend_mode(blend_mode mode) {
        (void) m_texture.set_blend_mode(mode);
    }

    void texture::update(std::span <const std::uint32_t> pixels) {
        ENFORCE(m_access == texture_access::streaming)("update() needs a streaming texture");
        ENFORCE(pixels.size() == static_cast <std::size_t>(m_size.width) * static_cast <std::size_t>(m_size.height))
            ("update() needs exactly width * height pixels");
        const auto result = m_texture.update(pixels.data(), m_size.width * static_cast <int>(sizeof(std::uint32_t)));
        ENFORCE(result.has_value())("texture update failed");
    }

    pixel_lock texture::lock() {
        ENFORCE(m_access == texture_access::streaming)("lock() needs a streaming texture");
        auto locked = m_texture.lock();
        ENFORCE(locked.has_value())("texture lock failed");
        return pixel_lock(&m_texture, locked->first, locked->second, m_size.width, m_size.height);
    }

    target_scope texture::bind(std::optional <color> clear) {
        ENFORCE(m_access == texture_access::target)("bind() needs a target texture");
        auto* r = get_renderer().get();
        SDL_Texture* previous = SDL_GetRenderTarget(r);
        ENFORCE(SDL_SetRenderTarget(r, m_texture.get()))("binding the render target failed: ", SDL_GetError());
        target_scope scope(previous);
        if (clear) {
            // Clearing uses the renderer's draw colour; restore it so a bind leaves it untouched.
            Uint8 pr = 0, pg = 0, pb = 0, pa = 0;
            SDL_GetRenderDrawColor(r, &pr, &pg, &pb, &pa);
            SDL_SetRenderDrawColor(r, clear->r, clear->g, clear->b, clear->a);
            SDL_RenderClear(r);
            SDL_SetRenderDrawColor(r, pr, pg, pb, pa);
        }
        return scope;
    }

    image texture::to_image() const {
        ENFORCE(m_access == texture_access::target)("to_image() needs a target texture");
        auto* r = get_renderer().get();
        SDL_Texture* previous = SDL_GetRenderTarget(r);
        ENFORCE(SDL_SetRenderTarget(r, m_texture.get()))("binding the render target failed: ", SDL_GetError());
        target_scope restore(previous);
        return read_current_target();
    }

    image capture_frame() {
        return read_current_target();
    }
}
