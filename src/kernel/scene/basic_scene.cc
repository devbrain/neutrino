//
// Created by igor on 02/06/2022.
//
#include "neutrino/kernel/scene/basic_scene.hh"

namespace neutrino::kernel {
  basic_scene::basic_scene()
  : m_manager(nullptr) {}

  basic_scene::~basic_scene() = default;

  scene_manager& basic_scene::manager() {
    return *m_manager;
  }

  const scene_manager& basic_scene::manager() const {
    return *m_manager;
  }
}