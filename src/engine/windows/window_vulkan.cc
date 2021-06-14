//
// Created by igor on 12/06/2021.
//

#include <neutrino/engine/windows/window_vulkan.hh>

namespace neutrino::engine {
    window_vulkan::window_vulkan() = default;


    window_vulkan::window_vulkan(window_flags_t flags)
            : engine_window(basic_window::VULKAN, flags)
    {

    }
}
