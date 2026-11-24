//
// Created by igor on 02/11/2026.
//

#pragma once

#include <neutrino/detail/generic_handle.hh>

namespace neutrino {
    namespace detail {
        struct atlas_id_tag;
    }

    /**
     * @brief Opaque handle to an @ref image_atlas, minted when the atlas is built (no
     *        renderer needed) and kept by moves and by the GPU upload.
     */
    using atlas_id = detail::generic_handle <detail::atlas_id_tag>;
}
