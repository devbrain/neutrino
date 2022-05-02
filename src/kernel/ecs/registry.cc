//
// Created by igor on 01/05/2022.
//

#include <neutrino/kernel/ecs/registry.hh>

namespace neutrino::ecs {

  registry::registry()
  : m_allocator(std::pmr::new_delete_resource()),
    m_entities (*m_allocator)
  {
  }

  registry::registry(std::pmr::memory_resource& mr)
      : m_allocator(&mr),
        m_entities (*m_allocator)
  {
  }

  id_t registry::create_id() const {
    return m_id_factory.next();
  }

  void registry::delete_entity(id_t entity_id) {
    for (auto& component : m_components) {
      component.erase (entity_id);
    }
    m_entities.erase (entity_id);
    m_id_factory.release (entity_id);
  }

  bool registry::exists(id_t entity_id) const {
    return m_entities.exists (entity_id);
  }

  void registry::clear() {
    m_components.clear();
    m_id_factory.clear();
    m_entities.clear();
  }
}
