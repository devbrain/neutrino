//
// Created by igor on 20/09/2020.
//

#include <array>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/utils/io/binary_reader.hh>
#include <neutrino/utils/exception.hh>

#include "game_assets.hh"

static constexpr std::size_t NUM_COLORS = 256;

neutrino::hal::palette load_palette (std::istream* is) {
  neutrino::utils::io::binary_reader io (*is, neutrino::utils::io::binary_reader::LITTLE_ENDIAN_BYTE_ORDER);
  neutrino::hal::palette pal (NUM_COLORS);
  for (std::size_t i = 0; i < NUM_COLORS; i++) {
    bool rc = true;
    uint8_t r, g, b;
    io >> r >> g >> b;
    if (!io.good ()) {
      rc = false;
    }
    pal.set (i, {
        (uint8_t) ((4u * r) & 0xFFu),
        (uint8_t) ((4u * g) & 0xFFu),
        (uint8_t) ((4u * b) & 0xFFu),
        0xFF
    });

    if (!rc) {
      break;
    }
  }
  return pal;
}

// --------------------------------------------------------------------------------------------------------------------
neutrino::hal::surface load_backdrop (std::istream* is) {
  neutrino::utils::io::binary_reader io (*is, neutrino::utils::io::binary_reader::LITTLE_ENDIAN_BYTE_ORDER);

  std::uint16_t w, h;

  auto pal = load_palette (is);
  ENFORCE(io.good ());

  io >> w >> h;
  ENFORCE(io.good ());

  std::vector<uint8_t> buff (w * h);
  io.read_raw (buff.data (), buff.size ());
  ENFORCE(io.good ());

  auto s = neutrino::hal::surface::make_8bit (w, h);
  s.set_palette (pal);
  union {
    uint8_t* rgba;
    void* pixels;
  } u{};

  std::size_t pitch;
  std::tie (u.pixels, pitch, std::ignore, std::ignore) = s.pixels_data ();

  uint8_t* row = u.rgba;
  for (uint16_t y = 0; y < h; y++) {
    for (uint16_t x = 0; x < w; x++) {
      ((uint8_t*) (row))[x] = buff[w * y + x];
    }
    row += pitch;
  }
  return s;
}

// --------------------------------------------------------------------------------------------------------------------
std::tuple<neutrino::hal::surface, std::vector<neutrino::math::rect>> load_tiles (std::istream* is,
                                                                                  const neutrino::hal::palette& pal) {
  std::vector<uint8_t> tiles;
  std::size_t num_tiles = 0;
  neutrino::utils::io::binary_reader io (*is, neutrino::utils::io::binary_reader::LITTLE_ENDIAN_BYTE_ORDER);
  while (true) {
    tiles.resize ((num_tiles + 1) * 256);
    io.read_raw (tiles.data () + num_tiles * 256, 256);
    if (!io.good ()) {
      break;
    }
    num_tiles++;
  }
  auto s = neutrino::hal::surface::make_rgba (16, 16 * num_tiles);

  union {
    uint8_t* rgba;
    void* pixels;
  } u{};

  std::size_t pitch;
  std::tie (u.pixels, pitch, std::ignore, std::ignore) = s.pixels_data ();
  uint8_t* row = u.rgba;
  std::vector<neutrino::math::rect> rects;
  for (std::size_t tl = 0; tl < num_tiles; tl++) {
    const uint8_t* curr_tile = tiles.data () + tl * 256;
    rects.emplace_back (0, tl * 16, 16, 16);
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        auto color_idx = curr_tile[16 * y + x];
        if (color_idx == 0) {
          ((uint32_t*) (row))[x] = s.map_color ({0, 0, 0, 0});
        }
        else {
          ((uint32_t*) (row))[x] = s.map_color (pal[color_idx]);
        }
      }
      row += pitch;
    }
  }
  return std::make_tuple (std::move (s), rects);
}

static std::array<uint8_t, 8> white{15, 15, 15, 15,
                                    15, 15, 15, 15};

static std::array<uint8_t, 8> grey{21, 22, 23, 24,
                                   25, 26, 27, 28};

static std::array<uint8_t, 8> ice{52, 53, 54, 55,
                                  56, 57, 58, 59};

static std::array<uint8_t, 8> fire{40, 41, 42, 43,
                                   44, 45, 46, 47};

static std::array<uint8_t, 8> swamp{48, 49, 50, 51,
                                    52, 53, 54, 55};

static std::array<uint8_t, 8> sand{42, 42, 43, 43,
                                   44, 44, 45, 45};

// --------------------------------------------------------------------------------------------------------------------

