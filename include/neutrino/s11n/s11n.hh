//
// Created by igor on 8/31/24.
//

#ifndef S11N_HH
#define S11N_HH

#include <tuple>
#include <array>
#include <string_view>
#include <bsw/macros.hh>
#include <neutrino/s11n/detail/archive.hh>

#define PRINT_SERIALIZATION_TUPLE_(i, x) std::make_tuple<std::size_t, std::string_view>(i , PPCAT(STRINGIZE(x), sv)),
#define SERIALIZATION_SCHEMA(...)                                                                                           \
    static constexpr std::array<std::tuple<std::size_t, std::string_view>, PP_NARG(__VA_ARGS__)> serialization_schema() {   \
        using namespace std::literals;                                                                                      \
        return {FOR_EACH_COUNT(PRINT_SERIALIZATION_TUPLE_, __VA_ARGS__)};	                                                \
    }

namespace neutrino::s11n {
    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    void serialize_to_yaml(const T& obj, std::ostream& os) {
        auto ar = detail::serialization_archive::create_yaml();
        detail::serialize_object(obj, ar);
        ar.serialize(os);
    }

    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    void deserialize_from_yaml(T& obj, std::istream& is) {
        auto ar = detail::deserialization_archive::create_yaml();
        ar.open(is);
        detail::deserialize_object(obj, ar);
    }

    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    T deserialize_from_yaml(std::istream& is) {
        T obj{};
        deserialize_from_yaml(obj, is);
        return obj;
    }

    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    void serialize_to_json(const T& obj, std::ostream& os) {
        auto ar = detail::serialization_archive::create_json();
        detail::serialize_object(obj, ar);
        ar.serialize(os);
    }

    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    void deserialize_from_json(T& obj, std::istream& is) {
        auto ar = detail::deserialization_archive::create_json();
        ar.open(is);
        detail::deserialize_object(obj, ar);
    }

    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    T deserialize_from_json(std::istream& is) {
        T obj{};
        deserialize_from_json(obj, is);
        return obj;
    }
}

#endif
