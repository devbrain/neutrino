//
// Created by igor on 18/04/2022.
//

#ifndef SRC_KERNEL_HDI_KBD_MAPPER_HH
#define SRC_KERNEL_HDI_KBD_MAPPER_HH

#include <functional>
#include <unordered_map>
#include <string>
#include <string_view>

#include <neutrino/kernel/hdi/events.hh>
#include <neutrino/kernel/hdi/input_config.hh>
#include <neutrino/hal/events/events.hh>

#include "kernel/hdi/kbd_actions.hh"


namespace neutrino {
  class kbd_mapper : public input_config_base {
    public:
      explicit kbd_mapper(events_holder& ev);

      void when_pressed(key_mod_t mod, scan_code_t code, std::string_view event) override;
      void when_pressed(scan_code_t code, std::string_view event) override;

      void when_released(key_mod_t mod, scan_code_t code, std::string_view event) override;
      void when_released(scan_code_t code, std::string_view event) override;

      void handle_event(const hal::events::keyboard& ev);

      // visitor(is_pressed, mod, code, event)
      void visit(const std::function<void(bool, const std::string&, const std::string&, const std::string&)>& visitor) override;
    private:
      events_holder&                                                 m_events;
      std::unordered_map<kbd_actions, std::string, kbd_actions_hash> m_data;
  };
}

#endif //SRC_KERNEL_HDI_KBD_MAPPER_HH
