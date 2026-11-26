//
// Created by igor on 25/11/2026.
//

#pragma once

/**
 * @file action.hh
 * @brief Actions: what the game asks for ("left", "fire"), bound to alternative inputs that
 *        can be rebound, saved and loaded (action_bindings.hh).
 *
 * An @ref action is checked against the frame's input_snapshot. A @ref global_action fires
 * on its own, before the scenes see the input. Both are RAII handles to a record in the
 * application's actions table: the record lives exactly as long as its object, which may be
 * a scene member, an application member, or anything else. Records made while no application
 * runs wait in a pending table and join the application at on_ready(); at on_quit() those
 * still alive go back, so an action outlives an application and serves the next one.
 *
 * Registering and removing are thread-safe (scenes may be built on a loading thread);
 * checking and firing happen on the main thread.
 */

#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/input/action_id.hh>
#include <neutrino/input/binding.hh>
#include <neutrino/input/input_snapshot.hh>

namespace neutrino {
    class action_record;

    /// @brief The live actions and global actions, in the order they were registered: those
    ///        of the application and of the scenes alive now. Empty ones are not listed.
    [[nodiscard]] NEUTRINO_EXPORT std::vector <action_record> action_records();

    namespace detail {
        struct action_access;

        /// @brief What action and global_action share: the handle to their record.
        class NEUTRINO_EXPORT action_handle {
            public:
                action_handle(const action_handle&) = delete;
                action_handle& operator =(const action_handle&) = delete;

                /// @brief Replaces the bindings; `rebind({})` is the deliberate "unbound" state.
                ///        The first bindings an action is given are kept as its defaults.
                void rebind(std::initializer_list <binding> bindings);
                void rebind(const std::vector <binding>& bindings);
                /// @brief Rebinds from the text form (binding.hh). A malformed text is logged,
                ///        the bindings are kept, and the answer is false.
                bool rebind(std::string_view text);

                /// @brief The current bindings; none while never given any.
                [[nodiscard]] std::vector <binding> bindings() const;

                /// @brief The text a rebinding screen shows for this action.
                void set_description(std::string text);
                [[nodiscard]] std::string description() const;

                /// @brief A disabled action answers false; a disabled global action does not
                ///        fire and takes no input. The flag is this action's own.
                void disable();
                void enable();
                [[nodiscard]] bool enabled() const;

            protected:
                explicit action_handle(bool global) noexcept;
                /// @p bound: @p bindings were given (possibly none); else only the description was.
                action_handle(bool global, std::string description, std::vector <binding> bindings,
                              bool bound, std::function <void()> fire);
                ~action_handle();

                action_handle(action_handle&& other) noexcept;
                action_handle& operator =(action_handle&& other) noexcept;

                [[nodiscard]] action_id id() const noexcept { return m_id; }

            private:
                friend struct action_access;   // the engine's own uses (action_bindings)

                void release() noexcept;
                void ensure_record();
                /// Rebinds from saved bindings: the defaults stay the game's.
                void rebind_loaded(const std::vector <binding>& bindings);
                /// Given bindings, by the game or from saved ones.
                [[nodiscard]] bool bound() const;

                action_id m_id;
                bool m_global;
        };
    }

    /**
     * @brief Something the game asks for, bound to alternative inputs:
     *        `action left{key::left, key::a, pad_button::dpad_left};` or, with the text a
     *        rebinding screen shows, `action left{"Move left", key::left, key::a};`.
     *
     * The bindings act as one virtual button (see check() in binding.hh). Default-constructed,
     * an action is empty and not in the table; it registers once it is given bindings with
     * rebind(). Checking one never given bindings answers false and logs a warning, once per
     * object. Movable (the record moves with it), not copyable.
     */
    class NEUTRINO_EXPORT action : public detail::action_handle {
        public:
            action() noexcept;

