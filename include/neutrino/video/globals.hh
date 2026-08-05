//
// Created by igor on 03/07/2026.
//

#pragma once

#include <concepts>

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

    // The same mapping in the strong spaces of <neutrino/world_space.hh>. The two spaces differ by
    // the HiDPI scale and the letterbox offset, so passing one where the other belongs is silently
    // wrong on exactly the setups (scaled displays, non-matching aspect ratios) least likely to be
    // the developer's own. Typed, that swap is a compile error and the direction of the mapping is
    // read off the signature.
    //
    // Constrained TEMPLATES, not plain overloads, for the same reason as world::set_velocity: a
    // plain overload differing only in a two-float parameter makes the long-legal
    // `to_render_coords({x, y})` ambiguous, since the braced list initializes point<float> and
    // window_pos equally well. A braced-init-list is a non-deduced context, so a template is not a
    // candidate for it and `{x, y}` keeps selecting the untyped overload exactly as before.

    /// @brief Map a window position into render space. @see to_render_coords(sdlpp::point<float>)
    template<std::same_as <window_pos> P>
    [[nodiscard]] inline render_pos to_render_coords(P p) {
        const auto r = to_render_coords(sdlpp::point <float>{p.x, p.y});
        return render_pos{r.x, r.y};
    }

    /// @brief Map a render position back into window space. @see to_window_coords(sdlpp::point<float>)
    template<std::same_as <render_pos> P>
    [[nodiscard]] inline window_pos to_window_coords(P p) {
        const auto w = to_window_coords(sdlpp::point <float>{p.x, p.y});
        return window_pos{w.x, w.y};
    }
}
