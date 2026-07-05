//
// Created by igor on 03/07/2026.
//

#pragma once

#include <neutrino/neutrino_export.h>
#include <sdlpp/video/renderer.hh>
#include <sdlpp/video/window.hh>

namespace neutrino {
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::renderer& get_renderer();
    NEUTRINO_EXPORT [[nodiscard]] sdlpp::window& get_window();
}