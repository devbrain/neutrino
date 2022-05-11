//
// Created by igor on 20/09/2020.
//

#ifndef NEUTRINO_GAME_ASSETS_HH
#define NEUTRINO_GAME_ASSETS_HH

#include <vector>
#include <tuple>

#include <neutrino/hal/video/palette.hh>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/math/rect.hh>
#include "rc/resource_loader.hh"
#include "rc/map.hh"


neutrino::hal::palette load_palette(std::istream* is);
neutrino::hal::surface load_backdrop(std::istream* is);
std::tuple<neutrino::hal::surface, std::vector<neutrino::math::rect>> load_tiles(std::istream*is,
                                                                                 const neutrino::hal::palette& pal);
std::vector<neutrino::hal::surface> load_fonts(std::istream* is, const neutrino::hal::palette& pal);


#endif //NEUTRINO_GAME_ASSETS_HH
