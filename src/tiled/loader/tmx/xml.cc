//
// Created by igor on 21/07/2021.
//

#include <cstring>
#include <neutrino/utils/exception.hh>
#include "xml.hh"


namespace neutrino::tiled::tmx {
    bool xml_node::is(const char *name) const {
        return !std::strcmp(m_node.name(), name);
    }

    bool xml_node::has_child(const char *name) const {
        return m_node.child(name);
    }

    bool xml_node::has_attribute(const char *name) const {
        return m_node.attribute(name);
    }

    unsigned xml_node::get_uint_attribute(const char *name, Requirement req, unsigned val) const {
        auto a = m_node.attribute(name);
        if (!a) {
            if (req == Requirement::MANDATORY) {
                RAISE_EX("Can not find attribute ", name);
            }
        }
        return a.as_uint(val);
    }

    int xml_node::get_int_attribute(const char *name, Requirement req, int val) const {
        auto a = m_node.attribute(name);
        if (!a) {
            if (req == Requirement::MANDATORY) {
                RAISE_EX("Can not find attribute ", name);
            }
        }
        return a.as_int(val);
    }

    double xml_node::get_double_attribute(const char *name, Requirement req, double val) const {
        auto a = m_node.attribute(name);
        if (!a) {
            if (req == Requirement::MANDATORY) {
                RAISE_EX("Can not find attribute ", name);
            }
        }
        return a.as_double(val);
    }

    bool xml_node::get_bool_attribute(const char *name, Requirement req, bool val) const {
        auto a = m_node.attribute(name);
        if (!a) {
            if (req == Requirement::MANDATORY) {
                RAISE_EX("Can not find attribute ", name);
            }
        }
        return a.as_bool(val);
    }

    std::string xml_node::get_string_attribute(const char *name, Requirement req, const char *val) const {
        auto a = m_node.attribute(name);
        if (!a) {
            if (req == Requirement::MANDATORY) {
                RAISE_EX("Can not find attribute ", name);
            }
        }
        return a.as_string(val);
    }

    bool xml_node::is_enum_attribute(const char *name, const char *value) const {
        return get_string_attribute(name, Requirement::OPTIONAL, "") == value;
    }

    std::string xml_node::get_text() const {
        const char *text = m_node.text().get();
        return text ? text : "";
    }
}
