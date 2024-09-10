//
// Created by igor on 9/9/24.
//

#ifndef NEUTRINO_S11N_ARCHIVE_HH
#define NEUTRINO_S11N_ARCHIVE_HH

#include <cstdint>
#include <string>
#include <memory>
#include <ostream>
#include <functional>
#include <optional>
#include <neutrino/neutrino_export.hh>
#include <neutrino/s11n/detail/traits_s11n.hh>
#include <neutrino/s11n/detail/s11n_convert.hh>
#include <neutrino/s11n/detail/cista.hh>
#include <bsw/exception.hh>

#include "sdlpp/events/event_types.hh"

namespace neutrino::s11n::detail {
    class serialization_archive_impl {
        public:
            virtual ~serialization_archive_impl();

            virtual void start_object() = 0;
            virtual void end_object() = 0;

            virtual void start_array() = 0;
            virtual void end_array() = 0;

            virtual void write_key(const char* name) = 0;
            virtual void write_bool(bool v) = 0;
            virtual void write_null() = 0;
            virtual void write_u8(uint8_t v) = 0;
            virtual void write_i8(int8_t v) = 0;
            virtual void write_u16(uint16_t v) = 0;
            virtual void write_i16(int16_t v) = 0;
            virtual void write_u32(uint32_t v) = 0;
            virtual void write_i32(int32_t v) = 0;
            virtual void write_u64(uint32_t v) = 0;
            virtual void write_i64(int32_t v) = 0;
            virtual void write_float(float v) = 0;
            virtual void write_double(double v) = 0;
            virtual void write_string(const char* v) = 0;

            virtual void serialize(std::ostream& os) = 0;
    };

    class NEUTRINO_EXPORT serialization_archive {
        public:
            static serialization_archive create_yaml();
            static serialization_archive create_json();

            explicit serialization_archive(std::unique_ptr <serialization_archive_impl> impl);
            serialization_archive(const serialization_archive&) = delete;
            serialization_archive& operator =(const serialization_archive&) = delete;

            ~serialization_archive();

            void start_object() const;
            void end_object() const;

            void start_array() const;
            void end_array() const;

            void write_key(const char* name) const;
            void write_null() const;

            void serialize(std::ostream& os) const;

            template<typename T>
            void write_value(const T& v) const {
                using value_type = std::decay_t <T>;
                static_assert(is_simple <value_type>::value || is_complete_v <value_type>);

                if constexpr (is_complete_v <s11n_converter <value_type>>) {
                    m_impl->write_string(s11n_converter <value_type>::to_string(v).c_str());
                } else {
                    if constexpr (std::is_same_v <value_type, bool>) {
                        m_impl->write_bool(v);
                    } else if constexpr (std::is_same_v <value_type, uint8_t>) {
                        m_impl->write_u8(v);
                    } else if constexpr (std::is_same_v <value_type, int8_t>) {
                        m_impl->write_i8(v);
                    } else if constexpr (std::is_same_v <value_type, uint16_t>) {
                        m_impl->write_u16(v);
                    } else if constexpr (std::is_same_v <value_type, int16_t>) {
                        m_impl->write_i16(v);
                    } else if constexpr (std::is_same_v <value_type, uint32_t>) {
                        m_impl->write_u32(v);
                    } else if constexpr (std::is_same_v <value_type, int32_t>) {
                        m_impl->write_i32(v);
                    } else if constexpr (std::is_same_v <value_type, uint64_t>) {
                        m_impl->write_u64(v);
                    } else if constexpr (std::is_same_v <value_type, int64_t>) {
                        m_impl->write_i64(v);
                    } else if constexpr (std::is_same_v <value_type, float>) {
                        m_impl->write_float(v);
                    } else if constexpr (std::is_same_v <value_type, double>) {
                        m_impl->write_double(v);
                    } else if constexpr (std::is_same_v <value_type, std::string>) {
                        m_impl->write_string(v.c_str());
                    } else {
                        static_assert(false, "unsupported type");
                    }
                }
            }

        private:
            std::unique_ptr <serialization_archive_impl> m_impl;
    };

    class deserialization_archive_impl {
        public:
            deserialization_archive_impl();
            virtual ~deserialization_archive_impl();

