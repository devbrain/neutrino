//
// Created by igor on 16/11/2026.
//

#pragma once

#include <cstdint>
#include <optional>
#include <neutrino/video/layer_style.hh>
#include <neutrino/neutrino_export.h>

namespace neutrino {
    /**
     * @brief What a layer holds in a frame that does not draw it (painter.md §2.5).
     */
    enum class redraw_mode : std::uint8_t {
        /// @brief Cleared at its first push in each frame; a frame that does not push it shows
        ///        nothing of it. The default.
        every_frame,
        /// @brief Kept between frames: a frame that does not push it shows its old contents,
        ///        and only `clear()` empties it. Its owner redraws it when it chooses, and in full
        ///        when `needs_redraw()` says so.
        on_demand,
    };

    /**
     * @brief What `declare_layer` is given (painter.md §2.5). Every field has a default.
     */
    struct NEUTRINO_EXPORT layer_settings {
        /// @brief The layer's own resolution. Unset: the pixel size of the target the scene is
        ///        drawn into (R42), followed through resizes.
        std::optional <dim> size;
        /// @brief See @ref redraw_mode.
        redraw_mode redraw{redraw_mode::every_frame};
        /// @brief How the layer is drawn when its scene's layers are combined; `style_of(id)`
        ///        changes it later.
        layer_style style{};
    };
}
