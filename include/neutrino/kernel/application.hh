//
// Created by igor on 07/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_APPLICATION_HH
#define INCLUDE_NEUTRINO_KERNEL_APPLICATION_HH

#include <memory>
#include <string>

#include <neutrino/kernel/events.hh>
#include <neutrino/kernel/system.hh>
#include <neutrino/kernel/systems/base_input_system.hh>
#include <neutrino/kernel/systems/video/video_system.hh>

#include <neutrino/hal/application.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::kernel {
  class application : public hal::application {
    public:
      template <typename ... System>
      application(std::unique_ptr<base_input_system> input_sys,
                  std::unique_ptr<video_system> video_sys,
                  std::unique_ptr<System> ... sys) {
        add_system (std::move(input_sys));
        add_system (std::move(video_sys));
        ((this->add_system (std::move(sys))), ...);
        post_init();
      }

      ~application() override;

      void show (int w, int h);
      void show (int w, int h, hal::window_flags_t flags);
      void show (int w, int h, int x, int y, hal::window_flags_t flags);

      void toggle_full_screen();
      void set_title(const std::string& title);

      void pause(bool v);
      [[nodiscard]] bool paused() const noexcept;
    private:
      void on_terminating () override;
      void on_low_memory () override;
      void on_will_enter_background () override;
      void on_in_background () override;
      void on_in_foreground () override;

      void clear () override;
      void update (std::chrono::milliseconds ms) override;
      void render () override;

      void post_init();
    private:
      static void add_system(std::unique_ptr<base_input_system> input_sys);
      static void add_system(std::unique_ptr<video_system> video_sys);
      [[maybe_unused]] static
      void add_system(std::unique_ptr<system> sys);
    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;
  };

  application* get_application();
}
#endif //INCLUDE_NEUTRINO_KERNEL_APPLICATION_HH
