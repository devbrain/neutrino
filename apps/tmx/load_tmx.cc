//
// Created by igor on 24/07/2021.
//

#include <sstream>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <neutrino/kernel/application.hh>
#include <neutrino/kernel/rc/world/world.hh>
#include <neutrino/kernel/gfx/world_renderer.hh>

#include "map/map_city.h"
#include "map/city.h"
#include "map/demo.h"
#include "map/tileset.h"
#include "map/formosa.h"
#include "map/overworld.h"
#include "map/moon_overlay.h"
#include "map/tiles.h"

// /home/igor/proj/ares/test/test.tmx

class app : public neutrino::application {
    static constexpr auto EV_EXIT = "exit";
    static constexpr auto EV_FULLSCREEN = "fullscreen";
    static constexpr auto EV_LEFT = "left";
    static constexpr auto EV_RIGHT = "right";
    static constexpr auto EV_UP = "up";
    static constexpr auto EV_DOWN = "down";

  public:
    explicit app (const std::string& path) : m_renderer (nullptr) {
      using namespace neutrino::kernel;
      if (path.empty()) {
        std::istringstream is;
        std::istringstream iss (std::string((char*)formosa, formosa_length));

        m_world = world::from_tmx (iss, [] (const std::string& name) {
          if (name == "city.png") {
            return std::string{(const char*) city, city_length};
          }
          else if (name == "overworld.png") {
            return std::string{(const char*) overworld, overworld_length};
          }
          else if (name == "tiles.png") {
            return std::string{(const char*) tiles, tiles_length};
          }
          else if (name == "moon_overlay.png") {
            return std::string{(const char*) moon_overlay, moon_overlay_length};
          }
          else if (name == "tileset.png") {
            return std::string{(const char*) tileset, tileset_length};
          }
          ENFORCE(false);
        }, m_atlas);
      } else {
        std::filesystem::path p = path;
        auto dir = p.parent_path();
        std::ifstream ifs(p, std::ios::binary);
        m_world = world::from_tmx (ifs, [dir](const std::string& name) -> std::string {
          std::ifstream t(dir/name);
          return std::string ((std::istreambuf_iterator<char>(t)),
                          std::istreambuf_iterator<char>());

        }, m_atlas);
      }
    }

  private:
    [[nodiscard]] neutrino::application_description describe() const noexcept override{
      neutrino::main_window_description d(800, 800);

      d.resizable (true);
      return {d, 60};
    }

    void init(neutrino::hal::renderer& renderer) override{
      m_renderer = &renderer;

      input_config().when_pressed (neutrino::key_mod_t::ALT, neutrino::scan_code_t::RETURN, EV_FULLSCREEN);
      input_config().when_pressed (neutrino::scan_code_t::ESCAPE, EV_EXIT);
      input_config().when_pressed (neutrino::scan_code_t::LEFT, EV_LEFT);
      input_config().when_pressed (neutrino::scan_code_t::RIGHT, EV_RIGHT);
      input_config().when_pressed (neutrino::scan_code_t::UP, EV_UP);
      input_config().when_pressed (neutrino::scan_code_t::DOWN, EV_DOWN);

      m_atlas.textures.convert_images (renderer);

      auto [wpx, wpy] = m_world.dimensions_in_pixels();

      m_world_renderer[0].set (&m_world, &m_atlas);
      m_window[0].dimensions ({(int)wpx,(int)wpy});
      m_window[0].screen_pos({10,10});
      m_window[0].world_pos({0,0});


      m_world_renderer[1].set (&m_world, &m_atlas);
      m_window[1].dimensions ({(int)wpx/2,(int)wpy/2});
      m_window[1].screen_pos({400,400});
      m_window[1].world_pos({0,0});
    }

    void update_logic(std::chrono::milliseconds ms) override {
      if (events()[EV_EXIT]) {
        this->close();
      }
      if (events()[EV_FULLSCREEN]) {
        this->toggle_fullscreen();
      }
      if (events()[EV_RIGHT]) {
        m_window[1].add_world_pos (1, 0);
      }
      if (events()[EV_LEFT]) {
        m_window[1].add_world_pos (-1, 0);
      }
      if (events()[EV_UP]) {
        m_window[1].add_world_pos (0, -1);
      }
      if (events()[EV_DOWN]) {
        m_window[1].add_world_pos (0, 1);
     //   std::cout << m_window[1].world_pos ().y << std::endl;
      }
      for (auto & i : m_world_renderer) {
        i.update (ms);
      }


    }

    void draw_frame() override {
      for (int i=0; i<2; i++) {
        m_world_renderer[i].draw (m_window[i], *m_renderer);
      }
      neutrino::math::rect w(m_window[0].screen_pos()+m_window[1].world_pos(), m_window[1].dimensions());
      m_renderer->active_color({0xFF, 0x0, 0x0, 0xFF});
      m_renderer->rectangle (w);

      neutrino::math::rect w2(m_window[1].screen_pos(), m_window[1].dimensions());
      m_renderer->active_color({0xFF, 0x0FF, 0xFF, 0xFF});
      m_renderer->rectangle (w2);
    }

    neutrino::hal::renderer* m_renderer;
    neutrino::kernel::gfx_assets m_atlas;
    neutrino::kernel::world  m_world;

    neutrino::kernel::world_renderer m_world_renderer[2];
    neutrino::kernel::world_window m_window[2];


};

int main ([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  std::string path = "";
  if (argc > 1) {
    path = argv[1];
  }
  app a (path);
  a.execute();

  return 0;

}
