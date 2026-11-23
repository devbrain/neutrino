//
// Created by igor on 26/10/2026.
//

#pragma once

/**
 * @file texture.hh
 * @brief The engine's GPU texture: one type, three ways for pixels to get in.
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
 *
 * A texture keeps no blend of its own: every draw takes the painter's (painter.md §2.8, R49).
 * What it does keep is its @ref alpha_mode, what its pixels hold, from which a draw picks the
 * form of that blend: @c target textures are premultiplied, the others straight.
 */

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/alpha_mode.hh>
#include <neutrino/video/color.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/image/image.hh>
#include <sdlpp/video/texture.hh>

namespace neutrino {
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

    namespace detail {
        /// @brief Internal: what a renderer_target_scope saves of the target bound when it
        ///        begins, and puts back when it ends (painter.md §3.1).
        struct target_state {
            /// The painter had a clip, so the target held it: if the painter has none when the
            /// scope ends, the target's clip is cleared; otherwise it keeps its own (R39).
            bool painter_clip{false};
            /// Drawing into the target did nothing: a layer's or a canvas's whose texture could
            /// not be made (painter.md §3.6).
            bool discards{false};
        };
    }

    /**
     * @brief While alive, neutrino draws land in a target texture; on destruction the previously
     *        current render target is restored. Returned by texture::bind().
     *
     * Moveable, so a binding can span separate calls (begin()/end() flows: hold it in a
     * @c std::optional member and reset() it). Scopes must end in reverse order of creation,
     * and before the application quits: ending one binds the previous target again, which
     * needs the renderer.
     */
    class NEUTRINO_EXPORT renderer_target_scope {
        public:
            renderer_target_scope(renderer_target_scope&& other) noexcept;
            renderer_target_scope& operator=(renderer_target_scope&& other) noexcept;
            renderer_target_scope(const renderer_target_scope&) = delete;
            renderer_target_scope& operator=(const renderer_target_scope&) = delete;
            ~renderer_target_scope();

            /// @brief Restore the previous render target now (idempotent).
            void end() noexcept;

        private:
            friend class texture;
            /// Saves @p r's current target; restores it when the scope ends.
            explicit renderer_target_scope(sdlpp::renderer& r); // sdlpp-ok: private

            std::optional <sdlpp::renderer::target_guard> m_guard; // sdlpp-ok: private
            /// The target bound before: put back when the scope ends.
            detail::target_state m_before;
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
            /// @brief A @c fixed texture holding @p img's pixels. Straight alpha, nearest-filtered.
            /// @return nullopt if the upload fails.
            [[nodiscard]] static std::optional <texture> from_image(const image& img);

            /// @brief A blank texture of @p size, nearest-filtered. A @c target texture is
            ///        premultiplied, the others straight (R49). A @c target texture's contents
            ///        start undefined -- compose() clears it by default.
            ///        @c streaming textures must use a 32-bit @p format (update() / lock() are 32-bit).
            /// @return nullopt if @p size is degenerate or the driver cannot create it (e.g. no
            ///         render-target support).
            [[nodiscard]] static std::optional <texture> create(dim size, texture_access access,
                                                               pixel_format format = pixel_format::RGBA8888);

            /// @brief Wrap an existing SDL texture (interop with code that creates its own). Keeps
            ///        the texture's filter as-is -- no engine defaults applied. The size and access
            ///        are read from @p raw, and the alpha mode follows the access, as for create();
            ///        set_alpha_mode() when the pixels say otherwise. @p raw must have been created on
            ///        the application's renderer, the only one neutrino draws with: a texture from
            ///        another renderer fails ENFORCE.
            [[nodiscard]] static texture adopt(sdlpp::texture raw); // sdlpp-ok

            texture(texture&&) noexcept = default;
            texture& operator=(texture&&) noexcept = default;
            texture(const texture&) = delete;
            texture& operator=(const texture&) = delete;
            ~texture() = default;

            [[nodiscard]] dim size() const noexcept { return m_size; }
            [[nodiscard]] texture_access access() const noexcept { return m_access; }

            void set_filter(texture_filter filter);

            /// @brief Say what the pixels hold (R49): a target drawn into with @c none and
            ///        translucent colour is straight; images exported premultiplied, or a
            ///        streaming texture written so, are premultiplied. It describes the pixels,
            ///        so it can change when they do.
            void set_alpha_mode(neutrino::alpha_mode mode) noexcept { m_alpha = mode; }
            /// @brief What the pixels hold: @c premultiplied for a @c target texture, @c straight
            ///        for the others, unless set_alpha_mode() said otherwise.
            [[nodiscard]] neutrino::alpha_mode alpha_mode() const noexcept { return m_alpha; }

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
            [[nodiscard]] renderer_target_scope bind(std::optional <color> clear = std::nullopt);

            /// @brief Draw into the texture: bind it, clear to @p clear (default transparent;
            ///        nullopt keeps the existing contents), run @p draw, restore the previous target.
            template <typename Fn>
            void compose(Fn&& draw, std::optional <color> clear = colors::transparent) {
                auto scope = bind(clear);
                std::forward <Fn>(draw)();
            }

            /// @brief Clear the texture to @p c (target textures only). The render target and the
            ///        draw colour are left as they were.
            void fill(const color& c) {
                auto scope = bind(c);
            }

            /// @brief Read the texture's pixels back to the CPU (target textures only), as
            ///        straight colour (R49): a premultiplied texture's colour is divided by its
            ///        alpha. A pixel with alpha 0 has no straight colour and reads back as
            ///        transparent black, even if additive light gave it colour.
            [[nodiscard]] image to_image() const;

            // ---- escape hatch ------------------------------------------------------------------

            /// @brief The underlying SDL texture. Drawing it directly bypasses pixel_snap and
            ///        draw-failure counting; prefer draw_texture().
            [[nodiscard]] const sdlpp::texture& native() const noexcept { return m_texture; } // sdlpp-ok
            [[nodiscard]] sdlpp::texture& native() noexcept { return m_texture; }             // sdlpp-ok

        private:
            texture(sdlpp::texture tex, dim size, texture_access access) noexcept // sdlpp-ok
                : m_texture(std::move(tex)), m_size(size), m_access(access),
                  m_alpha(access == texture_access::target ? neutrino::alpha_mode::premultiplied
                                                           : neutrino::alpha_mode::straight) {}

            sdlpp::texture m_texture; // sdlpp-ok: private
            dim m_size{0, 0};
            texture_access m_access{texture_access::fixed};
            neutrino::alpha_mode m_alpha{neutrino::alpha_mode::straight};
    };
}
