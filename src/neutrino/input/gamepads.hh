//
// Created by igor on 02/07/2026.
//

#pragma once

#include <array>
#include <optional>
#include <vector>
#include <sdlpp/input/gamepad.hh>
#include <sdlpp/app/game_application.hh> // for button_state

namespace neutrino {
    class gamepads {
        public:
            // Reported when handle_event processed a connect/disconnect;
            // index is the stable player slot the pad occupies (or left).
            struct connection_change {
                int index;
                bool connected;
            };

            void clear_state();
            std::optional <connection_change> handle_event(const sdlpp::event& ev);

            sdlpp::button_state get_gamepad_button_state(int gamepad_index, sdlpp::gamepad_button button) const noexcept;
            float get_gamepad_axis(int gamepad_index, sdlpp::gamepad_axis axis) const noexcept;

        private:
            // Slots are stable player indices: unplugging a pad frees its
            // slot (state zeroed) but does not shift the pads behind it,
            // so player 2 stays player 2 when player 1's pad dies. A newly
            // connected pad takes the first free slot.
            struct gamepad_state {
                std::optional <sdlpp::gamepad> pad;
                sdlpp::joystick_id instance_id{};
                std::array <sdlpp::button_state, static_cast <size_t>(sdlpp::gamepad_button::max)> buttons{};
                std::array <float, static_cast <size_t>(sdlpp::gamepad_axis::max)> axes{};

                [[nodiscard]] bool connected() const noexcept {
                    return pad.has_value();
                }
            };

            gamepad_state* find_by_instance(sdlpp::joystick_id id) noexcept;

            std::vector <gamepad_state> m_gamepads;
    };
}
