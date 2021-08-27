//
// Created by igor on 26/07/2021.
//

#ifndef NEUTRINO_SWITCH_BY_STRING_HH
#define NEUTRINO_SWITCH_BY_STRING_HH

// https://learnmoderncpp.com/2020/06/01/strings-as-switch-case-labels/

constexpr inline auto switcher (const char *s) {
  unsigned long long hash{}, c{};
  for (auto p = s; *p; ++p, ++c) {
    hash += *p << c;
  }
  return hash;
}

constexpr inline auto operator "" _case (const char *s, size_t) {
  return switcher (s);
}

#endif //NEUTRINO_SWITCH_BY_STRING_HH
