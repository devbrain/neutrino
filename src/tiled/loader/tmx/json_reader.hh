//
// Created by igor on 06/08/2021.
//

#ifndef NEUTRINO_JSON_READER_HH
#define NEUTRINO_JSON_READER_HH

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "reader.hh"
#include <neutrino/utils/spimpl.h>
#include <neutrino/math/point.hh>

/*!
@brief namespace for Niels Lohmann
@see https://github.com/nlohmann
@since version 1.0.0
*/
namespace nlohmann
{
    /*!
    @brief default JSONSerializer template argument

    This serializer ignores the template arguments and uses ADL
    ([argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl))
    for serialization.
    */
    template<typename T = void, typename SFINAE = void>
    struct adl_serializer;

    template<template<typename U, typename V, typename... Args> class ObjectType =
    std::map,
            template<typename U, typename... Args> class ArrayType = std::vector,
            class StringType = std::string, class BooleanType = bool,
            class NumberIntegerType = std::int64_t,
            class NumberUnsignedType = std::uint64_t,
            class NumberFloatType = double,
            template<typename U> class AllocatorType = std::allocator,
            template<typename T, typename SFINAE = void> class JSONSerializer =
            adl_serializer,
            class BinaryType = std::vector<std::uint8_t>>
    class basic_json;

    /*!
    @brief JSON Pointer

    A JSON pointer defines a string syntax for identifying a specific value
    within a JSON document. It can be used with functions `at` and
    `operator[]`. Furthermore, JSON pointers are the base for JSON patches.

    @sa [RFC 6901](https://tools.ietf.org/html/rfc6901)

    @since version 2.0.0
    */
    template<typename BasicJsonType>
    class json_pointer;

    /*!
    @brief default JSON class

    This type is the default specialization of the @ref basic_json class which
    uses the standard template types.

    @since version 1.0.0
    */
    using json = basic_json<>;

    template<class Key, class T, class IgnoredLess, class Allocator>
    struct ordered_map;

    /*!
    @brief ordered JSON class

    This type preserves the insertion order of object keys.

    @since version 3.9.0
    */
    using ordered_json = basic_json<nlohmann::ordered_map>;

}  // namespace nlohmann


namespace neutrino::tiled::tmx
{
    class json_reader : public reader
    {
    public:
        static void assert_type(const char* type, const reader& elt);
        static json_reader load(const char* json, std::size_t size, const char* root_elt_name);
        explicit json_reader(const nlohmann::json& node);
        json_reader(json_reader&&) = default;
        ~json_reader() override;


        void iterate_data_array(std::function<void(uint32_t  v)> f, const char* name = "data") const;
        void parse_each_element_of(const char* name, visitor_t func) const;
        bool has_element(const char* name) const noexcept;
        std::vector<math::point2f> parse_points(const char* name) const;
    private:
        void parse_each_element(visitor_t func) const override;
        void parse_many_elements(const char* name, visitor_t func) const override;
        void parse_one_element(const char* name, visitor_t func) const override;
        bool has_child(const char* name) const override;
        std::optional<std::string> get_attribute_value(const char* name) const override;
    private:
        struct impl;
        spimpl::unique_impl_ptr<impl> m_pimpl;
    };
}

#endif //NEUTRINO_JSON_READER_HH
