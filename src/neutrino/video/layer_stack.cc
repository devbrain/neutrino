//
// Created by igor on 16/11/2026.
//

#include <atomic>
#include "layer_stack.hh"

namespace neutrino::detail {
    std::uint32_t layer_stack::next_serial() noexcept {
        // Scenes, and so their painters, may be built on any thread: a loading thread, say.
        static std::atomic <std::uint32_t> s_counter{0};
        for (;;) {
            const std::uint32_t serial = s_counter.fetch_add(1, std::memory_order_relaxed);
            if (serial != layer_id::INVALID) {
                return serial;
            }
        }
    }

    layer_stack::layer_stack(dim layer_dims)
        : m_layer_dims(layer_dims), m_serial(next_serial()) {
    }

    layer_stack::~layer_stack() = default;

    bool layer_stack::validate(const layer_id& lid) const noexcept {
        return lid && (lid.m_stack == m_serial);
    }
}
