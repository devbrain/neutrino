//
// A frame-scoped snapshot of continuous input (pointer position, mouse buttons, keyboard
// scancodes, and gamepad states), sampled ONCE per frame by the application and handed to
// base_scene::fixed_update.
// A pure value type: no service lookup, directly constructible in a test so a scene's update
// can be driven with synthetic input (findings #12). Discrete, one-shot events still arrive
// through base_scene::handle_action.
//

#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <sdlpp/events/mouse_codes.hh>    // sdlpp::mouse_button
#include <sdlpp/events/keyboard_codes.hh> // sdlpp::scancode / keycode
#include <sdlpp/input/gamepad.hh>         // sdlpp::gamepad_button / gamepad_axis
#include <neutrino/neutrino_export.h>
#include <neutrino/world_space.hh>        // window_pos / render_pos
#include <neutrino/input/hotkey.hh>
#include <neutrino/input/mouse_click.hh>
#include <neutrino/input/gamepad_button.hh>

namespace neutrino {
    /// @brief The pointer position this frame, in BOTH window and render space. @c render is the one
    ///        scenes want: window input already mapped through to_render_coords() (so HiDPI scaling
    ///        and logical-presentation letterboxing are accounted for), so a scene never re-derives it.
    struct pointer_state {
        window_pos window{}; ///< Window pixels, as SDL delivers mouse coordinates.
        render_pos render{}; ///< Render / logical-presentation space.
        /// @brief Is the pointer actually over this application's window?
        bool on_screen{false};
    };

    /// @brief Per-frame edge/held state for one button or key. @c pressed / @c released are true ONLY on the
    ///        transition frame; @c held is true whenever the button is down.
    struct button_state {
        bool pressed = false;
        bool released = false;
        bool held = false;

        [[nodiscard]] explicit operator bool() const noexcept {
            return held;
        }
    };

    /// @brief Immutable, frame-scoped input, sampled once per frame and passed to fixed_update.
    class NEUTRINO_EXPORT input_snapshot {
        public:
            static constexpr std::size_t max_scancodes = 512;
            static constexpr std::size_t max_gamepad_slots = 4;
            static constexpr std::size_t max_gamepad_buttons = static_cast<std::size_t>(sdlpp::gamepad_button::max);
            static constexpr std::size_t max_gamepad_axes = static_cast<std::size_t>(sdlpp::gamepad_axis::max);

            struct gamepad_slot_state {
                std::bitset<max_gamepad_buttons> pressed{};
                std::bitset<max_gamepad_buttons> released{};
                std::bitset<max_gamepad_buttons> held{};
                std::array<float, max_gamepad_axes> axes{};
                bool connected = false;
            };

            input_snapshot() = default;

            input_snapshot(pointer_state pointer,
                           button_state left, button_state middle, button_state right) noexcept;

            /// @brief A copy with every @c pressed / @c released edge cleared and wheel reset,
            /// but @c held, axes, and pointer intact -- what the 2nd..Nth substep of a frame receives.
            [[nodiscard]] input_snapshot without_edges() const noexcept;

            // --- Pointer & Mouse ---
            [[nodiscard]] const pointer_state& pointer() const noexcept { return m_pointer; }
            [[nodiscard]] button_state mouse(sdlpp::mouse_button b) const noexcept;
            [[nodiscard]] int wheel() const noexcept { return m_wheel; }

            // --- Keyboard ---
            [[nodiscard]] button_state key(sdlpp::scancode scan) const noexcept;
            [[nodiscard]] button_state key(sdlpp::keycode code) const noexcept;
            [[nodiscard]] modifier modifiers() const noexcept { return m_modifiers; }

            // Direct keyboard conveniences
            [[nodiscard]] bool pressed(sdlpp::scancode scan) const noexcept { return key(scan).pressed; }
            [[nodiscard]] bool held(sdlpp::scancode scan) const noexcept { return key(scan).held; }
            [[nodiscard]] bool released(sdlpp::scancode scan) const noexcept { return key(scan).released; }

            // --- Gamepad ---
            [[nodiscard]] button_state gamepad_button_state(int slot, sdlpp::gamepad_button button) const noexcept;
            [[nodiscard]] float gamepad_axis(int slot, sdlpp::gamepad_axis axis) const noexcept;
            [[nodiscard]] bool gamepad_connected(int slot) const noexcept;
            [[nodiscard]] const gamepad_slot_state& gamepad(int slot) const noexcept;

            // --- Query Matcher Overloads ---
            [[nodiscard]] bool pressed(const hotkey& hk) const noexcept { return hk.pressed(*this); }
            [[nodiscard]] bool held(const hotkey& hk) const noexcept { return hk.held(*this); }
            [[nodiscard]] bool released(const hotkey& hk) const noexcept { return hk.released(*this); }

            [[nodiscard]] bool pressed(const mouse_click& mc) const noexcept { return mc.pressed(*this); }
            [[nodiscard]] bool held(const mouse_click& mc) const noexcept { return mc.held(*this); }
            [[nodiscard]] bool released(const mouse_click& mc) const noexcept { return mc.released(*this); }

            [[nodiscard]] bool pressed(const gamepad_button& gb) const noexcept { return gb.pressed(*this); }
            [[nodiscard]] bool held(const gamepad_button& gb) const noexcept { return gb.held(*this); }
            [[nodiscard]] bool released(const gamepad_button& gb) const noexcept { return gb.released(*this); }

            // --- Mutators (for application::sample_input and tests) ---
            void set_pointer(pointer_state ptr) noexcept { m_pointer = ptr; }
            void set_mouse(sdlpp::mouse_button b, button_state s) noexcept;
            void set_wheel(int delta) noexcept { m_wheel = delta; }
            void set_key(sdlpp::scancode scan, button_state s) noexcept;
            void set_modifiers(modifier mods) noexcept { m_modifiers = mods; }
            void set_gamepad_button(int slot, sdlpp::gamepad_button btn, button_state s) noexcept;
            void set_gamepad_axis(int slot, sdlpp::gamepad_axis axis, float val) noexcept;
            void set_gamepad_connected(int slot, bool connected) noexcept;

        private:
            pointer_state m_pointer{};
            button_state  m_left{};
            button_state  m_middle{};
            button_state  m_right{};
            button_state  m_x1{};
            button_state  m_x2{};
            int           m_wheel{0};

            std::bitset<max_scancodes> m_keys_pressed{};
            std::bitset<max_scancodes> m_keys_released{};
            std::bitset<max_scancodes> m_keys_held{};
            modifier m_modifiers{modifier::none};

            std::array<gamepad_slot_state, max_gamepad_slots> m_gamepads{};
    };
}
