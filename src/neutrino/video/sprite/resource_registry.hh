//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstdint>
#include <limits>
#include <unordered_map>
#include <utility>

#include <failsafe/enforce.hh>

#include "detail/generic_handle_factory.hh"

namespace neutrino::detail {
    /**
     * @brief Shared storage for registries keyed by opaque resource handles.
     *
     * @p Id is a generic_handle, minted by make_handle(): from one counter per handle kind,
     * so an id stays unique across every registry and application of the process, and an id
     * kept from an earlier application names nothing instead of another resource. This
     * helper owns only the common map/get/erase code.
     */
    template <typename Id, typename Resource>
    class resource_registry {
        public:
            /**
             * @brief Store a resource and return its new opaque id. An id still in use, met
             *        once the counter wraps, is skipped.
             */
            Id store(Resource resource) {
                constexpr auto max_attempts =
                    static_cast <std::uint64_t>(std::numeric_limits <std::uint32_t>::max()) + 1u;

                for (std::uint64_t attempt = 0; attempt < max_attempts; ++attempt) {
                    Id key = make_handle <Id>();

                    if (contains(key)) {
                        continue;
                    }

                    m_resources.emplace(key, std::move(resource));
                    return key;
                }

                ENFORCE(false, "Resource registry id space exhausted");
                return Id{};
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
             * @brief Does this registry currently store @p id?
             */
            [[nodiscard]] bool contains(Id id) const {
                return m_resources.find(id) != m_resources.end();
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

            /**
             * @brief Does any stored resource satisfy @p pred? Stops at the first match.
             */
            template <typename Predicate>
            [[nodiscard]] bool any_of(Predicate&& pred) const {
                for (const auto& entry : m_resources) {
                    if (pred(entry.second)) {
                        return true;
                    }
                }
                return false;
            }

        private:
            std::unordered_map <Id, Resource> m_resources;
    };
}
