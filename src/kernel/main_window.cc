//
// Created by igor on 18/10/2021.
//

#include "main_window.hh"
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {
  std::unique_ptr<hal::window> window_factory(std::optional<hal::window_flags_t> flags, video_system* sys) {
    if (dynamic_cast<accel_renderer_video_system*>(sys)) {
      if (flags) {
        return std::make_unique<main_window<hal::window_2d>>(*flags);
      } else {
        return std::make_unique<main_window<hal::window_2d>>();
      }
    } else if (dynamic_cast<opengl_video_system*>(sys)) {
      if (flags) {
        return std::make_unique<main_window<hal::window_opengl>>(*flags);
      } else {
        return std::make_unique<main_window<hal::window_opengl>>();
      }
    } else {
      RAISE_EX("Unkown video system");
    }
  }

  void set_up_system(hal::window* win, video_system* sys) {
    if (dynamic_cast<accel_renderer_video_system*>(sys)) {
      if (auto* w = dynamic_cast<main_window<hal::window_2d>*>(win); w) {
        w->set_up (sys);
      } else {
        RAISE_EX("window_2d is expected here");
      }
    } else if (dynamic_cast<opengl_video_system*>(sys)) {
      if (auto* w = dynamic_cast<main_window<hal::window_opengl>*>(win); w) {
        w->set_up (sys);
      } else {
        RAISE_EX("window_opengl is expected here");
      }
    } else {
      RAISE_EX("Unkown video system");
    }
  }
}