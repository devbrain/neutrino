//
// Created by igor on 08/05/2022.
//

#include <vector>
#include <neutrino/kernel/gfx/texture_atlas.hh>
#include "texture.hh"

namespace neutrino::kernel {
  struct texture_atlas::impl {
    std::vector<texture> m_atlas;
  };

  texture_atlas::texture_atlas ()
  : m_pimpl(spimpl::make_unique_impl<texture_atlas::impl>()) {

  }

  texture_atlas::~texture_atlas() = default;

  atlas_id_t texture_atlas::add(hal::renderer& renderer, const image& img) {
    atlas_id_t rc{m_pimpl->m_atlas.size()};
    m_pimpl->m_atlas.emplace_back (renderer, img);
    return rc;
  }

  atlas_id_t texture_atlas::add(lazy_tilesheet& ts) {
    atlas_id_t rc{m_pimpl->m_atlas.size()};
    m_pimpl->m_atlas.emplace_back (ts);
    return rc;
  }

  atlas_id_t texture_atlas::add(image_loader_t& loader) {
    atlas_id_t rc{m_pimpl->m_atlas.size()};
    m_pimpl->m_atlas.emplace_back (loader);
    return rc;
  }

  void texture_atlas::replace(atlas_id_t atlas_id, hal::renderer& renderer, const image& img) {
    texture new_texture(renderer, img);
    texture& old_texture = m_pimpl->m_atlas[atlas_id.value_of()];
    swap (new_texture, old_texture);
  }

  void texture_atlas::replace(atlas_id_t atlas_id, hal::renderer& renderer, const tilesheet & img) {
    texture new_texture(renderer, img);
    texture& old_texture = m_pimpl->m_atlas[atlas_id.value_of()];
    swap (new_texture, old_texture);
  }

  void texture_atlas::replace(atlas_id_t atlas_id, lazy_tilesheet& lt) {
    texture new_texture(lt);
    texture& old_texture = m_pimpl->m_atlas[atlas_id.value_of()];
    swap (new_texture, old_texture);
  }

  void texture_atlas::replace(atlas_id_t atlas_id, image_loader_t& img_ldr) {
    texture new_texture(img_ldr);
    texture& old_texture = m_pimpl->m_atlas[atlas_id.value_of()];
    swap (new_texture, old_texture);
  }

  void texture_atlas::convert_images(hal::renderer& renderer) {
    for (auto& t : m_pimpl->m_atlas) {
      t.convert (renderer);
    }
  }

  atlas_id_t texture_atlas::add(hal::renderer& renderer, const tilesheet& ts) {
    atlas_id_t rc{m_pimpl->m_atlas.size()};
    m_pimpl->m_atlas.emplace_back (renderer, ts);
    return rc;
  }

  bool texture_atlas::is_tilesheet(atlas_id_t atlas_id) const noexcept {
    return m_pimpl->m_atlas[atlas_id.value_of()].is_tilesheet();
  }

  cell_id_t texture_atlas::num_of_tiles(atlas_id_t atlas_id) const noexcept {
    return cell_id_t {m_pimpl->m_atlas[atlas_id.value_of()].num_tiles()};
  }

  tile_data  texture_atlas::tile_rectangle(atlas_id_t atlas_id, cell_id_t tile_id) const noexcept {
    return {std::make_pair(atlas_id, tile_id),
                     m_pimpl->m_atlas[atlas_id.value_of()].tile_rectangle (tile_id.value_of())};
  }

  void texture_atlas::draw(hal::renderer& renderer, const tile_data& tile, const math::point2d& dst_top_left) const {
    m_pimpl->m_atlas[tile.tile_id.first.value_of()].draw (renderer, tile.src, dst_top_left);
  }
}