            /// @brief Explicit, so a key or a matcher never turns into an action by itself
            ///        (`in.pressed(pad_button::south)` stays a pad query).
            template<bindable... B>
                requires (sizeof...(B) > 0)
            explicit action(const B&... bindings)
                : action(std::string{}, std::vector <binding>{binding(bindings)...}, true) {
            }

            /// @brief With no bindings, only the description is set: the action is still to
            ///        be given bindings (rebind()).
            template<bindable... B>
            explicit action(std::string description, const B&... bindings)
                : action(std::move(description), std::vector <binding>{binding(bindings)...},
                         sizeof...(B) > 0) {
            }

            action(action&& other) noexcept;
            action& operator =(action&& other) noexcept;
            ~action();

            /// @brief This frame's state of the virtual button; nothing while disabled.
            [[nodiscard]] button_state check(const input_snapshot& in) const;
            [[nodiscard]] bool pressed(const input_snapshot& in) const;
            [[nodiscard]] bool held(const input_snapshot& in) const;
            [[nodiscard]] bool released(const input_snapshot& in) const;

        private:
            action(std::string description, std::vector <binding> bindings, bool bound);

            mutable bool m_unset_reported = false;
    };

    /**
     * @brief Fires on its own, before the scenes:
     *        `global_action fullscreen{{mods::alt + key::return_key, mods::alt + key::kp_enter},
     *        toggle_fullscreen};`, optionally with a description first.
     *
     * One shot: it fires once on the press, checked once per frame before the fixed steps, and
     * takes that press (the edges of the bindings that fired) out of the scenes' input
     * snapshot; holding the key does not repeat it. The raw events still reach handle_event().
     * Two on one input: the first registered takes it. A throwing callback is logged and
     * contained. No scene is current while it runs. Default-constructed it is empty and never
     * fires; rebind() keeps the callback. Movable, not copyable.
     */
    class NEUTRINO_EXPORT global_action : public detail::action_handle {
        public:
            global_action() noexcept;
            global_action(std::vector <binding> bindings, std::function <void()> fire);
            global_action(std::string description, std::vector <binding> bindings, std::function <void()> fire);

            global_action(global_action&& other) noexcept;
            global_action& operator =(global_action&& other) noexcept;
            ~global_action();
    };

    /**
     * @brief While one lives, no global action fires and they take no input: a rebinding
     *        screen holds one so Alt+Enter or ` are captured instead of firing. Counted, so
     *        they nest. Any thread.
     */
    class NEUTRINO_EXPORT global_actions_suspended {
        public:
            global_actions_suspended() noexcept;
            ~global_actions_suspended();

            global_actions_suspended(const global_actions_suspended&) = delete;
            global_actions_suspended& operator =(const global_actions_suspended&) = delete;

            /// @brief Is any suspension alive?
            [[nodiscard]] static bool active() noexcept;
    };

    /**
     * @brief A rebinding screen's view of one live action: it asks the table on every call,
     *        so a screen may keep the rows across frames. Once its action is gone, it answers
     *        empty and rebinding does nothing.
     */
    class NEUTRINO_EXPORT action_record {
        public:
            /// @brief Is the action still alive?
            [[nodiscard]] bool alive() const;
            /// @brief A global action (it fires on its own) rather than an action.
            [[nodiscard]] bool is_global() const;
            [[nodiscard]] std::string description() const;
            [[nodiscard]] std::vector <binding> bindings() const;
            /// @brief The first bindings the action was given.
            [[nodiscard]] std::vector <binding> default_bindings() const;

            /// @brief Rebinds the action (a global one keeps its callback); false once it is gone.
            bool rebind(std::initializer_list <binding> bindings);
            bool rebind(const std::vector <binding>& bindings);
            /// @brief Back to the default bindings; false once the action is gone.
            bool reset();

            [[nodiscard]] bool operator==(const action_record&) const noexcept = default;

        private:
            explicit action_record(action_id id) noexcept
                : m_id(id) {
            }

            action_id m_id;

            friend std::vector <action_record> action_records();
    };
}
