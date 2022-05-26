//
// Created by igor on 05/04/2021.
//

#ifndef TOMBEXCAVATOR_AMIGA_IMAGE_HH
#define TOMBEXCAVATOR_AMIGA_IMAGE_HH

#include <cstdint>
#include <vector>
#include <string>
#include <iosfwd>
#include <optional>

#include "assets/image/amiga/iff/fourcc.hh"
#include "neutrino/hal/video/surface.hh"

namespace formats::image::amiga
{
    enum class image_type
    {
        ILBM,
        PBM,
        ACBM
    };

    enum class masking_type
    {
            NONE,
            HAS_MASK,
            HAS_TRANSPARENT_COLOR,
            LASSO
    };

    enum class compression_type
    {
        NONE,
        BYTE_RUN
    };

    struct rgb {
      uint8_t r,g,b,a;
    };

    using pal_t = std::vector<rgb>;

    struct image
    {
        explicit image(image_type t);
        image_type type;

        uint16_t w, h;
        uint8_t  nPlanes;
        masking_type masking;
        compression_type compression;
        uint16_t transparentColor;
        uint16_t pageWidth, pageHeight;

        pal_t colors;
        std::optional<uint32_t> viewport_mode;
        std::vector<uint8_t> body;

        void update(formats::iff::chunk_type chunk_type, std::istream& is, std::size_t size);
        bool convert(neutrino::hal::surface& out);
    };
}

#endif //TOMBEXCAVATOR_AMIGA_IMAGE_HH
