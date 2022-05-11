//
// Created by igor on 08/05/2022.
//
#include <neutrino/kernel/rc/world/object.hh>
#include <neutrino/utils/switch_by_string.hh>

namespace neutrino::kernel {
  std::size_t object::type() const noexcept {
    return m_type;
  }

  bool object::exists(const std::string& name) const noexcept {
    auto i = m_fields.find (name);
    return i != m_fields.end();
  }

  object::object(std::size_t type)
  : m_type(type) {}


  object::builder::builder(std::size_t type)
  {
    m_obj = std::unique_ptr<object>(new object(type));
  }

  object::builder::builder(const std::string& type) {
    m_obj = std::unique_ptr<object>(new object(switcher (type.c_str())));
  }

  object::builder::~builder() = default;

  object object::builder::build() {
    return *m_obj;
  }
}
