//
// Created by igor on 10/07/2026.
//

#pragma once

/**
 * @file texture_layer.hh
 * @brief A live / procedural background: a @ref render_layer that blits a
 *        game-owned texture across the viewport.
 *
 * Its content changes every frame, so -- unlike a static image layer -- it cannot be
 * content-keyed or cached. The game owns a @ref texture -- typically @c texture_access::streaming
 * rewritten by the CPU, or a @c target composed on the GPU -- and points this layer at it; the
 * layer draws it each frame, beneath the map when slotted with @ref world_compositor::insert_bottom.
 * It ignores the @ref sprite_batch -- there is one full-plane draw, nothing to depth-sort.
 */

#include <optional>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/world/render_layer.hh>


namespace neutrino {
    /**
     * @brief A @ref render_layer that fills the viewport with a game-owned texture.
     */
    class NEUTRINO_EXPORT texture_layer : public render_layer {
        public:
            /// @brief The texture to draw. Owned and updated by the game; may be null (no-op).
            const neutrino::texture* texture{nullptr};

            /// @brief Optional source sub-rectangle; unset draws the whole texture.
            std::optional <rect> source;

            void draw(const layer_view& view, sprite_batch& /*batch*/) override {
                if (texture == nullptr) {
                    return;
                }
                draw_texture(*texture, view.viewport, texture_draw_params{.src = source});
            }
    };
}
