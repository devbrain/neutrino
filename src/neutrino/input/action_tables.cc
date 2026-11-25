//
// Created by igor on 24/11/2026.
//

#include "action_tables.hh"
#include "detail/generic_handle_factory.hh"
#include <neutrino/detail/overloaded.hh>
#include <failsafe/failsafe.hh>

namespace neutrino::detail {
    action_table::action_table() = default;

    action_table::~action_table() = default;

    action_id action_table::bind(action_activation&& act) {
        auto key = make_handle <action_id>();
        m_table.emplace(key, action_entry{.activation = act, .handle = key});
        return key;
    }

    void action_table::bind(action_id key, action_activation&& act) {
        auto itr = m_table.find(key);
        ENFORCE(itr != m_table.end(), "action ", key, "not found");
        itr->second.activation = act;
    }

    void action_table::unbind(action_id key) {
        if (!key.valid()) {
            return;
        }
        auto itr = m_table.find(key);
        if (itr != m_table.end()) {
            m_table.erase(itr);
        } else {
            LOG_WARN("Attempt to unbind non existent action ", key);
        }
    }

    void action_table::description(action_id key, const std::string& descr) {
        auto itr = m_table.find(key);
        ENFORCE(itr != m_table.end(), "action ", key, "not found");
        itr->second.description = descr;
    }

    template<typename T>
    static button_state to_button_state(const T& x, const input_snapshot& in) {
        return {.pressed = in.pressed(x), .released = in.released(x), .held = in.held(x)};
    }

    button_state action_table::check(action_id key, const input_snapshot& in) const {
        if (!key.valid()) {
            return {};
        }
        auto itr = m_table.find(key);
        if (itr == m_table.end()) {
            return {};
        }
        const auto& e = itr->second;
        if (!e.enabled) {
            return {};
        }

        return std::visit(overloaded{
                              [&in](const hotkey& x) { return to_button_state(x, in); },
                              [&in](const mouse_click& x) { return to_button_state(x, in); },
                              [&in](const gamepad_button& x) { return to_button_state(x, in); },
                              [&key, &e](const std::monostate&) {
                                  if (!e.undefined_reported) {
                                      e.undefined_reported = true;
                                      LOG_ERROR("action ", key, " is undefined");
                                  }
                                  return button_state{};
                              }
                          }, e.activation);
    }
}
