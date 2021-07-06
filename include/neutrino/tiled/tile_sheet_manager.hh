//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_TILE_SHEET_MANAGER_HH
#define NEUTRINO_TILED_TILE_SHEET_MANAGER_HH

#include <neutrino/tiled/tile_sheet.hh>
#include <filesystem>
#include <iosfwd>

namespace neutrino::tiled {
    class tile_sheet_manager {
    public:
        explicit tile_sheet_manager(hal::renderer& renderer);

        std::size_t add(const hal::surface& image, const std::vector<math::rect>& coords);
        std::size_t add(const hal::surface& image);
        std::size_t add(std::istream& is, const std::vector<math::rect>& coords);
        std::size_t add(std::istream& is);
        std::size_t add(const std::filesystem::path& path, const std::vector<math::rect>& coords);
        std::size_t add(const std::filesystem::path& path);

    private:
        hal::renderer&          m_renderer;
        std::vector<tile_sheet> m_sheets;
    };
}

#endif