static std::tuple<neutrino::hal::surface, neutrino::math::rect> draw_glyph (uint8_t font_data[64][8],
                                                       int glyph_num,
                                                       const std::array<uint8_t, 8>& colors,
                                                       const neutrino::hal::palette& pal) {
  auto s = neutrino::hal::surface::make_rgba (8,8);

  uint8_t mask = 1;

  union {
    uint8_t* rgba;
    void* pixels;
  } u{};

  std::size_t pitch;
  std::tie (u.pixels, pitch, std::ignore, std::ignore) = s.pixels_data ();
  uint8_t* row = u.rgba;
  for (int r = 0; r < 8; r++) {
    mask = 1;
    for (int c = 7; c >= 0; c--) {
      uint32_t col = s.map_color ({0, 0, 0, 0});
      if ((font_data[glyph_num][r] & mask) == mask) {
        col = s.map_color (pal[colors[r]]);

      }

      ((uint32_t*) (row))[c] = col;
      mask = ((mask << 1u) & 0xFFu);
    }
    row += pitch;
  }
  return std::make_tuple (std::move (s), neutrino::math::rect{0, 0, 8, 8});
}

// --------------------------------------------------------------------------------------------------------------------
template <typename F>
static void load_fonts (std::istream *is, const neutrino::hal::palette& pal, const std::array<uint8_t, 8>& schema, F func) {
  uint8_t font_data[64][8] = {{0}};
  neutrino::utils::io::binary_reader io (*is, neutrino::utils::io::binary_reader::LITTLE_ENDIAN_BYTE_ORDER);
  io.read_raw ((char*)font_data, sizeof (font_data));
  ENFORCE(io.good());

  for (int i = 0; i < 64; i++) {
    func (draw_glyph (font_data, i, schema, pal));
  }
}
// --------------------------------------------------------------------------------------------------------------------
#if 0
#define MAX_LEVEL_NUM 112

static problem_descr desc[MAX_LEVEL_NUM] = {
    {1, 1, 1},
    {1, 1, 2},
    {1, 1, 3},
    {1, 1, 4},

    {2, 1, 1},
    {2, 1, 2},
    {2, 1, 3},
    {2, 1, 4},
    {2, 2, 1},
    {2, 2, 2},
    {2, 2, 3},
    {2, 2, 4},

    {3, 1, 1},
    {3, 1, 2},
    {3, 1, 3},
    {3, 1, 4},
    {3, 2, 1},
    {3, 2, 2},
    {3, 2, 3},
    {3, 2, 4},
    {3, 3, 1},
    {3, 3, 2},
    {3, 3, 3},
    {3, 3, 4},

    {4, 1, 1},
    {4, 1, 2},
    {4, 1, 3},
    {4, 1, 4},
    {4, 2, 1},
    {4, 2, 2},
    {4, 2, 3},
    {4, 2, 4},
    {4, 3, 1},
    {4, 3, 2},
    {4, 3, 3},
    {4, 3, 4},
    {4, 4, 1},
    {4, 4, 2},
    {4, 4, 3},
    {4, 4, 4},

    {5, 1, 1},
    {5, 1, 2},
    {5, 1, 3},
    {5, 1, 4},
    {5, 2, 1},
    {5, 2, 2},
    {5, 2, 3},
    {5, 2, 4},
    {5, 3, 1},
    {5, 3, 2},
    {5, 3, 3},
    {5, 3, 4},
    {5, 4, 1},
    {5, 4, 2},
    {5, 4, 3},
    {5, 4, 4},
    {5, 5, 1},
    {5, 5, 2},
    {5, 5, 3},
    {5, 5, 4},

    {6, 1, 1},
    {6, 1, 2},
    {6, 1, 3},
    {6, 1, 4},
    {6, 2, 1},
    {6, 2, 2},
    {6, 2, 3},
    {6, 2, 4},
    {6, 3, 1},
    {6, 3, 2},
    {6, 3, 3},
    {6, 3, 4},
    {6, 4, 1},
    {6, 4, 2},
    {6, 4, 3},
    {6, 4, 4},
    {6, 5, 1},
    {6, 5, 2},
    {6, 5, 3},
    {6, 5, 4},
    {6, 6, 1},
    {6, 6, 2},
    {6, 6, 3},
    {6, 6, 4},

    {7, 1, 1},
    {7, 1, 2},
    {7, 1, 3},
    {7, 1, 4},
    {7, 2, 1},
    {7, 2, 2},
    {7, 2, 3},
    {7, 2, 4},
    {7, 3, 1},
    {7, 3, 2},
    {7, 3, 3},
    {7, 3, 4},
    {7, 4, 1},
    {7, 4, 2},
    {7, 4, 3},
    {7, 4, 4},
    {7, 5, 1},
    {7, 5, 2},
    {7, 5, 3},
    {7, 5, 4},
    {7, 6, 1},
    {7, 6, 2},
    {7, 6, 3},
    {7, 6, 4},
    {7, 7, 1},
    {7, 7, 2},
    {7, 7, 3},
    {7, 7, 4}
};

