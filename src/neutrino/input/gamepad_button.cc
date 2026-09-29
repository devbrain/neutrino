#include <neutrino/input/gamepad_button.hh>
#include <neutrino/input/input_snapshot.hh>

namespace neutrino {

    bool gamepad_button::pressed(const input_snapshot& in) const noexcept {
        return in.gamepad_button_state(m_gamepad_index, m_button).pressed;
    }

    bool gamepad_button::held(const input_snapshot& in) const noexcept {
        return in.gamepad_button_state(m_gamepad_index, m_button).held;
    }

    bool gamepad_button::released(const input_snapshot& in) const noexcept {
        return in.gamepad_button_state(m_gamepad_index, m_button).released;
    }

} // namespace neutrino
