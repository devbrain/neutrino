//
// Created by igor on 26/10/2026.
//

#pragma once

/**
 * @file texture.hh
 * @brief The engine's GPU texture: one type, three ways for pixels to get in (roadmap P3.4).
 *
 * A @ref texture lives on the GPU and is drawn with draw_texture() / draw_texture_quad()
 * (draw.hh), which give it the same contract as every other draw: float render space, the
 * pixel_snap policy, and draw-failure counting. Its @ref texture_access decides how pixels get in:
 *
 *   - @c fixed     -- uploaded once from an @ref image (backdrops, wallpapers, atlases).
 *   - @c streaming -- rewritten by the CPU each frame through update() / lock() (software canvases).
 *   - @c target    -- drawn into by the GPU: neutrino draws land in it inside compose() / bind().
 *
 * Operations that only make sense for one mode (update() on a fixed texture, bind() on a
 * streaming one) fail ENFORCE and throw: they are programming errors, not runtime conditions.
 *
 * Inside compose() / bind() coordinates are the texture's own pixels ((0, 0) is its top-left):
 * SDL keeps viewport and scale per render target, and logical presentation applies only to the
 * window. Bindings nest -- each restores the target that was current before it.
 */

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/color.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/image.hh>
#include <sdlpp/video/texture.hh>

struct SDL_Texture;

namespace neutrino {
    /// @brief How drawn pixels combine with the target: @c blend_mode::none, @c ::blend, ...
    using blend_mode = sdlpp::blend_mode;   // sdlpp-ok

    /// @brief How a texture's pixels get in. See @ref texture.
    enum class texture_access : std::uint8_t {
        fixed,     ///< Uploaded once (from_image()); immutable afterwards.
        streaming, ///< Rewritten by the CPU (update() / lock()).
        target,    ///< Drawn into by the GPU (compose() / bind()).
    };

    /// @brief How a texture is sampled when drawn at a different size.
    enum class texture_filter : std::uint8_t {
        nearest,  ///< Hard pixel edges (pixel art). The default.
        linear,   ///< Bilinear smoothing.
        pixelart, ///< Sharp pixels with anti-aliased edges at non-integer scales.
    };

    class texture;

    /**
     * @brief While alive, neutrino draws land in a target texture; on destruction the previously
     *        current render target is restored. Returned by texture::bind().
     *
     * Moveable, so a binding can span separate calls (begin()/end() flows: hold it in a
     * @c std::optional member and reset() it). Scopes must end in reverse order of creation.
     */
    class NEUTRINO_EXPORT target_scope {
        public:
            target_scope(target_scope&& other) noexcept;
            target_scope& operator=(target_scope&& other) noexcept;
            target_scope(const target_scope&) = delete;
            target_scope& operator=(const target_scope&) = delete;
            ~target_scope();

            /// @brief Restore the previous render target now (idempotent).
            void end() noexcept;

        private:
            friend class texture;
            explicit target_scope(SDL_Texture* previous) noexcept
                : m_previous(previous), m_active(true) {}

            SDL_Texture* m_previous{nullptr};
            bool m_active{false};
    };

    /**
     * @brief Write access to a streaming texture's pixels; uploads on destruction.
     *        Returned by texture::lock(). Rows may be padded: index through row().
     */
    class NEUTRINO_EXPORT pixel_lock {
        public:
            pixel_lock(pixel_lock&& other) noexcept;
            pixel_lock& operator=(pixel_lock&&) = delete;
            pixel_lock(const pixel_lock&) = delete;
            pixel_lock& operator=(const pixel_lock&) = delete;
            ~pixel_lock();

            /// @brief The 32-bit pixels of row @p y (width() entries).
            [[nodiscard]] std::span <std::uint32_t> row(int y) const noexcept;
            [[nodiscard]] int width() const noexcept { return m_width; }
            [[nodiscard]] int height() const noexcept { return m_height; }

        private:
            friend class texture;
            pixel_lock(sdlpp::texture* tex, void* pixels, int pitch, int width, int height) noexcept // sdlpp-ok
                : m_texture(tex), m_pixels(static_cast <std::byte*>(pixels)), m_pitch(pitch),
                  m_width(width), m_height(height) {}