// -------------------------------------------------------------------------------------------------------------------
static map_item_t translate_item (int x) {
  switch (x) {
    case 0:
      return map_item_t::EMPTY;

    case 11:
      return map_item_t::BLOCK1;
    case 10:
      return map_item_t::BLOCK2;

    case 1:
      return map_item_t::GEM1;
    case 2:
      return map_item_t::GEM2;
    case 3:
      return map_item_t::GEM3;
    case 4:
      return map_item_t::GEM4;
    case 5:
      return map_item_t::GEM5;
    case 6:
      return map_item_t::GEM6;
    case 7:
      return map_item_t::GEM7;
    case 8:
      return map_item_t::GEM8;
    case 12:
      return map_item_t::PLATFORM_V;
    case 52:
      return map_item_t::DESTRUCTABLE;
    case 60:
      return map_item_t::PORTAL1;
    case 61:
      return map_item_t::PORTAL2;
    case 62:
      return map_item_t::PORTAL3;
    case 63:
      return map_item_t::PORTAL4;
    case 64:
      return map_item_t::PORTAL5;
    case 65:
      return map_item_t::PORTAL6;
    case 66:
      return map_item_t::PORTAL7;
    case 67:
      return map_item_t::PORTAL8;
    case 68:
      return map_item_t::PORTAL9;
    case 69:
      return map_item_t::PORTAL10;

    case 90:
      return map_item_t::CLOCK;
  };

  return map_item_t::UNDEFINED;
}

// -------------------------------------------------------------------------------------------------------------------
static level_map load_maps (sdl::io& io, int map_num) {
  static constexpr int w = 14;
  static constexpr int h = 12;
  static constexpr int size = w * h;
  uint8_t data[size + 10];
  Uint8 hot_x;
  Uint8 hot_y;
  Uint8 dummy[6];
  Uint8 m;
  Uint8 s;

  level_map mp;
  mp.w = w;
  mp.h = h;
  mp.problem = desc[map_num];

  if (io.read (data, sizeof (data), 1) != 1) {
    RAISE_EX("Failed to read level data");
  }

  mp.hot_x = data[size + 0];
  mp.hot_y = data[size + 1];
  mp.mins = data[size + 8];
  mp.secs = data[size + 9];

  mp.tiles.resize (size, map_item_t::UNDEFINED);

  for (int i = 0; i < size; i++) {
    mp.tiles[i] = translate_item (((int) data[i]) & 0xFF);
  }
  return mp;
}

// --------------------------------------------------------------------------------------------------------------------
game_assets::game_assets (resource_loader& loader, graphic_assets_builder& g_assets_builder) {
  auto palette = load_palette (loader.load (resource_t::PALETTE));
  m_logo = asset_id_t (0);
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::LOGO))));
  m_title = asset_id_t (1);
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::TITLE1))));
  m_first_backdrop = asset_id_t (2);
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC1))));
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC2))));
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC3))));
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC4))));
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC5))));
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC6))));
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC7))));
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC8))));
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC9))));
  m_backdrops.push_back (g_assets_builder.add_texture (load_backdrop (loader.load (resource_t::PIC10))));

  std::tie (m_first_tile, m_last_tile) = g_assets_builder.add_tile (
      load_tiles (loader.load (resource_t::BLOCKS), palette));

  std::array<uint8_t, 8>* schemas[] = {
      &white,
      &grey,
      &ice,
      &fire,
      &swamp,
      &sand,
      nullptr
  };

  int k = 0;
  while (true) {
    auto* schema = schemas[k++];
    if (!schema) {
      break;
    }
    std::vector<tile_id_t> curr;
    load_fonts (loader.load (resource_t::FONT), palette, *schema,
                [&g_assets_builder, &curr] (const std::tuple<sdl::surface, sdl::rect>& t) {
                  curr.push_back (g_assets_builder.add_tile (t));
                });
    m_glyphs.push_back (curr);
  }
  k = 0;
  auto levels_io = loader.load (resource_t::LEVELS1);
  while (true) {
    try {
      m_maps.push_back (load_maps (levels_io, k++));
    }
    catch (...) {
      break;
    }
  }
}
#endif
