//
// Created by igor on 31/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_STORAGE_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_STORAGE_HH

#include <unordered_map>
#include <memory>
#include <neutrino/assets/resources/resource_id.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets::detail {
  template <typename T>
  class resource_storage {
    public:
      resource_storage() = default;
      ~resource_storage() = default;

      void bind(const resource_id& id, std::unique_ptr<T> data) {
        auto itr = m_map.find (id);
        if (itr != m_map.end()) {
          RAISE_EX("Can not bind resource ", id, " - already exists");
        }
        m_map.insert (std::make_pair (id, std::move(data)));
      }

      std::shared_ptr<T> get(const resource_id& id) const {
        auto itr = m_map.find (id);
        if (itr == m_map.end()) {
          return nullptr;
        }
        return itr->second;
      }

      void release(const resource_id& id) {
        auto itr = m_map.find (id);
        if (itr != m_map.end()) {
          itr->second.reset();
          if (itr->second.use_count() <= 0) {
            m_map.erase (itr);
          }
        }
      }
    private:
      std::unordered_map<resource_id, std::shared_ptr<T>> m_map;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_STORAGE_HH
