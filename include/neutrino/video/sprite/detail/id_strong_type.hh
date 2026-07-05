//
// Created by igor on 05/07/2026.
//

#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>

namespace neutrino::details {
    template <typename Tag>
    class id_strong_type {
        friend struct std::hash <id_strong_type<Tag>>;
        static constexpr auto INVALID = std::numeric_limits <std::uint32_t>::max();

        public:
            /**
             * @brief Construct an invalid handle.
             */
            id_strong_type() = default;
            id_strong_type(const id_strong_type&) = default;
            id_strong_type& operator =(const id_strong_type&) = default;
            id_strong_type(id_strong_type&&) = default;
            id_strong_type& operator =(id_strong_type&&) = default;

            /**
             * @brief Is this not the null sentinel?
             */
            [[nodiscard]] bool valid() const noexcept {
                return m_value != INVALID;
            }

            bool operator ==(const id_strong_type& other) const = default;

            bool operator !=(const id_strong_type& other) const {
                return !(*this == other);
            }

            std::strong_ordering operator <=>(const id_strong_type& other) const = default;

        protected:
            explicit id_strong_type(std::uint32_t value)
                : m_value(value) {
            }

            std::uint32_t m_value{INVALID};
    };
}

template<typename Tag>
struct std::hash <neutrino::details::id_strong_type<Tag>> {
    [[nodiscard]] std::size_t operator()(const neutrino::details::id_strong_type<Tag>& id) const noexcept {
        return std::hash <std::size_t>{}(id.m_value);
    }
};
