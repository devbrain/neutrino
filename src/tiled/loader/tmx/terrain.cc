//
// Created by igor on 26/07/2021.
//

#include "terrain.hh"

namespace neutrino::tiled::tmx {
    terrain terrain::parse(const xml_node& elt) {
        auto name = elt.get_attribute("name");
        auto tile = elt.get_attribute<unsigned>("tile");

        terrain result(name, tile);
        component::parse(result, elt);
        return result;
    }
}