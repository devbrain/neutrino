//
// Created by igor on 28/07/2021.
//

#include "animation.hh"

namespace neutrino::tiled::tmx {
    animation animation::parse(const xml_node& elt) {
        animation obj;
        elt.parse_many_elements("frame", [&obj](const xml_node&e){
            auto id = e.get_int_attribute("tileid");
            auto d = e.get_uint_attribute("duration");
            obj.add({id, std::chrono::milliseconds{d}});
        });
        return obj;
    }
}
