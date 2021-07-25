//
// Created by igor on 24/07/2021.
//

#include "test_utils.hh"
#include <sstream>
#include <neutrino/utils/io/memory_stream_buf.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx::test {
    map load_map(const unsigned char* data, std::size_t length) {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_buffer((char*)data, length);
        if (!result)
        {
            RAISE_EX("Failed to load tmx :", result.description());
        }
        auto root = doc.child("map");
        if (!root)
        {
            RAISE_EX ("entry node <map> is missing");
        }
        return map::parse(xml_node(root));
    }
}