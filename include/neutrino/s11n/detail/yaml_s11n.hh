//
// Created by igor on 9/6/24.
//

#ifndef NEUTRINO_S11N_DETAIL_YAML_S11N_HH
#define NEUTRINO_S11N_DETAIL_YAML_S11N_HH

#include <string_view>

#include <bsw/exception.hh>
#include <neutrino/s11n/detail/cista.hh>
#include <neutrino/s11n/detail/traits_s11n.hh>
#include <yaml-cpp/yaml.h>

namespace neutrino::detail {
    template<typename T>
    void serialize_to_yaml_dispatch(const T& obj, YAML::Emitter& yaml);

    template<typename T>
    void deserialize_from_yaml_dispatch(T& field, const YAML::Node& node);

    template<typename T>
    void deserialize_from_yaml_obj(T& obj, const YAML::Node& node);

    template<typename T>
    void serialize_to_yaml_seq(const T& obj, YAML::Emitter& yaml) {
        yaml << YAML::BeginSeq;
        for (const auto& item : obj) {
            serialize_to_yaml_dispatch(item, yaml);
        }
        yaml << YAML::EndSeq;
    }

    template<typename T>
    void serialize_to_yaml_map(const T& obj, YAML::Emitter& yaml) {
        yaml << YAML::BeginMap;
        for (const auto& [k,v] : obj) {
            yaml << YAML::Key << k;
            yaml << YAML::Value;
            serialize_to_yaml_dispatch(v, yaml);
        }
        yaml << YAML::EndMap;
    }

    template<typename T>
    void serialize_to_yaml_obj(const T& obj, YAML::Emitter& yaml) {
        yaml << YAML::BeginMap;
        int idx = 0;
        cista::for_each_field(obj, [&yaml, &idx](const auto& field) {
            static constexpr auto schema = T::serialization_schema();
            yaml << YAML::Key << std::get <1>(schema[idx++]).data();
            yaml << YAML::Value;
            serialize_to_yaml_dispatch(field, yaml);
        });

        yaml << YAML::EndMap;
    }

    template<typename T>
    void serialize_simple(T& obj, YAML::Emitter& yaml) {
        yaml << obj;
    }

    inline
    void serialize_simple(const std::string_view& obj, YAML::Emitter& yaml) {
        yaml << obj.data();
    }

    template<typename T>
    void serialize_to_yaml_dispatch(const T& obj, YAML::Emitter& yaml) {
        using field_t = std::remove_cv_t <T>;
        if constexpr (is_simple <T>::value) {
            serialize_simple(obj, yaml);
        } else if constexpr (is_seq <field_t>::value) {
            serialize_to_yaml_seq(obj, yaml);
        } else if constexpr (std::is_class_v <field_t>) {
            serialize_to_yaml_obj(obj, yaml);
        } else if constexpr (is_map <field_t>::value) {
            serialize_to_yaml_map(obj, yaml);
        }
    }

    // ------------------------------------------------------------------------
    template<typename T>
    void deserialize_from_yaml_seq(T& field, const YAML::Node& node) {
        using vt = typename T::value_type;
        for (const auto& item : node) {
            vt value;
            deserialize_from_yaml_dispatch(value, item);
            field.emplace_back(std::move(value));
        }
    }

    // ------------------------------------------------------------------------
    template<typename T>
    void deserialize_from_yaml_map(T& field, const YAML::Node& node) {
        using vt = typename T::value_type;
        for (const auto& item : node) {
            vt value;
            deserialize_from_yaml_dispatch(value, item.second);
            field.emplace(item.first, std::move(value));
        }
    }

    // ------------------------------------------------------------------------
    template<typename T>
    void deserialize_from_yaml_dispatch(T& field, const YAML::Node& node) {
        using field_t = std::remove_cv_t <T>;
        if constexpr (is_simple <T>::value) {
            ENFORCE(node.Type() == YAML::NodeType::Scalar);
            field = node.as <field_t>();
        } else if constexpr (is_seq <field_t>::value) {
            ENFORCE(node.Type() == YAML::NodeType::Sequence);
            deserialize_from_yaml_seq(field, node);
        } else if constexpr (is_map <field_t>::value) {
            ENFORCE(node.Type() == YAML::NodeType::Map);
            deserialize_from_yaml_map(field, node);
        } else {
            ENFORCE(node.Type() == YAML::NodeType::Map);
            deserialize_from_yaml_obj(field, node);
        }
    }

    // ------------------------------------------------------------------------
    template<typename T>
    void deserialize_from_yaml_obj(T& obj, const YAML::Node& node) {
        int idx = 0;
        cista::for_each_field(obj, [&node, &idx](auto& field) {
            static constexpr auto schema = T::serialization_schema();
            std::string field_name(std::get <1>(schema[idx++]));
            deserialize_from_yaml_dispatch(field, node[field_name]);
        });
    }
}

#endif
