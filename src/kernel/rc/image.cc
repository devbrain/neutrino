//
// Created by igor on 07/05/2022.
//

#include <neutrino/kernel/rc/image.hh>
#include <neutrino/hal/video/image_loader.hh>

namespace neutrino::kernel {
  image load_image(std::istream& istream) {
    return hal::load (istream);
  }

  image load_image(const std::filesystem::path& path) {
    return hal::load (path);
  }

  void save_image(const std::filesystem::path& path, const image& img) {
    img.save (path);
  }

  void save_png(std::ostream& ostream, const image& img) {
    img.save_png (ostream);
  }

  void save_bmp(std::ostream& ostream, const image& img) {
    img.save_bmp (ostream);
  }

  void save_jpg(std::ostream& ostream, const image& img) {
    img.save_jpg (ostream);
  }

  void save_tga(std::ostream& ostream, const image& img) {
    img.save_tga (ostream);
  }
}