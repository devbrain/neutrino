//
// Created by igor on 24/07/2021.
//

#include <sstream>
#include <neutrino/tiled/loader/tmx.hh>
#include "map/map_city.h"
#include "map/city.h"

int main (int argc, char* argv[]) {
  using namespace neutrino::tiled;
  std::istringstream is;
  std::istringstream iss (std::string((char*)map_city, map_city_length));

  tmx::load (iss, [] (const std::string&) { return std::string{}; });
}
