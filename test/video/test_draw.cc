#include <doctest/doctest.h>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>

#include "test_application.hh"

#include <limits>
#include <utility>
#include <vector>

// Draws return void; a draw "succeeded" when SDL rejected nothing, i.e. the engine's
// draw-failure counter did not move across the call.
#define CHECK_DRAWS(...)                                                           \
    do {                                                                           \
        const auto draw_failures_before_ = neutrino::draw_failure_count();         \
        __VA_ARGS__;                                                               \
        CHECK(neutrino::draw_failure_count() == draw_failures_before_);            \
    } while (false)

TEST_SUITE("neutrino::video::draw") {
    TEST_CASE("immediate draw helpers render with an application context") {
        neutrino::test::test_application test_app("Video draw test scaffolding");

        sdlpp::color red = sdlpp::colors::red;
        CHECK_DRAWS(neutrino::set_draw_color(red));

        const auto current = neutrino::get_draw_color();
        CHECK(current.r == red.r);
        CHECK(current.g == red.g);
        CHECK(current.b == red.b);

        CHECK_DRAWS(neutrino::draw_point(10, 20));
        CHECK_DRAWS(neutrino::draw_point(15, 25, sdlpp::colors::green));
        CHECK_DRAWS(neutrino::draw_line(0, 0, 50, 50));
        CHECK_DRAWS(neutrino::draw_line(10, 0, 10, 40, sdlpp::colors::blue));
        CHECK_DRAWS(neutrino::draw_rect(10, 10, 40, 40));
        CHECK_DRAWS(neutrino::draw_rect(5, 5, 25, 25, sdlpp::colors::white));
        CHECK_DRAWS(neutrino::draw_rect_fill(15, 15, 30, 30));
        CHECK_DRAWS(neutrino::draw_circle(50, 50, 10));
        CHECK_DRAWS(neutrino::draw_circle_fill(50, 50, 10, sdlpp::colors::yellow));
        CHECK_DRAWS(neutrino::draw_arrow(neutrino::point{0, 0}, neutrino::point{20, 20}));
        CHECK_DRAWS(neutrino::draw_cross(neutrino::point{50, 50}, 5, sdlpp::colors::cyan));

        auto sprite_surface = sdlpp::surface::create_rgb(4, 4, sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(sprite_surface.has_value());
        REQUIRE(sprite_surface->put_pixel(0, 0, sdlpp::colors::white).has_value());

        std::vector <neutrino::cpu_texture_atlas_frame> sprite_frames;
        sprite_frames.emplace_back(neutrino::rect{0, 0, 4, 4});
        neutrino::cpu_texture_atlas sprite_atlas(std::move(*sprite_surface), std::move(sprite_frames));
        const auto sprite_sheet = neutrino::register_sprite_sheet(sprite_atlas, neutrino::atlas_texture_format::rgba);
        const auto sprite = neutrino::visual_ref(sprite_sheet, 0);

        CHECK_DRAWS(neutrino::draw_sprite(neutrino::point{20, 20}, sprite));
        CHECK_DRAWS(neutrino::draw_sprite(
            neutrino::point{24, 20},
            sprite,
            {.flip = neutrino::sprite_flip::horizontal | neutrino::sprite_flip::vertical}));
        CHECK_DRAWS(neutrino::draw_sprite(
            neutrino::point{28, 20},
            sprite,
            {.flip = neutrino::sprite_flip::diagonal}));
        CHECK_DRAWS(neutrino::draw_sprite(
            neutrino::point{32, 20},
            sprite,
            {.flip = neutrino::sprite_flip::diagonal | neutrino::sprite_flip::horizontal}));
        CHECK_DRAWS(neutrino::draw_sprite(
            neutrino::point{36, 20},
            sprite,
            {.flip = neutrino::sprite_flip::diagonal | neutrino::sprite_flip::vertical}));
        CHECK_DRAWS(neutrino::draw_sprite(
            neutrino::point{40, 20},
            sprite,
            {.flip = neutrino::sprite_flip::diagonal | neutrino::sprite_flip::horizontal | neutrino::sprite_flip::vertical}));
        CHECK_DRAWS(neutrino::draw_sprite(
            neutrino::point{52, 20},
            sprite,
            {.scale = 2.0f}));
        CHECK_DRAWS(neutrino::draw_sprite(
            neutrino::point{60, 20},
            sprite,
            {.scale = 2.0f, .flip = neutrino::sprite_flip::diagonal | neutrino::sprite_flip::horizontal}));

        const neutrino::sprite_appearance appearance{
            .visual = sprite,
            .flip = neutrino::sprite_flip::horizontal,
            .visible = true
        };
        CHECK_DRAWS(neutrino::draw_sprite(neutrino::point{44, 20}, appearance));
        CHECK_DRAWS(neutrino::draw_sprite(neutrino::point{68, 20}, appearance, {.scale = 2.0f}));
        // A horizontal draw flip toggles the appearance's intrinsic horizontal flip.
        CHECK_DRAWS(neutrino::draw_sprite(
            neutrino::point{72, 20},
            appearance,
            {.flip = neutrino::sprite_flip::horizontal}));

        const auto state = neutrino::create_sprite_state(appearance);
        CHECK_DRAWS(neutrino::draw_sprite(neutrino::point{46, 20}, state));
        CHECK_DRAWS(neutrino::draw_sprite(neutrino::point{76, 20}, state, {.scale = 2.0f}));
        CHECK_DRAWS(neutrino::draw_sprite(
            neutrino::point{80, 20},
            state,
            {.scale = 2.0f, .flip = neutrino::sprite_flip::horizontal}));

        const neutrino::sprite_appearance hidden{
            .visual = sprite,
            .visible = false
        };
        CHECK_DRAWS(neutrino::draw_sprite(neutrino::point{48, 20}, hidden));
        CHECK_DRAWS(neutrino::draw_sprite(neutrino::point{84, 20}, hidden, {.scale = 2.0f}));

        CHECK_DRAWS(neutrino::draw_sprite(neutrino::point{10, 10}, neutrino::sprite_appearance{}));
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
        CHECK_DRAWS(neutrino::draw_sprite(at, sprite, {.rotation_degrees = 90.0f}));
        CHECK_DRAWS(neutrino::draw_sprite(at, sprite, {.rotation_degrees = 120.0f}));
        CHECK_DRAWS(neutrino::draw_sprite(
            at, sprite, {.flip = sprite_flip::horizontal, .rotation_degrees = 45.0f}));

        // Diagonal path (render_geometry): rotation composed with a diagonal flip.
        CHECK_DRAWS(neutrino::draw_sprite(
            at, sprite, {.flip = sprite_flip::diagonal, .rotation_degrees = 90.0f}));
        CHECK_DRAWS(neutrino::draw_sprite(
            at, sprite,
            {.scale = 2.0f,
             .flip = sprite_flip::diagonal | sprite_flip::horizontal,
             .rotation_degrees = 120.0f}));

        // Negative angle and a full turn are accepted.
        CHECK_DRAWS(neutrino::draw_sprite(at, sprite, {.rotation_degrees = -90.0f}));
        CHECK_DRAWS(neutrino::draw_sprite(at, sprite, {.rotation_degrees = 360.0f}));

        // 0 degrees takes the no-rotation path (byte-identical: angle 0, no pivot).
        CHECK_DRAWS(neutrino::draw_sprite(at, sprite, {.rotation_degrees = 0.0f}));

        // A non-finite rotation is rejected.
        CHECK_THROWS(neutrino::draw_sprite(
            at, sprite, {.rotation_degrees = std::numeric_limits<float>::infinity()}));
    }
}
