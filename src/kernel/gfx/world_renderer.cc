//
// Created by igor on 11/05/2022.
//
#include <algorithm>
#include <neutrino/kernel/gfx/world_renderer.hh>
#include <neutrino/kernel/gfx/grid.hh>
#include <neutrino/hal/video/renderer_utils.hh>
#include <neutrino/utils/override.hh>
#include <neutrino/utils/exception.hh>

#include "animation_state.hh"
#include "texture_atlas.hh"

namespace neutrino::kernel {

  struct world_renderer::impl {
    impl ()
        : m_world (nullptr) {
    }

    void init_animation_state (const assets::animation_description& ad);
    [[nodiscard]] std::tuple<assets::tile_handle, math::rect> get_tile_data (const assets::tiles_layer& tlayer, int x, int y) const;
    [[nodiscard]] std::tuple<assets::tile_handle, math::rect> get_tile_data (const assets::image_layer& ilayer, int x, int y) const;
    [[nodiscard]] std::tuple<assets::tile_handle, math::rect> get_tile_data (const assets::color_layer& clayer, int x, int y) const;

    assets::world* m_world;
    texture_atlas m_textures;
    animation_state m_animation_state;
    grid m_grid;
  };

  void world_renderer::impl::init_animation_state (const assets::animation_description& ad) {
    m_animation_state.clear();
    for (auto& layer: m_world->layers ()) {
      std::visit (
          utils::overload (
              [this, &ad] (assets::tiles_layer& tlayer) {
                for (std::size_t y = 0; y < tlayer.height (); y++) {
                  for (std::size_t x = 0; x < tlayer.width (); x++) {
                    auto th = tlayer.get (x, y);
                    if (th.empty ()) {
                      continue;
                    }
                    if (th.is_animation ()) {
                      auto aseq_id = static_cast<assets::animation_seq_id_t>(th);
                      th = m_animation_state.add (ad.get (aseq_id));
                      tlayer.set (x, y, th);
                    }
                  }
                }
              },
              [] (auto&) {}),
          layer);
    }
  }

  std::tuple<assets::tile_handle, math::rect> world_renderer::impl::get_tile_data (const assets::tiles_layer& tlayer, int x, int y) const {
    auto th = tlayer.get (x, y);
    assets::tile_handle rc;
    if (th) {
      if (th.is_animation ()) {
        auto state_id = static_cast<assets::animation_state_id_t>(th);
        auto [handle, kind] = m_animation_state.frame (state_id);
        if (handle) {
          rc = handle;
        }
      }
      else {
        rc = th;
      }
    }
    if (rc) {
      auto src = m_textures.tile_rectangle (rc);
      m_grid.adjust (x, y, src);
      return {rc, src};
    }
    return {rc, m_grid.empty (x, y)};
  }

  std::tuple<assets::tile_handle, math::rect> world_renderer::impl::get_tile_data (const assets::image_layer& ilayer, int x, int y) const {
    auto tlid = ilayer.tile_id ();
    auto atlas_id = static_cast<assets::atlas_id_t>(tlid);
    ENFORCE(!m_textures.is_tilesheet (atlas_id));
    auto src = m_grid.empty (x, y);
    return {tlid, src};
  }

  std::tuple<assets::tile_handle, math::rect> world_renderer::impl::get_tile_data (const assets::color_layer& clayer, int x, int y) const {
    auto tlid = clayer.tile_id ();
    auto src = m_grid.empty (x, y);
    return {tlid, src};
  }

  world_renderer::world_renderer ()
      : m_pimpl (spimpl::make_unique_impl<impl> ()) {
  }

  world_renderer::~world_renderer () = default;

  void world_renderer::set (assets::world* w) {
    m_pimpl->m_world = w;
    m_pimpl->m_grid = grid(w);
    //m_pimpl->init_animation_state ();
  }

  void world_renderer::set (assets::world* w, const assets::world_assets* atlas) {
    m_pimpl->m_world = w;
    m_pimpl->m_grid = grid(w);
    m_pimpl->m_textures.assign (atlas->images);
    m_pimpl->init_animation_state (atlas->animation_sequences);
  }

  void world_renderer::update (std::chrono::milliseconds ms) {
    m_pimpl->m_animation_state.update (ms);
  }

  void world_renderer::draw (const world_window& window, hal::renderer& renderer) {
    if (!m_pimpl->m_world) {
      return;
    }
    m_pimpl->m_textures.convert_images (renderer);
    hal::clip_area ca (renderer, math::rect (window.screen_pos (), window.dimensions ()));

    for (const auto& layer: m_pimpl->m_world->layers ()) {
      std::visit (
          utils::overload (
              [this, &window, &renderer] (const auto& tlayer) {
                m_pimpl->m_grid.evaluate (window);
                auto screen_pos = window.screen_pos ();
                auto start_x = screen_pos.x;
                auto h = 0;
                const auto& grid = m_pimpl->m_grid;
                const auto min_y = grid.top_left_tile_y ();
                const auto max_y = grid.bottom_right_tile_y ();
                const auto min_x = grid.top_left_tile_x ();
                const auto max_x  = grid.bottom_right_tile_x ();

                for (int y = min_y; y <= max_y; y++) {
                  for (int x = min_x; x <= max_x; x++) {
                    auto [tlid, src] = m_pimpl->get_tile_data (tlayer, x, y);
                    m_pimpl->m_textures.draw (renderer, tlid, src, screen_pos);
                    h = std::max (src.dims.y, h);
                    screen_pos.x += src.dims.x;
                  }
                  screen_pos.x = start_x;
                  screen_pos.y += h;
                  h = 0;
                }
              }
          ),
          layer
      );
    }
  }
}
