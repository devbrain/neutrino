//
// Created by igor on 02/08/2021.
//

#include <doctest/doctest.h>
#include "test-rs/tiled_base64_zlib_infinite.h"
#include "test-rs/tilesheet.h"
#include "test_utils.hh"

using namespace neutrino::tiled::tmx;
TEST_CASE("test tmx infinite") {
    auto resolver = [](const std::string& p) -> std::string {
        if (p == "tilesheet.tsx") {
            return {(char*)tilesheet, tilesheet_length};
        }
        return {};
    };
    auto the_map = test::load_map(tiled_base64_zlib_infinite, tiled_base64_zlib_infinite_length, resolver);
    REQUIRE(the_map.infinite());
    REQUIRE(the_map.layers().size() == 3);
    const auto* tl = std::get_if<tile_layer>(&the_map.layers()[0]);
    REQUIRE(tl);
    REQUIRE(tl->chunks().size() == 4);
    const auto& chnk = tl->chunks()[0];
    REQUIRE(chnk.width() == 32);
    REQUIRE(chnk.height() == 32);
    REQUIRE(chnk.x() == -32);
    REQUIRE(chnk.y() == 0);
    REQUIRE(chnk.cells().size() == chnk.width()*chnk.height());
}