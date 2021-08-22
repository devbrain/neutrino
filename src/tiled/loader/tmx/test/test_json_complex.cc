#include <string>
#include <doctest/doctest.h>
#include "test_utils.hh"
#include "tiled/loader/tmx/json_reader.hh"
#include "tiled/loader/tmx/tile_layer.hh"
#include "tiled/loader/tmx/object_layer.hh"
#include "tiled/loader/tmx/object.hh"
#include "tiled/loader/tmx/wang_set.hh"
#include <iostream>

using namespace neutrino::tiled::tmx;

TEST_CASE("Parse a Map from Tiled's documentation")
{
    const std::string txt = R"(
            {
              "backgroundcolor": "#656667",
              "compressionlevel": 2,
              "height": 4,
              "layers": [],
              "nextobjectid": 1,
              "orientation": "orthogonal",
              "properties": [
                {
                  "name": "mapProperty1",
                  "type": "one",
                  "value": "string"
                },
                {
                  "name": "mapProperty2",
                  "type": "two",
                  "value": "string"
                }
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
    )";

    auto the_map = test::load_map(txt);

    REQUIRE(the_map.background_color() == "#656667");
    REQUIRE(the_map.width() == 4);
    REQUIRE(the_map.height() == 4);

    REQUIRE(the_map.orientation() == orientation_t::ORTHOGONAL);
    REQUIRE(the_map.render_order() == render_order_t::RIGHT_DOWN);
    REQUIRE(the_map.tile_width() == 32);
    REQUIRE(the_map.tile_height() == 32);

    REQUIRE(the_map.version() == "1");
    REQUIRE(the_map.get("mapProperty1") == test::to_prop(typed_string{"one", "string"}));
    REQUIRE(the_map.get("mapProperty2") == test::to_prop(typed_string{"two", "string"}));
}

TEST_CASE("Parse a Layer from Tiled's documentation - read simple values")
{
    SUBCASE("Layer - Tile Layer format")
    {
        const std::string txt = R"(
            {
              "data": [1, 2, 1, 2, 3, 1, 3, 1, 2, 2, 3, 3, 4, 4, 4, 1 ],
              "height": 4,
              "name": "ground",
              "opacity": 1,
              "properties": [
                {
                  "name": "tileLayerProp",
                  "type": "int",
                  "value": 1
                }
              ],
              "tintcolor": "#656667",
              "type": "tilelayer",
              "visible": true,
              "width": 4,
              "x": 0,
              "y": 0
            }
        )";
        auto tl = tile_layer::parse(json_reader::load(txt.c_str(), txt.size(), nullptr),
                                    nullptr);


        REQUIRE(tl.cells().size() == 16);
        REQUIRE(tl.width() == 4);
        REQUIRE(tl.height() == 4);
        REQUIRE(tl.name() == "ground");
        REQUIRE(tl.opacity() == 1);

        REQUIRE(tl.visible());
        REQUIRE(tl.offset_x() == 0);
        REQUIRE(tl.offset_y() == 0);
        REQUIRE(tl.tint() == "#656667");
        REQUIRE(tl.get("tileLayerProp") == test::to_prop((int) 1));
    }

    SUBCASE("Layer - Object Layer format")
    {
        const std::string txt = R"(
            {
              "draworder": "topdown",
              "height": 8,
              "name": "people",
              "objects": [],
              "opacity": 1,
              "properties": [
                {
                  "name": "layerProp1",
                  "type": "string",
                  "value": "someStringValue"
                }
              ],
              "type": "objectgroup",
              "visible": true,
              "width": 4,
              "x": 0,
              "y": 0
            }
)";
        auto tl = object_layer::parse(json_reader::load(txt.c_str(), txt.size(), nullptr),
                                      nullptr);
        REQUIRE(tl.draw_order() == draw_order_t::TOP_DOWN);
        REQUIRE(tl.name() == "people");
        REQUIRE(tl.opacity() == 1);
        REQUIRE(tl.visible());
        REQUIRE(tl.objects().empty());
        REQUIRE(tl.get("layerProp1") == test::to_prop(std::string{"someStringValue"}));
    }
}

