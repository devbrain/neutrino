//
// Created by igor on 8/31/24.
//

#ifndef S11N_HH
#define S11N_HH

#include <tuple>
#include <array>
#include <string_view>
#include <bsw/macros.hh>
#include <neutrino/s11n/detail/yaml_s11n.hh>
#include <neutrino/s11n/detail/json_s11n.hh>

#define PRINT_SERIALIZATION_TUPLE_(i, x) std::make_tuple<std::size_t, std::string_view>(i , PPCAT(STRINGIZE(x), sv)),
#define SERIALIZATION_SCHEMA(...)                                                                                           \
    static constexpr std::array<std::tuple<std::size_t, std::string_view>, PP_NARG(__VA_ARGS__)> serialization_schema() {   \
        using namespace std::literals;                                                                                      \
        return {FOR_EACH_COUNT(PRINT_SERIALIZATION_TUPLE_, __VA_ARGS__)};	                                                \
    }

namespace neutrino {
    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    void serialize_to_yaml(const T& obj, std::ostream& os) {
        YAML::Emitter out(os);
        detail::serialize_to_yaml_obj(obj, out);
    }

    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    void deserialize_from_yaml(T& obj, std::istream& is) {
        const YAML::Node root = YAML::Load(is);
        ENFORCE(root.Type() == YAML::NodeType::Map);
        detail::deserialize_from_yaml_obj(obj, root);
    }

    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    T deserialize_from_yaml(std::istream& is) {
        T obj{};
        detail::deserialize_from_yaml_obj(obj, is);
        return obj;
    }


    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    void serialize_to_json(const T& obj, std::ostream& os) {
        nlohmann::json j;
        detail::serialize_to_json_obj(obj, j);

        os << j.dump(4);
    }

    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
   void deserialize_from_json(T& obj, std::istream& is) {
        nlohmann::json j = nlohmann::json::parse(is);
        ENFORCE(j.is_object());
        detail::deserialize_from_json_obj(obj, j);
    }

    template<typename T, class = std::enable_if_t <std::is_class_v <T>>>
    T deserialize_from_json(std::istream& is) {
        T obj{};
        deserialize_from_json(obj, is);
        return obj;
    }
}

#endif
