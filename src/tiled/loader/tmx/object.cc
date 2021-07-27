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
        std::pair<int, bool> cnv(const std::string& x) {
            auto v = utils::trim(x);
            int r = 0;
            bool status = utils::number_parser::try_parse(v, r);
            return {r, status};
        }

        std::vector<math::point2d> parse_points(const std::string& points)
        {
            std::vector<math::point2d> res;
            utils::string_tokenizer grp(points, " ", utils::string_tokenizer::TOK_IGNORE_EMPTY);
            for (const auto g : grp) {
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
        auto id = elt.get_uint_attribute("id", Requirement::OPTIONAL);
        auto name = elt.get_string_attribute("name", Requirement::OPTIONAL);
        auto type = elt.get_string_attribute("type", Requirement::OPTIONAL);
        auto x = elt.get_uint_attribute("x");
        auto y = elt.get_uint_attribute("y");
        double rotation = elt.get_double_attribute("rotation", Requirement::OPTIONAL);
        bool visible = elt.get_bool_attribute("visible", Requirement::OPTIONAL, true);

        math::point2d origin{x, y};

        if (elt.has_child("polygon"))
        {
            polygon obj(id, name, type, origin, rotation, visible);
            component::parse(obj, elt);


            elt.parse_one_element("polygon", [&obj](const xml_node& elt) {
                std::string points = elt.get_string_attribute("points");
                obj.points(parse_points(points));
            });

            return obj;
        }

        if (elt.has_child("polyline"))
        {
            polyline obj(id, name, type, origin, rotation, visible);

            component::parse(obj, elt);
            elt.parse_one_element("polyline", [&obj](const xml_node& elt) {
                std::string points = elt.get_string_attribute("points");
                obj.points(parse_points(points));
            });
        }

        if (elt.has_attribute("gid"))
        {
            unsigned gid = elt.get_uint_attribute("gid");
            auto c = cell::decode_gid(gid);

            tile_object obj (id, name, type, origin, rotation, visible, c.gid(),
                             c.hor_flipped(), c.vert_flipped(), c.diag_flipped());
            component::parse(obj, elt);
            return obj;
        }

        unsigned width = elt.get_uint_attribute("width", Requirement::OPTIONAL);
        unsigned height = elt.get_uint_attribute("height", Requirement::OPTIONAL);

        if (elt.has_child("ellipse"))
        {
            ellipse obj(id, name, type, origin, rotation, visible, width, height);
            component::parse(obj, elt);

            return obj;
        }

        rectangle obj  (id, name, type, origin, rotation, visible, width, height);

        component::parse(obj, elt);
        return obj;
    }
}