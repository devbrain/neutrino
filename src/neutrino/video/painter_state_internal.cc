//
// Created by igor on 23/11/2026.
//

#include "painter_state_internal.hh"
#include "painter_state_stack.hh"
#include "services/service_locator.hh"
#include "video/draw_failures.hh"

namespace neutrino::detail {
    bool painter_has_clip() noexcept {
        const auto* p = painter_state_stack::maybe();
        return p && p->clip().has_value();
    }

    void clip_bound_texture() {
        if (auto* p = painter_state_stack::maybe(); p && p->clip()) {
            p->reapply_clip(); // the clip follows the painter (R39)
        } else if (service_locator::instance().maybe_renderer() != nullptr) {
            painter_state_stack::apply_clip(std::nullopt); // not the clip a pooled texture's last owner left
        }
    }

    void clip_after_scope(bool painter_clip_at_start) noexcept {
        try {
            if (service_locator::instance().maybe_renderer() == nullptr) {
                return; // the renderer is gone: no target to clip
            }
            if (auto* p = painter_state_stack::maybe(); p && p->clip()) {
                p->reapply_clip(); // the clip follows the painter (R39)
            } else if (painter_clip_at_start) {
                painter_state_stack::apply_clip(std::nullopt); // the target held the painter's clip, now gone
            }
            // Otherwise the target keeps its own clip: SDL keeps one per target.
        } catch (...) {
            count_draw_failure();
        }
    }
}
