//
// Created by igor on 26/07/2021.
//

#include "tile.hh"
#include <neutrino/utils/strings/string_tokenizer.hh>
#include <neutrino/utils/strings/number_parser.hh>

namespace neutrino::tiled::tmx
{
    tile tile::parse(const xml_node& elt) {
        unsigned id = elt.get_uint_attribute("id");

        std::array<unsigned, 4> terrain = { { INVALID, INVALID, INVALID, INVALID } };

        if (auto attr = elt.get_string_attribute("terrain", Requirement::OPTIONAL); !attr.empty())
        {
            utils::string_tokenizer tokenizer(attr, ",", utils::string_tokenizer::TOK_TRIM);
            unsigned t = 0;
            for (const auto& tok : tokenizer)
            {
                terrain[t++] = utils::number_parser::parse(tok);
            }
        }

        unsigned probability = elt.get_uint_attribute("probability", Requirement::OPTIONAL, 100);

        tile result(id, terrain, probability);

        component::parse(result, elt);

        elt.parse_one_element("image", [&result](const xml_node& e) {
            result.set_image(image::parse(e));
        });
        elt.parse_one_element("animation", [&result](const xml_node& e) {
            result.m_animation = animation::parse(e);
        });
        elt.parse_one_element("objectgroup", [&result](const xml_node& e) {
            result.m_objects = std::make_unique<object_layer>(object_layer::parse(e));
        });
        return result;
    }
}