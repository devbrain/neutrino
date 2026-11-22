//
// Created by igor on 17/11/2026.
//

#include <stdexcept>
#include <utility>
#include <vector>

#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/painter_state.hh>
#include "services/application_scoped.hh"
#include "services/service_locator.hh"
#include "video/blend_mode_sdl.hh"
#include "video/painter_state_internal.hh"
#include "video/draw_failures.hh"

#include <failsafe/exception.hh>
#include <failsafe/logger.hh>
#include <failsafe/enforce.hh>

namespace neutrino {
    namespace {
        // Until the painter keeps its own state and every draw call reads it (RM1 step 4), the
        // live colour, blend mode and line style are the renderer's: the draw calls read the
        // renderer, application::on_render() resets its draw colour for the frame's clear, and
        // simplex keeps its own colour on it between frames. So they are read from the renderer
        // and a restored state is written back to it. The draw colour goes through the 8-bit
        // set_draw_color(): a float colour set straight on the renderer comes back rounded
        // after a save and a restore. The clip is the painter's own (below).

        // Sets the clip of the target bound now. A failure is logged and counted, like a draw's.
        void apply_clip(const std::optional <rect>& clip) {
            auto& r = get_renderer();
            const auto set = clip ? r.set_clip_rect(*clip) : r.set_clip_rect(std::nullopt);
            if (!set) {
                LOG_ERROR("cannot set the renderer's clip rect:", set.error());
                detail::count_draw_failure();
            }
        }

        // The painter's state and its saves (painter.md §2.4): one save per push_state(), the
        // state to restore. Layers do not save it (R39): a layer push changes where draws go,
        // not how. Made on first use and destroyed when the application quits
        // (application_scoped, R38).
        class painter_state_stack_impl {
            public:
                painter_state_stack_impl() = default;

                ~painter_state_stack_impl() {
                    if (!m_saves.empty()) {
                        LOG_ERROR("the application quit with", m_saves.size(),
                                  "painter state save(s) never popped");
                    }
                }

                // The state now: the draw colour, blend mode and line style from the renderer,
                // and the clip the painter keeps.
                [[nodiscard]] painter_state current() const {
                    painter_state s;
                    s.draw_color = get_draw_color();
                    s.blend = get_draw_blend_mode();
                    s.line = get_line_style();
                    s.clip = m_clip;
                    return s;
                }

                // Makes @p s the state: the renderer's fields, and the clip on the target bound
                // now. The target's clip is touched only when the painter's changes, so a
                // push and pop that leave the clip alone keep a clip set straight on the
                // renderer.
                void apply(const painter_state& s) {
                    set_draw_color(s.draw_color);
                    set_draw_blend_mode(s.blend);
                    set_line_style(s.line);
                    const bool changed = m_clip != s.clip;
                    m_clip = s.clip;
                    if (changed) {
                        apply_clip(m_clip);
                    }
                }

                // The painter's clip, if it has one.
                [[nodiscard]] const std::optional <rect>& clip() const noexcept {
                    return m_clip;
                }

                // The painter's clip on the target bound now, after the bound target changed.
                void reapply_clip() const {
                    apply_clip(m_clip);
                }

                // Saves the current state; the state does not change.
                void push() {
                    m_saves.push_back(capture());
                }

                // Saves the current state, then makes @p state current.
                void push(const painter_state& state) {
                    m_saves.push_back(capture());
                    apply(state);
                }

                // How many saves are held.
                [[nodiscard]] std::size_t depth() const noexcept {
                    return m_saves.size();
                }

                // Restores the most recent save and drops it. The entry goes first, so the
                // stack stays consistent if writing the state fails.
                void pop() {
                    ENFORCE(!m_saves.empty(), "pop_state with nothing pushed");
                    const save saved = m_saves.back();
                    m_saves.pop_back();
                    apply(saved.state);
                    // The renderer's own blend, which may be one with no neutrino name.
                    if (!get_renderer().set_draw_blend_mode(saved.blend)) {
                        detail::count_draw_failure();
                    }
                }

