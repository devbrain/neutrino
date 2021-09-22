//
// Created by igor on 08/07/2021.
//

#ifndef NEUTRINO_TILED_LOADER_TMX_HH
#define NEUTRINO_TILED_LOADER_TMX_HH

#include <filesystem>
#include <iosfwd>
#include <tuple>


#include <neutrino/tiled/loader/path_resolver.hh>
#include <neutrino/tiled/world/world.hh>
#include <neutrino/tiled/world/builder/texture_atlas_builder.hh>

namespace neutrino::tiled::tmx {
  std::tuple<world, texture_atlas_builder> load (const char* text, std::size_t size, path_resolver_t resolver);
  std::tuple<world, texture_atlas_builder> load (std::istream& is, path_resolver_t resolver);
  std::tuple<world, texture_atlas_builder> load (std::filesystem::path& path, path_resolver_t resolver);
}

#endif