            sdlpp::texture* m_texture; // sdlpp-ok: private
            std::byte* m_pixels;
            int m_pitch;
            int m_width;
            int m_height;
    };

    /**
     * @brief A GPU texture. Move-only; created on the application's renderer.
     */
    class NEUTRINO_EXPORT texture {
        public:
            /// @brief A @c fixed texture holding @p img's pixels. Alpha-blended, nearest-filtered.
            /// @return nullopt if the upload fails.
            [[nodiscard]] static std::optional <texture> from_image(const image& img);

            /// @brief A blank texture of @p size. Alpha-blended, nearest-filtered. A @c target
            ///        texture's contents start undefined -- compose() clears it by default.
            ///        @c streaming textures must use a 32-bit @p format (update() / lock() are 32-bit).
            /// @return nullopt if @p size is degenerate or the driver cannot create it (e.g. no
            ///         render-target support).
            [[nodiscard]] static std::optional <texture> create(dim size, texture_access access,
                                                               pixel_format format = pixel_format::RGBA8888);

            /// @brief Wrap an existing SDL texture (interop with code that creates its own). Keeps
            ///        the texture's blend and filter state as-is -- no engine defaults applied.
            ///        @p access must describe how @p raw was created.
            [[nodiscard]] static texture adopt(sdlpp::texture raw, texture_access access); // sdlpp-ok

            texture(texture&&) noexcept = default;
            texture& operator=(texture&&) noexcept = default;
            texture(const texture&) = delete;
            texture& operator=(const texture&) = delete;
            ~texture() = default;

            [[nodiscard]] dim size() const noexcept { return m_size; }
            [[nodiscard]] texture_access access() const noexcept { return m_access; }

            void set_filter(texture_filter filter);
            void set_blend_mode(blend_mode mode);

            // ---- streaming ---------------------------------------------------------------------

            /// @brief Replace every pixel. @p pixels holds size().width * size().height 32-bit
            ///        values, row-major, in this texture's format.
            void update(std::span <const std::uint32_t> pixels);

            /// @brief Write pixels in place; they are uploaded when the lock is destroyed.
            [[nodiscard]] pixel_lock lock();

            // ---- target ------------------------------------------------------------------------

            /// @brief Make this texture the render target until the returned scope ends. If
            ///        @p clear is set, the texture is cleared to it first; nullopt keeps the
            ///        existing contents (persistent canvases). The draw colour is left untouched.
            [[nodiscard]] target_scope bind(std::optional <color> clear = std::nullopt);

            /// @brief Draw into the texture: bind it, clear to @p clear (default transparent;
            ///        nullopt keeps the existing contents), run @p draw, restore the previous target.
            template <typename Fn>
            void compose(Fn&& draw, std::optional <color> clear = colors::transparent) {
                auto scope = bind(clear);
                std::forward <Fn>(draw)();
            }

            /// @brief Read the texture's pixels back to the CPU (target textures only).
            [[nodiscard]] image to_image() const;

            // ---- escape hatch ------------------------------------------------------------------

            /// @brief The underlying SDL texture. Drawing it directly bypasses pixel_snap and
            ///        draw-failure counting; prefer draw_texture().
            [[nodiscard]] const sdlpp::texture& native() const noexcept { return m_texture; } // sdlpp-ok
            [[nodiscard]] sdlpp::texture& native() noexcept { return m_texture; }             // sdlpp-ok

        private:
            texture(sdlpp::texture tex, dim size, texture_access access) noexcept // sdlpp-ok
                : m_texture(std::move(tex)), m_size(size), m_access(access) {}

            sdlpp::texture m_texture; // sdlpp-ok: private
            dim m_size{0, 0};
            texture_access m_access{texture_access::fixed};
    };

    /// @brief Read the CURRENT render target back to the CPU: the window between drawing and
    ///        present (its contents are undefined after present), or the texture bound by
    ///        compose() / bind(). For a specific target texture prefer texture::to_image().
    NEUTRINO_EXPORT [[nodiscard]] image capture_frame();
}
