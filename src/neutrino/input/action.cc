//
// Created by igor on 26/11/2026.
//

#include <atomic>
#include <exception>
#include <memory>
#include <utility>

#include <neutrino/input/action.hh>
#include "input/action_tables.hh"
#include <failsafe/logger.hh>

namespace neutrino {
    namespace {
        // Every operation picks the table and uses it under detail::actions_mutex(), so records
        // moving between the pending and the application's table never race with it.
        template<typename F>
        decltype(auto) with_table(F&& f) {
            return detail::with_current_table(std::forward <F>(f));
        }

        [[nodiscard]] std::optional <detail::action_entry> find(action_id id) {
            if (!id.valid()) {
                return std::nullopt;
            }
            return with_table([id](const detail::action_table& t) { return t.find(id); });
        }
    }

    namespace detail {
        action_handle::action_handle(bool global) noexcept
            : m_global(global) {
        }

        action_handle::action_handle(bool global, std::string description, std::vector <binding> bindings,
                                     bool bound, std::function <void()> fire)
            : m_global(global) {
            action_entry entry;
            entry.bindings = std::move(bindings);
            entry.bound = bound;
            entry.description = std::move(description);
            entry.global = global;
            if (fire) {
                entry.fire = std::make_shared <const std::function <void()>>(std::move(fire));
            }
            m_id = with_table([&entry](action_table& t) { return t.add(std::move(entry)); });
        }

        action_handle::~action_handle() {
            release();
        }

        action_handle::action_handle(action_handle&& other) noexcept
            : m_id(std::exchange(other.m_id, {})),
              m_global(other.m_global) {
        }

        action_handle& action_handle::operator=(action_handle&& other) noexcept {
            if (this != &other) {
                release();
                m_id = std::exchange(other.m_id, {});
                m_global = other.m_global;
            }
            return *this;
        }

        void action_handle::release() noexcept {
            if (!m_id.valid()) {
                return;
            }
            try {
                with_table([this](action_table& t) { t.remove(m_id); });
            } catch (const std::exception& e) {
                LOG_ERROR("unbinding action", m_id, "threw:", e.what());
            } catch (...) {
                LOG_ERROR("unbinding action", m_id, "threw an unknown exception");
            }
            m_id = {};
        }

        void action_handle::ensure_record() {
            if (!m_id.valid()) {
                action_entry entry;
                entry.global = m_global;
                m_id = with_table([&entry](action_table& t) { return t.add(std::move(entry)); });
            }
        }

        void action_handle::rebind(std::initializer_list <binding> bindings) {
            rebind(std::vector <binding>(bindings));
        }

        void action_handle::rebind(const std::vector <binding>& bindings) {
            ensure_record();
            if (!with_table([&](action_table& t) { return t.rebind(m_id, bindings); })) {
                LOG_ERROR("action", m_id, "has no record to rebind");
            }
        }

        bool action_handle::rebind(std::string_view text) {
            auto parsed = bindings_from_string(text);
            if (!parsed) {
                LOG_WARN("Malformed bindings, the old ones are kept:", std::string{text});
                return false;
            }
            rebind(*parsed);
            return true;
        }

        void action_handle::rebind_loaded(const std::vector <binding>& bindings) {
            ensure_record();
            if (!with_table([&](action_table& t) { return t.rebind(m_id, bindings, false); })) {
                LOG_ERROR("action", m_id, "has no record to rebind");
            }
        }

        bool action_handle::bound() const {
            const auto entry = find(m_id);
            return entry && entry->bound;
        }

        std::vector <binding> action_handle::bindings() const {
            auto entry = find(m_id);
            return entry ? std::move(entry->bindings) : std::vector <binding>{};
        }

        void action_handle::set_description(std::string text) {
            ensure_record();
            with_table([&](action_table& t) { return t.set_description(m_id, std::move(text)); });
        }

        std::string action_handle::description() const {
            auto entry = find(m_id);
            return entry ? std::move(entry->description) : std::string{};
        }

        void action_handle::disable() {
            ensure_record();
            with_table([this](action_table& t) { return t.set_enabled(m_id, false); });
        }

