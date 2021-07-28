//
// Created by igor on 28/07/2021.
//

#include "image_layer.hh"

namespace neutrino::tiled::tmx {
    image_layer image_layer::parse(const xml_node& elt)
    {
        auto [name, opacity, visible] = layer::parse(elt);

        image_layer res(name, opacity, visible);
        component::parse(res, elt);


        elt.parse_one_element("image", [&res](const xml_node elt) {
            res.set_image(image::parse(elt));
        });

        return res;
    }
}