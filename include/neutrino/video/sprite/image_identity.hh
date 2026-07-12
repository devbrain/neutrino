//
// Created by igor on 12/07/2026.
//

#pragma once

/**
 * @file image_identity.hh
 * @brief Content identity of a @ref world_image, shared by every render cache.
 *
 * Lifted out of the tile @ref resource_cache so the sprite cache keys images the
 * same way. Memory bytes hash directly; a decoded surface uses the producer id (or a
 * canonical-RGBA pixel hash when it has none); a file is identified by a
 * (path, mtime, size) stat key mapping to a content hash recomputed only when the
 * file changes. Holds that stat cache, so it is stateful and not thread-safe.
 */

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

#include <neutrino/neutrino_export.h>
#include <neutrino/world/content_key.hh>
#include <neutrino/world/world_common.hh>

namespace neutrino {
    class NEUTRINO_EXPORT image_identifier {
        public:
            /**
             * @brief Content key of @p img by its source arm (disk / memory / surface).
             *
             * An empty image yields an empty identity. File images consult and populate
             * the stat cache; the hash is only recomputed when a file's (mtime, size) change.
             */
            [[nodiscard]] content_key identity(const world_image& img);

        private:
            struct stat_id {
                std::filesystem::file_time_type mtime;
                std::uintmax_t                  size;
                content_key                     content;
            };

            std::unordered_map <std::string, stat_id> m_stat_cache;
    };

    /**
     * @brief Fold one image's content identity into a running (@p digest, @p length).
     *
     * Folds the source arm (disk/memory/surface, so byte-identical images of different
     * kinds cannot collide), the identity hash + length, and the declared dimensions
     * (which drive src-rect/column math and can diverge from the decoded bytes). Shared
     * by every cache `key_for` so they hash images the same way.
     */
    NEUTRINO_EXPORT void fold_image_identity(std::uint64_t& digest, std::uint64_t& length,
                                             image_identifier& ident, const world_image& img);
}
