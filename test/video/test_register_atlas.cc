//
// register_atlas: the filter reaches every page texture, defaults to linear, and changes how a
// picture is sampled when drawn scaled; an indexed bank with a colour key reaches the GPU with
// the key transparent, its palette colours intact, and tint multiplying them.
//
#include <doctest/doctest.h>

#include <neutrino/video/atlas/image_atlas.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/image/image_builder.hh>
#include <neutrino/video/palette.hh>
#include <neutrino/video/sprite/texture_atlas.hh>
#include <neutrino/video/texture.hh>

#include "test_application.hh"
#include "image_atlas_access.hh"
#include "services/service_access.hh"
#include "video/sprite/texture_registry.hh"

#include <sdlpp/video/texture.hh>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>

using namespace neutrino;

namespace {
    image solid_rgba(dim size, color c) {
        auto b = rgba_image_builder(size);
        for (int y = 0; y < size.height; y++) {
            for (int x = 0; x < size.width; x++) {
                b.set(x, y, c);
            }
        }
        return std::move(b).build();
    }

    // Two 8x8 pictures under a 10x10 page cap (8 + 2 gutter): one picture per page.
    image_atlas two_page_atlas() {
        image_bank bank;
        bank.entries.push_back({solid_rgba({8, 8}, colors::red), 0});
        bank.entries.push_back({solid_rgba({8, 8}, colors::blue), 1});
        return build_atlas(bank, std::nullopt, dim{10, 10});
    }

    sdlpp::scale_mode page_scale_mode(atlas_id atlas, std::size_t picture) {
        const auto& page = require_texture_registry().get(atlas, picture).first;
        const auto mode = page.native().get_scale_mode();
        REQUIRE(mode.has_value());
        return *mode;
    }

    bool is_one_of(const color& c, const color& a, const color& b) {
        const auto same = [&](const color& o) { return c.r == o.r && c.g == o.g && c.b == o.b; };
        return same(a) || same(b);
    }

    // Draw a 2x1 red|blue picture stretched to 20x2 and report whether every pixel of the
    // stretched row is exactly red or blue (no blended texels).
    bool scaled_picture_keeps_texels(texture_filter filter) {
        auto b = rgba_image_builder({2, 1});
        b.set(0, 0, colors::red);
        b.set(1, 0, colors::blue);
        image_bank bank;
        bank.entries.push_back({std::move(b).build(), 0});
        const atlas_id atlas = register_atlas(build_atlas(bank), atlas_texture_format::rgba, filter);

        auto target = texture::create(dim{24, 4}, texture_access::target);
        REQUIRE(target.has_value());
        bool keeps = true;
        {
            const auto bound = target->bind(colors::black);
            const auto [page, src] = require_texture_registry().get(atlas, 0);
            texture_draw_params params;
            params.src = src;
            draw_texture(page, render_rect{0.0f, 0.0f, 20.0f, 2.0f}, params);
            const image frame = capture_frame();
            for (int x = 0; x < 20; x++) {
                const auto px = frame.get_pixel(x, 0);
                REQUIRE(px.has_value());
                keeps = keeps && is_one_of(*px, colors::red, colors::blue);
            }
        }
        unregister_atlas(atlas);
        return keeps;
    }

    // An indexed "glyph" font: index 0 is the transparent key (a loud magenta, so a leak shows),
    // 1 and 2 are the authored colours.
    constexpr std::uint8_t key = 0;
    constexpr color key_colour{255, 0, 255, 255};
    constexpr color ink_a{200, 100, 50, 255};
    constexpr color ink_b{40, 160, 220, 255};
    constexpr color background{0, 128, 0, 255};

    palette glyph_palette() {
        const std::array <std::uint8_t, 9> rgb{key_colour.r, key_colour.g, key_colour.b,
                                               ink_a.r, ink_a.g, ink_a.b,
                                               ink_b.r, ink_b.g, ink_b.b};
        return load_palette(rgb, palette_encoding::rgb8);
    }

