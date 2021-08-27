//
// Created by igor on 08/07/2021.
//

#include <fstream>
#include <neutrino/tiled/loader/tmx.hh>
#include <neutrino/tiled/world/tile_sheet_manager.hh>
#include <neutrino/tiled/world/world.hh>
#include <neutrino/utils/exception.hh>

#include "xml_reader.hh"
#include "json_reader.hh"
#include "map.hh"

namespace neutrino::tiled::tmx {

  map load_map (const char *data, std::size_t length, path_resolver_t resolver) {
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
  std::unique_ptr<world> load (std::filesystem::path &path, path_resolver_t resolver, tile_sheet_manager &tsm) {
    std::ifstream ifs (path, std::ios::binary | std::ios::in);
    return load (ifs, resolver, tsm);
  }
  // ---------------------------------------------------------------------------------------
  std::unique_ptr<world> load (std::istream &is, path_resolver_t resolver, tile_sheet_manager &tsm) {
    /*
     pugi::xml_document doc;
     pugi::xml_parse_result result = doc.load(is);
    if (!result)
    {
        RAISE_EX("Failed to load tmx :", result.description());
    }
    auto root = doc.child("map");
    if (!root)
    {
        RAISE_EX ("entry node <map> is missing");
    }
    map::parse(xml_reader(root), [](const std::string&) {return "";});
     */
    return nullptr;
  }
}
