//
// Created by igor on 31/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_STORAGE_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_STORAGE_HH

#include <unordered_map>
#include <atomic>
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

       void inc (const resource_id& id) const {
        auto itr = m_map.find (id);
        if (itr == m_map.end()) {
          return;
        }
        return itr->second.inc();
      }

      void release(const resource_id& id) {
        auto itr = m_map.find (id);
        if (itr != m_map.end()) {
          if (itr->second.dec()) {
            m_map.erase (itr);
          }
        }
      }

      [[nodiscard]] bool exists (const resource_id& id) const {
        return m_map.find (id) != m_map.end();
      }

      T* data(const resource_id& id) {
        auto itr = m_map.find (id);
        if (itr != m_map.end()) {
          return itr->second.data.get();
        }
        return nullptr;
      }

      const T* data(const resource_id& id) const {
        auto itr = m_map.find (id);
        if (itr != m_map.end()) {
          return itr->second.data.get();
        }
        return nullptr;
      }
    private:
      struct ref_counter {
        explicit ref_counter (std::unique_ptr<T> v)
        : data(std::move(v)), count(1) {
        }

        void inc() const {
          count++;
        }
        bool dec() const {
          count--;
          return (count == 0);
        }
        std::unique_ptr<T> data;
        mutable std::atomic<size_t> count;
      };
    private:
      std::unordered_map<resource_id, ref_counter> m_map;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_STORAGE_HH
