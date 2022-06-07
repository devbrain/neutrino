//
// Created by igor on 06/06/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SCENE_SCENE_HH
#define INCLUDE_NEUTRINO_KERNEL_SCENE_SCENE_HH

#include <memory_resource>
#include <neutrino/kernel/scene/basic_scene.hh>
#include <neutrino/kernel/ecs/registry.hh>

namespace neutrino::kernel {
  class scene : public basic_scene {
    private:
      scene();
      explicit scene(std::pmr::memory_resource* mr);

    protected:
      [[nodiscard]] ecs::id_t create_entity() const;
      [[nodiscard]] ecs::registry& registry();
      [[nodiscard]] const ecs::registry& registry() const;
    private:

  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SCENE_SCENE_HH
