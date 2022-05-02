//
// Created by igor on 01/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_ECS_REGISTRY_HH
#define INCLUDE_NEUTRINO_KERNEL_ECS_REGISTRY_HH

#include <cstdint>
#include <any>
#include <tuple>
#include <type_traits>

#include <neutrino/kernel/ecs/sparse_map.hh>
#include <neutrino/kernel/ecs/sparse_set.hh>
#include <neutrino/kernel/ecs/id_factory.hh>
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/mp/constexpr_for.hh>


namespace neutrino::ecs {
  namespace detail {
    using any_t = std::any;
    using component_val_t = std::tuple<id_t, any_t>;
    using entity_to_component = sparse_map<id_t, component_val_t>;
    using components_map_t = std::vector<entity_to_component>;
  }

  class registry {
    public:
      registry ();
      explicit registry (std::pmr::memory_resource& mr);

      registry (const registry&) = delete;
      registry& operator = (const registry&) = delete;

      [[nodiscard]] id_t create_id() const;

      template <typename Component>
      void attach_component (id_t entity_id, Component obj);

      template <typename Component>
      void detach_component (id_t entity_id);

      template <typename Component>
      [[nodiscard]] bool has_component (id_t entity_id) const;

      template <typename Component>
      Component& get_component (id_t entity_id);

      template <typename Component>
      const Component& get_component (id_t entity_id) const;

      void delete_entity (id_t entity_id);

      [[nodiscard]] bool exists (id_t entity_id) const;

      void clear();

      template <class ... Components>
      class view {
          friend class registry;
        protected:
          explicit view (detail::components_map_t& components);
        public:
          class iterator {
              friend class view<Components...>;
            public:
              using value_type = id_t;
              using pointer = value_type* ;
              using const_pointer = const value_type *;
              using reference = value_type & ;
              using const_reference = const value_type &;
              using iterator_category = std::random_access_iterator_tag;
              using size_type = std::size_t;
              using difference_type = std::ptrdiff_t;

              const_reference operator*() const noexcept;
              const_pointer operator->() const noexcept { return &(operator*()); }

              bool operator==(const iterator& other);
              bool operator!=(const iterator& other) { return !(*this == other); }

              iterator operator++();

              iterator();
              iterator(const iterator&) = default;
              iterator& operator = (const iterator&) = default;
            private:
              iterator(const view<Components...>* owner,
                       detail::entity_to_component::iterator itr, bool advance);

            private:
              const view<Components...>* m_owner;
              detail::entity_to_component::iterator m_cursor;
          };
          friend class iterator;
        public:
          iterator begin() const {
            return iterator(this, m_start, true);
          }

          iterator end() const {
            return iterator(this, m_end, false);
          }

        protected:
          detail::components_map_t& m_components;
          std::size_t m_index;
          detail::entity_to_component::iterator m_start;
          detail::entity_to_component::iterator m_end;
        private:
          [[nodiscard]] bool exists_in_all(id_t entity_id) const;
      };

      template <class ... Components>
      view<Components...> query () const;

      template <class ... Components, typename F>
      void for_each (F&& f);

      template <class ... Components>
      void erase();
    private:
      std::pmr::memory_resource* m_allocator;
      detail::components_map_t m_components;
      sparse_set<id_t> m_entities;
      mutable id_factory  m_id_factory;
  };

  // ============================================================================
  // Implementation
  // ============================================================================
  namespace detail {
    class family {
      public:
        template <typename T>
        static id_t type () noexcept {
          static const auto value = identifier ();
          return value;
        }

        static id_t identifier () noexcept {
          static id_t value = 0;
          return value++;
        }

    };

    template <class Component>
    id_t get_component_id () {
      return detail::family::type<Component> ();
    }
  }

  template <typename Component>
  void registry::attach_component (id_t entity_id, Component obj) {
    const auto component_index = detail::get_component_id<Component> ();
    const auto n = m_components.size ();
    if (component_index >= n) {
      ENFORCE(component_index == n);
      m_components.emplace_back (*m_allocator);
    }
    m_components[component_index].insert (entity_id, std::make_tuple (entity_id, obj));
    m_entities.insert (entity_id);
  }

  template <typename Component>
  void registry::detach_component (id_t entity_id) {
    const auto component_index = detail::get_component_id<Component> ();
    ENFORCE(component_index < m_components.size ());
    m_components[component_index].erase (entity_id);
  }

