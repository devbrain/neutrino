//
// Created by igor on 08/07/2021.
//

#include <fstream>
#include <neutrino/tiled/loader/tmx.hh>
#include <neutrino/tiled/world/tile_sheet_manager.hh>
#include <neutrino/tiled/world/world.hh>
#include <neutrino/utils/exception.hh>
#include <pugixml.hpp>

#include "map.hh"


namespace neutrino::tiled::tmx {

    std::unique_ptr<world> load(std::filesystem::path& path, tile_sheet_manager& tsm) {
        std::ifstream ifs(path, std::ios::binary | std::ios::in);
        return load(ifs, tsm);
    }
    // ---------------------------------------------------------------------------------------
    std::unique_ptr<world> load(std::istream& is, tile_sheet_manager& tsm) {
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
        map::parse(xml_node(root), [](const std::string&) {return "";});
        return nullptr;
    }
}
