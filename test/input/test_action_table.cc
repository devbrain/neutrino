//
// Created by igor on 25/11/2026.
//

#include <doctest/doctest.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <failsafe/logger/backend/capture_backend.hh>

#include <neutrino/input/action.hh>
#include <neutrino/input/binding.hh>
#include <neutrino/input/input_snapshot.hh>
#include <neutrino/input/keys.hh>
#include "input/action_tables.hh"

using namespace neutrino;

namespace {
    using detail::action_entry;
    using failsafe::logger::backends::scoped_log_capture;

    constexpr button_state down{.pressed = true, .released = false, .held = true};
    constexpr button_state held_on{.pressed = false, .released = false, .held = true};

    [[nodiscard]] input_snapshot pressing(std::initializer_list <key> keys) {
        input_snapshot in;
        for (const auto k : keys) {
            in.set_key(k, down);
        }
        return in;
    }

    [[nodiscard]] input_snapshot holding(key k) {
        input_snapshot in;
        in.set_key(k, held_on);
        return in;
    }

    [[nodiscard]] action_entry bound(std::vector <binding> bindings) {
        action_entry e;
        e.bindings = std::move(bindings);
        e.bound = true;
        return e;
    }

    [[nodiscard]] action_entry global(std::vector <binding> bindings, std::function <void()> fire) {
        auto e = bound(std::move(bindings));
        e.global = true;
        e.fire = std::make_shared <const std::function <void()>>(std::move(fire));
        return e;
    }
}

TEST_SUITE("neutrino::detail::action_table") {
    TEST_CASE("the first bindings a record is given are its defaults") {
        detail::action_table table;
        const auto made = table.add(bound({key::a}));
        CHECK(table.rebind(made, {key::b}));
        CHECK(table.find(made)->bindings == std::vector <binding>{key::b});
        CHECK(table.find(made)->defaults == std::vector <binding>{key::a});
        CHECK(table.reset(made));
        CHECK(table.find(made)->bindings == std::vector <binding>{key::a});

        const auto later = table.add(action_entry{});   // given a description only, say
        CHECK(table.rebind(later, {key::c}));
        CHECK(table.rebind(later, {key::d}));
        CHECK(table.find(later)->defaults == std::vector <binding>{key::c});

        CHECK_FALSE(table.rebind(action_id{}, {key::e}));
        CHECK_FALSE(table.reset(action_id{}));
    }

    TEST_CASE("check: nothing for a missing or never-bound record, nothing held while disabled") {
        detail::action_table table;
        const auto in = holding(key::space);
        CHECK_FALSE(table.check(action_id{}, in).has_value());

        const auto unbound = table.add(action_entry{});
        CHECK_FALSE(table.check(unbound, in).has_value());

        const auto live = table.add(bound({key::space}));
        CHECK(table.check(live, in)->held);
        CHECK(table.set_enabled(live, false));
        REQUIRE(table.check(live, in).has_value());
        CHECK_FALSE(table.check(live, in)->held);

        // A disabled record answers (nothing held) rather than "never set".
        CHECK(table.set_enabled(unbound, false));
        CHECK(table.check(unbound, in).has_value());
    }

    TEST_CASE("records are listed in the order they were registered, once bound (a global one with its callback)") {
        detail::action_table table;
        const auto first = table.add(bound({key::a}));
        const auto unbound = table.add(action_entry{});
        auto no_callback = bound({key::b});
        no_callback.global = true;
        (void)table.add(std::move(no_callback));
        const auto fires = table.add(global({key::c}, [] {}));
        const auto none = table.add(bound({}));   // deliberately unbound: listed
        CHECK(table.listed() == std::vector{first, fires, none});

        CHECK(table.rebind(unbound, {key::d}));
        CHECK(table.listed() == std::vector{first, unbound, fires, none});
    }

    TEST_CASE("removing a record not in the table is logged") {
        detail::action_table table;
        const auto id = table.add(bound({key::a}));
        table.remove(id);
        scoped_log_capture logs(LOGGER_LEVEL_WARN);
        table.remove(id);
        CHECK(logs.count("non existent") == 1);
    }

    TEST_CASE("merge() moves the records: the source is left without them") {
        detail::action_table pending;
        detail::action_table live;
        const auto in = holding(key::space);
        const auto id = pending.add(bound({key::space}));

        live.merge(std::move(pending));
        CHECK(live.check(id, in)->held);
        // merge() leaves the source empty, and usable: a copy would come back in the next merge.
        CHECK_FALSE(pending.check(id, in).has_value());   // NOLINT(bugprone-use-after-move)

        live.merge(std::move(pending));                   // NOLINT(bugprone-use-after-move)
        CHECK(live.check(id, in)->held);
    }
}

