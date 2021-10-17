//
// Created by igor on 07/10/2021.
//

#include <neutrino/kernel/system.hh>

namespace neutrino::kernel {
  system::~system () = default;

  void system::on_paused ([[maybe_unused]] bool paused) {
  }
}
