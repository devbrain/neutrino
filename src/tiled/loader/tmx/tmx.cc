//
// Created by igor on 08/07/2021.
//

#include <fstream>
#include <algorithm>

#include <neutrino/tiled/loader/tmx.hh>
#include <neutrino/tiled/world/world.hh>
#include <neutrino/tiled/world/tile_sheet_builder.hh>

#include <neutrino/utils/exception.hh>
#include <utility>

#include "xml_reader.hh"
#include "json_reader.hh"
#include "color.hh"
#include "map.hh"

namespace neutrino::tiled::tmx {
  // first_gid -> tileset
  using gid_tileset_t = std::tuple<unsigned int, unsigned int>;
  using gid_map_t = std::vector<gid_tileset_t>;
  using ts_builder_info_t = std::vector<tile_sheet_info>;

  static std::tuple<gid_map_t, ts_builder_info_t> create_tilesheet_info(map& raw) {
    auto& tilesets = raw.tile_sets ();
    if (tilesets.empty ()) {
      RAISE_EX("No tilesets found in the map");
    }
    ts_builder_info_t tiles_builder_info;

    gid_map_t gid_map;
    std::size_t tileset_index = 0;
    for (auto& ts : tilesets) {
      gid_map.emplace_back (ts.first_gid (), tileset_index++);

      // prepare image
      auto* img = ts.get_image ();
      if (!img) {
        RAISE_EX("tileset (\"", ts.name (), "\" : first_gid = ", ts.first_gid (), ") does not have attached image");
      }
      std::optional<hal::color> transparent_color;
      if (img->transparent ()) {
        colori tcl{*img->transparent ()};
        transparent_color = hal::color{tcl.r, tcl.g, tcl.b, tcl.a};
      }
      if (!img->source ().empty ()) {
        tiles_builder_info.emplace_back (
            named_image{transparent_color, img->size (), img->format (), img->source ()},
            ts.tile_width (),
            ts.tile_height (),
            ts.spacing (),
            ts.margin (),
            ts.offset_x(),
            ts.offset_y(),
            ts.tile_count()
            );
      }
      else {
        if (!img->data ().empty ()) {
          tiles_builder_info.emplace_back (
              // steal image data here
              image_data{transparent_color, img->size (), img->format (), std::move (img->data ())},
              ts.tile_width (),
              ts.tile_height (),
              ts.spacing (),
              ts.margin (),
              ts.offset_x(),
              ts.offset_y(),
              ts.tile_count()
              );
        }
        else {
          RAISE_EX("tileset (\"", ts.name (), "\" : first_gid = ", ts.first_gid (), ") does has an empty image definition");
        }
      }
    }
    std::sort(gid_map.begin(), gid_map.end(), [](const gid_tileset_t& a, const gid_tileset_t& b) {
      return std::get<0>(a) < std::get<0>(b);
    });

    return {std::move(gid_map), std::move(tiles_builder_info)};
  }
  // ==================================================================================================
  static map load_map (const char* data, std::size_t length, path_resolver_t resolver) {
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

  // ---------------------------------------------------------------------------------------
  std::unique_ptr<world> load (std::filesystem::path& path, path_resolver_t resolver) {
    std::ifstream ifs (path, std::ios::binary | std::ios::in);
    if (!ifs) {
      RAISE_EX("Failed to open map file ", path);
    }
    try {
      return load (ifs, std::move (resolver));
    }
    catch (exception& e) {
      RAISE_EX_WITH_CAUSE(std::move (e), "Failed to load map file", path);
    }
  }

  // ---------------------------------------------------------------------------------------
  std::unique_ptr<world> load (std::istream& is, path_resolver_t resolver) {
    auto txt = std::vector<char> ((std::istreambuf_iterator<char> (is)),
                                  std::istreambuf_iterator<char> ());
    return load (txt.data (), txt.size (), std::move (resolver));
  }

  // ---------------------------------------------------------------------------------------
  std::unique_ptr<world> load (const char* text, std::size_t size, path_resolver_t resolver) {
    auto raw = load_map (text, size, std::move (resolver));

    auto [gid_map, tiles_builder_info] = create_tilesheet_info (raw);
    tile_sheet_builder ts_bilder;

    for (const auto& [gid, idx] : gid_map) {
      ts_bilder << std::move(tiles_builder_info[idx]);
    }
    return nullptr;
  }
}
