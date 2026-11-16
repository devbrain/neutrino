//
// Tests for the internal texture allocator and its handles (painter.md §3.3).
//
// Every case runs inside a test_application: handles reach the allocator the application
// publishes, so that is the allocator under test.
//
#include <doctest/doctest.h>

#include <neutrino/video/draw.hh>
#include <neutrino/video/texture.hh>

#include "test_application.hh"
#include "services/service_access.hh"
#include "video/texture_allocator.hh"

#include <utility>

using namespace neutrino;
using neutrino::detail::texture_allocator;
using neutrino::detail::texture_handle;
using neutrino::detail::texture_spec;

namespace {
    constexpr dim SMALL{4, 4};
    constexpr dim MEDIUM{8, 8};
    constexpr dim WIDE{16, 8};

    [[nodiscard]] constexpr std::size_t bytes(dim size) {
        return 4u * static_cast <std::size_t>(size.width) * static_cast <std::size_t>(size.height);
    }

    // The invariant the stats can show: every texture made is held, pooled or destroyed.
    void check_balance(const texture_allocator& a) {
        const auto& s = a.stats();
        CHECK(s.created - s.destroyed == s.held + s.pooled);
    }

    [[nodiscard]] bool holds(const texture_handle& h, const color& c) {
        const image img = h.get().to_image();
        const auto px = img.get_pixel(1, 1);
        REQUIRE(px.has_value());
        return px->r == c.r && px->g == c.g && px->b == c.b;
    }
}

