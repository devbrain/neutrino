//
// Created by igor on 26/07/2021.
//

#include "terrain.hh"

namespace neutrino::tiled::tmx {
    terrain terrain::parse(const xml_node& elt) {
        try {
            auto name = elt.get_attribute("name");
            auto tile = elt.get_attribute<unsigned>("tile");

            terrain result(name, tile);
            component::parse(result, elt);
            return result;
        } catch(exception& e) {
            auto name = elt.get_string_attribute("name", Requirement::OPTIONAL, "<unknown>");
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse terrain [", name, "]");
        }
    }
}