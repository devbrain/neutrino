//
// Created by igor on 02/11/2026.
//

#pragma once

#include <neutrino/detail/id_strong_type.hh>
#include <neutrino/neutrino_export.h>

namespace neutrino {
    namespace details {
        struct atlas_id_tag;
    }

    class NEUTRINO_EXPORT atlas_id
        : public details::id_strong_type <details::atlas_id_tag> {
        friend struct details::id_access;

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
    : std::hash <neutrino::details::id_strong_type <neutrino::details::atlas_id_tag>> {
};