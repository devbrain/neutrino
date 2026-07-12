//
// Created by igor on 05/07/2026.
//

#include <doctest/doctest.h>

#include <neutrino/world/tmx_loader.hh>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#ifndef NEUTRINO_TEST_DATA_DIR
#error "NEUTRINO_TEST_DATA_DIR must point to the test fixture directory"
#endif

namespace {
    const auto data_root = std::filesystem::path{NEUTRINO_TEST_DATA_DIR};

    [[nodiscard]] std::string read_fixture(const std::filesystem::path& relative) {
        std::ifstream input(data_root / relative, std::ios::binary);
        REQUIRE_MESSAGE(input, "missing test fixture: " << (data_root / relative));
        std::ostringstream out;
        out << input.rdbuf();
        return out.str();
    }

    [[nodiscard]] neutrino::tmx_path_resolver resolver_for(const std::filesystem::path& directory) {
        return [directory](std::string_view source) {
            return read_fixture(directory / std::filesystem::path(source));
        };
    }

    [[nodiscard]] neutrino::world load_world(
        const std::filesystem::path& relative,
        neutrino::tmx_path_resolver resolver = {}) {
        return neutrino::load_tmx_world(read_fixture(relative), std::move(resolver));
    }

    [[nodiscard]] const neutrino::world_tile_layer& tile_layer(
        const neutrino::world& world,
        std::size_t index) {
        auto layers = world.tile_layers();
        REQUIRE(index < layers.size());
        return *layers[index];
    }

    [[nodiscard]] const neutrino::world_image_layer& image_layer(
        const neutrino::world& world,
        std::size_t index) {
        auto layers = world.image_layers();
        REQUIRE(index < layers.size());
        return *layers[index];
    }

    [[nodiscard]] const neutrino::world_object_layer& object_layer(
        const neutrino::world& world,
        std::size_t index) {
        auto layers = world.object_layers();
        REQUIRE(index < layers.size());
        return *layers[index];
    }

