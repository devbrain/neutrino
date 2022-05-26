//
// Created by igor on 28/06/2021.
//

#include <vector>
#include <ios>
#include <fstream>

#include "image_loader.hh"
#include "neutrino/utils/exception.hh"
#include "stb_image_loader.hh"

namespace neutrino::assets {

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
  hal::surface image_loader::create (SDL_Surface* impl) {
    return hal::surface(impl);
  }
  // -------------------------------------------------------------------------

  void image_loader::init () {
    detail::register_stb_loaders();
  }
}
