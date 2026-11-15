//
// Created by igor on 03/07/2026.
//
#include <neutrino/video/globals.hh>

#include "services/service_locator.hh"

namespace neutrino {
    namespace {
        pixel_snap g_pixel_snap = pixel_snap::off;
    }

    void set_pixel_snap(pixel_snap mode) noexcept {
        g_pixel_snap = mode;
    }

    pixel_snap get_pixel_snap() noexcept {
        return g_pixel_snap;
    }

    sdlpp::renderer& get_renderer() {
        return service_locator::instance().get_renderer();
    }

    sdlpp::window& get_window() {
        return service_locator::instance().get_window();
    }

    dim render_size() {
        // Logical presentation, when active, defines the coordinate space scenes
        // draw in; the drawable's pixel size is only the render space in reflow mode.
        if (const auto lp = get_renderer().get_logical_presentation();
            lp && lp->mode != sdlpp::logical_presentation::disabled) {
            return dim{lp->width, lp->height};
        }

        if (const auto out = get_renderer().get_output_size <dim>()) {
            return *out;
        }
        return dim{0, 0};
    }

    sdlpp::point <float> to_render_coords(sdlpp::point <float> window_pt) {
        // On failure fall back to the input: the right no-op for reflow with no
        // DPI scaling (the identity mapping).
        return get_renderer().coordinates_from_window(window_pt).value_or(window_pt);
    }

    sdlpp::point <float> to_window_coords(sdlpp::point <float> render_pt) {
        return get_renderer().coordinates_to_window(render_pt).value_or(render_pt);
    }
}
