//
// Created by igor on 26/11/2026.
//

#pragma once

/**
 * @file binding.hh
 * @brief One input an action can be bound to, and the text form of bindings.
 *
 * A binding is a plain key, a plain mouse button, or one of the matchers:
 *
 *   - `key::space`, `mouse_button::left`: modifiers are ignored, as `in.pressed(key)` does;
 *   - `hotkey`, `mouse_click`: strict modifiers (`mods::ctrl + key::s` matches only while Ctrl,
 *     and no other modifier, is held; `hotkey{key::space}` only while none is);
 *   - `gamepad_button`: a button on one pad; a plain `pad_button::south` is the button on
 *     slot 0, as `gamepad_button(pad_button)` is.
 *
 * Text form (layout-independent: keys go by scancode names):
 *
 *   - bindings are separated by ", ": `"Ctrl+F, Space, Pad South, Mouse Left"`;
 *   - modifiers come first, each followed by `+`: `Ctrl`, `Shift`, `Alt`, `GUI` (either side),
 *     `LCtrl`, `RCtrl`, `LShift`, ... (one side); `None` is the strict binding with no modifier
 *     (`"None+Space"` is `hotkey{key::space}`, `"Space"` is `key::space`);
 *   - mouse buttons: `Mouse Left`, `Mouse Middle`, `Mouse Right`, `Mouse X1`, `Mouse X2`;
 *   - pad buttons: `Pad South` (slot 0), `Pad 2 South` (slot 1; players count from 1), with
 *     the names `South`, `East`, `West`, `North`, `Back`, `Guide`, `Start`, `Left Stick`,
 *     `Right Stick`, `Left Shoulder`, `Right Shoulder`, `DPad Up`, `DPad Down`, `DPad Left`,
 *     `DPad Right`, `Misc 1` .. `Misc 6`, `Left Paddle 1`, `Left Paddle 2`, `Right Paddle 1`,
 *     `Right Paddle 2`, `Touchpad`; pad buttons take no modifiers;
 *   - keys are written with the engine's names: SDL's default scancode names, fixed, so the
 *     text does not change with the platform (SDL renames some keys when video starts:
 *     "Left Option" on macOS); Return2 is `Return 2`. Reading also takes the names SDL gives
 *     keys on any platform ("Left Option", "Left Windows", "Left Command") and its current ones;
 *   - names are matched without regard to case or runs of blanks;
 *   - what has no name is written by number, and reads back: `Scancode <n>`, `Mouse <n>`,
 *     `Pad Button <n>`; a pad slot is any number (`Pad 7 South` is slot 6);
 *   - a hotkey built from a keycode is written as the scancode it maps to under the current
 *     layout, so it comes back as a scancode binding; one that maps to no key is
 *     `Scancode 0`, which binds nothing.
 *
 * Parsing never throws: a malformed text gives nothing.
 */

#include <concepts>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/input/gamepad_button.hh>
#include <neutrino/input/hotkey.hh>
#include <neutrino/input/keys.hh>
#include <neutrino/input/mouse_click.hh>

namespace neutrino {
    class input_snapshot;
    struct button_state;

    /**
     * @brief One input an action can be bound to; see the file comment. A variant (std::visit,
     *        std::get and std::holds_alternative work on it) made only from a key, a mouse
     *        button, a pad button or a matcher: a number does not make one, although a keycode
     *        is a number (build `hotkey{keycode}` for that).
     */
    class binding : public std::variant <key, mouse_button, hotkey, mouse_click, gamepad_button> {
        public:
            using variant_type = std::variant <key, mouse_button, hotkey, mouse_click, gamepad_button>;

            binding(key k) noexcept
                : variant_type(k) {
            }

            binding(mouse_button button) noexcept
                : variant_type(button) {
            }

            /// @brief A plain pad button is the button on slot 0.
            binding(pad_button button) noexcept
                : variant_type(gamepad_button{button}) {
            }

            // Exactly the matcher: a number must not reach hotkey's keycode constructor.
            template<std::same_as <hotkey> M>
            binding(const M& h) noexcept
                : variant_type(h) {
            }

            template<std::same_as <mouse_click> M>
            binding(const M& c) noexcept
                : variant_type(c) {
            }

            template<std::same_as <gamepad_button> M>
            binding(const M& g) noexcept
                : variant_type(g) {
            }

            [[nodiscard]] const variant_type& as_variant() const noexcept {
                return *this;
            }

            [[nodiscard]] bool operator==(const binding& other) const noexcept {
                return as_variant() == other.as_variant();
            }
    };

    /// @brief What a binding is made from: a key, a mouse button, a pad button, or a matcher.
    template<typename T>
    concept bindable = std::same_as <T, key> || std::same_as <T, mouse_button> || std::same_as <T, pad_button>
                       || std::same_as <T, hotkey> || std::same_as <T, mouse_click>
                       || std::same_as <T, gamepad_button> || std::same_as <T, binding>;

    /// @brief This frame's state of one binding.
    [[nodiscard]] NEUTRINO_EXPORT button_state check(const binding& b, const input_snapshot& in) noexcept;

    /**
     * @brief Alternative bindings as one virtual button: held while any binding is held;
     *        pressed when it goes from none held to some held (a tap within the frame counts);
     *        released when it goes to none held.
     *
     * No memory: whether a binding was down before the frame follows from its own state in the
     * snapshot. A strict binding follows the modifiers held now, so letting go of Ctrl while S
     * is held ends `Ctrl+S` without a released edge.
     */
    [[nodiscard]] NEUTRINO_EXPORT button_state check(std::span <const binding> bindings,
                                                     const input_snapshot& in) noexcept;

    /// @brief The text form of one binding: `"Ctrl+S"`, `"Space"`, `"None+Space"`, `"Pad South"`.
    [[nodiscard]] NEUTRINO_EXPORT std::string to_string(const binding& b);

    /// @brief The text form of a list of bindings, separated by ", "; empty for no bindings.
    [[nodiscard]] NEUTRINO_EXPORT std::string to_string(std::span <const binding> bindings);

    /// @brief One binding from its text form; nothing if the text is malformed.
    [[nodiscard]] NEUTRINO_EXPORT std::optional <binding> binding_from_string(std::string_view text);

    /// @brief A list of bindings from its text form (an empty or blank text is no bindings);
    ///        nothing if any of them is malformed.
    [[nodiscard]] NEUTRINO_EXPORT std::optional <std::vector <binding>> bindings_from_string(std::string_view text);
}
