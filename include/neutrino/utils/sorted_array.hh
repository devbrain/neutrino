#ifndef NEUTRINO_UTILS_SORTED_ARRAY_HH
#define NEUTRINO_UTILS_SORTED_ARRAY_HH

#include <algorithm>

namespace neutrino::utils {

  template <typename Container>
  struct sorted_array {

    static void sort (Container& vec) {
      std::sort (vec.begin (), vec.end ());
    }

    static auto insert (Container& vec, const typename Container::value_type& value) {
      auto it = std::lower_bound (vec.begin (), vec.end (), value);
      return vec.insert (it, value);
    }

    static bool exists (Container& vec, const typename Container::value_type& value) {
      return std::binary_search (vec.begin (), vec.end (), value);
    }

    static void erase (Container& vec, const typename Container::value_type& value) {
      auto lb = std::lower_bound (std::begin (vec), std::end (vec), value);
      if (lb != std::end (vec) && *lb == value) {
        auto ub = std::upper_bound (lb, std::end (vec), value);
        vec.erase (lb, ub);
      }
    }

    static auto find (Container& vec, const typename Container::value_type& value) {
      auto lb = std::lower_bound (std::begin (vec), std::end (vec), value);
      if (lb != std::end (vec) && *lb == value) {
        return lb;
      }
      return std::end (vec);
    }
  };

}

#endif
