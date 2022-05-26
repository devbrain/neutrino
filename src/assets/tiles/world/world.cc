//
// Created by igor on 09/05/2022.
//
/**
 * TODO:
 * transparent images
 * tile layer paralax
 * tile layer offset
 * tile layer tint
 */

#include <fstream>
#include <iostream>
#include <map>
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/override.hh>
#include <neutrino/utils/io/memory_stream_buf.hh>
#include <neutrino/assets/tiles/world/world.hh>
#include <neutrino/assets/image/image_io.hh>
#include <utility>

#include "assets/tiles/tmx/map.hh"
#include "assets/tiles/tmx/json_reader.hh"
#include "assets/tiles/tmx/xml_reader.hh"

namespace neutrino::assets {
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

  std::tuple<std::size_t, std::size_t> world::dimensions_in_pixels() const {
    return std::make_tuple (m_width*m_tile_width, m_height*m_tile_height);
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
  /*
   * TMX BUILDER
   */

  using gid_map_t = std::map<unsigned int, std::tuple<atlas_id_t, std::size_t>>;
  using ani_map_t = std::map<unsigned int, std::tuple<animation_seq_id_t, unsigned int, const std::vector<tmx::frame>*>>;

  static std::tuple<gid_map_t, ani_map_t> build_atlas(const tmx::map& map, const path_resolver_t& resolver, world_assets& atlas) {
    gid_map_t gid_map;
    ani_map_t ani_map;

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
      auto lazy_loader_fn = [resolver, name]() -> hal::surface {
        std::string input = resolver(name);
        utils::io::memory_input_stream is(input.c_str(), static_cast<std::streamsize>(input.size()));
        return load_image (is);
      };

      tilesheet_info ti (ts.tile_width(), ts.tile_height(), ts.spacing(), ts.margin(), ts.offset_x(), ts.offset_y(), ts.tile_count());
      lazy_tilesheet_info lti(*ts_img->width(), *ts_img->height(), ti);
      lazy_tilesheet lazy_ts = make_tilesheet (image(lazy_loader_fn), lti);
      auto id = atlas.images.add (lazy_ts);
      gid_map.insert (std::make_pair (ts.first_gid(), std::make_tuple (id, ts.tile_count())));
      for (const auto& tli : ts) {
        if (tli.has_image()) {
          RAISE_EX("Error in tileset ", ts.name(), " : tile images are not supported");
        }
        const auto& tla = tli.get_animation();
        const auto tile_id = tli.id();
        if (!tla.frames().empty()) {
          auto asq = atlas.animation_sequences.create (animation_description::CIRCULAR);
          ani_map.insert(std::make_pair(ts.first_gid() + tile_id, std::make_tuple (asq.key(), ts.first_gid(), &tla.frames())));
        }
      }
    }

    return {gid_map, ani_map};
  }

  static std::pair<atlas_id_t, cell_id_t> lookup_gid(unsigned int gid, const gid_map_t& gid_map) {
    for (const auto& [k, v] : gid_map) {
      auto sz = std::get<1>(v);
      if ( gid >=k && gid < k + sz) {
        atlas_id_t atlas_id = std::get<0>(v);
        cell_id_t cell_id = cell_id_t(gid - k);
        return {atlas_id, cell_id};
      }
    }
    return {make_invalid<atlas_id_t>(), make_invalid<cell_id_t>()};
  }

  static tile_handle create_tile(const tmx::cell& c,
                                 const std::tuple<gid_map_t, ani_map_t>& mapping,
                                 animation_description& ani_descr) {
    const auto& [gid_map, ani_map] = mapping;
    auto ani_itr = ani_map.find (c.gid ());
    if (ani_itr != ani_map.end ()) {
      const auto [ani_seq_id, ts_gid, frames] = ani_itr->second;
      ENFORCE(frames != nullptr);
      ENFORCE(ani_descr.exists (ani_seq_id));
      auto ani_seq = ani_descr.get (ani_seq_id);
      for (const auto& frame : *frames) {
        auto gid = ts_gid + frame.id();
        auto delay = frame.duration();
        auto tl = lookup_gid (gid, gid_map);
        ani_seq.add (tile_handle (tl.first, tl.second), delay);
      }
      return tile_handle (ani_seq_id);
    }
    else {
      bool h_flipped = c.hor_flipped ();
      bool v_flipped = c.vert_flipped ();
      bool d_flipped = c.diag_flipped ();

      auto tl = lookup_gid (c.gid (), gid_map);
      return {tl.first, tl.second, h_flipped, v_flipped, d_flipped};
    }
  }