    [[nodiscard]] bool same_cells(
        const std::vector <neutrino::world_tile_cell>& lhs,
        const std::vector <neutrino::world_tile_cell>& rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (lhs[i].gid != rhs[i].gid || lhs[i].flip != rhs[i].flip) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool has_property(
        const neutrino::world_component& component,
        std::string_view name,
        const neutrino::world_property& expected) {
        const auto actual = component.get_property(name);
        return actual && *actual == expected;
    }

    void check_point(const neutrino::world_point& point, float x, float y) {
        CHECK(point.x == doctest::Approx(x));
        CHECK(point.y == doctest::Approx(y));
    }
}

TEST_CASE("tmx loader reads csv xml and base64 tile encodings") {
    static const std::vector <std::uint32_t> expected = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
        1621, 1622, 1623, 1624, 1625, 1626, 1627, 1628, 1629, 1630,
        1639, 1640, 1641, 1642, 1643, 1644, 1645, 1646, 1647, 1648,
        61, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    for (const auto& file : {
             "test1/test_csv_level.tmx",
             "test1/test_xml_level.tmx",
             "test1/test_base64_level.tmx"
         }) {
        const auto world = load_world(file);
        const auto& layer = tile_layer(world, 0);
        REQUIRE(layer.cells.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            CHECK(layer.cells[i].gid == expected[i]);
        }
    }
}

TEST_CASE("tmx loader reads compressed tile data") {
    const auto zlib_world = load_world("test-rs/tiled_base64_zlib.tmx");
    const auto gzip_world = load_world("test-rs/tiled_base64_gzip.tmx");
    const auto json_gzip_world = load_world("test-rs/js_tiled_base64_gzip.json");
#if defined(NEUTRINO_TMX_ENABLE_ZSTD)
    const auto zstd_world = load_world("test-rs/tiled_base64_zstandard.tmx");
#endif

    const auto& zlib_layer = tile_layer(zlib_world, 0);
    const auto& gzip_layer = tile_layer(gzip_world, 0);
    const auto& json_gzip_layer = tile_layer(json_gzip_world, 0);
#if defined(NEUTRINO_TMX_ENABLE_ZSTD)
    const auto& zstd_layer = tile_layer(zstd_world, 0);
#endif

    REQUIRE(zlib_layer.cells.size() == 10000);
    REQUIRE(gzip_layer.cells.size() == 10000);
    REQUIRE(json_gzip_layer.cells.size() == 10000);
#if defined(NEUTRINO_TMX_ENABLE_ZSTD)
    REQUIRE(zstd_layer.cells.size() == 10000);
#endif
    CHECK(same_cells(zlib_layer.cells, gzip_layer.cells));
    CHECK(same_cells(json_gzip_layer.cells, gzip_layer.cells));
#if defined(NEUTRINO_TMX_ENABLE_ZSTD)
    CHECK(same_cells(zstd_layer.cells, gzip_layer.cells));
#endif
}

TEST_CASE("tmx loader decodes tiled flip flags") {
    const auto world = load_world("test-rs/tiled_flipped.tmx", resolver_for("test-rs"));
    const auto& layer = tile_layer(world, 0);
    REQUIRE(layer.cells.size() == 4);

    CHECK(layer.cells[0].gid == 3);
    CHECK(layer.cells[0].diagonally_flipped());
    CHECK(layer.cells[0].horizontally_flipped());
    CHECK(layer.cells[0].vertically_flipped());

    CHECK(layer.cells[1].gid == 3);
    CHECK(!layer.cells[1].diagonally_flipped());
    CHECK(!layer.cells[1].horizontally_flipped());
    CHECK(layer.cells[1].vertically_flipped());

    CHECK(layer.cells[2].gid == 3);
    CHECK(!layer.cells[2].diagonally_flipped());
    CHECK(layer.cells[2].horizontally_flipped());
    CHECK(!layer.cells[2].vertically_flipped());

    CHECK(layer.cells[3].gid == 3);
    CHECK(layer.cells[3].diagonally_flipped());
    CHECK(!layer.cells[3].horizontally_flipped());
    CHECK(!layer.cells[3].vertically_flipped());
}

TEST_CASE("tmx loader reads simple json maps") {
    const auto world = load_world("json/simple_map.json");
    REQUIRE(!world.infinite());
    CHECK(world.orientation() == neutrino::world_orientation::orthogonal);
    CHECK(world.render_order() == neutrino::world_render_order::left_down);
    CHECK(world.tile_width() == 16);
    CHECK(world.tile_height() == 16);
    CHECK(world.width() == 16);
    CHECK(world.height() == 16);

    REQUIRE(world.tilesets().size() == 1);
    const auto& tileset = world.tilesets()[0];
    CHECK(tileset.first_gid == 1);
    CHECK(tileset.name == "demo-tileset");
    CHECK(tileset.tile_count == 48);
    CHECK(tileset.tile_width == 16);
    CHECK(tileset.tile_height == 16);
    CHECK(tileset.spacing == 0);
    CHECK(tileset.margin == 0);
    CHECK(tileset.columns == 8);
    REQUIRE(tileset.image);
    CHECK(std::get <neutrino::image_from_disk>(tileset.image->source).source == "../demo-tileset.png");
    CHECK(tileset.image->width == 128);
    CHECK(tileset.image->height == 96);

    REQUIRE(world.layers().size() == 1);
    const auto& layer = tile_layer(world, 0);
    CHECK(layer.name == "simple_layer");
    CHECK(layer.width == 16);
    CHECK(layer.height == 16);
    REQUIRE(layer.cells.size() == 256);
    const std::vector <std::uint32_t> expected = {
        1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7, 0, 0,
        9, 10, 11, 12, 13, 14, 15, 9, 10, 11, 12, 13, 14, 15, 0, 0,
        17, 18, 19, 20, 21, 22, 23, 17, 18, 19, 20, 21, 22, 23, 0, 0,
        25, 26, 27, 28, 29, 30, 31, 25, 26, 27, 28, 29, 30, 31, 0, 0,
        33, 34, 35, 36, 37, 38, 39, 0, 0, 35, 36, 37, 38, 39, 0, 0,
        41, 42, 43, 44, 45, 46, 47, 0, 0, 43, 44, 45, 46, 47, 0, 0,
        1, 2, 3, 4, 0, 0, 0, 0, 0, 3, 4, 5, 6, 7, 0, 0,
        9, 10, 11, 12, 0, 0, 0, 0, 0, 11, 12, 13, 14, 15, 0, 0,
        17, 18, 19, 20, 21, 22, 23, 17, 18, 19, 20, 21, 22, 23, 0, 0,
        25, 26, 27, 28, 29, 30, 31, 25, 26, 27, 28, 29, 30, 31, 0, 0,
        33, 34, 35, 36, 37, 38, 39, 33, 34, 35, 36, 37, 38, 39, 0, 0,
        41, 42, 43, 44, 45, 46, 47, 41, 42, 43, 44, 45, 46, 47, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CHECK(layer.cells[i].gid == expected[i]);
    }
}

TEST_CASE("tmx loader parses tiled colors and json property value types") {
    const auto world = neutrino::load_tmx_world(R"(
        {
          "backgroundcolor": "#ffaa07ff",
          "height": 1,
          "layers": [],
          "orientation": "orthogonal",
          "properties": [
            {"name": "argb", "type": "color", "value": "#ffaa07ff"},
            {"name": "rgb", "type": "color", "value": "#aa07ff"},
            {"name": "plain_rgb", "type": "color", "value": "aa07ff"},
            {"name": "color", "type": "color", "value": "#ff268176"},
            {"name": "file_ref", "type": "file", "value": "../demo-tileset.png"},
            {"name": "hp", "type": "int", "value": 4},
            {"name": "is_player", "type": "bool", "value": true},
            {"name": "jump_force", "type": "float", "value": 10},
            {"name": "name", "type": "string", "value": "Mario"}
          ],
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )");

    CHECK(world.background_color() == sdlpp::color{170, 7, 255, 255});
    CHECK(has_property(world, "argb", sdlpp::color{170, 7, 255, 255}));
    CHECK(has_property(world, "rgb", sdlpp::color{170, 7, 255, 255}));
    CHECK(has_property(world, "plain_rgb", sdlpp::color{170, 7, 255, 255}));
    CHECK(has_property(world, "color", sdlpp::color{38, 129, 118, 255}));
    CHECK(has_property(world, "file_ref", std::filesystem::path{"../demo-tileset.png"}));
    CHECK(has_property(world, "hp", std::int64_t{4}));
    CHECK(has_property(world, "is_player", true));
    CHECK(has_property(world, "jump_force", 10.0f));
    CHECK(has_property(world, "name", std::string{"Mario"}));
}

TEST_CASE("tmx loader preserves json properties and object variants") {
    const auto world = neutrino::load_tmx_world(R"(
        {
          "backgroundcolor": "#656667",
          "height": 4,
          "layers": [
            {
              "draworder": "topdown",
              "name": "objects",
              "objects": [
                {
                  "gid": 5,
                  "height": 0,
                  "id": 1,
                  "name": "villager",
                  "properties": [{"name": "hp", "type": "int", "value": 12}],
                  "rotation": 90,
                  "type": "npc",
                  "visible": true,
                  "width": 0,
                  "x": 32,
                  "y": 32
                },
                {
                  "ellipse": true,
                  "height": 152,
                  "id": 13,
                  "visible": true,
                  "width": 248,
                  "x": 560,
                  "y": 808
                },
                {
                  "point": true,
                  "height": 0,
                  "id": 20,
                  "visible": true,
                  "width": 0,
                  "x": 220,
                  "y": 350
                },
                {
                  "height": 0,
                  "id": 15,
                  "polygon": [
                    {"x": 0, "y": 0},
                    {"x": 152, "y": 88},
                    {"x": 136, "y": -128},
                    {"x": 80, "y": -280},
                    {"x": 16, "y": -288}
                  ],
                  "visible": true,
                  "width": 0,
                  "x": -176,
                  "y": 432
                },
                {
                  "height": 0,
                  "id": 16,
                  "polyline": [
                    {"x": 0, "y": 0},
                    {"x": 248, "y": -32},
                    {"x": 376, "y": 72},
                    {"x": 544, "y": 288},
                    {"x": 656, "y": 120},
                    {"x": 512, "y": 0}
                  ],
                  "visible": true,
                  "width": 0,
                  "x": 240,
                  "y": 88
                },
                {
                  "height": 19,
                  "id": 15,
                  "text": {"text": "Hello World", "wrap": true},
                  "visible": true,
                  "width": 248,
                  "x": 48,
                  "y": 136
                }
              ],
              "opacity": 1,
              "type": "objectgroup",
              "visible": true
            }
          ],
          "orientation": "orthogonal",
          "properties": [
            {"name": "mapProperty1", "type": "one", "value": "string"},
            {"name": "mapProperty2", "type": "two", "value": "string"}
          ],
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "type": "map",
          "version": 1,
          "width": 4
        }
    )");

    CHECK(world.background_color() == sdlpp::color{101, 102, 103, 255});
    CHECK(has_property(
        world,
        "mapProperty1",
        neutrino::world_typed_string{"one", "string"}));
    CHECK(has_property(
        world,
        "mapProperty2",
        neutrino::world_typed_string{"two", "string"}));

    const auto& layer = object_layer(world, 0);
    REQUIRE(layer.objects.size() == 6);

    const auto* rectangle = std::get_if <neutrino::world_rectangle_object>(&layer.objects[0]);
    REQUIRE(rectangle);
    CHECK(rectangle->gid == 5);
    CHECK(rectangle->id == 1);
    CHECK(rectangle->name == "villager");
    CHECK(rectangle->rotation == doctest::Approx(90.0));
    CHECK(rectangle->type == "npc");
    CHECK(rectangle->origin.x == doctest::Approx(32.0));
    CHECK(rectangle->origin.y == doctest::Approx(32.0));
    CHECK(has_property(*rectangle, "hp", std::int64_t{12}));

    const auto* ellipse = std::get_if <neutrino::world_ellipse_object>(&layer.objects[1]);
    REQUIRE(ellipse);
    CHECK(ellipse->id == 13);
    CHECK(ellipse->width == doctest::Approx(248.0));
    CHECK(ellipse->height == doctest::Approx(152.0));
    CHECK(ellipse->origin.x == doctest::Approx(560.0));
    CHECK(ellipse->origin.y == doctest::Approx(808.0));

    const auto* point = std::get_if <neutrino::world_point_object>(&layer.objects[2]);
    REQUIRE(point);
    CHECK(point->id == 20);
    CHECK(point->origin.x == doctest::Approx(220.0));
    CHECK(point->origin.y == doctest::Approx(350.0));

    const auto* polygon = std::get_if <neutrino::world_polygon_object>(&layer.objects[3]);
    REQUIRE(polygon);
    REQUIRE(polygon->points.size() == 5);
    check_point(polygon->points[2], 136.0f, -128.0f);
    check_point(polygon->points[4], 16.0f, -288.0f);

    const auto* polyline = std::get_if <neutrino::world_polyline_object>(&layer.objects[4]);
    REQUIRE(polyline);
    REQUIRE(polyline->points.size() == 6);
    check_point(polyline->points[4], 656.0f, 120.0f);
    check_point(polyline->points[5], 512.0f, 0.0f);

    const auto* text = std::get_if <neutrino::world_text_object>(&layer.objects[5]);
    REQUIRE(text);
    CHECK(text->id == 15);
    CHECK(text->wrap);
    CHECK(text->text == "Hello World");
}

TEST_CASE("tmx loader composes grouped layer state") {
    const auto world = neutrino::load_tmx_world(R"(
        {
          "height": 1,
          "layers": [
            {
              "id": 1,
              "name": "hidden parent",
              "offsetx": 3.5,
              "offsety": 4.25,
              "opacity": 0.5,
              "parallaxx": 0.5,
              "parallaxy": 0.25,
              "type": "group",
              "visible": false,
              "layers": [
                {
                  "data": [1],
                  "height": 1,
                  "id": 2,
                  "name": "child",
                  "offsetx": 7.25,
                  "offsety": -1.5,
                  "opacity": 0.5,
                  "parallaxx": 0.5,
                  "parallaxy": 0.5,
                  "type": "tilelayer",
                  "visible": true,
                  "width": 1
                }
              ]
            }
          ],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )");

    REQUIRE(world.layers().size() == 1);
    const auto& layer = tile_layer(world, 0);
    CHECK(layer.name == "child");
    CHECK(!layer.visible);
    CHECK(layer.opacity == doctest::Approx(0.25));
    CHECK(layer.offset.x == doctest::Approx(10.75));
    CHECK(layer.offset.y == doctest::Approx(2.75));
    CHECK(layer.parallax_x == doctest::Approx(0.25));
    CHECK(layer.parallax_y == doctest::Approx(0.125));
    REQUIRE(layer.cells.size() == 1);
    CHECK(layer.cells[0].gid == 1);
}

TEST_CASE("tmx loader accepts flipped object gids") {
    const auto world = neutrino::load_tmx_world(R"(
        {
          "height": 1,
          "layers": [
            {
              "draworder": "topdown",
              "name": "objects",
              "objects": [
                {
                  "gid": 3221225477,
                  "height": 32,
                  "id": 1,
                  "visible": true,
                  "width": 32,
                  "x": 0,
                  "y": 0
                }
              ],
              "type": "objectgroup",
              "visible": true
            }
          ],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )");

    const auto& objects = object_layer(world, 0).objects;
    REQUIRE(objects.size() == 1);
    const auto* object = std::get_if <neutrino::world_rectangle_object>(&objects[0]);
    REQUIRE(object);
    CHECK(object->gid == 5);
    CHECK((object->flip & neutrino::sprite_flip::horizontal) == neutrino::sprite_flip::horizontal);
    CHECK((object->flip & neutrino::sprite_flip::vertical) == neutrino::sprite_flip::vertical);
    CHECK((object->flip & neutrino::sprite_flip::diagonal) == neutrino::sprite_flip::none);
}

TEST_CASE("tmx loader ignores tile authoring metadata (probability)") {
    // Probability is Tiled editor metadata; the loader accepts but does not parse
    // it, and still produces the tile.
    const auto world = neutrino::load_tmx_world(R"(
        {
          "height": 1,
          "layers": [],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [
            {
              "columns": 1,
              "firstgid": 1,
              "name": "weighted",
              "tilecount": 1,
              "tileheight": 32,
              "tiles": [
                {"id": 0, "probability": 0.5}
              ],
              "tilewidth": 32
            }
          ],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )");

    REQUIRE(world.tilesets().size() == 1);
    const auto* tile = world.tilesets()[0].tile(0);
    REQUIRE(tile);
    CHECK(tile->id == 0);
}

TEST_CASE("tmx loader accepts xml tile elements without gid") {
    const auto world = neutrino::load_tmx_world(R"(
        <map version="1.10" orientation="orthogonal" renderorder="right-down"
             width="2" height="2" tilewidth="32" tileheight="32">
          <layer id="1" name="tiles" width="2" height="2">
            <data>
              <tile gid="1"/>
              <tile/>
              <tile gid="2"/>
              <tile/>
            </data>
          </layer>
        </map>
    )");

    const auto& layer = tile_layer(world, 0);
    REQUIRE(layer.cells.size() == 4);
    CHECK(layer.cells[0].gid == 1);
    CHECK(layer.cells[1].gid == 0);
    CHECK(layer.cells[2].gid == 2);
    CHECK(layer.cells[3].gid == 0);
}

TEST_CASE("tmx loader reads tiled xml fixture with empty tile cells") {
    const auto world = load_world("test-rs/tiled_xml.tmx");
    const auto& layer = tile_layer(world, 0);
    CHECK(layer.width == 100);
    CHECK(layer.height == 100);
    REQUIRE(layer.cells.size() == 10000);
    CHECK(layer.cells[0].gid == 30);
    CHECK(layer.cells[9].gid == 30);
    CHECK(layer.cells[10].gid == 0);
}

TEST_CASE("tmx loader accepts json csv array tile data") {
    const auto world = neutrino::load_tmx_world(R"(
        {
          "height": 2,
          "layers": [
            {
              "data": [1, 2, 3, 4],
              "encoding": "csv",
              "height": 2,
              "name": "tiles",
              "type": "tilelayer",
              "visible": true,
              "width": 2
            }
          ],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "type": "map",
          "width": 2
        }
    )");

    const auto& layer = tile_layer(world, 0);
    REQUIRE(layer.cells.size() == 4);
    CHECK(layer.cells[0].gid == 1);
    CHECK(layer.cells[3].gid == 4);
}

TEST_CASE("tmx loader rejects tile data with the wrong cell count") {
    CHECK_THROWS_AS(neutrino::load_tmx_world(R"(
        <map version="1.10" orientation="orthogonal" renderorder="right-down"
             width="2" height="2" tilewidth="32" tileheight="32">
          <layer id="1" name="tiles" width="2" height="2">
            <data>
              <tile gid="1"/>
            </data>
          </layer>
        </map>
    )"), std::runtime_error);

    CHECK_THROWS_AS(neutrino::load_tmx_world(R"(
        {
          "height": 2,
          "layers": [
            {
              "data": "AQAAAA==",
              "encoding": "base64",
              "height": 2,
              "name": "tiles",
              "type": "tilelayer",
              "visible": true,
              "width": 2
            }
          ],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "type": "map",
          "width": 2
        }
    )"), std::runtime_error);
}

TEST_CASE("tmx loader reports json and floating-point parse errors consistently") {
    CHECK_THROWS_AS(neutrino::load_tmx_world(R"({"map": )"), std::runtime_error);

    CHECK_THROWS_AS(neutrino::load_tmx_world(R"(
        {
          "height": 1,
          "layers": [
            {
              "data": ["x"],
              "height": 1,
              "name": "bad",
              "type": "tilelayer",
              "visible": true,
              "width": 1
            }
          ],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )"), std::runtime_error);

    CHECK_THROWS_AS(neutrino::load_tmx_world(R"(
        <map version="1.10" orientation="orthogonal" renderorder="right-down"
             width="1" height="1" tilewidth="32" tileheight="32">
          <objectgroup id="1" name="objects">
            <object id="1" x="0" y="0">
              <polygon points="abc,1"/>
            </object>
          </objectgroup>
        </map>
    )"), std::runtime_error);
}

TEST_CASE("tmx loader json errors carry the failing element path") {
    try {
        static_cast <void>(neutrino::load_tmx_world(R"(
            {
              "map": {
                "height": 1,
                "layers": [
                  {
                    "data": [1, "x"],
                    "height": 1,
                    "name": "bad",
                    "type": "tilelayer",
                    "visible": true,
                    "width": 2
                  }
                ],
                "orientation": "orthogonal",
                "renderorder": "right-down",
                "tileheight": 32,
                "tilesets": [],
                "tilewidth": 32,
                "type": "map",
                "width": 2
              }
            }
        )"));
        FAIL("expected a TMX parse error");
    } catch (const std::runtime_error& e) {
        const std::string message = e.what();
        CHECK(message.find("TMX parse error") != std::string::npos);
        CHECK(message.find("/layers/0/data/1") != std::string::npos);
    }
}

TEST_CASE("tmx loader accepts utf8 bom documents and external tilesets") {
    const std::string bom = "\xef\xbb\xbf";
    const auto world = neutrino::load_tmx_world(bom + R"(
        {
          "height": 1,
          "layers": [],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )");
    CHECK(world.width() == 1);

    const auto external_world = neutrino::load_tmx_world(R"(
        <map version="1.10" orientation="orthogonal" renderorder="right-down"
             width="1" height="1" tilewidth="32" tileheight="32">
          <tileset firstgid="1" source="tiles.tsx"/>
        </map>
    )", [bom](std::string_view source) {
        CHECK(source == "tiles.tsx");
        return bom + R"(
            <tileset version="1.10" name="bom-tiles" tilewidth="32" tileheight="32"
                     tilecount="1" columns="1"/>
        )";
    });

    REQUIRE(external_world.tilesets().size() == 1);
    CHECK(external_world.tilesets()[0].name == "bom-tiles");
}

TEST_CASE("tmx loader rejects default external tileset path escapes") {
    const auto root = std::filesystem::temp_directory_path() / "neutrino_tmx_escape_test";
    const auto map_dir = root / "maps";
    std::filesystem::create_directories(map_dir);

    {
        std::ofstream map_file(map_dir / "map.tmx", std::ios::binary);
        REQUIRE(map_file);
        map_file << R"(
            <map version="1.10" orientation="orthogonal" renderorder="right-down"
                 width="1" height="1" tilewidth="32" tileheight="32">
              <tileset firstgid="1" source="../outside.tsx"/>
            </map>
        )";
    }
    {
        std::ofstream tileset_file(root / "outside.tsx", std::ios::binary);
        REQUIRE(tileset_file);
        tileset_file << R"(
            <tileset version="1.10" name="outside" tilewidth="32" tileheight="32"
                     tilecount="1" columns="1"/>
        )";
    }

    CHECK_THROWS_AS(neutrino::load_tmx_world_file(map_dir / "map.tmx"), std::runtime_error);
}

TEST_CASE("tmx loader accepts legacy json object-form properties and tiles") {
    const auto world = neutrino::load_tmx_world(R"(
        {
          "height": 1,
          "layers": [],
          "orientation": "orthogonal",
          "properties": {
            "legacy": "value",
            "typed": {"type": "int", "value": 7}
          },
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [
            {
              "columns": 1,
              "firstgid": 1,
              "name": "legacy-tiles",
              "tilecount": 1,
              "tileheight": 32,
              "tiles": {
                "0": {
                  "probability": 0.25,
                  "properties": {
                    "solid": "yes"
                  }
                }
              },
              "tilewidth": 32
            }
          ],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )");

    CHECK(has_property(world, "legacy", std::string{"value"}));
    CHECK(has_property(world, "typed", std::int64_t{7}));
    REQUIRE(world.tilesets().size() == 1);
    const auto* tile = world.tilesets()[0].tile(0);
    REQUIRE(tile);
    CHECK(has_property(*tile, "solid", std::string{"yes"}));
}

TEST_CASE("tmx loader ignores wangsets") {
    // Wang sets are Tiled autotiling metadata; the loader accepts the document
    // but does not carry them into the neutral world model.
    const auto world = neutrino::load_tmx_world(R"(
        {
          "height": 1,
          "layers": [],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [
            {
              "columns": 1,
              "firstgid": 1,
              "name": "wang",
              "tilecount": 1,
              "tileheight": 32,
              "tilewidth": 32,
              "wangsets": [
                {
                  "colors": [
                    {"color": "#ff0000", "name": "Red", "probability": 1, "tile": -1}
                  ],
                  "name": "wang",
                  "tile": -1,
                  "wangtiles": [
                    {"tileid": 268435457, "wangid": [1, 0, 1, 0, 1, 0, 1, 0]}
                  ]
                }
              ]
            }
          ],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )");

    REQUIRE(world.tilesets().size() == 1);
    CHECK(world.tilesets()[0].name == "wang"); // parsed fine, wang data simply dropped
}

TEST_CASE("world tileset rejects out of range tile rectangles") {
    neutrino::world_tileset tileset;
    tileset.tile_width = 32;
    tileset.tile_height = 32;
    tileset.tile_count = 1;
    tileset.columns = 1;
    tileset.image = neutrino::world_image{};
    tileset.image->width = 32;
    tileset.image->height = 32;

    CHECK_NOTHROW(static_cast <void>(tileset.tile_rect(0)));
    CHECK_THROWS_AS(static_cast <void>(tileset.tile_rect(1)), std::out_of_range);

    tileset.tile_count = 2;
    CHECK_THROWS_AS(static_cast <void>(tileset.tile_rect(1)), std::out_of_range);
}

TEST_CASE("world tileset converts global ids and reports ownership") {
    neutrino::world_tileset tileset;
    tileset.first_gid = 5;
    tileset.tile_count = 4;

    CHECK(tileset.to_local(5) == 0);
    CHECK(tileset.to_local(8) == 3);
    CHECK(tileset.to_global(3) == 8);
    CHECK_THROWS_AS(static_cast <void>(tileset.to_local(4)), std::out_of_range);
    CHECK_THROWS_AS(static_cast <void>(tileset.to_local(0)), std::out_of_range);

    CHECK(!tileset.contains(0));
    CHECK(!tileset.contains(4));
    CHECK(tileset.contains(5));
    CHECK(tileset.contains(8));
    CHECK(!tileset.contains(9));

    tileset.tile_count = 0;
    CHECK(tileset.contains(1000000));
    CHECK(!tileset.contains(4));
}

TEST_CASE("world resolves the owning tileset for a gid") {
    neutrino::world world;

    neutrino::world_tileset first;
    first.name = "first";
    first.first_gid = 1;
    first.tile_count = 4;
    world.add_tileset(std::move(first));

    neutrino::world_tileset second;
    second.name = "second";
    second.first_gid = 101;
    world.add_tileset(std::move(second));

    const auto owner_name = [&world](neutrino::world_tile_id gid) {
        const auto* tileset = world.tileset_for(gid);
        REQUIRE(tileset != nullptr);
        return tileset->name;
    };

    CHECK(world.tileset_for(0) == nullptr);
    CHECK(owner_name(1) == "first");
    CHECK(owner_name(4) == "first");
    CHECK(owner_name(100) == "first");
    CHECK(owner_name(101) == "second");
    CHECK(owner_name(50000) == "second");

    const neutrino::world empty;
    CHECK(empty.tileset_for(1) == nullptr);
}

TEST_CASE("tmx loader decodes hexagonal 120 degree rotation as degrees") {
    const auto world = neutrino::load_tmx_world(R"(
        <map version="1.10" orientation="hexagonal" renderorder="right-down"
             width="2" height="1" tilewidth="14" tileheight="12"
             hexsidelength="6" staggeraxis="y" staggerindex="odd">
          <tileset firstgid="1" name="hex" tilewidth="14" tileheight="12" tilecount="4" columns="2">
            <image source="hex.png" width="28" height="24"/>
          </tileset>
          <layer id="1" name="ground" width="2" height="1">
            <data encoding="csv">268435457,4026531841</data>
          </layer>
          <objectgroup id="2" name="objects">
            <object id="1" gid="268435458" x="0" y="12" width="14" height="12"/>
          </objectgroup>
        </map>
    )");

    const auto& layer = tile_layer(world, 0);
    REQUIRE(layer.cells.size() == 2);

    CHECK(layer.cells[0].gid == 1);
    CHECK(layer.cells[0].flip == neutrino::sprite_flip::none);
    CHECK(layer.cells[0].rotation_degrees == 120.0f);

    CHECK(layer.cells[1].gid == 1);
    CHECK(layer.cells[1].horizontally_flipped());
    CHECK(layer.cells[1].vertically_flipped());
    CHECK(layer.cells[1].diagonally_flipped());
    CHECK(layer.cells[1].rotation_degrees == 120.0f);

    const auto& objects = object_layer(world, 0);
    REQUIRE(objects.objects.size() == 1);
    const auto& object = std::get <neutrino::world_rectangle_object>(objects.objects[0]);
    CHECK(object.gid == 2);
    CHECK(object.rotation == doctest::Approx(120.0)); // hex-120 folded into object rotation
}

TEST_CASE("tmx loader preserves tiled documentation layer and chunk examples") {
    const auto world = neutrino::load_tmx_world(R"(
        {
          "backgroundcolor": "#656667",
          "compressionlevel": 2,
          "height": 4,
          "layers": [
            {
              "data": [1, 2, 1, 2, 3, 1, 3, 1, 2, 2, 3, 3, 4, 4, 4, 1],
              "height": 4,
              "name": "ground",
              "opacity": 1,
              "properties": [
                {"name": "tileLayerProp", "type": "int", "value": 1}
              ],
              "tintcolor": "#656667",
              "type": "tilelayer",
              "visible": true,
              "width": 4,
              "x": 0,
              "y": 0
            },
            {
              "draworder": "topdown",
              "height": 8,
              "name": "people",
              "objects": [],
              "opacity": 1,
              "properties": [
                {"name": "layerProp1", "type": "string", "value": "someStringValue"}
              ],
              "type": "objectgroup",
              "visible": true,
              "width": 4,
              "x": 0,
              "y": 0
            }
          ],
          "nextobjectid": 1,
          "orientation": "orthogonal",
          "properties": [
            {"name": "mapProperty1", "type": "one", "value": "string"},
            {"name": "mapProperty2", "type": "two", "value": "string"}
          ],
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "version": 1,
          "tiledversion": "1.0.3",
          "type": "map",
          "width": 4
        }
    )");

    CHECK(world.background_color() == sdlpp::color{101, 102, 103, 255});
    CHECK(world.width() == 4);
    CHECK(world.height() == 4);
    CHECK(world.orientation() == neutrino::world_orientation::orthogonal);
    CHECK(world.render_order() == neutrino::world_render_order::right_down);
    CHECK(world.tile_width() == 32);
    CHECK(world.tile_height() == 32);
    CHECK(has_property(world, "mapProperty1", neutrino::world_typed_string{"one", "string"}));
    CHECK(has_property(world, "mapProperty2", neutrino::world_typed_string{"two", "string"}));

    const auto& ground = tile_layer(world, 0);
    REQUIRE(ground.cells.size() == 16);
    CHECK(ground.width == 4);
    CHECK(ground.height == 4);
    CHECK(ground.name == "ground");
    CHECK(ground.opacity == doctest::Approx(1.0));
    CHECK(ground.visible);
    CHECK(ground.offset.x == doctest::Approx(0.0));
    CHECK(ground.offset.y == doctest::Approx(0.0));
    REQUIRE(ground.tint);
    CHECK(*ground.tint == sdlpp::color{101, 102, 103, 255});
    CHECK(has_property(ground, "tileLayerProp", std::int64_t{1}));
    const std::vector <std::uint32_t> expected_cells = {
        1, 2, 1, 2,
        3, 1, 3, 1,
        2, 2, 3, 3,
        4, 4, 4, 1
    };
    for (std::size_t i = 0; i < expected_cells.size(); ++i) {
        CHECK(ground.cells[i].gid == expected_cells[i]);
    }

    const auto& people = object_layer(world, 0);
    CHECK(people.draw_order == neutrino::world_object_draw_order::top_down);
    CHECK(people.name == "people");
    CHECK(people.opacity == doctest::Approx(1.0));
    CHECK(people.visible);
    CHECK(people.objects.empty());
    CHECK(has_property(people, "layerProp1", std::string{"someStringValue"}));

    const auto chunk_world = neutrino::load_tmx_world(R"(
        {
          "height": 1,
          "infinite": true,
          "layers": [
            {
              "chunks": [
                {
                  "data": [1, 2, 1, 2, 3, 1, 3, 1, 2, 2, 3, 3, 4, 4, 4, 1],
                  "height": 4,
                  "width": 4,
                  "x": 4,
                  "y": -16
                }
              ],
              "height": 1,
              "name": "chunks",
              "type": "tilelayer",
              "visible": true,
              "width": 1
            }
          ],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )");

    const auto& chunks = tile_layer(chunk_world, 0);
    REQUIRE(chunks.chunks.size() == 1);
    CHECK(chunks.chunks[0].height == 4);
    CHECK(chunks.chunks[0].width == 4);
    CHECK(chunks.chunks[0].x == 4);
    CHECK(chunks.chunks[0].y == -16);
    REQUIRE(chunks.chunks[0].cells.size() == 16);
}

TEST_CASE("tmx loader preserves tiled documentation tileset tile properties") {
    const auto world = neutrino::load_tmx_world(R"(
        {
          "height": 1,
          "layers": [],
          "orientation": "orthogonal",
          "renderorder": "right-down",
          "tileheight": 32,
          "tilesets": [
            {
              "columns": 19,
              "firstgid": 1,
              "image": "../image/fishbaddie_parts.png",
              "objectalignment": "bottomleft",
              "imageheight": 480,
              "imagewidth": 640,
              "margin": 3,
              "name": "",
              "properties": [
                {"name": "myProperty1", "type": "string", "value": "myProperty1_value"}
              ],
              "spacing": 1,
              "tilecount": 266,
              "tileheight": 32,
              "tilewidth": 32,
              "tiles": [
                {
                  "id": 11,
                  "properties": [
                    {"name": "myProperty2", "type": "string", "value": "myProperty2_value"}
                  ],
                  "terrain": [0, 1, 0, 1]
                }
              ],
              "wangsets": [
                {
                  "colors": [
                    {"color": "#ff0000", "name": "Red", "probability": 1, "tile": -1},
                    {"color": "#00ff00", "name": "Green", "probability": 1, "tile": -1},
                    {"color": "#0000ff", "name": "Blue", "probability": 1, "tile": -1},
                    {"color": "#ff7700", "name": "Orange", "probability": 1, "tile": -1}
                  ],
                  "name": "FirstWang",
                  "properties": [
                    {"name": "floating_wang", "type": "float", "value": 14.6},
                    {"name": "is_wang", "type": "bool", "value": true}
                  ],
                  "tile": -1,
                  "wangtiles": [
                    {"dflip": false, "hflip": false, "tileid": 0, "vflip": false,
                     "wangid": [3, 0, 1, 0, 1, 0, 3, 0]},
                    {"dflip": false, "hflip": false, "tileid": 1, "vflip": false,
                     "wangid": [1, 0, 1, 0, 1, 0, 1, 0]},
                    {"dflip": false, "hflip": false, "tileid": 2, "vflip": false,
                     "wangid": [3, 0, 3, 0, 1, 0, 1, 0]},
                    {"dflip": false, "hflip": false, "tileid": 3, "vflip": false,
                     "wangid": [3, 0, 1, 0, 1, 0, 1, 0]},
                    {"dflip": false, "hflip": false, "tileid": 4, "vflip": false,
                     "wangid": [2, 0, 2, 0, 1, 0, 1, 0]},
                    {"dflip": false, "hflip": false, "tileid": 8, "vflip": false,
                     "wangid": [1, 0, 1, 0, 3, 0, 3, 0]},
                    {"dflip": false, "hflip": false, "tileid": 9, "vflip": false,
                     "wangid": [2, 0, 1, 0, 1, 0, 1, 0]},
                    {"dflip": false, "hflip": false, "tileid": 10, "vflip": false,
                     "wangid": [1, 0, 3, 0, 3, 0, 1, 0]},
                    {"dflip": false, "hflip": false, "tileid": 16, "vflip": false,
                     "wangid": [3, 0, 3, 0, 3, 0, 3, 0]},
                    {"dflip": false, "hflip": false, "tileid": 17, "vflip": false,
                     "wangid": [3, 0, 3, 0, 3, 0, 3, 0]},
                    {"dflip": false, "hflip": false, "tileid": 18, "vflip": false,
                     "wangid": [3, 0, 3, 0, 1, 0, 1, 0]}
                  ]
                }
              ]
            }
          ],
          "tilewidth": 32,
          "type": "map",
          "width": 1
        }
    )");

    REQUIRE(world.tilesets().size() == 1);
    const auto& tileset = world.tilesets()[0];
    CHECK(tileset.columns == 19);
    CHECK(tileset.first_gid == 1);
    CHECK(tileset.name.empty());
    CHECK(tileset.margin == 3);
    CHECK(tileset.spacing == 1);
    CHECK(tileset.tile_count == 266);
    CHECK(tileset.tile_width == 32);
    CHECK(tileset.tile_height == 32);
    CHECK(has_property(tileset, "myProperty1", std::string{"myProperty1_value"}));
    REQUIRE(tileset.image);
    CHECK(std::holds_alternative <neutrino::image_from_disk>(tileset.image->source)); // external file, not embedded
    CHECK(tileset.image->width == 640);
    CHECK(tileset.image->height == 480);
    CHECK(std::get <neutrino::image_from_disk>(tileset.image->source).source == "../image/fishbaddie_parts.png");

    const auto* tile = tileset.tile(11);
    REQUIRE(tile);
    CHECK(tile->id == 11);
    CHECK(has_property(*tile, "myProperty2", std::string{"myProperty2_value"}));
    // Terrain / wang authoring metadata is intentionally not carried into the model.
}

TEST_CASE("tmx loader reads infinite maps and chunks") {
    for (const auto& file : {
             "test-rs/tiled_base64_zlib_infinite.tmx",
             "test-rs/js_tiled_base64_zlib_infinite.json"
         }) {
        const auto world = load_world(file, resolver_for("test-rs"));
        REQUIRE(world.infinite());
        auto layers = world.tile_layers();
        REQUIRE(layers.size() == 3);
        const auto& first = *layers[0];
        REQUIRE(first.chunks.size() == 4);
        const auto& chunk = first.chunks[0];
        CHECK(chunk.width == 32);
        CHECK(chunk.height == 32);
        CHECK(chunk.x == -32);
        CHECK(chunk.y == 0);
        CHECK(chunk.cells.size() == static_cast <std::size_t>(chunk.width * chunk.height));
    }
}

TEST_CASE("tmx loader reads image layers") {
    for (const auto& file : {
             "test-rs/tiled_image_layers.tmx",
             "test-rs/js_tiled_image_layers.json"
         }) {
        const auto world = load_world(file);
        REQUIRE(world.image_layers().size() == 2);
        CHECK(image_layer(world, 0).name == "Image Layer 1");
        CHECK(!image_layer(world, 0).image);
        const auto& second = image_layer(world, 1);
        CHECK(second.name == "Image Layer 2");
        REQUIRE(second.image);
        CHECK(std::get <neutrino::image_from_disk>(second.image->source).source == "tilesheet.png");
        if (second.image->width != 0) {
            CHECK(second.image->width == 448);
        }
        if (second.image->height != 0) {
            CHECK(second.image->height == 192);
        }
    }
}

TEST_CASE("tmx loader reads properties animations groups and text objects") {
    for (const auto& file : {
             "test4/example.tmx",
             "test4/js_example.json"
         }) {
        const auto world = load_world(file);
        CHECK(has_property(world, "BigInteger", std::int64_t{999999999}));
        CHECK(has_property(world, "EmptyProperty", std::string{}));
        CHECK(has_property(world, "FalseProperty", false));
        CHECK(has_property(world, "FileProperty", std::filesystem::path{"torches.png"}));
        CHECK(has_property(world, "FloatProperty", 0.12f));
        CHECK(has_property(world, "IntProperty", std::int64_t{1234}));
        CHECK(has_property(world, "NegativeFloatProperty", -0.12f));
        CHECK(has_property(world, "NegativeIntProperty", std::int64_t{-1234}));
        CHECK(has_property(world, "StringProperty", std::string{"A string value"}));
        CHECK(has_property(world, "TrueProperty", true));
        CHECK(has_property(world, "YellowProperty", sdlpp::color{255, 255, 0, 255}));

        REQUIRE(world.tilesets().size() == 2);
        const auto& tileset = world.tilesets()[1];
        CHECK(tileset.first_gid == 49);
        REQUIRE(tileset.image);
        CHECK(tileset.image->width == 96);
        CHECK(tileset.image->height == 32);
        const auto* tile = tileset.tile(0);
        REQUIRE(tile);
        REQUIRE(tile->objects);
        REQUIRE(!tile->objects->objects.empty());
        const auto* object = std::get_if <neutrino::world_rectangle_object>(&tile->objects->objects[0]);
        REQUIRE(object);
        CHECK(object->width == doctest::Approx(3.875));
        CHECK(object->height == doctest::Approx(8.0));
        CHECK(object->origin.x == doctest::Approx(14.0));
        CHECK(object->origin.y == doctest::Approx(16.875));
        REQUIRE(tile->animation.size() == 4);
        CHECK(tile->animation[0].duration == std::chrono::milliseconds{200});
        CHECK(tile->animation[0].tile == 0);

        bool found_group_child = false;
        const auto tile_layers = world.tile_layers();
        for (const auto* layer : tile_layers) {
            if (layer->name == "Testing Child Tile Layer") {
                found_group_child = true;
                CHECK(layer->width == 10);
                CHECK(layer->height == 10);
                CHECK(layer->offset.x == doctest::Approx(11.0));
                CHECK(layer->offset.y == doctest::Approx(2.0));
            }
        }
        CHECK(found_group_child);
    }

    const auto text_world = load_world("test3/b64zlib.tmx");
    const auto& objects = object_layer(text_world, 0).objects;
    REQUIRE(objects.size() == 5);
    const auto* text = std::get_if <neutrino::world_text_object>(&objects[4]);
    REQUIRE(text);
    CHECK(text->text == "Hello World");
    CHECK(text->bold);
    CHECK(text->italic);
    CHECK(text->wrap);
    CHECK(text->color == sdlpp::color{255, 0, 0, 255});
    CHECK(text->id == 5);
}

TEST_CASE("tmx loader reads complex example objects and tilesets") {
    const auto world = load_world("test1/example.tmx");
    CHECK(world.tile_width() == 32);
    CHECK(world.tile_height() == 32);
    CHECK(world.width() == 10);
    CHECK(world.height() == 10);
    CHECK(world.background_color() == sdlpp::color{89, 89, 89, 255});
    CHECK(has_property(world, "name", std::string{"Test"}));
    CHECK(has_property(world, "test", std::string{"foo"}));

    REQUIRE(world.tilesets().size() == 2);
    const auto& first_tileset = world.tilesets()[0];
    CHECK(first_tileset.first_gid == 1);
    CHECK(first_tileset.name == "marioenemies");
    CHECK(first_tileset.tile_width == 32);
    CHECK(first_tileset.tile_height == 32);
    REQUIRE(first_tileset.tile(0));
    CHECK(first_tileset.tile(0)->id == 0);
    CHECK(has_property(*first_tileset.tile(0), "wall", std::string{"true"}));
    REQUIRE(first_tileset.tile(1));
    CHECK(first_tileset.tile(1)->id == 1);
    CHECK(has_property(*first_tileset.tile(1), "wall", std::string{"true"}));

    const auto& second_tileset = world.tilesets()[1];
    CHECK(second_tileset.first_gid == 101);
    CHECK(second_tileset.name == "marioobjects");
    CHECK(second_tileset.tile_width == 32);
    CHECK(second_tileset.tile_height == 32);
    REQUIRE(second_tileset.image);
    CHECK(std::get <neutrino::image_from_disk>(second_tileset.image->source).source == "../flappyman/docs/marioobjects.png");
    CHECK(second_tileset.image->width == 576);
    CHECK(second_tileset.image->height == 336);

    const std::vector <std::uint32_t> expected_ids = {
        101, 100, 280, 1, 1, 0, 0, 0, 0, 0, 1, 101, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0
    };
    const auto& first_layer = tile_layer(world, 0);
    REQUIRE(first_layer.cells.size() == expected_ids.size());
    for (std::size_t i = 0; i < expected_ids.size(); ++i) {
        CHECK(first_layer.cells[i].gid == expected_ids[i]);
    }

    auto object_layers = world.object_layers();
    REQUIRE(object_layers.size() == 2);
    CHECK(object_layers[1]->name == "TestObject2");
    CHECK(object_layers[1]->objects.empty());
    CHECK(object_layers[0]->name == "TestObject");
    CHECK(object_layers[0]->draw_order == neutrino::world_object_draw_order::top_down);
    CHECK(object_layers[0]->color == sdlpp::color{160, 160, 164, 255});
    CHECK(has_property(*object_layers[0], "objectlayerprop", std::string{"666"}));

    REQUIRE(object_layers[0]->objects.size() == 4);
    const auto* rectangle = std::get_if <neutrino::world_rectangle_object>(&object_layers[0]->objects[0]);
    REQUIRE(rectangle);
    CHECK(rectangle->id == 1);
    CHECK(rectangle->origin.x == doctest::Approx(7.0));
    CHECK(rectangle->origin.y == doctest::Approx(11.0));
    CHECK(rectangle->width == doctest::Approx(51.0));
    CHECK(rectangle->height == doctest::Approx(51.0));
    CHECK(has_property(*rectangle, "asquare", std::string{"test"}));

    const auto* polygon = std::get_if <neutrino::world_polygon_object>(&object_layers[0]->objects[2]);
    REQUIRE(polygon);
    REQUIRE(polygon->points.size() == 15);
    check_point(polygon->points[0], 0.0f, 0.0f);
    check_point(polygon->points[1], 147.0f, -21.0f);
    check_point(polygon->points[2], 110.0f, -118.0f);
    check_point(polygon->points[3], -77.0f, -85.0f);
    check_point(polygon->points[4], -136.0f, 92.0f);
    check_point(polygon->points[5], -34.0f, 189.0f);
    check_point(polygon->points[6], 111.0f, 86.0f);
    check_point(polygon->points[7], 1.0f, 401.0f);
    check_point(polygon->points[8], -355.0f, 137.0f);
    check_point(polygon->points[9], -266.0f, -87.0f);
    check_point(polygon->points[10], -228.0f, -145.0f);
    check_point(polygon->points[11], -72.0f, -170.0f);
    check_point(polygon->points[12], 103.0f, -156.0f);
    check_point(polygon->points[13], 149.0f, -137.0f);
    check_point(polygon->points[14], 702.0f, 101.0f);
}

TEST_CASE("tmx loader reads isometric staggered maps with a tileset grid") {
    for (const auto& file : {
             "wang/isometric_staggered_grass_and_water.tmx",
             "wang/js_isometric_staggered_grass_and_water.json"
         }) {
        const auto world = load_world(file);
        CHECK(world.orientation() == neutrino::world_orientation::staggered);
        CHECK(world.render_order() == neutrino::world_render_order::right_down);
        CHECK(world.infinite());
        CHECK(world.stagger_axis() == neutrino::world_stagger_axis::y);
        CHECK(world.stagger_index() == neutrino::world_stagger_index::odd);
        REQUIRE(world.tilesets().size() == 1);

        const auto& tileset = world.tilesets()[0];
        REQUIRE(tileset.grid);
        CHECK(!tileset.grid->orthogonal);
        CHECK(tileset.grid->width == 64);
        CHECK(tileset.grid->height == 32);
        // Wang sets in these fixtures are authoring metadata, not carried into the model.
    }
}

TEST_CASE("tmx loader derives columns/tilecount from the image when omitted") {
    // Older / hand-authored maps (e.g. some hexagonal examples) omit columns and
    // tilecount on a uniform tileset; the loader must derive them from the image or
    // the tileset builds no tiles.
    const auto world = neutrino::load_tmx_world(R"(<?xml version="1.0"?>
<map version="1.0" orientation="orthogonal" width="2" height="2" tilewidth="16" tileheight="16" infinite="0">
 <tileset firstgid="1" name="t" tilewidth="16" tileheight="16">
  <image source="x.png" width="64" height="48"/>
 </tileset>
 <layer id="1" name="L" width="2" height="2"><data encoding="csv">1,2,3,4</data></layer>
</map>)");

    REQUIRE(world.tilesets().size() == 1);
    CHECK(world.tilesets()[0].columns == 4);     // 64 / 16
    CHECK(world.tilesets()[0].tile_count == 12); // 4 columns * (48/16 = 3) rows
}