    // 4x4 glyph: a keyed one-pixel border around a 2x2 centre, left column ink_a, right ink_b.
    // The keyed border is what build_atlas extrudes into the gutter.
    image keyed_glyph(const palette& pal) {
        auto b = indexed_image_builder({4, 4}, pal, key);
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                const bool centre = x >= 1 && x <= 2 && y >= 1 && y <= 2;
                b.set(x, y, centre ? static_cast <std::uint8_t>(x) : key);
            }
        }
        return std::move(b).build();
    }

    image_atlas keyed_glyph_atlas() {
        const palette pal = glyph_palette();
        image_bank bank;
        bank.entries.push_back({keyed_glyph(pal), 0});
        bank.entries.push_back({keyed_glyph(pal), 1});
        return build_atlas(bank);
    }

    bool same_rgb(const color& c, const color& expected, int tolerance = 0) {
        const auto near = [&](std::uint8_t a, std::uint8_t b) { return std::abs(int{a} - int{b}) <= tolerance; };
        return near(c.r, expected.r) && near(c.g, expected.g) && near(c.b, expected.b);
    }

    // Draw picture 0 of @p atlas at x2 into a 12x12 target cleared to the background, with
    // @p tint, and return the frame. The picture covers (0, 0)..(8, 8).
    image draw_glyph_x2(atlas_id atlas, const color& tint) {
        auto target = texture::create(dim{12, 12}, texture_access::target);
        REQUIRE(target.has_value());
        const auto bound = target->bind(background);
        const auto [page, src] = require_texture_registry().get(atlas, 0);
        texture_draw_params params;
        params.src = src;
        params.tint = tint;
        draw_texture(page, render_rect{0.0f, 0.0f, 8.0f, 8.0f}, params);
        return capture_frame();
    }

    color frame_pixel(const image& frame, int x, int y) {
        const auto px = frame.get_pixel(x, y);
        REQUIRE(px.has_value());
        return *px;
    }

    // The keyed border (2 px wide at x2) and everything past the picture show the background;
    // the centre shows the palette colours multiplied by @p tint.
    void check_keyed_glyph(const image& frame, const color& tint) {
        const auto scaled = [&](const color& c) {
            return color{static_cast <std::uint8_t>(c.r * tint.r / 255), static_cast <std::uint8_t>(c.g * tint.g / 255),
                         static_cast <std::uint8_t>(c.b * tint.b / 255), 255};
        };
        for (int y = 0; y < 12; y++) {
            for (int x = 0; x < 12; x++) {
                const color px = frame_pixel(frame, x, y);
                CAPTURE(x);
                CAPTURE(y);
                const bool centre = x >= 2 && x <= 5 && y >= 2 && y <= 5;
                if (!centre) {
                    CHECK(same_rgb(px, background));
                } else {
                    CHECK(same_rgb(px, scaled(x <= 3 ? ink_a : ink_b), 1));
                }
            }
        }
    }
}

TEST_SUITE("neutrino::video register_atlas filter") {
    TEST_CASE("pages are linear-filtered by default") {
        neutrino::test::test_application app("atlas filter default");
        auto cpu = two_page_atlas();
        REQUIRE(atlas_test::page_count(cpu) == 2);
        const atlas_id atlas = register_atlas(std::move(cpu));
        CHECK(page_scale_mode(atlas, 0) == sdlpp::scale_mode::linear);
        CHECK(page_scale_mode(atlas, 1) == sdlpp::scale_mode::linear);
        unregister_atlas(atlas);
    }

    TEST_CASE("the requested filter reaches every page") {
        neutrino::test::test_application app("atlas filter pages");
        const auto check = [](texture_filter filter, sdlpp::scale_mode expected) {
            const atlas_id atlas = register_atlas(two_page_atlas(), atlas_texture_format::automatic, filter);
            CHECK(page_scale_mode(atlas, 0) == expected);
            CHECK(page_scale_mode(atlas, 1) == expected);
            unregister_atlas(atlas);
        };
        check(texture_filter::nearest, sdlpp::scale_mode::nearest);
        check(texture_filter::linear, sdlpp::scale_mode::linear);
        check(texture_filter::pixelart, sdlpp::scale_mode::pixelart);
    }

    TEST_CASE("nearest keeps texels when a picture is drawn scaled; linear blends them") {
        neutrino::test::test_application app("atlas filter sampling");
        CHECK(scaled_picture_keeps_texels(texture_filter::nearest));
        CHECK_FALSE(scaled_picture_keeps_texels(texture_filter::linear));
    }
}

TEST_SUITE("neutrino::video register_atlas indexed banks") {
    TEST_CASE("an indexed bank with a colour key stays INDEX8 on the CPU") {
        const image_atlas cpu = keyed_glyph_atlas();
        REQUIRE(atlas_test::page_count(cpu) == 1);
        CHECK(atlas_test::page(cpu, 0).format() == pixel_format::INDEX8);
    }

    TEST_CASE("after upload the key is transparent and the palette colours are intact") {
        neutrino::test::test_application app("indexed atlas upload");
        SUBCASE("automatic upload format") {
            const atlas_id atlas = register_atlas(keyed_glyph_atlas(), atlas_texture_format::automatic,
                                                  texture_filter::nearest);
            check_keyed_glyph(draw_glyph_x2(atlas, colors::white), colors::white);
            unregister_atlas(atlas);
        }
        SUBCASE("explicit RGBA upload format") {
            const atlas_id atlas = register_atlas(keyed_glyph_atlas(), atlas_texture_format::rgba,
                                                  texture_filter::nearest);
            check_keyed_glyph(draw_glyph_x2(atlas, colors::white), colors::white);
            unregister_atlas(atlas);
        }
    }

    TEST_CASE("a tint multiplies the authored colours and leaves the key transparent") {
        neutrino::test::test_application app("indexed atlas tint");
        const atlas_id atlas = register_atlas(keyed_glyph_atlas(), atlas_texture_format::rgba,
                                              texture_filter::nearest);
        const color tint{128, 255, 64, 255};
        check_keyed_glyph(draw_glyph_x2(atlas, tint), tint);
        unregister_atlas(atlas);
    }
}
