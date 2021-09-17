//
// Created by igor on 26/07/2021.
//

#ifndef NEUTRINO_PATH_RESOLVER_HH
#define NEUTRINO_PATH_RESOLVER_HH

#include <functional>
#include <string>

namespace neutrino::tiled {
  // path resolver accepts path in tmx file and returns content of the file pointed by path
  using path_resolver_t = std::function<std::string (const std::string& path)>;
}

#endif
