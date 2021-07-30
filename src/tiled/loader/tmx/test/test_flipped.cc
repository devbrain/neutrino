//
// Created by igor on 29/07/2021.
//

#include <doctest/doctest.h>
#include "test_utils.hh"
#include "test-rs/tiled_flipped.h"
#include "test-rs/tilesheet.h"

using namespace neutrino::tiled::tmx;

TEST_CASE("test_flipped") {
    auto resolver = [](const std::string& p) -> std::string {
        if (p == "tilesheet.tsx") {
            return std::string((char*)tilesheet, tilesheet_length);
        }
        return {};
    };
    auto the_map = test::load_map(tiled_flipped, tiled_flipped_length, resolver);
    REQUIRE(!the_map.layers().empty());
    const auto* tl = std::get_if<tile_layer>(&the_map.layers()[0]);
    REQUIRE(tl != nullptr);

    REQUIRE(tl->cells().size() == 4);
    const auto& t1 = tl->cells()[0];
    const auto& t2 = tl->cells()[1];
    const auto& t3 = tl->cells()[2];
    const auto& t4 = tl->cells()[3];

    REQUIRE(t1.gid() == 3);
    REQUIRE(t1.diag_flipped());
    REQUIRE(t1.hor_flipped());
    REQUIRE(t1.vert_flipped());

    REQUIRE(t2.gid() == 3);
    REQUIRE(!t2.diag_flipped());
    REQUIRE(!t2.hor_flipped());
    REQUIRE(t2.vert_flipped());

    REQUIRE(t3.gid() == 3);
    REQUIRE(!t3.diag_flipped());
    REQUIRE(t3.hor_flipped());
    REQUIRE(!t3.vert_flipped());

    REQUIRE(t4.gid() == 3);
    REQUIRE(t4.diag_flipped());
    REQUIRE(!t4.hor_flipped());
    REQUIRE(!t4.vert_flipped());
}