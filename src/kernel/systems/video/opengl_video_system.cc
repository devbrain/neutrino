//
// Created by igor on 18/10/2021.
//

#include <neutrino/kernel/systems/video/opengl_video_system.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {
  void opengl_video_system::init(hal::window& w) {
    if (!dynamic_cast<hal::window_opengl*>(&w)) {
      RAISE_EX("OpenGL video system should be initialized with OpenGL window");
    }
  }

  void opengl_video_system::clear() {

  }

  void opengl_video_system::present() {

  }
}
