//
// Created by igor on 9/9/24.
//
#include "yaml_archive_impl.hh"
#include <bsw/exception.hh>


namespace neutrino::s11n::detail {
    yaml_serialization_archive_impl::yaml_serialization_archive_impl() = default;

    yaml_serialization_archive_impl::~yaml_serialization_archive_impl() = default;

    void yaml_serialization_archive_impl::start_object() {
        yaml << YAML::BeginMap;
    }

    void yaml_serialization_archive_impl::end_object() {
        yaml << YAML::EndMap;
    }

    void yaml_serialization_archive_impl::start_array() {
        yaml << YAML::BeginSeq;
    }

    void yaml_serialization_archive_impl::end_array() {
        yaml << YAML::EndSeq;
    }

    void yaml_serialization_archive_impl::write_key(const char* name) {
        yaml << YAML::Key << name;
    }

    void yaml_serialization_archive_impl::write_bool(bool v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_null() {
        yaml << YAML::Value << YAML::Null;
    }

    void yaml_serialization_archive_impl::write_u8(uint8_t v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_i8(int8_t v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_u16(uint16_t v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_i16(int16_t v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_u32(uint32_t v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_i32(int32_t v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_u64(uint32_t v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_i64(int32_t v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_float(float v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_double(double v) {
        primitive_write(v);
    }

    void yaml_serialization_archive_impl::write_string(const char* v) {
        yaml << YAML::Value << v;
    }

    void yaml_serialization_archive_impl::serialize(std::ostream& os) {
        os << yaml.c_str();
    }

    yaml_deserialization_archive_impl::yaml_deserialization_archive_impl() = default;


    yaml_deserialization_archive_impl::~yaml_deserialization_archive_impl() = default;

    void yaml_deserialization_archive_impl::open(std::istream& is) {
        root = YAML::Load(is);
    }

    bool yaml_deserialization_archive_impl::is_primitive() const {
        return root.Type() == YAML::NodeType::Scalar;
    }

    bool yaml_deserialization_archive_impl::is_null() const {
        return root.Type() == YAML::NodeType::Null;
    }

    bool yaml_deserialization_archive_impl::is_map() const {
        return root.Type() == YAML::NodeType::Map;
    }

    bool yaml_deserialization_archive_impl::is_array() const {
        return root.Type() == YAML::NodeType::Sequence;
    }

    bool yaml_deserialization_archive_impl::has_key(const char* key) const {
        if (root[key]) {
            return true;
        }
        return false;
    }

    std::unique_ptr<deserialization_archive_impl> yaml_deserialization_archive_impl::get(const char* key) const {
        if (has_key(key)) {
            return std::unique_ptr<yaml_deserialization_archive_impl>(new yaml_deserialization_archive_impl(root[key]));
        }
        return nullptr;
    }

    void yaml_deserialization_archive_impl::read_bool(bool& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_u8(uint8_t& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_i8(int8_t& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_u16(uint16_t& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_i16(int16_t& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_u32(uint32_t& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_i32(int32_t& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_u64(uint32_t& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_i64(int32_t& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_float(float& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_double(double& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::read_string(std::string& v) const {
        read_primitive(v);
    }

    void yaml_deserialization_archive_impl::for_each_array(
        const std::function<void(const deserialization_archive_impl&)>& f) const {
        ENFORCE(is_array());
        for (const auto& item : root) {
            yaml_deserialization_archive_impl ar{item};
            f(ar);
        }
    }

    void yaml_deserialization_archive_impl::for_each_map(
        const std::function<void(const std::string&, const deserialization_archive_impl&)>& f) const {
        ENFORCE(is_map());
        for (const auto& item : root) {
            yaml_deserialization_archive_impl ar{item.second};
            f(item.first.as<std::string>(), ar);
        }
    }

    yaml_deserialization_archive_impl::yaml_deserialization_archive_impl(const YAML::Node& node)
        : root(node) {
    }
}
