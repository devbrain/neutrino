//
// Created by igor on 06/08/2021.
//

#ifndef NEUTRINO_JSON_READER_HH
#define NEUTRINO_JSON_READER_HH

#include <nlohmann/json.hpp>
#include <neutrino/utils/exception.hh>
#include "reader.hh"

namespace neutrino::tiled::tmx
{
    class json_reader : public reader
    {
    public:
        explicit json_reader(const nlohmann::json& node)
        : m_node(node)
        {}
    private:
        void parse_each_element(visitor_t func) const override;
        void parse_many_elements(const char* name, visitor_t func) const override;
        void parse_one_element(const char* name, visitor_t func) const override;
        std::string get_text() const override;
        bool has_child(const char* name) const override;
        std::optional<std::string> get_attribute_value(const char* name) const override;
    private:
        const nlohmann::json& m_node;
    };
}

#endif //NEUTRINO_JSON_READER_HH
