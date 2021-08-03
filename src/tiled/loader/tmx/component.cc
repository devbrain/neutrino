//
// Created by igor on 19/07/2021.
//

#include "component.hh"
#include <neutrino/utils/strings/number_parser.hh>
#include <neutrino/utils/switch_by_string.hh>


namespace neutrino::tiled::tmx
{
    bool component::contains(const std::string& name) const noexcept
    {
        return m_prop.find(name) != m_prop.end();
    }

    bool component::empty() const noexcept
    {
        return m_prop.empty();
    }

    std::optional<property_t> component::get(const std::string& name) const noexcept
    {
        auto i = m_prop.find(name);
        if (i == m_prop.end())
        {
            return std::nullopt;
        }
        return i->second;
    }

    void component::parse(component& obj, const xml_node& elt, const component* parent)
    {
        if (parent)
        {
            for (const auto&[name, val] : parent->m_prop)
            {
                obj.add(name, val);
            }
        }
        elt.parse_one_element("properties", [&obj](const xml_node& e) {
            e.parse_many_elements("property", [&obj](const xml_node& inner) {
                std::string name = inner.get_string_attribute("name");
                ENFORCE(!name.empty());
                std::string value = inner.get_string_attribute("value", Requirement::OPTIONAL, "");
                if (value.empty())
                {
                    value = inner.get_text();
                }

                std::string type = inner.get_string_attribute("type", Requirement::OPTIONAL, "string");
                if (type != "string")
                {
                    if (value.empty())
                    {
                        RAISE_EX("Empty property value for non string type");
                    }
                }
                try
                {
                    switch (switcher(type.c_str()))
                    {
                        case "bool"_case:
                            obj.add(name, utils::number_parser::parse_bool(value));
                            break;
                        case "int"_case:
                            obj.add(name, utils::number_parser::parse64(value));
                            break;
                        case "float"_case:
                            obj.add(name, (float) utils::number_parser::parse_float(value));
                            break;
                        case "color"_case:
                            obj.add(name, colori(value));
                            break;
                        case "file"_case:
                            obj.add(name, std::filesystem::path(value.empty() ? "." : value));
                            break;
                        case "object"_case:
                            obj.add(name, object_id(utils::number_parser::parse(value)));
                            break;
                        default:
                            obj.add(name, value);
                            break;
                    }
                }
                catch (exception& e)
                {
                    RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse property [", name, "] of type [", type, "]");
                }
            });
        });

    }
}