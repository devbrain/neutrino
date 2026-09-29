#include <neutrino/input/mouse_click.hh>
#include <neutrino/input/input_snapshot.hh>
#include "modifier_match.hh"

namespace neutrino {

    namespace {
        bool check_state(const input_snapshot& in, modifier mods, sdlpp::mouse_button button, bool button_state::*member) noexcept {
            auto state = in.mouse(button);
            if (!(state.*member)) {
                return false;
            }
            return input_detail::match_modifiers(mods, in.modifiers());
        }
    }

    bool mouse_click::pressed(const input_snapshot& in) const noexcept {
        return check_state(in, m_mods, m_button, &button_state::pressed);
    }

    bool mouse_click::held(const input_snapshot& in) const noexcept {
        return check_state(in, m_mods, m_button, &button_state::held);
    }

    bool mouse_click::released(const input_snapshot& in) const noexcept {
        return check_state(in, m_mods, m_button, &button_state::released);
    }

} // namespace neutrino
