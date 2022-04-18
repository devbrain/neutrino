//
// Created by igor on 18/04/2022.
//

#include "kbd_actions.hh"
#include <functional>

namespace neutrino {
  kbd_actions::kbd_actions(const hal::events::keyboard& ev)
  : pressed(ev.pressed), mod(ev.mod), scan_code(ev.code) {}

  kbd_actions::kbd_actions(bool _pressed, key_mod_t _mod, scan_code_t _scan_code)
  : pressed(_pressed), mod(_mod), scan_code(_scan_code) {}


  template <class T>
  inline void hash_combine(std::size_t& seed, const T& v)
  {
      std::hash<T> hasher;
      seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

  std::size_t kbd_actions_hash::operator()(const kbd_actions& a) const {
    std::size_t seed = 0;
    hash_combine (seed, a.pressed);
    hash_combine (seed, a.mod.bits());
    hash_combine (seed, a.scan_code);
    return seed;
  }
}