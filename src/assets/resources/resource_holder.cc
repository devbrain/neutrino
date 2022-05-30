//
// Created by igor on 29/05/2022.
//

#include <neutrino/assets/resources/resource_id.hh>
#include <neutrino/assets/resources/resource_holder.hh>
#include <neutrino/utils/register_at_exit.hh>

namespace neutrino::assets {

  bool resource_holder::exists (resource_name rn) const {
    return m_resources.find (rn) != m_resources.end();
  }

  const std::any& resource_holder::get (resource_name rn) const {
    auto itr = m_resources.find (rn);
    if (itr == m_resources.end()) {
      RAISE_EX("Can not find resource ", rn.name());
    }
    return itr->second;
  }

  void resource_holder::drop (resource_name rn) {
    auto itr = m_resources.find (rn);
    if (itr != m_resources.end()) {
      m_resources.erase (itr);
    }
  }

  static resource_holder* global_resources_holder = nullptr;


  resource_holder& get_resources_holder() {
    if (!global_resources_holder) {
      global_resources_holder = new resource_holder;
      static bool registered = false;
      if (!registered) {
        register_at_exit (destroy_resources_holder);
        registered = true;
      }
    }
    return *global_resources_holder;
  }

  void destroy_resources_holder() {
    delete global_resources_holder;
    global_resources_holder = nullptr;
  }
}