TEST_CASE("Parse a Chunk from Tiled's documentation - read simple values")
{
    const std::string txt = R"(
    {
        "data":[1, 2, 1, 2, 3, 1, 3, 1, 2, 2, 3, 3, 4, 4, 4, 1],
        "height":16,
        "width":16,
        "x":4,
        "y":-16
    }
    )";
    auto c = chunk::parse(json_reader::load(txt.c_str(), txt.size(), nullptr), "", "");
    REQUIRE(c.height() == 16);
    REQUIRE(c.width() == 16);
    REQUIRE(c.x() == 4);
    REQUIRE(c.y() == -16);
    REQUIRE(c.cells().size() == 16);
}

template<typename T>
static T parse(const std::string& txt)
{
    auto objp = parse_object(json_reader::load(txt.c_str(), txt.size(), nullptr));
    const auto* obj = std::get_if<T>(&objp);
    REQUIRE(obj != nullptr);
    return *obj;
}

TEST_CASE("Parse an Object from Tiled's documentation - read simple values")
{
    SUBCASE("Object - regular")
    {
        auto obj = parse<object>(R"({
          "gid": 5,
          "height": 0,
          "id": 1,
          "name": "villager",
          "properties": [
            {
              "name": "hp",
              "type": "int",
              "value": 12
            }
          ],
          "rotation": 90,
          "type": "npc",
          "visible": true,
          "width": 0,
          "x": 32,
          "y": 32
        })");

        REQUIRE(obj.gid() == 5);
        REQUIRE(obj.id() == 1);
        REQUIRE(obj.name() == "villager");
        REQUIRE(obj.rotation() == 90);
        REQUIRE(obj.type() == "npc");
        REQUIRE(obj.visible());
        REQUIRE(obj.width() == 0);
        REQUIRE(obj.height() == 0);
        REQUIRE(obj.origin() == neutrino::math::point2f(32, 32));
        REQUIRE(obj.get("hp") == test::to_prop(12));
    }

    SUBCASE("Object - ellipse")
    {
        auto obj = parse<ellipse>(R"(
            {
                "ellipse":true,
                "height":152,
                "id":13,
                "name":"",
                "rotation":0,
                "type":"",
                "visible":true,
                "width":248,
                "x":560,
                "y":808
            }
        )");

        REQUIRE(obj.id() == 13);
        REQUIRE(obj.name().empty());
        REQUIRE(obj.rotation() == 0);
        REQUIRE(obj.type().empty());
        REQUIRE(obj.visible());
        REQUIRE(obj.width() == 248);
        REQUIRE(obj.height() == 152);
        REQUIRE(obj.origin() == neutrino::math::point2f{560, 808});
    }

    SUBCASE("Object - point")
    {
        auto obj = parse<point>(R"(
            {
                "point":true,
                "height":0,
                "id":20,
                "name":"",
                "rotation":0,
                "type":"",
                "visible":true,
                "width":0,
                "x":220,
                "y":350
                }
        )");
        REQUIRE(obj.id() == 20);
        REQUIRE(obj.name().empty());
        REQUIRE(obj.rotation() == 0);
        REQUIRE(obj.type().empty());
        REQUIRE(obj.visible());
        REQUIRE(obj.width() == 0);
        REQUIRE(obj.height() == 0);
        REQUIRE(obj.origin() == neutrino::math::point2f{220, 350});
    }

    SUBCASE("Object - polygon")
    {
        auto obj = parse<polygon>(R"(
        {
            "height":0,
            "id":15,
            "name":"",
            "polygon":
                [
                    {
                        "x":0,
                        "y":0
                    },
                    {
                        "x":152,
                        "y":88
                    },
                    {
                        "x":136,
                        "y":-128
                    },
                    {
                        "x":80,
                        "y":-280
                    },
                    {
                        "x":16,
                        "y":-288
                    }
                ],
            "rotation":0,
            "type":"",
            "visible":true,
            "width":0,
            "x":-176,
            "y":432
            }
        )");

        REQUIRE(obj.id() == 15);
        REQUIRE(obj.name().empty());
        REQUIRE(obj.rotation() == 0);
        REQUIRE(obj.type().empty());
        REQUIRE(obj.visible());
        REQUIRE(obj.width() == 0);
        REQUIRE(obj.height() == 0);
        REQUIRE(obj.origin() == neutrino::math::point2f{-176, 432});
        REQUIRE(obj.points().size() == 5);
        REQUIRE(obj.points()[2] == neutrino::math::point2f{136, -128});
    }

    SUBCASE("Object - polyline")
    {
        auto obj = parse<polyline>(R"(
            {
            "height":0,
            "id":16,
            "name":"",
            "polyline":[
                {
                    "x":0,
                    "y":0
                },
                {
                    "x":248,
                    "y":-32
                },
                {
                    "x":376,
                    "y":72
                },
                {
                    "x":544,
                    "y":288
                },
                {
                    "x":656,
                    "y":120
                },
                {
                    "x":512,
                    "y":0
                }
            ],
            "rotation":0,
            "type":"",
            "visible":true,
            "width":0,
            "x":240,
            "y":88
            }
        )");
        REQUIRE(obj.id() == 16);
        REQUIRE(obj.name().empty());
        REQUIRE(obj.rotation() == 0);
        REQUIRE(obj.type().empty());
        REQUIRE(obj.visible());
        REQUIRE(obj.width() == 0);
        REQUIRE(obj.height() == 0);
        REQUIRE(obj.origin() == neutrino::math::point2f{240, 88});
        REQUIRE(obj.points().size() == 6);
        REQUIRE(obj.points()[4] == neutrino::math::point2f{656, 120});
    }

    SUBCASE("Object - text")
    {
        auto obj = parse<text>(R"(
            {
                "height":19,
                "id":15,
                "name":"",
                "text":
                        {
                            "text":"Hello World",
                            "wrap":true
                        },
                "rotation":0,
                "type":"",
                "visible":true,
                "width":248,
                "x":48,
                "y":136
                }
        )");
        REQUIRE(obj.id() == 15);
        REQUIRE(obj.name().empty());
        REQUIRE(obj.rotation() == 0);
        REQUIRE(obj.type().empty());
        REQUIRE(obj.visible());
        REQUIRE(obj.width() == 248);
        REQUIRE(obj.height() == 19);
        REQUIRE(obj.origin() == neutrino::math::point2f{48, 136});
        REQUIRE(obj.wrap());
        REQUIRE(obj.data() == "Hello World");
    }
}

