//
// Created by igor on 26/07/2021.
//

#ifndef NEUTRINO_PARSE_DATA_HH
#define NEUTRINO_PARSE_DATA_HH

#include <string>
#include <vector>
#include <variant>

namespace neutrino::tiled::tmx {
    using data_buff_t = std::vector<char>;
    using int_buff_t = std::vector<int>;
    using parsed_buf_t = std::variant<data_buff_t , int_buff_t >;
    parsed_buf_t parse_data(const std::string& encoding, const std::string& compression, const std::string& data);
}

#endif //NEUTRINO_PARSE_DATA_HH
