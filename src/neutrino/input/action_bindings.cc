//
// Created by igor on 26/11/2026.
//

#include <neutrino/input/action_bindings.hh>

#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include <neutrino/input/action.hh>
#include <neutrino/input/binding.hh>
#include <neutrino/input/input_snapshot.hh>
#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>

#include "action_access.hh"
#include "binding_text.hh"

namespace neutrino {
    namespace {
        // The list holds the actions by pointer: a const list still rebinds them.
        [[nodiscard]] detail::action_handle& handle_of(const action_bindings::entry& e) {
            return std::visit([](auto* target) -> detail::action_handle& { return *target; }, e.target);
        }

        // A keycode hotkey that maps to no key under the current layout is written as
        // "Scancode 0": it reads back, and binds nothing.
        [[nodiscard]] bool binds_no_key(const binding& b) {
            const auto* h = std::get_if <hotkey>(&b);
            if (h == nullptr) {
                return false;
            }
            const auto code = static_cast <int>(h->resolve_scancode());
            return code <= 0 || code >= static_cast <int>(input_snapshot::max_scancodes);
        }
    }

    action_bindings::entry::entry(std::string name, action& target)
        : name(std::move(name)),
          target(&target) {
    }

    action_bindings::entry::entry(std::string name, global_action& target)
        : name(std::move(name)),
          target(&target) {
    }

    action_bindings::action_bindings(std::initializer_list <entry> entries)
        : m_entries(entries) {
        for (std::size_t i = 0; i < m_entries.size(); ++i) {
            const auto& name = m_entries[i].name;
            ENFORCE(!name.empty() && name.find_first_of("=#\r\n") == std::string::npos
                    && input_detail::trim(name) == name,
                    "action_bindings: the action name '", name, "' cannot be written as a line");
            for (std::size_t j = 0; j < i; ++j) {
                ENFORCE(m_entries[j].name != name, "action_bindings: the action name '", name, "' is listed twice");
            }
        }
    }

    std::ostream& operator<<(std::ostream& out, const action_bindings& bindings) {
        for (const auto& e : bindings.entries()) {
            const auto& handle = handle_of(e);
            // One never given bindings has nothing to save: its line would read back as
            // "deliberately unbound".
            if (!detail::action_access::bound(handle)) {
                continue;
            }
            const auto list = handle.bindings();
            for (const auto& b : list) {
                if (binds_no_key(b)) {
                    LOG_WARN("A binding of", e.name, "maps to no key under this keyboard layout; saved as",
                             to_string(b));
                }
            }
            out << e.name << " = " << to_string(list) << '\n';
        }
        return out;
    }

    std::istream& operator>>(std::istream& in, const action_bindings& bindings) {
        const auto& entries = bindings.entries();
        std::vector <bool> seen(entries.size(), false);
        std::string line;
        while (std::getline(in, line)) {
            const auto text = input_detail::trim(line);
            if (text.empty() || text.front() == '#') {
                continue;
            }
            // The name ends at the first '='; the bindings may contain '=' (the '=' key).
            const auto equals = text.find('=');
            if (equals == std::string_view::npos) {
                LOG_WARN("A bindings line without '=' is skipped:", std::string{text});
                continue;
            }
            const auto name = input_detail::trim(text.substr(0, equals));
            const auto value = input_detail::trim(text.substr(equals + 1));
            std::size_t index = 0;
            while (index < entries.size() && entries[index].name != name) {
                ++index;
            }
            if (index == entries.size()) {
                LOG_WARN("Bindings for an action not listed are skipped:", std::string{name});
                continue;
            }
            seen[index] = true;
            const auto parsed = bindings_from_string(value);
            if (!parsed) {
                LOG_WARN("Malformed bindings for", std::string{name}, "- the old ones are kept:", std::string{value});
                continue;
            }
            // Saved bindings are not the game's defaults: reset() still goes back to those.
            detail::action_access::load(handle_of(entries[index]), *parsed);
        }
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (!seen[i]) {
                LOG_WARN("No bindings for", entries[i].name, "- the old ones are kept");
            }
        }
        return in;
    }
}
