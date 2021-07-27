//
// Created by igor on 26/07/2021.
//

#include "tile_layer.hh"
#include "parse_data.hh"
#include <neutrino/utils/override.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx
{
    tile_layer tile_layer::parse(const xml_node& elt)
    {
        auto[name, opacity, visible] = layer::parse(elt);
        tile_layer result(name, opacity, visible);

        component::parse(result, elt);
        elt.parse_one_element("data", [&result](const xml_node e) {
            auto encoding = e.get_string_attribute("encoding", Requirement::OPTIONAL);
            auto compression = e.get_string_attribute("compression", Requirement::OPTIONAL);
            if (encoding.empty() && compression.empty())
            {

                    e.parse_many_elements("tile", [&result](const xml_node& telt) {
                        auto gid = telt.get_uint_attribute("gid");
                        result.add(cell::decode_gid(gid));
                    });

            } else
            {
                auto data = parse_data(encoding, compression, e.get_text());
                std::visit(
                        utils::overload(
                                [&result](const data_buff_t& buff) {
                                    const auto sz = buff.size();
                                    ENFORCE(sz % 4 == 0);
                                    for (std::size_t i = 0; i < sz; i += 4)
                                    {
                                        unsigned gid = buff[i] | (buff[i + 1] << 8) | (buff[i + 2] << 16) |
                                                       (buff[i + 3] << 24);

                                        result.add(cell::decode_gid(gid));
                                    }
                                },
                                [&result](const int_buff_t& buff) {
                                    for (auto gid : buff)
                                    {
                                        result.add(cell::decode_gid(gid));
                                    }
                                }),
                        data
                );
            }
        });
        return result;
    }
}
