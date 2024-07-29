//
// Created by igor on 7/27/24.
//

#ifndef NEUTRINO_ECS_REGISTRY_HH
#define NEUTRINO_ECS_REGISTRY_HH

#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <boost/callable_traits.hpp>

#include <jg/dense_hash_map.hpp>
#include <neutrino/neutrino_export.hh>
#include <neutrino/ecs/types.hh>
#include <neutrino/ecs/detail/component_bucket.hh>
#include <bsw/mp/type_name/type_name.hpp>
#include <bsw/mp/typelist.hh>
#include <bsw/mp/constexpr_for.hh>

#include "neutrino/ecs/registry.hh"
#include "neutrino/ecs/types.hh"

namespace neutrino::ecs {
    template<typename... Components>
    class registry_iterator;

    class NEUTRINO_EXPORT registry {
        template<typename... Components>
        friend class registry_iterator;

        public:
            explicit registry(std::size_t max_components = 64);

            template<typename Component, typename... Args>
            void bind_component(entity_id_t key, Args&&... args);

            [[nodiscard]] bool exists(entity_id_t e) const;

            template<typename Component>
            [[nodiscard]] bool has_component(entity_id_t e) const;

            template<typename Component>
            void remove_component(entity_id_t e);

            void remove_entity(entity_id_t e);

            [[nodiscard]] std::vector <std::string> list_components(entity_id_t e) const;

            template<typename Component>
            const Component& get_component(entity_id_t e) const;

            template<typename Component>
            Component& get_component(entity_id_t e);

            template<typename... Components>
            registry_iterator <Components...> iterator();

            template<typename Callable>
            void iterate(Callable&& func);

        private:
            template<typename T>
            [[nodiscard]] detail::component_bucket* get_bucket_by_type();

            template<typename T>
            [[nodiscard]] const detail::component_bucket* get_bucket_by_type() const;

            using components_holder_t = jg::dense_hash_map <
                std::size_t, std::unique_ptr <detail::component_bucket, void(*)(detail::component_bucket*)>>;
            components_holder_t m_components;
            std::size_t m_max_components;

            struct NEUTRINO_EXPORT component_info {
                component_info(std::string  name_, std::size_t bucket)
                    : name(std::move(name_)),
                      bucket(bucket) {
                }

                std::string name;
                std::size_t bucket;
            };

            using components_id_map_t = jg::dense_hash_map <std::size_t, std::size_t>;
            components_id_map_t m_components_id_map;

            using components_names_map_t = jg::dense_hash_map <std::size_t, component_info>;
            components_names_map_t m_components_names_map;

            struct NEUTRINO_EXPORT entity_description {
                explicit entity_description(std::size_t max_components, std::size_t index);
                std::vector <bool> components;
            };

            using entities_map_t = jg::dense_hash_map <detail::underlying_entity_type, entity_description>;
            entities_map_t m_ents_map;
    };

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
    class registry_iterator {
        friend class registry;

        public:
            [[nodiscard]] bool has_next() const {
                return m_iterator.has_next();
            }

            std::tuple <entity_id_t, Components*...> next(bool* all_is_full = nullptr) {
                auto [buff, _] = m_iterator.next();
                auto eid = m_iterator.get_key();
                detail::bucket_accessor <Components...> ba(m_buckets, all_is_full);
                return std::make_tuple(entity_id_t(eid), ba.template get <Components>(eid, buff)...);
            }

            std::tuple <entity_id_t, const Components*...> cnext(bool* all_is_full = nullptr) {
                auto [buff, _] = m_iterator.next();
                auto eid = m_iterator.get_key();
                const detail::bucket_accessor <Components...> ba(m_buckets, all_is_full);
                return std::make_tuple(entity_id_t(eid), ba.template get <Components>(eid, buff)...);
            }

        protected:
            explicit registry_iterator(registry& owner)
                : m_owner(owner),
                  m_buckets(create_buckets_vector(owner)),
                  m_iterator(*m_buckets[0]) {
            }