TEST_CASE("Parse a Tileset from Tiled's documentation - read simple values")
{
    const std::string txt = R"(
        {
            "columns":19,
            "firstgid":1,
            "image":"..\/image\/fishbaddie_parts.png",
            "objectalignment":"bottomleft",
            "imageheight":480,
            "imagewidth":640,
            "margin":3,
            "name":"",
            "properties":[
                {
                    "name":"myProperty1",
                    "type":"string",
                    "value":"myProperty1_value"
                }
            ],
            "spacing":1,
            "tilecount":266,
            "tileheight":32,
            "tilewidth":32
            }
    )";

    auto ts = tile_set::parse(json_reader::load(txt.c_str(), txt.size(), nullptr), test::null_resolver);
    REQUIRE(ts.columns() == 19);
    REQUIRE(ts.first_gid() == 1);
    REQUIRE(ts.name().empty());
    REQUIRE(ts.margin() == 3);
    REQUIRE(ts.spacing() == 1);
    REQUIRE(ts.tile_count() == 266);
    REQUIRE(ts.tile_width() == 32);
    REQUIRE(ts.tile_height() == 32);
    REQUIRE(ts.get("myProperty1") == test::to_prop(std::string("myProperty1_value")));
    const auto* img = ts.get_image();
    REQUIRE(img);
    REQUIRE(img->data().empty());
    REQUIRE(img->width() == 640);
    REQUIRE(img->height() == 480);
    REQUIRE(img->source() == "../image/fishbaddie_parts.png");
}

