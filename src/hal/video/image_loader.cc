//
// Created by igor on 28/06/2021.
//

#include <vector>
#include <ios>
#include <fstream>

#include <neutrino/hal/video/image_loader.hh>
#include <neutrino/utils/exception.hh>
#include "stb_image_loader.hh"

namespace neutrino::hal {

  namespace detail {

    std::vector<image_loader*>& loaders_registry () {
      static std::vector<image_loader*> loaders;
      return loaders;
    }
  }

  image_loader::image_loader () {

    detail::loaders_registry ().push_back (this);
  }

  // ------------------------------------------------------------------------
  image_loader::~image_loader () = default;

  // -------------------------------------------------------------------------
  surface image_loader::create (std::unique_ptr<detail::surface_impl>&& impl) {
    return {std::move (impl)};
  }
  // -------------------------------------------------------------------------
  surface load (const std::filesystem::path& path) {
    std::ifstream is (path, std::ios::in | std::ios::binary);
    if (!is) {
      RAISE_EX("Failed to open file ", path);
    }
    return load (is);
  }

  // -------------------------------------------------------------------------
  surface load (std::istream& is) {
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

  void image_loader::init () {
    detail::register_stb_loaders();
  }
}
