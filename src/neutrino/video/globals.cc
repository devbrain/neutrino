//
// Created by igor on 03/07/2026.
//
#include <neutrino/video/globals.hh>

#include <SDL3/SDL.h>

#include "services/service_locator.hh"

namespace neutrino {
    sdlpp::renderer& get_renderer() {
        return service_locator::instance().get_renderer();
    }

    sdlpp::window& get_window() {
        return service_locator::instance().get_window();
    }

    dim render_size() {
        auto* r = get_renderer().get();

        // Logical presentation, when active, defines the coordinate space scenes
        // draw in; the drawable's pixel size is only the render space in reflow mode.
        int lw = 0;
        int lh = 0;
        SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
        if (SDL_GetRenderLogicalPresentation(r, &lw, &lh, &mode)
            && mode != SDL_LOGICAL_PRESENTATION_DISABLED) {
            return dim{lw, lh};
        }

        if (const auto out = get_renderer().get_output_size <dim>()) {
            return *out;
        }
        return dim{0, 0};
    }

    sdlpp::point <float> to_render_coords(sdlpp::point <float> window_pt) {
        auto x = window_pt.x;
        auto y = window_pt.y;
        // On failure SDL leaves x/y untouched; falling back to the input is the
        // right no-op for reflow with no DPI scaling (the identity mapping).
        SDL_RenderCoordinatesFromWindow(get_renderer().get(), window_pt.x, window_pt.y, &x, &y);
        return {x, y};
    }

    sdlpp::point <float> to_window_coords(sdlpp::point <float> render_pt) {
        auto x = render_pt.x;
        auto y = render_pt.y;
        SDL_RenderCoordinatesToWindow(get_renderer().get(), render_pt.x, render_pt.y, &x, &y);
        return {x, y};
    }
}
