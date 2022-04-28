//
// Created by igor on 17/04/2022.
//

#include <neutrino/kernel/application.hh>
#include <neutrino/kernel/gfx/grid_image.hh>
#include <neutrino/utils/io/memory_stream_buf.hh>
#include "assets/foo.h"

class app : public neutrino::application {
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

      input_config().when_pressed (neutrino::scan_code_t::ESCAPE, "EXIT");
      input_config().when_pressed (neutrino::key_mod_t::LALT, neutrino::scan_code_t::F, "FULLSCREEN");

      // Load resources
      neutrino::utils::io::memory_input_stream is((const char*)foo, sizeof (foo));
      neutrino::gfx::tile_sheet_info ti(64, 255, 0, 0, 0,0, 4);
      neutrino::gfx::grid_image src(
            is,
            ti
          );
      clips = src.positions();
      texture = src.create_texture (renderer);
    }

    /**
     * This method is called every frame
     * @param ms time passed since last frame
     */
    void update_logic(std::chrono::milliseconds ms) override {
      if (events()["EXIT"]) {
        this->close();
      }
      if (events()["FULLSCREEN"]) {
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

