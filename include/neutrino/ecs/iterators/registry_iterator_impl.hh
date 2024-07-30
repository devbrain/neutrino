//
// Created by igor on 7/30/24.
//

#ifndef NEUTRINO_ECS_ITERATORS_REGISTRY_ITERATOR_IMPL_HH
#define NEUTRINO_ECS_ITERATORS_REGISTRY_ITERATOR_IMPL_HH

#include <neutrino/ecs/iterators/registry_iterator.hh>
#include <neutrino/ecs/registry.hh>

namespace neutrino::ecs {

    namespace detail {
        template<typename... Components>
        struct bucket_accessor {
            explicit bucket_accessor(std::vector <component_bucket*>& buckets, bool* all_is_full)
                : buckets(buckets),
                  m_all_is_full(all_is_full) {
                if (m_all_is_full) {
                    *m_all_is_full = true;
                }
            }

            using ct_list_t = bsw::mp::type_list <Components...>;
            std::vector <component_bucket*>& buckets;
            bool* m_all_is_full;

            template<typename T>
            T* get(std::size_t eid, char* buff) {
                constexpr auto idx = bsw::mp::type_list_find_first_v <T, ct_list_t>;
                if constexpr (idx == 0) {
                    if (buff) {
                        return std::launder(reinterpret_cast <T*>(buff));
                    }
                    if (m_all_is_full) {
                        *m_all_is_full = false;
                    }
                    return nullptr;
                } else {
                    char* cbuff = buckets[idx]->get_block_by_name(eid);
                    if (cbuff) {
                        return std::launder(reinterpret_cast <T*>(cbuff));
                    }
                    if (m_all_is_full) {
                        *m_all_is_full = false;
                    }
                    return nullptr;
                }
            }

            template<typename T>
            const T* get(std::size_t eid, char* buff) const {
                constexpr auto idx = bsw::mp::type_list_find_first_v <T, ct_list_t>;
                if constexpr (idx == 0) {
                    if (buff) {
                        return std::launder(reinterpret_cast <const T*>(buff));
                    }
                    if (m_all_is_full) {
                        *m_all_is_full = false;
                    }
                    return nullptr;
                } else {
                    char* cbuff = buckets[idx]->get_block_by_name(eid);
                    if (cbuff) {
                        return std::launder(reinterpret_cast <const T*>(cbuff));
                    }
                    if (m_all_is_full) {
                        *m_all_is_full = false;
                    }
                    return nullptr;
                }
            }
        };
    }

    template<typename... Components>
    bool registry_iterator <Components...>::has_next() const {
        return m_iterator.has_next();
    }

    template<typename... Components>
    std::tuple <entity_id_t, Components*...> registry_iterator <Components...>::next(bool* all_is_full) {
        auto [buff, _] = m_iterator.next();
        auto eid = m_iterator.get_key();
        detail::bucket_accessor <Components...> ba(m_buckets, all_is_full);
        return std::make_tuple(entity_id_t(eid), ba.template get <Components>(eid, buff)...);
    }

    template<typename... Components>
    std::tuple <entity_id_t, const Components*...> registry_iterator <Components...>::cnext(bool* all_is_full) {
        auto [buff, _] = m_iterator.next();
        auto eid = m_iterator.get_key();
        const detail::bucket_accessor <Components...> ba(m_buckets, all_is_full);
        return std::make_tuple(entity_id_t(eid), ba.template get <Components>(eid, buff)...);
    }

    template<typename... Components>
    registry_iterator <Components...>::registry_iterator(registry& owner)
        : m_owner(owner),
          m_buckets(create_buckets_vector(owner)),
          m_iterator(*m_buckets[0]) {
    }

    template<typename... Components>
    std::vector <detail::component_bucket*> registry_iterator <Components...>::create_buckets_vector(registry& reg) {
        std::vector <detail::component_bucket*> buckets;
        bsw::mp::for_types <Components...>([&reg, &buckets](auto tp) {
            using ct = std::remove_const_t <std::remove_pointer_t <decltype(tp)>>;
            auto* bucket = reg.get_bucket_by_type <ct>();
            ENFORCE(bucket != nullptr);
            buckets.push_back(bucket);
        });
        ENFORCE(!buckets.empty());
        return buckets;
    }

    template<typename Component>
    bool registry_iterator <Component>::has_next() const {
        return m_iterator.has_next();
    }

    template<typename Component>
    std::tuple <entity_id_t, Component*> registry_iterator <Component>::next(bool* all_is_full) {
        auto [eid, c] = m_iterator.get_next_entity();
        if (all_is_full) {
            *all_is_full = true;
        }
        return {eid, &c};
    }

    template<typename Component>
    std::tuple <entity_id_t, const Component*> registry_iterator <Component>::cnext(bool* all_is_full) {
        auto [eid, c] = m_iterator.get_next_entity();
        if (all_is_full) {
            *all_is_full = true;
        }
        return {eid, &c};
    }

    template<typename Component>
    registry_iterator <Component>::registry_iterator(registry& owner)
        : m_owner(owner),
          m_iterator(*m_owner.get_bucket_by_type <Component>()) {
    }

    template<typename... Components>
    registry_iterator <std::tuple <Components...>>::registry_iterator(registry& owner)
        : registry_iterator <Components...>(owner) {
    }
}

#endif
