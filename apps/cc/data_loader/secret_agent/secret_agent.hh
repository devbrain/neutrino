//
// Created by igor on 7/12/24.
//

#ifndef  SECRET_AGENT_HH
#define  SECRET_AGENT_HH

#include <filesystem>
#include <assets/assets.hh>
#include "raw_map.hh"

std::vector<raw_map> extract_sa_maps(const std::filesystem::path& file);
neutrino::assets::tileset extract_sa_tileset(const std::filesystem::path& file);
neutrino::assets::tileset extract_sa_mini_tileset(const std::filesystem::path& file);

#endif
