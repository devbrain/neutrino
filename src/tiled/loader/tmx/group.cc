//
// Created by igor on 02/08/2021.
//

#include "group.hh"
#include "layer.hh"

namespace neutrino::tiled::tmx {
  group group::parse (const reader &elt, const group *parent) {
    group res;

    if (parent) {
      res.offsetx = parent->offsetx;
      res.offsety = parent->offsety;
      res.opacity = parent->opacity;
      res.visible = parent->visible;
      res.tint = parent->tint;
    }
    auto name = elt.get_string_attribute ("name", "<unknown>");
    try {
      if (elt.has_attribute ("offsetx")) {
        res.offsetx = elt.get_int_attribute ("offsetx");
      }
      if (elt.has_attribute ("offsety")) {
        res.offsety = elt.get_int_attribute ("offsety");
      }
      if (elt.has_attribute ("opacity")) {
        res.opacity = (float) elt.get_double_attribute ("opacity");
      }
      if (elt.has_attribute ("visible")) {
        res.visible = elt.get_bool_attribute ("visible");
      }
      if (elt.has_attribute ("tintcolor")) {
        res.tint = colori (elt.get_string_attribute ("tintcolor"));
      }
      component::parse (res, elt, parent);
    }
    catch (exception &e) {
      RAISE_EX_WITH_CAUSE(std::move (e), "Failed to parse group [", name, "]");
    }
    return res;
  }

  std::tuple<std::string, int, int, float, bool, colori, int>
  group::parse_content (const reader &elt, const group *self) {
    auto[name, opacity, visible, id] = layer::parse (elt);

    int offsetx = default_offset_x, offsety = default_offset_y;
    colori tint;

    if (elt.has_attribute ("offsetx")) {
      offsetx = elt.get_int_attribute ("offsetx");
    }
    else {
      if (self && self->offsetx) {
        offsetx = *self->offsetx;
      }
    }

    if (elt.has_attribute ("offsety")) {
      offsety = elt.get_int_attribute ("offsety");
    }
    else {
      if (self && self->offsety) {
        offsety = *self->offsety;
      }
    }

    if (elt.has_attribute ("tintcolor")) {
      tint = colori (elt.get_string_attribute ("tintcolor"));
    }
    else {
      if (self && self->tint) {
        tint = *self->tint;
      }
    }

    if (!elt.has_attribute ("opacity") && self && self->opacity) {
      opacity = *self->opacity;
    }
    if (!elt.has_attribute ("visible") && self && self->visible) {
      visible = *self->visible;
    }

    return {name, offsetx, offsety, (float) opacity, visible, tint, id};
  }
}