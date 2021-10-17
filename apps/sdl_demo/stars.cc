//
// Created by igor on 17/10/2021.
//

#include "demoscene.hh"

struct star {
  float x;
  float y;
  float z;
};

class star_field : public neutrino::kernel::system {
    static constexpr float MAX_DEPTH = 256.0;
  public:
    explicit star_field (neutrino::kernel::vga256& screen)
        : m_screen (screen),
          m_center_x (m_screen.width () / 2),
          m_center_y (m_screen.height () / 2) {
      m_screen.palette ()[1] = neutrino::hal::color{0xff, 0xff, 0xff};
      for (int i = 0; i < 100; i++) {
        m_stars.push_back (create_star ());
      }
    }

  private:
    void update (std::chrono::milliseconds ms) override {
      float speed = 0.1;
      for (auto& s: m_stars) {
        auto count = ms.count ();
        float z = s.z - speed * count;
        if (z <= 0) {
          s = create_star ();
          continue;
        }
        s.z = z;
        int x = m_center_x + (int) (s.x * MAX_DEPTH / z);
        int y = m_center_y + (int) (s.y * MAX_DEPTH / z);
        if (x < 0 || x >= m_screen.width () || y < 0 || y >= m_screen.height ()) {
          s = create_star ();
          continue;
        }
      }
    };

    void present () override {
      m_screen.cls ();
      for (const auto& s: m_stars) {
        int x = m_center_x + (int) (s.x * MAX_DEPTH / s.z);
        int y = m_center_y + (int) (s.y * MAX_DEPTH / s.z);
        m_screen.surface ()[m_screen.width () * y + x] = 1;
      }
    };

    star create_star () {
      float x = -m_center_x + rand () % m_screen.width ();
      float y = -m_center_y + rand () % m_screen.height ();
      return {x, y, MAX_DEPTH};
    }

  private:
    neutrino::kernel::vga256& m_screen;
    int m_center_x;
    int m_center_y;
    std::vector<star> m_stars;
};

int main ([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  neutrino::kernel::demo_effect::run<star_field> (320, 200);
}