//
// Tests for the internal texture allocator, its handles and its slot table (painter.md §3.3).
//
// The allocator cases run inside a test_application: handles reach the allocator the
// application publishes, so that is the allocator under test. Every case checks the §3.3
// invariants after each operation (see `invariants` below).
//
#include <doctest/doctest.h>

#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/texture.hh>
#include <neutrino/video/texture_pool_stats.hh>

#include "test_application.hh"
#include "scene/scenes_manager.hh"
#include "services/service_access.hh"
#include "services/service_locator.hh"
#include "video/texture_allocator.hh"
#include "video/texture_table.hh"

#include <algorithm>
#include <atomic>
#include <initializer_list>
#include <memory>
#include <optional>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace neutrino::detail {
    // Only the allocator makes a table; the table's own tests run on one made here.
    struct texture_table_test_access {
        template<typename Fn>
        static void with_table(Fn&& fn) {
            texture_table table;
            fn(table);
        }
    };
}

using namespace neutrino;
using neutrino::detail::bind_pooled;
using neutrino::detail::target_geometry;
using neutrino::detail::texture_allocator;
using neutrino::detail::texture_handle;
using neutrino::detail::texture_slot;
using neutrino::detail::texture_spec;
using neutrino::detail::texture_table;
using neutrino::detail::texture_table_test_access;

namespace {
    constexpr dim SMALL{4, 4};
    constexpr dim MEDIUM{8, 8};
    constexpr dim WIDE{16, 8};

    [[nodiscard]] constexpr std::size_t bytes(dim size) {
        return 4u * static_cast <std::size_t>(size.width) * static_cast <std::size_t>(size.height);
    }

    [[nodiscard]] bool same(const color& a, const color& b) {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }

    // True if every pixel of the handle's texture is @p c.
    [[nodiscard]] bool only(const texture_handle& h, const color& c) {
        const texture& t = h.get();
        const image img = t.to_image();
        for (int y = 0; y < t.size().height; ++y) {
            for (int x = 0; x < t.size().width; ++x) {
                const auto px = img.get_pixel(x, y);
                if (!px || !same(*px, c)) {
                    return false;
                }
            }
        }
        return true;
    }

    // A handle the test still owns, with the colour it asked for.
    struct watched {
        const texture_handle& h;
        color clear;
    };

    // The invariants of painter.md §3.3. check() is given every handle the test still owns;
    // it is called after each operation, so the peak per size it records is the true peak.
    class invariants {
        public:
            explicit invariants(const texture_allocator& a)
                : m_a(a) {
            }

            void check(std::initializer_list <watched> handles) {
                const auto& s = m_a.stats();
                // Every texture made is held, pooled or destroyed.
                CHECK(s.created - s.destroyed == s.held + s.pooled);

                std::set <const texture*> held;
                std::unordered_map <dim, std::size_t> held_of;
                std::size_t held_bytes = 0;
                for (const auto& w : handles) {
                    if (!w.h) {
                        continue;                            // empty, failed or detached
                    }
                    const texture& t = w.h.get();
                    CHECK(held.insert(&t).second);           // no two handles share a texture
                    CHECK(t.size() == w.h.size());           // a live texture has its handle's size
                    if (w.h.needs_redraw()) {
                        CHECK(only(w.h, w.clear));           // ... and, needing a redraw, only
                    }                                        //     its clear colour
                    ++held_of[t.size()];
                    held_bytes += bytes(t.size());
                }
                // Every held texture is in exactly one live handle.
                CHECK(s.held == held.size());
                CHECK(s.held_bytes == held_bytes);

                for (const auto& [size, n] : held_of) {
                    m_peak[size] = std::max(m_peak[size], n);
                }
                std::size_t pooled = 0;
                std::size_t pooled_bytes = 0;
                for (const auto& [size, peak] : m_peak) {
                    const auto itr = held_of.find(size);
                    const std::size_t now_held = itr == held_of.end() ? 0 : itr->second;
                    const std::size_t now_pooled = m_a.pooled_count(size);
                    // A size never has more textures than the most of it held at once.
                    CHECK(now_held + now_pooled <= peak);
                    pooled += now_pooled;
                    pooled_bytes += now_pooled * bytes(size);
                }
                // Every pooled texture has a size some handle held.
                CHECK(s.pooled == pooled);
                CHECK(s.pooled_bytes == pooled_bytes);
            }

