//
// Created by igor on 02/11/2026.
//

#include <atomic>
#include "atlas_id_gen.hh"

namespace neutrino::details {
    atlas_id make_atlas_id() {
        static std::atomic <uint32_t> atlas_id_counter = 0;
        return details::id_access::make <atlas_id>(atlas_id_counter.fetch_add(1, std::memory_order_relaxed));
    }
}