        private:
            static std::vector <detail::component_bucket*> create_buckets_vector(registry& reg) {
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

        private:
            registry& m_owner;
            std::vector <detail::component_bucket*> m_buckets;
            detail::component_bucket_iterator m_iterator;
    };

    template<typename Component>
    class registry_iterator <Component> {
        friend class registry;

        public:
            [[nodiscard]] bool has_next() const {
                return m_iterator.has_next();
            }

            std::tuple <entity_id_t, Component*> next(bool* all_is_full = nullptr) {
                auto [eid, c] = m_iterator.get_next_entity();
                if (all_is_full) {
                    *all_is_full = true;
                }
                return {eid, &c};
            }

            std::tuple <entity_id_t, const Component*> cnext(bool* all_is_full = nullptr) {
                auto [eid, c] = m_iterator.get_next_entity();
                if (all_is_full) {
                    *all_is_full = true;
                }
                return {eid, &c};
            }

        protected:
            explicit registry_iterator(registry& owner)
                : m_owner(owner),
                  m_iterator(*m_owner.get_bucket_by_type <Component>()) {
            }

        private:
            registry& m_owner;
            detail::typed_component_bucket_iterator <Component> m_iterator;
    };


    template<typename... Components>
    class registry_iterator <std::tuple <Components...>> : public registry_iterator <Components...> {
        public:
            explicit registry_iterator(registry& owner)
                : registry_iterator <Components...>(owner) {
            }
    };

    class NEUTRINO_EXPORT entity_builder {
        public:
            explicit entity_builder(registry& registry_);

            template<typename Component, typename... Args>
            const entity_builder& with_component(Args&&... args) const;

            entity_id_t build() const;

        private:
            registry& m_registry;
            static detail::underlying_entity_type s_entity_counter;
    };

    template<typename Component, typename... Args>
    void registry::bind_component(entity_id_t key, Args&&... args) {
        constexpr std::size_t bucket_key = type_hash_v <Component>;
        auto itr = m_components.find(bucket_key);
        if (itr == m_components.end()) {
            m_components.insert({
                bucket_key,
                detail::typed_component_bucket <Component>::construct_and_create(
                    m_max_components, key, std::forward <Args>(args)...)
            });
        } else {
            detail::typed_component_bucket <Component>::construct(*itr->second, key, std::forward <Args>(args)...);
        }
        auto ci = m_components_id_map.find(bucket_key);
        std::size_t component_index = 0;
        if (ci == m_components_id_map.end()) {
            auto cmp_id = m_components_id_map.size();
            m_components_id_map.insert(std::make_pair(bucket_key, cmp_id));
            m_components_names_map.insert(std::make_pair(cmp_id, component_info(std::string(type_name_v <Component>), bucket_key)));
            component_index = cmp_id;
        } else {
            component_index = ci->second;
        }
        auto ei = m_ents_map.find(key.value_of());
        if (ei == m_ents_map.end()) {
            m_ents_map.insert(std::make_pair(key.value_of(), entity_description(m_max_components, component_index)));
        } else {
            ei->second.components[component_index] = true;
        }
    }

    template<typename Component>
    bool registry::has_component(entity_id_t e) const {
        constexpr auto bucket_key = type_hash_v <Component>;
        auto ci = m_components_id_map.find(bucket_key);
        if (ci == m_components_id_map.end()) {
            return false;
        }
        auto ei = m_ents_map.find(e.value_of());
        if (ei == m_ents_map.end()) {
            return false;
        }
        return ei->second.components[ci->second];
    }

    template<typename Component>
    void registry::remove_component(entity_id_t e) {
        constexpr auto bucket_key = type_hash_v <Component>;
        auto i = m_components.find(bucket_key);
        if (i != m_components.end()) {
            detail::typed_component_bucket <Component>::destruct(*i->second, e);
        }

        auto ci = m_components_id_map.find(bucket_key);
        if (ci == m_components.end()) {
            return;
        }
        auto ei = m_ents_map.find(e.value_of());
        if (ei == m_ents_map.end()) {
            return;
        }
        ei->second.components[ci->second] = false;
    }

