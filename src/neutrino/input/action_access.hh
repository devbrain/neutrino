//
// Created by igor on 26/11/2026.
//

#pragma once

#include <vector>

#include <neutrino/input/action.hh>

namespace neutrino::detail {
    /// @brief What the engine does with an action that a game does not: action_bindings loads
    ///        saved bindings without making them the defaults, and writes only bound actions.
    struct action_access {
        static void load(action_handle& a, const std::vector <binding>& bindings) {
            a.rebind_loaded(bindings);
        }

        [[nodiscard]] static bool bound(const action_handle& a) {
            return a.bound();
        }
    };
}
