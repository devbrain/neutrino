//
// Created by igor on 06/08/2021.
//

#include "json_reader.hh"

namespace neutrino::tiled::tmx
{
    void json_reader::parse_each_element(visitor_t func) const
    {
        for (const auto& obj : m_node) {
            func(json_reader(obj));
        }
    }
    // -------------------------------------------------------------------------------------------
    void json_reader::parse_many_elements(const char* name, visitor_t func) const
    {
        auto itr = m_node.find(name);
        if (itr != m_node.end()) {
            if (itr->is_object()) {
                const auto& child = *itr;
                for (const auto& obj : child) {
                    func(json_reader(obj));
                }
            }
        }
    }
    // -------------------------------------------------------------------------------------------
    void json_reader::parse_one_element(const char* name, visitor_t func) const
    {
        auto itr = m_node.find(name);
        if (itr != m_node.end()) {
            if (itr->is_object()) {
                const auto& child = *itr;
                func(json_reader(child));
            }
        }
    }
    // -------------------------------------------------------------------------------------------
    std::string json_reader::get_text() const
    {
        return "";
    }
    // -------------------------------------------------------------------------------------------
    bool json_reader::has_child(const char* name) const
    {
        auto itr = m_node.find(name);
        if (itr != m_node.end()) {
            return itr->is_object();
        }
        return false;
    }
    // -------------------------------------------------------------------------------------------
    std::optional<std::string> json_reader::get_attribute_value(const char* name) const
    {
        auto itr = m_node.find(name);
        if (itr != m_node.end()) {
            if (!itr->is_object() && !itr->is_array()) {
                return itr->get<std::string>();
            }
        }
        return std::nullopt;
    }
}
