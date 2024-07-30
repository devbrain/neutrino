//
// Created by igor on 7/30/24.
//

#ifndef NEUTRINO_ECS_ITERATORS_REGISTRY_ITERATOR_HH
#define NEUTRINO_ECS_ITERATORS_REGISTRY_ITERATOR_HH

#include <tuple>
#include <neutrino/ecs/types.hh>
#include <neutrino/ecs/detail/component_bucket.hh>

namespace neutrino::ecs {
    class registry;

    template<typename... Components>
    class registry_iterator {
        friend class registry;

        public:
            [[nodiscard]] bool has_next() const;

            std::tuple <entity_id_t, Components*...> next(bool* all_is_full = nullptr);

            std::tuple <entity_id_t, const Components*...> cnext(bool* all_is_full = nullptr);

        protected:
            explicit registry_iterator(registry& owner);

        private:
            static std::vector <detail::component_bucket*> create_buckets_vector(registry& reg);

        private:
            registry& m_owner;
            std::vector <detail::component_bucket*> m_buckets;
            detail::component_bucket_iterator m_iterator;
    };

    template<typename Component>
    class registry_iterator <Component> {
        friend class registry;

        public:
            [[nodiscard]] bool has_next() const;

            std::tuple <entity_id_t, Component*> next(bool* all_is_full = nullptr);
            std::tuple <entity_id_t, const Component*> cnext(bool* all_is_full = nullptr);

        protected:
            explicit registry_iterator(registry& owner);

        private:
            registry& m_owner;
            detail::typed_component_bucket_iterator <Component> m_iterator;
    };

    template<typename... Components>
    class registry_iterator <std::tuple <Components...>> : public registry_iterator <Components...> {
        public:
        explicit registry_iterator(registry& owner);
    };
}

#endif
