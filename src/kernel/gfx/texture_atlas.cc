//
// Created by igor on 08/05/2022.
//

#include <vector>
#include <tuple>
#include <map>
#include <neutrino/kernel/gfx/texture_atlas.hh>
#include <neutrino/kernel/gfx/grid.hh>
#include <neutrino/hal/video/renderer_utils.hh>
#include "tiled_image.hh"

namespace neutrino::kernel {
  struct texture_atlas::impl {
    std::vector<tiled_image> m_atlas;

    using transformed_map_t = std::map<tile_handle, tiled_image>;
    mutable transformed_map_t m_transformed;

    void remove_flipped(atlas_id_t atlas_id) {
        auto itr = m_transformed.begin();
        while (itr != m_transformed.end()) {
          if (static_cast<atlas_id_t> (itr->first) == atlas_id) {
            itr = m_transformed.erase (itr);
          } else {
            itr++;
          }
        }
    }

    const tiled_image* get_tile_sheet (hal::renderer& r, tile_handle ti) const {
      auto atlas_id = static_cast<atlas_id_t>(ti);
      if (ti.is_flipped ()) {
        auto itr = m_transformed.find (ti);
        if (itr != m_transformed.end ()) {
          return &itr->second;
        }
        const auto ri = ti.rotation ();
        auto cell_id = static_cast<cell_id_t>(ti);
        const auto& parent = m_atlas[atlas_id.value_of ()];
        const auto& parent_texture = parent.texture();
        auto original = parent.tile_rectangle (cell_id);
        auto new_dims = grid::eval_transormed_dims (original.dims, ri);
        itr = m_transformed.insert (std::make_pair (ti, tiled_image (r, parent_texture.get_pixel_format(), new_dims))).first;

        hal::use_texture tt (r, itr->second.texture ());

        math::rect dst (0, 0, new_dims.x, new_dims.y);
        auto flip = hal::renderer::flip::NONE;
        if (ri.hflip) {
          flip = hal::renderer::flip::HORIZONTAL;
        }
        else if (ri.vflip) {
          flip = hal::renderer::flip::VERTICAL;
        }
        r.copy (parent_texture, original, dst, ri.degree, flip);
        return &itr->second;
      }
      else {
        return &m_atlas.at (atlas_id.value_of ());
      }
    }
  };

  texture_atlas::texture_atlas ()
      : m_pimpl (spimpl::make_unique_impl<texture_atlas::impl> ()) {
  }

  texture_atlas::~texture_atlas () = default;

  atlas_id_t texture_atlas::add (hal::renderer& renderer, const image& img) {
    atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (renderer, img);
    return rc;
  }

  atlas_id_t texture_atlas::add (const lazy_tilesheet& ts) {
    atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (ts);
    return rc;
  }

  atlas_id_t texture_atlas::add (const image_loader_t& loader) {
    atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (loader);
    return rc;
  }

  void texture_atlas::replace (atlas_id_t atlas_id, hal::renderer& renderer, const image& img) {
    tiled_image new_texture (renderer, img);
    tiled_image& old_texture = m_pimpl->m_atlas[atlas_id.value_of ()];
    swap (new_texture, old_texture);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::replace (atlas_id_t atlas_id, hal::renderer& renderer, const tilesheet& img) {
    tiled_image new_texture (renderer, img);
    tiled_image& old_texture = m_pimpl->m_atlas[atlas_id.value_of ()];
    swap (new_texture, old_texture);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::replace (atlas_id_t atlas_id, const lazy_tilesheet& lt) {
    tiled_image new_texture (lt);
    tiled_image& old_texture = m_pimpl->m_atlas[atlas_id.value_of ()];
    swap (new_texture, old_texture);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::replace (atlas_id_t atlas_id, const image_loader_t& img_ldr) {
    tiled_image new_texture (img_ldr);
    tiled_image& old_texture = m_pimpl->m_atlas[atlas_id.value_of ()];
    swap (new_texture, old_texture);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::convert_images (hal::renderer& renderer) {
    for (auto& t: m_pimpl->m_atlas) {
      t.convert (renderer);
    }
  }

  atlas_id_t texture_atlas::add (hal::renderer& renderer, const tilesheet& ts) {
    atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (renderer, ts);
    return rc;
  }

  bool texture_atlas::is_tilesheet (atlas_id_t atlas_id) const noexcept {
    return m_pimpl->m_atlas[atlas_id.value_of ()].is_tilesheet ();
  }


  math::rect texture_atlas::tile_rectangle (const tile_handle& th) const noexcept {
    auto atlas_id = static_cast<atlas_id_t >(th);
    auto tile_id = static_cast<cell_id_t>(th);
    if (!th.is_flipped ()) {
      return m_pimpl->m_atlas[atlas_id.value_of ()].tile_rectangle (tile_id);
    }
    else {
      auto original = m_pimpl->m_atlas[atlas_id.value_of ()].tile_rectangle (tile_id);
      auto new_dims = grid::eval_transormed_dims (original.dims, th.rotation ());
      return {0, 0, new_dims.x, new_dims.y};
    }
  }

  void texture_atlas::draw (hal::renderer& renderer,
                            const tile_handle& tile,
                            const math::rect& src,
                            const math::point2d& dst_top_left) const {
    m_pimpl->get_tile_sheet (renderer, tile)->draw (renderer, src, dst_top_left);
  }

}