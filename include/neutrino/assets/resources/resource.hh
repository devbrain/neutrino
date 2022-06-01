//
// Created by igor on 31/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_HH

#include <memory>
#include <iosfwd>

#include <neutrino/assets/resources/resource_id.hh>
#include <neutrino/assets/resources/resource_storage.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {

  template <typename T, typename ... Args>
  class resource_reader;

  template <typename T>
  class resource {
      template <typename, typename...>
      friend class resource_reader;
    public:
      resource();
      resource(const resource& other);
      resource& operator = (const resource& other);

      resource(resource&& other) noexcept;
      resource& operator = (resource&& other) noexcept;

      ~resource();

      T* get();
      T* operator -> ();

      const T* get() const;
      const T* operator -> () const;

      [[nodiscard]] resource_id id() const noexcept;

      static resource<T> get(const resource_id& id);

      static resource<T> get(const std::string& name) {
        return get(resource_id(name));
      }

      static resource<T> get(const char* name) {
        return get(resource_id(name));
      }

      template <std::size_t N>
      inline
      static resource<T> get(const char(& value)[N]) {
        return get(resource_id(value));
      }
    private:
      resource(const resource_id& id, std::unique_ptr<T> data);
      explicit resource(const resource_id& id);
    private:
      resource_id                          m_id;
      static detail::resource_storage<T>   m_storage;
  };
}

// ------------------------------------------------------------------------------
// Implementation
// ------------------------------------------------------------------------------

namespace neutrino::assets {
  template <typename T>
  detail::resource_storage<T> resource<T>::m_storage;

  template <typename T>
  resource<T>::resource () = default;

  template <typename T>
  resource<T>::resource(const resource_id& id)
  : m_id(id) {
    m_storage.inc(m_id);
  }

  template <typename T>
  resource<T>::resource(const resource_id& id, std::unique_ptr<T> data)
  : m_id(id) {
    ENFORCE(id);
    m_storage.bind (id, std::move(data));
  }

  template <typename T>
  resource <T>::resource(const resource& other)
  : m_id(other.m_id) {
    if (m_id) {
      m_storage.inc(m_id);
    }
  }


  template <typename T>
  resource<T>& resource <T>::operator = (const resource& other) {
    if (this != & other) {
      m_storage.release (m_id);
      m_id = other.m_id;
      m_storage.inc (m_id);
    }
    return *this;
  }

  template <typename T>
  resource <T>::resource(resource&& other) noexcept
      : m_id (other.m_id) {
    other.m_id = {};
  }

  template <typename T>
  resource<T>& resource<T>::operator = (resource&& other) noexcept {
    if (this != &other) {
      m_id = other.m_id;
      other.m_id = {};
    }
    return *this;
  }

  template <typename T>
  resource<T>::~resource() {
    m_storage.release (m_id);
  }


  template <typename T>
  T* resource<T>::get() {
    return m_storage.data (m_id);
  }

  template <typename T>
  T* resource<T>::operator -> () {
    return get();
  }

  template <typename T>
  const T* resource<T>::get() const {
    return m_storage.data (m_id);
  }

  template <typename T>
  const T* resource<T>::operator -> () const {
    return get();
  }

  template <typename T>
  resource_id resource<T>::id() const noexcept {
    return m_id;
  }

  template <typename T>
  resource<T> resource<T>::get(const resource_id& id) {
    if (!m_storage.exists (id)) {
      RAISE_EX("Can not find resource ", id);
    }
    return resource<T>(id);
  }
}

#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_HH
