#include <neutrino/input/input_snapshot.hh>
#include <SDL3/SDL_keyboard.h>
#include <algorithm>

namespace neutrino {

    input_snapshot::input_snapshot(pointer_state pointer,
                                   button_state left,
                                   button_state middle,
                                   button_state right) noexcept
        : m_pointer(pointer), m_left(left), m_middle(middle), m_right(right) {
    }

    input_snapshot input_snapshot::without_edges() const noexcept {
        input_snapshot copy = *this;
        copy.m_left.pressed = false;
        copy.m_left.released = false;
        copy.m_middle.pressed = false;
        copy.m_middle.released = false;
        copy.m_right.pressed = false;
        copy.m_right.released = false;
        copy.m_x1.pressed = false;
        copy.m_x1.released = false;
        copy.m_x2.pressed = false;
        copy.m_x2.released = false;
        copy.m_wheel = 0;

        copy.m_keys_pressed.reset();
        copy.m_keys_released.reset();

        for (auto& gp : copy.m_gamepads) {
            gp.pressed.reset();
            gp.released.reset();
        }

        return copy;
    }

    button_state input_snapshot::mouse(sdlpp::mouse_button b) const noexcept {
        switch (b) {
            case sdlpp::mouse_button::left:   return m_left;
            case sdlpp::mouse_button::middle: return m_middle;
            case sdlpp::mouse_button::right:  return m_right;
            case sdlpp::mouse_button::x1:     return m_x1;
            case sdlpp::mouse_button::x2:     return m_x2;
            default:                          return {};
        }
    }

    void input_snapshot::set_mouse(sdlpp::mouse_button b, button_state s) noexcept {
        switch (b) {
            case sdlpp::mouse_button::left:   m_left = s; break;
            case sdlpp::mouse_button::middle: m_middle = s; break;
            case sdlpp::mouse_button::right:  m_right = s; break;
            case sdlpp::mouse_button::x1:     m_x1 = s; break;
            case sdlpp::mouse_button::x2:     m_x2 = s; break;
            default:                          break;
        }
    }

    button_state input_snapshot::key(sdlpp::scancode scan) const noexcept {
        const auto idx = static_cast<std::size_t>(scan);
        if (idx >= max_scancodes) {
            return {};
        }
        return button_state{
            m_keys_pressed.test(idx),
            m_keys_released.test(idx),
            m_keys_held.test(idx)
        };
    }

    button_state input_snapshot::key(sdlpp::keycode code) const noexcept {
        SDL_Scancode scan = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(code), nullptr);
        return key(static_cast<sdlpp::scancode>(scan));
    }

    void input_snapshot::set_key(sdlpp::scancode scan, button_state s) noexcept {
        const auto idx = static_cast<std::size_t>(scan);
        if (idx >= max_scancodes) {
            return;
        }
        m_keys_pressed.set(idx, s.pressed);
        m_keys_released.set(idx, s.released);
        m_keys_held.set(idx, s.held);
    }

    button_state input_snapshot::gamepad_button_state(int slot, sdlpp::gamepad_button button) const noexcept {
        if (slot < 0 || static_cast<std::size_t>(slot) >= max_gamepad_slots) {
            return {};
        }
        const auto& gp = m_gamepads[static_cast<std::size_t>(slot)];
        if (!gp.connected) {
            return {};
        }
        const auto btn_idx = static_cast<std::size_t>(button);
        if (btn_idx >= max_gamepad_buttons) {
            return {};
        }
        return button_state{
            gp.pressed.test(btn_idx),
            gp.released.test(btn_idx),
            gp.held.test(btn_idx)
        };
    }

    float input_snapshot::gamepad_axis(int slot, sdlpp::gamepad_axis axis) const noexcept {
        if (slot < 0 || static_cast<std::size_t>(slot) >= max_gamepad_slots) {
            return 0.0f;
        }
        const auto& gp = m_gamepads[static_cast<std::size_t>(slot)];
        if (!gp.connected) {
            return 0.0f;
        }
        const auto axis_idx = static_cast<std::size_t>(axis);
        if (axis_idx >= max_gamepad_axes) {
            return 0.0f;
        }
        return gp.axes[axis_idx];
    }

    bool input_snapshot::gamepad_connected(int slot) const noexcept {
        if (slot < 0 || static_cast<std::size_t>(slot) >= max_gamepad_slots) {
            return false;
        }
        return m_gamepads[static_cast<std::size_t>(slot)].connected;
    }

    const input_snapshot::gamepad_slot_state& input_snapshot::gamepad(int slot) const noexcept {
        static const gamepad_slot_state empty{};
        if (slot < 0 || static_cast<std::size_t>(slot) >= max_gamepad_slots) {
            return empty;
        }
        return m_gamepads[static_cast<std::size_t>(slot)];
    }

    void input_snapshot::set_gamepad_button(int slot, sdlpp::gamepad_button btn, button_state s) noexcept {
        if (slot < 0 || static_cast<std::size_t>(slot) >= max_gamepad_slots) {
            return;
        }
        auto& gp = m_gamepads[static_cast<std::size_t>(slot)];
        const auto btn_idx = static_cast<std::size_t>(btn);
        if (btn_idx >= max_gamepad_buttons) {
            return;
        }
        gp.pressed.set(btn_idx, s.pressed);
        gp.released.set(btn_idx, s.released);
        gp.held.set(btn_idx, s.held);
    }

    void input_snapshot::set_gamepad_axis(int slot, sdlpp::gamepad_axis axis, float val) noexcept {
        if (slot < 0 || static_cast<std::size_t>(slot) >= max_gamepad_slots) {
            return;
        }
        auto& gp = m_gamepads[static_cast<std::size_t>(slot)];
        const auto axis_idx = static_cast<std::size_t>(axis);
        if (axis_idx >= max_gamepad_axes) {
            return;
        }
        gp.axes[axis_idx] = val;
    }

    void input_snapshot::set_gamepad_connected(int slot, bool connected) noexcept {
        if (slot < 0 || static_cast<std::size_t>(slot) >= max_gamepad_slots) {
            return;
        }
        m_gamepads[static_cast<std::size_t>(slot)].connected = connected;
    }

} // namespace neutrino
