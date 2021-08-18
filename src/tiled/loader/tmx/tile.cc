//
// Created by igor on 26/07/2021.
//

#include "tile.hh"
#include "json_reader.hh"
#include <neutrino/utils/strings/string_tokenizer.hh>
#include <neutrino/utils/strings/number_parser.hh>

namespace neutrino::tiled::tmx
{
    tile tile::parse(const reader& elt) {
        try {
            unsigned id = elt.get_uint_attribute("id");

            std::array<unsigned, 4> terrain = { {INVALID_TERRAIN, INVALID_TERRAIN, INVALID_TERRAIN, INVALID_TERRAIN } };
            if (const auto* json_rdr = dynamic_cast<const json_reader*>(&elt); json_rdr) {
                if (json_rdr->has_element("terrain")) {
                    int idx = 0;
                    json_rdr->iterate_data_array([&terrain, &idx](uint32_t v) {
                        terrain[idx++] = v;
                        }, "terrain");
                }
            } else {
                if (auto attr = elt.get_string_attribute("terrain", ""); !attr.empty())
                {
                    utils::string_tokenizer tokenizer(attr, ",", utils::string_tokenizer::TOK_TRIM);
                    unsigned t = 0;
                    for (const auto& tok : tokenizer)
                    {
                        terrain[t++] = utils::number_parser::parse(tok);
                    }
                }
            }
            unsigned probability = elt.get_uint_attribute("probability",  100);

            tile result(id, terrain, probability);

            component::parse(result, elt);

            elt.parse_one_element("image", [&result](const reader& e) {
                result.set_image(image::parse(e));
            });
            if (const auto* json_rdr = dynamic_cast<const json_reader*>(&elt); json_rdr) {
                if (json_rdr->has_element("animation")) {
                    result.m_animation = animation::parse(elt);
                }
            }
            else {
                elt.parse_one_element("animation", [&result](const reader& e) {
                    result.m_animation = animation::parse(e);
                    });
            }
            elt.parse_one_element("objectgroup", [&result](const reader& e) {
                result.m_objects = std::make_unique<object_layer>(object_layer::parse(e));
            });
            return result;
        } catch (exception& e) {
            auto id = elt.get_string_attribute("id", "<missing>");
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse tile, id [", id, "]");
        }
    }
}