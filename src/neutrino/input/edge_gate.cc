//
// See edge_gate.hh.
//

#include "input/edge_gate.hh"

namespace neutrino::input_detail {
    namespace {
        // Frame-loop state, only ever touched on the main thread between fixed substeps (the same
        // thread that polls input), so no synchronisation is needed.
        bool g_edges_suppressed = false;
    } // namespace

    bool edges_suppressed() noexcept {
        return g_edges_suppressed;
    }

    bool set_edges_suppressed(bool value) noexcept {
        const bool prev = g_edges_suppressed;
        g_edges_suppressed = value;
        return prev;
    }
} // namespace neutrino::input_detail
