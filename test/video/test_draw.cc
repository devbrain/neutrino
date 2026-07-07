#include <doctest/doctest.h>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>

#include "test_application.hh"

#include <limits>
#include <utility>
#include <vector>

TEST_SUITE("neutrino::video::draw") {
    TEST_CASE("immediate draw helpers render with an application context") {
        neutrino::test::test_application test_app("Video draw test scaffolding");

        sdlpp::color red = sdlpp::colors::red;
        auto color_res = neutrino::set_draw_color(red);
        REQUIRE(color_res.has_value());

        auto get_color_res = neutrino::get_draw_color();
        REQUIRE(get_color_res.has_value());
        CHECK(get_color_res.value().r == red.r);
        CHECK(get_color_res.value().g == red.g);
        CHECK(get_color_res.value().b == red.b);

        CHECK(neutrino::draw_point(10, 20).has_value());
        CHECK(neutrino::draw_point(15, 25, sdlpp::colors::green).has_value());
        CHECK(neutrino::draw_line(0, 0, 50, 50).has_value());
        CHECK(neutrino::draw_line(10, 0, 10, 40, sdlpp::colors::blue).has_value());
        CHECK(neutrino::draw_rect(10, 10, 40, 40).has_value());
        CHECK(neutrino::draw_rect(5, 5, 25, 25, sdlpp::colors::white).has_value());
        CHECK(neutrino::draw_rect_fill(15, 15, 30, 30).has_value());
        CHECK(neutrino::draw_circle(50, 50, 10).has_value());
        CHECK(neutrino::draw_circle_fill(50, 50, 10, sdlpp::colors::yellow).has_value());
        CHECK(neutrino::draw_arrow(neutrino::point{0, 0}, neutrino::point{20, 20}).has_value());
        CHECK(neutrino::draw_cross(neutrino::point{50, 50}, 5, sdlpp::colors::cyan).has_value());

        auto sprite_surface = sdlpp::surface::create_rgb(4, 4, sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(sprite_surface.has_value());
        REQUIRE(sprite_surface->put_pixel(0, 0, sdlpp::colors::white).has_value());

        std::vector <neutrino::cpu_texture_atlas_frame> sprite_frames;
        sprite_frames.emplace_back(neutrino::rect{0, 0, 4, 4});
        neutrino::cpu_texture_atlas sprite_atlas(std::move(*sprite_surface), std::move(sprite_frames));
        const auto sprite_sheet = neutrino::register_sprite_sheet(sprite_atlas, neutrino::atlas_texture_format::rgba);
        const auto sprite = neutrino::visual_ref(sprite_sheet, 0);

        CHECK(neutrino::draw_sprite(neutrino::point{20, 20}, sprite).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{24, 20},
            sprite,
            {.flip = neutrino::sprite_flip::horizontal | neutrino::sprite_flip::vertical}).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{28, 20},
            sprite,
            {.flip = neutrino::sprite_flip::diagonal}).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{32, 20},
            sprite,
            {.flip = neutrino::sprite_flip::diagonal | neutrino::sprite_flip::horizontal}).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{36, 20},
            sprite,
            {.flip = neutrino::sprite_flip::diagonal | neutrino::sprite_flip::vertical}).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{40, 20},
            sprite,
            {.flip = neutrino::sprite_flip::diagonal | neutrino::sprite_flip::horizontal | neutrino::sprite_flip::vertical}).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{52, 20},
            sprite,
            {.scale = 2.0f}).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{60, 20},
            sprite,
            {.scale = 2.0f, .flip = neutrino::sprite_flip::diagonal | neutrino::sprite_flip::horizontal}).has_value());

        const neutrino::sprite_appearance appearance{
            .visual = sprite,
            .flip = neutrino::sprite_flip::horizontal,
            .visible = true
        };
        CHECK(neutrino::draw_sprite(neutrino::point{44, 20}, appearance).has_value());
        CHECK(neutrino::draw_sprite(neutrino::point{68, 20}, appearance, {.scale = 2.0f}).has_value());
        // A horizontal draw flip toggles the appearance's intrinsic horizontal flip.
        CHECK(neutrino::draw_sprite(
            neutrino::point{72, 20},
            appearance,
            {.flip = neutrino::sprite_flip::horizontal}).has_value());

        const auto state = neutrino::create_sprite_state(appearance);
        CHECK(neutrino::draw_sprite(neutrino::point{46, 20}, state).has_value());
        CHECK(neutrino::draw_sprite(neutrino::point{76, 20}, state, {.scale = 2.0f}).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{80, 20},
            state,
            {.scale = 2.0f, .flip = neutrino::sprite_flip::horizontal}).has_value());

        const neutrino::sprite_appearance hidden{
            .visual = sprite,
            .visible = false
        };
        CHECK(neutrino::draw_sprite(neutrino::point{48, 20}, hidden).has_value());
        CHECK(neutrino::draw_sprite(neutrino::point{84, 20}, hidden, {.scale = 2.0f}).has_value());

        CHECK(neutrino::draw_sprite(neutrino::point{10, 10}, neutrino::sprite_appearance{}).has_value());
    }

    TEST_CASE("sprite flip composition toggles flags") {
        using neutrino::sprite_flip;
        CHECK((sprite_flip::horizontal ^ sprite_flip::horizontal) == sprite_flip::none);
        CHECK((sprite_flip::none ^ sprite_flip::horizontal) == sprite_flip::horizontal);
        CHECK(((sprite_flip::horizontal | sprite_flip::diagonal) ^ sprite_flip::horizontal) == sprite_flip::diagonal);
        auto flip = sprite_flip::vertical;
        flip ^= sprite_flip::vertical | sprite_flip::horizontal;
        CHECK(flip == sprite_flip::horizontal);
    }

    TEST_CASE("sprite rotation renders through both draw paths") {
        neutrino::test::test_application app("Sprite rotation test scaffolding");

        auto surface = sdlpp::surface::create_rgb(4, 4, sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());
        REQUIRE(surface->fill(sdlpp::colors::white).has_value());
        std::vector <neutrino::cpu_texture_atlas_frame> frames;
        frames.emplace_back(neutrino::rect{0, 0, 4, 4});
        neutrino::cpu_texture_atlas atlas(std::move(*surface), std::move(frames));
        const auto sheet = neutrino::register_sprite_sheet(atlas, neutrino::atlas_texture_format::rgba);
        const auto sprite = neutrino::visual_ref(sheet, 0);
        const neutrino::point at{20, 20};

        using neutrino::sprite_flip;

        // Non-diagonal path (copy_ex): rotation alone, hex-120, and rotation + flip.
        CHECK(neutrino::draw_sprite(at, sprite, {.rotation_degrees = 90.0f}).has_value());
        CHECK(neutrino::draw_sprite(at, sprite, {.rotation_degrees = 120.0f}).has_value());
        CHECK(neutrino::draw_sprite(
            at, sprite, {.flip = sprite_flip::horizontal, .rotation_degrees = 45.0f}).has_value());

        // Diagonal path (render_geometry): rotation composed with a diagonal flip.
        CHECK(neutrino::draw_sprite(
            at, sprite, {.flip = sprite_flip::diagonal, .rotation_degrees = 90.0f}).has_value());
        CHECK(neutrino::draw_sprite(
            at, sprite,
            {.scale = 2.0f,
             .flip = sprite_flip::diagonal | sprite_flip::horizontal,
             .rotation_degrees = 120.0f}).has_value());

        // Negative angle and a full turn are accepted.
        CHECK(neutrino::draw_sprite(at, sprite, {.rotation_degrees = -90.0f}).has_value());
        CHECK(neutrino::draw_sprite(at, sprite, {.rotation_degrees = 360.0f}).has_value());

        // 0 degrees takes the no-rotation path (byte-identical: angle 0, no pivot).
        CHECK(neutrino::draw_sprite(at, sprite, {.rotation_degrees = 0.0f}).has_value());

        // A non-finite rotation is rejected.
        CHECK_THROWS((void)neutrino::draw_sprite(
            at, sprite, {.rotation_degrees = std::numeric_limits<float>::infinity()}));
    }
}
