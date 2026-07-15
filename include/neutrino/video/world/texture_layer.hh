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
 * content-keyed or cached. The game creates
 * and updates the texture (typically @c texture_access::streaming) and points this
 * layer at it; the layer just draws it each frame, beneath the map when slotted with
 * @ref world_compositor::insert_bottom. It ignores the @ref sprite_batch -- there is
 * one full-plane blit, nothing to depth-sort.
 */

#include <optional>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/world/render_layer.hh>

#include <sdlpp/video/texture.hh>

namespace neutrino {
    /**
     * @brief A @ref render_layer that fills the viewport with a game-owned texture.
     */
    class NEUTRINO_EXPORT texture_layer : public render_layer {
        public:
            /// @brief The texture to draw. Owned and updated by the game; may be null (no-op).
            const sdlpp::texture* texture{nullptr};

            /// @brief Optional source sub-rectangle; unset draws the whole texture.
            std::optional <rect> source;

            void draw(const layer_view& view, sprite_batch& /*batch*/) override {
                if (texture == nullptr) {
                    return;
                }
                const std::optional <rect> dst{view.viewport};
                // A per-frame draw failure is a no-op, never a throw.
                (void) get_renderer().copy(*texture, source, dst);
            }
    };
}
