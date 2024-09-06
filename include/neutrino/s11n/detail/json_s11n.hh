//
// Created by igor on 9/6/24.
//

#ifndef NEUTRINO_S11N_JSON_S11N_HH
#define NEUTRINO_S11N_JSON_S11N_HH

#include <algorithm>
#include <neutrino/s11n/detail/traits_s11n.hh>
#include <neutrino/s11n/detail/cista.hh>
#include <nlohmann/json.hpp>

namespace neutrino::detail {

    template <typename T>
    void serialize_to_json_obj(const T&obj, nlohmann::json& out);

    template <typename T>
    nlohmann::json dispatch_json_serialize(const T& obj) {
        nlohmann::json j;
        if constexpr (is_simple<T>::value) {
            j = obj;
        } else if constexpr (is_seq <T>::value) {
            if constexpr (is_seq<T>::simple_type) {
                j = obj;
            } else {
                j = nlohmann::json::array();
                for (const auto& e : obj) {
                    j.push_back(dispatch_json_serialize(e));
                }
            }
        } else if constexpr (is_map <T>::value) {
            j = nlohmann::json::object();
            for (const auto&[k, v] : obj) {
                j[k] = dispatch_json_serialize(v);
            }
        } else if constexpr (std::is_class_v<T>) {
            serialize_to_json_obj(obj, j);
        }
        return j;
    }

    template <typename T>
    void serialize_to_json_obj(const T&obj, nlohmann::json& out) {
        int idx = 0;
        cista::for_each_field(obj, [&out, &idx](const auto& field) {
            using field_t = std::remove_cv_t <decltype(field)>;
            static constexpr auto schema = T::serialization_schema();
            auto key = std::get <1>(schema[idx++]);
            if constexpr (is_simple <field_t>::value) {
                out[key] = field;
            } else if constexpr (is_seq<field_t>::value) {
                if constexpr (is_seq<field_t>::simple_type) {
                    out[key] = field;
                } else {
                    out[key] = dispatch_json_serialize(field);
                }
            } else {
                out[key] = dispatch_json_serialize(field);
            }
        });
    }

    template <typename T>
    void deserialize_from_json_obj(T&obj, const nlohmann::json& j);

    template <typename T>
    void dispatch_json_deserialize(T& obj, const nlohmann::json& j) {
        if constexpr (is_simple<T>::value) {
           obj = j.get<T>();
        } else if constexpr (is_seq <T>::value) {
            for (const auto& e : j) {
                typename is_seq<T>::inner x {};
                dispatch_json_deserialize(x, e);
                obj.emplace_back(x);
            }
        } else if constexpr (is_map <T>::value) {
            for (const auto&[k, v] : obj) {
                typename is_seq<T>::inner x {};
                dispatch_json_deserialize(x, v);
                obj.emplace(k, x);
            }
        } else if constexpr (std::is_class_v<T>) {
            deserialize_from_json_obj(obj, j);
        }
    }

    template <typename T>
    void deserialize_from_json_obj(T&obj, const nlohmann::json& j) {
        int idx = 0;
        cista::for_each_field(obj, [&j, &idx](auto& field) {
            using field_t = std::remove_cv_t <decltype(field)>;
            static constexpr auto schema = T::serialization_schema();
            auto key = std::get <1>(schema[idx++]);
            if constexpr (is_simple <field_t>::value) {
                field = j[key];
            } else if constexpr (is_seq<field_t>::value) {
                if constexpr (is_seq<field_t>::simple_type) {
                    field = j[key];
                } else {
                    dispatch_json_deserialize(field, j[key]);
                }
            } else {
                dispatch_json_deserialize(field, j[key]);
            }
        });
    }

}

#endif
