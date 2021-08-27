//
// Created by igor on 06/07/2021.
//

#include <neutrino/tiled/world/tile_sheet_manager.hh>
#include <neutrino/hal/video/image_loader.hh>

namespace neutrino::tiled {
  tile_sheet_id_t tile_sheet_manager::add (hal::surface image, const std::vector<math::rect> &coords) {
    auto n = m_sheets.size ();
    m_sheets.emplace_back (std::move (image), coords);
    return tile_sheet_id_t{n};
  }
  // ------------------------------------------------------------------------------------------------------------
  tile_sheet_id_t tile_sheet_manager::add (hal::surface image) {
    auto n = m_sheets.size ();
    m_sheets.emplace_back (std::move (image));
    return tile_sheet_id_t{n};
  }
  // ------------------------------------------------------------------------------------------------------------
  tile_sheet_id_t tile_sheet_manager::add (std::istream &is, const std::vector<math::rect> &coords) {
    return add (hal::load (is), coords);
  }
  // ------------------------------------------------------------------------------------------------------------
  tile_sheet_id_t tile_sheet_manager::add (std::istream &is) {
    return add (hal::load (is));
  }
  // ------------------------------------------------------------------------------------------------------------
  tile_sheet_id_t tile_sheet_manager::add (const std::filesystem::path &path, const std::vector<math::rect> &coords) {
    return add (hal::load (path), coords);
  }
  // ------------------------------------------------------------------------------------------------------------
  tile_sheet_id_t tile_sheet_manager::add (const std::filesystem::path &path) {
    return add (hal::load (path));
  }
  // ------------------------------------------------------------------------------------------------------------
  const tile_sheet &tile_sheet_manager::get (tile_sheet_id_t idx) {
    return m_sheets[value_of (idx)];
  }
}