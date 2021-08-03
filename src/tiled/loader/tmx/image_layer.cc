//
// Created by igor on 28/07/2021.
//

#include "image_layer.hh"

namespace neutrino::tiled::tmx {
    image_layer image_layer::parse(const xml_node& elt, const group* parent)
    {
        auto [name, offsetx, offsety, opacity, visible, tint] = group::parse_content(elt, parent);
        try {
            image_layer res(name, opacity, visible, offsetx, offsety, tint);
            component::parse(res, elt, parent);

            elt.parse_one_element("image", [&res](const xml_node elt) {
                res.set_image(image::parse(elt));
            });
            return res;
        } catch (exception& e) {
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse image layer [", name, "]");
        }

    }
}