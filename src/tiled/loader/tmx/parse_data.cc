//
// Created by igor on 26/07/2021.
//

#include <sstream>
#include "parse_data.hh"
#include "csv/parser.hpp"

#include <neutrino/utils/io/base64.hh>
#include <neutrino/utils/io/inflating_stream.hh>
#include <neutrino/utils/strings/string_utils.hh>
#include <neutrino/utils/strings/number_parser.hh>
#include <neutrino/utils/switch_by_string.hh>
#include <neutrino/utils/exception.hh>

static void decode_csv(const std::string& data, std::vector<int>& out)
{
    using namespace aria::csv;
    std::istringstream is(data);
    // use heap here, because CSV parser abuses stack
    std::unique_ptr<CsvParser> parser = std::make_unique<CsvParser>(is);

    for (;;)
    {
        auto field = parser->next_field();
        if (field.type == FieldType::DATA)
        {
            auto val = neutrino::utils::trim(*field.data);
            if (!val.empty())
            {
                int v = neutrino::utils::number_parser::parse(neutrino::utils::trim(*field.data));
                out.push_back(v);
            }
        } else
        {
            if (field.type == FieldType::CSV_END)
            {
                break;
            }
        }
    }
}

namespace neutrino::tiled::tmx
{
    parsed_buf_t parse_data(const std::string& encoding, const std::string& compression, const std::string& data)
    {
        if (encoding == "csv" || encoding.empty())
        {
            if (!compression.empty())
            {
                RAISE_EX("compression ", compression, " is specified for CSV data");
            }
            std::vector<int> res;
            decode_csv(data, res);
            return res;
        } else
        {
            if (encoding == "base64")
            {
                std::istringstream is(utils::trim(data));
                utils::io::base64_decoder decoder(is);
                std::string res;
                if (!compression.empty())
                {

                    auto type = utils::io::inflating_stream_buf::STREAM_ZLIB;
                    switch (switcher(compression.c_str()))
                    {
                        case "gzip"_case:
                            type = utils::io::inflating_stream_buf::STREAM_GZIP;
                            break;
                        case "zlib"_case:
                            type = utils::io::inflating_stream_buf::STREAM_ZLIB;
                            break;
                        default:
                            RAISE_EX("Unsupported compression type ", compression);
                    }
                    utils::io::inflating_input_stream inflating_input_stream(decoder, type);
                    inflating_input_stream >> res;
                } else
                {
                    decoder >> res;
                }
                return res;
            } else
            {
                RAISE_EX("Unknown encoding ", encoding);
            }
        }
    }
}