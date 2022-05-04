//
// Created by igor on 03/05/2022.
//
#include <memory>
#include <iostream>
#include "rc/bi_loader.hh"
#include "rc/game_assets.hh"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    std::unique_ptr<resource_loader> loader = std::make_unique<bi_loader> ();
    auto pic1_ios = loader->load (resource_t::PIC1);
    auto s = load_backdrop (pic1_ios.get ());
    s.save ("pic1.png");

    auto pal_ios = loader->load (resource_t::PALETTE);
    auto pal = load_palette (pal_ios.get());

    auto tl_ios = loader->load (resource_t::BLOCKS);
    auto [tls, rects] = load_tiles (tl_ios.get(), pal);
    tls.save ("blocks.png");

  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}