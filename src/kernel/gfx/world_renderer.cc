//
// Created by igor on 11/05/2022.
//
#include <algorithm>
#include <neutrino/kernel/gfx/world_renderer.hh>
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
  : m_world(nullptr), m_atlas(nullptr) {}

  void world_renderer::set(world* w) {
    m_world = w;
  }

  void world_renderer::set(const texture_atlas* atlas) {
    m_atlas = atlas;
  }

  void world_renderer::update (std::chrono::milliseconds ms) {
    // TODO
  }



  struct world_coords {
    world_coords(const world* w, const world_window& window) {
      tl_w = w->tile_width();
      tl_h = w->tile_height();

      int tiles_in_window_w = window.dimensions().x / tl_w;
      if (window.dimensions().x % tl_w) {
        tiles_in_window_w++;
      }
      if (tiles_in_window_w > w->width()) {
        tiles_in_window_w = w->width();
      }

      int tiles_in_window_h = window.dimensions().y / tl_h;
      if (window.dimensions().y % tl_h) {
        tiles_in_window_h++;
      }
      if (tiles_in_window_h > w->height()) {
        tiles_in_window_h = w->height();
      }


      top_left_tile_x = window.world_pos().x / tl_w;
      if (top_left_tile_x + tiles_in_window_w >= w->width()) {
        top_left_tile_x = w->width() - tiles_in_window_w;
        left_offset_px = 0;
      } else {
        left_offset_px = window.world_pos ().x % tl_w;
      }

      top_left_tile_y = window.world_pos().y / tl_h;
      if (top_left_tile_y + tiles_in_window_h >= w->height()) {
        top_left_tile_y = w->height() - tiles_in_window_h;
        top_offset_px = 0;
      } else {
        top_offset_px = window.world_pos ().y % tl_h;
      }

      auto bottom = window.world_pos() + window.dimensions();

      bottom_right_tile_x = bottom.x / tl_w;
      if (bottom_right_tile_x >= w->width()) {
        bottom_right_tile_x = w->width() - 1;
        right_offset_px = 0;
      } else {
        right_offset_px = window.dimensions ().x % tl_w;
      }

      bottom_right_tile_y = bottom.y / tl_h;
      if (bottom_right_tile_y >= w->height()) {
        bottom_right_tile_y = w->height() - 1;
        bottom_offset_px = 0;
      } else {
        bottom_offset_px = window.dimensions ().y % tl_h;
      }
    }

    void adjust(math::rect& rect, int x, int y) {
      if (x == top_left_tile_x) {
        rect.point.x += left_offset_px;
        rect.dims.x -= left_offset_px;
      } else if (x == bottom_right_tile_y) {
        rect.dims.x = right_offset_px;
      }

      if (y == top_left_tile_y) {
        rect.point.y += top_offset_px;
        rect.dims.y -= top_offset_px;
      } else if (y == bottom_right_tile_y) {
        rect.dims.y = bottom_offset_px;
      }
    }

    int tl_w;
    int tl_h;

    int top_left_tile_x;
    int left_offset_px;

    int top_left_tile_y;
    int top_offset_px;

    int bottom_right_tile_x;
    int right_offset_px;

    int bottom_right_tile_y;
    int bottom_offset_px;
  };

  void world_renderer::draw(const world_window& window, hal::renderer& renderer) {
    if (!m_world || !m_atlas) {
      return;
    }

    world_coords wc(m_world, window);

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
                      ENFORCE(!m_atlas->is_tilesheet (tlid.atlas_id));
                      auto tdi = m_atlas->tile_rectangle (tlid);
                      tdi.src.dims = window.dimensions();
                      m_atlas->draw(renderer, tdi, window.screen_pos());
                },
                [this, &window, &renderer, &wc](const tiles_layer& tlayer) {
                  auto screen_pos = window.screen_pos();
                  auto start_x = screen_pos.x;
                  auto h = 0;
                  for (int y = wc.top_left_tile_y; y <= wc.bottom_right_tile_y; y++) {
                    for (int x = wc.top_left_tile_x; x <= wc.bottom_right_tile_x; x++) {
                      auto tlid = tlayer.get (x, y);
                      if (tlid.valid()) {
                        auto tdi = m_atlas->tile_rectangle (tlid);
                        wc.adjust (tdi.src, x, y);
                        m_atlas->draw (renderer, tdi, screen_pos);
                        h = std::max (tdi.src.dims.y, h);
                        screen_pos.x += tdi.src.dims.x;
                      } else {
                        math::rect src(0,0, wc.tl_w, wc.tl_h);
                        wc.adjust (src, x, y);
                        h = std::max (src.dims.y, h);
                        screen_pos.x += src.dims.x;
                      }
                    }
                    screen_pos.x = start_x;
                    screen_pos.y += h;
                  }
                }
              ),
          layer
          );
    }
  }
}