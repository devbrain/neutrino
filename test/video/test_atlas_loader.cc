#include <doctest/doctest.h>

#include <neutrino/video/sprite/atlas_loader.hh>
#include <neutrino/video/sprite/sprite_cache.hh>
#include <neutrino/video/sprite/sprite_def.hh>

#include "test_application.hh"
#include "video/test_images.hh"

#include <sdlpp/video/surface.hh>

#include <utility>
#include <variant>

using namespace neutrino;
using namespace neutrino::test;

namespace {
    // A real Aseprite `--format json-array` export: a 32x32 sheet, four 16px frames (frame
    // 1 trimmed), and two frame tags -> clips.
    constexpr const char* aseprite_json = R"({
      "frames": [
        {"filename":"hero 0","frame":{"x":0,"y":0,"w":16,"h":16},"trimmed":false,
         "spriteSourceSize":{"x":0,"y":0,"w":16,"h":16},"sourceSize":{"w":16,"h":16},"duration":100},
        {"filename":"hero 1","frame":{"x":16,"y":0,"w":12,"h":10},"trimmed":true,
         "spriteSourceSize":{"x":2,"y":3,"w":12,"h":10},"sourceSize":{"w":16,"h":16},"duration":120},
        {"filename":"hero 2","frame":{"x":0,"y":16,"w":16,"h":16},"trimmed":false,
         "spriteSourceSize":{"x":0,"y":0,"w":16,"h":16},"sourceSize":{"w":16,"h":16},"duration":90},
        {"filename":"hero 3","frame":{"x":16,"y":16,"w":16,"h":16},"trimmed":false,
         "spriteSourceSize":{"x":0,"y":0,"w":16,"h":16},"sourceSize":{"w":16,"h":16},"duration":90}
      ],
      "meta": {"image":"hero.png","size":{"w":32,"h":32},"frameTags":[
        {"name":"walk","from":0,"to":2,"direction":"forward"},
        {"name":"idle","from":3,"to":3,"direction":"forward"}
      ]}
    })";

}

TEST_SUITE("neutrino::video atlas_loader") {
    TEST_CASE("load_aseprite_atlas parses frames, trim, and tag clips") {
        const sprite_def def = load_aseprite_atlas(aseprite_json);

        // Image source + declared size.
        REQUIRE(std::holds_alternative <image_from_disk>(def.image.source));
        CHECK(std::get <image_from_disk>(def.image.source).source == "hero.png");
        CHECK(def.image.width == 32);
        CHECK(def.image.height == 32);

        // Frames -> named "0".."3".
        REQUIRE(def.visuals.size() == 4);
        CHECK(def.visuals[0].name == "0");
        CHECK(def.visuals[0].src.x == 0);
        CHECK(def.visuals[0].src.w == 16);
        CHECK_FALSE(def.visuals[0].trim_offset.has_value());

        // Frame 1 is trimmed: src is the packed (12x10) rect; trim/source carry the rest.
        CHECK(def.visuals[1].src.w == 12);
        REQUIRE(def.visuals[1].trim_offset.has_value());
        CHECK(def.visuals[1].trim_offset->x == 2);
        CHECK(def.visuals[1].trim_offset->y == 3);
        REQUIRE(def.visuals[1].source_size.has_value());
        CHECK(def.visuals[1].source_size->width == 16);
        CHECK(def.visuals[1].source_size->height == 16);

        // Tags -> clips over frame indices, with per-frame durations.
        REQUIRE(def.clips.size() == 2);
        CHECK(def.clips[0].name == "walk");
        REQUIRE(def.clips[0].frames.size() == 3);        // frames 0,1,2
        CHECK(def.clips[0].frames[0].visual == "0");
        CHECK(def.clips[0].frames[1].visual == "1");
        CHECK(def.clips[0].frames[1].duration.count() == doctest::Approx(120.0f));
        CHECK(def.clips[1].name == "idle");
        CHECK(def.clips[1].frames.size() == 1);          // frame 3
        CHECK(def.clips[1].frames[0].visual == "3");
    }

    TEST_CASE("a loaded atlas builds through the cache with no hand code") {
        neutrino::test::test_application app("atlas_loader build");
        sprite_cache cache;

        // Export -> def -> (swap the disk image for a self-contained generated one) -> build.
        sprite_def def = load_aseprite_atlas(aseprite_json);
        def.image = bmp_image(32, 32);

        const sprite_set_handle set = cache.acquire(def);
        REQUIRE(set.valid());
        CHECK(set.visual("0").has_value());
        CHECK(set.visual("3").has_value());
        CHECK(set.clip("walk").has_value());
        CHECK(set.clip("idle").has_value());

        sprite_instance hero = set.spawn("walk");
        CHECK(hero.valid());
        CHECK(hero.switch_to("idle"));
    }
}
