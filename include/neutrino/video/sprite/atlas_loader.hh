//
// Created by igor on 12/07/2026.
//

#pragma once

/**
 * @file atlas_loader.hh
 * @brief Populate a @ref sprite_def from an exported atlas.
 *
 * The loadable-asset payoff: an exported sprite atlas becomes a `sprite_def` with no
 * hand code, which the sprite cache then builds. v1 ships one format -- Aseprite's JSON
 * export (`--format json-array`) -- with frame rects, trim metadata, per-frame durations,
 * and frame tags → named clips. Broader format support (TexturePacker, hash form) is a
 * later addition on the same `sprite_def` target.
 */

#include <string_view>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/sprite_def.hh>

namespace neutrino {
    /**
     * @brief Parse an Aseprite JSON (`json-array`) atlas export into a @ref sprite_def.
     *
     * Frames become explicit named visuals `"0".."N-1"` (packed rect from `frame`, trim
     * from `sourceSize`/`spriteSourceSize` when `trimmed`, origin defaulting to the
     * untrimmed top-left -- the basic export carries no pivot). `meta.frameTags` become
     * named clips over the frame indices, honoring `forward`/`reverse` direction and
     * per-frame `duration`. The image source is set to `image_from_disk{meta.image}` (a
     * relative path the caller resolves against the export's directory); `meta.size` fills
     * the declared dimensions.
     *
     * @throws (nlohmann::json) on malformed JSON or a missing required field.
     */
    [[nodiscard]] NEUTRINO_EXPORT sprite_def load_aseprite_atlas(std::string_view json);
}
