//
// Created by igor on 16/11/2026.
//

#pragma once

#include <cstdint>
#include <neutrino/video/layer_id.hh>
#include <neutrino/video/geometry_types.hh>

namespace neutrino::detail {
    /**
     * @brief The internal owner of one painter's layers (painter.md §3.4).
     *
     * It will hold their settings, their targets from the texture allocator, the push stack
     * and the frame's flags. It knows its size and the allocator, not scenes, so a scene's
     * painter and an explicit painter use the same class. So far it holds its size and its
     * serial, and checks that a layer_id is its own.
     *
     * Not copyable: the layer_ids it hands out carry its serial.
     */
    class layer_stack {
        public:
            /// @brief A stack for a painter of size @p layer_dims, with a serial no other stack
            ///        has.
            explicit layer_stack(dim layer_dims);
            ~layer_stack();

            layer_stack(const layer_stack&) = delete;
            layer_stack& operator = (const layer_stack&) = delete;

        private:
            /// True if @p lid names a layer (it is not a default-constructed id) and was made
            /// by this stack. Does not yet check the index against the declared layers.
            [[nodiscard]] bool validate(const layer_id& lid) const noexcept;
            /// The next serial of a global counter, safe from any thread. A counter that wraps
            /// skips layer_id's INVALID, the value that names no stack.
            [[nodiscard]] static std::uint32_t next_serial() noexcept;
        private:
            /// The painter's size: what a layer without a size of its own is made at.
            dim m_layer_dims;
            /// Carried by every layer_id this stack makes.
            std::uint32_t m_serial;
    };
}
