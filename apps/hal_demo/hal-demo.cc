//
// Created by igor on 17/04/2022.
//

#include <neutrino/kernel/application.hh>

class app : public neutrino::application {
  public:
    app () : m_renderer (nullptr), x(100), y (100) {

    }

  private:
    [[nodiscard]] neutrino::application_description describe() const noexcept override{
      neutrino::main_window_description d(320, 200);
      return {d, 60};
    }
    /**
     * This method is called before entering the game loop and after video initialization
     */
    void init(neutrino::hal::renderer& renderer) override{
      m_renderer = &renderer;

      input_config().when_pressed (neutrino::scan_code_t::ESCAPE, "EXIT");
    }

    /**
     * This method is called every frame
     * @param ms time passed since last frame
     */
    void update_logic(std::chrono::milliseconds ms) override {
      if (events()["EXIT"]) {
        this->close();
      }
      if (auto m = events()[neutrino::pointer_button_t::LEFT]) {
        x = m->x;
        y = m->y;
      }
    }

    void draw_frame() override {
      m_renderer->active_color({0xFF,0,0,0xFF});
      m_renderer->aa_circle (x, y, 20);
    }

    neutrino::hal::renderer* m_renderer;
    int x;
    int y;
};

int main(int argc, char* argv[]) {
  app a;
  a.execute();
  return 0;
}

