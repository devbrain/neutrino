//
// Created by igor on 02/05/2022.
//

#include <iostream>
#include <neutrino/assets/image/image_io.hh>

int main(int argc, char* argv[]) {
  //neutrino::assets::load_fon ("/home/igor/proj/ares/fon/120TMS18.FON");
  try {
    auto s = neutrino::assets::load_image ("/home/igor/tmp/1/crater.ham");
    neutrino::assets::save ("/home/igor/tmp/1/crater.png", s);
  } catch (std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
  }
  return 0;
}

