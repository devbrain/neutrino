//
// Created by igor on 07/06/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_HH
#define INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_HH

#include <cstdint>
#include <vector>

namespace neutrino::assets {
  namespace detail {
    template <bool Is64>
    struct bitmask_traits;

    template <>
    struct bitmask_traits<true> {
      using word_t = uint64_t;
      static constexpr auto bits_in_word = 64;
      static constexpr auto exp = 6;
      static constexpr word_t unit = 1;
    };

    template <>
    struct bitmask_traits<false> {
      using word_t = uint32_t;
      static constexpr auto bits_in_word = 32;
      static constexpr auto exp = 5;
      static constexpr word_t unit = 1;
    };

    using bitmask_traits_t = bitmask_traits<sizeof(void*) == 8>;
    static_assert ((1 << bitmask_traits_t::exp) == bitmask_traits_t::bits_in_word);
  }

  class bitmask {
    public:
      using word_t = detail::bitmask_traits_t::word_t;
    public:
      bitmask(int w, int h);

      [[nodiscard]] int width() const noexcept;
      [[nodiscard]] int height() const noexcept;

      void set(int x, int y);
      void clear(int x, int y);
      [[nodiscard]] bool get(int x, int y) const;

      static bool overlap(const bitmask& a, const bitmask& b, int xoffs, int yoffs);
    private:
      int m_w;
      int m_h;
      std::vector<word_t> m_data;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_HH
