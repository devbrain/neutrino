//
// Created by igor on 19/08/2021.
//

#include "wang_set.hh"
#include "cell.hh"
#include "json_reader.hh"

#include <neutrino/utils/strings/string_tokenizer.hh>
#include <neutrino/utils/strings/number_parser.hh>

namespace neutrino::tiled::tmx {
  wang_color wang_color::parse (const reader &elt) {
    try {
      auto name = elt.get_string_attribute ("name");
      colori color (elt.get_string_attribute ("color"));
      auto tile = elt.get_int_attribute ("tile");
      auto prob = elt.get_double_attribute ("probability", 0.0);

      wang_color ret{color, std::move (name), tile, prob};
      component::parse (ret, elt);
      return ret;
    }
    catch (exception &e) {
      auto name = elt.get_string_attribute ("name", "<missing>");
      auto id = elt.get_string_attribute ("tile", "<missing>");

      RAISE_EX_WITH_CAUSE(std::move (e), "Failed to parse wangcolor name [", name, "], tile [", id, "]");
    }
  }

  wang_tile::wang_data_t wang_tile::parse_corners (const reader &elt) {

    wang_data_t res;
    std::size_t t = 0;
    try {
      if (const auto *json_rdr = dynamic_cast<const json_reader *>(&elt); json_rdr) {

        json_rdr->iterate_data_array ([&res, &t] (unsigned int v) {
          res.at (t++) = v;
        }, "wangid");
      }
      else {
        auto data = elt.get_string_attribute ("wangid");
        utils::string_tokenizer tokenizer (data, ",", utils::string_tokenizer::TOK_TRIM);
        for (const auto &tok : tokenizer) {
          res.at (t++) = utils::number_parser::parse (tok);
        }
      }
    }
    catch (std::exception &e) {
      RAISE_EX(e.what ());
    }
    if (t != res.size ()) {
      RAISE_EX("Not enough data for wangid. Expected number of elements", res.size (), " actual number of elements is", t);
    }
    return res;
  }

  wang_tile wang_tile::parse (const reader &elt) {
    try {
      auto tileid = elt.get_uint_attribute ("tileid");

      cell c{tileid};
      auto hflip = elt.get_bool_attribute ("hflip", c.hor_flipped ());
      auto dflip = elt.get_bool_attribute ("dflip", c.diag_flipped ());
      auto vflip = elt.get_bool_attribute ("vflip", c.vert_flipped ());

      return {parse_corners (elt), tileid, hflip, dflip, vflip};
    }
    catch (exception &e) {
      auto id = elt.get_string_attribute ("tileid", "<missing>");

      RAISE_EX_WITH_CAUSE(std::move (e), "wangtile id [", id, "]");
    }
  }

  wang_set wang_set::parse (const reader &elt) {
    try {
      auto name = elt.get_string_attribute ("name");
      auto id = elt.get_int_attribute ("tile");

      wang_set ws (name, id);
      component::parse (ws, elt);

      const char *tiles_name = "wangtile";
      const char *colors_name = "wangcolor";

      if (reader::is_json (elt)) {
        tiles_name = "wangtiles";
        colors_name = "colors";
      }

      elt.parse_many_elements (tiles_name, [&ws] (const reader &e) {
        ws.add_tile (wang_tile::parse (e));
      });

      elt.parse_many_elements (colors_name, [&ws] (const reader &e) {
        ws.add_color (wang_color::parse (e));
      });

      return ws;
    }
    catch (exception &e) {
      auto name = elt.get_string_attribute ("name", "<missing>");
      auto id = elt.get_string_attribute ("tile", "<missing>");

      RAISE_EX_WITH_CAUSE(std::move (e), "Failed to parse wangset name [", name, "], tile [", id, "]");
    }
  }
}