//
// Created by igor on 06/05/2022.
//

#include "rc/rc.hh"
#include "rc/game_assets.hh"

neutrino::assets::tilesheet load_blocks(resource_loader* loader) {
  auto pal_ios = loader->load (resource_t::PALETTE);
  auto pal = load_palette (pal_ios.get());

  auto tl_ios = loader->load (resource_t::BLOCKS);
  return neutrino::assets::make_tilesheet (load_tiles (tl_ios.get(), pal));
}

std::vector<neutrino::assets::tilesheet> load_fonts(resource_loader* loader) {
  auto pal_ios = loader->load (resource_t::PALETTE);
  auto pal = load_palette (pal_ios.get());

  auto fnt_ios = loader->load (resource_t::FONT);
  auto fonts = load_fonts (fnt_ios.get(), pal);
  std::vector<neutrino::assets::tilesheet> res;
  for (auto& img : fonts) {
    neutrino::assets::tilesheet_info ti(8,8, 0,0, 0,0, 64);
    res.push_back (neutrino::assets::make_tilesheet (std::move(img), ti));
  }
  return res;
}
/*
neutrino::assets::world_assets load_graphics(resource_loader* loader) {

}
 */