//
// Created by igor on 26/07/2021.
//

#include <cstring>
#include "image.hh"
#include "parse_data.hh"

namespace neutrino::tiled::tmx {
    std::unique_ptr<image> image::parse(const xml_node& elt) {
        std::string format = elt.get_string_attribute("format", Requirement::OPTIONAL);
        std::string source = elt.get_string_attribute("source");
        std::string trans = elt.get_string_attribute("trans", Requirement::OPTIONAL);
        unsigned width = elt.get_uint_attribute("width", Requirement::OPTIONAL);
        unsigned height = elt.get_uint_attribute("height", Requirement::OPTIONAL);
        auto result = std::make_unique<image>(format, source, trans, width, height);

        elt.parse_one_element("data", [&result](const xml_node& elt) {
            auto encoding = elt.get_string_attribute("encoding", Requirement::OPTIONAL);
            auto compression = elt.get_string_attribute("compression", Requirement::OPTIONAL);
            result->m_data = std::get<data_buff_t>(parse_data(encoding, compression, elt.get_text()));
        });
        return result;
    }
}