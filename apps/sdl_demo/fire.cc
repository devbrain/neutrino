//
// Created by igor on 06/06/2021.
//

#include <neutrino/demoscene/demoscene.hh>
#include <neutrino/engine/application.hh>
#include <vector>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 360

using namespace neutrino;

struct fire_app : public demoscene::scene {
  public:
    fire_app ()
        : demoscene::scene (SCREEN_WIDTH, SCREEN_HEIGHT),
          fire (SCREEN_WIDTH * SCREEN_HEIGHT) {

    }

    void effect (demoscene::vga& vga) override {

      int j = SCREEN_WIDTH * (SCREEN_HEIGHT - 1);
      for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
        int random = 1 + (int) (16.0 * (rand () / (RAND_MAX + 1.0)));
        if (random
            > 9) { /* the lower the value, the intenser the fire, compensate a lower value with a higher decay value*/
          fire[j + i] = 255; /*maximum heat*/
        }
        else {
          fire[j + i] = 0;
        }
      }

      /* move fire upwards, start at bottom*/
      int temp;
      for (int index = 0; index < 60; ++index) {
        for (int i = 0; i < SCREEN_WIDTH - 1; ++i) {
          if (i == 0) /* at the left border*/
          {
            temp = fire[j];
            temp += fire[j + 1];
            temp += fire[j - SCREEN_WIDTH];
            temp /= 3;
          }
          else {
            if (i == SCREEN_WIDTH - 1) /* at the right border*/
            {
              temp = fire[j + i];
              temp += fire[j - SCREEN_WIDTH + i];
              temp += fire[j + i - 1];
              temp /= 3;
            }
            else {
              temp = fire[j + i];
              temp += fire[j + i + 1];
              temp += fire[j + i - 1];
              temp += fire[j - SCREEN_WIDTH + i];
              temp >>= 2;
            }
          }
          if (temp > 1) {
            temp -= 1;
          } /* decay */

          fire[j - SCREEN_WIDTH + i] = temp;
        }
        j -= SCREEN_WIDTH;
      }

      uint8_t* image =
          vga.surface ().data () + (SCREEN_WIDTH * SCREEN_HEIGHT) - 1;  /*start in the right bottom corner*/

      /* draw fire array to screen from bottom to top + 300*/

      for (int i = SCREEN_HEIGHT - 3; i >= 300; --i) {
        for (j = SCREEN_WIDTH - 1; j >= 0; --j) {
          *image = fire[i * SCREEN_WIDTH + j];
          image--;
        }
      }
    }

    void init (demoscene::vga& vga) override {
      auto& colors = vga.palette ();
      /* create a suitable fire palette, this is crucial for a good effect */
      /* black to blue, blue to red, red to yellow, yellow to white*/

      for (int i = 0; i < 32; ++i) {
        /* black to blue, 32 values*/
        colors[i].b = i << 1;

        /* blue to red, 32 values*/
        colors[i + 32].r = i << 3;
        colors[i + 32].b = 64 - (i << 1);

        /*red to yellow, 32 values*/
        colors[i + 64].r = 255;
        colors[i + 64].g = i << 3;

        /* yellow to white, 162 */
        colors[i + 96].r = 255;
        colors[i + 96].g = 255;
        colors[i + 96].b = i << 2;
        colors[i + 128].r = 255;
        colors[i + 128].g = 255;
        colors[i + 128].b = 64 + (i << 2);
        colors[i + 160].r = 255;
        colors[i + 160].g = 255;
        colors[i + 160].b = 128 + (i << 2);
        colors[i + 192].r = 255;
        colors[i + 192].g = 255;
        colors[i + 192].b = 192 + i;
        colors[i + 224].r = 255;
        colors[i + 224].g = 255;
        colors[i + 224].b = 224 + i;
      }
    }

  private:
    std::vector<uint8_t> fire;
};

int main ([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {

  neutrino::engine::application app (nullptr);

  fire_app window;
  window.show ();
  app.run (30);

  return 0;
}