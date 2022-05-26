//
// Created by igor on 28/06/2021.
//

#ifndef NEUTRINO_IMAGE_LOADER_HH
#define NEUTRINO_IMAGE_LOADER_HH

#include <iosfwd>
#include <memory>
#include <filesystem>
#include <vector>

#include "neutrino/hal/video/surface.hh"
#include "neutrino/utils/macros.hh"

namespace neutrino::hal::detail {
    struct surface_impl;
  }

namespace neutrino::assets {


  class image_loader {
    public:
      image_loader ();
      virtual ~image_loader ();
      virtual hal::surface load (std::istream& is) = 0;
      virtual bool test (std::istream& is) const = 0;

      static void init();
    protected:
      static hal::surface create (SDL_Surface* impl);
  };

  namespace detail {
    std::vector<image_loader*>& loaders_registry ();
  }
}

#define NEUTRINO_REGISTER_IMAGE_LOADER(LOADER_TYPE) static LOADER_TYPE ANONYMOUS_VAR(register_image_loader)

#endif //NEUTRINO_IMAGE_LOADER_HH
