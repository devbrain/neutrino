//
// Created by igor on 08/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_RC_WORLD_OBJECT_HH
#define INCLUDE_NEUTRINO_KERNEL_RC_WORLD_OBJECT_HH

#include <any>
#include <map>
#include <string>
#include <memory>

#include <neutrino/utils/strings/string_constant.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {



  class object {
    public:
      class builder {
        public:
          explicit builder(std::size_t type);
          explicit builder(const std::string& type);

          builder(const builder&) = delete;
          builder& operator = (const builder&) = delete;

          ~builder();

          template <typename T>
          builder& set(const std::string& name, T val);

          object build();
        private:
          std::unique_ptr<object> m_obj;
      };

      friend class builder;
    public:
      [[nodiscard]] std::size_t type() const noexcept;

      template <typename T>
      T get(const std::string& name) const;

      template <typename T, std::size_t N>
      T get(const utils::string_constant<N>& name) const;

      [[nodiscard]] bool exists(const std::string& name) const noexcept;

      template <std::size_t N>
      [[nodiscard]] bool exists(const utils::string_constant<N>& name) const noexcept;
    private:
      explicit object(std::size_t type);

      template <typename T>
      void set(const std::string& name, T val);
    private:
      std::size_t m_type;
      std::map<std::string, std::any> m_fields;
  };

  template <typename T>
  T object::get(const std::string& name) const {
    auto i = m_fields.find (name);
    if (i == m_fields.end()) {
      RAISE_EX("Can not find field ", name);
    }
    return std::any_cast<T>(i->second);
  }

  template <typename T, std::size_t N>
  T object::get(const utils::string_constant<N>& name) const {
    auto i = m_fields.find (name.to_string());
    if (i == m_fields.end()) {
      RAISE_EX("Can not find field ", name.to_string());
    }
    return std::any_cast<T>(i->second);
  }

  template <typename T>
  void object::set(const std::string& name, T val) {
    m_fields.insert (std::make_pair (name, std::any(val)));
  }

  template <std::size_t N>
  bool object::exists(const utils::string_constant<N>& name) const noexcept {
    return exists (name.to_string());
  }

  template <typename T>
  object::builder& object::builder::set(const std::string& name, T val) {
    m_obj->template set (name, val);
    return *this;
  }
}

#endif //INCLUDE_NEUTRINO_KERNEL_RC_WORLD_OBJECT_HH
