//
// Created by igor on 02/08/2021.
//

#ifndef NEUTRINO_GROUP_HH
#define NEUTRINO_GROUP_HH

#include <optional>
#include <tuple>
#include "xml.hh"
#include "component.hh"
#include "color.hh"

namespace neutrino::tiled::tmx {

    struct group : public component
    {
        static constexpr int default_offset_x = 0;
        static constexpr int default_offset_y = 0;
        static constexpr float default_opacity = 1.0f;
        static constexpr bool default_visible = true;

        std::optional<int> offsetx;
        std::optional<int> offsety;
        std::optional<float> opacity;
        std::optional<bool> visible;
        std::optional<colori> tint;

        static group parse(const xml_node& elt, const group* parent = nullptr);
        // name, offsetx, offsety, opacity, visible, tint
        static std::tuple<std::string, int, int, float, bool, colori> parse_content(const xml_node& elt, const group* self);
    };
}

#endif //NEUTRINO_GROUP_HH
