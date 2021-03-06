//
// Created by igor on 24/06/2021.
//

#include <cmath>
#include "demoscene.hh"



using namespace neutrino;

class plasma : public kernel::system {
  public:
    explicit plasma (kernel::vga256& screen)
    : m_screen(screen){
      /*create sin lookup table */
      for (int i = 0; i < 512; i++) {
        auto rad = ((float) i * 0.703125)
                   * 0.0174532; /* 360 / 512 * degree to rad, 360 degrees spread over 512 values to be able to use AND 512-1 instead of using modulo 360*/
        aSin[i] = sin (rad) * 1024; /*using fixed point math with 1024 as base*/
      }
      init();
    }
  private:
    void update ([[maybe_unused]] std::chrono::milliseconds ms) override {}

    void present () override {
      uint16_t i, j;
      uint8_t index;
      int x;
      uint8_t* image = m_screen.surface ().data ();

      tpos4 = pos4;
      tpos3 = pos3;

      for (i = 0; i < m_screen.height(); ++i) {
        tpos1 = pos1 + 5;
        tpos2 = pos2 + 3;

        tpos3 &= 511;
        tpos4 &= 511;

        for (j = 0; j < m_screen.width(); ++j) {
          tpos1 &= 511;
          tpos2 &= 511;

          x = aSin[tpos1] + aSin[tpos2] + aSin[tpos3] + aSin[tpos4]; /*actual plasma calculation*/

          index = 128 + (x
              >> 4); /*fixed point multiplication but optimized so basically it says (x * (64 * 1024) / (1024 * 1024)), x is already multiplied by 1024*/

          *image++ = index;

          tpos1 += 5;
          tpos2 += 3;
        }

        tpos4 += 3;
        tpos3 += 1;
      }

      /* move plasma */

      pos1 += 9;
      pos3 += 8;
    }

    void init () {
      auto& colors = m_screen.palette ();
      /* create palette */
      for (int i = 0; i < 64; ++i) {
        colors[i].r = i << 2;
        colors[i].g = 255 - ((i << 2) + 1);
        colors[i + 64].r = 255;
        colors[i + 64].g = (i << 2) + 1;
        colors[i + 128].r = 255 - ((i << 2) + 1);
        colors[i + 128].g = 255 - ((i << 2) + 1);
        colors[i + 192].g = (i << 2) + 1;
      }
    }

  private:
    uint16_t pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0, tpos1, tpos2, tpos3, tpos4;
    int aSin[512];
    kernel::vga256& m_screen;
};

int main ([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  kernel::demo_effect::run<plasma>(480, 360);
  return 0;
}