TEST_CASE("Property-tests - Set properties from json")
{
    const std::string txt = R"(
    {
            "properties" : [
                    {
                        "name":"color",
                        "type":"color",
                        "value":"#ff268176"
                    },
                    {
                        "name":"file_ref",
                        "type":"file",
                        "value":"..\/demo-tileset.png"
                    },
                    {
                        "name":"hp",
                        "type":"int",
                        "value":4
                    },
                    {
                        "name":"is_player",
                        "type":"bool",
                        "value":true
                    },
                    {
                        "name":"jump_force",
                        "type":"float",
                        "value":10
                    },
                    {
                        "name":"name",
                        "type":"string",
                        "value":"Mario"
                    }
        ]}
    )";
    component c;
    component::parse(c, json_reader::load(txt.c_str(), txt.size(), nullptr));

    REQUIRE(c.get("color") == test::to_prop(colori("#ff268176")));
    REQUIRE(c.get("file_ref") == test::to_prop(std::filesystem::path("../demo-tileset.png")));
    REQUIRE(c.get("hp") == test::to_prop((int) 4));
    REQUIRE(c.get("is_player") == test::to_prop(true));
    REQUIRE(c.get("jump_force") == test::to_prop(10.0f));
    REQUIRE(c.get("name") == test::to_prop(std::string{"Mario"}));
}

TEST_CASE("Parse a Tile from Tiled's documentation - read simple values")
{
    const std::string txt = R"(
        {
            "id":11,
            "properties":[
            {
                "name":"myProperty2",
                "type":"string",
                "value":"myProperty2_value"
            }],
            "terrain":[0, 1, 0, 1]
        }
    )";
    const auto tl = tile::parse(json_reader::load(txt.c_str(), txt.size(), nullptr));
    REQUIRE (tl.id() == 11);
    REQUIRE(tl.get("myProperty2") == test::to_prop(std::string("myProperty2_value")));
    REQUIRE (tl.terrain()[0] == 0);
    REQUIRE (tl.terrain()[1] == 1);
    REQUIRE (tl.terrain()[2] == 0);
    REQUIRE (tl.terrain()[3] == 1);
}

