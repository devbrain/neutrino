//
// Created by igor on 03/05/2022.
//
#include <memory>
#include <iostream>
#include "rc/bi_loader.hh"
#include "rc/game_assets.hh"
#include "../../src/assets/image/image_loader.hh"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    std::unique_ptr<resource_loader> loader = std::make_unique<bi_loader> ();
    auto pic1_ios = loader->load (resource_t::PIC1);
    auto s = load_backdrop (pic1_ios.get ());
    s.save ("pic1.png");



  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}