#include <doctest/doctest.h>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>

#include "test_application.hh"

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

        CHECK(neutrino::draw_sprite(
            neutrino::point{20, 20},
            sprite,
            neutrino::sprite_flip::none).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{24, 20},
            sprite,
            neutrino::sprite_flip::horizontal | neutrino::sprite_flip::vertical).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{28, 20},
            sprite,
            neutrino::sprite_flip::diagonal).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{32, 20},
            sprite,
            neutrino::sprite_flip::diagonal | neutrino::sprite_flip::horizontal).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{36, 20},
            sprite,
            neutrino::sprite_flip::diagonal | neutrino::sprite_flip::vertical).has_value());
        CHECK(neutrino::draw_sprite(
            neutrino::point{40, 20},
            sprite,
            neutrino::sprite_flip::diagonal | neutrino::sprite_flip::horizontal | neutrino::sprite_flip::vertical).has_value());

        const neutrino::sprite_appearance appearance{
            .visual = sprite,
            .flip = neutrino::sprite_flip::horizontal,
            .visible = true
        };
        CHECK(neutrino::draw_sprite(neutrino::point{44, 20}, appearance).has_value());

        const auto state = neutrino::create_sprite_state(appearance);
        CHECK(neutrino::draw_sprite(neutrino::point{46, 20}, state).has_value());

        const neutrino::sprite_appearance hidden{
            .visual = sprite,
            .visible = false
        };
        CHECK(neutrino::draw_sprite(neutrino::point{48, 20}, hidden).has_value());
    }
}
