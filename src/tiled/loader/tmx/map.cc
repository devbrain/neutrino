//
// Created by igor on 21/07/2021.
//

#include "map.hh"
#include "tile_layer.hh"
#include "json_reader.hh"
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/switch_by_string.hh>

namespace neutrino::tiled::tmx {
  map map::parse (const reader& elt, path_resolver_t resolver) {
    try {
      json_reader::assert_type ("map", elt);

      std::string version = elt.get_string_attribute ("version", "1.0");

      static const std::map<std::string, orientation_t> orientation_mp = {
          {"orthogonal", orientation_t::ORTHOGONAL},
          {"isometric", orientation_t::ISOMETRIC},
          {"staggered", orientation_t::STAGGERED},
          {"hexagonal", orientation_t::HEXAGONAL},
      };

      auto orientation = elt.parse_enum ("orientation", orientation_t::UNKNOWN, orientation_mp);
      auto width = elt.get_uint_attribute ("width");
      auto height = elt.get_uint_attribute ("height");
      auto tilewidth = elt.get_uint_attribute ("tilewidth");
      auto tileheight = elt.get_uint_attribute ("tileheight");
      auto bgcolor = elt.get_string_attribute ("backgroundcolor", "#000000");

      static const std::map<std::string, render_order_t> render_order_mp = {
          {"right-down", render_order_t::RIGHT_DOWN},
          {"right-up", render_order_t::RIGHT_UP},
          {"left-down", render_order_t::LEFT_DOWN},
          {"left-up", render_order_t::LEFT_UP},
      };
      auto render_order = elt.parse_enum ("renderorder", render_order_t::RIGHT_DOWN, render_order_mp);

      auto side_length = elt.get_uint_attribute ("hexsidelength", 0);

      static const std::map<std::string, stagger_axis_t> stagger_axis_mp = {
          {"x", stagger_axis_t::X},
          {"y", stagger_axis_t::Y}
      };
      auto axis = elt.parse_enum ("staggeraxis", stagger_axis_t::Y, stagger_axis_mp);
      static const std::map<std::string, stagger_index_t> stagger_index_mp = {
          {"odd", stagger_index_t::ODD},
          {"even", stagger_index_t::EVEN}
      };

      auto index = elt.parse_enum ("staggerindex", stagger_index_t::ODD, stagger_index_mp);

      auto infinite = elt.get_bool_attribute ("infinite", false);

      map result (version, orientation, width, height, tilewidth, tileheight, colori (bgcolor), render_order,
                  side_length, axis, index, infinite);

      const char* tileset_name = dynamic_cast<const json_reader*>(&elt) ? "tilesets" : "tileset";

      component::parse (result, elt);

      elt.parse_many_elements (tileset_name, [&result, &resolver] (const reader& e) {
        result.m_tilesets.push_back (tile_set::parse (e, resolver));
      });

      parse_group (elt, result, nullptr, resolver);

      return result;
    }
    catch (exception& e) {
      RAISE_EX_WITH_CAUSE(std::move (e), "Failed to parse map");
    }
  }

  // -------------------------------------------------------------------------------------------
  void map::parse_group (const reader& elt, map& result, const group* parent, path_resolver_t resolver) {
    if (dynamic_cast<const json_reader*>(&elt)) {
      elt.parse_many_elements ("layers", [&result, &resolver, parent] (const reader& e) {
        auto type = e.get_string_attribute ("type");
        switch (switcher (type.c_str ())) {
          case "tilelayer"_case:
            result.m_layers.emplace_back (tile_layer::parse (e, parent));
            break;
          case "objectgroup"_case:
            result.m_object_layers.emplace_back (object_layer::parse (e, parent));
            break;
          case "imagelayer"_case:
            result.m_layers.emplace_back (image_layer::parse (e, parent));
            break;
          case "group"_case: {
            auto current = group::parse (e, parent);
            parse_group (e, result, &current, resolver);
          }
            break;
          default:
            RAISE_EX("Unknown layer type ", type);
        }
      });
    }
    else {
      elt.parse_many_elements ("layer", [&result, parent] (const reader& e) {
        result.m_layers.emplace_back (tile_layer::parse (e, parent));
      });
      elt.parse_many_elements ("objectgroup", [&result, parent] (const reader& e) {
        result.m_object_layers.emplace_back (object_layer::parse (e, parent));
      });
      elt.parse_many_elements ("imagelayer", [&result, parent] (const reader& e) {
        result.m_layers.emplace_back (image_layer::parse (e, parent));
      });
      elt.parse_many_elements ("group", [&result, &resolver, parent] (const reader& e) {
        auto current = group::parse (e, parent);
        parse_group (e, result, &current, resolver);
      });
    }
  }
}
