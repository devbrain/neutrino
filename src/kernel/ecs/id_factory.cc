//
// Created by igor on 02/05/2022.
//

#include <neutrino/kernel/ecs/id_factory.hh>

namespace neutrino::ecs {
  id_factory::id_factory()
  : m_top(1) {

  }

  id_t id_factory::next() {
    id_t ret = m_top;
    if (!m_queue.empty()) {
      ret = m_queue.top();
      m_queue.pop();
    } else {
      m_top++;
    }
    return ret;
  }

  void id_factory::clear() {
    m_top = 1;
    queue_t empty;
    std::swap (m_queue, empty);
  }

  void id_factory::release(id_t v) {
    m_queue.push (v);
  }
}