TEST_CASE("Wang-tests - everything Wang - simple")
{
    const std::string txt = R"(
            {
                "colors":[
                {
                    "color":"#ff0000",
                    "name":"Red",
                    "probability":1,
                    "tile":-1
                },
                {
                    "color":"#00ff00",
                    "name":"Green",
                    "probability":1,
                    "tile":-1
                },
                {
                    "color":"#0000ff",
                    "name":"Blue",
                    "probability":1,
                    "tile":-1
                },
                {
                    "color":"#ff7700",
                    "name":"Orange",
                    "probability":1,
                    "tile":-1
                }],
                "name":"FirstWang",
                "properties":[
                    {
                        "name":"floating_wang",
                        "type":"float",
                        "value":14.6
                    },
                    {
                        "name":"is_wang",
                        "type":"bool",
                        "value":true
                    }],
                "tile":-1,
                "wangtiles":[
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":0,
                        "vflip":false,
                        "wangid":[3, 0, 1, 0, 1, 0, 3, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":1,
                        "vflip":false,
                        "wangid":[1, 0, 1, 0, 1, 0, 1, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":2,
                        "vflip":false,
                        "wangid":[3, 0, 3, 0, 1, 0, 1, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":3,
                        "vflip":false,
                        "wangid":[3, 0, 1, 0, 1, 0, 1, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":4,
                        "vflip":false,
                        "wangid":[2, 0, 2, 0, 1, 0, 1, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":8,
                        "vflip":false,
                        "wangid":[1, 0, 1, 0, 3, 0, 3, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":9,
                        "vflip":false,
                        "wangid":[2, 0, 1, 0, 1, 0, 1, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":10,
                        "vflip":false,
                        "wangid":[1, 0, 3, 0, 3, 0, 1, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":16,
                        "vflip":false,
                        "wangid":[3, 0, 3, 0, 3, 0, 3, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":17,
                        "vflip":false,
                        "wangid":[3, 0, 3, 0, 3, 0, 3, 0]
                    },
                    {
                        "dflip":false,
                        "hflip":false,
                        "tileid":18,
                        "vflip":false,
                        "wangid":[3, 0, 3, 0, 1, 0, 1, 0]
                    }]
            }
        )";

    wang_set ws = wang_set::parse(json_reader::load(txt.c_str(), txt.size(), nullptr));
    REQUIRE(ws.name() == "FirstWang");
    REQUIRE(ws.local_tile() == -1);
    REQUIRE(ws.colors().size() == 4);
    REQUIRE(ws.get("is_wang") == test::to_prop(true));

    const auto& c = ws.colors()[3];
    REQUIRE(c.color() == "#ff7700");
    REQUIRE(c.name() == "Orange");
    REQUIRE(c.prob() == 1);
    REQUIRE(c.local_tile() == -1);

    REQUIRE(ws.tiles().size() == 11);
    const auto& t = ws.tiles()[10];

    REQUIRE(!t.diag_flipped());
    REQUIRE(!t.hor_flipped());
    REQUIRE(!t.vert_flipped());
    REQUIRE(t.gid() == 18);

    const auto& w = t.wang_id();

    REQUIRE(w[wang_tile::TOP] == 3);
    REQUIRE(w[wang_tile::TOP_RIGHT] == 0);
    REQUIRE(w[wang_tile::RIGHT] == 3);
    REQUIRE(w[wang_tile::BOTTOM_RIGHT] == 0);
    REQUIRE(w[wang_tile::BOTTOM] == 1);
    REQUIRE(w[wang_tile::BOTTOM_LEFT] == 0);
    REQUIRE(w[wang_tile::LEFT] == 1);
    REQUIRE(w[wang_tile::TOP_LEFT] == 0);
}

#if 0

TEST_CASE( "Parse an Object from Tiled's documentation - read simple values", "[tiled][object]" )
{

    SECTION("Object - template")
    {
        nlohmann::json j = "{\n"
                           "    \"id\":13,\n"
                           "    \"template\":\"useless_template_object.tx\",\n"
                           "    \"x\":104,\n"
                           "    \"y\":34\n"
                           "}"_json;

        tson::Object obj;
        std::unique_ptr<tson::IJson> json = std::make_unique<tson::NlohmannJson>(&j);
        bool parseOk = obj.parse(*json);
        bool hasCorrectValues = (
                obj.getId() == 13 &&
                obj.getTemplate() == "useless_template_object.tx" &&
                obj.getPosition() == tson::Vector2i(104, 34) &&
                obj.getObjectType() == tson::ObjectType::Template
        );

        REQUIRE((parseOk && hasCorrectValues));
    }
}

TEST_CASE( "Parse a Frame", "[tiled][frame]" )
{
    nlohmann::json j = "{\n"
                       "  \"duration\":100,\n"
                       "  \"tileid\":6\n"
                       "}"_json;

    tson::Frame frame;
    std::unique_ptr<tson::IJson> json = std::make_unique<tson::NlohmannJson>(&j);
    bool parseOk = frame.parse(*json);
    bool hasCorrectValues = (
            frame.getDuration() == 100 &&
            frame.getTileId() == 7
    );

    REQUIRE((parseOk && hasCorrectValues));
}

TEST_CASE( "Parse a Terrain from Tiled's documentation", "[tiled][terrain]" )
{
    nlohmann::json j = "{\n"
                       "  \"name\":\"chasm\",\n"
                       "  \"tile\":12\n"
                       "}"_json;

    tson::Terrain terrain;
    std::unique_ptr<tson::IJson> json = std::make_unique<tson::NlohmannJson>(&j);
    bool parseOk = terrain.parse(*json);
    bool hasCorrectValues = (
            terrain.getName() == "chasm" &&
            terrain.getTile() == 12
    );

    REQUIRE((parseOk && hasCorrectValues));
}





#endif