  template <typename Component>
  bool registry::has_component (id_t entity_id) const {
    const auto component_index = detail::get_component_id<Component> ();
    ENFORCE(component_index < m_components.size ());
    return m_components[component_index].exists (entity_id);
  }

  template <typename Component>
  Component& registry::get_component (id_t entity_id) {
    const auto component_index = detail::get_component_id<Component> ();
    ENFORCE(component_index < m_components.size ());
    return std::any_cast<Component&> (std::get<1> (m_components[component_index].get (entity_id)));
  }

  template <typename Component>
  const Component& registry::get_component (id_t entity_id) const {
    const auto component_index = detail::get_component_id<Component> ();
    ENFORCE(component_index < m_components.size ());
    return std::any_cast<const Component&> (std::get<1> (m_components[component_index].get (entity_id)));
  }

  template <class ... Components>
  registry::view<Components...> registry::query () const {
    return view<Components...> (const_cast<detail::components_map_t&>(m_components));
  }

  template <class ... Components, typename F>
  void registry::for_each (F&& f) {
    auto v = query<Components...>();
    for (auto id : v) {
      f.template operator()(id);
    }
  }

  template <class ... Components>
  void registry::erase() {
    for_each<Components...>([this](auto id) {
      this->delete_entity (id);
    });
  }
  // ======================================================================================================

    template <class ... Components>
    registry::view<Components...>::view (detail::components_map_t& components)
        : m_components (components),
          m_index (0) {
      if (m_components.empty ()) {
        return;
      }
      std::size_t smallest = std::numeric_limits<std::size_t>::max ();

      mp::for_types<Components...> ([this, &smallest] (auto type_ptr) {
        using cpt = decltype (type_ptr);
        using t = std::remove_pointer_t<cpt>;
        using pt = std::remove_const_t<t>;

        const auto component_index = detail::get_component_id<pt> ();
        ENFORCE(component_index < m_components.size ());
        if (m_components[component_index].size () < smallest) {
          smallest = m_components[component_index].size ();
          m_index = component_index;
        }
      });
      m_start = m_components[m_index].begin ();
      m_end = m_components[m_index].end ();
    }

    template <class ... Components>
    bool registry::view<Components...>::exists_in_all(id_t entity_id) const {
      bool found = true;
      mp::for_types<Components...> ([this, &found, entity_id] (auto type_ptr) {
        if (found) {
          using cpt = decltype (type_ptr);
          using t = std::remove_pointer_t<cpt>;
          using pt = std::remove_const_t<t>;

          const auto component_index = detail::get_component_id<pt> ();
          ENFORCE(component_index < m_components.size ());
          if (component_index != m_index) {
            if (!m_components[component_index].exists (entity_id)) {
              found = false;
            }
          }
        }
      });
      return found;
    }


    template <class ... Components>
    typename registry::view<Components...>::iterator::const_reference
    registry::view<Components...>::iterator::operator*() const noexcept {
      return std::get<0>(m_cursor.operator*());
    }


    template <class ... Components>
    bool registry::view<Components...>::iterator::operator==(const registry::view<Components...>::iterator& other) {
      return (m_cursor == other.m_cursor);
    }


    template <class ... Components>
    typename registry::view<Components...>::iterator registry::view<Components...>::iterator::operator++() {
      auto curr = m_cursor;
      if (m_cursor != m_owner->m_end) {
        m_cursor++;
      }
      while (m_cursor != m_owner->m_end) {
        auto key = std::get<0>(*m_cursor);
        if (!m_owner->exists_in_all (key)) {
          m_cursor++;
        } else {
          break;
        }
      }
      iterator ret(m_owner, curr, false);
      return ret;
    }

    template <class ... Components>
    registry::view<Components...>::iterator::iterator()
    : m_owner(nullptr),
      m_cursor() {

    }

    template <class ... Components>
    registry::view<Components...>::iterator::iterator(const registry::view<Components...>* owner,
                                                      detail::entity_to_component::iterator itr,
                                                      bool advance)
    : m_owner(owner),
      m_cursor(itr) {

        if (advance) {
          while (m_cursor != m_owner->m_end) {
            auto key = std::get<0> (*m_cursor);
            if (!m_owner->exists_in_all (key)) {
              m_cursor++;
            }
            else {
              break;
            }
          }
        }
    }
}

#endif //INCLUDE_NEUTRINO_KERNEL_ECS_REGISTRY_HH
