//
// Created by igor on 02/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_ECS_ID_FACTORY_HH
#define INCLUDE_NEUTRINO_KERNEL_ECS_ID_FACTORY_HH

#include <queue>
#include <vector>
#include <functional>

namespace neutrino::ecs {
  using id_t = std::size_t;

  class id_factory {
    public:
      id_factory();

      id_t next();
      void clear();
      void release(id_t v);
    private:
      using queue_t = std::priority_queue<id_t, std::vector<id_t>, std::greater<>>;
    private:
      id_t    m_top;
      queue_t m_queue;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_ECS_ID_FACTORY_HH
