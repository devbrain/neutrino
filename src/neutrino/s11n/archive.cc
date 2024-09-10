//
// Created by igor on 9/9/24.
//

#include <neutrino/s11n/detail/archive.hh>
#include "yaml_archive_impl.hh"
#include "json_archive_impl.hh"
#include <bsw/exception.hh>

namespace neutrino::s11n::detail {
    serialization_archive_impl::~serialization_archive_impl() = default;

    serialization_archive serialization_archive::create_yaml() {
        return serialization_archive{std::make_unique <yaml_serialization_archive_impl>()};
    }

    serialization_archive serialization_archive::create_json() {
        return serialization_archive{std::make_unique <json_serialization_archive_impl>()};
    }

    serialization_archive::serialization_archive(std::unique_ptr <serialization_archive_impl> impl)
        : m_impl(std::move(impl)) {
    }

    serialization_archive::~serialization_archive() = default;

    void serialization_archive::start_object() const {
        m_impl->start_object();
    }

    void serialization_archive::end_object() const {
        m_impl->end_object();
    }

    void serialization_archive::start_array() const {
        m_impl->start_array();
    }

    void serialization_archive::end_array() const {
        m_impl->end_array();
    }

    void serialization_archive::write_key(const char* name) const {
        m_impl->write_key(name);
    }

    void serialization_archive::write_null() const {
        m_impl->write_null();
    }

    void serialization_archive::serialize(std::ostream& os) const {
        m_impl->serialize(os);
    }

    deserialization_archive_impl::deserialization_archive_impl() = default;

    deserialization_archive_impl::~deserialization_archive_impl() = default;

    deserialization_archive deserialization_archive::create_yaml() {
        return deserialization_archive{std::make_unique <yaml_deserialization_archive_impl>()};
    }

    deserialization_archive deserialization_archive::create_json() {
        return deserialization_archive{std::make_unique <json_deserialization_archive_impl>()};
    }

    deserialization_archive::deserialization_archive(std::unique_ptr <deserialization_archive_impl>&& impl)
        : m_pimpl(std::move(impl)), m_temp(nullptr) {
    }

    deserialization_archive::~deserialization_archive() = default;

    void deserialization_archive::open(std::istream& is) const {
        ENFORCE(m_pimpl);
        m_pimpl->open(is);
    }

    bool deserialization_archive::is_empty() const {
        return !m_pimpl && !m_temp;
    }

    bool deserialization_archive::is_primitive() const {
        return impl()->is_primitive();
    }

    bool deserialization_archive::is_null() const {
        return impl()->is_null();
    }

    bool deserialization_archive::is_map() const {
        return impl()->is_map();
    }

    bool deserialization_archive::is_array() const {
        return impl()->is_array();
    }

    bool deserialization_archive::has_key(const char* key) const {
        ENFORCE(impl()->is_map());
        return impl()->has_key(key);
    }

    std::optional <deserialization_archive> deserialization_archive::get(const char* key) const {
        if (!is_empty()) {
            ENFORCE(impl()->is_map());
            if (!impl()->has_key(key)) {
                return std::nullopt;
            }
            return std::make_optional <deserialization_archive>(impl()->get(key));
        }
        return std::nullopt;
    }

    void deserialization_archive::for_each(const std::function <void(const deserialization_archive&)>& f) const {
        ENFORCE(impl()->is_array());
        impl()->for_each_array([&f](const deserialization_archive_impl& archive) {
            const deserialization_archive ar(archive);
            f(ar);
        });
    }

    void deserialization_archive::for_each(
        const std::function <void(const std::string&, const deserialization_archive&)>& f) const {
        ENFORCE(impl()->is_map());
        impl()->for_each_map([&f](const std::string& name, const deserialization_archive_impl& archive) {
            const deserialization_archive ar(archive);
            f(name, ar);
        });
    }

    deserialization_archive::deserialization_archive(const deserialization_archive_impl& impl)
        : m_temp(&impl) {
    }

    const deserialization_archive_impl* deserialization_archive::impl() const {
        if (m_temp) {
            return m_temp;
        }
        ENFORCE(m_pimpl);
        return m_pimpl.get();
    }

}
