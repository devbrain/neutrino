//
// Created by igor on 27/07/2021.
//

#include "object_layer.hh"

namespace neutrino::tiled::tmx {
    object_layer object_layer::parse(const xml_node& elt, const group* parent) {
        auto [name, offsetx, offsety, opacity, visible, tint] = group::parse_content(elt, parent);


        std::string color = elt.get_string_attribute("color", Requirement::OPTIONAL, "#a0a0a4");

        static const std::map<std::string, draw_order_t> mp = {
                {"topdown", draw_order_t::TOP_DOWN},
                {"index", draw_order_t::INDEX}
        };

        auto order = elt.parse_enum("draworder", draw_order_t::TOP_DOWN, mp);

        object_layer obj(name, opacity, visible, colori(color), order, offsetx, offsety, tint);
        component::parse(obj, elt, parent);

        elt.parse_many_elements("object", [&obj](const xml_node elt) {
            obj.add(parse_object(elt));
        });
        return obj;
    }
}