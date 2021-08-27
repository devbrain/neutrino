//
// Created by igor on 19/07/2021.
//

#include <neutrino/tiled/window.hh>

namespace neutrino::tiled {
  window::window (int w, int h)
      : engine::main_window (w, h) {

  }
  // --------------------------------------------------------------------
  window::~window () noexcept {

  }
  // --------------------------------------------------------------------
  void window::after_window_opened () {

  }
}