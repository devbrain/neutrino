//
// Created by igor on 24/11/2026.
//

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <neutrino/input/action_id.hh>
#include <neutrino/input/binding.hh>
#include <neutrino/input/input_snapshot.hh>
#include "services/application_scoped.hh"

namespace neutrino {
    namespace detail {
        /// @brief The record of one action or global action.
        struct action_entry {
            std::vector <binding> bindings;
            /// The first bindings the action was given, for "reset to defaults".
            std::vector <binding> defaults;
            /// Given bindings (constructor or rebind; none counts). Until then the record is
            /// not listed, and checking it is "never set".
            bool bound = false;
            std::string description;
            bool enabled = true;
            bool global = false;
            /// A global action's callback; shared so firing runs it outside the table's lock.
            std::shared_ptr <const std::function <void()>> fire;

            /// Listed for rebinding screens: bound, and a global action has its callback.
            [[nodiscard]] bool listed() const noexcept {
                return bound && (!global || fire);
            }
        };

        /**
         * @brief The actions table: the application's (application_scoped), or the pending one
         *        while no application runs. Every member takes the table's lock: records are
         *        added and removed from any thread; checking and firing are on the main thread.
         *        Records are kept in handle order, which is the order they were registered.
         */
        class action_table {
            public:
                action_table();
                ~action_table();

                /// @brief A new record. If it is bound, its bindings are also its defaults.
                [[nodiscard]] action_id add(action_entry entry);
                /// @brief Removes a record; one not here is logged.
                void remove(action_id id);

                /// @brief Replaces the bindings; the first ones a record is given become its
                ///        defaults, unless @p first_are_defaults is false (saved bindings being
                ///        loaded: the defaults are the game's). False if there is no such record.
                bool rebind(action_id id, std::vector <binding> bindings, bool first_are_defaults = true);
                /// @brief Back to the defaults. False if there is no such record.
                bool reset(action_id id);
                bool set_description(action_id id, std::string text);
                bool set_enabled(action_id id, bool enabled);

                /// @brief A copy of the record, if there is one.
                [[nodiscard]] std::optional <action_entry> find(action_id id) const;
                /// @brief Nothing if there is no such record or it was never bound; nothing held
                ///        while disabled; else the state of its bindings as one virtual button.
                [[nodiscard]] std::optional <button_state> check(action_id id, const input_snapshot& in) const;

                /// @brief The listed records, in the order they were registered.
                [[nodiscard]] std::vector <action_id> listed() const;

                /**
                 * @brief Fires the global actions pressed in @p in, in the order they were
                 *        registered, unless suspended. Each takes the edges of the bindings that
                 *        fired, so the ones after it and the scenes' snapshot do not have them.
                 *
                 * A callback runs outside the lock, may add or remove records (a removed one is
                 * skipped, a new one is checked next time), and is contained if it throws.
                 *
                 * @return @p in without the edges taken; nothing if no global action fired, so
                 *         the scenes get @p in itself.
                 */
                [[nodiscard]] std::optional <input_snapshot> fire_globals(const input_snapshot& in);

                /// @brief Moves every record of @p other into this table; @p other is left empty.
                ///        ENFORCE: no handle is in both (handles are minted once, by make_handle()).
                void merge(action_table&& other);

            private:
                mutable std::mutex m_mutex;
                std::map <action_id, action_entry> m_records;
        };

        /// @brief Held while the table records go to is chosen and used, and while records move
        ///        between the tables: a loading thread then never adds to a table that was just
        ///        emptied, or is being destroyed. Never destroyed (actions may die in static
        ///        destruction). Taken before a table's own lock.
        [[nodiscard]] std::mutex& actions_mutex();

        /// @brief The table records go to now: the application's from adopt_pending_actions()
        ///        to return_actions(), else the pending one. Hold actions_mutex(), or be on the
        ///        main thread (the only one that moves records between tables).
        [[nodiscard]] action_table& current_action_table();

        /// @brief Calls @p f with the current table, under actions_mutex().
        template<typename F>
        decltype(auto) with_current_table(F&& f) {
            const std::lock_guard lock(actions_mutex());
            return std::forward <F>(f)(current_action_table());
        }

        /// @brief on_ready(): the pending records move into the application's table, which is
        ///        current from now on.
        void adopt_pending_actions();

        /// @brief on_quit(), before the application scope closes: the records move back to the
        ///        pending table, which is current again. Does nothing if not adopted.
        void return_actions();
    }

    using action_table = detail::application_scoped <detail::action_table>;
    using pending_action_table = detail::action_table;
    /// @brief Where records wait while no application runs. Made once, never destroyed: an
    ///        action may be destroyed during static destruction.
    [[nodiscard]] pending_action_table* get_pending_actions_table();
}