TEST_SUITE("neutrino::detail::action_table global actions") {
    TEST_CASE("a global action fires once on the press and takes its edges; holding does not repeat it") {
        detail::action_table table;
        int fired = 0;
        (void)table.add(global({key::f11}, [&fired] { ++fired; }));

        const auto taken = table.fire_globals(pressing({key::f11}));
        CHECK(fired == 1);
        REQUIRE(taken.has_value());
        CHECK_FALSE(taken->key(key::f11).pressed);
        CHECK(taken->key(key::f11).held);   // only the edges are taken

        CHECK_FALSE(table.fire_globals(holding(key::f11)).has_value());
        CHECK(fired == 1);
    }

    TEST_CASE("when nothing fires, the input is left as it is") {
        detail::action_table table;
        (void)table.add(global({key::f11}, [] {}));
        CHECK_FALSE(table.fire_globals(pressing({key::a})).has_value());
    }

    TEST_CASE("only the edges of the bindings that fired are taken") {
        detail::action_table table;
        (void)table.add(global({mods::ctrl + key::s, key::f5}, [] {}));
        auto in = pressing({key::s, key::b});
        in.set_modifiers(mods::lctrl);
        const auto taken = table.fire_globals(in);
        REQUIRE(taken.has_value());
        CHECK_FALSE(taken->key(key::s).pressed);   // Ctrl+S fired
        CHECK(taken->key(key::b).pressed);         // B is the scenes'
    }

    TEST_CASE("two on one input: the first registered takes it") {
        detail::action_table table;
        int first = 0;
        int second = 0;
        (void)table.add(global({key::f11}, [&first] { ++first; }));
        (void)table.add(global({key::f11}, [&second] { ++second; }));
        (void)table.fire_globals(pressing({key::f11}));
        CHECK(first == 1);
        CHECK(second == 0);
    }

    TEST_CASE("a disabled global action does not fire and takes nothing: another on the same input fires") {
        detail::action_table table;
        int first = 0;
        int second = 0;
        const auto off = table.add(global({key::f11}, [&first] { ++first; }));
        (void)table.add(global({key::f11}, [&second] { ++second; }));
        CHECK(table.set_enabled(off, false));
        (void)table.fire_globals(pressing({key::f11}));
        CHECK(first == 0);
        CHECK(second == 1);
    }

    TEST_CASE("while suspended, no global action fires and they take nothing; suspensions nest") {
        detail::action_table table;
        int fired = 0;
        (void)table.add(global({key::grave}, [&fired] { ++fired; }));
        {
            const global_actions_suspended outer;
            {
                const global_actions_suspended inner;
                CHECK(global_actions_suspended::active());
            }
            CHECK(global_actions_suspended::active());
            CHECK_FALSE(table.fire_globals(pressing({key::grave})).has_value());
            CHECK(fired == 0);
        }
        CHECK_FALSE(global_actions_suspended::active());
        (void)table.fire_globals(pressing({key::grave}));
        CHECK(fired == 1);
    }

    TEST_CASE("a throwing callback is logged and contained; the others still fire") {
        detail::action_table table;
        int fired = 0;
        (void)table.add(global({key::f11}, [] { throw std::runtime_error("callback failed"); }));
        (void)table.add(global({key::f12}, [&fired] { ++fired; }));
        scoped_log_capture logs;
        (void)table.fire_globals(pressing({key::f11, key::f12}));
        CHECK(fired == 1);
        CHECK(logs.count("callback failed") == 1);
    }

    TEST_CASE("a callback may remove and add records: a removed one is skipped, a new one is checked next time") {
        detail::action_table table;
        int removed_fired = 0;
        int added_fired = 0;
        action_id removed;
        (void)table.add(global({key::f11}, [&] {
            table.remove(removed);
            (void)table.add(global({key::f12}, [&added_fired] { ++added_fired; }));
        }));
        removed = table.add(global({key::f12}, [&removed_fired] { ++removed_fired; }));

        (void)table.fire_globals(pressing({key::f11, key::f12}));
        CHECK(removed_fired == 0);
        CHECK(added_fired == 0);

        (void)table.fire_globals(pressing({key::f12}));
        CHECK(added_fired == 1);
    }
}
