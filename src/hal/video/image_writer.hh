//
// Created by igor on 04/05/2022.
//

#ifndef SRC_HAL_VIDEO_IMAGE_WRITER_HH
#define SRC_HAL_VIDEO_IMAGE_WRITER_HH

#include <ostream>
#include <filesystem>
#include <hal/sdl/surface.hh>

namespace neutrino::hal {
  bool write_png(std::ostream& os, const SDL_Surface* s);
  bool write_bmp(std::ostream& os, const SDL_Surface* s);
  bool write_jpg(std::ostream& os, const SDL_Surface* s);
  bool write_tga(std::ostream& os, const SDL_Surface* s);

  bool write_image(const std::filesystem::path& ofile, const SDL_Surface* s);

}

#endif //SRC_HAL_VIDEO_IMAGE_WRITER_HH
