//
// Created by igor on 09/06/2021.
//

#ifndef NEUTRINO_HAL_MESSAGE_BROKER_HH
#define NEUTRINO_HAL_MESSAGE_BROKER_HH

#include <neutrino/hal/events/events_broker.hh>
#include <hal/sdl/events/system_events.hh>
#include <neutrino/utils/observer.hh>

namespace neutrino::hal {
  class message_broker : public events_broker, public utils::observer<sdl::events::user> {
    private:
      void on_event (const sdl::events::user &ev);
  };
}

#endif
