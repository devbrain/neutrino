//
// Created by igor on 18/04/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_EVENTS_HH
#define INCLUDE_NEUTRINO_KERNEL_EVENTS_HH

#include <neutrino/hal/events/events.hh>
#include <neutrino/utils/spimpl.h>

#include <string_view>

namespace neutrino {
  using key_mod_t = hal::events::key_mod_t;
  using scan_code_t = hal::events::scan_code_t;

  class kbd_mapper;
  class application;
  class events_holder {
      friend class kbd_mapper;
      friend class application;
    public:
      events_holder ();
      ~events_holder ();

      [[nodiscard]] bool operator [] (std::string_view x) const;

      [[nodiscard]] bool pressed (scan_code_t btn) const;
      [[nodiscard]] bool pressed (key_mod_t mod, scan_code_t btn) const;

      [[nodiscard]] bool released (scan_code_t btn) const;
      [[nodiscard]] bool released (key_mod_t mod, scan_code_t btn) const;
    private:
      void reset();

      void set(std::string_view x);
      void set(bool is_pressed, key_mod_t mod, scan_code_t btn);
    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_EVENTS_HH
