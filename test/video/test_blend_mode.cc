//
// Tests for neutrino's blend_mode and its internal mapping to SDL's (video/blend_mode_sdl.hh).
//
#include <doctest/doctest.h>

#include <neutrino/video/blend_mode.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/texture.hh>

#include "test_application.hh"
#include "video/blend_mode_sdl.hh"

#include <array>
#include <set>
#include <utility>

using namespace neutrino;
using neutrino::detail::from_sdl_blend;
using neutrino::detail::to_sdl_blend;

namespace {
    // Every neutrino blend mode with the SDL preset it stands for, written out by name so the
    // test does not depend on either side's numbers.
    constexpr std::array MODES{
        std::pair{blend_mode::none, sdlpp::blend_mode::none},
        std::pair{blend_mode::blend, sdlpp::blend_mode::blend},
        std::pair{blend_mode::add, sdlpp::blend_mode::add},
        std::pair{blend_mode::mul, sdlpp::blend_mode::mul},
        std::pair{blend_mode::mod, sdlpp::blend_mode::mod},
        std::pair{blend_mode::blend_premultiplied, sdlpp::blend_mode::blend_premultiplied},
        std::pair{blend_mode::add_premultiplied, sdlpp::blend_mode::add_premultiplied},
    };
}

TEST_SUITE("neutrino::blend_mode") {
    TEST_CASE("every mode maps to its SDL preset and back") {
        std::set <sdlpp::blend_mode> seen;
        for (const auto& [mode, sdl] : MODES) {
            CAPTURE(static_cast <std::uint32_t>(mode));
            CHECK(to_sdl_blend(mode) == sdl);
            CHECK(from_sdl_blend(sdl) == mode);
            seen.insert(to_sdl_blend(mode));
        }
        CHECK(seen.size() == MODES.size());           // no two modes share an SDL preset
    }

    TEST_CASE("a value neutrino has no name for maps to nothing") {
        CHECK(to_sdl_blend(static_cast <blend_mode>(99)) == sdlpp::blend_mode::invalid);
        CHECK_FALSE(from_sdl_blend(sdlpp::blend_mode::invalid).has_value());
        // SDL reads any other value as a packed custom blend.
        CHECK_FALSE(from_sdl_blend(static_cast <sdlpp::blend_mode>(0x03)).has_value());
    }

    TEST_CASE("set_draw_blend_mode reaches SDL at the next draw; get_draw_blend_mode reads it back") {
        test::test_application app("blend mode draw");
        for (const auto& [mode, sdl] : MODES) {
            CAPTURE(static_cast <std::uint32_t>(mode));
            set_draw_blend_mode(mode);
            CHECK(get_draw_blend_mode() == mode);
            draw_point(0, 0);                         // a draw sets the blend it uses (R48)
            CHECK(get_renderer().get_draw_blend_mode() == sdl);
        }
    }

    TEST_CASE("a value outside the enum fails at SDL, counted, and leaves the mode as it was") {
        test::test_application app("blend mode invalid");
        set_draw_blend_mode(blend_mode::add);
        const auto failures = draw_failure_count();
        set_draw_blend_mode(static_cast <blend_mode>(99));
        CHECK(draw_failure_count() == failures + 1);
        CHECK(get_draw_blend_mode() == blend_mode::add);
    }

    TEST_CASE("texture::set_blend_mode reaches the SDL texture") {
        test::test_application app("blend mode texture");
        auto t = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(t.has_value());
        for (const auto& [mode, sdl] : MODES) {
            CAPTURE(static_cast <std::uint32_t>(mode));
            t->set_blend_mode(mode);
            const auto got = t->native().get_blend_mode();
            REQUIRE(got.has_value());
            CHECK(*got == sdl);
        }
    }
}
