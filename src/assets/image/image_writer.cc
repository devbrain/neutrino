//
// Created by igor on 04/05/2022.
//
#include <fstream>
#include <ios>
#include "image_writer.hh"
#include "assets/image/thirdparty/stb_image_write.h"
#include "assets/ios_rwops.hh"
#include "neutrino/utils/exception.hh"
#include "assets/image/thirdparty/pngpp/png.hpp"

static void ostream_write_func (void* context, void* data, int size) {
  auto* os = (std::ostream*) context;
  os->write ((char*) data, size);
}

static png::image<png::index_pixel> create8bit_png(const SDL_Surface* s) {
  if (!s->format && !s->format->palette) {
    RAISE_EX("Palette is missing");
  }
  png::image<png::index_pixel> out(s->w, s->h);
  png::palette palette(s->format->palette->ncolors);
  for (int i=0; i<s->format->palette->ncolors; i++) {
    auto* c = s->format->palette->colors + i;
    palette[i] = png::color(c->r, c->g, c->b);
  }
  out.set_palette (palette);
  SDL_PixelFormat * vfmt = s->format;
  Uint8 bytes_per_pixel = vfmt->BytesPerPixel;

  for (int y=0; y<s->h; y++) {
    for (int x=0; x<s->w; x++) {
      unsigned char* pixel_ptr = (unsigned char*)s->pixels + y * s->pitch + x * bytes_per_pixel;
      out.set_pixel (x, y, *pixel_ptr);
    }
  }
  return out;
}

static Uint32 getpixel(const SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;
    case 2:
        return *(Uint16 *)p;
    case 3:
        if constexpr (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
    case 4:
        return *(Uint32 *)p;
    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

static png::image<png::rgb_pixel> create24bit_png(const SDL_Surface* s) {
  png::image<png::rgb_pixel> out(s->w, s->h);

  for (int y=0; y<s->h; y++) {
    for (int x=0; x<s->w; x++) {
      png::rgb_pixel px;
      SDL_GetRGB(getpixel(s, x, y), s->format, &px.red, &px.green, &px.blue);
      out.set_pixel (x, y, px);
    }
  }
  return out;
}

static png::image<png::rgba_pixel> create32bit_png(const SDL_Surface* s) {

  png::image<png::rgba_pixel> out(s->w, s->h);

  for (int y=0; y<s->h; y++) {
    for (int x=0; x<s->w; x++) {
      png::rgba_pixel px;
      SDL_GetRGBA(getpixel(s, x, y), s->format, &px.red, &px.green, &px.blue, &px.alpha);
      out.set_pixel (x, y, px);
    }
  }
  return out;
}

namespace neutrino::assets {
  bool write_png (std::ostream& os, const SDL_Surface* s) {
    if (s->format->BytesPerPixel == 1) {
      auto img = create8bit_png(s);
      img.write_stream (os);
      return os.good();
    } else if (s->format->BytesPerPixel == 2 || s->format->BytesPerPixel == 3) {
      auto img = create24bit_png(s);
      img.write_stream (os);
      return os.good();
    } else if (s->format->BytesPerPixel == 4) {
      auto img = create32bit_png(s);
      img.write_stream (os);
      return os.good();
    }
    RAISE_EX("Unsupported pixel format: ", s->format->BytesPerPixel);
  }

  bool write_bmp (std::ostream& os, const SDL_Surface* s) {
    ostream_wrapper osw(&os);

    if (SDL_SaveBMP_RW (const_cast<SDL_Surface*>(s), osw.handle(), 0) != 0) {
      RAISE_EX("Failed to save BMP: ", SDL_GetError());
    }
    return true;
  }

  bool write_jpg (std::ostream& os, const SDL_Surface* s) {

    return
        stbi_write_jpg_to_func (ostream_write_func, &os, s->w, s->h, s->format->BytesPerPixel, s->pixels, 90)
        > 0
        && os.good ();
  }

  bool write_tga (std::ostream& os, const SDL_Surface* s) {
    return
        stbi_write_tga_to_func (ostream_write_func, &os, s->w, s->h, s->format->BytesPerPixel, s->pixels)
        > 0
        && os.good ();
  }

  bool write_image(const std::filesystem::path& ofile, const SDL_Surface* s) {
    auto ext = ofile.extension().string();
    if (ext == ".png") {
      std::ofstream ofs(ofile, std::ios::out | std::ios::trunc | std::ios::binary);
      return write_png (ofs, s);
    } else if (ext == ".tga") {
      std::ofstream ofs(ofile, std::ios::out | std::ios::trunc | std::ios::binary);
      return write_tga (ofs, s);
    } else if (ext == ".jpg") {
      std::ofstream ofs(ofile, std::ios::out | std::ios::trunc | std::ios::binary);
      return write_jpg (ofs, s);
    } else if (ext == ".bmp") {
      std::ofstream ofs(ofile, std::ios::out | std::ios::trunc | std::ios::binary);
      return write_bmp (ofs, s);
    }
    RAISE_EX("Unknown output format ", ext);
  }
}