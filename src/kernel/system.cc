//
// Created by igor on 07/10/2021.
//

#include <neutrino/kernel/system.hh>
#include "kernel/systems_manager.hh"

namespace neutrino::kernel {

  system::systems::systems ()
  : m_video(nullptr)
  {
  }

  system::system() = default;
  system::~system () = default;

  void system::on_paused ([[maybe_unused]] bool paused) {
  }

  void system::setup () {
    m_systems.m_video = get_systems_manager()->get_video_system();
  }
}