    template<typename Component>
    const Component& registry::get_component(entity_id_t e) const {
        constexpr auto bucket_key = type_hash_v <Component>;
        auto i = m_components.find(bucket_key);
        ENFORCE(i != m_components.end());
        return detail::typed_component_bucket <Component>::get(*i->second, e);
    }

    template<typename Component>
    Component& registry::get_component(entity_id_t e) {
        constexpr auto bucket_key = type_hash_v <Component>;
        auto i = m_components.find(bucket_key);
        ENFORCE(i != m_components.end());
        return detail::typed_component_bucket <Component>::get(*i->second, e);
    }

    template<typename... Components>
    registry_iterator <Components...> registry::iterator() {
        return registry_iterator <Components...>(*this);
    }

    namespace detail {
        template<typename T>
        T call_adaptor(T x) {
            return x;
        }

        template<typename T>
        T& call_adaptor(T* x) {
            return *x;
        }

        template<int ...>
        struct seq {
        };

        template<int N, int ... S>
        struct gens : gens <N - 1, N - 1, S...> {
        };

        template<int ... S>
        struct gens <0, S...> {
            typedef seq <S...> type;
        };

        template<typename Callable, typename Tuple, int ... S>
        void callFunc(Callable&& func, const Tuple& params, seq <S...>) {
            func(call_adaptor(std::get <S>(params))...);
        }

        template<typename Callable, typename Tuple, int ... S>
        boost::callable_traits::return_type_t<Callable> callFuncRet(Callable&& func, const Tuple& params, seq <S...>) {
            return func(call_adaptor(std::get <S>(params))...);
        }

        template<typename... T>
        struct parse_args;

        template<typename... T>
        struct parse_args <std::tuple <const entity_id_t&, T...>> {
            using first_arg = const entity_id_t&;
            using components = std::tuple <std::decay_t <T>...>;
        };

        template<typename... T>
        struct parse_args <std::tuple <const entity_id_t, T...>> {
            using first_arg = entity_id_t;
            using components = std::tuple <std::decay_t <T>...>;
        };

        template<typename... T>
        struct parse_args <std::tuple <entity_id_t, T...>> {
            using first_arg = entity_id_t;
            using components = std::tuple <std::decay_t <T>...>;
        };
    }

    template<typename Callable>
    void registry::iterate(Callable&& func) {
        using args = boost::callable_traits::args_t <Callable>;
        using pargs_t = detail::parse_args <args>;
        registry_iterator <typename pargs_t::components> itr(*this);
        auto idx_seq = typename detail::gens<std::tuple_size_v<args>>::type();
        while (itr.has_next()) {
            bool has_all = false;
            auto tuple = itr.next(&has_all);
            if (has_all) {
                if constexpr (boost::callable_traits::has_void_return_v<Callable>) {
                    detail::callFunc(func, tuple, idx_seq);
                } else {
                    if (!detail::callFuncRet(func, tuple, idx_seq)) {
                        break;
                    }
                }
            }
        }
    }

    template<typename T>
    detail::component_bucket* registry::get_bucket_by_type() {
        constexpr std::size_t bucket_key = type_hash_v <T>;
        auto itr = m_components.find(bucket_key);
        if (itr == m_components.end()) {
            return nullptr;
        }
        return itr->second.get();
    }

    template<typename T>
    const detail::component_bucket* registry::get_bucket_by_type() const {
        constexpr std::size_t bucket_key = type_hash_v <T>;
        auto itr = m_components.find(bucket_key);
        if (itr == m_components.end()) {
            return nullptr;
        }
        return itr->second.get();
    }

    template<typename Component, typename... Args>
    const entity_builder& entity_builder::with_component(Args&&... args) const {
        m_registry.bind_component <Component>(entity_id_t(s_entity_counter), std::forward <Args>(args)...);
        return *this;
    }
}

#endif
