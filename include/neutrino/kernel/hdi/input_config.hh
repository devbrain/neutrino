//
// Created by igor on 18/04/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_HDI_INPUT_CONFIG_HH
#define INCLUDE_NEUTRINO_KERNEL_HDI_INPUT_CONFIG_HH

#include <string_view>
#include <string>
#include <functional>

#include <neutrino/kernel/hdi/events.hh>

namespace neutrino {
  class input_config_base {
    public:
      virtual ~input_config_base();

      virtual void when_pressed (key_mod_t mod, scan_code_t code, std::string_view event) = 0;
      virtual void when_pressed (scan_code_t code, std::string_view event) = 0;

      virtual void when_released (key_mod_t mod, scan_code_t code, std::string_view event) = 0;
      virtual void when_released (scan_code_t code, std::string_view event) = 0;

      // visitor(is_pressed, mod, code, event)
      virtual void visit (const std::function<void (bool, const std::string&, const std::string&, const std::string&)>& visitor) = 0;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_HDI_INPUT_CONFIG_HH
