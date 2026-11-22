//
// Created by igor on 22/11/2026.
//

/// @file composition.hh
/// @brief What a compositor receives, and the default compositor (painter.md §2.9).

#pragma once

#include <functional>
#include <span>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/layer_style.hh>
#include <neutrino/video/layer_id.hh>

namespace neutrino {
    class texture;

    namespace detail {
        class layer_stack;
        class scene_frame;
    }

    /**
     * @brief A scene's finished layers, handed to its compositor, which draws them onto the
     *        scene's canvas (painter.md §2.9, R43).
     *
     * Made by the engine for one compositor call, after the scene's render(), with the canvas
     * bound and the default painter state. The free draw functions draw onto the canvas
     * meanwhile: a vignette, a debug overlay. Pushing a layer fails: the layers are finished.
     * A layer the compositor does not draw is not shown.
     *
     * Valid only during the call it is passed to; do not keep it. Main thread only.
     */
    class NEUTRINO_EXPORT composition {
        friend class detail::scene_frame;

        public:
            composition(const composition&) = delete;
            composition& operator=(const composition&) = delete;

            /// @brief The scene's base layer, then its declared layers, in declaration order.
            [[nodiscard]] std::span <const layer_id> layers() const noexcept;
            /// @brief True if the layer has something to show: pushed this frame, or redrawn on
            ///        demand and not needing a redraw. ENFORCE: a layer of this scene.
            [[nodiscard]] bool has_content(layer_id lid) const;
            /// @brief Draws the layer onto the canvas with its own style. Does
            ///        nothing for a layer without content. ENFORCE: a layer of this scene.
            void draw_layer(layer_id lid);
            /// @brief Draws the layer onto the canvas with @p settings instead of its own: a
            ///        second copy, a part, another blend. Does nothing for a layer without
            ///        content. ENFORCE: a layer of this scene; @p settings as the struct says.
            void draw_layer(layer_id lid, const layer_style& settings);
            /// @brief draw_layer() for each layer from @p first to @p last, both included, in
            ///        declaration order. ENFORCE: both are this scene's, @p first not after
            ///        @p last.
            void draw_layers(layer_id first, layer_id last);
            /// @brief The layer's texture, for draw_texture() or a quad with a source rect; null
            ///        while it has none. ENFORCE: a layer of this scene.
            [[nodiscard]] const neutrino::texture* texture(layer_id lid) const;
            /// @brief The canvases of the visible scenes below this one, bottom to top, without
            ///        those with nothing drawn: what will show under this scene's canvas.
            ///        Read-only; valid during this call only.
            [[nodiscard]] std::span <const neutrino::texture* const> below() const noexcept;
        private:
            composition(detail::layer_stack& layers, std::span <const neutrino::texture* const> below) noexcept;

            detail::layer_stack& m_layers;
            std::span <const neutrino::texture* const> m_below;
    };

    /// @brief Draws a scene's layers onto its canvas: what a scene's compositor is.
    using compositor = std::function <void(composition&)>;

    /**
     * @brief The compositor every scene starts with: each layer with content, in declaration
     *        order, with its own style.
     *
     * A public function: a compositor of the game's own can call it as a building block, and
     * `set_compositor(default_compositor)` puts it back.
     */
    NEUTRINO_EXPORT void default_compositor(composition& c);
}
