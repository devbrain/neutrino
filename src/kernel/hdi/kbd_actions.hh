//
// Created by igor on 18/04/2022.
//

#ifndef SRC_KERNEL_HDI_KBD_ACTIONS_HH
#define SRC_KERNEL_HDI_KBD_ACTIONS_HH

#include <cstddef>
#include <neutrino/kernel/hdi/events.hh>
#include <neutrino/hal/events/events.hh>

namespace neutrino {
  struct kbd_actions {
    explicit kbd_actions(const hal::events::keyboard& ev);
    kbd_actions(bool _pressed, key_mod_t _mod, scan_code_t _scan_code);

    bool pressed;
    key_mod_t mod;
    scan_code_t scan_code;
  };

  inline
  bool operator == (const kbd_actions& a, const kbd_actions& b) {
    return a.pressed == b.pressed && a.scan_code == b.scan_code && a.mod == b.mod;
  }

  struct kbd_actions_hash {
    std::size_t operator()(const kbd_actions& a) const;
  };
}

#endif //SRC_KERNEL_HDI_KBD_ACTIONS_HH
