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
     * @brief A scale factor per axis (render_model.md R43). One @c float scales both axes:
     *        `settings.scale = 2.0f`.
     */
    struct NEUTRINO_EXPORT layer_scale {
        float x{1.0f};
        float y{1.0f};

        /// @brief No scaling.
        constexpr layer_scale() noexcept = default;
        /// @brief @p uniform on both axes. Implicit, so a plain @c float reads as a uniform scale.
        constexpr layer_scale(float uniform) noexcept // NOLINT(google-explicit-constructor)
            : x(uniform), y(uniform) {
        }
        /// @brief @p sx horizontally, @p sy vertically: a TV-off transition collapses @c y to 0.
        constexpr layer_scale(float sx, float sy) noexcept
            : x(sx), y(sy) {
        }

        friend constexpr bool operator==(const layer_scale&, const layer_scale&) noexcept = default;
    };

    /**
     * @brief How a finished layer is drawn when its scene's layers are combined: its style
     *        (painter.md §2.7, R45).
     *
     * Given at declaration as layer_settings::style, changed through base_scene::style_of(id).
     * It is live: change it at any time, and the compositor reads it when it runs, after
     * `render()`. Animating it frame by frame is how a layer fades, shakes or turns; nothing
     * is redrawn into the layer. A scene itself has no style: its picture is copied as it is,
     * with the blend its flags give. The flat settings place the layer as one quad (R43): flipped,
     * then scaled and rotated about the pivot, then moved by the offset. Mesh effects come with
     * RM4.
     *
     * Positions and sizes are in render units: offset, pivot, destination, and source unless
     * the layer has a size of its own, when source is in its own pixels.
     */
    struct NEUTRINO_EXPORT layer_style {
        /// @brief How the layer combines with what is below; the painter uses the premultiplied
        ///        form of @c blend and @c add (painter.md §2.8).
        blend_mode blend{blend_mode::blend};
        /// @brief 0 (invisible) to 1. Scales colour as well as alpha.
        float      opacity{1.0f};
        /// @brief Multiplies the layer's colour: fade to black, a damage flash, night.
        color      tint{colors::white};
        /// @brief Moves the layer, after scaling and rotation: screen shake.
        point      offset{};
        /// @brief Scales the layer about @ref pivot, per axis. Finite and not negative; 0 hides
        ///        it. Mirroring is @ref flip.
        layer_scale scale{};
        /// @brief Rotates the layer about @ref pivot, clockwise. Finite.
        float      rotation_degrees{0.0f};
        /// @brief The centre of @ref scale and @ref rotation_degrees, from the top-left of the
        ///        rectangle the layer is drawn onto. Unset: that rectangle's centre.
        std::optional <point> pivot;
        /// @brief Mirrors the layer horizontally, vertically or both. A diagonal flip is refused
        ///        (render_model.md R35).
        flip_mode  flip{};
        /// @brief Only this part of the layer. In render units for a layer without a size of
        ///        its own, like everything drawn into it; in its own pixels for a layer with one.
        ///        Unset: all of it.
        std::optional <rect> source;
        /// @brief Where the layer, or its @ref source part, lands: stretched onto this
        ///        rectangle. Unset: in place — a layer that mirrors the canvas lands on the same
        ///        pixels, and a layer with a size of its own is stretched over the render area.
        std::optional <rect> destination;
    };
}
