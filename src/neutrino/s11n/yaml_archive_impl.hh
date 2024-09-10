//
// Created by igor on 9/9/24.
//

#ifndef NEUTRINO_S11N_YAML_ARCHIVE_IMPL_HH
#define NEUTRINO_S11N_YAML_ARCHIVE_IMPL_HH

#include <neutrino/s11n/detail/archive.hh>
#include <yaml-cpp/yaml.h>

namespace neutrino::s11n::detail {
    class yaml_serialization_archive_impl : public serialization_archive_impl {
        public:
            yaml_serialization_archive_impl();
            ~yaml_serialization_archive_impl() override;

            void start_object() override;
            void end_object() override;
            void start_array() override;
            void end_array() override;
            void write_key(const char* name) override;
            void write_bool(bool v) override;
            void write_null() override;
            void write_u8(uint8_t v) override;
            void write_i8(int8_t v) override;
            void write_u16(uint16_t v) override;
            void write_i16(int16_t v) override;
            void write_u32(uint32_t v) override;
            void write_i32(int32_t v) override;
            void write_u64(uint32_t v) override;
            void write_i64(int32_t v) override;
            void write_float(float v) override;
            void write_double(double v) override;
            void write_string(const char* v) override;

            void serialize(std::ostream& os) override;
        private:
            YAML::Emitter yaml;

            template<typename T>
            void primitive_write(const T& v) {
                yaml << YAML::Value;
                yaml << v;
            }
    };

    class yaml_deserialization_archive_impl : public deserialization_archive_impl {
        public:
            yaml_deserialization_archive_impl();
            ~yaml_deserialization_archive_impl() override;
            void open(std::istream& is) override;
            [[nodiscard]] bool is_primitive() const override;
            [[nodiscard]] bool is_null() const override;
            [[nodiscard]] bool is_map() const override;
            [[nodiscard]] bool is_array() const override;
            [[nodiscard]] bool has_key(const char* key) const override;
            [[nodiscard]] std::unique_ptr<deserialization_archive_impl> get(const char* key) const override;
            void read_bool(bool& v) const override;
            void read_u8(uint8_t& v) const override;
            void read_i8(int8_t& v) const override;
            void read_u16(uint16_t& v) const override;
            void read_i16(int16_t& v) const override;
            void read_u32(uint32_t& v) const override;
            void read_i32(int32_t& v) const override;
            void read_u64(uint32_t& v) const override;
            void read_i64(int32_t& v) const override;
            void read_float(float& v) const override;
            void read_double(double& v) const override;
            void read_string(std::string& v) const override;
            void for_each_array(const std::function<void(const deserialization_archive_impl&)>& f) const override;
            void for_each_map(
                const std::function<void(const std::string&, const deserialization_archive_impl&)>& f) const override;
        private:
            explicit yaml_deserialization_archive_impl(const YAML::Node& node);
            YAML::Node root;

            template <typename T>
            void read_primitive(T& v) const {
                v = root.as<T>();
            }

    };
}

#endif
