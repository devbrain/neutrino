//
// Created by igor on 06/05/2022.
//

#ifndef SRC_HAL_VIDEO_AMIGA_LBM_HH
#define SRC_HAL_VIDEO_AMIGA_LBM_HH

#include <iosfwd>
#include "neutrino/hal/video/surface.hh"

namespace neutrino::assets::detail {
  bool is_lbm(std::istream& is);
  hal::surface load_lbm (std::istream& is);
}


#endif //SRC_HAL_VIDEO_AMIGA_LBM_HH
