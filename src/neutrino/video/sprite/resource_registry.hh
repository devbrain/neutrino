//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstdint>
#include <map>
#include <utility>

#include <failsafe/enforce.hh>

namespace neutrino::details {
    /**
     * @brief Shared storage for registries keyed by opaque resource handles.
     *
     * Domain registries keep their creation policy and pass a private id factory
     * into @ref store. This helper owns only the common map/counter/get/erase code.
     */
    template <typename Id, typename Resource>
    class resource_registry {
        public:
            /**
             * @brief Store a resource and return its new opaque id.
             */
            template <typename MakeId>
            Id store(MakeId make_id, Resource resource) {
                Id key = make_id(s_counter);
                m_resources.emplace(key, std::move(resource));
                ++s_counter;
                return key;
            }

            /**
             * @brief Resolve a mutable resource.
             *
             * @pre @p id must identify a resource stored in this registry.
             */
            Resource& get(Id id) {
                auto itr = m_resources.find(id);
                ENFORCE(itr != m_resources.end());
                return itr->second;
            }

            /**
             * @brief Resolve a const resource.
             *
             * @pre @p id must identify a resource stored in this registry.
             */
            const Resource& get(Id id) const {
                auto itr = m_resources.find(id);
                ENFORCE(itr != m_resources.end());
                return itr->second;
            }

            /**
             * @brief Remove a resource if present.
             */
            void erase(Id id) {
                auto itr = m_resources.find(id);
                if (itr != m_resources.end()) {
                    m_resources.erase(itr);
                }
            }

            /**
             * @brief Visit each stored resource.
             */
            template <typename Function>
            void for_each_resource(Function&& fn) {
                for (auto& entry : m_resources) {
                    fn(entry.second);
                }
            }

            /**
             * @brief Visit each stored resource without mutation.
             */
            template <typename Function>
            void for_each_resource(Function&& fn) const {
                for (const auto& entry : m_resources) {
                    fn(entry.second);
                }
            }

        private:
            std::map <Id, Resource> m_resources;
            static inline std::uint32_t s_counter = 0;
    };
}
