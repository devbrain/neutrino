//
// Created by igor on 21/07/2021.
//

#ifndef NEUTRINO_READER_HH
#define NEUTRINO_READER_HH

#include <string>
#include <map>
#include <optional>
#include <functional>

#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx
{
    class reader
    {
    public:
        using visitor_t = std::function<void(const reader&)>;
    public:
        virtual ~reader();

        virtual void parse_each_element(visitor_t func) const = 0;
        virtual void parse_many_elements(const char* name, visitor_t func) const = 0;
        virtual void parse_one_element(const char* name, visitor_t func) const = 0;

        [[nodiscard]] bool has_attribute(const char* name) const;
        [[nodiscard]] virtual std::string get_text() const = 0;
        [[nodiscard]] virtual bool has_child(const char *name) const = 0;

        [[nodiscard]] unsigned get_uint_attribute(const char* name) const;
        [[nodiscard]] unsigned get_uint_attribute(const char* name, unsigned val) const;

        [[nodiscard]] int get_int_attribute(const char* name) const;
        [[nodiscard]] int get_int_attribute(const char* name, int val) const;

        [[nodiscard]] double get_double_attribute(const char* name) const;
        [[nodiscard]] double get_double_attribute(const char* name, double val) const;

        [[nodiscard]] bool get_bool_attribute(const char* name) const;
        [[nodiscard]] bool get_bool_attribute(const char* name, bool val) const;

        [[nodiscard]] std::string get_string_attribute(const char* name) const;
        [[nodiscard]] std::string get_string_attribute(const char* name, const char* val) const;

        template<typename T>
        T parse_enum(const char* name, T val, const std::map<std::string, T>& mp) const
        {
            if (!has_attribute(name))
            {
                return val;
            }
            auto s = get_string_attribute(name);
            auto i = mp.find(s);
            if (i == mp.end())
            {
                RAISE_EX("Atrribute ", name, " has unknown value ", s);
            }
            return i->second;
        }

    protected:
        virtual std::optional<std::string> get_attribute_value(const char* name) const = 0;
    };
}

#endif
