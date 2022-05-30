//
// Created by igor on 29/05/2022.
//

#ifndef INCLUDE_NEUTRINO_UTILS_ID_HH
#define INCLUDE_NEUTRINO_UTILS_ID_HH
#include <cassert>
#include <cstdint>
#include <cstdio>

#include <iostream>
#include <map>
#include <string>
#include <typeinfo>
// https://github.com/r-lyeh-archived/id/blob/master/id.hpp

namespace neutrino::utils::details {
/* private API */
  class id {
      static bool check_for_collisions (size_t hash, const char* str) {
        static std::map<size_t, std::string> map;
        auto found = map.find (hash);
        if (found != map.end ()) {
          if (found->second != str) {
            std::cerr << "<id/id.hpp> says: error! '"
                      << map[hash] << "' and '" << str << "' hashes collide!" << std::endl;
            return false;
          }
        }
        else {
          map.insert (std::pair<size_t, std::string> (hash, str));
        }
        return true;
      }

      template <unsigned int N, unsigned int I>
      struct fnv1a {
        constexpr static unsigned int hash (const char (& str)[N]) {
          return (id::fnv1a<N, I - 1>::hash (str) ^ str[I - 1]) * 16777619u;
        }
      };

      template <unsigned int N>
      struct fnv1a<N, 1> {
        constexpr static unsigned int hash (const char (& str)[N]) {
          return (2166136261u ^ str[0]) * 16777619u;
        }
      };

    public:

      // compile-time hashed IDs have MSB flag enabled
      template <unsigned int N>
      constexpr static unsigned int gen (const char (& str)[N]) {
        return assert(check_for_collisions (id::fnv1a<N, N>::hash (str) | 0x80000000, str)), id::fnv1a<N, N>::hash (str)
                                                                                             | 0x80000000;
      }

      // sequential IDs have MSB flag disabled
      static unsigned int& gen () {
        static unsigned int seq = 0;
#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsequence-point"
#endif
        return seq = (++seq) & 0x7FFFFFFFF, seq;
#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
      }
  };
}

namespace neutrino::utils {
  template <std::size_t N>
  inline
  unsigned int id(const char(& value)[N]) {
    return details::id::gen (value);
  }
}

#endif //INCLUDE_NEUTRINO_UTILS_ID_HH
