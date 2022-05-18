//
// Created by igor on 09/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_RC_WORLD_WORLD_HH
#define INCLUDE_NEUTRINO_KERNEL_RC_WORLD_WORLD_HH

#include <variant>
#include <vector>
#include <istream>
#include <filesystem>

#include <neutrino/kernel/rc/image.hh>
#include <neutrino/kernel/rc/world/image_layer.hh>
#include <neutrino/kernel/rc/world/tiles_layer.hh>
#include <neutrino/kernel/rc/world/object.hh>
#include <neutrino/kernel/rc/path_resolver.hh>
#include <neutrino/kernel/gfx/gfx_assets.hh>


namespace neutrino::kernel {
  namespace tmx {
    class map;
  }

  class world {
    public:
      using layer_t = std::variant<image_layer, tiles_layer, color>;
      using layers_t = std::vector<layer_t>;
      using objects_t = std::vector<object>;
    public:

      world() = default;

      static world from_tmx(std::istream& is, const path_resolver_t& resolver, gfx_assets& accets);
      static world from_tmx(const std::filesystem::path& path, const path_resolver_t& resolver, gfx_assets& assets);

      world(std::size_t width, std::size_t height, std::size_t tile_width, std::size_t tile_height);

      void add(image_layer layer);
      void add(tiles_layer layer);
      void add(object obj);

      layers_t& layers() noexcept;
      [[nodiscard]] const layers_t& layers() const noexcept;

      objects_t& objects() noexcept;
      [[nodiscard]] const objects_t& objects() const noexcept;

      [[nodiscard]] std::size_t width() const noexcept;
      [[nodiscard]] std::size_t height() const noexcept;
      [[nodiscard]] std::size_t tile_width() const noexcept;
      [[nodiscard]] std::size_t tile_height() const noexcept;

      [[nodiscard]] std::tuple<std::size_t, std::size_t> dimensions_in_pixels() const;
    private:
      static world from_tmx(const tmx::map& map, const path_resolver_t& resolver, gfx_assets& assets);
    private:
      std::size_t m_width;
      std::size_t m_height;
      std::size_t m_tile_width;
      std::size_t m_tile_height;
      layers_t m_layers;
      objects_t m_objects;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_RC_WORLD_WORLD_HH
