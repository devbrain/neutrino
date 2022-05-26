//
// Created by igor on 26/05/2022.
//
#include <fstream>
#include <ios>
#include <neutrino/utils/exception.hh>
#include <neutrino/assets/image/image_io.hh>
#include "stb_image_loader.hh"
#include "image_loader.hh"
#include "image_writer.hh"

namespace neutrino::assets {
  hal::surface load_image (const std::filesystem::path& path) {
    std::ifstream is (path, std::ios::in | std::ios::binary);
    if (!is) {
      RAISE_EX("Failed to open file ", path);
    }
    return load_image (is);
  }

  // -------------------------------------------------------------------------
  hal::surface load_image (std::istream& is) {
    detail::register_stb_loaders();
    auto offs = is.tellg ();
    for (auto* loader : detail::loaders_registry ()) {
      if (loader->test (is)) {
        is.seekg (offs, std::ios::beg);
        auto s = loader->load (is);
        if (!s) {
          RAISE_EX("Failed to load image");
        }
        return s;
      }
      is.seekg (offs, std::ios::beg);
    }
    RAISE_EX("Failed to load image");
  }

  void save_png(std::ostream& os, const hal::surface& s) {
    write_png (os, s.native());
  }

  void save_bmp(std::ostream& os, const hal::surface& s) {
    write_bmp (os, s.native());
  }

  void save_tga(std::ostream& os, const hal::surface& s) {
    write_tga (os, s.native());
  }

  void save_jpg(std::ostream& os, const hal::surface& s) {
    write_jpg (os, s.native());
  }

  void save(const std::filesystem::path& path, const hal::surface& s) {
    write_image (path, s.native());
  }
}
