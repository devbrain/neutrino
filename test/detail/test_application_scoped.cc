//
// Tests for application_scoped<T> and the application scope (services/application_scoped.hh).
//
// Most cases open and close the scope by hand, as application::on_ready() / on_quit() do;
// the last one runs a real application.
//
#include <doctest/doctest.h>

#include "test_application.hh"
#include "services/application_scoped.hh"

#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using neutrino::detail::application_scope_is_open;
using neutrino::detail::application_scoped;
using neutrino::detail::at_quit;
using neutrino::detail::close_application_scope;
using neutrino::detail::open_application_scope;

namespace {
    // What the test types did, in order: "+name" when made, "-name" when destroyed.
    std::vector <std::string> g_log;

    struct first {
        first() { g_log.emplace_back("+first"); }
        ~first() { g_log.emplace_back("-first"); }
    };

    struct second {
        second() { g_log.emplace_back("+second"); }
        ~second() { g_log.emplace_back("-second"); }
    };

    // Uses first in its constructor, so first is made first and destroyed last.
    struct dependent {
        dependent() {
            (void) application_scoped <first>::instance();
            g_log.emplace_back("+dependent");
        }
        ~dependent() {
            // During quit: first is still alive, dependent itself already reads as gone.
            g_log.emplace_back(application_scoped <first>::maybe() ? "-dependent(first alive)"
                                                                   : "-dependent(first gone)");
            g_log.emplace_back(application_scoped <dependent>::maybe() ? "-dependent(self visible)"
                                                                       : "-dependent(self gone)");
        }
    };

    std::atomic <int> g_made{0};

    struct counted {
        counted() { g_made.fetch_add(1); }
    };

    // Opens a fresh scope for one case and closes it at the end, whatever happens.
    struct scope {
        scope() {
            g_log.clear();
            open_application_scope();
        }
        ~scope() {
            close_application_scope();
        }
    };
}

TEST_SUITE("neutrino::detail::application_scoped") {
    TEST_CASE("outside the scope nothing is made") {
        close_application_scope();
        CHECK_FALSE(application_scope_is_open());
        CHECK(application_scoped <first>::maybe() == nullptr);
        CHECK_THROWS((void) application_scoped <first>::instance());
        CHECK(application_scoped <first>::maybe() == nullptr);
    }

    TEST_CASE("instance() makes the object once; maybe() sees it") {
        scope s;
        CHECK(application_scoped <first>::maybe() == nullptr);
        first& a = application_scoped <first>::instance();
        first& b = application_scoped <first>::instance();
        CHECK(&a == &b);
        CHECK(application_scoped <first>::maybe() == &a);
        CHECK(g_log == std::vector <std::string>{"+first"});
    }

    TEST_CASE("closing the scope destroys newest first") {
        {
            scope s;
            (void) application_scoped <first>::instance();
            (void) application_scoped <second>::instance();
        }
        CHECK(g_log == std::vector <std::string>{"+first", "+second", "-second", "-first"});
        CHECK(application_scoped <first>::maybe() == nullptr);
        CHECK(application_scoped <second>::maybe() == nullptr);
    }

    TEST_CASE("a dependency made in a constructor outlives what made it") {
        {
            scope s;
            (void) application_scoped <dependent>::instance();
        }
        CHECK(g_log == std::vector <std::string>{
            "+first", "+dependent",
            "-dependent(first alive)", "-dependent(self gone)",
            "-first"});
    }

    TEST_CASE("after quit nothing is made; the next scope gets a new object") {
        g_made = 0;
        {
            scope s;
            (void) application_scoped <counted>::instance();
            CHECK(g_made == 1);
        }
        CHECK(application_scoped <counted>::maybe() == nullptr);
        CHECK_THROWS((void) application_scoped <counted>::instance());
        CHECK(g_made == 1);
        {
            scope s;
            (void) application_scoped <counted>::instance();
            CHECK(g_made == 2);
        }
    }

    TEST_CASE("at_quit is refused once the scope is closed, so nothing waits for the next one") {
        bool ran = false;
        close_application_scope();
        CHECK_THROWS(at_quit([&] { ran = true; }));
        {
            scope s;                                   // the next application's scope
        }
        CHECK_FALSE(ran);                              // nothing was left behind to run here
    }

    TEST_CASE("a callback that throws is logged and the others still run") {
        std::vector <int> ran;
        {
            scope s;
            at_quit([&] { ran.push_back(1); });
            at_quit([] { throw std::runtime_error("boom"); });
            at_quit([&] { ran.push_back(3); });
        }
        CHECK(ran == std::vector <int>{3, 1});
        close_application_scope();                     // again: nothing left to run
        CHECK(ran == std::vector <int>{3, 1});
    }

    TEST_CASE("concurrent first use makes one object") {
        g_made = 0;
        scope s;
        std::vector <counted*> seen(8, nullptr);
        std::vector <std::thread> threads;
        for (std::size_t i = 0; i < seen.size(); ++i) {
            threads.emplace_back([&seen, i] { seen[i] = &application_scoped <counted>::instance(); });
        }
        for (auto& t : threads) {
            t.join();
        }
        CHECK(g_made == 1);
        for (auto* p : seen) {
            CHECK(p == seen.front());
        }
    }

    TEST_CASE("an application opens the scope in on_ready and closes it in on_quit") {
        close_application_scope();
        {
            neutrino::test::test_application app("application scoped");
            CHECK(application_scope_is_open());
            (void) application_scoped <first>::instance();
            CHECK(application_scoped <first>::maybe() != nullptr);
            app.shutdown();
            CHECK_FALSE(application_scope_is_open());
            CHECK(application_scoped <first>::maybe() == nullptr);
        }
        CHECK_FALSE(application_scope_is_open());
    }
}
