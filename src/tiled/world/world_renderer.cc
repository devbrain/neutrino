//
// Created by igor on 07/07/2021.
//

#include <neutrino/tiled/world/world_renderer.hh>
#include <neutrino/tiled/world/world.hh>
#include <neutrino/tiled/world/camera.hh>
#include <neutrino/tiled/world/tile_sheet_manager.hh>
#include <neutrino/hal/video/renderer.hh>
#include <neutrino/utils/override.hh>

namespace neutrino::tiled {

  namespace {
    void clip (math::rect& window, int w, int h) {
      if (window.dims[0] > w) {
        window.dims[0] = w;
      }
      if (window.dims[1] > h) {
        window.dims[1] = h;
      }

      if (window.point[0] < 0) {
        window.point[0] = 0;
      }
      if (window.point[1] < 0) {
        window.point[1] = 0;
      }
      if (window.point[0] + window.dims[0] > w) {
        window.point[0] = w - window.dims[0];
      }
      if (window.point[1] + window.dims[1] > h) {
        window.point[1] = h - window.dims[1];
      }
    }
  }

  // ------------------------------------------------------------------------------------------
  world_renderer::world_renderer (hal::renderer& renderer, tile_sheet_manager& manager)
      : m_renderer (renderer), m_tiles_manager (manager) {
    auto[w, h] = renderer.logical_size ();
    m_screen = {0, 0, (int) w, (int) h};
  }

  // ------------------------------------------------------------------------------------------
  void world_renderer::draw (const world& w, const camera& c) const {
    for (std::size_t layer_id = 0; layer_id < w.m_layers.size (); layer_id++) {

      math::rect window = c.m_cameras[layer_id];

      const auto& layer = w.m_layers[layer_id];

      std::visit (neutrino::utils::overload (
          [this, &window] (const tiles_layer& tl) {
            int width_px = tl.tile_width * tl.w;
            int height_px = tl.tile_height * tl.tile_height;
            clip (window, width_px, height_px);
            draw (tl, window);
          },
          [this, &window] (const image_layer& tl) {
            int width_px = tl.tile_width * tl.w;
            int height_px = tl.tile_height * tl.tile_height;
            clip (window, width_px, height_px);
            draw (tl, window);
          }
      ), layer);

      auto itr = w.m_sprites_to_layers.find (layer_id_t{layer_id});
      const std::vector<std::size_t>* bound_sprites = nullptr;
      if (itr != w.m_sprites_to_layers.end ()) {
        bound_sprites = &itr->second;
      }
    }
  }

  // -----------------------------------------------------------------------------------
  void world_renderer::draw (const tiles_layer& layer, const math::rect& view_port) const {
    int start_x = view_port.point[0];
    int start_y = view_port.point[0];
    int end_x = view_port.point[0] + view_port.dims[0];
    int end_y = view_port.point[1] + view_port.dims[1];

    int tile_start_x = start_x / layer.tile_width;
    int tile_end_x = end_x / layer.tile_width;

    int tile_start_y = start_y / layer.tile_height;
    int tile_end_y = end_y / layer.tile_height;

    int first_tile_offset_x = tile_start_x * layer.tile_width;
    int last_tile_offset_x = tile_end_x * layer.tile_width;

    int first_tile_offset_y = tile_start_y * layer.tile_height;
    int last_tile_offset_y = tile_end_x * layer.tile_height;

    // top left
    math::rect src_rect{0, first_tile_offset_y, first_tile_offset_x, layer.tile_height - first_tile_offset_y};
    math::rect dst_rect = src_rect;

    //draw_tile (layer, tile_start_x, tile_start_y, src_rect, dst_rect);


  }

  // -----------------------------------------------------------------------------------
  void world_renderer::draw (const image_layer& layer, const math::rect& view_port) const {
    auto image = layer.image;
    const auto& sheet = m_tiles_manager.get (image.tile_sheet_id);
    //  sheet.draw(m_renderer, image, view_port, m_screen);
  }
  // -----------------------------------------------------------------------------------
  /*
  void world_renderer::draw_tile(const tiles_layer& layer, int tile_x, int tile_y, const math::rect& src_rect, const math::rect& dst_rect) {

  }
   */
}