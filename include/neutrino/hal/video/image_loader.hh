//
// Created by igor on 28/06/2021.
//

#ifndef NEUTRINO_IMAGE_LOADER_HH
#define NEUTRINO_IMAGE_LOADER_HH

#include <iosfwd>
#include <memory>
#include <filesystem>

#include <neutrino/hal/video/surface.hh>
#include <neutrino/utils/macros.hh>

namespace neutrino::hal {

  surface load (std::istream &is);
  surface load (const std::filesystem::path &path);

  namespace detail {
    struct surface_impl;
  }

  class image_loader {
    public:
      image_loader ();
      virtual ~image_loader ();
      virtual surface load (std::istream &is) = 0;
      virtual bool test (std::istream &is) const = 0;
    protected:
      static surface create (std::unique_ptr<detail::surface_impl> &&impl);
  };
}

#define NEUTRINO_REGISTER_IMAGE_LOADER(LOADER_TYPE) static LOADER_TYPE ANONYMOUS_VAR(register_image_loader)

#endif //NEUTRINO_IMAGE_LOADER_HH
