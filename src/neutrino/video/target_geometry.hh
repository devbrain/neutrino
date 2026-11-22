//
// Created by igor on 21/11/2026.
//

#pragma once

#include <sdlpp/video/renderer.hh>

#include <neutrino/video/geometry_types.hh>

namespace neutrino::detail {
    /**
     * @brief What a scene's canvas and layers mirror: the target the scene manager draws into
     *        (render_model.md R42).
     *
     * Its pixel size, and its logical presentation: SDL 3.4 keeps the presentation per
     * target, so a texture given the same size and presentation as the window maps render
     * space onto its pixels exactly as the window does.
     */
    struct target_geometry {
        /// @brief The target's size in pixels.
        dim size{0, 0};
        /// @brief The logical size render space has; zero while the presentation is disabled.
        dim logical{0, 0};
        /// @brief How render space is fitted into @ref size.
        sdlpp::logical_presentation mode{sdlpp::logical_presentation::disabled}; // sdlpp-ok: internal

        /// @brief A target of no size.
        target_geometry() = default;
        /// @brief A target of @p pixels without a logical presentation: render space is its
        ///        pixels. Implicit, so a plain size reads as one.
        target_geometry(dim pixels) noexcept // NOLINT(google-explicit-constructor)
            : size(pixels) {
        }

        friend bool operator==(const target_geometry&, const target_geometry&) = default;
    };

    /// @brief The bound target's geometry: its whole pixel size, bars included, and its
    ///        logical presentation. A target the renderer cannot describe reads as one of no
    ///        size.
    [[nodiscard]] target_geometry bound_target_geometry();

    /// @brief Sets @p g's logical presentation on the bound target: what a canvas or layer
    ///        does at every bind, since a pooled texture keeps its last owner's. A failure is
    ///        counted in draw_failure_count().
    void apply_presentation(const target_geometry& g);
}
