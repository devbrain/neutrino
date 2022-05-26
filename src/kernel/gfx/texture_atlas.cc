//
// Created by igor on 08/05/2022.
//

#include <vector>
#include <map>
#include <variant>
#include "texture_atlas.hh"
#include <neutrino/kernel/gfx/grid.hh>
#include <neutrino/hal/video/renderer_utils.hh>
#include <neutrino/utils/override.hh>
#include "tiled_image.hh"

namespace neutrino::kernel {
  using source_image_t = std::variant<tiled_image, assets::color>;
  struct texture_atlas::impl {

    impl()
    : m_convert_needed(false) {
    }

    std::vector<source_image_t> m_atlas;
    bool m_convert_needed;

    using transformed_map_t = std::map<assets::tile_handle, tiled_image>;
    mutable transformed_map_t m_transformed;

    void remove_flipped(assets::atlas_id_t atlas_id) {
        auto itr = m_transformed.begin();
        while (itr != m_transformed.end()) {
          if (static_cast<assets::atlas_id_t> (itr->first) == atlas_id) {
            itr = m_transformed.erase (itr);
          } else {
            itr++;
          }
        }
    }

    const tiled_image* get_tile_sheet (hal::renderer& r, assets::tile_handle ti) const {
      auto atlas_id = static_cast<assets::atlas_id_t>(ti);
      if (ti.is_flipped ()) {
        auto itr = m_transformed.find (ti);
        if (itr != m_transformed.end ()) {
          return &itr->second;
        }
        const auto ri = ti.rotation ();
        auto cell_id = static_cast<assets::cell_id_t>(ti);
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

    void set(assets::atlas_id_t atlas_id, source_image_t si) {
      auto n = m_atlas.size();
      if (atlas_id.value_of() != n) {
        auto it_pos = m_atlas.begin() + atlas_id.value_of();
        m_atlas.insert (it_pos, std::move(si));
      } else {
        m_atlas.emplace_back (std::move(si));
      }
    }

  };

  texture_atlas::texture_atlas ()
      : m_pimpl (spimpl::make_unique_impl<texture_atlas::impl> ()) {
  }

  texture_atlas::~texture_atlas () = default;

  assets::atlas_id_t texture_atlas::add (hal::renderer& renderer, const assets::tilesheet& ts) {
    assets::atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (tiled_image{renderer, ts});

    return rc;
  }

  assets::atlas_id_t texture_atlas::add (hal::renderer& renderer, const hal::surface& img) {
    assets::atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (tiled_image{renderer, img});
    return rc;
  }

  assets::atlas_id_t texture_atlas::add (const assets::lazy_tilesheet& ts) {
    assets::atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (tiled_image{ts});
    m_pimpl->m_convert_needed = true;
    return rc;
  }

  assets::atlas_id_t texture_atlas::add (const assets::image& loader) {
    assets::atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (tiled_image{loader});
    m_pimpl->m_convert_needed = true;
    return rc;
  }

  assets::atlas_id_t texture_atlas::add(const assets::color& bgcolor) {
    assets::atlas_id_t rc{m_pimpl->m_atlas.size ()};
    m_pimpl->m_atlas.emplace_back (bgcolor);
    return rc;
  }

  void texture_atlas::replace (assets::atlas_id_t atlas_id, hal::renderer& renderer, const hal::surface& img) {
    source_image_t si {tiled_image(renderer, img)};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::replace (assets::atlas_id_t atlas_id, hal::renderer& renderer, const assets::tilesheet& img) {
    source_image_t si {tiled_image(renderer, img)};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::replace (assets::atlas_id_t atlas_id, const assets::lazy_tilesheet& lt) {
    source_image_t si {tiled_image(lt)};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
    m_pimpl->m_convert_needed = true;
  }

  void texture_atlas::replace (assets::atlas_id_t atlas_id, const assets::image& img_ldr) {
    source_image_t si {tiled_image(img_ldr)};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
    m_pimpl->m_convert_needed = true;
  }

  void texture_atlas::replace(assets::atlas_id_t atlas_id, const assets::color& bg_color) {
    source_image_t si {bg_color};
    std::swap(si, m_pimpl->m_atlas[atlas_id.value_of ()]);
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::set(assets::atlas_id_t atlas_id, hal::renderer& renderer, const hal::surface& img) {
    source_image_t si {tiled_image(renderer, img)};
    m_pimpl->set (atlas_id, std::move(si));
    m_pimpl->remove_flipped (atlas_id);
  }
  void texture_atlas::set(assets::atlas_id_t atlas_id, hal::renderer& renderer, const assets::tilesheet & img) {
    source_image_t si {tiled_image(renderer, img)};
    m_pimpl->set (atlas_id, std::move(si));
    m_pimpl->remove_flipped (atlas_id);
  }
  void texture_atlas::set(assets::atlas_id_t atlas_id, const assets::lazy_tilesheet& lt) {
    source_image_t si {tiled_image(lt)};
    m_pimpl->set (atlas_id, std::move(si));
    m_pimpl->remove_flipped (atlas_id);
    m_pimpl->m_convert_needed = true;
  }
  void texture_atlas::set(assets::atlas_id_t atlas_id, const assets::image& img_ldr) {
    source_image_t si {tiled_image(img_ldr)};
    m_pimpl->set (atlas_id, std::move(si));
    m_pimpl->remove_flipped (atlas_id);
    m_pimpl->m_convert_needed = true;
  }

  void texture_atlas::set(assets::atlas_id_t atlas_id, const assets::color& bg_color){
    source_image_t si {bg_color};
    m_pimpl->set (atlas_id, std::move(si));
    m_pimpl->remove_flipped (atlas_id);
  }

  void texture_atlas::assign(const assets::image_atlas& atlas) {
    for (auto i = atlas.begin(); i != atlas.end(); i++) {
      assets::atlas_id_t atlas_id = i->first;
      const auto& val = i->second;
      std::visit(utils::overload(
          [this, atlas_id](const auto& x) {
            set(atlas_id, x);
          }
          ), val);
    }
  }

  void texture_atlas::convert_images (hal::renderer& renderer) {
    if (m_pimpl->m_convert_needed) {
      for (auto& t: m_pimpl->m_atlas) {
        if (tiled_image* tl = std::get_if<tiled_image> (&t)) {
          tl->convert (renderer);
        }
      }
      m_pimpl->m_convert_needed = false;
    }
  }



  bool texture_atlas::is_tilesheet (assets::atlas_id_t atlas_id) const noexcept {
    if (const auto* tl = std::get_if<tiled_image>(&m_pimpl->m_atlas[atlas_id.value_of ()])) {
      return tl->is_tilesheet ();
    }
    return false;
  }


  math::rect texture_atlas::tile_rectangle (const assets::tile_handle& th) const noexcept {
    ENFORCE(th);
    auto atlas_id = static_cast<assets::atlas_id_t >(th);
    if (const auto* tl = std::get_if<tiled_image>(&m_pimpl->m_atlas[atlas_id.value_of ()])) {
      auto tile_id = static_cast<assets::cell_id_t>(th);
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
                            const assets::tile_handle& tile,
                            const math::rect& src,
                            const math::point2d& dst_top_left) const {
    if (!tile) {
      return;
    }
    auto atlas_id = static_cast<assets::atlas_id_t >(tile);
    if (const auto* col = std::get_if<assets::color>(&m_pimpl->m_atlas[atlas_id.value_of ()])) {
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
