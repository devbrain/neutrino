//
// Created by igor on 28/07/2021.
//

#include "animation.hh"
#include "json_reader.hh"
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx {
  animation animation::parse (const reader &elt) {
    try {
      animation obj;
      if (const auto *json_rdr = dynamic_cast<const json_reader *>(&elt); json_rdr) {
        json_rdr->parse_each_element_of ("animation", [&obj] (const reader &e) {
          auto id = e.get_int_attribute ("tileid");
          auto d = e.get_uint_attribute ("duration");
          obj.add ({id, std::chrono::milliseconds{d}});
        });
      }
      else {
        elt.parse_many_elements ("frame", [&obj] (const reader &e) {
          auto id = e.get_int_attribute ("tileid");
          auto d = e.get_uint_attribute ("duration");
          obj.add ({id, std::chrono::milliseconds{d}});
        });
      }

      return obj;
    }
    catch (exception &e) {
      RAISE_EX_WITH_CAUSE(std::move (e), "Failed to parse animation");
    }
  }
}
