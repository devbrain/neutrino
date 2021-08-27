//
// Created by igor on 06/07/2021.
//

#include <neutrino/tiled/world/tile_sheet.hh>
#include <neutrino/hal/video/renderer.hh>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/utils/exception.hh>
#include "tile_sheet_impl.hh"

namespace neutrino::tiled {
  // --------------------------------------------------------------------------------------------------------------
  tile_sheet::tile_sheet (hal::surface image, const std::vector<math::rect> &coords)
      : m_pimpl (spimpl::make_unique_impl<detail::tile_sheet_impl> (std::move (image), &coords)) {
  }
  // --------------------------------------------------------------------------------------------------------------
  tile_sheet::tile_sheet (hal::surface image)
      : m_pimpl (spimpl::make_unique_impl<detail::tile_sheet_impl> (std::move (image), nullptr)) {
  }
  // --------------------------------------------------------------------------------------------------------------
  tile_sheet::~tile_sheet () = default;
  // --------------------------------------------------------------------------------------------------------------
  bool tile_sheet::is_image () const noexcept {
    return m_pimpl->is_image;
  }
  // --------------------------------------------------------------------------------------------------------------
  std::size_t tile_sheet::size () const noexcept {
    if (!m_pimpl) {
      return 0;
    }
    return m_pimpl->rects.size ();
  }
  // --------------------------------------------------------------------------------------------------------------
  math::dimension_t tile_sheet::dimension (std::size_t idx) const {
    if (idx < size ()) {
      RAISE_EX("Index ", idx, " is out of range (", size (), ")");
    }
    return m_pimpl->rects[idx].dims;
  }
}