//
// Created by igor on 28/06/2021.
//

#include <neutrino/hal/video/image_loader.hh>
#include <hal/video/surface_impl.hh>
#include "hal/ios_rwops.hh"
#include "hal/video/amiga/lbm.hh"
#include "stb_image_loader.hh"

#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#elif defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4456 4701 4267 4244)
#endif

#define STBI_NO_STDIO
#include "hal/video/thirdparty/SDL_stbimage.h"

namespace {
#include "hal/video/thirdparty/SDL_lbm.h"
#include "hal/video/thirdparty/SDL_pcx.h"

}

#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop

#elif defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace neutrino::hal::detail {
  class stbi_image_loader : public image_loader {
    public:
      stbi_image_loader ()
          : m_surface (nullptr) {
      }

      surface load ([[maybe_unused]] std::istream& is) override {
        if (m_surface) {
          auto impl = std::make_unique<surface_impl> (sdl::object<SDL_Surface> (m_surface, true));
          return create (std::move (impl));
        }
        return {};
      }

      bool test (std::istream& is) const override {
        istream_wrapper io (&is);
        m_surface = STBIMG_Load_RW (io.handle (), 0);
        return (m_surface != nullptr);
      }

    private:
      mutable SDL_Surface* m_surface;
  };



  using loader_fn_ptr = SDL_Surface* (*) (SDL_RWops* src);
  using tester_fn_ptr = int (*) (SDL_RWops* src);

  class sdl_image_loader : public image_loader {
    public:
      sdl_image_loader (loader_fn_ptr loader, tester_fn_ptr tester)
          : m_loader (loader), m_tester (tester) {
      }

      surface load ([[maybe_unused]] std::istream& is) override {
        istream_wrapper io (&is);
        auto s = m_loader (io.handle ());
        if (s) {
          auto impl = std::make_unique<surface_impl> (sdl::object<SDL_Surface> (s, true));
          return create (std::move (impl));
        }
        return {};
      }

      bool test (std::istream& is) const override {
        istream_wrapper io (&is);

        return m_tester (io.handle ()) != 0;
      }

    private:
      loader_fn_ptr m_loader;
      tester_fn_ptr m_tester;
  };

  class pcx_loader : public sdl_image_loader {
    public:
      pcx_loader ()
          : sdl_image_loader (IMG_LoadPCX_RW, IMG_isPCX) {
      }
  };



  class lbm_loader : public image_loader {
    public:

      surface load (std::istream& is) override {
        return load_lbm (is);
      }

      bool test (std::istream& is) const override {
        return is_lbm (is);
      }
  };

  void register_stb_loaders() {
    static bool initialized = false;
    if (!initialized) {
      NEUTRINO_REGISTER_IMAGE_LOADER(lbm_loader);
      NEUTRINO_REGISTER_IMAGE_LOADER(pcx_loader);
      NEUTRINO_REGISTER_IMAGE_LOADER(stbi_image_loader);
      initialized = true;
    }
  }
}

