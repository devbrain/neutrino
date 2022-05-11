//
// Created by igor on 09/05/2022.
//
/**
 * TODO:
 * transparent images
 * image layers
 * tile layer paralax
 * tile layer offset
 * tile layer tint
 * cells flip
 */


#include <fstream>
#include <map>
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/override.hh>
#include <neutrino/utils/io/memory_stream_buf.hh>
#include <neutrino/kernel/rc/world/world.hh>
#include <neutrino/kernel/rc/image.hh>
#include <utility>

#include "kernel/rc/tmx/map.hh"
#include "kernel/rc/tmx/json_reader.hh"
#include "kernel/rc/tmx/xml_reader.hh"

namespace neutrino::kernel {
  world::world (std::size_t width, std::size_t height, std::size_t tile_width,
                std::size_t tile_height)
      : m_width (width),
        m_height (height),
        m_tile_width (tile_width),
        m_tile_height (tile_height) {
  }

  void world::add (image_layer layer) {
    m_layers.emplace_back (layer);
  }

  void world::add (tiles_layer layer) {
    m_layers.emplace_back (std::move (layer));
  }

  void world::add (object obj) {
    m_objects.emplace_back (std::move (obj));
  }

  world::layers_t& world::layers () noexcept {
    return m_layers;
  }

  const world::layers_t& world::layers () const noexcept {
    return m_layers;
  }

  world::objects_t& world::objects () noexcept {
    return m_objects;
  }

  const world::objects_t& world::objects () const noexcept {
    return m_objects;
  }

  std::size_t world::width () const noexcept {
    return m_width;
  }

  std::size_t world::height () const noexcept {
    return m_height;
  }

  std::size_t world::tile_width () const noexcept {
    return m_tile_width;
  }

  std::size_t world::tile_height () const noexcept {
    return m_tile_height;
  }

  static tmx::map load_tmx(std::istream& is, path_resolver_t resolver) {
    using namespace tmx;
    std::istreambuf_iterator<char> eos;
    std::string s(std::istreambuf_iterator<char>(is), eos);
    const auto* data = s.c_str();
    const auto length = s.size();
    auto doc_type = reader::guess_document_type (data, length);
    switch (doc_type) {
      case reader::XML_DOCUMENT:
        return map::parse (xml_reader::load (data, length, "map"), std::move (resolver));
      case reader::JSON_DOCUMENT:
        return map::parse (json_reader::load (data, length, nullptr), std::move (resolver));
      default:
        RAISE_EX("Unknown document type");
    }
  }

  using gid_map_t = std::map<unsigned int, std::tuple<atlas_id_t, std::size_t>>;

  static gid_map_t build_atlas(const tmx::map& map, const path_resolver_t& resolver, texture_atlas& atlas) {
    gid_map_t gid_map;

    for (const auto& ts : map.tile_sets()) {
      if (!ts.has_image()) {
        RAISE_EX("Tileset ", ts.name(), " should contain image");
      }
      auto* ts_img = ts.get_image();
      if (ts_img->transparent()) {
        RAISE_EX("Transparent field is not supported");
      }
      if (!ts_img->width() || !ts_img->height()) {
        RAISE_EX("Image without dimensions is not supported");
      }
      auto name = ts_img->source();
      auto lazy_loader_fn = [resolver, name]() -> image {
        std::string input = resolver(name);
        utils::io::memory_input_stream is(input.c_str(), input.size());
        return load_image (is);
      };



      tilesheet_info ti (ts.tile_width(), ts.tile_height(), ts.spacing(), ts.margin(), ts.offset_x(), ts.offset_y(), ts.tile_count());
      lazy_tilesheet_info lti(*ts_img->width(), *ts_img->height(), ti);
      lazy_tilesheet lazy_ts = make_tilesheet (lazy_loader_fn, lti);
      auto id = atlas.add (lazy_ts);
      gid_map.insert (std::make_pair (ts.first_gid(), std::make_tuple (id, ts.tile_count())));
    }

    return gid_map;
  }

  static tile_id_t lookup_gid(unsigned int gid, const gid_map_t& gid_map) {
    for (const auto& [k, v] : gid_map) {
      auto sz = std::get<1>(v);
      if ( gid >=k && gid < k + sz) {
        atlas_id_t atlas_id = std::get<0>(v);
        cell_id_t cell_id = cell_id_t(gid - k);
        return {atlas_id, cell_id};
      }
    }
    return {};
  }

  static tiles_layer create_tiles_layer(const tmx::tile_layer& tlayer, const gid_map_t& gid_map) {
    tiles_layer res(tlayer.width(), tlayer.height());
    if (tlayer.parallax_x() > 1 || tlayer.parallax_y() > 1) {
      RAISE_EX("Paralax is not supported yet");
    }
    if (tlayer.offset_x() > 0 || tlayer.offset_y() > 0) {
      RAISE_EX("Offset is not supported yet");
    }

    if (!tlayer.chunks().empty()) {
      RAISE_EX("Chunks are not supported yet");
    }

    ENFORCE(!tlayer.cells().empty());
    std::size_t x = 0;
    std::size_t y = 0;
    for (const auto c : tlayer.cells()) {
      if (c.diag_flipped() || c.hor_flipped() || c.vert_flipped()) {
        RAISE_EX("Flipped cells are not supported yet");
      }
      auto tl = lookup_gid (c.gid(), gid_map);
      res.set(x, y, tl.atlas_id, tl.cell_id);
      x++;
      if (x >= tlayer.width()) {
        x = 0;
        y++;
      }
    }
    return res;
  }

  world world::from_tmx(const tmx::map& map, const path_resolver_t& resolver, texture_atlas& atlas) {
    world w (map.width(), map.height(), map.tile_width(), map.tile_height());
    auto gid_map = build_atlas (map, resolver, atlas);

    for (const auto& layer : map.layers()) {
      std::visit (
          utils::overload(
              [](const tmx::image_layer& img_layer) {
                RAISE_EX("Image layer is not supported");
                },
              [&w, &gid_map](const tmx::tile_layer& tile_layer) {
                w.m_layers.push_back (create_tiles_layer (tile_layer, gid_map));
              }
              ),
              layer
          );
    }

    return w;
  }

  world world::from_tmx(std::istream& is, const path_resolver_t& resolver, texture_atlas& atlas) {
    return from_tmx (load_tmx (is, resolver), resolver, atlas);
  }

  world world::from_tmx(const std::filesystem::path& path, const path_resolver_t& resolver, texture_atlas& atlas) {
    std::ifstream ifs(path, std::ios::binary | std::ios::in);
    if (!ifs) {
      RAISE_EX("Failed to open file ", path);
    }
    return from_tmx(ifs, resolver, atlas);
  }

}