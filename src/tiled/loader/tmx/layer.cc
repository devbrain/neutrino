//
// Created by igor on 26/07/2021.
//

#include "layer.hh"

namespace neutrino::tiled::tmx {
  std::tuple<std::string, double, bool, int> layer::parse (const reader& elt) {
    auto name = elt.get_string_attribute ("name", "");
    auto opacity = elt.get_double_attribute ("opacity", 1.0);
    auto visible = elt.get_bool_attribute ("visible", true);
    auto id = elt.get_int_attribute ("id", -1);
    return {name, opacity, visible, id};
  }
}