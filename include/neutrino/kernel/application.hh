//
// Created by igor on 07/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_APPLICATION_HH
#define INCLUDE_NEUTRINO_KERNEL_APPLICATION_HH

#include <memory>
#include <neutrino/hal/application.hh>
#include <neutrino/kernel/application_description.hh>
#include <neutrino/kernel/hdi/events.hh>
#include <neutrino/kernel/hdi/input_config.hh>
#include <neutrino/kernel/hdi/pointer_config.hh>
#include <neutrino/kernel/ecs/registry.hh>
#include <neutrino/kernel/system/context.hh>

namespace neutrino {
  class main_window;
  namespace kernel {
    class scene_manager;
  }
  class application : public hal::application {
      friend class main_window;
      friend class kernel::scene_manager;
    public:
      application();
      ~application() override;

      void execute();

    protected:
      input_config_base& input_config();
      pointer_config_base& mouse_config();
      events_holder& events();
      kernel::scene_manager& manager();

      [[nodiscard]] ecs::id_t create_entity() const;
      [[nodiscard]] ecs::registry& registry();
      [[nodiscard]] const ecs::registry& registry() const;

    protected:
      [[nodiscard]] virtual application_description describe() const noexcept = 0;
      /**
       * This method is called before entering the game loop and after video initialization
       */
      virtual void init(kernel::context& ctx) = 0;

      /**
       * This method is called every frame
       * @param ms time passed since last frame
       */
      virtual void update_logic(std::chrono::milliseconds ms);

      virtual void draw_frame();

      /**
       * This method is called before application is exiting and while video is still active
       */
      virtual void on_exit();

    protected:
      // Event handlers
      void on_terminating () override;
      void on_low_memory () override;
      void on_will_enter_background () override;
      void on_in_background () override;
      void on_in_foreground () override;

      virtual void on_window_resized (unsigned new_w, unsigned new_h);
      /**
       * This method is called when entering to the paused state
       */
      virtual void on_paused();

      /**
       * This method is called when exiting from the paused state
       */
      virtual void on_resumed();
    public:
      // Commands
      /**
       * Closes application
       */
      void close();

      /**
       * Toggle fullscreen
       */
      void toggle_fullscreen();

      /**
       * Pause game loop
       */
      void set_paused(bool v);
      [[nodiscard]] bool is_paused() const noexcept;
    private:
      void update (std::chrono::milliseconds ms) override;
      void on_keyboard_input (const hal::events::keyboard& ev);
      void on_pointer_input (const hal::events::pointer& ev);
      void do_draw_frame();
    private:
      struct impl;
      std::unique_ptr<impl> m_pimpl;
  };
}
#endif //INCLUDE_NEUTRINO_KERNEL_APPLICATION_HH
