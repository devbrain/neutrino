//
// Created by igor on 16/11/2026.
//

#pragma once

#include <optional>
#include <neutrino/video/blend_mode.hh>
#include <neutrino/video/flip_mode.hh>
#include <neutrino/video/color.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/neutrino_export.h>

namespace neutrino {
    /**
     * @brief How a finished layer — or a scene's canvas — is drawn when it is combined
     *        (painter.md §2.7).
     *
     * The settings are live: change them at any time, and the compositor reads them when it
     * runs, after `render()`. Animating them frame by frame is how layer effects move; nothing
     * is redrawn into the layer. The flat settings place the layer as one quad; mesh effects
     * come with RM4.
     */
    struct NEUTRINO_EXPORT layer_composite_settings {
        /// @brief How the layer combines with what is below; the painter uses the premultiplied
        ///        form of @c blend and @c add (painter.md §2.8).
        blend_mode blend{blend_mode::blend};
        /// @brief 0 (invisible) to 1. Scales colour as well as alpha.
        float      opacity{1.0f};
        /// @brief Multiplies the layer's colour: fade to black, a damage flash, night.
        color      tint{colors::white};
        /// @brief Moves the layer: screen shake.
        point      offset{};
        /// @brief Scales the layer about @ref pivot.
        float      scale{1.0f};
        /// @brief Rotates the layer about @ref pivot, clockwise.
        float      rotation_degrees{0.0f};
        /// @brief The centre of @ref scale and @ref rotation_degrees.
        point      pivot{};
        /// @brief Mirrors the layer. Whether @c diagonal is allowed is open (render_model.md R35).
        flip_mode  flip{};
        /// @brief Only this part of the layer, in its pixels. Unset: all of it.
        std::optional<rect> source;
    };
}
