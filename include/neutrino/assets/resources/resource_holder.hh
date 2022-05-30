//
// Created by igor on 29/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_HOLDER_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_HOLDER_HH

#include <any>
#include <string>
#include <unordered_map>
#include <neutrino/assets/resources/resource_name.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {
  class resource_holder {
    public:
      template <typename T>
      void add (resource_name rn, T value) {
        ENFORCE(m_resources.find (rn) == m_resources.end());
        m_resources.insert (std::make_pair(rn, std::move(value)));
      }

      [[nodiscard]] bool exists(resource_name rn) const;
      [[nodiscard]] const std::any& get(resource_name rn) const;
      void drop(resource_name rn);

    private:
      std::unordered_map<resource_name, std::any> m_resources;
  };

  resource_holder& get_resources_holder();
  void destroy_resources_holder();
}

#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_HOLDER_HH
