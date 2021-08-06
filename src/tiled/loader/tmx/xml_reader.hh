//
// Created by igor on 06/08/2021.
//

#ifndef NEUTRINO_XML_READER_HH
#define NEUTRINO_XML_READER_HH

#include <pugixml.hpp>
#include <optional>
#include "reader.hh"


namespace neutrino::tiled::tmx
{
    class xml_reader : public reader
    {
    public:
        explicit xml_reader(const pugi::xml_node& node)
                : m_node(node)
        {}
    private:
        void parse_each_element(visitor_t func) const override;
        void parse_many_elements(const char* name, visitor_t func) const override;
        void parse_one_element(const char* name, visitor_t func) const override;

        std::string get_text() const override;
        bool has_child(const char *name) const override;
        std::optional<std::string> get_attribute_value(const char* name) const override;
    private:
        const pugi::xml_node& m_node;
    };
}

#endif //NEUTRINO_XML_READER_HH
