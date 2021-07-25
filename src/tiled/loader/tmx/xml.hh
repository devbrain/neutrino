//
// Created by igor on 21/07/2021.
//

#ifndef NEUTRINO_XML_HH
#define NEUTRINO_XML_HH

#include <iostream>
#include <pugixml.hpp>
#include <type_traits>
#include <map>
#include <optional>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx
{
    enum class Requirement
    {
        OPTIONAL,
        MANDATORY,
    };

    class xml_node
    {
    public:
        explicit xml_node(const pugi::xml_node& node)
                : m_node(node)
        {}

        bool is(const char* name) const;

        bool has_child(const char* name) const;

        template<typename Func>
        void parse_each_element(Func func) const
        {
            auto child = m_node.first_child();

            while (child)
            {
                func(xml_node(child));
                child = child.next_sibling();
            }
        }

        template<typename Func>
        void parse_many_elements(const char* name, Func func) const
        {
            auto child = m_node.child(name);
            while (child)
            {
                func(xml_node(child));
                child = child.next_sibling(name);
            }
        }

        template<typename Func>
        void parse_one_element(const char* name, Func func) const
        {
            auto child = m_node.child(name);

            if (!child)
            {
                return;
            }

            func(xml_node(child));
            child = child.next_sibling(name);

            if (child != nullptr)
            {
                std::clog << "Multiple chidren where a single child was expected for element: " << name << '\n';
            }
        }

        bool has_attribute(const char* name) const;

        std::string get_attribute(const char* name, Requirement req = Requirement::MANDATORY, const char* val = "") const {
            return get_string_attribute(name, req, val);
        }

        template<typename T>
        T get_attribute(const char* name, Requirement req = Requirement::MANDATORY, T val = {}) const
        {
            if constexpr (std::is_same_v<int, T>)
            {
                return get_int_attribute(name, req, val);
            } else
            {
                if constexpr (std::is_same_v<unsigned, T>)
                {
                    return get_uint_attribute(name, req, val);
                } else
                {
                    if constexpr(std::is_same_v<bool, T>)
                    {
                        return get_bool_attribute(name, req, val);
                    } else
                    {
                        if constexpr(std::is_same_v<double, T>)
                        {
                            return get_double_attribute(name, req, val);
                        } else
                        {
                            if constexpr(std::is_same_v<std::string, T> || std::is_convertible_v<T, std::string>)
                            {
                                return get_string_attribute(name, req, val);
                            } else
                            {
                                static_assert("Unsupported type");
                            }
                        }
                    }
                }
            }
        }

        unsigned get_uint_attribute(const char* name, Requirement req = Requirement::MANDATORY, unsigned val = 0) const;

        int get_int_attribute(const char* name, Requirement req = Requirement::MANDATORY, int val = 0) const;

        double get_double_attribute(const char* name, Requirement req = Requirement::MANDATORY, double val = 0.) const;

        bool get_bool_attribute(const char* name, Requirement req = Requirement::MANDATORY, bool val = false) const;

        std::string
        get_string_attribute(const char* name, Requirement req = Requirement::MANDATORY, const char* val = "") const;

        bool is_enum_attribute(const char* name, const char* value) const;

        template <typename T>
        T parse_enum(const char* name, T val, const std::map<std::string, T>& mp) const {
            if (!has_attribute(name)) {
                return val;
            }
            auto s = get_string_attribute(name);
            auto i = mp.find(s);
            if (i == mp.end()) {
                RAISE_EX("Atrribute ", name, " has unknown value ", s);
            }
            return i->second;
        }

        [[nodiscard]] std::string get_text() const;

    private:
        const pugi::xml_node& m_node;
    };
}
#endif
