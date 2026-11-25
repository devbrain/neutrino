//
// Created by igor on 24/11/2026.
//

#pragma once

#include <string>
#include <optional>
#include <unordered_map>

#include <neutrino/input/action_id.hh>
#include <neutrino/input/input_snapshot.hh>
#include "input/action_activation.hh"


namespace neutrino::detail {
    struct action_entry {
        action_activation activation;
        bool enabled{true};
        std::string description;
        action_id   handle;
        /// check() found no activation and logged it; it logs once per record, not per tick.
        mutable bool undefined_reported{false};
    };

    class action_table {
        public:
            action_table();
            ~action_table();

            action_id bind(action_activation&& act);
            void bind(action_id key, action_activation&& act);
            void unbind(action_id key);

            void description(action_id key, const std::string& descr);

            button_state check(action_id key, const input_snapshot& in) const;
        private:
            std::unordered_map<action_id, action_entry> m_table;
    };
}