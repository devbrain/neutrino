//
// Created by igor on 22/07/2026.
//

#pragma once

/**
 * @file render_texture.hh
 * @brief An offscreen render target: compose neutrino draws into a texture once, blit it many.
 *
 * Wraps a target-access @c sdlpp::texture and the renderer's target guard. @ref compose binds
 * the texture as the render target, so anything drawn inside it -- a @ref sprite_batch flush,
 * @c draw_* primitives -- lands in the texture instead of on screen; @ref blit then draws that
 * texture wherever needed. This is the missing primitive for composited *static* content: a
 * level backdrop, a cached HUD frame, a minimap, a bake-once effect. Compose when the content
 * changes (e.g. per level), not every frame; the texture is alpha-blended, so transparent
 * regions of the compose show through when it is blitted.
 */

#include <optional>
#include <utility>

#include <sdlpp/video/renderer.hh>
#include <sdlpp/video/texture.hh>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/globals.hh>

namespace neutrino {
    /**
     * @brief An offscreen render target you compose into once and blit many times.
     */
    class NEUTRINO_EXPORT render_texture {
        public:
            /**
             * @brief Create an RGBA target texture of @p size on the global renderer.
             *
             * Alpha-blended, so it composites over whatever it is later blitted onto.
             * @return nullopt if @p size is degenerate, or the texture cannot be created
             *         (e.g. the renderer/driver has no render-target support).
             */
            [[nodiscard]] static std::optional <render_texture> create(dim size);

            render_texture(const render_texture&) = delete;
            render_texture& operator=(const render_texture&) = delete;
            render_texture(render_texture&&) = default;
            render_texture& operator=(render_texture&&) = default;
            ~render_texture() = default;

            /**
             * @brief Draw into the texture: bind it as the render target, clear to @p clear
             *        (default transparent -- a target's contents start undefined), run @p draw,
             *        then restore the previous target.
             *
             * @p draw issues ordinary neutrino draws (a @ref sprite_batch flush, @c draw_*),
             * which now land in this texture. Call it once, or whenever the content changes --
             * not every frame.
             */
            template <typename Fn>
            void compose(Fn&& draw, const sdlpp::color& clear = sdlpp::color{0, 0, 0, 0}) {
                sdlpp::renderer& r = get_renderer();
                const sdlpp::renderer::target_guard guard(r, m_texture);
                // clear() / draw() mutate the renderer's shared draw colour; the target_guard
                // restores the render target but not the colour, so scope it here.
                const auto prev_color = r.get_draw_color();
                (void) r.set_draw_color(clear);
                (void) r.clear();
                std::forward <Fn>(draw)();
                if (prev_color) {
                    (void) r.set_draw_color(*prev_color);
                }
            }

            /// @brief Blit the whole texture to @p dst on the current render target.
            void blit(const rect& dst) const;

            /// @brief Blit the texture 1:1 at the origin (dst == its own size).
            void blit() const;

            [[nodiscard]] const sdlpp::texture& texture() const noexcept { return m_texture; }
            [[nodiscard]] dim size() const noexcept { return m_size; }

        private:
            render_texture(sdlpp::texture tex, dim size) noexcept
                : m_texture(std::move(tex)), m_size(size) {}

            sdlpp::texture m_texture;
            dim m_size{0, 0};
    };
}
