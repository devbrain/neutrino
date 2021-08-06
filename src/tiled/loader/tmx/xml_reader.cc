//
// Created by igor on 21/07/2021.
//

#include <iostream>
#include "xml_reader.hh"


namespace neutrino::tiled::tmx {


    bool xml_reader::has_child(const char *name) const {
        return m_node.child(name);
    }

    std::optional<std::string> xml_reader::get_attribute_value(const char* name) const
    {
        const auto a = m_node.attribute(name);
        if (a) {
            return a.as_string();
        }
        return std::nullopt;
    }

    std::string xml_reader::get_text() const {
        const char *text = m_node.text().get();
        return text ? text : "";
    }

    void xml_reader::parse_each_element(visitor_t func) const
    {
        auto child = m_node.first_child();

        while (child)
        {
            func(xml_reader(child));
            child = child.next_sibling();
        }
    }


    void xml_reader::parse_many_elements(const char* name, visitor_t func) const
    {
        auto child = m_node.child(name);
        while (child)
        {
            func(xml_reader(child));
            child = child.next_sibling(name);
        }
    }


    void xml_reader::parse_one_element(const char* name, visitor_t func) const
    {
        auto child = m_node.child(name);

        if (!child)
        {
            return;
        }

        func(xml_reader(child));
        child = child.next_sibling(name);

        if (child != nullptr)
        {
            std::clog << "Multiple chidren where a single child was expected for element: " << name << '\n';
        }
    }
}
