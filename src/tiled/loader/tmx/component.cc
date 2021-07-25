//
// Created by igor on 19/07/2021.
//

#include "component.hh"

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

    void component::parse(component& obj, const xml_node& elt)
    {
        elt.parse_one_element("properties", [&obj](const xml_node& e) {
            e.parse_many_elements("property", [&obj](const xml_node& inner) {
                std::string name = inner.get_string_attribute("name");
                ENFORCE(!name.empty());
                std::string value = inner.get_string_attribute("value");
                std::string type = inner.get_string_attribute("type", Requirement::OPTIONAL, "");
                if (type == "bool")
                {
                    obj.add(name, value == "true" || value == "yes" || value == "t" || value == "y");
                } else
                {
                    if (type == "int")
                    {
                        if (!value.empty())
                        {
                            obj.add(name, std::atoi(value.c_str()));
                        } else
                        {
                            obj.add(name, 0);
                        }
                    } else
                    {
                        if (type == "float")
                        {
                            float f = 0;
                            if (!value.empty())
                            {
                                f = std::atof(value.c_str());
                            }
                            obj.add(name, f);
                        }
                        else
                        {
                            if (type == "color") {
                                colori  c;
                                if (!value.empty())
                                {
                                    c = colori(value);
                                }
                                obj.add(name, c);
                            }
                            else
                            {
                                if (type == "file")
                                {
                                    std::filesystem::path p = value.empty() ? "." : value;
                                    obj.add(name, p);
                                }
                                else
                                {
                                    if (type == "object")
                                    {
                                        object_id o;
                                        if (!value.empty())
                                        {
                                            o.id = std::atoi(value.c_str());
                                        }
                                        obj.add(name, o);
                                    }
                                    else
                                    {
                                        obj.add(name, value);
                                    }
                                }
                            }
                        }
                    }
                }
            });
        });
    }
}