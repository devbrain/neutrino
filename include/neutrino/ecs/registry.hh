//
// Created by igor on 7/27/24.
//

#ifndef NEUTRINO_ECS_REGISTRY_HH
#define NEUTRINO_ECS_REGISTRY_HH

#include <utility>
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <tuple>
#include <type_traits>

#include <boost/callable_traits.hpp>
#include <sul/dynamic_bitset.hpp>

#include <jg/dense_hash_map.hpp>
#include <neutrino/neutrino_export.hh>
#include <neutrino/ecs/types.hh>
#include <neutrino/ecs/detail/component_bucket.hh>
#include <bsw/mp/type_name/type_name.hpp>
#include <bsw/mp/typelist.hh>
#include <bsw/mp/constexpr_for.hh>

#include "neutrino/ecs/iterators/registry_iterator.hh"
#include "neutrino/ecs/types.hh"

namespace neutrino::ecs {

    class NEUTRINO_EXPORT registry {
        template<typename... Components>
        friend class registry_iterator;

        template<typename... Components>
        friend class entities_set_iterator;

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
                sul::dynamic_bitset<> components;
            };

            using entities_map_t = jg::dense_hash_map <detail::underlying_entity_type, entity_description>;
            entities_map_t m_ents_map;
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
        constexpr auto idx_seq = typename detail::gens<std::tuple_size_v<args>>::type();
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

#include "neutrino/ecs/iterators/registry_iterator_impl.hh"

#endif
