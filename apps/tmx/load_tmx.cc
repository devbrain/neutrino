//
// Created by igor on 24/07/2021.
//

#include <sstream>
#include <neutrino/engine/application.hh>
#include <neutrino/tiled/loader/tmx.hh>
#include "map/map_city.h"
#include "map/city.h"

int main ([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  neutrino::engine::application app (nullptr);

  using namespace neutrino::tiled;
  std::istringstream is;
  std::istringstream iss (std::string((char*)map_city, map_city_length));

  auto [world, atlas_builder] = tmx::load (iss, [] (const std::string&) { return std::string{}; });
}
