//
// Created by igor on 17/04/2022.
//

#include <neutrino/kernel/application.hh>

#include <neutrino/utils/io/memory_stream_buf.hh>
#include "assets/foo.h"
#include "assets/faces.h"

class app : public neutrino::application {
    static constexpr auto EV_EXIT = "exit";
    static constexpr auto EV_FULLSCREEN = "fullscreen";
  public:
    app () : m_renderer (nullptr), frame_num (0), frame_duration(0) {
    }

  private:
    [[nodiscard]] neutrino::application_description describe() const noexcept override{
      neutrino::main_window_description d(640, 480);
      d.resizable (true);
      return {d, 60};
    }
    /**
     * This method is called before entering the game loop and after video initialization
     */

    void init(neutrino::hal::renderer& renderer) override{
      m_renderer = &renderer;

      input_config().when_pressed (neutrino::key_mod_t::ALT, neutrino::scan_code_t::RETURN, EV_FULLSCREEN);
      input_config().when_pressed (neutrino::scan_code_t::ESCAPE, EV_EXIT);


      // Load resources
      neutrino::utils::io::memory_input_stream is((const char*)faces, sizeof (faces));
      neutrino::gfx::tile_sheet_info ti(64, 64, 0, 0, 0, 0, 4);

      neutrino::hal::color keycolor(0xFF, 0x00, 0xFF);

      neutrino::gfx::grid_image src(
            is,
            ti
          );
      clips = src.positions();
      texture = src.create_texture (renderer, keycolor);
    }

    /**
     * This method is called every frame
     * @param ms time passed since last frame
     */
    void update_logic(std::chrono::milliseconds ms) override {
      if (events()[EV_EXIT]) {
        this->close();
      }
      if (events()[EV_FULLSCREEN]) {
        this->toggle_fullscreen();
      }

      frame_duration += ms;
      if (frame_duration > std::chrono::milliseconds(100)) {
        frame_num++;
        frame_duration = std::chrono::milliseconds(0);
      }
      if (frame_num >= clips.size()) {
        frame_num = 0;
      }
    }

    void draw_frame() override {
      m_renderer->active_color({0xFF,0,0,0xFF});
      auto src = clips[frame_num];
      auto w = m_renderer->logical_size();

      m_renderer->line (0,0, w[0], w[1]);
      m_renderer->line (w[0],0, 0, w[1]);

      auto half = (w - src.dims)/2;
      neutrino::math::rect dst(half[0], half[1], src.dims[0], src.dims[1]);
      m_renderer->copy (texture, src, dst);
    }

    neutrino::hal::renderer* m_renderer;

    std::vector<neutrino::math::rect> clips;
    neutrino::hal::texture texture;
    unsigned int frame_num;
    std::chrono::milliseconds frame_duration;
};

int main(int argc, char* argv[]) {
  app a;
  a.execute();
  return 0;
}