TEST_SUITE("neutrino::detail::texture_allocator") {
    TEST_CASE("acquire creates a texture cleared to the spec's colour that needs a redraw") {
        test::test_application app("allocator acquire");
        auto& a = require_texture_allocator();

        auto h = a.acquire(texture_spec{MEDIUM, colors::red});
        REQUIRE(h);
        CHECK(h.size() == MEDIUM);
        CHECK(h.needs_redraw());
        CHECK(holds(h, colors::red));

        const auto& s = a.stats();
        CHECK(s.created == 1);
        CHECK(s.held == 1);
        CHECK(s.held_bytes == bytes(MEDIUM));
        CHECK(s.pooled == 0);
        check_balance(a);

        h.mark_drawn();
        CHECK_FALSE(h.needs_redraw());
        h.clear();                                   // clearing does not change the flag
        CHECK_FALSE(h.needs_redraw());
        CHECK(holds(h, colors::red));
    }

    TEST_CASE("get() gives the texture to draw into; a targets reset clears the drawing") {
        test::test_application app("allocator draw");
        auto& a = require_texture_allocator();

        auto h = a.acquire(texture_spec{MEDIUM, colors::red});
        h.get().compose([] { draw_rect_fill(rect{0, 0, 8, 8}, colors::blue); }, std::nullopt);
        h.mark_drawn();
        CHECK(holds(h, colors::blue));

        a.on_targets_reset();
        CHECK(h.needs_redraw());
        CHECK(holds(h, colors::red));                // back to the clear colour

        texture_handle empty;
        CHECK_THROWS((void) empty.get());            // the non-const overload enforces too
    }

    TEST_CASE("a texture given back is reused at its size only, cleared to the new colour") {
        test::test_application app("allocator reuse");
        auto& a = require_texture_allocator();

        {
            auto h = a.acquire(texture_spec{MEDIUM, colors::red});
            h.mark_drawn();
        }
        CHECK(a.stats().held == 0);
        CHECK(a.stats().held_bytes == 0);
        CHECK(a.stats().pooled == 1);
        CHECK(a.stats().pooled_bytes == bytes(MEDIUM));
        check_balance(a);

        auto other = a.acquire(texture_spec{SMALL, colors::blue});   // another size: created
        CHECK(a.stats().created == 2);
        CHECK(a.stats().reused == 0);
        CHECK(a.stats().pooled == 1);

        auto again = a.acquire(texture_spec{MEDIUM, colors::blue});  // same size: reused
        CHECK(a.stats().created == 2);
        CHECK(a.stats().reused == 1);
        CHECK(a.stats().pooled == 0);
        CHECK(a.stats().pooled_bytes == 0);
        CHECK(a.stats().held == 2);
        CHECK(a.stats().held_bytes == bytes(MEDIUM) + bytes(SMALL));
        CHECK(again.needs_redraw());
        CHECK(holds(again, colors::blue));           // the old red never shows
        check_balance(a);
    }

    TEST_CASE("moving moves the texture; assigning over a handle gives its own back first") {
        test::test_application app("allocator move");
        auto& a = require_texture_allocator();

        auto first = a.acquire(texture_spec{MEDIUM, colors::red});
        texture_handle moved = std::move(first);
        CHECK_FALSE(first);
        CHECK(first.size() == dim{0, 0});
        CHECK(first.needs_redraw());
        REQUIRE(moved);
        CHECK(moved.size() == MEDIUM);
        CHECK(a.stats().held == 1);

        auto second = a.acquire(texture_spec{SMALL, colors::blue});
        moved = std::move(second);                   // moved's MEDIUM goes to the pool
        CHECK_FALSE(second);
        CHECK(moved.size() == SMALL);
        CHECK(a.stats().held == 1);
        CHECK(a.stats().pooled == 1);
        CHECK(a.stats().pooled_bytes == bytes(MEDIUM));
        check_balance(a);
    }

    TEST_CASE("a degenerate size gives a failed handle that resize tries again") {
        test::test_application app("allocator failed");
        auto& a = require_texture_allocator();

        auto h = a.acquire(texture_spec{dim{0, 8}, colors::red});
        CHECK_FALSE(h);
        CHECK(h.size() == dim{0, 8});                // the size asked for
        CHECK(h.needs_redraw());
        h.mark_drawn();                              // nothing to mark
        CHECK(h.needs_redraw());
        CHECK_NOTHROW(h.clear());
        CHECK_THROWS((void) h.get());
        CHECK(a.stats().created == 0);
        CHECK(a.stats().held == 0);

        a.resize(h, MEDIUM);
        REQUIRE(h);
        CHECK(h.size() == MEDIUM);
        CHECK(h.needs_redraw());
        CHECK(holds(h, colors::red));
        CHECK(a.stats().created == 1);
        CHECK(a.stats().held == 1);
        check_balance(a);
    }

    TEST_CASE("resize keeps the texture at the same size and replaces it at another") {
        test::test_application app("allocator resize");
        auto& a = require_texture_allocator();

        auto h = a.acquire(texture_spec{MEDIUM, colors::red});
        h.mark_drawn();
        a.resize(h, MEDIUM);                         // same size: contents and flag kept
        CHECK_FALSE(h.needs_redraw());
        CHECK(a.stats().created == 1);
        CHECK(a.stats().destroyed == 0);

        a.resize(h, WIDE);                           // another size: old destroyed, not pooled
        REQUIRE(h);
        CHECK(h.size() == WIDE);
        CHECK(h.needs_redraw());
        CHECK(holds(h, colors::red));
        CHECK(a.stats().created == 2);
        CHECK(a.stats().destroyed == 1);
        CHECK(a.stats().pooled == 0);
        CHECK(a.stats().held == 1);
        CHECK(a.stats().held_bytes == bytes(WIDE));
        check_balance(a);

        texture_handle empty;
        a.resize(empty, MEDIUM);                     // nothing to resize
        CHECK_FALSE(empty);
        CHECK(a.stats().created == 2);
    }

    TEST_CASE("drop_pooled destroys one size; a requested trim waits for begin_frame") {
        test::test_application app("allocator trim");
        auto& a = require_texture_allocator();

        {
            auto m = a.acquire(texture_spec{MEDIUM, colors::red});
            auto s = a.acquire(texture_spec{SMALL, colors::red});
        }
        CHECK(a.stats().pooled == 2);

        a.drop_pooled(MEDIUM);
        CHECK(a.stats().pooled == 1);
        CHECK(a.stats().pooled_bytes == bytes(SMALL));
        CHECK(a.stats().destroyed == 1);
        a.drop_pooled(MEDIUM);                       // none left of that size
        CHECK(a.stats().destroyed == 1);

        a.begin_frame();                             // no trim requested: nothing happens
        CHECK(a.stats().pooled == 1);
        a.request_trim();
        CHECK(a.stats().pooled == 1);                // not before the next frame
        a.begin_frame();
        CHECK(a.stats().pooled == 0);
        CHECK(a.stats().pooled_bytes == 0);
        CHECK(a.stats().destroyed == 2);
        a.begin_frame();                             // the request was used up
        CHECK(a.stats().destroyed == 2);
        check_balance(a);
    }

    TEST_CASE("the resets mark live handles; a device reset recreates them in place") {
        test::test_application app("allocator resets");
        auto& a = require_texture_allocator();

        {
            auto given_back = a.acquire(texture_spec{SMALL, colors::blue});
        }
        auto h = a.acquire(texture_spec{MEDIUM, colors::red});
        h.mark_drawn();

        a.on_targets_reset();                        // contents lost; the pool is left alone
        CHECK(h.needs_redraw());
        CHECK(holds(h, colors::red));
        CHECK(a.stats().pooled == 1);
        CHECK(a.stats().destroyed == 0);

        h.mark_drawn();
        a.on_device_reset();                         // pool destroyed, h recreated in place
        REQUIRE(h);
        CHECK(h.size() == MEDIUM);
        CHECK(h.needs_redraw());
        CHECK(holds(h, colors::red));
        CHECK(a.stats().pooled == 0);
        CHECK(a.stats().destroyed == 2);             // the pooled one and h's old one
        CHECK(a.stats().created == 3);
        CHECK(a.stats().held == 1);
        CHECK(a.stats().held_bytes == bytes(MEDIUM));
        check_balance(a);
    }

    TEST_CASE("a handle that outlives the application is detached") {
        texture_handle h;
        {
            test::test_application app("allocator detach");
            h = require_texture_allocator().acquire(texture_spec{MEDIUM, colors::red});
            REQUIRE(h);
        }
        CHECK_FALSE(h);
        CHECK(h.size() == dim{0, 0});
        CHECK(h.needs_redraw());
        CHECK_NOTHROW(h.mark_drawn());
        CHECK_NOTHROW(h.clear());
        CHECK_THROWS((void) h.get());

        // A later application's allocator does not hold h's slot.
        test::test_application next("allocator after detach");
        CHECK_FALSE(h);
        CHECK(require_texture_allocator().stats().held == 0);
    }                                                // h's destructor does nothing
}
