//
// Created by igor on 30/05/2022.
//

#ifndef INCLUDE_NEUTRINO_UTILS_STRINGS_STRING_HASH_HH
#define INCLUDE_NEUTRINO_UTILS_STRINGS_STRING_HASH_HH

#include <cstdint>

namespace neutrino::utils {

    template <typename T> struct fnv1;
    template <typename T> struct fnv1a;

    namespace detail {
      template <typename T> struct fnv_internal;
      template <> struct fnv_internal<uint32_t> {
        constexpr static uint32_t default_offset_basis = 0x811C9DC5;
        constexpr static uint32_t prime = 0x01000193;
      };
    }

  template <> struct fnv1<uint32_t> : public detail::fnv_internal<uint32_t>
  {
    constexpr static inline uint32_t hash(char const* const aString, const uint32_t val = default_offset_basis)
    {
      return (aString[0] == '\0') ? val : hash(&aString[1], (val * prime) ^ uint32_t(aString[0]));
    }

    template <std::size_t N>
    constexpr static inline uint32_t hash(const char(& value)[N], const uint32_t val = default_offset_basis) {
      return hash(&value[0], val);
    }
  };

  template <> struct fnv1a<uint32_t> : public detail::fnv_internal<uint32_t>
  {
    constexpr static inline uint32_t hash(char const* const aString, const uint32_t val = default_offset_basis)
    {
      return (aString[0] == '\0') ? val : hash(&aString[1], (val ^ uint32_t(aString[0])) * prime);
    }

    template <std::size_t N>
    constexpr static inline uint32_t hash(const char(& value)[N], const uint32_t val = default_offset_basis) {
      return hash(&value[0], val);
    }
  };
}

#endif //INCLUDE_NEUTRINO_UTILS_STRINGS_STRING_HASH_HH
