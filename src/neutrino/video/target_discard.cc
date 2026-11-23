//
// See target_discard.hh.
//

#include "video/target_discard.hh"

namespace neutrino::detail {
    namespace {
        // The bound target discards drawing (painter.md §3.6). False outside frames: nothing
        // sets it but a failed layer or canvas, whose scopes end with the frame.
        bool g_discards = false;
    }

    bool target_discards() noexcept {
        return g_discards;
    }

    bool set_target_discards(bool discards) noexcept {
        return std::exchange(g_discards, discards);
    }
}