        private:
            const texture_allocator& m_a;
            std::unordered_map <dim, std::size_t> m_peak;
    };

    // The tests' software renderer has no texture size limit, so limit_texture_size() gives it
    // one, as a GPU renderer has: SDL reads the property each time it creates a texture and
    // refuses one wider or taller than it, before allocating anything.
    constexpr int LIMIT = 64;
    constexpr dim TOO_BIG{LIMIT + 1, 4};

    void limit_texture_size() {
        const auto props = get_renderer().get_properties();
        REQUIRE(props.has_value());
        REQUIRE(SDL_SetNumberProperty(*props, SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, LIMIT));
    }

    [[nodiscard]] SDL_Event event_of(SDL_EventType type) {
        SDL_Event ev{};
        ev.type = type;
        return ev;
    }

    // What the allocator looked like at each step of holding_application's life.
    struct lifecycle_log {
        bool live_on_enter{false};
        bool live_on_exit{false};
        std::size_t pooled_after_exit{0};
        bool live_in_teardown{false};
        std::size_t pooled_after_teardown{0};
    };

    lifecycle_log g_lifecycle;

    // Holds a target from on_enter() to on_exit().
    struct holding_scene final : base_scene {
        void on_enter() override {
            m_target = require_texture_allocator().acquire(texture_spec{MEDIUM, colors::red});
            g_lifecycle.live_on_enter = static_cast <bool>(m_target);
        }

        void on_exit() override {
            g_lifecycle.live_on_exit = static_cast <bool>(m_target);
            m_target = texture_handle{};
            g_lifecycle.pooled_after_exit = require_texture_allocator().stats().pooled;
        }

        void fixed_update(sim_duration, const input_snapshot&) override {}
        void render() override {}

        texture_handle m_target;
    };

    // Holds a target from ready() to teardown(), and runs holding_scene.
    class holding_application final : public application {
        public:
            using application::application;

        protected:
            void ready() override {
                m_target = require_texture_allocator().acquire(texture_spec{SMALL, colors::blue});
            }

            void teardown() override {
                g_lifecycle.live_in_teardown = static_cast <bool>(m_target);
                m_target = texture_handle{};
                g_lifecycle.pooled_after_teardown = require_texture_allocator().stats().pooled;
            }

            std::unique_ptr <base_scene> create_initial_scene() override {
                return std::make_unique <holding_scene>();
            }

        private:
            texture_handle m_target;
    };
}

