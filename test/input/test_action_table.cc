//
// Created by igor on 25/11/2026.
//

#include <doctest/doctest.h>

#include <neutrino/input/hotkey.hh>
#include <neutrino/input/input_snapshot.hh>
#include <neutrino/input/keys.hh>
#include "input/action_tables.hh"

#include <failsafe/logger/backend/capture_backend.hh>

#include <variant>

namespace {
    using neutrino::button_state;
    using neutrino::hotkey;
    using neutrino::input_snapshot;
    using neutrino::key;
    using neutrino::detail::action_table;

    [[nodiscard]] input_snapshot holding(key k) {
        input_snapshot in;
        in.set_key(k, button_state{.pressed = false, .released = false, .held = true});
        return in;
    }
}

TEST_SUITE("neutrino::detail::action_table") {
    TEST_CASE("bind(id, activation) rebinds a record in the table, and adds none for another id") {
        action_table table;
        const auto in = holding(key::space);

        const auto live = table.bind(hotkey{key::a});
        CHECK_FALSE(table.check(live, in).held);
        table.bind(live, hotkey{key::space});
        CHECK(table.check(live, in).held);

        const auto removed = table.bind(hotkey{key::a});
        table.unbind(removed);
        CHECK_THROWS(table.bind(removed, hotkey{key::space}));
        CHECK_FALSE(table.check(removed, in).held);

        CHECK_THROWS(table.bind(neutrino::action_id{}, hotkey{key::space}));
    }

    TEST_CASE("an action with no activation is logged once, not at every check") {
        action_table table;
        const input_snapshot in;
        const auto first = table.bind(std::monostate{});
        const auto second = table.bind(std::monostate{});

        failsafe::logger::backends::scoped_log_capture logs;
        for (int tick = 0; tick < 3; ++tick) {
            CHECK_FALSE(table.check(first, in).held);
        }
        CHECK(logs.count("is undefined") == 1);

        CHECK_FALSE(table.check(second, in).held);   // its own record: logged once too
        CHECK(logs.count("is undefined") == 2);
    }
}
