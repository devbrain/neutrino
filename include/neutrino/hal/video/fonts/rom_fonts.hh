//
// Created by igor on 10/10/2021.
//

#ifndef INCLUDE_NEUTRINO_HAL_VIDEO_FONTS_ROM_FONTS_HH
#define INCLUDE_NEUTRINO_HAL_VIDEO_FONTS_ROM_FONTS_HH
#include <vector>
namespace neutrino::hal {
  enum class font
  {
    IBM_8x8,
    IBM_8x14,
    IBM_8x16
  };

  [[nodiscard]] const std::vector<std::vector<bool>>& get_glyph(font font_type, char ch);
}
#endif //INCLUDE_NEUTRINO_HAL_VIDEO_FONTS_ROM_FONTS_HH
