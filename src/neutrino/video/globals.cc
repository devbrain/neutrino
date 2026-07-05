//
// Created by igor on 03/07/2026.
//
#include <neutrino/video/globals.hh>
#include "services/service_locator.hh"

namespace neutrino {
    sdlpp::renderer& get_renderer() {
        return service_locator::instance().get_renderer();
    }

    sdlpp::window& get_window() {
        return service_locator::instance().get_window();
    }
}
