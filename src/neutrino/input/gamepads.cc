//
// Created by igor on 02/07/2026.
//

#include <algorithm>
#include "gamepads.hh"

namespace neutrino {
    void gamepads::clear_state() {
        // Clear transient button states
        for (auto& pad : m_gamepads) {
            for (auto& btn : pad.buttons) {
                btn.pressed = false;
                btn.released = false;
            }
        }
    }

    gamepads::gamepad_state* gamepads::find_by_instance(sdlpp::joystick_id id) noexcept {
        auto it = std::find_if(m_gamepads.begin(), m_gamepads.end(), [id](const auto& g) {
            return g.connected() && g.instance_id == id;
        });
        return it != m_gamepads.end() ? &*it : nullptr;
    }

    std::optional <gamepads::connection_change> gamepads::handle_event(const sdlpp::event& e) {
        if (e.type() == sdlpp::event_type::gamepad_added) {
            if (auto* dev_ev = e.as <sdlpp::gamepad_device_event>()) {
                if (find_by_instance(dev_ev->which)) {
                    return std::nullopt; // duplicate added event
                }
                if (auto pad_res = sdlpp::gamepad::open(dev_ev->which)) {
                    // Take the first free slot so player indices stay stable.
                    auto it = std::find_if(m_gamepads.begin(), m_gamepads.end(), [](const auto& g) {
                        return !g.connected();
                    });
                    if (it == m_gamepads.end()) {
                        it = m_gamepads.emplace(m_gamepads.end());
                    }
                    it->pad.emplace(std::move(*pad_res));
                    it->instance_id = dev_ev->which;
                    it->buttons = {};
                    it->axes = {};
                    return connection_change{static_cast <int>(it - m_gamepads.begin()), true};
                }
            }
        } else if (e.type() == sdlpp::event_type::gamepad_removed) {
            if (auto* dev_ev = e.as <sdlpp::gamepad_device_event>()) {
                if (auto* slot = find_by_instance(dev_ev->which)) {
                    // Free the slot but keep its position (stable indices).
                    slot->pad.reset();
                    slot->buttons = {};
                    slot->axes = {};
                    return connection_change{static_cast <int>(slot - m_gamepads.data()), false};
                }
            }
        } else if (e.type() == sdlpp::event_type::gamepad_button_down) {
            if (auto* btn_ev = e.as <sdlpp::gamepad_button_event>()) {
                if (auto* slot = find_by_instance(btn_ev->which)) {
                    if (btn_ev->button < static_cast <Uint8>(sdlpp::gamepad_button::max)) {
                        auto& btn = slot->buttons[btn_ev->button];
                        btn.pressed = true;
                        btn.held = true;
                    }
                }
            }
        } else if (e.type() == sdlpp::event_type::gamepad_button_up) {
            if (auto* btn_ev = e.as <sdlpp::gamepad_button_event>()) {
                if (auto* slot = find_by_instance(btn_ev->which)) {
                    if (btn_ev->button < static_cast <Uint8>(sdlpp::gamepad_button::max)) {
                        auto& btn = slot->buttons[btn_ev->button];
                        btn.released = true;
                        btn.held = false;
                    }
                }
            }
        } else if (e.type() == sdlpp::event_type::gamepad_axis_motion) {
            if (auto* axis_ev = e.as <sdlpp::gamepad_axis_event>()) {
                if (auto* slot = find_by_instance(axis_ev->which)) {
                    if (axis_ev->axis < static_cast <Uint8>(sdlpp::gamepad_axis::max)) {
                        float val = static_cast <float>(axis_ev->value) / 32767.0f;
                        if (val < -1.0f) val = -1.0f;
                        if (val > 1.0f) val = 1.0f;
                        slot->axes[axis_ev->axis] = val;
                    }
                }
            }
        }
        return std::nullopt;
    }

    sdlpp::button_state gamepads::get_gamepad_button_state(int gamepad_index,
                                                           sdlpp::gamepad_button button) const noexcept {
        if (gamepad_index >= 0 && static_cast <size_t>(gamepad_index) < m_gamepads.size()) {
            auto btn_idx = static_cast <size_t>(button);
            if (btn_idx < m_gamepads[gamepad_index].buttons.size()) {
                return m_gamepads[gamepad_index].buttons[btn_idx];
            }
        }
        return {};
    }

    float gamepads::get_gamepad_axis(int gamepad_index, sdlpp::gamepad_axis axis) const noexcept {
        if (gamepad_index >= 0 && static_cast <size_t>(gamepad_index) < m_gamepads.size()) {
            auto axis_idx = static_cast <size_t>(axis);
            if (axis_idx < m_gamepads[gamepad_index].axes.size()) {
                return m_gamepads[gamepad_index].axes[axis_idx];
            }
        }
        return 0.0f;
    }
}
