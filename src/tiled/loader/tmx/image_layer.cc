//
// Created by igor on 28/07/2021.
//

#include "image_layer.hh"
#include "json_reader.hh"

namespace neutrino::tiled::tmx {
    image_layer image_layer::parse(const reader& elt, const group* parent)
    {
        auto [name, offsetx, offsety, opacity, visible, tint, id] = group::parse_content(elt, parent);
        try {
            image_layer res(name, opacity, visible, id, offsetx, offsety, tint);
            component::parse(res, elt, parent);
            if (const auto* json_rdr = dynamic_cast<const json_reader*>(&elt); json_rdr) {
                res.set_image(image::parse(*json_rdr));
            }
            else {
                elt.parse_one_element("image", [&res](const reader& elt) {
                    res.set_image(image::parse(elt));
                    });
            }
            return res;
        } catch (exception& e) {
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse image layer [", name, "]");
        }

    }
}