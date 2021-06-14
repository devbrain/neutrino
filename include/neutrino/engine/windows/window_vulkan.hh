//
// Created by igor on 11/06/2021.
//

#ifndef NEUTRINO_ENGINE_WINDOWS_WINDOW_VULKAN_HH
#define NEUTRINO_ENGINE_WINDOWS_WINDOW_VULKAN_HH

#include <neutrino/engine/windows/engine_window.hh>

namespace neutrino::engine {
    class window_vulkan : public engine_window {
    public:
        window_vulkan();
        window_vulkan(window_flags_t flags);
    };
}

#endif
