//
// Created by igor on 18/04/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_EVENTS_HH
#define INCLUDE_NEUTRINO_KERNEL_EVENTS_HH

#include <string_view>
#include <optional>

#include <neutrino/hal/events/events.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino {
  using key_mod_t = hal::events::key_mod_t;
  using scan_code_t = hal::events::scan_code_t;
  using pointer_button_t = hal::events::pointer_button_t;
  using pointer_state_t = hal::events::pointer_state_t;

  struct mouse_data {
    pointer_button_t button;
    pointer_state_t  state;
    int x;
    int y;
  };

  using mouse_t = std::optional<mouse_data>;

  class kbd_mapper;
  class pointer_mapper;
  class application;
  class events_holder {
      friend class kbd_mapper;
      friend class application;
      friend class pointer_mapper;
    public:
      events_holder ();
      ~events_holder ();

      [[nodiscard]] bool operator [] (std::string_view x) const;
      [[nodiscard]] mouse_t operator [] (pointer_state_t x) const;
      [[nodiscard]] mouse_t operator [] (pointer_button_t x) const;


      [[nodiscard]] bool pressed (scan_code_t btn) const;
      [[nodiscard]] bool pressed (key_mod_t mod, scan_code_t btn) const;
      [[nodiscard]] mouse_t pressed (pointer_button_t btn) const;

      [[nodiscard]] mouse_t moved (pointer_button_t btn) const;
      [[nodiscard]] mouse_t moved () const;

      [[nodiscard]] bool released (scan_code_t btn) const;
      [[nodiscard]] bool released (key_mod_t mod, scan_code_t btn) const;
      [[nodiscard]] mouse_t released (pointer_button_t btn) const;

    private:
      void reset();

      void set(std::string_view x);
      void set(bool is_pressed, key_mod_t mod, scan_code_t btn);
      void set(const hal::events::pointer& ev);
    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_EVENTS_HH