TEST_SUITE("neutrino::detail::texture_allocator") {
    TEST_CASE("acquire creates a texture cleared to the spec's colour that needs a redraw") {
        test::test_application app("allocator acquire");
        auto& a = require_texture_allocator();
        invariants inv(a);

        auto h = a.acquire(texture_spec{MEDIUM, colors::red});
        REQUIRE(h);
        CHECK(h.size() == MEDIUM);
        CHECK(h.needs_redraw());
        inv.check({{h, colors::red}});

        const auto& s = a.stats();
        CHECK(s.created == 1);
        CHECK(s.held == 1);
        CHECK(s.held_bytes == bytes(MEDIUM));
        CHECK(s.pooled == 0);

        auto clear = a.acquire(texture_spec{SMALL, colors::transparent});
        REQUIRE(clear);
        inv.check({{h, colors::red}, {clear, colors::transparent}});

        h.mark_drawn();
        CHECK_FALSE(h.needs_redraw());
        inv.check({{h, colors::red}, {clear, colors::transparent}});
        h.clear();                                   // clearing does not change the flag
        CHECK_FALSE(h.needs_redraw());
        CHECK(only(h, colors::red));
        inv.check({{h, colors::red}, {clear, colors::transparent}});
    }

    TEST_CASE("get() gives the texture to draw into; a targets reset clears the drawing") {
        test::test_application app("allocator draw");
        auto& a = require_texture_allocator();
        invariants inv(a);

        auto h = a.acquire(texture_spec{MEDIUM, colors::red});
        h.get().compose([] { draw_rect_fill(rect{0, 0, 8, 8}, colors::blue); }, std::nullopt);
        h.mark_drawn();
        CHECK(only(h, colors::blue));
        inv.check({{h, colors::red}});

        a.on_targets_reset();
        CHECK(h.needs_redraw());
        CHECK(only(h, colors::red));                 // back to the clear colour
        inv.check({{h, colors::red}});

        texture_handle empty;
        CHECK_THROWS((void) empty.get());            // the non-const overload enforces too
    }

    TEST_CASE("a texture given back is reused at its size only, cleared to the new colour") {
        test::test_application app("allocator reuse");
        auto& a = require_texture_allocator();
        invariants inv(a);

        {
            auto h = a.acquire(texture_spec{MEDIUM, colors::red});
            inv.check({{h, colors::red}});
            h.mark_drawn();
        }
        CHECK(a.stats().held == 0);
        CHECK(a.stats().pooled == 1);
        CHECK(a.stats().pooled_bytes == bytes(MEDIUM));
        inv.check({});

        auto other = a.acquire(texture_spec{SMALL, colors::blue});   // another size: created
        CHECK(a.stats().created == 2);
        CHECK(a.stats().reused == 0);
        CHECK(a.stats().pooled == 1);
        inv.check({{other, colors::blue}});

        auto again = a.acquire(texture_spec{MEDIUM, colors::blue});  // same size: reused
        CHECK(a.stats().created == 2);
        CHECK(a.stats().reused == 1);
        CHECK(a.stats().pooled == 0);
        CHECK(a.stats().held == 2);
        CHECK(again.needs_redraw());
        CHECK(only(again, colors::blue));            // the old red never shows
        inv.check({{other, colors::blue}, {again, colors::blue}});
    }

    TEST_CASE("several textures of one size are pooled and reused one each") {
        test::test_application app("allocator same size");
        auto& a = require_texture_allocator();
        invariants inv(a);

        {
            auto h1 = a.acquire(texture_spec{MEDIUM, colors::red});
            auto h2 = a.acquire(texture_spec{MEDIUM, colors::red});
            auto h3 = a.acquire(texture_spec{MEDIUM, colors::blue});
            inv.check({{h1, colors::red}, {h2, colors::red}, {h3, colors::blue}});
        }
        CHECK(a.pooled_count(MEDIUM) == 3);
        inv.check({});

        auto r1 = a.acquire(texture_spec{MEDIUM, colors::green});
        auto r2 = a.acquire(texture_spec{MEDIUM, colors::green});
        CHECK(a.stats().created == 3);
        CHECK(a.stats().reused == 2);
        CHECK(a.pooled_count(MEDIUM) == 1);
        inv.check({{r1, colors::green}, {r2, colors::green}});    // two distinct textures

        a.drop_pooled(MEDIUM);                       // the one left in the pool
        CHECK(a.pooled_count(MEDIUM) == 0);
        CHECK(a.stats().destroyed == 1);
        inv.check({{r1, colors::green}, {r2, colors::green}});

        r1 = texture_handle{};
        r2 = texture_handle{};
        CHECK(a.pooled_count(MEDIUM) == 2);
        inv.check({});

        auto n1 = a.acquire(texture_spec{MEDIUM, colors::red});
        auto n2 = a.acquire(texture_spec{MEDIUM, colors::red});
        auto n3 = a.acquire(texture_spec{MEDIUM, colors::red});  // the pool is empty: created
        CHECK(a.stats().reused == 4);
        CHECK(a.stats().created == 4);
        inv.check({{n1, colors::red}, {n2, colors::red}, {n3, colors::red}});
    }

    TEST_CASE("moving moves the texture; assigning over a handle gives its own back first") {
        test::test_application app("allocator move");
        auto& a = require_texture_allocator();
        invariants inv(a);

        auto first = a.acquire(texture_spec{MEDIUM, colors::red});
        texture_handle moved = std::move(first);
        CHECK_FALSE(first);
        CHECK(first.size() == dim{0, 0});
        CHECK(first.needs_redraw());
        REQUIRE(moved);
        CHECK(moved.size() == MEDIUM);
        CHECK(a.stats().held == 1);
        inv.check({{first, colors::red}, {moved, colors::red}});

        auto second = a.acquire(texture_spec{SMALL, colors::blue});
        inv.check({{moved, colors::red}, {second, colors::blue}});
        moved = std::move(second);                   // moved's MEDIUM goes to the pool
        CHECK_FALSE(second);
        CHECK(moved.size() == SMALL);
        CHECK(a.stats().held == 1);
        CHECK(a.stats().pooled == 1);
        CHECK(a.stats().pooled_bytes == bytes(MEDIUM));
        inv.check({{moved, colors::blue}, {second, colors::blue}});
    }

    TEST_CASE("a degenerate size gives a failed handle, counted, that resize tries again") {
        test::test_application app("allocator failed");
        auto& a = require_texture_allocator();
        invariants inv(a);

        const auto failures = draw_failure_count();
        auto h = a.acquire(texture_spec{dim{0, 8}, colors::red});
        CHECK_FALSE(h);
        CHECK(draw_failure_count() == failures + 1);
        CHECK(h.size() == dim{0, 8});                // the size asked for
        CHECK(h.needs_redraw());
        h.mark_drawn();                              // nothing to mark
        CHECK(h.needs_redraw());
        CHECK_NOTHROW(h.clear());
        CHECK_THROWS((void) h.get());
        CHECK(a.stats().created == 0);
        CHECK(a.stats().held == 0);
        inv.check({{h, colors::red}});

        a.resize(h, MEDIUM);
        REQUIRE(h);
        CHECK(draw_failure_count() == failures + 1);
        CHECK(h.size() == MEDIUM);
        CHECK(h.needs_redraw());
        CHECK(a.stats().created == 1);
        CHECK(a.stats().held == 1);
        inv.check({{h, colors::red}});
    }

    TEST_CASE("a size SDL refuses gives a failed handle, counted, from acquire and resize") {
        test::test_application app("allocator refused");
        auto& a = require_texture_allocator();
        invariants inv(a);
        limit_texture_size();
        const dim big = TOO_BIG;

        const auto failures = draw_failure_count();
        auto h = a.acquire(texture_spec{big, colors::red});
        CHECK_FALSE(h);
        CHECK(h.size() == big);
        CHECK(h.needs_redraw());
        CHECK(draw_failure_count() == failures + 1);
        CHECK(a.stats().created == 0);
        inv.check({{h, colors::red}});

        auto live = a.acquire(texture_spec{MEDIUM, colors::blue});
        live.mark_drawn();
        inv.check({{h, colors::red}, {live, colors::blue}});
        a.resize(live, big);                         // failed at the new size, old one destroyed
        CHECK_FALSE(live);
        CHECK(live.size() == big);
        CHECK(live.needs_redraw());
        CHECK(draw_failure_count() == failures + 2);
        CHECK(a.stats().destroyed == 1);
        CHECK(a.stats().held == 0);
        inv.check({{h, colors::red}, {live, colors::blue}});

        a.resize(live, MEDIUM);                      // and tried again
        REQUIRE(live);
        CHECK(draw_failure_count() == failures + 2);
        inv.check({{h, colors::red}, {live, colors::blue}});
    }

    TEST_CASE("resize keeps the texture at the same size and replaces it at another") {
        test::test_application app("allocator resize");
        auto& a = require_texture_allocator();
        invariants inv(a);

        auto h = a.acquire(texture_spec{MEDIUM, colors::red});
        h.mark_drawn();
        a.resize(h, MEDIUM);                         // same size: contents and flag kept
        CHECK_FALSE(h.needs_redraw());
        CHECK(a.stats().created == 1);
        CHECK(a.stats().destroyed == 0);
        inv.check({{h, colors::red}});

        a.resize(h, WIDE);                           // another size: old destroyed, not pooled
        REQUIRE(h);
        CHECK(h.size() == WIDE);
        CHECK(h.needs_redraw());
        CHECK(a.stats().created == 2);
        CHECK(a.stats().destroyed == 1);
        CHECK(a.stats().pooled == 0);
        CHECK(a.stats().held == 1);
        CHECK(a.stats().held_bytes == bytes(WIDE));
        inv.check({{h, colors::red}});

        texture_handle empty;
        a.resize(empty, MEDIUM);                     // nothing to resize
        CHECK_FALSE(empty);
        CHECK(a.stats().created == 2);
        inv.check({{h, colors::red}, {empty, colors::red}});
    }

    TEST_CASE("drop_pooled destroys one size; a requested trim waits for begin_frame") {
        test::test_application app("allocator trim");
        auto& a = require_texture_allocator();
        invariants inv(a);

        {
            auto m = a.acquire(texture_spec{MEDIUM, colors::red});
            auto s = a.acquire(texture_spec{SMALL, colors::red});
            inv.check({{m, colors::red}, {s, colors::red}});
        }
        CHECK(a.stats().pooled == 2);
        inv.check({});

        a.drop_pooled(MEDIUM);
        CHECK(a.stats().pooled == 1);
        CHECK(a.stats().pooled_bytes == bytes(SMALL));
        CHECK(a.stats().destroyed == 1);
        inv.check({});
        a.drop_pooled(MEDIUM);                       // none left of that size
        CHECK(a.stats().destroyed == 1);
        inv.check({});

        a.begin_frame();                             // no trim requested: nothing happens
        CHECK(a.stats().pooled == 1);
        a.request_trim();
        CHECK(a.stats().pooled == 1);                // not before the next frame
        a.begin_frame();
        CHECK(a.stats().pooled == 0);
        CHECK(a.stats().pooled_bytes == 0);
        CHECK(a.stats().destroyed == 2);
        inv.check({});
        a.begin_frame();                             // the request was used up
        CHECK(a.stats().destroyed == 2);
        inv.check({});
    }

    TEST_CASE("the resets mark live handles; a device reset recreates them in place") {
        test::test_application app("allocator resets");
        auto& a = require_texture_allocator();
        invariants inv(a);

        {
            auto given_back = a.acquire(texture_spec{SMALL, colors::blue});
            inv.check({{given_back, colors::blue}});
        }
        auto h = a.acquire(texture_spec{MEDIUM, colors::red});
        h.mark_drawn();
        inv.check({{h, colors::red}});

        a.on_targets_reset();                        // contents lost; the pool is left alone
        CHECK(h.needs_redraw());
        CHECK(a.stats().pooled == 1);
        CHECK(a.stats().destroyed == 0);
        inv.check({{h, colors::red}});

        h.mark_drawn();
        a.on_device_reset();                         // pool destroyed, h recreated in place
        REQUIRE(h);
        CHECK(h.size() == MEDIUM);
        CHECK(h.needs_redraw());
        CHECK(a.stats().pooled == 0);
        CHECK(a.stats().destroyed == 2);             // the pooled one and h's old one
        CHECK(a.stats().created == 3);
        CHECK(a.stats().held == 1);
        inv.check({{h, colors::red}});
    }

    TEST_CASE("a device reset tries every failed handle again; a targets reset does not") {
        test::test_application app("allocator reset failed");
        auto& a = require_texture_allocator();
        invariants inv(a);

        limit_texture_size();
        auto refused = a.acquire(texture_spec{TOO_BIG, colors::red});
        auto zero = a.acquire(texture_spec{dim{0, 8}, colors::red});
        auto live = a.acquire(texture_spec{MEDIUM, colors::blue});
        live.mark_drawn();
        inv.check({{refused, colors::red}, {zero, colors::red}, {live, colors::blue}});

        const auto failures = draw_failure_count();
        a.on_targets_reset();                        // contents only: nothing is made
        CHECK(draw_failure_count() == failures);
        CHECK(live.needs_redraw());
        inv.check({{refused, colors::red}, {zero, colors::red}, {live, colors::blue}});

        live.mark_drawn();
        a.on_device_reset();
        CHECK(draw_failure_count() == failures + 2); // both failed handles tried, at their sizes
        CHECK_FALSE(refused);
        CHECK(refused.size() == TOO_BIG);
        CHECK(refused.needs_redraw());
        CHECK_FALSE(zero);
        CHECK(zero.size() == dim{0, 8});
        REQUIRE(live);
        CHECK(live.needs_redraw());
        inv.check({{refused, colors::red}, {zero, colors::red}, {live, colors::blue}});
    }

    TEST_CASE("the application's render reset events reach the allocator") {
        test::test_application app("allocator reset events");
        auto& a = require_texture_allocator();
        invariants inv(a);

        auto h = a.acquire(texture_spec{MEDIUM, colors::red});
        h.get().compose([] { draw_rect_fill(rect{0, 0, 8, 8}, colors::blue); }, std::nullopt);
        h.mark_drawn();
        inv.check({{h, colors::red}});

        app.feed(event_of(SDL_EVENT_RENDER_TARGETS_RESET));
        CHECK(h.needs_redraw());
        CHECK(only(h, colors::red));
        CHECK(a.stats().created == 1);
        inv.check({{h, colors::red}});

        h.mark_drawn();
        app.feed(event_of(SDL_EVENT_RENDER_DEVICE_RESET));
        REQUIRE(h);
        CHECK(h.needs_redraw());
        CHECK(a.stats().created == 2);
        CHECK(a.stats().destroyed == 1);
        inv.check({{h, colors::red}});
    }

    TEST_CASE("low memory trims the pool at the start of the next frame") {
        test::test_application app("allocator low memory");
        // No scene: a scene's frame would take a canvas texture after the trim (R41), one no
        // handle here watches.
        service_locator::instance().get_scenes_manager().finish();
        auto& a = require_texture_allocator();
        invariants inv(a);

        {
            auto given_back = a.acquire(texture_spec{MEDIUM, colors::red});
            inv.check({{given_back, colors::red}});
        }
        auto held = a.acquire(texture_spec{SMALL, colors::blue});
        CHECK(a.stats().pooled == 1);
        inv.check({{held, colors::blue}});

        app.feed(event_of(SDL_EVENT_LOW_MEMORY));
        CHECK(a.stats().pooled == 1);                // only requested
        app.iterate();                               // on_render -> begin_frame()
        CHECK(a.stats().pooled == 0);
        CHECK(a.stats().destroyed == 1);
        CHECK(held);                                 // held textures are untouched
        inv.check({{held, colors::blue}});
    }

    TEST_CASE("a trim request needs no allocator; a new allocator drops an old request") {
        CHECK(maybe_texture_allocator() == nullptr);
        CHECK_NOTHROW(texture_allocator::request_trim());          // nothing to reach: no fault

        test::test_application app("allocator stale request");
        auto& a = require_texture_allocator();
        {
            auto given_back = a.acquire(texture_spec{MEDIUM, colors::red});
        }
        CHECK(a.stats().pooled == 1);
        app.iterate();                                             // on_render -> begin_frame()
        CHECK(a.stats().pooled == 1);                              // the old request was dropped
        texture_allocator::request_trim();
        app.iterate();
        CHECK(a.stats().pooled == 0);
    }

    TEST_CASE("low-memory requests from another thread across the application's quit") {
        std::atomic <bool> stop{false};
        std::atomic <long> sent{0};
        std::thread low_memory([&] {
            while (!stop.load()) {
                texture_allocator::request_trim();                 // what on_low_memory() does
                sent.fetch_add(1);
            }
        });
        // Stopped and joined however the block below ends: a joinable thread's destructor
        // would end the whole test run.
        struct joiner {
            std::atomic <bool>& stop;
            std::thread& thread;
            ~joiner() {
                stop = true;
                if (thread.joinable()) {
                    thread.join();
                }
            }
        } const join_at_exit{stop, low_memory};
        {
            test::test_application app("allocator low memory race");
            {
                auto given_back = require_texture_allocator().acquire(texture_spec{MEDIUM, colors::red});
            }
            app.iterate();
            app.shutdown();                                        // destroys the allocator meanwhile
        }
        stop = true;
        low_memory.join();
        CHECK(sent.load() > 0);
        CHECK(maybe_texture_allocator() == nullptr);
    }

    TEST_CASE("texture_pool_stats() and trim_texture_pool() reach the application's allocator") {
        CHECK_THROWS((void) texture_pool_stats());   // no application
        CHECK_THROWS(trim_texture_pool());

        test::test_application app("allocator public");
        auto& a = require_texture_allocator();
        invariants inv(a);

        {
            auto given_back = a.acquire(texture_spec{MEDIUM, colors::red});
            inv.check({{given_back, colors::red}});
        }
        auto held = a.acquire(texture_spec{SMALL, colors::blue});
        inv.check({{held, colors::blue}});

        const pool_stats s = texture_pool_stats();
        CHECK(s.held == 1);
        CHECK(s.held_bytes == bytes(SMALL));
        CHECK(s.pooled == 1);
        CHECK(s.pooled_bytes == bytes(MEDIUM));
        CHECK(s.created == 2);
        CHECK(s.reused == 0);
        CHECK(s.destroyed == 0);

        trim_texture_pool();                         // at once, not at the next frame
        CHECK(texture_pool_stats().pooled == 0);
        CHECK(texture_pool_stats().destroyed == 1);
        CHECK(held);
        inv.check({{held, colors::blue}});
    }

    TEST_CASE("the allocator lives from before ready() until after teardown()") {
        g_lifecycle = lifecycle_log{};
        {
            test::basic_test_application <holding_application> app("allocator lifetime");
            CHECK(g_lifecycle.live_on_enter);        // published before the first scene enters
            CHECK(require_texture_allocator().stats().held == 2);

            app.shutdown();
            CHECK(g_lifecycle.live_on_exit);         // scenes exit with the allocator alive
            CHECK(g_lifecycle.pooled_after_exit == 1);
            CHECK(g_lifecycle.live_in_teardown);     // and so does teardown()
            CHECK(g_lifecycle.pooled_after_teardown == 2);
            CHECK(maybe_texture_allocator() == nullptr);
        }
    }

    TEST_CASE("empty() is true without a slot; a failed handle is not empty") {
        texture_handle detached;
        {
            test::test_application app("allocator empty");
            auto& a = require_texture_allocator();

            texture_handle none;
            CHECK(none.empty());

            auto live = a.acquire(texture_spec{MEDIUM, colors::red});
            CHECK_FALSE(live.empty());

            auto failed = a.acquire(texture_spec{dim{0, 8}, colors::red});
            CHECK_FALSE(failed);
            CHECK_FALSE(failed.empty());                     // keeps its slot and size

            texture_handle moved = std::move(live);
            CHECK(live.empty());
            CHECK_FALSE(moved.empty());
            moved = texture_handle{};                        // given back
            CHECK(moved.empty());

            detached = a.acquire(texture_spec{SMALL, colors::red});
            CHECK_FALSE(detached.empty());
        }
        CHECK(detached.empty());
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

TEST_SUITE("neutrino::detail::bind_pooled") {
    TEST_CASE("a texture taken from the pool is cleared once, as it is bound") {
        test::test_application app("bind pooled clear");
        auto& a = require_texture_allocator();
        {
            auto red = a.acquire(texture_spec{MEDIUM, colors::red});   // pooled with red in it
        }
        REQUIRE(a.stats().pooled == 1);
        texture_handle h;
        {
            auto bound = bind_pooled(h, texture_spec{MEDIUM, colors::transparent}, false, target_geometry{});
            REQUIRE(bound.has_value());
        }
        CHECK(a.stats().reused >= 1);
        const image img = h.get().to_image();
        const auto px = img.get_pixel(0, 0);
        REQUIRE(px.has_value());
        CHECK(px->a == 0);                         // not the previous owner's red
        CHECK(h.needs_redraw());
    }

    TEST_CASE("a kept texture is cleared only when asked") {
        test::test_application app("bind pooled keep");
        texture_handle h;
        {
            auto bound = bind_pooled(h, texture_spec{MEDIUM, colors::transparent}, false, target_geometry{});
            REQUIRE(bound.has_value());
            draw_rect_fill(rect{0, 0, 4, 4}, colors::red);
        }
        {
            auto bound = bind_pooled(h, texture_spec{MEDIUM, colors::transparent}, false, target_geometry{});
        }
        CHECK(h.get().to_image().get_pixel(1, 1)->r == 255);   // kept
        {
            auto bound = bind_pooled(h, texture_spec{MEDIUM, colors::transparent}, true, target_geometry{});
        }
        CHECK(h.get().to_image().get_pixel(1, 1)->a == 0);     // wiped
    }
}

TEST_SUITE("neutrino::detail::texture_table") {
    TEST_CASE("a freed index is reused, the most recently freed first") {
        // Each slot is told apart by its size; slots need no texture, so no application.
        const auto slot = [](int n) {
            return texture_slot{texture_spec{dim{n, n}, colors::red}, std::nullopt, true};
        };
        const auto sizes = [](texture_table& t) {
            std::vector <int> out;
            t.for_each([&](texture_slot& s) { out.push_back(s.spec.size.width); });
            return out;
        };

        texture_table_test_access::with_table([&](texture_table& t) {
            CHECK(t.add(slot(1)) == 0);
            CHECK(t.add(slot(2)) == 1);
            CHECK(t.add(slot(3)) == 2);
            CHECK(sizes(t) == std::vector{1, 2, 3});

            t.remove(1);
            t.remove(0);
            CHECK_FALSE(t.is_valid(0));
            CHECK_FALSE(t.is_valid(1));
            CHECK(t.is_valid(2));
            CHECK(sizes(t) == std::vector{3});       // for_each skips free slots

            CHECK(t.add(slot(4)) == 0);              // freed last, reused first
            CHECK(t.add(slot(5)) == 1);
            CHECK(t.add(slot(6)) == 3);              // none free: a new index at the end
            CHECK(sizes(t) == std::vector{4, 5, 3, 6});
            CHECK(t.get(1).spec.size == dim{5, 5});
        });
    }

    TEST_CASE("get and remove enforce an occupied index; is_valid only asks") {
        texture_table_test_access::with_table([](texture_table& t) {
            CHECK_FALSE(t.is_valid(0));
            CHECK_FALSE(t.is_valid(texture_table::INVALID_SLOT));

            const auto idx = t.add(texture_slot{texture_spec{SMALL, colors::red}, std::nullopt, true});
            CHECK(t.is_valid(idx));
            CHECK_FALSE(t.is_valid(idx + 1));
            CHECK_THROWS((void) t.get(idx + 1));      // out of range
            CHECK_THROWS(t.remove(idx + 1));

            t.remove(idx);
            CHECK_FALSE(t.is_valid(idx));
            CHECK_THROWS((void) t.get(idx));          // free
            CHECK_THROWS(t.remove(idx));              // freed twice
            const auto& ct = t;
            CHECK_THROWS((void) ct.get(idx));
        });
    }
}