            virtual void open(std::istream& is) = 0;

            [[nodiscard]] virtual bool is_primitive() const = 0;
            [[nodiscard]] virtual bool is_null() const = 0;
            [[nodiscard]] virtual bool is_map() const = 0;
            [[nodiscard]] virtual bool is_array() const = 0;
            [[nodiscard]] virtual bool has_key(const char* key) const = 0;
            [[nodiscard]] virtual std::unique_ptr <deserialization_archive_impl> get(const char* key) const = 0;

            virtual void read_bool(bool& v) const = 0;
            virtual void read_u8(uint8_t& v) const = 0;
            virtual void read_i8(int8_t& v) const = 0;
            virtual void read_u16(uint16_t& v) const = 0;
            virtual void read_i16(int16_t& v) const = 0;
            virtual void read_u32(uint32_t& v) const = 0;
            virtual void read_i32(int32_t& v) const = 0;
            virtual void read_u64(uint32_t& v) const = 0;
            virtual void read_i64(int32_t& v) const = 0;
            virtual void read_float(float& v) const = 0;
            virtual void read_double(double& v) const = 0;
            virtual void read_string(std::string& v) const = 0;

            virtual void for_each_array(const std::function <void (const deserialization_archive_impl&)>& f) const = 0;
            virtual void for_each_map(
                const std::function <void (const std::string&, const deserialization_archive_impl&)>& f) const = 0;
    };

    class NEUTRINO_EXPORT deserialization_archive {
        public:
            static deserialization_archive create_yaml();
            static deserialization_archive create_json();
            explicit deserialization_archive(std::unique_ptr <deserialization_archive_impl>&& impl);

            deserialization_archive(const deserialization_archive&) = delete;
            deserialization_archive& operator =(const deserialization_archive&) = delete;
            ~deserialization_archive();

            void open(std::istream& is) const;
            [[nodiscard]] bool is_empty() const;
            [[nodiscard]] bool is_primitive() const;
            [[nodiscard]] bool is_null() const;
            [[nodiscard]] bool is_map() const;
            [[nodiscard]] bool is_array() const;
            [[nodiscard]] bool has_key(const char* key) const;
            [[nodiscard]] std::optional <deserialization_archive> get(const char* key) const;

            template<typename T>
            void read(T& v) const {
                using value_type = std::decay_t <T>;
                static_assert(is_simple <value_type>::value || is_complete_v <value_type>);

                if constexpr (is_complete_v <s11n_converter <value_type>>) {
                    std::string encoded;
                    impl()->read_string(encoded);
                    v = s11n_converter <value_type>::from_string(v);
                } else {
                    if constexpr (std::is_same_v <value_type, bool>) {
                        impl()->read_bool(v);
                    } else if constexpr (std::is_same_v <value_type, uint8_t>) {
                        impl()->read_u8(v);
                    } else if constexpr (std::is_same_v <value_type, int8_t>) {
                        impl()->read_i8(v);
                    } else if constexpr (std::is_same_v <value_type, uint16_t>) {
                        impl()->read_u16(v);
                    } else if constexpr (std::is_same_v <value_type, int16_t>) {
                        impl()->read_i16(v);
                    } else if constexpr (std::is_same_v <value_type, uint32_t>) {
                        impl()->read_u32(v);
                    } else if constexpr (std::is_same_v <value_type, int32_t>) {
                        impl()->read_i32(v);
                    } else if constexpr (std::is_same_v <value_type, uint64_t>) {
                        impl()->read_u64(v);
                    } else if constexpr (std::is_same_v <value_type, int64_t>) {
                        impl()->read_i64(v);
                    } else if constexpr (std::is_same_v <value_type, float>) {
                        impl()->read_float(v);
                    } else if constexpr (std::is_same_v <value_type, double>) {
                        impl()->read_double(v);
                    } else if constexpr (std::is_same_v <value_type, std::string>) {
                        impl()->read_string(v);
                    } else {
                        static_assert(false, "unsupported type");
                    }
                }
            }

            void for_each(const std::function <void (const deserialization_archive&)>& f) const;
            void for_each(const std::function <void (const std::string&, const deserialization_archive&)>& f) const;

