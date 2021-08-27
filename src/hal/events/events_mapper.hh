//
// Created by igor on 12/06/2021.
//

#ifndef NEUTRINO_HAL_EVENTS_EVENTS_MAPPER_HH
#define NEUTRINO_HAL_EVENTS_EVENTS_MAPPER_HH

#include <neutrino/hal/events/events.hh>
#include <hal/sdl/events/system_events.hh>

namespace neutrino::hal::events {
  keyboard map_event (const sdl::events::keyboard &kv);

  pointer map_event (const sdl::events::mouse_button &m);
  pointer map_event (const sdl::events::mouse_motion &m);
  pointer map_event (const sdl::events::mouse_wheel &m);

  pointer map_event (const sdl::events::touch_device_button &m);
  pointer map_event (const sdl::events::touch_device_motion &m);
  pointer map_event (const sdl::events::touch_device_wheel &m);
}

#endif
