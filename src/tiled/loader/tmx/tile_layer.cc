//
// Created by igor on 26/07/2021.
//

#include "tile_layer.hh"
#include "parse_data.hh"
#include "xml_reader.hh"
#include "json_reader.hh"
#include <neutrino/utils/override.hh>
#include <neutrino/utils/byte_order.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx
{
    // ==============================================================================
    namespace
    {
        template<typename T>
        void parse_inner_data(T& result, const reader& e, const std::string& encoding, const std::string& compression)
        {

            std::string text;
            if (const auto* xml_rdr = dynamic_cast<const xml_reader*>(&e); xml_rdr) {
                text = xml_rdr->get_text();
            } else {
                text = e.get_string_attribute("data");
            }
            auto data = parse_data(encoding, compression, text);
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
    tile_layer tile_layer::parse(const reader& elt, const group* parent)
    {
        json_reader::assert_type("tilelayer", elt);
        auto [name, offsetx, offsety, opacity, visible, tint, id] = group::parse_content(elt, parent);

        try {
            auto parallax_x = elt.get_double_attribute("parallaxx", 1.0);
            auto parallax_y = elt.get_double_attribute("parallaxy", 1.0);
            auto width = elt.get_int_attribute("width");
            auto height = elt.get_int_attribute("height");

            tile_layer result(name, opacity, visible, id, offsetx, offsety, (float)parallax_x, (float)parallax_y, tint, width, height);

            component::parse(result, elt, parent);
            if (const auto* json_rdr = dynamic_cast<const json_reader*>(&elt); json_rdr) {
                auto encoding = json_rdr->get_string_attribute("encoding", "");
                auto compression = json_rdr->get_string_attribute("compression", "");
                if (encoding.empty() && compression.empty())
                {
                    json_rdr->iterate_data_array([&result](uint32_t gid){
                        result.add(cell::decode_gid(gid));
                    });

                } else {
                    if (elt.has_child("chunks")) {
                        elt.parse_many_elements("chunks", [&result, &encoding=std::as_const(encoding),
                                                           &compression = std::as_const(compression)](const reader& rdr) {
                            result.add(chunk::parse(rdr, encoding, compression));
                        });
                    } else {
                        parse_inner_data(result, elt, encoding, compression);
                    }
                }
            } else {
                elt.parse_one_element("data", [&result](const reader& e) {
                    auto encoding = e.get_string_attribute("encoding", "");
                    auto compression = e.get_string_attribute("compression", "");
                    if (encoding.empty() && compression.empty())
                    {
                        e.parse_many_elements("tile", [&result](const reader& telt) {
                            auto gid = telt.get_uint_attribute("gid");
                            result.add(cell::decode_gid(gid));
                        });
                    } else
                    {
                        if (e.has_child("chunk")) {
                            e.parse_many_elements("chunk", [&result, &encoding=std::as_const(encoding),
                                                            &compression = std::as_const(compression)](const reader& celt) {
                                result.add(chunk::parse(celt, encoding, compression));
                            });
                        } else {
                            parse_inner_data(result, e, encoding, compression);
                        }
                    }
                });
            }
            return result;
        } catch (exception& e) {
            auto id = elt.get_string_attribute("id", "<missing>");
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse layer [", name, "], id [", id, "]");
        }
    }
    // ===========================================================================================================
    chunk chunk::parse(const reader& elt, const std::string& encoding, const std::string& compression)
    {
        auto x = elt.get_int_attribute("x");
        auto y = elt.get_int_attribute("y");
        auto w = elt.get_int_attribute("width");
        auto h = elt.get_int_attribute("height");

        chunk result(x, y, w, h);

        if (encoding.empty() && compression.empty())
        {
            if (const auto* json_rdr = dynamic_cast<const json_reader*>(&elt); json_rdr) {
                json_rdr->iterate_data_array([&result](uint32_t gid){
                    result.add(cell::decode_gid(gid));
                });
            } else {
                elt.parse_many_elements("tile", [&result](const reader& telt) {
                    auto gid = telt.get_uint_attribute("gid");
                    result.add(cell::decode_gid(gid));
                });
            }
        } else
        {
            parse_inner_data(result, elt, encoding, compression);
        }
        return result;
    }
}
