//
// Created by igor on 10/10/2021.
//

#include <neutrino/hal/video/fonts/rom_fonts.hh>
#include <cstdint>

#include "video/fonts/IBM_VGA_8x16.h"
#include "video/fonts/IBM_VGA_8x14.h"
#include "video/fonts/IBM_PC_V1_8x8.h"

namespace neutrino::hal {
  namespace {
    struct glyph_holder {
        using glyph_data_t = std::vector<std::vector<bool>>;
        glyph_data_t font_8x8[256];
        glyph_data_t font_8x14[256];
        glyph_data_t font_8x16[256];

        glyph_holder () {
          init (IBM_PC_V1_8x8, font_8x8, 8);
          init (IBM_VGA_8x14, font_8x14, 14);
          init (IBM_VGA_8x16, font_8x16, 16);
        }

      private:
        static void init (const uint8_t* data, glyph_data_t* out, std::size_t fy) {
          for (int i = 0; i < 256; i++) {
            get_glyph (data, fy, (uint8_t) i, out[i]);
          }
        }

        static void get_glyph (const uint8_t* buffer, std::size_t fy, uint8_t ch, std::vector<std::vector<bool>>& ret) {
          const std::size_t off = ch * fy;
          ret.resize (fy);
          for (uint8_t y = 0; y < (uint8_t) fy; y++) {
            const uint8_t line_data = buffer[off + y];
            ret[y].resize (8);
            for (uint8_t x = 0; x < 8; x++) {
              uint8_t mask = 8 - x;
              ret[y][x] = ((line_data >> mask) & 0x01);
            }
          }
        }
    };
  } // anon ns
  const std::vector<std::vector<bool>>& get_glyph(font font_type, char ch)
  {
    static glyph_holder holder;
    glyph_holder::glyph_data_t* input =
        (font_type == font::IBM_8x8) ? holder.font_8x8 :
        (font_type == font::IBM_8x14) ? holder.font_8x14 : holder.font_8x16;
    return input[(unsigned char)ch];
  }
}