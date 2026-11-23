//
// See painter_context.hh.
//

#include "video/painter_context.hh"

#include <cstdint>

#include <failsafe/logger.hh>

#include <neutrino/video/globals.hh>

#include "services/application_scoped.hh"
#include "services/service_locator.hh"
#include "video/draw_failures.hh"
#include "video/painter_state_stack.hh"

namespace neutrino::detail {
    namespace {
        [[nodiscard]] bool have_renderer() noexcept {
            return service_locator::instance().maybe_renderer() != nullptr;
        }

        // The current scene's clip; none with no scene current.
        [[nodiscard]] std::optional <rect> current_clip() noexcept {
            const painter_state_stack* p = painter_state_stack::maybe();
            return p != nullptr ? p->clip() : std::nullopt;
        }

        // Logs @p what, left at a frame boundary: the first time, then every 1000th, so a game
        // that leaves it every frame cannot flood the log.
        void report_left(std::uint32_t& count, const char* what, const char* when) noexcept {
            if (count++ % 1000 == 0) {
                try {
                    LOG_ERROR(what, when, "(painter.md §3.1); reset");
                } catch (...) {
                }
            }
            count_draw_failure();
        }

        // Sets whether the bound target discards drawing; returns the value it had. Setting
        // false with no context made yet makes none: nothing discards then.
        bool set_target_discards(bool discards) {
            if (!discards) {
                painter_context* c = painter_context::maybe();
                return c != nullptr && std::exchange(c->discards, false);
            }
            return std::exchange(painter_context::instance().discards, true);
        }
    }

    painter_context& painter_context::instance() {
        return application_scoped <painter_context>::instance();
    }

    painter_context* painter_context::maybe() noexcept {
        return application_scoped <painter_context>::maybe();
    }

    void check_idle(const char* when) noexcept {
        painter_context* c = painter_context::maybe();
        if (c == nullptr) {
            return;                           // nothing made yet, or the application has quit
        }
        static std::uint32_t s_frames_left = 0;
        static std::uint32_t s_discards_left = 0;
        if (c->frame != nullptr) {
            c->frame = nullptr;
            report_left(s_frames_left, "a scene's frame was still open", when);
        }
        if (c->discards) {
            c->discards = false;
            report_left(s_discards_left, "the bound target still discarded drawing — a texture::bind() "
                        "scope ended after the failed layer or canvas it began in —", when);
        }
    }

    bool target_discards() noexcept {
        const painter_context* c = painter_context::maybe();
        return c != nullptr && c->discards;
    }

    void apply_clip(const std::optional <rect>& clip) {
        auto& r = get_renderer();
        const auto set = clip ? r.set_clip_rect(*clip) : r.set_clip_rect(std::nullopt);
        if (!set) {
            LOG_ERROR("cannot set the renderer's clip rect:", set.error());
            count_draw_failure();
        }
    }

    void try_apply_clip(const std::optional <rect>& clip) noexcept {
        try {
            if (have_renderer()) {
                apply_clip(clip);
            }
        } catch (...) {
            count_draw_failure();
        }
    }

    void texture_bound() {
        (void) set_target_discards(false);    // a real target, inside a failed layer too
        if (const auto clip = current_clip()) {
            apply_clip(clip);                 // the clip follows the painter (R39)
        } else if (have_renderer()) {
            apply_clip(std::nullopt);         // not the clip a pooled texture's last owner left
        }
    }

    target_state save_target() noexcept {
        return {current_clip().has_value(), target_discards()};
    }

    void restore_target(const target_state& saved) noexcept {
        if (painter_context* c = painter_context::maybe()) {
            c->discards = saved.discards;     // the target bound again draws as it did
        }
        if (const auto clip = current_clip()) {
            try_apply_clip(clip);             // the clip follows the painter (R39)
        } else if (saved.painter_clip) {
            try_apply_clip(std::nullopt);     // the target held the painter's clip, now gone
        }
        // Otherwise the target keeps its own clip: SDL keeps one per target.
    }

    discard_scope::discard_scope()
        : m_previous(set_target_discards(true)) {
    }

    discard_scope::~discard_scope() {
        if (m_owns) {
            if (painter_context* c = painter_context::maybe()) {
                c->discards = m_previous;
            }
        }
    }
}
