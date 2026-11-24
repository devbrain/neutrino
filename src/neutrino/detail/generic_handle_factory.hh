//
// Created by igor on 24/11/2026.
//

#pragma once

#include <atomic>
#include <cstdint>
#include <limits>

#include <neutrino/detail/generic_handle.hh>

namespace neutrino::detail {
    /**
     * @brief A new, valid @p Handle (a generic_handle), from a counter of @p Handle's own: no
     *        call returns one an earlier call returned until the counter wraps, after 2^32 - 1
     *        handles; a registry that keeps handles skips those still in use. Thread-safe.
     *
     * The counter is a function-local static of this template, so there is one per shared
     * object that instantiates it. That is why this header is private: only libneutrino mints
     * handles. In a public header a game's own library would get a counter of its own, and
     * mint handles the engine already gave out.
     */
    template <typename Handle>
    [[nodiscard]] Handle make_handle() {
        constexpr auto invalid = std::numeric_limits <std::uint32_t>::max(); // the null handle's value
        static std::atomic <std::uint32_t> counter = 0;
        std::uint32_t value;
        do {
            value = counter.fetch_add(1, std::memory_order_relaxed);
        } while (value == invalid);
        return id_access::make <Handle>(value);
    }
}
