//
// Created by igor on 06/08/2021.
//

#include "reader.hh"
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/strings/number_parser.hh>

namespace neutrino::tiled::tmx
{
    reader::~reader() = default;


    unsigned reader::get_uint_attribute(const char *name) const {
        auto a = get_attribute_value(name);
        if (!a) {
            RAISE_EX("Can not find attribute ", name);
        }
        return utils::number_parser::parse_unsigned(*a);
    }

    unsigned reader::get_uint_attribute(const char *name, unsigned val) const {
        auto a = get_attribute_value(name);
        if (!a) {
            return val;
        }
        return utils::number_parser::parse_unsigned(*a);
    }

    int reader::get_int_attribute(const char *name) const {
        auto a = get_attribute_value(name);
        if (!a) {
            RAISE_EX("Can not find attribute ", name);
        }
        return utils::number_parser::parse(*a);
    }

    int reader::get_int_attribute(const char *name, int val) const {
        auto a = get_attribute_value(name);
        if (!a) {
            return val;
        }
        return utils::number_parser::parse(*a);
    }

    double reader::get_double_attribute(const char *name) const {
        auto a = get_attribute_value(name);
        if (!a) {
            RAISE_EX("Can not find attribute ", name);
        }
        return utils::number_parser::parse_float(*a);
    }

    double reader::get_double_attribute(const char *name, double val) const {
        auto a = get_attribute_value(name);
        if (!a) {
            return val;
        }
        return utils::number_parser::parse_float(*a);
    }

    bool reader::get_bool_attribute(const char *name) const {
        auto a = get_attribute_value(name);
        if (!a) {
            RAISE_EX("Can not find attribute ", name);
        }
        return utils::number_parser::parse_bool(*a);
    }

    bool reader::get_bool_attribute(const char *name, bool val) const {
        auto a = get_attribute_value(name);
        if (!a) {
            return val;
        }
        return utils::number_parser::parse_bool(*a);
    }

    std::string reader::get_string_attribute(const char *name) const {
        auto a = get_attribute_value(name);
        if (!a) {
            RAISE_EX("Can not find attribute ", name);
        }
        return *a;
    }

    std::string reader::get_string_attribute(const char *name, const char *val) const {
        auto a = get_attribute_value(name);
        if (!a) {
            return val;
        }
        return *a;
    }

    bool reader::has_attribute(const char* name) const {
        auto a = get_attribute_value(name);
        return a.has_value();
    }
}