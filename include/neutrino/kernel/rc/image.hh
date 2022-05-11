//
// Created by igor on 07/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_RC_IMAGE_HH
#define INCLUDE_NEUTRINO_KERNEL_RC_IMAGE_HH

#include <filesystem>
#include <iosfwd>
#include <neutrino/hal/video/surface.hh>

namespace neutrino::kernel {
  using color   = hal::color;
  using palette = hal::palette;
  using image   = hal::surface;

  image load_image(std::istream& istream);
  image load_image(const std::filesystem::path& path);

  void save_image(const std::filesystem::path& path, const image& img);
  void save_png(std::ostream& ostream, const image& img);
  void save_bmp(std::ostream& ostream, const image& img);
  void save_jpg(std::ostream& ostream, const image& img);
  void save_tga(std::ostream& ostream, const image& img);
}

#endif //INCLUDE_NEUTRINO_KERNEL_RC_IMAGE_HH
