//
// Created by igor on 02/11/2026.
//

#pragma once

#include <neutrino/detail/id_strong_type.hh>
#include <neutrino/neutrino_export.h>

namespace neutrino {
    namespace detail {
        struct atlas_id_tag;
    }

    class NEUTRINO_EXPORT atlas_id
        : public detail::id_strong_type <detail::atlas_id_tag> {
        friend struct detail::id_access;

        public:
            atlas_id() = default;

        private:
            explicit atlas_id(std::uint32_t value)
                : id_strong_type(value) {
            }
    };
}

template<>
struct std::hash <neutrino::atlas_id>
    : std::hash <neutrino::detail::id_strong_type <neutrino::detail::atlas_id_tag>> {
};