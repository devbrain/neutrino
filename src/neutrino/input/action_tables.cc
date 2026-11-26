//
// Created by igor on 24/11/2026.
//

#include <atomic>
#include <exception>
#include <utility>

#include "action_tables.hh"
#include "detail/generic_handle_factory.hh"
#include <neutrino/detail/overloaded.hh>
#include <neutrino/input/action.hh>
#include <failsafe/failsafe.hh>

namespace neutrino::detail {
    namespace {
        // Takes this frame's edges of one binding out of the snapshot; held is left as it is.
        void take_edges(input_snapshot& in, const binding& b) {
            const auto held_only = [](button_state s) {
                return button_state{.pressed = false, .released = false, .held = s.held};
            };
            std::visit(overloaded{
                           [&](key k) { in.set_key(k, held_only(in.key(k))); },
                           [&](mouse_button m) { in.set_mouse(m, held_only(in.mouse(m))); },
                           [&](const hotkey& h) {
                               const auto k = h.resolve_scancode();
                               in.set_key(k, held_only(in.key(k)));
                           },
                           [&](const mouse_click& c) {
                               in.set_mouse(c.button(), held_only(in.mouse(c.button())));
                           },
                           [&](const gamepad_button& g) {
                               const auto s = in.gamepad_button_state(g.gamepad_index(), g.button());
                               in.set_gamepad_button(g.gamepad_index(), g.button(), held_only(s));
                           }
                       }, b);
        }
    }

    action_table::action_table() = default;

    action_table::~action_table() = default;

    action_id action_table::add(action_entry entry) {
        if (entry.bound) {
            entry.defaults = entry.bindings;
        }
        const auto id = make_handle <action_id>();
        const std::lock_guard lock(m_mutex);
        m_records.emplace(id, std::move(entry));
        return id;
    }

    void action_table::remove(action_id id) {
        if (!id.valid()) {
            return;
        }
        const std::lock_guard lock(m_mutex);
        if (m_records.erase(id) == 0) {
            LOG_WARN("Attempt to unbind non existent action", id);
        }
    }

    bool action_table::rebind(action_id id, std::vector <binding> bindings, bool first_are_defaults) {
        const std::lock_guard lock(m_mutex);
        const auto itr = m_records.find(id);
        if (itr == m_records.end()) {
            return false;
        }
        auto& e = itr->second;
        if (!e.bound) {
            if (first_are_defaults) {
                e.defaults = bindings;
            }
            e.bound = true;
        }
        e.bindings = std::move(bindings);
        return true;
    }

    bool action_table::reset(action_id id) {
        const std::lock_guard lock(m_mutex);
        const auto itr = m_records.find(id);
        if (itr == m_records.end()) {
            return false;
        }
        itr->second.bindings = itr->second.defaults;
        return true;
    }

    bool action_table::set_description(action_id id, std::string text) {
        const std::lock_guard lock(m_mutex);
        const auto itr = m_records.find(id);
        if (itr == m_records.end()) {
            return false;
        }
        itr->second.description = std::move(text);
        return true;
    }

    bool action_table::set_enabled(action_id id, bool enabled) {
        const std::lock_guard lock(m_mutex);
        const auto itr = m_records.find(id);
        if (itr == m_records.end()) {
            return false;
        }
        itr->second.enabled = enabled;
        return true;
    }

    std::optional <action_entry> action_table::find(action_id id) const {
        const std::lock_guard lock(m_mutex);
        const auto itr = m_records.find(id);
        if (itr == m_records.end()) {
            return std::nullopt;
        }
        return itr->second;
    }

    std::optional <button_state> action_table::check(action_id id, const input_snapshot& in) const {
        const std::lock_guard lock(m_mutex);
        const auto itr = m_records.find(id);
        if (itr == m_records.end()) {
            return std::nullopt;
        }
        const auto& e = itr->second;
        if (!e.enabled) {
            return button_state{};
        }
        if (!e.bound) {
            return std::nullopt;
        }
        return neutrino::check(e.bindings, in);
    }

    std::vector <action_id> action_table::listed() const {
        const std::lock_guard lock(m_mutex);
        std::vector <action_id> ids;
        for (const auto& [id, e] : m_records) {
            if (e.listed()) {
                ids.push_back(id);
            }
        }
        return ids;
    }

    std::optional <input_snapshot> action_table::fire_globals(const input_snapshot& live) {
        if (global_actions_suspended::active()) {
            return std::nullopt;
        }
        // Copied at the first press taken; later global actions check the copy.
        std::optional <input_snapshot> taken;
        const auto current = [&]() -> const input_snapshot& { return taken ? *taken : live; };
        std::vector <action_id> globals;
        {
            const std::lock_guard lock(m_mutex);
            for (const auto& [id, e] : m_records) {
                if (e.global) {
                    globals.push_back(id);
                }
            }
        }
        for (const auto id : globals) {
            std::shared_ptr <const std::function <void()>> fire;
            {
                const std::lock_guard lock(m_mutex);
                const auto itr = m_records.find(id);
                if (itr == m_records.end()) {
                    continue;   // removed by an earlier callback
                }
                const auto& e = itr->second;
                if (!e.enabled || !e.bound || !e.fire || !neutrino::check(e.bindings, current()).pressed) {
                    continue;
                }
                // The press is this action's: the scenes, and the global actions after it, do
                // not see the edges of the bindings that fired.
                std::vector <binding> fired;
                for (const auto& b : e.bindings) {
                    if (neutrino::check(b, current()).pressed) {
                        fired.push_back(b);
                    }
                }
                if (!taken) {
                    taken = live;
                }
                for (const auto& b : fired) {
                    take_edges(*taken, b);
                }
                fire = e.fire;
            }
            try {
                (*fire)();
            } catch (const std::exception& ex) {
                LOG_ERROR("A global action threw:", ex.what());
            } catch (...) {
                LOG_ERROR("A global action threw an unknown exception");
            }
        }
        return taken;
    }

    void action_table::merge(action_table&& other) {
        // Moves the nodes, not copies: a record left in the source would come back in the next
        // application after its action was gone. A key already here stays behind in the source.
        const std::scoped_lock lock(m_mutex, other.m_mutex);
        m_records.merge(other.m_records);
        ENFORCE(other.m_records.empty(), "action table corrupted:", other.m_records.size(),
                "keys are in both tables");
    }

    namespace {
        // Set while the application's table holds the records (adopt to return).
        std::atomic <bool> s_adopted{false};
    }

    std::mutex& actions_mutex() {
        static auto* mutex = new std::mutex;
        return *mutex;
    }

    action_table& current_action_table() {
        if (s_adopted.load(std::memory_order_acquire)) {
            if (auto* table = neutrino::action_table::maybe()) {
                return *table;
            }
        }
        return *get_pending_actions_table();
    }

    void adopt_pending_actions() {
        const std::lock_guard lock(actions_mutex());
        neutrino::action_table::instance().merge(std::move(*get_pending_actions_table()));
        s_adopted.store(true, std::memory_order_release);
    }

    void return_actions() {
        const std::lock_guard lock(actions_mutex());
        if (!s_adopted.exchange(false, std::memory_order_acq_rel)) {
            return;
        }
        if (auto* table = neutrino::action_table::maybe()) {
            get_pending_actions_table()->merge(std::move(*table));
        }
    }
}

namespace neutrino {
    pending_action_table* get_pending_actions_table() {
        static auto* s_pending_table = new detail::action_table;
        return s_pending_table;
    }
}
