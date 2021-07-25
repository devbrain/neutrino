//
// Created by igor on 24/07/2021.
//

#ifndef NEUTRINO_TEST_UTILS_HH
#define NEUTRINO_TEST_UTILS_HH


#include "tiled/loader/tmx/xml.hh"
#include "tiled/loader/tmx/map.hh"

namespace neutrino::tiled::tmx::test {
    map load_map(const unsigned char* data, std::size_t length);
}

#endif

