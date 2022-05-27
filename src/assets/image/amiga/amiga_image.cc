//
// Created by igor on 05/04/2021.
//

#include "amiga_image.hh"
#include "assets/image/amiga/iff/iff_events.hh"
#include "assets/image/amiga/iff/ea/ea_events.hh"
#include "neutrino/utils/exception.hh"
#include "neutrino/utils/override.hh"
#include "amiga_chunks.hh"

extern "C"
{
#include "assets/image/thirdparty/libamivideo/palette.h"
#include "assets/image/thirdparty/libamivideo/screen.h"
#include "assets/image/thirdparty/libamivideo/viewportmode.h"
#include "assets/image/thirdparty/libamivideo/amivideotypes.h"
}

#include <cstring>

using ILBM_Image = formats::image::amiga::image;

static bool ILBM_imageIsILBM (const ILBM_Image* image) {
  return (image->type == formats::image::amiga::image_type::ILBM);
}

static bool ILBM_imageIsACBM (const ILBM_Image* image) {
  return (image->type == formats::image::amiga::image_type::ACBM);
}

static bool ILBM_imageIsPBM (const ILBM_Image* image) {
  return (image->type == formats::image::amiga::image_type::PBM);
}

static unsigned int ILBM_calculateRowSize (const ILBM_Image* image) {
  unsigned int rowSizeInWords = image->w / 16;

  if (image->w % 16 != 0) {
    rowSizeInWords++;
  }

  return (rowSizeInWords * 2);
}

static unsigned int ILBM_calculateNumOfColors (const ILBM_Image* image) {
  switch (image->nPlanes) {
    case 1:
      return 2;
    case 2:
      return 4;
    case 3:
      return 8;
    case 4:
      return 16;
    case 5:
      return 32;
    case 6:
      return 64;
    case 7:
      return 128;
    case 8:
      return 256;
    default:
      return 0;
  }
}

static formats::image::amiga::pal_t ILBM_generateGrayscaleColorMap (const ILBM_Image* image) {
  unsigned int numOfColors = ILBM_calculateNumOfColors (image);
  formats::image::amiga::pal_t colorMap (numOfColors);

  unsigned int i;

  for (i = 0; i < numOfColors; i++) {
    auto* colorRegister = &colorMap[i];
    unsigned int value = i * 0xff / (numOfColors - 1);

    colorRegister->r = value;
    colorRegister->g = value;
    colorRegister->b = value;
  }

  return colorMap;
}

// --------------------------------------------------------------------------------------------------------------------------
static void SDL_ILBM_initPaletteFromImage (const ILBM_Image* image, amiVideo_Palette* palette) {
  if (image->colors.empty ()) {
    /* If no colormap is provided by the image, use a generated grayscale one */
    auto colorMap = ILBM_generateGrayscaleColorMap (image);
    amiVideo_setBitplanePaletteColors (palette, (amiVideo_Color*) colorMap.data (), colorMap.size ());
  }
  else {
    amiVideo_setBitplanePaletteColors (palette, (amiVideo_Color*) image->colors.data (),
                                       image->colors.size ());
  } /* Otherwise, use the provided color map */
}

static amiVideo_ULong SDL_ILBM_extractViewportModeFromImage (const ILBM_Image* image) {
  amiVideo_ULong paletteFlags, resolutionFlags;

  if (!image->viewport_mode) {
    paletteFlags = 0; /* If no viewport value is set, assume 0 value */
  }
  else {
    paletteFlags = amiVideo_extractPaletteFlags (*image->viewport_mode);
  } /* Only the palette flags can be considered "reliable" from a viewport mode value */

  /* Resolution flags are determined by looking at the page dimensions */
  resolutionFlags = amiVideo_autoSelectViewportMode (image->pageWidth, image->pageHeight);

  /* Return the combined settings of the previous */
  return paletteFlags | resolutionFlags;
}

