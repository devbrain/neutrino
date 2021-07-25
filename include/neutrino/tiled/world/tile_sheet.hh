//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_TILE_SHEET_HH
#define NEUTRINO_TILED_WORLD_TILE_SHEET_HH

#include <neutrino/math/rect.hh>
#include <neutrino/utils/spimpl.h>
#include <neutrino/tiled/world/tile_description.hh>
#include <vector>

namespace neutrino
{
    namespace hal
    {
        class renderer;
        class surface;
    }

    namespace tiled
    {
        namespace detail
        {
            struct tile_sheet_impl;
        }

        class tile_sheet
        {
        public:
            tile_sheet(hal::surface image, const std::vector<math::rect>& coords);
            tile_sheet(hal::surface image);
            ~tile_sheet();

            tile_sheet(tile_sheet&&) = default;
            tile_sheet& operator = (tile_sheet&&) = default;

            [[nodiscard]] bool is_image() const noexcept;
            [[nodiscard]] std::size_t size() const noexcept;
            [[nodiscard]] math::dimension_t dimension(std::size_t idx) const;
        private:
            spimpl::unique_impl_ptr<detail::tile_sheet_impl> m_pimpl;
        };
    } // ns tiled
}
#endif

