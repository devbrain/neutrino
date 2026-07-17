//
// Created by igor on 03/07/2026.
//

#pragma once

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <sdlpp/utility/geometry.hh>
#include <sdlpp/video/renderer.hh>
#include <sdlpp/video/window.hh>

namespace neutrino {
    /// @brief The active application's renderer, the target every draw call issues to.
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::renderer& get_renderer();
    /// @brief The active application's window (the renderer's presentation surface).
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::window& get_window();

    /// @brief The size of the coordinate space scenes draw in: the logical size
    /// when logical presentation is active, otherwise the drawable's pixel size.
    /// This is what a scene's viewport and camera should use. Derived from the
    /// renderer state every call — never cached, so it cannot drift.
    NEUTRINO_EXPORT [[nodiscard]] dim render_size();

    /// @brief Map a window point (as delivered in mouse/touch events) into render
    /// coordinates. One call accounts for BOTH the HiDPI scale AND logical-
    /// presentation letterboxing, so all input coordinates must go through it.
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::point <float> to_render_coords(sdlpp::point <float> window_pt);

    /// @brief Inverse of to_render_coords(): map a render point back to a window point.
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::point <float> to_window_coords(sdlpp::point <float> render_pt);
}
