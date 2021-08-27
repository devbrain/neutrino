//
// Created by igor on 08/07/2021.
//

#ifndef NEUTRINO_TILED_LOADER_TMX_HH
#define NEUTRINO_TILED_LOADER_TMX_HH

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <neutrino/tiled/loader/path_resolver.hh>

namespace neutrino::tiled {
  class tile_sheet_manager;

  class world;

  namespace tmx {
    void load (const char *text, std::size_t size, path_resolver_t resolver);
    std::unique_ptr<world> load (std::istream &is, path_resolver_t resolver, tile_sheet_manager &tsm);
    std::unique_ptr<world> load (std::filesystem::path &path, path_resolver_t resolver, tile_sheet_manager &tsm);
  }
}

#endif
