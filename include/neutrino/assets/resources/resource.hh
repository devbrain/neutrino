//
// Created by igor on 31/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_HH

#include <memory>
#include <iosfwd>

#include <neutrino/utils/mp/type_name/type_name.hpp>
#include <neutrino/assets/resources/resource_id.hh>
#include <neutrino/assets/resources/resource_storage.hh>

namespace neutrino::assets {

  class resource_processing_context {
    public:
      virtual ~resource_processing_context();
      virtual void on_load_begin(const resource_id& id);
      virtual void on_load_end(const resource_id& id);
      virtual void on_load_failure(const resource_id& id);

      virtual void on_save_begin(const resource_id& id);
      virtual void on_save_end(const resource_id& id);
      virtual void on_save_failure(const resource_id& id);
  };

  template <typename T, typename ... Args>
  class resource {
    public:
      resource(const resource_id& id, resource_processing_context* ctx);

      virtual ~resource();

      void load(Args&&...args);
      void save(std::ostream& os) const;

      T* get();
      const T* get() const;

      resource_id id() const noexcept;

      static std::shared_ptr<T> get(const resource_id& id);
    protected:
      virtual std::unique_ptr<T> do_load(Args... args) = 0;
      virtual void do_save(std::ostream& os, const T& data);
    private:
      resource_id                          m_id;
      mutable resource_processing_context* m_ctx;
      static detail::resource_storage<T>   m_storage;
  };
}

// ------------------------------------------------------------------------------
// Implementation
// ------------------------------------------------------------------------------

namespace neutrino::assets {
  template <typename T, typename ... Args>
  detail::resource_storage<T> resource<T, Args...>::m_storage;

  template <typename T, typename ... Args>
  resource<T, Args...>::resource(const resource_id& id, resource_processing_context* ctx)
  : m_id(id), m_ctx(ctx) {

  }

  template <typename T, typename ... Args>
  resource<T, Args...>::~resource() {
    m_storage.release (m_id);
  }

  template <typename T, typename ... Args>
  void resource<T, Args...>::load(Args&&...args) {
    if (m_ctx) {
      m_ctx->on_load_begin (m_id);
    }
    try {
      m_storage.bind (m_id, this->do_load (std::forward<Args&&>(args)...));
    } catch (...) {
      if (m_ctx) {
        m_ctx->on_load_failure (m_id);
        throw;
      }
    }
    if (m_ctx) {
      m_ctx->on_load_end (m_id);
    }
  }

  template <typename T, typename ... Args>
  void resource<T, Args...>::save(std::ostream& os) const {
    auto* obj = this->get();
    if (!obj) {
      if (m_ctx) {
        m_ctx->on_save_failure(m_id);
      }
      return;
    }
    if (m_ctx) {
      m_ctx->on_save_begin (m_id);
    }
    try {
      this->do_save (os, *obj);
    } catch (...) {
      if (m_ctx) {
        m_ctx->on_save_failure(m_id);
      }
      throw ;
    }
    if (m_ctx) {
      m_ctx->on_save_end(m_id);
    }
  }

  template <typename T, typename ... Args>
  void resource<T, Args...>::do_save([[maybe_unused]] std::ostream& os, [[maybe_unused]] const T& data) {
    RAISE_EX("Saving of resource type ", type_name_v<T>, " is not implemented");
  }

  template <typename T, typename ... Args>
  T* resource<T, Args...>::get() {
    return m_storage.get (m_id).get();
  }

  template <typename T, typename ... Args>
  const T* resource<T, Args...>::get() const {
    return m_storage.get (m_id).get();
  }

  template <typename T, typename ... Args>
  resource_id resource<T, Args...>::id() const noexcept {
    return m_id;
  }

  template <typename T, typename ... Args>
  std::shared_ptr<T> resource<T, Args...>::get(const resource_id& id) {
    return m_storage.get (id);
  }
}

#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_HH
