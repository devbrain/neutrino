//
// Created by igor on 02/05/2022.
//

#include <iostream>
#include <neutrino/assets/image/bitmask.hh>
#include <neutrino/hal/video/fonts/rom_fonts.hh>

int main(int argc, char* argv[]) {
  /*
  //neutrino::assets::load_fon ("/home/igor/proj/ares/fon/120TMS18.FON");
  try {
    auto s = neutrino::assets::load_image ("/home/igor/tmp/1/crater.ham");
    neutrino::assets::save ("/home/igor/tmp/1/crater.png", s);
  } catch (std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
  }
   */
  auto v = neutrino::hal::get_glyph (neutrino::hal::font::IBM_8x16, 'A');
  neutrino::assets::bitmask bm(8, 16);
  for (int y=0; y<16; y++) {
    for (int x=0; x<8; x++) {
      if (v[y][x]) {
        bm.set (x, y);
      }
    }
  }

  for (int y=0; y<16; y++) {
    for (int x = 0; x < 8; x++) {
      if (bm.get (x, y)) {
        std::cout << "+";
      } else {
        std::cout << " ";
      }
    }
    std::cout << std::endl;
  }
  return 0;
}

