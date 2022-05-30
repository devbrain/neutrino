//
// Created by igor on 29/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_UNIQUE_RESOURCE_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_UNIQUE_RESOURCE_HH

#include <optional>
#include <neutrino/assets/resources/resource_name.hh>
#include <neutrino/assets/resources/resource_holder.hh>
#include <neutrino/utils/exception.hh>
#include <utility>

namespace neutrino::assets {

  namespace detail {
    class handle {
      public:

        constexpr handle() noexcept = default;

        explicit handle(resource_name p)
        : m_data(p) {
        }

        explicit handle(std::optional<resource_name> p)
            : m_data(p) {
        }

        void set(resource_name p) {
          drop();
          m_data = p;
        }

        void drop () {
          if (m_data) {
            get_resources_holder().drop (*m_data);
            m_data = std::nullopt;
          }
        }

        std::optional<resource_name> release () {
          auto temp = m_data;
          m_data = std::nullopt;
          return temp;
        }

        [[nodiscard]] bool empty() const noexcept {
          return !m_data.has_value();
        }

        ~handle() noexcept = default;

        [[nodiscard]] const std::any& get() const {
          return get_resources_holder().get (*m_data);
        }
        [[nodiscard]] resource_name name() const {
          return m_data.value();
        }
      private:
        std::optional<resource_name> m_data;
    };
  }


  template <typename T>
   class unique_resource {
    public:
      using pointer = T*;
      using element_type = T;


      /// Default constructor, creates a unique_resource that owns nothing
      constexpr unique_resource() noexcept = default;

      /// Takes ownership from a pointer
      explicit unique_resource (resource_name p) noexcept
          : _impl{p} {
      }
      /// Move constructor: takes ownership from a unique_resource of the same type
      unique_resource(unique_resource&& up) noexcept
          : _impl{up.release ()} {
      }

      /// Invokes the deleter if the stored pointer is not null
      ~unique_resource () noexcept {
        _impl.drop();
      }

      // 20.7.1.2.3, assignment

      /// Move assignment: takes ownership from a unique_resource of the same type
      unique_resource& operator = (unique_resource&& up) noexcept {
        reset (up.release ());
        return *this;
      }

      // 20.7.1.2.4, observers

      /// Dereferences pointer to the managed object
      const element_type& operator * () const noexcept {
        ENFORCE(!_impl.empty());
        return *std::any_cast<element_type>(&_impl.get());
      }

      /// Dereferences pointer to the managed object
      pointer operator -> () const noexcept {
        ENFORCE(!_impl.empty());
        return std::any_cast<element_type>(&_impl.get());
      }

      /// Gets the stored pointer
      [[nodiscard]] resource_name  get () const noexcept {
        ENFORCE(!_impl.empty());
        return _impl.name();
      }

      /// Checks if there is an associated managed object
      explicit operator bool () const noexcept {
        return !_impl.empty();
      }

      // 20.7.1.2.5 modifiers

      /// Releases ownership to the returned raw pointer
      std::optional<resource_name> release () noexcept {
        if (_impl.empty()) {
          return {};
        }
        return _impl.release();
      }

      /// Resets unique_resource to empty and takes ownership from a pointer
      void reset (resource_name rn) noexcept {
        _impl.set(rn);
      }

      /// Resets unique_resource to empty
      void
      reset () noexcept {
        _impl.drop();
        _impl = {};
      }

      /// Swaps with another unique_resource
      void
      swap (unique_resource& up) noexcept {
        using std::swap;
        swap (_impl, up._impl);
      }

      // Disable copy from lvalue

      /// Disables copy constructor
      unique_resource (const unique_resource&) = delete;

      /// Disables copy assignment
      unique_resource& operator = (const unique_resource&) = delete;

    private:
      detail::handle _impl;
  };

  template <typename T, std::size_t N, typename Loader>
  unique_resource<T> make_unique_resource(const char(& value)[N], Loader&& loader) {
    resource_name rn(value);
    get_resources_holder().add(rn, std::forward<Loader>(loader)());
    return unique_resource<T>{rn};
  }
}
#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_UNIQUE_RESOURCE_HH