static void ILBM_unpackByteRun (ILBM_Image* image) {
  auto& body = image->body;

  /* Only perform decompression if the body is compressed and present */
  if (image->compression == formats::image::amiga::compression_type::BYTE_RUN && !body.empty ()) {
    /* Counters */
    unsigned int count = 0;
    unsigned int readBytes = 0;

    /* Allocate decompressed chunk attributes */

    auto chunkSize = ILBM_calculateRowSize (image) * image->h * image->nPlanes;
    std::vector<uint8_t> decompressedChunkData (chunkSize);

    /* Perform RLE decompression */

    while (readBytes < body.size ()) {
      int byte = (char) body[readBytes];
      readBytes++;

      if (byte >= 0 && byte <= 127) /* Take the next byte bytes + 1 literally */
      {
        int i;

        for (i = 0; i < byte + 1; i++) {
          decompressedChunkData[count] = body[readBytes];
          readBytes++;
          count++;
        }
      }
      else {
        if (byte >= -127 && byte <= -1) /* Replicate the next byte, -byte + 1 times */
        {
          uint8_t ubyte;
          int i;

          ubyte = body[readBytes];
          readBytes++;

          for (i = 0; i < -byte + 1; i++) {
            decompressedChunkData[count] = ubyte;
            count++;
          }
        }
        else {
          RAISE_EX("Unknown byte run encoding byte!");
        }
      }
    }

    /* Free the compressed chunk data */

    std::swap (image->body, decompressedChunkData);

  }
}

static void ILBM_deinterleaveToBitplaneMemory (const ILBM_Image* image, uint8_t** bitplanePointers) {
  if (!image->body.empty ()) {
    unsigned int i;
    int count = 0; /* Offset in the interleaved source */
    int hOffset = 0; /* Horizontal offset in resulting bitplanes */
    unsigned int rowSize = ILBM_calculateRowSize (image);

    for (i = 0; i < image->h; i++) {
      unsigned int j;

      for (j = 0; j < image->nPlanes; j++) {
        memcpy (bitplanePointers[j] + hOffset, image->body.data () + count, rowSize);
        count += rowSize;
      }

      hOffset += rowSize;
    }
  }
}

static void ILBM_deinterleave (ILBM_Image* image) {
  auto nPlanes = image->nPlanes;
  unsigned int bitplaneSize = ILBM_calculateRowSize (image) * image->h;
  std::vector<uint8_t> result (bitplaneSize * nPlanes);

#define MAX_NUM_OF_BITPLANES 32

  unsigned int i;
  unsigned int offset = 0;
  uint8_t* bitplanePointers[MAX_NUM_OF_BITPLANES];

  /* Set bitplane pointers */

  for (i = 0; i < nPlanes; i++) {
    bitplanePointers[i] = result.data () + offset;
    offset += bitplaneSize;
  }

  /* Deinterleave and write results to the bitplane addresses */
  ILBM_deinterleaveToBitplaneMemory (image, bitplanePointers);

  /* Return result */

  std::swap (image->body, result);

}

static void SDL_ILBM_attachImageToScreen (ILBM_Image* image, amiVideo_Screen* screen) {
  /* Determine which viewport mode is best for displaying the image */
  auto viewportMode = SDL_ILBM_extractViewportModeFromImage (image);

  /* Initialize the screen with the image's dimensions, bitplane depth, and viewport mode */
  amiVideo_initScreen (screen, image->w, image->h, image->nPlanes, 8, viewportMode);

  /* Sets the colors of the palette */
  SDL_ILBM_initPaletteFromImage (image, &screen->palette);

  /* Decompress the image body */
  ILBM_unpackByteRun (image);

  /* Attach the appropriate pixel surface to the screen */
  if (ILBM_imageIsPBM (image)) {
    amiVideo_setScreenUncorrectedChunkyPixelsPointer (screen, (amiVideo_UByte*) image->body.data (),
                                                      image->w); /* A PBM has chunky pixels in its body */
  }
  else {
    if (ILBM_imageIsACBM (image)) {
      amiVideo_setScreenBitplanes (screen,
                                   (amiVideo_UByte*) image->body.data ()); /* Set bitplane pointers of the conversion screen */
    }
    else {
      if (ILBM_imageIsILBM (image)) {
        /* Amiga ILBM image has interleaved scanlines per bitplane. We have to deinterleave it in order to be able to convert it */
        ILBM_deinterleave (image);
        amiVideo_setScreenBitplanes (screen,
                                     (amiVideo_UByte*) image->body.data ()); /* Set bitplane pointers of the conversion screen */
      }
    }
  }
}

static constexpr uint32_t Rmask = 0x00FF0000;
static constexpr uint32_t Gmask = 0x0000FF00;
static constexpr uint32_t Bmask = 0x000000FF;
static constexpr uint32_t Amask = 0xFF000000;

static int eval_mask (uint32_t xmask) noexcept {
  int shift = 0;
  for (uint32_t mask = xmask; !(mask & 0x01u); mask >>= 1u) {
    ++shift;
  }
  return shift;
}

