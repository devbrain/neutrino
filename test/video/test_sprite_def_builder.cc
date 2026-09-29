#include <doctest/doctest.h>

#include <chrono>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <neutrino/video/sprite/atlas_loader.hh>
#include <neutrino/video/sprite/sprite_cache.hh>
#include <neutrino/video/sprite/sprite_def.hh>
#include <neutrino/video/sprite/sprite_def_builder.hh>

#include "test_application.hh"
#include "video/test_images.hh"

using namespace neutrino;
using namespace neutrino::test;
using namespace std::chrono_literals;

namespace {
    constexpr const char* test_aseprite_json = R"({
      "frames": [
        {"filename":"hero 0","frame":{"x":0,"y":0,"w":16,"h":16},"trimmed":false,
         "spriteSourceSize":{"x":0,"y":0,"w":16,"h":16},"sourceSize":{"w":16,"h":16},"duration":100},
        {"filename":"hero 1","frame":{"x":16,"y":0,"w":12,"h":10},"trimmed":true,
         "spriteSourceSize":{"x":2,"y":3,"w":12,"h":10},"sourceSize":{"w":16,"h":16},"duration":120},
        {"filename":"hero 2","frame":{"x":0,"y":16,"w":16,"h":16},"trimmed":false,
         "spriteSourceSize":{"x":0,"y":0,"w":16,"h":16},"sourceSize":{"w":16,"h":16},"duration":90}
      ],
      "meta": {"image":"hero_atlas.png","size":{"w":32,"h":32},"frameTags":[
        {"name":"walk","from":0,"to":1,"direction":"forward"},
        {"name":"idle","from":2,"to":2,"direction":"forward"}
      ]}
    })";
}