        void action_handle::enable() {
            if (m_id.valid()) {
                with_table([this](action_table& t) { return t.set_enabled(m_id, true); });
            }
        }

        bool action_handle::enabled() const {
            const auto entry = find(m_id);
            return !entry || entry->enabled;
        }
    }

    // ---- action ------------------------------------------------------------------------------

    action::action() noexcept
        : action_handle(false) {
    }

    action::action(std::string description, std::vector <binding> bindings, bool bound)
        : action_handle(false, std::move(description), std::move(bindings), bound, {}) {
    }

    action::action(action&& other) noexcept
        : action_handle(std::move(other)),
          m_unset_reported(other.m_unset_reported) {
    }

    action& action::operator=(action&& other) noexcept {
        if (this != &other) {
            action_handle::operator=(std::move(other));
            m_unset_reported = other.m_unset_reported;
        }
        return *this;
    }

    action::~action() = default;

    button_state action::check(const input_snapshot& in) const {
        if (id().valid()) {
            const auto state = with_table([this, &in](const detail::action_table& t) { return t.check(id(), in); });
            if (state) {
                return *state;
            }
        }
        if (!m_unset_reported) {
            m_unset_reported = true;
            LOG_WARN("An action was checked before it was given bindings:", description());
        }
        return {};
    }

    bool action::pressed(const input_snapshot& in) const {
        return check(in).pressed;
    }

    bool action::held(const input_snapshot& in) const {
        return check(in).held;
    }

    bool action::released(const input_snapshot& in) const {
        return check(in).released;
    }

    // ---- global_action -----------------------------------------------------------------------

    global_action::global_action() noexcept
        : action_handle(true) {
    }

    global_action::global_action(std::vector <binding> bindings, std::function <void()> fire)
        : action_handle(true, {}, std::move(bindings), true, std::move(fire)) {
    }

    global_action::global_action(std::string description, std::vector <binding> bindings,
                                 std::function <void()> fire)
        : action_handle(true, std::move(description), std::move(bindings), true, std::move(fire)) {
    }

    global_action::global_action(global_action&& other) noexcept = default;

    global_action& global_action::operator=(global_action&& other) noexcept = default;

    global_action::~global_action() = default;

    // ---- global_actions_suspended ------------------------------------------------------------

    namespace {
        std::atomic <int> s_suspensions{0};
    }

    global_actions_suspended::global_actions_suspended() noexcept {
        s_suspensions.fetch_add(1, std::memory_order_relaxed);
    }

    global_actions_suspended::~global_actions_suspended() {
        s_suspensions.fetch_sub(1, std::memory_order_relaxed);
    }

    bool global_actions_suspended::active() noexcept {
        return s_suspensions.load(std::memory_order_relaxed) > 0;
    }

    // ---- action_record -----------------------------------------------------------------------

    bool action_record::alive() const {
        return find(m_id).has_value();
    }

    bool action_record::is_global() const {
        const auto entry = find(m_id);
        return entry && entry->global;
    }

    std::string action_record::description() const {
        auto entry = find(m_id);
        return entry ? std::move(entry->description) : std::string{};
    }

    std::vector <binding> action_record::bindings() const {
        auto entry = find(m_id);
        return entry ? std::move(entry->bindings) : std::vector <binding>{};
    }

    std::vector <binding> action_record::default_bindings() const {
        auto entry = find(m_id);
        return entry ? std::move(entry->defaults) : std::vector <binding>{};
    }

    bool action_record::rebind(std::initializer_list <binding> bindings) {
        return rebind(std::vector <binding>(bindings));
    }

    bool action_record::rebind(const std::vector <binding>& bindings) {
        return with_table([&](detail::action_table& t) { return t.rebind(m_id, bindings); });
    }

    bool action_record::reset() {
        return with_table([this](detail::action_table& t) { return t.reset(m_id); });
    }

    std::vector <action_record> action_records() {
        std::vector <action_record> records;
        for (const auto id : with_table([](const detail::action_table& t) { return t.listed(); })) {
            records.push_back(action_record{id});
        }
        return records;
    }
}
