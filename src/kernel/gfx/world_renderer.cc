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

  class world_tiles_translator {
    public:
      explicit world_tiles_translator (const world* w);
      world_tiles_translator(unsigned tile_width, unsigned tile_height, unsigned width_in_tiles, unsigned height_in_tiles);

      void evaluate (int world_pos_x, int world_pos_y, int window_width, int window_height);
      void evaluate (const world_window& w);

      [[nodiscard]] int top_left_tile_x() const;
      [[nodiscard]] int top_left_tile_y() const;
      [[nodiscard]] int bottom_right_tile_x() const;
      [[nodiscard]] int bottom_right_tile_y() const;

      [[nodiscard]] int up_pixels_start() const;
      [[nodiscard]] int left_pixels_start() const;
      [[nodiscard]] int bottom_pixels_end() const;
      [[nodiscard]] int right_pixels_end() const;

      [[nodiscard]] int tile_width() const;
      [[nodiscard]] int tile_height() const;

      void adjust(int tx, int ty, math::rect& r) const;
    private:
      int m_tile_width;
      int m_tile_height;
      int m_world_width;
      int m_world_height;

      int m_top_left_tile_x;
      int m_top_left_tile_y;
      int m_bottom_right_tile_x;
      int m_bottom_right_tile_y;

      int m_up_pixels_start;
      int m_left_pixels_start;
      int m_bottom_pixels_end;
      int m_right_pixels_end;
  };

  void world_renderer::draw(const world_window& window, hal::renderer& renderer) {
    if (!m_world || !m_atlas) {
      return;
    }

    //world_coords wc(m_world, window);


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
                [this, &window, &renderer](const tiles_layer& tlayer) {
                  world_tiles_translator wc(m_world);
                  wc.evaluate (window);
                  auto screen_pos = window.screen_pos();
                  auto start_x = screen_pos.x;
                  auto h = 0;
                  for (int y = wc.top_left_tile_y(); y <= wc.bottom_right_tile_y(); y++) {
                    for (int x = wc.top_left_tile_x(); x <= wc.bottom_right_tile_x(); x++) {
                      auto tlid = tlayer.get (x, y);
                      if (tlid.valid()) {
                        auto tdi = m_atlas->tile_rectangle (tlid);
                        wc.adjust (x, y, tdi.src);
                        m_atlas->draw (renderer, tdi, screen_pos);
                        h = std::max (tdi.src.dims.y, h);
                        screen_pos.x += tdi.src.dims.x;
                      } else {
                        math::rect src(0, 0, wc.tile_width(), wc.tile_height());
                        wc.adjust (x, y, src);
                        h = std::max (src.dims.y, h);
                        screen_pos.x += src.dims.x;
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

  world_tiles_translator::world_tiles_translator (const world* w)
  : world_tiles_translator(w->tile_width(), w->tile_height(), w->width(), w->height()) {
  }

  world_tiles_translator::world_tiles_translator(unsigned tile_width, unsigned tile_height,
                                                 unsigned width_in_tiles, unsigned height_in_tiles)
                                                 :m_tile_width(static_cast<int>(tile_width)),
                                                  m_tile_height(static_cast<int>(tile_height)),
                                                  m_world_width(static_cast<int>(width_in_tiles)),
                                                  m_world_height(static_cast<int>(height_in_tiles))
                                                 {
  }

  static int clamp(int v, int m, int M) {
    if (v < m) {
      return m;
    }
    if (v > M) {
      return M;
    }
    return v;
  }

  void world_tiles_translator::evaluate (const world_window& w) {
      auto wp = w.world_pos();
      auto d = w.dimensions();
    evaluate (wp[0], wp[1], d[0], d[1]);
  }

  void world_tiles_translator::evaluate (int world_pos_x, int world_pos_y, int window_width, int window_height) {
    auto wtx = clamp (world_pos_x, 0, m_world_width*m_tile_width - window_width);
    auto wty = clamp (world_pos_y, 0, m_world_height*m_tile_height - window_height);

    auto wbx = wtx + window_width;
    auto wby = wty + window_height;

    m_top_left_tile_x = wtx / m_tile_width;
    m_left_pixels_start = wtx % m_tile_width;

    m_top_left_tile_y = wty / m_tile_height;
    m_up_pixels_start = wty % m_tile_height;

    m_bottom_right_tile_x = wbx / m_tile_width;
    m_right_pixels_end = wbx % m_tile_width;

    if (m_right_pixels_end == 0) {
      m_bottom_right_tile_x--;
      m_right_pixels_end = m_tile_width;
    }

    m_bottom_right_tile_y = wby / m_tile_height;
    m_bottom_pixels_end = wby % m_tile_height;
    if (m_bottom_pixels_end == 0) {
      m_bottom_right_tile_y --;
      m_bottom_pixels_end = m_tile_height;
    }

  }

  void world_tiles_translator::adjust(int tx, int ty, math::rect& r) const {
    if (tx == m_top_left_tile_x) {
      r.point.x += m_left_pixels_start;
      r.dims.x -= m_left_pixels_start;
    } else if (tx == m_bottom_right_tile_x) {
      r.dims.x = m_right_pixels_end;
    }

    if (ty == m_top_left_tile_y) {
      r.point.y += m_up_pixels_start;
      r.dims.y -= m_up_pixels_start;
    } else if (ty == m_bottom_right_tile_y) {
      r.dims.y = m_bottom_pixels_end;
    }
  }

  int world_tiles_translator::top_left_tile_x() const {
    return m_top_left_tile_x;
  }

  int world_tiles_translator::top_left_tile_y() const {
    return m_top_left_tile_y;
  }

  int world_tiles_translator::bottom_right_tile_x() const {
    return m_bottom_right_tile_x;
  }

  int world_tiles_translator::bottom_right_tile_y() const {
    return m_bottom_right_tile_y;
  }

  int world_tiles_translator::up_pixels_start() const {
    return m_up_pixels_start;
  }

  int world_tiles_translator::left_pixels_start() const {
    return m_left_pixels_start;
  }

  int world_tiles_translator::bottom_pixels_end() const {
    return m_bottom_pixels_end;
  }

  int world_tiles_translator::right_pixels_end() const {
    return m_right_pixels_end;
  }

  int world_tiles_translator::tile_width() const {
    return m_tile_width;
  }

  int world_tiles_translator::tile_height() const {
    return m_tile_height;
  }
}