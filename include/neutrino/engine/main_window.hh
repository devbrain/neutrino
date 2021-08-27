//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_ENGINE_MAIN_WINDOW_HH
#define NEUTRINO_ENGINE_MAIN_WINDOW_HH

#include <neutrino/engine/scene_window.hh>

namespace neutrino::engine {
  using main_window = scene_window<hal::window_2d>;
#if defined(NEUTRINO_HAS_OPENGL)
  using main_window_opengl = scene_window<hal::window_opengl>;
#endif
#if defined(NEUTRINO_HAS_VULKAN)
  using main_window_vulkan = scene_window<hal::window_vulkan>;
#endif
}

#endif
