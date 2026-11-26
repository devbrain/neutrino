//
// Created by igor on 26/11/2026.
//

#pragma once

#include <initializer_list>
#include <iosfwd>
#include <string>
#include <variant>
#include <vector>

#include <neutrino/neutrino_export.h>

namespace neutrino {
    class action;
    class global_action;

    /**
     * @brief Named actions, for saving and loading their bindings on any stream:
     * @code
     * action_bindings b{{"left", left}, {"fire", fire}, {"fullscreen", m_fullscreen}};
     * std::ofstream out(path);   // the game chooses the stream, e.g. under sdlpp's get_pref_path
     * out << b;
     * std::ifstream in(path);
     * in >> b;
     * @endcode
     *
     * The game names its actions only here. Writing gives one `name = bindings` line per action
     * (the text form of binding.hh), except an action never given bindings; a keycode binding
     * that maps to no key under the current layout is logged. Reading takes the whole stream,
     * then rebinds each listed action, whatever the order of the lines; the bindings read are
     * not the defaults, so resetting an action still goes back to the game's. A bad or missing
     * line keeps the old bindings and is logged; a line for a name not listed is logged and
     * skipped; blank lines and lines starting with '#' are skipped. Reading never throws on bad
     * text, and does not change the stream's state beyond what reading it to the end does. The
     * actions must outlive the list.
     */
    class NEUTRINO_EXPORT action_bindings {
        public:
            /// @brief One named action. ENFORCE (in action_bindings' constructor): the name is not
            ///        empty, has no '=', '#', line break or blank at either end, and is listed once.
            struct entry {
                entry(std::string name, action& target);
                entry(std::string name, global_action& target);

                std::string name;
                std::variant <action*, global_action*> target;
            };

            action_bindings(std::initializer_list <entry> entries);

            [[nodiscard]] const std::vector <entry>& entries() const noexcept {
                return m_entries;
            }

        private:
            std::vector <entry> m_entries;
    };

    /// @brief Writes one `name = bindings` line per listed action.
    NEUTRINO_EXPORT std::ostream& operator<<(std::ostream& out, const action_bindings& bindings);

    /// @brief Reads the stream to its end and rebinds the listed actions (see action_bindings).
    NEUTRINO_EXPORT std::istream& operator>>(std::istream& in, const action_bindings& bindings);
}
