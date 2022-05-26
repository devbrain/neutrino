//
// Created by igor on 26/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_IMAGE_IMAGE_IO_HH
#define INCLUDE_NEUTRINO_ASSETS_IMAGE_IMAGE_IO_HH

#include <iosfwd>
#include <memory>
#include <filesystem>

#include <neutrino/hal/video/surface.hh>
#include <neutrino/hal/video/palette.hh>

namespace neutrino::assets {
  hal::surface load_image (std::istream& is);
  hal::surface load_image (const std::filesystem::path& path);

  void save_png(std::ostream& os, const hal::surface& s);
  void save_bmp(std::ostream& os, const hal::surface& s);
  void save_tga(std::ostream& os, const hal::surface& s);
  void save_jpg(std::ostream& os, const hal::surface& s);

  void save(const std::filesystem::path& path, const hal::surface& s);
}

#endif //INCLUDE_NEUTRINO_ASSETS_IMAGE_IMAGE_IO_HH
