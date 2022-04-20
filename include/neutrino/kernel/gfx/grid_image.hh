//
// Created by igor on 19/04/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GFX_GRID_IMAGE_HH
#define INCLUDE_NEUTRINO_KERNEL_GFX_GRID_IMAGE_HH

#include <iosfwd>
#include <functional>
#include <vector>
#include <filesystem>

#include <neutrino/math/rect.hh>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/hal/video/texture.hh>
#include <neutrino/hal/video/renderer.hh>
#include <neutrino/kernel/gfx/tilesheet_info.hh>

namespace neutrino::gfx {
  class grid_image {
    public:
      using rect_func_t = std::function<math::rect(std::size_t idx)>;
    public:
      grid_image(hal::surface surface, const tile_sheet_info& inf);
      grid_image(const std::filesystem::path& path, const tile_sheet_info& inf);
      grid_image(std::istream& is, const tile_sheet_info& inf);

      grid_image(hal::surface surface, std::size_t num_tiles, const rect_func_t& rect_eval);
      grid_image(const std::filesystem::path& path, std::size_t num_tiles, const rect_func_t& rect_eval);
      grid_image(std::istream& is, std::size_t num_tiles, const rect_func_t& rect_eval);

      [[nodiscard]] std::vector<math::rect> positions() const noexcept;
      [[nodiscard]] hal::texture create_texture(const hal::renderer& renderer, hal::color key_color) const;
      [[nodiscard]] hal::texture create_texture(const hal::renderer& renderer) const;
    private:
      void setup_positions(const tile_sheet_info& inf);
      void setup_positions(std::size_t num_tiles, const rect_func_t& rect_eval);
    private:
      hal::surface m_image;
      std::vector<math::rect> m_positions;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_GFX_GRID_IMAGE_HH
