#include <doctest/doctest.h>
#include <neutrino/video/sprite/cpu_sprites_atlas.hh>

TEST_SUITE("neutrino::video::sprite::sprites_atlas") {
    TEST_CASE("cpu_sprite_frame evaluates RGBA alpha masks") {
        auto surface = sdlpp::surface::create_rgb(3, 1, sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());

        REQUIRE(surface->put_pixel(0, 0, sdlpp::color{255, 0, 0, 255}).has_value());
        REQUIRE(surface->put_pixel(1, 0, sdlpp::color{0, 255, 0, 0}).has_value());
        REQUIRE(surface->put_pixel(2, 0, sdlpp::color{0, 0, 255, 1}).has_value());

        auto mask = neutrino::cpu_sprite_frame::evaluate_bitmask(*surface, neutrino::rect{0, 0, 3, 1});
        REQUIRE(mask.has_value());
        CHECK(mask->get(0, 0));
        CHECK_FALSE(mask->get(1, 0));
        CHECK(mask->get(2, 0));
    }

    TEST_CASE("cpu_sprite_frame evaluates explicit RGB key color masks") {
        auto surface = sdlpp::surface::create_rgb(3, 1, sdlpp::pixel_format_enum::RGB24);
        REQUIRE(surface.has_value());

        REQUIRE(surface->put_pixel(0, 0, sdlpp::color{255, 0, 255}).has_value());
        REQUIRE(surface->put_pixel(1, 0, sdlpp::color{20, 30, 40}).has_value());
        REQUIRE(surface->put_pixel(2, 0, sdlpp::color{255, 0, 255}).has_value());

        neutrino::cpu_sprite_frame_mask_options options;
        options.key_color = sdlpp::color{255, 0, 255};

        auto mask = neutrino::cpu_sprite_frame::evaluate_bitmask(*surface, neutrino::rect{0, 0, 3, 1}, options);
        REQUIRE(mask.has_value());
        CHECK_FALSE(mask->get(0, 0));
        CHECK(mask->get(1, 0));
        CHECK_FALSE(mask->get(2, 0));
    }

    TEST_CASE("cpu_sprite_frame evaluates INDEX8 key color index masks") {
        auto surface = sdlpp::surface::create_rgb(3, 1, sdlpp::pixel_format_enum::INDEX8);
        REQUIRE(surface.has_value());

        auto* pixels = static_cast<uint8_t*>(surface->get()->pixels);
        REQUIRE(pixels != nullptr);
        pixels[0] = 0;
        pixels[1] = 2;
        pixels[2] = 0;

        neutrino::cpu_sprite_frame_mask_options options;
        options.key_color_index = 0;

        auto mask = neutrino::cpu_sprite_frame::evaluate_bitmask(*surface, neutrino::rect{0, 0, 3, 1}, options);
        REQUIRE(mask.has_value());
        CHECK_FALSE(mask->get(0, 0));
        CHECK(mask->get(1, 0));
        CHECK_FALSE(mask->get(2, 0));
    }
}
