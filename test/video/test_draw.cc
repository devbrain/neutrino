#include <doctest/doctest.h>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>

#include "test_application.hh"

#include <limits>
#include <memory>
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

namespace {
    // Brace-init pins (P2.1): a braced {x, y} must keep selecting the integer overloads -- the
    // render_pos / render_rect forms are constrained templates precisely so this stays
    // unambiguous. Never called; it only has to compile.
    [[maybe_unused]] void brace_init_still_selects_integer_overloads(neutrino::sprite_state_id state) {
        neutrino::draw_point({1, 2});
        neutrino::draw_point({1, 2}, sdlpp::colors::red);
        neutrino::draw_line({0, 0}, {5, 5});
        neutrino::draw_rect({0, 0, 4, 4});
        neutrino::draw_rect_fill({0, 0, 4, 4}, sdlpp::colors::red);
        neutrino::draw_sprite({1, 2}, state);
        neutrino::draw_line_aa({0, 0}, {5, 5});
        neutrino::draw_line_thick({0, 0}, {5, 5}, 2.0f);
        neutrino::draw_line_dashed({0, 0}, {5, 5}, 2, 1);
        neutrino::draw_line_dotted({0, 0}, {5, 5}, 2);
        neutrino::draw_arrow({0, 0}, {5, 5});
        neutrino::draw_cross({3, 3});
    }

    [[maybe_unused]] void float_overloads_are_reachable(neutrino::sprite_state_id state) {
        neutrino::draw_point(neutrino::render_pos{1.5f, 2.5f});
        neutrino::draw_line(neutrino::render_pos{0.5f, 0.f}, {5.f, 5.f});
        neutrino::draw_rect(neutrino::render_rect{0.5f, 0.5f, 4.f, 4.f});
        neutrino::draw_rect_fill(neutrino::render_rect{0.5f, 0.5f, 4.f, 4.f}, sdlpp::colors::red);
        neutrino::draw_circle(neutrino::render_pos{8.f, 8.f}, 3.5f);
        neutrino::draw_circle_fill(neutrino::render_pos{8.f, 8.f}, 3.5f, sdlpp::colors::red);
        neutrino::draw_sprite(neutrino::render_pos{1.5f, 2.5f}, state);
        const neutrino::render_pos a{0.5f, 0.5f};
        const neutrino::render_pos b{5.5f, 5.5f};
        neutrino::draw_line_aa(a, b, sdlpp::colors::red);
        neutrino::draw_line_thick(a, b, 2.0f);
        neutrino::draw_line_dashed(a, b, 2.0f, 1.0f, sdlpp::colors::red);
        neutrino::draw_line_dotted(a, b, 2.0f);
        neutrino::draw_arrow(a, b);
        neutrino::draw_arrow(a, b, 6.0f, 30.0f, sdlpp::colors::red);
        neutrino::draw_cross(a);
        neutrino::draw_cross(a, 4.0f, 1.0f, sdlpp::colors::red);
    }

    // Restores the process-wide snapping policy when a test changes it.
    class snap_scope {
        public:
            explicit snap_scope(neutrino::pixel_snap mode) : m_prev(neutrino::get_pixel_snap()) {
                neutrino::set_pixel_snap(mode);
            }
            ~snap_scope() { neutrino::set_pixel_snap(m_prev); }
            snap_scope(const snap_scope&) = delete;
            snap_scope& operator=(const snap_scope&) = delete;

        private:
            neutrino::pixel_snap m_prev;
    };

    // An offscreen target drawn at a 4x render scale, like a 640x360 logical canvas in a larger
    // window. At 1:1 SDL's pixel-centre coverage rule makes a fractional edge indistinguishable from
    // a rounded one; at 4x, logical 10.25 lands on physical pixel 41 unsnapped and 40 snapped.
    class scaled_target {
        public:
            static constexpr float scale = 4.0f;

