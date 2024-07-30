//
// Created by igor on 7/22/24.
//

#ifndef COMPONENT_BUCKET_HH
#define COMPONENT_BUCKET_HH

#include <cstdint>
#include <tuple>
#include <memory>
#include <type_traits>
#include <new>
#include <neutrino/neutrino_export.hh>
#include <neutrino/ecs/types.hh>
#include <neutrino/ecs/detail/bi_map.hh>

namespace neutrino::ecs::detail {
    class NEUTRINO_EXPORT component_bucket_iterator;

    class NEUTRINO_EXPORT component_bucket {
        friend class component_bucket_iterator;

        public:
            using key_t = underlying_entity_type;
            using index_t = uint16_t;

        public:
            component_bucket(std::size_t alignment,
                std::size_t size_of_element,
                uint16_t count,
                void (*destructor) (component_bucket& bucket, entity_id_t entity_id));
            ~component_bucket();

            [[nodiscard]] std::tuple <index_t, char*> get_free() const;
            [[nodiscard]] char* get_block_by_name(key_t name) const;
            [[nodiscard]] key_t get_key_by_index(index_t idx) const;

            void mark_occupied(key_t name, index_t index);
            void mark_free(key_t name);

            [[nodiscard]] std::size_t capacity() const;
            [[nodiscard]] std::size_t size() const;

            void destruct(entity_id_t entity_id);
        private:
            char* m_storage;
            std::size_t m_size_of_element;
            std::size_t m_capacity;
            using names_map_t = bi_map <key_t, index_t>;
            names_map_t m_names_map;
            mutable sorted_array <index_t> m_free;

            void (*m_destructor) (component_bucket& bucket, entity_id_t entity_id);
    };

    class NEUTRINO_EXPORT component_bucket_iterator {
        public:
            explicit component_bucket_iterator(const component_bucket& owner);
            [[nodiscard]] bool has_next() const;
            std::tuple <char*, std::size_t> next();
            [[nodiscard]] component_bucket::key_t get_key() const;

        protected:
            const component_bucket& m_owner;

        private:
            std::size_t m_cur_block;
            std::size_t m_free_pos;
            std::size_t m_free_idx;
            std::size_t m_blocks_seen;
            std::size_t m_last_taken;
    };

    template<typename T>
    class typed_component_bucket_iterator : component_bucket_iterator {
        public:
            explicit typed_component_bucket_iterator(component_bucket& bucket)
                : component_bucket_iterator(bucket) {
            }

            using component_bucket_iterator::has_next;

            T& get_next() {
                auto [element, _] = next();
                ENFORCE(element != nullptr);
                return *std::launder(reinterpret_cast <T*>(element));
            }

            std::tuple <entity_id_t, T&> get_next_entity() {
                auto [element, idx] = next();
                ENFORCE(element != nullptr);
                return {entity_id_t(m_owner.get_key_by_index(idx)), *std::launder(reinterpret_cast <T*>(element))};
            }

            const T& get_next() const {
                auto [element, _] = const_cast <typed_component_bucket_iterator <T>*>(this)->next();
                ENFORCE(element != nullptr);
                return *std::launder(reinterpret_cast <const T*>(element));
            }

            std::tuple <entity_id_t, const T&> get_next_entity() const {
                auto [element, idx] = const_cast <typed_component_bucket_iterator <T>*>(this)->next();
                ENFORCE(element != nullptr);
                return {
                    entity_id_t(m_owner.get_key_by_index(idx)), *std::launder(reinterpret_cast <const T*>(element))
                };
            }

            [[nodiscard]] entity_id_t get_entity() const {
                return entity_id_t(get_key());
            }
    };

    template<typename T>
    class typed_component_bucket {
        public:
            static void destroy(component_bucket* bucket) {
                component_bucket_iterator itr(*bucket);
                while (itr.has_next()) {
                    auto [buff, _] = itr.next();
                    std::destroy_at(std::launder(reinterpret_cast <T*>(buff)));
                }
            }

            static std::unique_ptr <component_bucket, void(*)(component_bucket*)> create(uint16_t capacity) {
                return std::unique_ptr <component_bucket, void(*)(component_bucket*)>(
                    new component_bucket(alignof(T), sizeof(T), capacity, typed_component_bucket <T>::destruct),
                    typed_component_bucket <T>::destroy);
            }

            template<typename... Args>
            static void construct(component_bucket& bucket, entity_id_t entity_id, Args&&... args) {
                auto [index, buff] = bucket.get_free();
                ENFORCE(buff != nullptr);
                ::new(buff) T(std::forward <Args>(args)...);
                bucket.mark_occupied(entity_id.value_of(), index);
            }

            template<typename... Args>
            static std::unique_ptr <component_bucket, void(*)(component_bucket*)> construct_and_create(uint16_t capacity, entity_id_t entity_id, Args&&... args) {
                auto out = create(capacity);
                auto [index, buff] = out->get_free();
                ENFORCE(buff != nullptr);
                ::new(buff) T(std::forward <Args>(args)...);
                out->mark_occupied(entity_id.value_of(), index);
                return out;
            }

            static void destruct(component_bucket& bucket, entity_id_t entity_id) {
                char* buff = bucket.get_block_by_name(entity_id.value_of());
                if (buff) {
                    std::destroy_at(std::launder(reinterpret_cast <T*>(buff)));
                    bucket.mark_free(entity_id.value_of());
                }
            }

            template<typename... Args>
            static void replace(component_bucket& bucket, entity_id_t entity_id, Args&&... args) {
                char* buff = bucket.get_block_by_name(entity_id.value_of());
                ENFORCE(buff);
                ::new(buff) T(std::forward <Args>(args)...);
            }

            [[nodiscard]] bool exists(component_bucket& bucket, entity_id_t entity_id) const {
                return bucket.get_block_by_name(entity_id.value_of()) != nullptr;
            }

            static T& get(component_bucket& bucket, entity_id_t entity_id) {
                char* buff = bucket.get_block_by_name(entity_id.value_of());
                ENFORCE(buff);
                return *std::launder(reinterpret_cast <T*>(buff));
            }

            static T* get(component_bucket* bucket, entity_id_t entity_id) {
                char* buff = bucket->get_block_by_name(entity_id.value_of());
                if (buff) {
                    return *std::launder(reinterpret_cast <T*>(buff));
                }
                return nullptr;
            }

            const T& get(const component_bucket& bucket, entity_id_t entity_id) const {
                const char* buff = bucket.get_block_by_name(entity_id.value_of());
                ENFORCE(buff);
                return *std::launder(reinterpret_cast <const T*>(buff));
            }

            const T* get(const component_bucket* bucket, entity_id_t entity_id) const {
                const char* buff = bucket->get_block_by_name(entity_id.value_of());
                if (buff) {
                    return *std::launder(reinterpret_cast <const T*>(buff));
                }
                return nullptr;
            }

            static typed_component_bucket_iterator <T> iterator(component_bucket& bucket) {
                return typed_component_bucket_iterator <T>(bucket);
            }
    };
}
#endif
