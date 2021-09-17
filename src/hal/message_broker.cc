//
// Created by igor on 09/06/2021.
//

#include "message_broker.hh"
#include <algorithm>

namespace neutrino::hal {
  void message_broker::on_event (const sdl::events::user& ev) {
    detail::event_handler_holder key{static_cast<uint32_t>(ev.code)};
    auto itr = utils::sorted_array<container_t>::find (m_table, key);
    if (itr != m_table.end ()) {
      itr->value->handle (ev.data1);
    }
    destructor_t destructor = reinterpret_cast<destructor_t>(ev.data2);
    destructor (ev.data1);
  }
}