//
// Created by igor on 02/08/2021.
//

#include "group.hh"
#include "layer.hh"
namespace neutrino::tiled::tmx
{
    group group::parse(const xml_node& elt, const group* parent)
    {
        group res;

        if (parent) {
            res.offsetx = parent->offsetx;
            res.offsety = parent->offsety;
            res.opacity = parent->opacity;
            res.visible = parent->visible;
            res.tint    = parent->tint;
        }
        auto name = elt.get_string_attribute("name", Requirement::OPTIONAL, "<unknown>");
        try {
            if (elt.has_attribute("offsetx"))
            {
                res.offsetx = elt.get_attribute<int>("offsetx");
            }
            if (elt.has_attribute("offsety"))
            {
                res.offsety = elt.get_attribute<int>("offsety");
            }
            if (elt.has_attribute("opacity"))
            {
                res.opacity = elt.get_attribute<double>("opacity");
            }
            if (elt.has_attribute("visible"))
            {
                res.visible = elt.get_attribute<bool>("visible");
            }
            if (elt.has_attribute("tintcolor"))
            {
                res.tint = colori(elt.get_string_attribute("tintcolor"));
            }
            component::parse(res, elt, parent);
        } catch (exception &e) {
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse group [", name, "]");
        }
        return res;
    }

    std::tuple<std::string, int, int, float, bool, colori> group::parse_content(const xml_node& elt, const group* self) {
        auto [name, opacity, visible] = layer::parse(elt);
        int offsetx = default_offset_x, offsety = default_offset_y;
        colori tint;

        if (elt.has_attribute("offsetx"))
        {
            offsetx = elt.get_attribute<int>("offsetx");
        } else {
            if (self && self->offsetx) {
                offsetx = *self->offsetx;
            }
        }

        if (elt.has_attribute("offsety"))
        {
            offsetx = elt.get_attribute<int>("offsety");
        } else {
            if (self && self->offsety) {
                offsetx = *self->offsety;
            }
        }

        if (elt.has_attribute("tintcolor"))
        {
            tint = colori(elt.get_string_attribute("tintcolor"));
        } else {
            if (self && self->tint) {
                tint = *self->tint;
            }
        }

        if (!elt.has_attribute("opacity") && self && self->opacity)
        {
            opacity = *self->opacity;
        }
        if (!elt.has_attribute("visible") && self && self->visible)
        {
            visible = *self->visible;
        }

        return {name, offsetx, offsety, opacity, visible, tint};
    }
}