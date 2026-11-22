//
// Created by igor on 21/11/2026.
//

#include "video/target_geometry.hh"

#include <neutrino/video/globals.hh>

#include "video/draw_failures.hh"

namespace neutrino::detail {
    namespace {
        std::uint64_t s_target_switches = 0;
    }

    std::uint64_t target_switches() noexcept {
        return s_target_switches;
    }

    void count_target_switch() noexcept {
        ++s_target_switches;
    }

    target_geometry bound_target_geometry() {
        auto& r = get_renderer();
        target_geometry g;
        // The whole target, bars included: under a logical presentation the current output
        // size is only the presentation rect's.
        if (const auto size = r.get_target_size <dim>()) {
            g.size = *size;
        }
        if (const auto lp = r.get_logical_presentation();
            lp && lp->mode != sdlpp::logical_presentation::disabled) {
            g.logical = dim{lp->width, lp->height};
            g.mode = lp->mode;
        }
        return g;
    }

    void apply_presentation(const target_geometry& g) {
        auto& r = get_renderer();
        // Each change makes SDL recompute the view and queue a viewport and a clip command:
        // skipped when the target has this presentation already (a pooled texture often does).
        const bool disabled = g.mode == sdlpp::logical_presentation::disabled;
        if (const auto now = r.get_logical_presentation()) {
            const bool now_disabled = now->mode == sdlpp::logical_presentation::disabled;
            if (disabled ? now_disabled
                         : now->mode == g.mode && now->width == g.logical.width && now->height == g.logical.height) {
                return;
            }
        }
        if (!r.set_logical_presentation(g.logical.width, g.logical.height, g.mode)) {
            count_draw_failure();
        }
    }
}
