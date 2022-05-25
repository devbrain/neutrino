//
// Created by igor on 08/05/2022.
//

#include <vector>
#include <map>
#include <variant>
#include <neutrino/kernel/gfx/texture_atlas.hh>
#include <neutrino/kernel/gfx/grid.hh>
#include <neutrino/hal/video/renderer_utils.hh>
#include "tiled_image.hh"

namespace neutrino::kernel {
  using source_image_t = std::variant<tiled_image, color>;
  struct texture_atlas::impl {
    std::vector<source_image_t> m_atlas;

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
        const auto& parent = std::get<tiled_image>(m_atlas[atlas_id.value_of ()]);
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
        return std::get_if<tiled_image>(&m_atlas.at (atlas_id.value_of ()));
      }
    }
  };

  texture_atlas::texture_atlas ()
      : m_pimpl (spimpl::make_unique_impl<texture_atlas::impl> ()) {
  }

  texture_atlas::~texture_atlas () = default;

  atlas_id_t texture_atlas::add (hal::renderer& renderer, const tilesheet& ts) {
    atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (tiled_image{renderer, ts});
    return rc;
  }

  atlas_id_t texture_atlas::add (hal::renderer& renderer, const image& img) {
    atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (tiled_image{renderer, img});
    return rc;
  }

  atlas_id_t texture_atlas::add (const lazy_tilesheet& ts) {
    atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (tiled_image{ts});
    return rc;
  }

  atlas_id_t texture_atlas::add (const image_loader_t& loader) {
    atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (tiled_image{loader});
    return rc;
  }

  atlas_id_t texture_atlas::add(const color& bgcolor) {
    atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (bgcolor);
    return rc;
  }

  void texture_atlas::replace (atlas_id_t atlas_id, hal::renderer& renderer, const image& img) {
    source_image_t si {tiled_image(renderer, img)};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::replace (atlas_id_t atlas_id, hal::renderer& renderer, const tilesheet& img) {
    source_image_t si {tiled_image(renderer, img)};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::replace (atlas_id_t atlas_id, const lazy_tilesheet& lt) {
    source_image_t si {tiled_image(lt)};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::replace (atlas_id_t atlas_id, const image_loader_t& img_ldr) {
    source_image_t si {tiled_image(img_ldr)};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::replace(atlas_id_t atlas_id, const color& bg_color) {
    source_image_t si {bg_color};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::convert_images (hal::renderer& renderer) {
    for (auto& t: m_pimpl->m_atlas) {
      if (tiled_image* tl = std::get_if<tiled_image>(&t)) {
        tl->convert (renderer);
      }
    }
  }



  bool texture_atlas::is_tilesheet (atlas_id_t atlas_id) const noexcept {
    if (const auto* tl = std::get_if<tiled_image>(&m_pimpl->m_atlas[atlas_id.value_of ()])) {
      return tl->is_tilesheet ();
    }
    return false;
  }


  math::rect texture_atlas::tile_rectangle (const tile_handle& th) const noexcept {
    ENFORCE(th);
    auto atlas_id = static_cast<atlas_id_t >(th);
    if (const auto* tl = std::get_if<tiled_image>(&m_pimpl->m_atlas[atlas_id.value_of ()])) {
      auto tile_id = static_cast<cell_id_t>(th);
      if (!th.is_flipped ()) {
        return tl->tile_rectangle (tile_id);
      }
      else {
        auto original = tl->tile_rectangle (tile_id);
        auto new_dims = grid::eval_transormed_dims (original.dims, th.rotation ());
        return {0, 0, new_dims.x, new_dims.y};
      }
    } else {
      RAISE_EX("Should not be here");
    }
  }

  void texture_atlas::draw (hal::renderer& renderer,
                            const tile_handle& tile,
                            const math::rect& src,
                            const math::point2d& dst_top_left) const {
    if (!tile) {
      return;
    }
    auto atlas_id = static_cast<atlas_id_t >(tile);
    if (const auto* col = std::get_if<color>(&m_pimpl->m_atlas[atlas_id.value_of ()])) {
      math::rect rect(dst_top_left, src.dims);
      auto old = renderer.active_color();
      renderer.active_color(*col);
      renderer.rectangle_filled (rect);
      renderer.active_color(old);
    } else {
      m_pimpl->get_tile_sheet (renderer, tile)->draw (renderer, src, dst_top_left);
    }
  }
}