            private:
                // What a push saves: the state, with the renderer's blend mode as it was. A
                // custom blend set on the renderer (through native()) has no neutrino name, so
                // current() cannot read it; a save keeps it as it is and puts it back, so the
                // engine's own pushes (the compositor's) work whatever the renderer was left
                // with.
                struct save {
                    painter_state state;
                    sdlpp::blend_mode blend; // sdlpp-ok: internal
                };

                [[nodiscard]] save capture() const {
                    save s;
                    s.state.draw_color = get_draw_color();
                    s.state.line = get_line_style();
                    s.state.clip = m_clip;
                    if (const auto raw = get_renderer().get_draw_blend_mode()) {
                        s.blend = *raw;
                    } else {
                        s.blend = sdlpp::blend_mode::none; // sdlpp-ok: internal
                        detail::count_draw_failure();
                    }
                    s.state.blend = detail::from_sdl_blend(s.blend).value_or(blend_mode::none);
                    return s;
                }

                // The painter's clip. SDL keeps one per target; this one follows the painter,
                // so it is applied to whichever target is bound.
                std::optional <rect> m_clip;
                std::vector <save> m_saves;
        };
    }

    using painter_state_stack = detail::application_scoped <painter_state_stack_impl>;

    bool detail::painter_has_clip() noexcept {
        const auto* p = painter_state_stack::maybe();
        return p && p->clip().has_value();
    }

    void detail::clip_bound_texture() {
        if (auto* p = painter_state_stack::maybe(); p && p->clip()) {
            p->reapply_clip();                     // the clip follows the painter (R39)
        } else if (service_locator::instance().maybe_renderer() != nullptr) {
            apply_clip(std::nullopt);              // not the clip a pooled texture's last owner left
        }
    }

    void detail::clip_after_scope(bool painter_clip_at_start) noexcept {
        try {
            if (service_locator::instance().maybe_renderer() == nullptr) {
                return;                            // the renderer is gone: no target to clip
            }
            if (auto* p = painter_state_stack::maybe(); p && p->clip()) {
                p->reapply_clip();                 // the clip follows the painter (R39)
            } else if (painter_clip_at_start) {
                apply_clip(std::nullopt);          // the target held the painter's clip, now gone
            }
            // Otherwise the target keeps its own clip: SDL keeps one per target.
        } catch (...) {
            detail::count_draw_failure();
        }
    }

    std::size_t detail::painter_save_depth() noexcept {
        const auto* p = painter_state_stack::maybe();
        return p ? p->depth() : 0;
    }

    void detail::pop_painter_saves_to(std::size_t depth) noexcept {
        auto* p = painter_state_stack::maybe();
        while (p && p->depth() > depth) {
            try {
                p->pop();
            } catch (const std::exception& e) {
                LOG_ERROR("cannot restore a painter state save:", e.what());
            } catch (...) {
                LOG_ERROR("cannot restore a painter state save");
            }
        }
    }

    painter_state_scope scoped_state() {
        return {};
    }

    painter_state_scope scoped_state(const painter_state& s) {
        return painter_state_scope{s};
    }

    painter_state get_painter_state() {
        return painter_state_stack::instance().current();
    }

    void push_state() {
        painter_state_stack::instance().push();
    }

    void push_state(const painter_state& s) {
        painter_state_stack::instance().push(s);
    }

    void pop_state() {
        painter_state_stack::instance().pop();
    }

    painter_state_scope::painter_state_scope(painter_state_scope&& other) noexcept
        : m_valid(std::exchange(other.m_valid, false)) {
    }

    painter_state_scope::~painter_state_scope() {
        finalize();
    }

    painter_state_scope::painter_state_scope()
        : m_valid(true) {
        push_state();
    }

    painter_state_scope::painter_state_scope(const painter_state& state)
        : m_valid(true) {
        push_state(state);
    }

    void painter_state_scope::finalize() noexcept {
        // maybe(), not instance(): a scope may end after the application has quit, and then
        // there is nothing to pop.
        if (m_valid) {
            try {
                if (auto* s = painter_state_stack::maybe()) {
                    s->pop();
                }
            } catch (const std::exception& e) {
                LOG_ERROR("could not pop the painter state:", e.what());
            } catch (...) {
                LOG_ERROR("could not pop the painter state: unknown exception");
            }
            m_valid = false;
        }
    }
}
