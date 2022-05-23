//
// Created by igor on 11/05/2022.
//
#include <algorithm>
#include <neutrino/kernel/gfx/world_renderer.hh>
#include <neutrino/hal/video/renderer_utils.hh>
#include <neutrino/kernel/gfx/grid.hh>
#include <neutrino/utils/override.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {
  world_window::world_window(math::point2d screen_pos, math::dimension2di_t dimensions)
  : m_screen_top_left(screen_pos), m_world_top_left(), m_dimensions(dimensions) {

  }

  world_window::world_window(math::point2d screen_pos, math::point2d world_pos, math::dimension2di_t dimensions)
      : m_screen_top_left(screen_pos), m_world_top_left(world_pos), m_dimensions(dimensions) {

  }

  void world_window::screen_pos(math::point2d pos) noexcept {
    m_screen_top_left = pos;
  }
  math::point2d world_window::screen_pos() const noexcept {
    return m_screen_top_left;
  }

  void world_window::add_screen_pos(math::point2d pos) noexcept {
    m_screen_top_left += pos;
  }

  void world_window::add_screen_pos(int dx, int dy) noexcept {
    m_screen_top_left += math::point2d(dx, dy);
  }

  void world_window::world_pos(math::point2d pos) noexcept {
    m_world_top_left = pos;
  }

  math::point2d world_window::world_pos() const noexcept {
    return m_world_top_left;
  }

  void world_window::add_world_pos(math::point2d pos) noexcept {
    m_world_top_left += pos;
  }

  void world_window::add_world_pos(int dx, int dy) noexcept {
    m_world_top_left += math::point2d (dx, dy);
  }

  void world_window::dimensions(math::dimension2di_t dims) noexcept {
    m_dimensions = dims;
  }

  math::dimension2di_t world_window::dimensions() const noexcept {
    return m_dimensions;
  }

  void world_window::add_dimensions(math::dimension2di_t dims) noexcept {
    m_dimensions += dims;
  }

  void world_window::add_dimensions(int dx, int dy) noexcept {
    m_dimensions += math::dimension2di_t (dx, dy);
  }

  void world_window::clip(const math::dimension2di_t& screen_dims, const math::dimension2di_t& world_dims) {
    // fix dimensions
    if (m_dimensions.x <= 0) {
      m_dimensions.x = 1;
    } else if (m_dimensions.x >= screen_dims.x) {
      m_dimensions.x = screen_dims.x;
    }
    if (m_dimensions.y <= 0) {
      m_dimensions.y = 1;
    } else if (m_dimensions.y >= screen_dims.y) {
      m_dimensions.y = screen_dims.y;
    }

    // fix screen
    if (m_screen_top_left.x < 0) {
      m_screen_top_left.x = 0;
    }
    if (m_screen_top_left.y < 0) {
      m_screen_top_left.y = 0;
    }
    if (m_screen_top_left.x + m_dimensions.x > screen_dims.x) {
      m_screen_top_left.x = screen_dims.x - m_dimensions.x;
    }
    if (m_screen_top_left.y + m_dimensions.y > screen_dims.y) {
      m_screen_top_left.y = screen_dims.y - m_dimensions.y;
    }
    // fix world
    if (m_world_top_left.x < 0) {
      m_world_top_left.x = 0;
    }
    if (m_world_top_left.y < 0) {
      m_world_top_left.y = 0;
    }
    if (m_world_top_left.x + m_dimensions.x > world_dims.x) {
      m_world_top_left.x = world_dims.x - m_dimensions.x;
    }
    if (m_world_top_left.y + m_dimensions.y > world_dims.y) {
      m_world_top_left.y = world_dims.y - m_dimensions.y;
    }
  }

  world_renderer::world_renderer()
  : m_world(nullptr), m_assets(nullptr) {}

  void world_renderer::set(world* w) {
    m_world = w;
    init_animation_state();
  }
  void world_renderer::set(world* w, const gfx_assets* atlas) {
    m_world = w;
    m_assets = atlas;
    init_animation_state();
  }


  void world_renderer::update (std::chrono::milliseconds ms) {
    m_animation_state.update (ms);
  }

  void world_renderer::init_animation_state() {
    ENFORCE(m_assets != nullptr);
    for (auto& layer : m_world->layers()) {
      std::visit(
          utils::overload (
              [this](tiles_layer& tlayer) {
                for (std::size_t y=0; y<tlayer.height(); y++) {
                  for (std::size_t x=0; x<tlayer.width(); x++) {
                    auto th = tlayer.get (x, y);
                    if (th.empty()) {
                      continue;
                    }
                    if (th.is_animation()) {
                      auto aseq_id = static_cast<animation_seq_id_t>(th);
                      th = m_animation_state.add (m_assets->animation_sequences.get(aseq_id));
                      tlayer.set(x, y, th);
                    }
                  }
                }
              },
              [](auto&) {}),
              layer);
    }
  }

  tile_handle world_renderer::get_tile_data(const tiles_layer& tlayer, int x, int y) {
      auto th = tlayer.get (x, y);
      if (!th) {
        return th;
      }
      if (th.is_animation()) {
        auto state_id = static_cast<animation_state_id_t>(th);
        auto [handle, kind] = m_animation_state.frame (state_id);
        if (handle) {
          return handle;
        } else {
          return {};
        }
      } else {
        return th;
      }
  }

  void world_renderer::draw(const world_window& window, hal::renderer& renderer) {
    if (!m_world || !m_assets) {
      return;
    }
    hal::clip_area ca(renderer, math::rect(window.screen_pos(), window.dimensions()));

    for (const auto& layer : m_world->layers()) {
      std::visit(
          utils::overload (
                [&window, &renderer](const color& c) {
                    math::rect out(window.screen_pos(), window.dimensions());
                    auto old = renderer.active_color();
                    renderer.active_color(c);
                    renderer.rectangle_filled (out);
                    renderer.active_color(old);
                },
                [this, &window, &renderer](const image_layer& img) {
                      auto tlid = img.tile_id();
                      auto atlas_id = static_cast<atlas_id_t>(tlid);
                      ENFORCE(!m_assets->textures.is_tilesheet (atlas_id));
                      auto src = m_assets->textures.tile_rectangle (tlid);
                      grid wc(m_world);
                      wc.evaluate (window);
                      auto screen_pos = window.screen_pos();
                      auto start_x = screen_pos.x;
                      auto tw = (int)m_world->tile_width();
                      auto th = (int)m_world->tile_height();
                      int h = 0;
                      for (int y = wc.top_left_tile_y(); y <= wc.bottom_right_tile_y(); y++) {
                        for (int x = wc.top_left_tile_x (); x <= wc.bottom_right_tile_x (); x++) {
                            src.point = screen_pos;
                            src.dims.x = tw;
                            src.dims.y = th;
                            wc.adjust (x, y, src);
                            m_assets->textures.draw (renderer, tlid, src, screen_pos);
                            screen_pos.x += src.dims.x;
                            h = std::max (src.dims.y, h);
                        }
                        screen_pos.x = start_x;
                        screen_pos.y += h;
                      }
                },
                [this, &window, &renderer](const tiles_layer& tlayer) {
                  grid wc(m_world);
                  wc.evaluate (window);
                  auto screen_pos = window.screen_pos();
                  auto start_x = screen_pos.x;
                  auto h = 0;
                  for (int y = wc.top_left_tile_y(); y <= wc.bottom_right_tile_y(); y++) {
                    for (int x = wc.top_left_tile_x(); x <= wc.bottom_right_tile_x(); x++) {
                      auto tlid = get_tile_data (tlayer, x, y);
                      if (tlid) {
                          auto src = m_assets->textures.tile_rectangle (tlid);
                          wc.adjust (x, y, src);
                          m_assets->textures.draw (renderer, tlid, src, screen_pos);
                          h = std::max (src.dims.y, h);
                          screen_pos.x += src.dims.x;
                      } else {
                        auto d = wc.empty (x, y);
                        h = std::max (d.dims.y, h);
                        screen_pos.x += d.dims.x;
                      }
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