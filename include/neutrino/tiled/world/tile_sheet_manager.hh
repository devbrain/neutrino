//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_TILE_SHEET_MANAGER_HH
#define NEUTRINO_TILED_WORLD_TILE_SHEET_MANAGER_HH

#include <neutrino/tiled/world/types.hh>
#include <neutrino/tiled/world/tile_sheet.hh>
#include <filesystem>
#include <iosfwd>

namespace neutrino::tiled {
  class tile_sheet_manager {
    public:
      tile_sheet_id_t add (hal::surface image, const std::vector<math::rect>& coords);
      tile_sheet_id_t add (hal::surface image);
      tile_sheet_id_t add (std::istream& is, const std::vector<math::rect>& coords);
      tile_sheet_id_t add (std::istream& is);
      tile_sheet_id_t add (const std::filesystem::path& path, const std::vector<math::rect>& coords);
      tile_sheet_id_t add (const std::filesystem::path& path);

      const tile_sheet& get (tile_sheet_id_t idx);
    private:
      std::vector<tile_sheet> m_sheets;
  };
}

#endif