static const uint32_t Rshift = eval_mask (Rmask);
static const uint32_t Gshift = eval_mask (Gmask);
static const uint32_t Bshift = eval_mask (Bmask);
static const uint32_t Ashift = eval_mask (Amask);
namespace {
  struct Surface {

    Surface (unsigned int w, unsigned int h, int depth)
        : pitch (w),
          height (h),
          bit_depth (depth) {
      if (depth == 8) {
        pixels.resize (w * h, 0);
      }
      else {
        pixels.resize (w * h * sizeof (uint32_t), 0);
      }
    }

    [[nodiscard]] neutrino::hal::surface convert () const {
      neutrino::hal::surface out = bit_depth == 32 ? neutrino::hal::surface::make_rgba (pitch, height)
                                                   : neutrino::hal::surface::make_8bit (pitch, height);

      if (bit_depth == 8) {
        neutrino::hal::palette p (palette.size ());
        for (std::size_t i = 0; i < palette.size (); i++) {
          neutrino::hal::color c;
          c.r = palette[i].r;
          c.g = palette[i].g;
          c.b = palette[i].b;
          p.set (i, c);
        }
        out.set_palette (p);
        for (unsigned int y = 0; y < height; y++) {
          for (unsigned int x = 0; x < pitch; x++) {
            uint8_t color = pixels[y * pitch + x];
            out.put_pixel (x, y, color);
          }
        }
      }
      else {
        const auto* dwords = (uint32_t*) pixels.data ();
        for (unsigned int y = 0; y < height; y++) {
          for (unsigned int x = 0; x < pitch; x++) {
            uint32_t color = dwords[y * pitch + x];
            uint8_t r = (color & Rmask) >> Rshift;
            uint8_t g = (color & Gmask) >> Gshift;
            uint8_t b = (color & Bmask) >> Bshift;
            uint8_t a = (color & Amask) >> Ashift;
            out.put_pixel (x, y, out.map_color ({r, g, b, a}));
          }
        }
      }
      return out;
    }

    unsigned int pitch;
    unsigned int height;
    int bit_depth;
    std::vector<uint8_t> pixels;
    formats::image::amiga::pal_t palette;
  };
}

static void SDL_ILBM_setSurfacePaletteFromScreenPalette (amiVideo_Palette* palette, Surface& surface) {
  for (unsigned int i = 0; i < palette->chunkyFormat.numOfColors; i++) {
    const auto& x = palette->chunkyFormat.color[i];
    formats::image::amiga::rgb c{x.r, x.g, x.b, x.a};
    surface.palette.push_back (c);
  }
}

static Surface* SDL_ILBM_createUncorrectedChunkySurfaceFromScreen (amiVideo_Screen* screen) {
  auto* surface = new Surface (screen->width, screen->height, 8);

  /* Sets the uncorrected chunky pixels pointer of the conversion struct to that of the SDL pixel surface */
  amiVideo_setScreenUncorrectedChunkyPixelsPointer (screen, surface->pixels.data (), surface->pitch);

  /* Convert the colors of the the bitplane palette to the format of the chunky palette */
  amiVideo_convertBitplaneColorsToChunkyFormat (&screen->palette);

  /* Set the palette of the target SDL surface */
  SDL_ILBM_setSurfacePaletteFromScreenPalette (&screen->palette, *surface);

  return surface;
}

static Surface* SDL_ILBM_createUncorrectedRGBSurfaceFromScreen (amiVideo_Screen* screen, const ILBM_Image* image) {
  auto* surface = new Surface (screen->width, screen->height, 32);
  int allocateUncorrectedMemory = !ILBM_imageIsPBM (image);
  /* Set the uncorrected RGB pixels pointer of the conversion struct to that of the SDL pixel surface */
  amiVideo_setScreenUncorrectedRGBPixelsPointer (screen, (amiVideo_ULong*) surface->pixels.data (), surface->pitch,
                                                 allocateUncorrectedMemory, Rshift, Gshift, Bshift, Ashift);

  return surface;
}

static void SDL_ILBM_renderUncorrectedChunkyImage (const ILBM_Image* image, amiVideo_Screen* screen) {
  if (ILBM_imageIsPBM (image)) {
    memcpy (screen->uncorrectedChunkyFormat.pixels, image->body.data (),
            image->body.size ()); /* For a PBM no conversion is needed => simply copy the data into the surface */
  }
  else {
    amiVideo_convertScreenBitplanesToChunkyPixels (screen); /* Convert the bitplanes to chunky pixels */
  }
}

