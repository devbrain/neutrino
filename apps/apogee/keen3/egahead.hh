//
// Created by igor on 09/06/2022.
//

#ifndef APPS_APOGEE_KEEN3_EGAHEAD_HH
#define APPS_APOGEE_KEEN3_EGAHEAD_HH

#include <iostream>
#include <array>
#include <vector>
#include <cstdint>

namespace neutrino::appogee::keen3 {
  class egahead {
    public:
      struct header {
        uint32_t Latplansiz; /* Size of EGALATCH plane; should be one quarter of the size of the UNCOMPRESSED EGALATCH
                          size. This is the size of the four planes in the EGALATCH file */
        uint32_t Sprplansiz; /* Size of EGASPRIT plane; should be one fifth of the size of the UNCOMPRESSED EGASPRIT
                          size. This is the size of the five planes in the EGASPRIT file */
        uint32_t Bmpdatstart; /* Where in the EGAHEAD the entries for unmasked graphics (Excluding font and tiles.)
                           start. If there are none, this is blank. */
        uint32_t Sprdatstart; /* Where in the EGAHEAD the entries for masked graphics (Sprites) start; by default
                           this is right after the unmasked graphics. */
        uint16_t Fontnum;     /* Number of 8x8 font entries are in the font; since font is written first in EGALATCH,
                           this x 8 in bytes defines the offset into each plane that the unmasked bitmaps start */
        uint32_t Fontloc;     /* Offset in plane where font data starts. Should be zero since font is first.
                            In Keen 1-3 also the location in LATCH file where font data starts
                         */
        uint16_t Unknum; //      Used for the ending screen until this was removed. Number of screen graphics.
        uint32_t Unkloc; //      Used for screen graphics until removal. Offset in plane where screen data starts.
        uint16_t Tilenum; //     Number of 16x16 tiles
        uint32_t Tileloc; //     Offset in EGALATCH plane where tile data starts.

        uint16_t Bmpnum; //      Number of unmasked bitmaps
        uint32_t Bmploc;//      Offset in plane where unmasked bitmap data starts.
        uint16_t Spritenum; //   Number of sprite images
        uint32_t Spriteloc; /*   Offset in EGASPRIT plane of start of sprite data. Is,
                           of course, zero. */
        uint16_t Compression; /* Add 2 to this byte if EGALATCH is compressed, add 1
                           to it if EGAHEAD is compressed. Thus uncompressed
                           graphics have this set at 0 and fully compressed at 3 */
      };
      struct unmasked_data {
        uint16_t size_h; // The width of the graphic divided by 8
        uint16_t size_w; /* The height of the graphic in pixels; if this cannot be
                     divided into neat 16 byte pieces, the extra data,
                     usually 8 bytes, is added to the size. */
        uint32_t loc;   /* When added to the graphic offset in the header, gives
                     the location of the start of the graphic data in the
                     plane. For the first graphic this is thus zero. */
        std::array<char, 8> name; // Name of the graphic, padded with nulls.
      };

      struct masked_data {
        uint16_t width; //      The width of the graphic divided by 8
        uint16_t height; /*    The height of the graphic in pixels;  the same rule
                     applies as for unmasked graphics except now we have: */
        uint16_t loc_offset; /* Usually 8, this is the number of bytes 'extra' that
                     must be added to the location to reach the start of
                     the sprite data. This appears when a sprite is so
                     small, usually 8x8 pixels, that it doesn't fill a
                     multiple of 16 bytes. This will affect ALL sprites
                     after the aberration until another one occurs to fix
                     the shortfall. */
        uint16_t location; /*   Multiplying this by 16 bytes gives the location of
                     the start of the sprite data in the EGA plane. */
        uint16_t hitbox_ul_h;
        uint16_t hitbox_ul_v;
        uint16_t hitbox_br_h;
        uint16_t hitbox_br_v;
        std::array<char, 16> Name; /*       The sprite name, usually includes a number and is
                     usually only 10 bytes long. May spill into the next
                     field in games that do not use that. */
      };
    public:
      explicit egahead (std::istream& is);
    private:
      header m_header;
      std::vector<unmasked_data> m_unmasked_data;
      std::vector<masked_data>   m_masked_data;
  };

  std::ostream& operator << (std::ostream& os, const egahead::header& v);
  std::ostream& operator << (std::ostream& os, const egahead::unmasked_data& v);
  std::ostream& operator << (std::ostream& os, const egahead::masked_data& v);
}

#endif //APPS_APOGEE_KEEN3_EGAHEAD_HH
