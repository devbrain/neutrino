//
// Created by igor on 06/07/2021.
//

#include <neutrino/tiled/tile_sheet_manager.hh>
#include <neutrino/hal/video/image_loader.hh>

namespace neutrino::tiled {
    tile_sheet_manager::tile_sheet_manager(hal::renderer& renderer)
    : m_renderer(renderer) {}
    // ------------------------------------------------------------------------------------------------------------
    std::size_t tile_sheet_manager::add(const hal::surface& image, const std::vector<math::rect>& coords) {
        auto n = m_sheets.size();
        m_sheets.emplace_back(m_renderer, image, coords);
        return n;
    }
    // ------------------------------------------------------------------------------------------------------------
    std::size_t tile_sheet_manager::add(const hal::surface& image) {
        auto n = m_sheets.size();
        m_sheets.emplace_back(m_renderer, image);
        return n;
    }
    // ------------------------------------------------------------------------------------------------------------
    std::size_t tile_sheet_manager::add(std::istream& is, const std::vector<math::rect>& coords) {
        return add(hal::load(is), coords);
    }
    // ------------------------------------------------------------------------------------------------------------
    std::size_t tile_sheet_manager::add(std::istream& is) {
        return add(hal::load(is));
    }
    // ------------------------------------------------------------------------------------------------------------
    std::size_t tile_sheet_manager::add(const std::filesystem::path& path, const std::vector<math::rect>& coords) {
        return add(hal::load(path), coords);
    }
    // ------------------------------------------------------------------------------------------------------------
    std::size_t tile_sheet_manager::add(const std::filesystem::path& path) {
        return add(hal::load(path));
    }
}