  static tiles_layer create_tiles_layer(const tmx::tile_layer& tlayer,
                                        const std::tuple<gid_map_t, ani_map_t>& mapping,
                                        animation_description& ani_descr) {

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
      auto th = create_tile (c, mapping, ani_descr);

      res.set (x, y, th);
      x++;
      if (x >= tlayer.width ()) {
        x = 0;
        y++;
      }
    }
    return res;
  }

  world world::from_tmx(const tmx::map& map, const path_resolver_t& resolver, world_assets& assets) {
    world w (map.width(), map.height(), map.tile_width(), map.tile_height());
    auto mappings = build_atlas (map, resolver, assets);
    if (map.background_color() != tmx::colori()) {
      color c;
      c.r = map.background_color().r;
      c.g = map.background_color().g;
      c.b = map.background_color().b;
      c.a = map.background_color().a;
      auto atlas_id = assets.images.add(c);
      w.m_layers.push_back (color_layer{tile_handle(atlas_id, cell_id_t(0))});
    }
    for (const auto& layer : map.layers()) {
      std::visit (
          utils::overload(
              [&w, &resolver, &assets](const tmx::image_layer& img_layer) {
                const auto* image_data = img_layer.get_image();
                ENFORCE(image_data != nullptr);
                auto tint = img_layer.tint();
                auto offset_x = img_layer.offset_x();
                auto offset_y = img_layer.offset_y();
                auto name = image_data->source();

                auto lazy_loader_fn = [resolver, name, tint, offset_x, offset_y]() -> hal::surface {
                  std::string input = resolver(name);
                  utils::io::memory_input_stream is(input.c_str(), static_cast<std::streamsize>(input.size()));
                  auto s = load_image (is);
                  if (offset_x > 0 || offset_y > 0) {
                    auto [w, h] = s.dimensions();
                    ENFORCE(offset_x < (int)w && offset_y < (int)h);
                    hal::surface s2(w-offset_x, h-offset_y, s.get_pixel_format());
                    math::rect src_rect(offset_x, offset_y, (int)(w-offset_x), (int)(h-offset_y));
                    math::rect dst_rect(0, 0, (int)(w-offset_x), (int)(h-offset_y));
                    if (tint != tmx::colori(255, 255, 255, 255)) {
                      s.color_mod (tint.r, tint.b, tint.b);
                    }
                    s.blit (src_rect, s2, dst_rect);
                    return s2;
                  }
                  if (tint != tmx::colori(255, 255, 255, 255)) {
                    s.color_mod (tint.r, tint.b, tint.b);
                    auto [w, h] = s.dimensions();
                    hal::surface s2(w, h, s.get_pixel_format());
                    s.blit (s2);
                    return s2;
                  }
                  return s;
                };

                auto atlas_id = assets.images.add (image(lazy_loader_fn));
                image_layer img_l(tile_handle(atlas_id, cell_id_t(0)));
                w.m_layers.push_back (img_l);
                },
              [&w, &mappings, &assets](const tmx::tile_layer& tile_layer) {
                w.m_layers.push_back (create_tiles_layer (tile_layer, mappings, assets.animation_sequences));
              }
              ),
              layer
          );
    }
    return w;
  }

  world world::from_tmx(std::istream& is, const path_resolver_t& resolver, world_assets& assets) {
    return from_tmx (load_tmx (is, resolver), resolver, assets);
  }

  world world::from_tmx(const std::filesystem::path& path, const path_resolver_t& resolver, world_assets& assets) {
    std::ifstream ifs(path, std::ios::binary | std::ios::in);
    if (!ifs) {
      RAISE_EX("Failed to open file ", path);
    }
    return from_tmx(ifs, resolver, assets);
  }
}
