//
// Created by igor on 16/11/2026.
//

#pragma once

#include <cstdint>
#include <limits>
#include <iosfwd>

#include <neutrino/neutrino_export.h>

namespace neutrino {
    namespace detail {
        class layer_stack;
    }
    class NEUTRINO_EXPORT layer_id;
    /// @brief Writes @p v as `(serial:index)`, with `INVALID` for a part that names nothing.
    NEUTRINO_EXPORT std::ostream& operator << (std::ostream& os, const layer_id& v);

    /**
     * @brief Names one layer of one painter: what `declare_layer` returns (painter.md §2.5).
     *
     * A small value. It holds the serial of the layer stack that declared the layer and the
     * layer's index, which is also its place in the declaration order (painter.md §3.4). Every
     * layer call checks both, so a layer of another painter — another scene's included — is
     * rejected. It owns nothing: copying or destroying it does not touch the layer, which lives
     * as long as its painter.
     *
     * A default-constructed id names no layer; using it fails ENFORCE.
     */
    class NEUTRINO_EXPORT layer_id {
        friend class detail::layer_stack;
        friend std::ostream& operator << (std::ostream& os, const layer_id& v);
        public:
            /// @brief An id that names no layer.
            layer_id() = default;

            /// @brief True when both name the same layer of the same stack.
            friend bool operator==(layer_id, layer_id) = default;

            /// @brief True if the id was made by a layer stack: it names a layer. It does not
            ///        say that the stack still exists.
            explicit operator bool() const noexcept {
                return m_stack != INVALID && m_index != INVALID;
            }

        private:
            layer_id(std::uint32_t stack, std::uint32_t idx)
                : m_stack(stack), m_index(idx) {
            }

            /// The value of either field when it names nothing; no stack takes it as a serial.
            static constexpr auto INVALID = std::numeric_limits<std::uint32_t>::max();

            std::uint32_t m_stack{INVALID}; ///< The serial of the stack that declared it; INVALID: none.
            std::uint32_t m_index{INVALID}; ///< Its position in declaration order; INVALID: none.
    };
}
