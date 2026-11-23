//
// Created by igor on 17/11/2026.
//

#include <exception>
#include <utility>

#include <neutrino/video/painter_state.hh>
#include "video/painter_state_stack.hh"

#include <failsafe/logger.hh>

namespace neutrino {
    painter_state_scope scoped_state() {
        return {};
    }

    painter_state_scope scoped_state(const painter_state& s) {
        return painter_state_scope{s};
    }

    painter_state get_painter_state() {
        return detail::painter_state_stack::instance().state();
    }

    void push_state() {
        (void) detail::painter_state_stack::instance().push();
    }

    void push_state(const painter_state& s) {
        (void) detail::painter_state_stack::instance().push(s);
    }

    void pop_state() {
        detail::painter_state_stack::instance().pop();
    }

    painter_state_scope::painter_state_scope(painter_state_scope&& other) noexcept
        : m_save(std::exchange(other.m_save, 0)) {
    }

    painter_state_scope::~painter_state_scope() {
        finalize();
    }

    painter_state_scope::painter_state_scope()
        : m_save(detail::painter_state_stack::instance().push()) {
    }

    painter_state_scope::painter_state_scope(const painter_state& state)
        : m_save(detail::painter_state_stack::instance().push(state)) {
    }

    void painter_state_scope::finalize() noexcept {
        // maybe(), not instance(): a scope may end with no scene current, and then there is
        // nothing of its own to pop.
        const std::uint64_t save = std::exchange(m_save, 0);
        if (save == 0) {
            return;
        }
        try {
            auto* s = detail::painter_state_stack::maybe();
            if (s == nullptr || !s->pop_save(save)) {
                LOG_ERROR("a painter state scope ended after its push was undone, or outside the "
                          "scene that made it: nothing popped");
            }
        } catch (const std::exception& e) {
            LOG_ERROR("could not pop the painter state:", e.what());
        } catch (...) {
            LOG_ERROR("could not pop the painter state: unknown exception");
        }
    }
}