TEST_SUITE("neutrino::video sprite_def_builder") {
    TEST_CASE("builder configures image from disk file, size, and transparent color") {
        const sprite_def def = sprite_def_builder()
            .from_file("player.png", 64, 32)
            .transparent_color(sdlpp::color{255, 0, 255, 255})
            .build();

        REQUIRE(std::holds_alternative <image_from_disk>(def.image.source));
        CHECK(std::get <image_from_disk>(def.image.source).source == "player.png");
        CHECK(def.image.width == 64);
        CHECK(def.image.height == 32);
        REQUIRE(def.image.transparent_color.has_value());
        CHECK(def.image.transparent_color->r == 255);
        CHECK(def.image.transparent_color->g == 0);
        CHECK(def.image.transparent_color->b == 255);
    }

    TEST_CASE("builder configures image from memory buffers (vector, span, string_view)") {
        const std::vector <std::uint8_t> dummy_bytes = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

        SUBCASE("vector overload") {
            const sprite_def def = sprite_def_builder()
                .from_memory(dummy_bytes, 128, 64)
                .build();

            REQUIRE(std::holds_alternative <image_from_memory>(def.image.source));
            CHECK(std::get <image_from_memory>(def.image.source).bytes == dummy_bytes);
            CHECK(def.image.width == 128);
            CHECK(def.image.height == 64);
        }

        SUBCASE("span overload") {
            const std::span <const std::uint8_t> sp(dummy_bytes.data(), dummy_bytes.size());
            const sprite_def def = sprite_def_builder()
                .from_memory(sp, 64, 64)
                .build();

            REQUIRE(std::holds_alternative <image_from_memory>(def.image.source));
            CHECK(std::get <image_from_memory>(def.image.source).bytes == dummy_bytes);
            CHECK(def.image.width == 64);
            CHECK(def.image.height == 64);
        }

        SUBCASE("string_view overload") {
            const std::string_view sv(reinterpret_cast <const char*>(dummy_bytes.data()), dummy_bytes.size());
            const sprite_def def = sprite_def_builder()
                .from_memory(sv, 32, 32)
                .build();

            REQUIRE(std::holds_alternative <image_from_memory>(def.image.source));
            CHECK(std::get <image_from_memory>(def.image.source).bytes == dummy_bytes);
            CHECK(def.image.width == 32);
            CHECK(def.image.height == 32);
        }
    }

    TEST_CASE("builder configures image from stream") {
        const std::string raw = "fake_encoded_image_bytes_from_resource_stream";
        std::stringstream ss(raw);

        const sprite_def def = sprite_def_builder()
            .from_stream(ss, 48, 48)
            .build();

        REQUIRE(std::holds_alternative <image_from_memory>(def.image.source));
        const auto& bytes = std::get <image_from_memory>(def.image.source).bytes;
        CHECK(bytes.size() == raw.size());
        CHECK(std::string(bytes.begin(), bytes.end()) == raw);
        CHECK(def.image.width == 48);
        CHECK(def.image.height == 48);
    }

    TEST_CASE("builder configures image from surface and automatically adopts dimensions") {
        auto surf = sdlpp::surface::create_rgb(40, 20, sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surf.has_value());

        const sprite_def def = sprite_def_builder()
            .from_surface(std::move(*surf), /*identity=*/42)
            .build();

        REQUIRE(std::holds_alternative <image_from_surface>(def.image.source));
        const auto& s = std::get <image_from_surface>(def.image.source);
        REQUIRE(s.pixels != nullptr);
        CHECK(s.identity == 42);
        CHECK(def.image.width == 40);
        CHECK(def.image.height == 20);
    }

    TEST_CASE("builder configures grid slicing and clears it") {
        sprite_def_builder builder;
        builder.from_file("tiles.png")
            .with_grid(16, 16, sprite_origin_rule::bottom_center, /*margin=*/2, /*spacing=*/1, /*columns=*/4, /*count=*/8);

        sprite_def def = builder.build();
        REQUIRE(def.grid.has_value());
        CHECK(def.grid->cell_w == 16);
        CHECK(def.grid->cell_h == 16);
        CHECK(def.grid->origin == sprite_origin_rule::bottom_center);
        CHECK(def.grid->margin == 2);
        CHECK(def.grid->spacing == 1);
        CHECK(def.grid->columns == 4);
        CHECK(def.grid->count == 8);

        builder.clear_grid();
        def = builder.build();
        CHECK_FALSE(def.grid.has_value());
    }

    TEST_CASE("builder adds visual definitions and trimmed visuals") {
        const sprite_def def = sprite_def_builder()
            .from_file("atlas.png")
            .add_visual("idle", rect{0, 0, 16, 16}, point{8, 16})
            .add_visual("jump", rect{16, 0, 16, 16}, sprite_origin_rule::center)
            .add_trimmed_visual("attack", rect{32, 2, 12, 10}, point{8, 16}, dim{16, 16}, point{2, 3})
            .build();

        REQUIRE(def.visuals.size() == 3);
        CHECK(def.visuals[0].name == "idle");
        CHECK(def.visuals[0].src == rect{0, 0, 16, 16});
        CHECK(def.visuals[0].origin == point{8, 16});
        CHECK_FALSE(def.visuals[0].source_size.has_value());

        CHECK(def.visuals[1].name == "jump");
        CHECK(def.visuals[1].src == rect{16, 0, 16, 16});
        CHECK(def.visuals[1].origin == point{8, 8}); // center of 16x16

        CHECK(def.visuals[2].name == "attack");
        CHECK(def.visuals[2].src == rect{32, 2, 12, 10});
        CHECK(def.visuals[2].origin == point{8, 16});
        REQUIRE(def.visuals[2].source_size.has_value());
        CHECK(def.visuals[2].source_size->width == 16);
        CHECK(def.visuals[2].source_size->height == 16);
        REQUIRE(def.visuals[2].trim_offset.has_value());
        CHECK(def.visuals[2].trim_offset->x == 2);
        CHECK(def.visuals[2].trim_offset->y == 3);
    }

    TEST_CASE("builder configures clips with fluent sub-builder and convenience methods") {
        const sprite_def def = sprite_def_builder()
            .from_file("sheet.png")
            // 1. Fluent sub-builder
            .clip("walk", /*loop=*/true)
                .frame("0", 100ms)
                .frame("1", 120ms, sprite_flip::horizontal)
                .frame("2", 100ms)
                .end_clip()
            // 2. Convenience by visual names
            .add_clip("run", {"run_0", "run_1"}, 80ms, /*loop=*/true)
            // 3. Convenience by grid indices
            .add_clip("jump", {3, 4}, 150ms, /*loop=*/false)
            // 4. Convenience by grid range [first, first + count)
            .add_clip_range("fall", 5, 3, 90ms, /*loop=*/true)
            .build();

        REQUIRE(def.clips.size() == 4);

        // walk
        CHECK(def.clips[0].name == "walk");
        CHECK(def.clips[0].loop == true);
        REQUIRE(def.clips[0].frames.size() == 3);
        CHECK(def.clips[0].frames[0].visual == "0");
        CHECK(def.clips[0].frames[0].duration.count() == doctest::Approx(100.0f));
        CHECK(def.clips[0].frames[0].flip == sprite_flip::none);
        CHECK(def.clips[0].frames[1].visual == "1");
        CHECK(def.clips[0].frames[1].duration.count() == doctest::Approx(120.0f));
        CHECK(def.clips[0].frames[1].flip == sprite_flip::horizontal);

        // run
        CHECK(def.clips[1].name == "run");
        REQUIRE(def.clips[1].frames.size() == 2);
        CHECK(def.clips[1].frames[0].visual == "run_0");
        CHECK(def.clips[1].frames[1].visual == "run_1");

        // jump
        CHECK(def.clips[2].name == "jump");
        CHECK(def.clips[2].loop == false);
        REQUIRE(def.clips[2].frames.size() == 2);
        CHECK(def.clips[2].frames[0].visual == "3");
        CHECK(def.clips[2].frames[1].visual == "4");

        // fall
        CHECK(def.clips[3].name == "fall");
        REQUIRE(def.clips[3].frames.size() == 3);
        CHECK(def.clips[3].frames[0].visual == "5");
        CHECK(def.clips[3].frames[1].visual == "6");
        CHECK(def.clips[3].frames[2].visual == "7");
    }

    TEST_CASE("builder validate catches invalid configurations") {
        SUBCASE("missing image source throws") {
            sprite_def_builder builder;
            CHECK_THROWS_AS(builder.build(), std::runtime_error);
        }

        SUBCASE("clip with empty name throws") {
            sprite_def_builder builder;
            builder.from_file("a.png").clip("", true).frame("0", 100ms).end_clip();
            CHECK_THROWS_AS(builder.build(), std::runtime_error);
        }

        SUBCASE("clip with no frames throws") {
            sprite_def_builder builder;
            builder.from_file("a.png").clip("empty_clip", true).end_clip();
            CHECK_THROWS_AS(builder.build(), std::runtime_error);
        }

        SUBCASE("clip with non-positive duration throws") {
            sprite_def_builder builder;
            builder.from_file("a.png").clip("bad_duration", true).frame("0", 0ms).end_clip();
            CHECK_THROWS_AS(builder.build(), std::runtime_error);
        }
    }

    TEST_CASE("resource file / archive workflow with Aseprite JSON") {
        const std::vector <std::uint8_t> mock_png_bytes = {1, 2, 3, 4, 5};

        SUBCASE("load_aseprite_metadata into pre-configured memory image") {
            const sprite_def def = sprite_def_builder()
                .from_memory(mock_png_bytes)
                .load_aseprite_metadata(test_aseprite_json)
                .build();

            // Preserves memory image source!
            REQUIRE(std::holds_alternative <image_from_memory>(def.image.source));
            CHECK(std::get <image_from_memory>(def.image.source).bytes == mock_png_bytes);
            CHECK(def.image.width == 32);
            CHECK(def.image.height == 32);
            CHECK(def.visuals.size() == 3);
            CHECK(def.clips.size() == 2);
        }

        SUBCASE("from_aseprite_json with explicit image bytes") {
            const sprite_def def = sprite_def_builder::from_aseprite_json(
                test_aseprite_json, mock_png_bytes).build();

            REQUIRE(std::holds_alternative <image_from_memory>(def.image.source));
            CHECK(std::get <image_from_memory>(def.image.source).bytes == mock_png_bytes);
            CHECK(def.visuals.size() == 3);
            CHECK(def.clips.size() == 2);
        }

        SUBCASE("from_aseprite_json with resource archive resolver callback") {
            // Simulate reading an archive where hero_atlas.png is stored alongside the metadata
            bool resolved_hero_atlas = false;
            auto archive_resolver = [&] (std::string_view name) -> std::vector <std::uint8_t> {
                if (name == "hero_atlas.png") {
                    resolved_hero_atlas = true;
                    return mock_png_bytes;
                }
                return {};
            };

            const sprite_def def = sprite_def_builder::from_aseprite_json(
                test_aseprite_json, archive_resolver).build();

            CHECK(resolved_hero_atlas);
            REQUIRE(std::holds_alternative <image_from_memory>(def.image.source));
            CHECK(std::get <image_from_memory>(def.image.source).bytes == mock_png_bytes);
        }

        SUBCASE("atlas_loader overloads using memory bytes and resolver") {
            // Test load_aseprite_atlas overloads
            const sprite_def def1 = load_aseprite_atlas(test_aseprite_json, mock_png_bytes);
            REQUIRE(std::holds_alternative <image_from_memory>(def1.image.source));
            CHECK(std::get <image_from_memory>(def1.image.source).bytes == mock_png_bytes);

            const sprite_def def2 = load_aseprite_atlas(
                test_aseprite_json,
                [&] (std::string_view name) {
                    CHECK(name == "hero_atlas.png");
                    return mock_png_bytes;
                });
            REQUIRE(std::holds_alternative <image_from_memory>(def2.image.source));
            CHECK(std::get <image_from_memory>(def2.image.source).bytes == mock_png_bytes);
        }
    }

    TEST_CASE("end-to-end integration: sprite_def_builder through sprite_cache") {
        neutrino::test::test_application app("builder cache test");
        sprite_cache cache;

        const world_image img = bmp_image(32, 32);
        REQUIRE(std::holds_alternative <image_from_memory>(img.source));
        const auto& bmp_bytes = std::get <image_from_memory>(img.source).bytes;

        const sprite_def def = sprite_def_builder()
            .from_memory(bmp_bytes, 32, 32)
            .with_grid(16, 16, sprite_origin_rule::center)
            .add_clip_range("spin", 0, 4, 100ms, /*loop=*/true)
            .build();

        const sprite_set_handle set = cache.acquire(def);
        REQUIRE(set.valid());
        CHECK(set.visual_count() == 4);
        CHECK(set.visual("0").has_value());
        CHECK(set.clip("spin").has_value());

        sprite_instance inst = set.spawn("spin");
        REQUIRE(inst.valid());
        CHECK(inst.state().valid());
    }
}
