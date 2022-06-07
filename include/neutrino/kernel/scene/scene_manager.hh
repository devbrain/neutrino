//
// Created by igor on 02/06/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SCENE_SCENE_MANAGER_HH
#define INCLUDE_NEUTRINO_KERNEL_SCENE_SCENE_MANAGER_HH

#include <chrono>
#include <memory>
#include <neutrino/kernel/system/context.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::hal {
  class renderer;
}

namespace neutrino {
  class application;

  namespace kernel {
    class basic_scene;
    class scene_manager {
        friend class neutrino::application;
      private:
        struct pass_token {};
      public:
        scene_manager(context& ctx, neutrino::application* app, pass_token tk);
        ~scene_manager();

        void push(std::unique_ptr<basic_scene> sc);
        void pop();
        void replace(std::unique_ptr<basic_scene> sc);
      private:
        void update_logic (std::chrono::milliseconds ms);
        void draw_frame ();
      private:
        struct impl;
        spimpl::unique_impl_ptr<impl> m_pimpl;
    };
  }
}

#endif //INCLUDE_NEUTRINO_KERNEL_SCENE_SCENE_MANAGER_HH
