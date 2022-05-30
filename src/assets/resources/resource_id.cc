//
// Created by igor on 30/05/2022.
//

#include <iostream>
#include <neutrino/assets/resources/resource_id.hh>
#include <neutrino/utils/register_at_exit.hh>
#include "database.hpp"

namespace neutrino::assets {
  namespace detail {
    foonathan::string_id::basic_database& get_database () {
      static foonathan::string_id::default_database* db = nullptr;
      if (!db) {
        db = new foonathan::string_id::default_database;
        neutrino::register_at_exit ([](){ delete db; });
      }
      return *db;
    }
  }

  resource_id::resource_id()
      : m_id("", detail::get_database()) {
  }

  resource_id::resource_id(const std::string& name)
  : m_id(name.c_str(), detail::get_database()) {
  }

  resource_id::resource_id(const char* name)
  : m_id(name, detail::get_database()) {
  }

  resource_id::hash_t resource_id::hash() const noexcept {
    return m_id.hash_code();
  }

  std::string resource_id::name() const noexcept {
    const char* pname = m_id.string();
    if (!pname) {
      return "";
    }
    return pname;
  }

  std::ostream& operator << (std::ostream& os, const resource_id& rid) {
    os << rid.hash() << ":" << rid.name();
    return os;
  }
}

