//
// Created by igor on 9/9/24.
//

#include "json_archive_impl.hh"

#include "sdlpp/events/event_types.hh"

namespace neutrino::s11n::detail {
    json_serialization_archive_impl::~json_serialization_archive_impl() = default;

    void json_serialization_archive_impl::start_object() {
        if (m_stack.empty()) {
            if (!m_key.empty()) {
                m_root[m_key] = nlohmann::json::object();
                m_stack.push(&m_root.at(m_key));
                m_key.clear();
            }
        } else {
            auto current = m_stack.top();
            if (current->is_array()) {
                ENFORCE(m_key.empty());
                current->push_back(nlohmann::json::object());
                m_stack.push(&current->back());
            } else if (current->is_object()) {
                ENFORCE(!m_key.empty());
                current->operator[](m_key) = nlohmann::json::object();
                m_stack.push(&current->at(m_key));
                m_key.clear();
            } else {
                RAISE_EX("Should be array or object");
            }
        }
    }

    void json_serialization_archive_impl::end_object() {
        if(!m_stack.empty()) {
            ENFORCE(m_stack.top()->is_object());
            m_stack.pop();
        }
    }

    void json_serialization_archive_impl::start_array() {
        if (m_stack.empty()) {
            if (m_root.empty()) {
                m_root = nlohmann::json::array();
                m_stack.push(&m_root);
            } else {
                if (m_root.is_object()) {
                    ENFORCE(!m_key.empty());
                    m_root[m_key] = nlohmann::json::array();
                    m_stack.push(&m_root.at(m_key));
                    m_key.clear();
                } else {
                    ENFORCE(m_key.empty());
                    m_root.push_back(nlohmann::json::array());
                    m_stack.push(&m_root.back());
                }
            }
        } else {
            auto current = m_stack.top();
            if (current->is_object()) {
                ENFORCE(!m_key.empty());
                current->operator[](m_key) = nlohmann::json::array();
                m_stack.push(&current->at(m_key));
                m_key.clear();
            } else if (current->is_array()) {
                ENFORCE(m_key.empty());
                current->push_back(nlohmann::json::array());
                m_stack.push(&current->back());
            } else {
                RAISE_EX("Should be array or object");
            }
        }
    }

    void json_serialization_archive_impl::end_array() {
        if (!m_stack.empty()) {
            ENFORCE(m_stack.top()->is_array());
            m_stack.pop();
        }
    }

    void json_serialization_archive_impl::write_key(const char* name) {
        m_key = name;
    }

    void json_serialization_archive_impl::write_bool(bool v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_null() {
        primitive_write(nullptr);
    }

    void json_serialization_archive_impl::write_u8(uint8_t v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_i8(int8_t v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_u16(uint16_t v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_i16(int16_t v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_u32(uint32_t v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_i32(int32_t v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_u64(uint32_t v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_i64(int32_t v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_float(float v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_double(double v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::write_string(const char* v) {
        primitive_write(v);
    }

    void json_serialization_archive_impl::serialize(std::ostream& os) {
        os << m_root.dump(4);
    }

    json_deserialization_archive_impl::json_deserialization_archive_impl()
        : root (nullptr), is_owner(true) {
    }

    json_deserialization_archive_impl::~json_deserialization_archive_impl() {
        if (is_owner) {
            delete root;
        }
    }

    void json_deserialization_archive_impl::open(std::istream& is) {
        root = new nlohmann::json(nlohmann::json::parse(is));
    }

    bool json_deserialization_archive_impl::is_primitive() const {
        return root->is_primitive() && !root->is_null();
    }

    bool json_deserialization_archive_impl::is_null() const {
        return root->is_null();
    }

    bool json_deserialization_archive_impl::is_map() const {
        return root->is_object();
    }

    bool json_deserialization_archive_impl::is_array() const {
        return root->is_array();
    }

    bool json_deserialization_archive_impl::has_key(const char* key) const {
        return root->contains(key);
    }

    std::unique_ptr<deserialization_archive_impl> json_deserialization_archive_impl::get(const char* key) const {
        if (has_key(key)) {
            return std::unique_ptr<deserialization_archive_impl>(new json_deserialization_archive_impl{root->operator[](key)});
        }
        return nullptr;
    }

    void json_deserialization_archive_impl::read_bool(bool& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_u8(uint8_t& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_i8(int8_t& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_u16(uint16_t& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_i16(int16_t& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_u32(uint32_t& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_i32(int32_t& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_u64(uint32_t& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_i64(int32_t& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_float(float& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_double(double& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::read_string(std::string& v) const {
        read_primitive(v);
    }

    void json_deserialization_archive_impl::for_each_array(
        const std::function<void(const deserialization_archive_impl&)>& f) const {
        ENFORCE(is_array());
        for (const auto& v : *root) {
            json_deserialization_archive_impl ar{v};
            f(ar);
        }
    }

    void json_deserialization_archive_impl::for_each_map(
        const std::function<void(const std::string&, const deserialization_archive_impl&)>& f) const {
        ENFORCE(is_map());
        for (const auto& it : root->items()) {
            json_deserialization_archive_impl ar{it.value()};
            f(it.key(), ar);
        }
    }

    json_deserialization_archive_impl::json_deserialization_archive_impl(const nlohmann::json& node)
        : root (&node), is_owner(false) {
    }
}
