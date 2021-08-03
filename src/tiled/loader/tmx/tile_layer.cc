//
// Created by igor on 26/07/2021.
//

#include "tile_layer.hh"
#include "parse_data.hh"
#include <neutrino/utils/override.hh>
#include <neutrino/utils/byte_order.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx
{
    // ==============================================================================
    namespace
    {
        template<typename T>
        void parse_inner_data(T& result, const xml_node& e, const std::string& encoding, const std::string& compression)
        {
            auto data = parse_data(encoding, compression, e.get_text());
            std::visit(
                    utils::overload(
                            [&result](const data_buff_t& buff) {
                                const auto sz = buff.size();
                                ENFORCE(sz % 4 == 0);
                                for (std::size_t i = 0; i < sz; i += 4)
                                {
                                    union
                                    {
                                        uint8_t* bytes;
                                        uint32_t* words;
                                    } u = {(uint8_t*) buff.data() + i};
                                    unsigned gid = neutrino::utils::byte_order::from_little_endian(*u.words);
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
    }
    // ==============================================================================

    tile_layer tile_layer::parse(const xml_node& elt, const group* parent)
    {
        auto [name, offsetx, offsety, opacity, visible, tint] = group::parse_content(elt, parent);
        try {
            auto parallax_x = elt.get_attribute<double>("parallaxx", Requirement::OPTIONAL, 1.0);
            auto parallax_y = elt.get_attribute<double>("parallaxy", Requirement::OPTIONAL, 1.0);

            tile_layer result(name, opacity, visible, offsetx, offsety, (float)parallax_x, (float)parallax_y, tint);

            component::parse(result, elt, parent);
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
                    if (e.has_child("chunk")) {
                        e.parse_many_elements("chunk", [&result, &encoding=std::as_const(encoding),
                                                        &compression = std::as_const(compression)](const xml_node& celt) {
                            result.add(chunk::parse(celt, encoding, compression));
                        });
                    } else {
                        parse_inner_data(result, e, encoding, compression);
                    }
                }
            });
            return result;
        } catch (exception& e) {
            auto id = elt.get_string_attribute("id", Requirement::OPTIONAL, "<missing>");
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse layer [", name, "], id [", id, "]");
        }
    }
    // ===========================================================================================================
    chunk chunk::parse(const xml_node& elt, const std::string& encoding, const std::string& compression)
    {
        auto x = elt.get_attribute<int>("x");
        auto y = elt.get_attribute<int>("y");
        auto w = elt.get_attribute<int>("width");
        auto h = elt.get_attribute<int>("height");

        chunk result(x, y, w, h);

        if (encoding.empty() && compression.empty())
        {
            elt.parse_many_elements("tile", [&result](const xml_node& telt) {
                auto gid = telt.get_uint_attribute("gid");
                result.add(cell::decode_gid(gid));
            });

        } else
        {
            parse_inner_data(result, elt, encoding, compression);
        }
        return result;
    }
}
