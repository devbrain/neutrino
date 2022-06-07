//
// Created by igor on 02/06/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SCENE_BASIC_SCENE_HH
#define INCLUDE_NEUTRINO_KERNEL_SCENE_BASIC_SCENE_HH

#include <chrono>
#include <neutrino/kernel/hdi/events.hh>
#include <neutrino/kernel/system/context.hh>

namespace neutrino::kernel {
  class scene_manager;

  class basic_scene {
      friend class scene_manager;
    public:
      basic_scene();
      virtual ~basic_scene();

      virtual void handle_input(events_holder& events, context& ctx) = 0;
      virtual void update_logic (std::chrono::milliseconds ms, context& ctx) = 0;
      virtual void present (context& ctx) = 0;
    protected:
      [[nodiscard]] scene_manager& manager();
      [[nodiscard]] const scene_manager& manager() const;
    private:
      scene_manager* m_manager;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SCENE_BASIC_SCENE_HH
