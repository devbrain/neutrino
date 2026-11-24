//
// Created by igor on 24/11/2026.
//

#pragma once

#include <cstdint>
#include <functional>

#include <neutrino/detail/id_strong_type.hh>

namespace neutrino::detail {
    /**
     * @brief A plain engine handle: an @ref id_strong_type with nothing added.
     *
     * Each kind is an alias over its own tag, so handles of different kinds cannot be mixed:
     * @code
     * namespace detail { struct atlas_id_tag; }
     * using atlas_id = detail::generic_handle <detail::atlas_id_tag>;
     * @endcode
     * Only @ref id_access mints one: from an engine counter (make_handle(), in
     * detail/generic_handle_factory.hh), or from the value a registry chose. A handle that
     * carries more than its value (sprite_visual_id) derives from @ref id_strong_type instead.
     */
    template<typename TAG>
    class generic_handle : public id_strong_type <TAG> {
        friend struct id_access;
        public:
            /**
             * @brief Construct an invalid handle.
             */
            generic_handle() = default;

        private:
            explicit generic_handle(std::uint32_t value)
                : id_strong_type <TAG>(value) {
            }
    };
}

/**
 * @brief @c std::hash for every @ref neutrino::detail::generic_handle, so handles key
 *        @c unordered_map / @c unordered_set. A specialization for the base is not chosen
 *        for a derived type, so the handle needs its own; it hashes as the base does.
 */
template<typename TAG>
struct std::hash <neutrino::detail::generic_handle <TAG>>
    : std::hash <neutrino::detail::id_strong_type <TAG>> {
};
