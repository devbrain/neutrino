//
// Created by igor on 01/06/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_IO_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_IO_HH

#include <iostream>
#include <neutrino/assets/resources/resource_id.hh>
#include <neutrino/assets/resources/resource.hh>

namespace neutrino::assets {

  class resource_processing_context {
    public:
      virtual ~resource_processing_context ();
      virtual void on_load_begin (const resource_id& id);
      virtual void on_load_end (const resource_id& id);
      virtual void on_load_failure (const resource_id& id);

      virtual void on_save_begin (const resource_id& id);
      virtual void on_save_end (const resource_id& id);
      virtual void on_save_failure (const resource_id& id);
  };

  template <typename T>
  class resource_writer {
    public:
      virtual ~resource_writer() = default;

      void save(std::ostream& os, const resource_id& id, resource_processing_context* ctx = nullptr) {
        auto r = resource<T>::get (id).get();
        save(os, r, id, ctx);
      }

      void save(std::ostream& os, const std::string& id, resource_processing_context* ctx = nullptr) {
        save(os, resource_id(id), ctx);
      }

      void save(std::ostream& os, const char* id, resource_processing_context* ctx = nullptr) {
        save(os, resource_id(id), ctx);
      }

      template <std::size_t N>
      inline
      void save(std::ostream& os, const char(& id)[N], resource_processing_context* ctx = nullptr) {
        save(os, resource_id(id), ctx);
      }

      void save(std::ostream& os, const resource<T>& res, resource_processing_context* ctx = nullptr) {
        auto id = res.id();
        auto r = resource<T>::get (id).get();
        save(os, r, id, ctx);
      }
    protected:
      virtual void do_save(std::ostream& os, const T& data) = 0;
    private:
      void save(std::ostream& os, const T* r, const resource_id& id,  resource_processing_context* ctx) {
        if (!r) {
          if (ctx) {
            ctx->on_save_failure (id);
          }
          RAISE_EX("Resource ", id, " is unbound");
        }
        if (ctx) {
          ctx->on_save_begin (id);
        }
        try {
          do_save (os, *r);
          if (ctx) {
            ctx->on_save_end (id);
          }
        } catch (...) {
          if (ctx) {
            ctx->on_save_failure (id);
          }
          throw ;
        }
      }
  };

  template <typename T, typename ... Args>
  class resource_reader {
    public:
      virtual ~resource_reader() = default;

      [[nodiscard]] bool exists(const resource_id& id) const {
        return resource<T>::m_storage.exists(id);
      }

      resource<T> load(Args&&... args, const std::string& id, resource_processing_context* ctx = nullptr) {
          return load(std::forward<Args>(args)..., resource_id(id), ctx);
      }

      resource<T> load(Args&&... args, const char* id, resource_processing_context* ctx = nullptr) {
        return load(std::forward<Args>(args)..., resource_id(id), ctx);
      }

      template <std::size_t N>
      inline
      resource<T> load(Args&&... args, const char(& id)[N], resource_processing_context* ctx = nullptr) {
        return load(std::forward<Args>(args)..., resource_id(id), ctx);
      }

      resource<T> load(Args&&... args, const resource_id& id, resource_processing_context* ctx = nullptr) {
        if (ctx) {
          ctx->on_load_begin (id);
        }
        try {
          auto p = do_load (std::forward<Args>(args)...);
          if (!p) {
            if (ctx) {
              ctx->on_load_failure (id);
            }
            RAISE_EX("Failed to load resource ", id);
          }
          if (ctx) {
            ctx->on_load_end (id);
          }
          return resource<T>(id, std::move(p));
        } catch (...) {
          if (ctx) {
            ctx->on_load_failure (id);
          }
          throw;
        }
      }
    protected:
      virtual std::unique_ptr<T> do_load(Args... args) = 0;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_IO_HH