static void SDL_ILBM_renderUncorrectedRGBImage (const ILBM_Image* image, amiVideo_Screen* screen) {
  if (ILBM_imageIsPBM (image)) {
    if (screen->bitplaneDepth == 24 || screen->bitplaneDepth == 32) {
      /* For images with a higher than 8 bitplane depth (true color images) => copy chunky data to the RGB section and reorder the pixels */
      memcpy (screen->uncorrectedRGBFormat.pixels, image->body.data (), image->body.size ());
      amiVideo_reorderRGBPixels (screen);
    }
    else {
      amiVideo_convertScreenChunkyPixelsToRGBPixels (screen);
    } /* Convert chunky to RGB data */

  }
  else {
    amiVideo_convertScreenBitplanesToRGBPixels (screen); /* Convert the bitplanes to RGB pixels */
  }
}

static Surface* createSurfaceFromScreen (amiVideo_Screen* screen, ILBM_Image* image) {
  auto realFormat = amiVideo_autoSelectColorFormat (screen);

  /* Create and render the surface */
  Surface* surface = nullptr;
  if (realFormat == AMIVIDEO_CHUNKY_FORMAT) {
    surface = SDL_ILBM_createUncorrectedChunkySurfaceFromScreen (screen);
    SDL_ILBM_renderUncorrectedChunkyImage (image, screen);
  }
  else {
    surface = SDL_ILBM_createUncorrectedRGBSurfaceFromScreen (screen, image);
    SDL_ILBM_renderUncorrectedRGBImage (image, screen);
  }
  return surface;
}

namespace formats::image::amiga {
#define ILBM_MSK_NONE 0
#define ILBM_MSK_HAS_MASK 1
#define ILBM_MSK_HAS_TRANSPARENT_COLOR 2
#define ILBM_MSK_LASSO  3

  // ============================================================================================================
  bool image::convert (neutrino::hal::surface& out) {
    bool rc = false;
    amiVideo_Screen screen;
    try {
      SDL_ILBM_attachImageToScreen (this, &screen);
      std::unique_ptr<Surface> surface (createSurfaceFromScreen (&screen, this));
      out = surface->convert ();
      rc = true;
    }
    catch (std::exception&) {
    }
    amiVideo_cleanupScreen (&screen);
    return rc;
  }

  // ============================================================================================================
  void image::update (formats::iff::chunk_type chunk_type, std::istream& is, std::size_t size) {
    using namespace formats::amiga;
    std::visit (neutrino::utils::overload (
                    [this] (const bmhd& e) {
                      w = e.Width;
                      h = e.Height;
                      nPlanes = e.Bitplanes;
                      masking = masking_type::NONE;
                      switch (e.Masking) {
                        case ILBM_MSK_HAS_MASK:
                          masking = masking_type::HAS_MASK;
                          break;
                        case ILBM_MSK_NONE:
                          masking = masking_type::NONE;
                          break;
                        case ILBM_MSK_HAS_TRANSPARENT_COLOR:
                          masking = masking_type::HAS_TRANSPARENT_COLOR;
                          break;
                        case ILBM_MSK_LASSO:
                          masking = masking_type::LASSO;
                          break;
                        default:
                          RAISE_EX("Unknown masking type ", (int) e.Masking);
                      }
                      if (e.Compress > 1) {
                        RAISE_EX("Unknown compression type ", (int) e.Compress);
                      }
                      else {
                        compression = (e.Compress == 1) ? compression_type::BYTE_RUN : compression_type::NONE;
                      }
                      pageWidth = e.PageWidth;
                      pageHeight = e.Height;
                    },
                    // --------------------------------------------------------
                    [this] (const vport& e) {
                      viewport_mode = e.mode;
                    },
                    // --------------------------------------------------------
                    [this] (const cmap& e) {
                      colors.resize (e.colors.size ());
                      std::memcpy (colors.data (), e.colors.data (), e.colors.size () * sizeof (cmap::cmap_entry));
                    },
                    // --------------------------------------------------------
                    [this] (const formats::amiga::body& e) {
                      this->body = e.data;
                    },
                    [this] (const bitplanes& e) {
                        this->body = e.data;
                    },
                    // --------------------------------------------------------
                    [] (const std::monostate&) {}
                ),
                formats::iff::parse_chunk<bmhd, vport, cmap, formats::amiga::body, bitplanes> (chunk_type, is, size));
  }

  // ========================================================================================================
  image::image (image_type t)
      : type (t) {
  }
}

