//
// Created by igor on 23/11/2026.
//

#include "painter_state_stack.hh"

#include <exception>

#include <failsafe/failsafe.hh>

#include "video/painter_context.hh"

namespace neutrino::detail {
    namespace {
        // Ids of saves, so a painter_state_scope pops its own save and nothing else.
        std::uint64_t g_last_save_id = 0;

        [[nodiscard]] std::optional <rect> clip_of(const painter_state_stack* stack) noexcept {
            return stack ? stack->clip() : std::nullopt;
        }
    }

    painter_state_stack::painter_state_stack() = default;

    painter_state_stack::~painter_state_stack() {
        if (painter_context* c = painter_context::maybe(); c != nullptr && c->scene == this) {
            c->scene = nullptr;                   // a scene destroyed while current
        }
        if (!m_saves.empty()) {
            LOG_ERROR("a scene was destroyed with", m_saves.size(), "painter state save(s) never popped");
        }
    }

    painter_state_stack* painter_state_stack::maybe() noexcept {
        const painter_context* c = painter_context::maybe();
        return c != nullptr ? c->scene : nullptr;
    }

    painter_state_stack& painter_state_stack::instance() {
        painter_state_stack* current = maybe();
        ENFORCE(current != nullptr, "a painter call with no scene current: draw and set the painter "
                "state from a scene's on_enter, update, events or render (R48)");
        return *current;
    }

    const painter_state& painter_state_stack::state() const noexcept {
        return m_state;
    }

    void painter_state_stack::set_draw_color(const color& c) noexcept {
        m_state.draw_color = c;
    }

    void painter_state_stack::set_blend(blend_mode mode) noexcept {
        m_state.blend = mode;
    }

    void painter_state_stack::set_line(const line_style& style) noexcept {
        m_state.line = style;
    }

    const std::optional <rect>& painter_state_stack::clip() const noexcept {
        return m_state.clip;
    }

    void painter_state_stack::apply(const painter_state& s) {
        const bool clip_changed = m_state.clip != s.clip;
        m_state = s;
        if (clip_changed) {
            apply_clip(m_state.clip);
        }
    }

    std::uint64_t painter_state_stack::push() {
        m_saves.push_back(save{m_state, ++g_last_save_id});
        return m_saves.back().id;
    }

    std::uint64_t painter_state_stack::push(const painter_state& state) {
        const std::uint64_t id = push();
        apply(state);
        return id;
    }

    void painter_state_stack::pop() {
        ENFORCE(!m_saves.empty(), "pop_state with nothing pushed");
        // The entry goes first, so the stack stays consistent if setting the clip fails.
        const painter_state saved = m_saves.back().state;
        m_saves.pop_back();
        apply(saved);
    }

    bool painter_state_stack::pop_save(std::uint64_t id) {
        if (m_saves.empty() || m_saves.back().id != id) {
            return false;
        }
        pop();
        return true;
    }

    std::size_t painter_state_stack::depth() const noexcept {
        return m_saves.size();
    }

    void painter_state_stack::pop_to(std::size_t depth) noexcept {
        while (m_saves.size() > depth) {
            try {
                pop();
            } catch (const std::exception& e) {
                LOG_ERROR("cannot restore a painter state save:", e.what());
            } catch (...) {
                LOG_ERROR("cannot restore a painter state save");
            }
        }
    }

    void painter_state_stack::end_call(std::size_t depth, const char* what) {
        const std::size_t left = m_saves.size();
        if (left == depth) {
            return;
        }
        // Undone first, as an unbalanced layer push is: the state from before is back.
        pop_to(depth);
        ENFORCE(left > depth, what, "popped more painter state saves than it pushed");
        ENFORCE(false, what, "ended with", left - depth, "push_state() not popped");
    }

    // ---- current_scope --------------------------------------------------------------------------

    painter_state_stack::current_scope::current_scope(painter_state_stack& stack)
        : m_stack(stack),
          m_previous(painter_context::instance().scene),
          m_depth(stack.depth()) {
        if (clip_of(m_previous) != m_stack.clip()) {
            try_apply_clip(m_stack.clip());
        }
        painter_context::instance().scene = &m_stack;
    }

    painter_state_stack::current_scope::~current_scope() {
        m_stack.pop_to(m_depth);                  // the call threw with pushes left
        if (painter_context* c = painter_context::maybe()) {
            c->scene = m_previous;
        }
        if (clip_of(m_previous) != m_stack.clip()) {
            try_apply_clip(clip_of(m_previous));
        }
    }

    void painter_state_stack::current_scope::finish(const char* what) {
        m_stack.end_call(m_depth, what);
    }
}
