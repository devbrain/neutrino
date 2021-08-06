//
// Created by igor on 27/07/2021.
//

#include "object.hh"
#include "cell.hh"
#include <neutrino/utils/strings/string_tokenizer.hh>
#include <neutrino/utils/strings/number_parser.hh>
#include <neutrino/utils/strings/string_utils.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx {
    namespace
    {
        std::pair<float, bool> cnv(const std::string& x) {
            auto v = utils::trim(x);
            double r = 0;
            bool status = utils::number_parser::try_parse_float(v, r);
            return {(float)r, status};
        }

        std::vector<math::point2f> parse_points(const std::string& points)
        {
            std::vector<math::point2f> res;
            utils::string_tokenizer grp(points, " ", utils::string_tokenizer::TOK_IGNORE_EMPTY);
            for (const auto& g : grp) {
                utils::string_tokenizer p(g, ",", utils::string_tokenizer::TOK_IGNORE_EMPTY);
                ENFORCE(p.count() == 2);
                auto [x, s1] = cnv(p[0]);
                auto [y, s2] = cnv(p[1]);
                if (s1 && s2) {
                    res.emplace_back(x, y);
                }
            }
            return res;
        }
    }

    object_t parse_object(const xml_node& elt)
    {

        try {
            auto id = elt.get_uint_attribute("id");
            auto name = elt.get_string_attribute("name", Requirement::OPTIONAL);
            auto type = elt.get_string_attribute("type", Requirement::OPTIONAL);
            auto x = elt.get_double_attribute("x", Requirement::OPTIONAL);
            auto y = elt.get_double_attribute("y", Requirement::OPTIONAL);
            auto width = elt.get_double_attribute("width", Requirement::OPTIONAL);
            auto height = elt.get_double_attribute("height", Requirement::OPTIONAL);
            auto gid = elt.get_int_attribute("gid", Requirement::OPTIONAL);
            auto rotation = elt.get_double_attribute("rotation", Requirement::OPTIONAL);
            bool visible = elt.get_bool_attribute("visible", Requirement::OPTIONAL, true);

            math::point2f origin{x, y};
            auto c = cell::decode_gid(gid);
            object_attribs atts(id, name, type, origin, width, height, rotation, visible, c.gid(),
                                c.hor_flipped(), c.vert_flipped(), c.diag_flipped());

            if (elt.has_child("polygon"))
            {
                polygon obj(atts);
                component::parse(obj, elt);


                elt.parse_one_element("polygon", [&obj](const xml_node& elt) {
                    std::string points = elt.get_string_attribute("points");
                    obj.points(parse_points(points));
                });

                return obj;
            }

            if (elt.has_child("polyline"))
            {
                polyline obj(atts);

                component::parse(obj, elt);
                elt.parse_one_element("polyline", [&obj](const xml_node& elt) {
                    std::string points = elt.get_string_attribute("points");
                    obj.points(parse_points(points));
                });
            }


            if (elt.has_child("ellipse"))
            {
                ellipse obj(atts);
                component::parse(obj, elt);

                return obj;
            }

            if (elt.has_child("point"))
            {
                point obj(atts);
                component::parse(obj, elt);

                return obj;
            }

            if (elt.has_child("text")) {
                text obj(atts);
                component::parse(obj, elt);
                elt.parse_one_element("text", [&obj](const xml_node& elt) {
                    obj.parse(elt);
                });
                return obj;
            }

            object obj(atts);
            component::parse(obj, elt);
            return obj;
        } catch (exception& e) {
            auto id = elt.get_string_attribute("id", Requirement::OPTIONAL, "<missing>");
            auto name = elt.get_string_attribute("name", Requirement::OPTIONAL, "<unknown>");
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse object [", name, "], id [", id, "]");
        }
    }

    void text::parse(const xml_node& elt)
    {
        try {
            m_font_family = elt.get_string_attribute("fontfamily", Requirement::OPTIONAL, "sans-serif");
            m_pixel_size = elt.get_int_attribute("pixelsize", Requirement::OPTIONAL, 16);
            m_wrap = elt.get_bool_attribute("wrap", Requirement::OPTIONAL);
            m_bold = elt.get_bool_attribute("bold", Requirement::OPTIONAL);
            m_italic = elt.get_bool_attribute("italic", Requirement::OPTIONAL);
            m_underline = elt.get_bool_attribute("underline", Requirement::OPTIONAL);
            m_strike = elt.get_bool_attribute("strikeout", Requirement::OPTIONAL);
            m_kerning = elt.get_bool_attribute("kerning", Requirement::OPTIONAL, true);

            m_color = colori(elt.get_string_attribute("color", Requirement::OPTIONAL, "#000000"));

            static const std::map<std::string, text::halign_t> hmp = {
                    {"left", text::halign_t::LEFT},
                    {"right", text::halign_t::RIGHT},
                    {"center", text::halign_t::CENTER},
                    {"justify", text::halign_t::JUSTIFY}
            };
            m_halign = elt.parse_enum("halign", text::halign_t::LEFT, hmp);
            static const std::map<std::string, text::valign_t> vmp = {
                    {"center", text::valign_t::CENTER},
                    {"top", text::valign_t::TOP},
                    {"bottom", text::valign_t::BOTTOM}
            };
            m_valign = elt.parse_enum("valign", text::valign_t::TOP, vmp);

            m_data = elt.get_text();
        } catch (exception& e) {
            auto id = elt.get_string_attribute("id", Requirement::OPTIONAL, "<missing>");
            auto name = elt.get_string_attribute("name", Requirement::OPTIONAL, "<unknown>");
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse text object [", name, "], id [", id, "]");
        }
    }
}
