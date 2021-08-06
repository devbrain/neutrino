//
// Created by igor on 26/07/2021.
//

#include "image.hh"
#include "parse_data.hh"

namespace neutrino::tiled::tmx {
    std::unique_ptr<image> image::parse(const reader& elt) {
        std::string format = elt.get_string_attribute("format", "");
        std::string source = elt.get_string_attribute("source");
        std::string trans = elt.get_string_attribute("trans", "");
        unsigned width = elt.get_uint_attribute("width", 0);
        unsigned height = elt.get_uint_attribute("height", 0);
        auto result = std::make_unique<image>(format, source, trans, width, height);

        elt.parse_one_element("data", [&result](const reader& elt) {
            auto encoding = elt.get_string_attribute("encoding", "");
            auto compression = elt.get_string_attribute("compression", "");
            result->m_data = std::get<data_buff_t>(parse_data(encoding, compression, elt.get_text()));
        });
        return result;
    }
}