//
// Created by igor on 26/07/2021.
//

#include "cell.hh"

namespace neutrino::tiled::tmx {
    cell cell::decode_gid(unsigned int gid) {
        static constexpr unsigned FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
        static constexpr unsigned FLIPPED_VERTICALLY_FLAG   = 0x40000000;
        static constexpr unsigned FLIPPED_DIAGONALLY_FLAG   = 0x20000000;

        // Read out the flags
        bool hflip = (gid & FLIPPED_HORIZONTALLY_FLAG);
        bool vflip = (gid & FLIPPED_VERTICALLY_FLAG);
        bool dflip = (gid & FLIPPED_DIAGONALLY_FLAG);

        // Clear the flags
        gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

        return cell(gid, hflip, vflip, dflip);
    }
}
