//
// Created by igor on 15/11/2026.
//

#pragma once

#include <cstddef>
#include <neutrino/neutrino_export.h>

namespace neutrino {
    /// @brief The texture allocator's pool, as texture_pool_stats() reports it (painter.md §3.3).
    struct NEUTRINO_EXPORT pool_stats {
        std::size_t held{0}; // textures in live handles
        std::size_t pooled{0}; // textures waiting for reuse
        std::size_t held_bytes{0}; // width × height × 4 per texture
        std::size_t pooled_bytes{0};
        std::size_t created{0}; // since the application started
        std::size_t reused{0}; // acquires served from the pool
        std::size_t destroyed{0};
    };

    /// @brief The texture allocator's counts now (painter.md §2.14). ENFORCE: the application is
    ///        ready.
    NEUTRINO_EXPORT [[nodiscard]] pool_stats texture_pool_stats();
}
