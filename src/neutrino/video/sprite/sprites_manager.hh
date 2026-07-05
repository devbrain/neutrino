//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstdint>

#include <neutrino/video/sprite/sprite_sheet.hh>

#include "resource_registry.hh"

namespace neutrino {
    /**
     * @brief Internal owner of registered sprite sheet resources.
     *
     * The manager stores static sprite sheet metadata only. It does not own world
     * objects, positions, physics state, or animation clocks.
     */
    class sprites_manager {
        public:
            /**
             * @brief Store a sheet and return its opaque handle.
             */
            sprite_sheet_id create(sprite_sheet sheet);

            /**
             * @brief Resolve a mutable registered sheet.
             *
             * @pre @p id must identify a sheet stored in this manager.
             */
            sprite_sheet& get(sprite_sheet_id id);

            /**
             * @brief Resolve a const registered sheet.
             *
             * @pre @p id must identify a sheet stored in this manager.
             */
            const sprite_sheet& get(sprite_sheet_id id) const;

            /**
             * @brief Remove a registered sheet if present.
             */
            void erase(sprite_sheet_id id);

        private:
            static sprite_sheet_id make_id(std::uint32_t value);

            details::resource_registry <sprite_sheet_id, sprite_sheet> m_sheets;
    };
}
