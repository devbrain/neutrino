//
// Created by igor on 24/07/2021.
//

#include <sstream>
#include <neutrino/kernel/application.hh>
#include <neutrino/kernel/controller.hh>
#include <neutrino/kernel/systems/input_system.hh>
#include <neutrino/kernel/systems/video/video2d/video2d.hh>
#include <neutrino/utils/observer.hh>
#include <neutrino/utils/exception.hh>
#include <neutrino/tiled/loader/tmx.hh>
#include "map/map_city.h"
#include "map/city.h"

namespace neutrino::apogee {

  class controller : public kernel::controller<> {
  };

  using input_def = kernel::input_system<kernel::events::quit, kernel::events::full_screen, kernel::events::pause>;
  class input : public input_def {
    public:
      explicit input(controller& ctrl)
          : input_def (ctrl){
        when_pressed (kernel::events::scan_code_t::ESCAPE, kernel::events::quit{});
        when_pressed (kernel::events::scan_code_t::RETURN, kernel::events::key_mod_t::ALT, kernel::events::full_screen{});
        when_pressed (kernel::events::scan_code_t::SPACE , kernel::events::pause{});
      }
  };

  class world_manager : public kernel::system {
    public:
      world_manager(int w, int h)
      : m_video (nullptr),
        m_width (w),
        m_height (h) {
      }

      void update([[maybe_unused]] std::chrono::milliseconds ms) override {

      }

      void present() override {
        m_video->active_color (hal::color (0xFF, 0xFF, 0xFF));
        m_video->line(0, 0, m_width, m_height);
        //m_video->active_color (hal::color (0x0, 0x0, 0x0));
      }

      void setup() override {
        kernel::system::setup();
        m_video = dynamic_cast<neutrino::kernel::video2d_system*>(m_systems.m_video);
      }
    private:
      neutrino::kernel::video2d_system* m_video;
      int m_width;
      int m_height;
  };
}

int main ([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  using namespace neutrino::tiled;
  std::istringstream is;
  std::istringstream iss (std::string((char*)map_city, map_city_length));

  auto [world, atlas_builder] = tmx::load (iss, [] (const std::string&) { return std::string{}; });

  neutrino::apogee::controller the_controller;
  neutrino::kernel::video2d_system video;
  int w = 640;
  int h = 480;
  int desired_fps = 60;
  neutrino::kernel::application app (std::make_unique<neutrino::apogee::input> (the_controller),
                           std::make_unique<neutrino::kernel::video2d_system>(),
                           std::make_unique<neutrino::apogee::world_manager> (w, h));
  app.show (w, h);
  app.run (desired_fps);

}