            scaled_target()
                : m_renderer(neutrino::get_renderer().get()),
                  m_previous(SDL_GetRenderTarget(m_renderer)) {
                m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                              64, 64);
                REQUIRE(m_texture != nullptr);
                REQUIRE(SDL_SetRenderTarget(m_renderer, m_texture));
                REQUIRE(SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255));
                REQUIRE(SDL_RenderClear(m_renderer));
                REQUIRE(SDL_SetRenderScale(m_renderer, scale, scale));
            }

            ~scaled_target() {
                SDL_SetRenderScale(m_renderer, 1.0f, 1.0f);
                SDL_SetRenderTarget(m_renderer, m_previous);
                SDL_DestroyTexture(m_texture);
            }

            scaled_target(const scaled_target&) = delete;
            scaled_target& operator=(const scaled_target&) = delete;

            // True if the PHYSICAL pixel (x, y) of the target is lit (not the black background).
            [[nodiscard]] bool lit(int x, int y) {
                REQUIRE(SDL_SetRenderScale(m_renderer, 1.0f, 1.0f));
                std::unique_ptr <SDL_Surface, decltype(&SDL_DestroySurface)> pixels(
                    SDL_RenderReadPixels(m_renderer, nullptr), SDL_DestroySurface);
                REQUIRE(SDL_SetRenderScale(m_renderer, scale, scale));
                REQUIRE(pixels);
                Uint8 r = 0, g = 0, b = 0, a = 0;
                REQUIRE(SDL_ReadSurfacePixel(pixels.get(), x, y, &r, &g, &b, &a));
                return r != 0 || g != 0 || b != 0;
            }

        private:
            SDL_Renderer* m_renderer;
            SDL_Texture* m_previous;
            SDL_Texture* m_texture{nullptr};
    };

    [[nodiscard]] neutrino::sprite_visual_ref white_square_sprite() {
        auto surface = sdlpp::surface::create_rgb(4, 4, sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());
        REQUIRE(surface->fill(sdlpp::colors::white).has_value());
        std::vector <neutrino::cpu_texture_atlas_frame> frames;
        frames.emplace_back(neutrino::rect{0, 0, 4, 4});
        neutrino::cpu_texture_atlas atlas(std::move(*surface), std::move(frames));
        return neutrino::visual_ref(neutrino::register_sprite_sheet(atlas, neutrino::atlas_texture_format::rgba), 0);
    }
}

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
    TEST_CASE("float rectangles reach SDL unrounded unless pixel_snap::logical") {
        neutrino::test::test_application app("Float draw test scaffolding");
        const neutrino::render_rect r{10.25f, 10.25f, 2.0f, 2.0f}; // physical [41, 49) at 4x

        SUBCASE("off: the fractional edge lands between logical pixels") {
            snap_scope snap(neutrino::pixel_snap::off);
            scaled_target target;
            CHECK_DRAWS(neutrino::draw_rect_fill(r, sdlpp::colors::red));
            CHECK_FALSE(target.lit(40, 40));
            CHECK(target.lit(41, 41));
            CHECK(target.lit(48, 48));
            CHECK_FALSE(target.lit(49, 49));
        }

        SUBCASE("logical: edges snap to whole logical pixels") {
            snap_scope snap(neutrino::pixel_snap::logical);
            scaled_target target;
            CHECK_DRAWS(neutrino::draw_rect_fill(r, sdlpp::colors::red)); // -> logical [10, 12)
            CHECK(target.lit(40, 40));
            CHECK(target.lit(47, 47));
            CHECK_FALSE(target.lit(48, 48));
        }
    }

    TEST_CASE("float sprite positions reach SDL unrounded unless pixel_snap::logical") {
        neutrino::test::test_application app("Float sprite test scaffolding");
        const auto sprite = white_square_sprite(); // 4x4, origin at its top-left

        SUBCASE("off") {
            snap_scope snap(neutrino::pixel_snap::off);
            scaled_target target;
            CHECK_DRAWS(neutrino::draw_sprite(neutrino::render_pos{10.25f, 10.25f}, sprite));
            CHECK_FALSE(target.lit(40, 40));
            CHECK(target.lit(41, 41));
        }

        SUBCASE("logical") {
            snap_scope snap(neutrino::pixel_snap::logical);
            scaled_target target;
            CHECK_DRAWS(neutrino::draw_sprite(neutrino::render_pos{10.25f, 10.25f}, sprite));
            CHECK(target.lit(40, 40));
            CHECK_FALSE(target.lit(56, 56));
        }
    }

    TEST_CASE("integer sprite positions draw identically under both snap policies") {
        neutrino::test::test_application app("Integer sprite test scaffolding");
        const auto sprite = white_square_sprite();

        for (const auto mode : {neutrino::pixel_snap::off, neutrino::pixel_snap::logical}) {
            CAPTURE(static_cast <int>(mode));
            snap_scope snap(mode);
            scaled_target target;
            CHECK_DRAWS(neutrino::draw_sprite(neutrino::point{10, 10}, sprite));
            CHECK_FALSE(target.lit(39, 39));
            CHECK(target.lit(40, 40));
            CHECK(target.lit(55, 55));
            CHECK_FALSE(target.lit(56, 56));
        }
    }

    TEST_CASE("the application applies its configured pixel_snap at startup") {
        neutrino::application_config cfg;
        cfg.title = "Pixel snap config";
        cfg.flags = sdlpp::window_flags::hidden;
        cfg.snap = neutrino::pixel_snap::logical;
        snap_scope restore(neutrino::pixel_snap::off);
        neutrino::test::test_application app(cfg);
        CHECK(neutrino::get_pixel_snap() == neutrino::pixel_snap::logical);
    }
}