        private:
            explicit deserialization_archive(const deserialization_archive_impl& impl);
            [[nodiscard]] const deserialization_archive_impl* impl() const;
            std::unique_ptr <deserialization_archive_impl> m_pimpl;
            const deserialization_archive_impl* m_temp;
    };

    template<class T>
    void serialize_object(const T& obj, const serialization_archive& ar);

    template<class T>
    void serialize_generic(const T& obj, const serialization_archive& ar) {
        using field_t = std::decay_t <T>;
        if constexpr (is_optional <field_t>::value) {
            if (obj) {
                serialize_generic(obj.value(), ar);
            }
        } else if constexpr (is_simple <field_t>::value) {
            ar.write_value(obj);
        } else if constexpr (is_seq <field_t>::value) {
            ar.start_array();
            for (const auto& v : obj) {
                serialize_generic(v, ar);
            }
            ar.end_array();
        } else if constexpr (is_map <field_t>::value) {
            ar.start_object();
            for (const auto& [k, v] : obj) {
                ar.write_key(k);
                serialize_generic(v, ar);
            }
            ar.end_object();
        } else if constexpr (std::is_class_v <field_t>) {
            serialize_object(obj, ar);
        } else {
            static_assert(false, "unknown type");
        }
    }

    template<class T>
    void serialize_object(const T& obj, const serialization_archive& ar) {
        ar.start_object();
        int idx = 0;
        cista::for_each_field(obj, [&ar, &idx](auto& field) {
            static constexpr auto schema = T::serialization_schema();
            auto key = std::get <1>(schema[idx++]);
            ar.write_key(key.data());
            serialize_generic(field, ar);
        });
        ar.end_object();
    }

    template<class T>
    void deserialize_object(T& obj, const deserialization_archive& ar);
    template<class T>
    void deserialize_generic(T& obj, const deserialization_archive& ar);

    template<class T>
    struct array_wrapper;

    template<class T>
    struct array_wrapper <std::vector <T>> {
        explicit array_wrapper(std::vector <T>& obj)
            : m_arr(obj) {
        }

        void append(const deserialization_archive& ar) {
            T v;
            deserialize_generic(v, ar);
            m_arr.push_back(v);
        }

        std::vector <T>& m_arr;
    };

    template<class T, std::size_t N>
    struct array_wrapper <std::array <T, N>> {
        explicit array_wrapper(std::vector <T>& obj)
            : m_arr(obj), idx(0) {
        }

        void append(const deserialization_archive& ar) {
            ENFORCE(idx < N);
            deserialize_generic(m_arr[idx], ar);
            idx++;
        }

        std::array <T, N>& m_arr;
        std::size_t idx;
    };

    template<class T>
    void deserialize_generic(T& obj, const deserialization_archive& ar) {
        using field_t = std::decay_t <T>;
        if constexpr (is_optional <field_t>::value) {
            if (!ar.is_null()) {
                typename is_optional <field_t>::inner val;
                deserialize_generic(val, ar);
                obj = val;
            }
        } else if constexpr (is_simple <field_t>::value) {
            ENFORCE(ar.is_primitive());
            ar.read(obj);
        } else if constexpr (is_seq <field_t>::value) {
            ENFORCE(ar.is_array());
            array_wrapper <field_t> arr(obj);
            ar.for_each([&arr](const deserialization_archive& inner) {
                arr.append(inner);
            });
        } else if constexpr (is_map <field_t>::value) {
            ENFORCE(ar.is_map());
            ar.for_each([&obj](const std::string& key, const deserialization_archive& inner) {
                typename is_map <field_t>::inner val;
                deserialize_generic(val, inner);
                obj.emplace(key, val);
            });
        } else if constexpr (std::is_class_v <field_t>) {
            deserialize_object(obj, ar);
        } else {
            static_assert(false, "unsupported type");
        }
    }

    template<class T>
    void deserialize_object(T& obj, const deserialization_archive& ar) {
        ENFORCE(ar.is_map());
        int idx = 0;
        cista::for_each_field(obj, [&ar, &idx](auto& field) {
            static constexpr auto schema = T::serialization_schema();
            auto key = std::get <1>(schema[idx++]);

            auto inner = ar.get(key.data());
            if (inner) {
                deserialize_generic(field, *inner);
            }
        });
    }
}

#endif
