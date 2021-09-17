//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_TILE_SHEET_HH
#define NEUTRINO_TILED_WORLD_TILE_SHEET_HH

#include <functional>
#include <vector>
#include <optional>
#include <variant>
#include <string>

#include <neutrino/math/rect.hh>
#include <neutrino/hal/video/color.hh>
#include <neutrino/hal/video/texture.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino {

  namespace tiled {


    class tile_sheet {
      public:
        ~tile_sheet ();
        tile_sheet (tile_sheet&&) = default;
        tile_sheet& operator = (tile_sheet&&) = default;
      private:

    };
  } // ns tiled
}
#endif

