#ifndef NEUTRINO_TILED_COLOR_HH
#define NEUTRINO_TILED_COLOR_HH

#include <type_traits>
#include <tuple>
#include <cstdint>
#include <string>
#include <neutrino/utils/strings/number_parser.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx {
  namespace detail {
    template <typename T>
    struct rgb_hex_extractor {
      static T parse (const std::string& color, std::string::size_type idx) {
        unsigned x;
        if (!utils::number_parser::try_parse_hex (color.substr (idx, 2), x)) {
          RAISE_EX("Failed to parse ", color.substr (idx, 2), " as hex number");
        }
        return static_cast<T>(x);
      }

      static constexpr T unit () noexcept {
        return 255;
      }

      static constexpr T zero () noexcept {
        return 0;
      }
    };

    template <>
    struct rgb_hex_extractor<float> {
      static float parse (const std::string& color, std::string::size_type idx) {
        unsigned x;
        if (!utils::number_parser::try_parse_hex (color.substr (idx, 2), x)) {
          RAISE_EX("Failed to parse ", color.substr (idx, 2), " as hex number");
        }
        return (float) x / 255.0f;
      }

      static constexpr float unit () noexcept {
        return 1.0f;
      }

      static constexpr float zero () noexcept {
        return 0.0f;
      }
    };

    template <typename T>
    std::tuple<T, T, T, T> parse_hex_color (const std::string& color) {
      if (color.size () == 9) {
        return std::make_tuple (
            rgb_hex_extractor<T>::parse (color, 3),
            rgb_hex_extractor<T>::parse (color, 5),
            rgb_hex_extractor<T>::parse (color, 7),
            rgb_hex_extractor<T>::parse (color, 1));
      }
      else if (color.size () == 7) {
        return std::make_tuple (
            rgb_hex_extractor<T>::parse (color, 1),
            rgb_hex_extractor<T>::parse (color, 3),
            rgb_hex_extractor<T>::parse (color, 5),
            rgb_hex_extractor<T>::unit ());
      }
      RAISE_EX("should not be here");
    }
  } // ns detail

  template <typename T>
  struct color {
    public:
      /*!
       * Parses color from Tiled's own color format, which is #aarrggbb in hex format or optionally #rrggbb.
       * @param color Color in "#rrggbbaa" hex format.
       * @example "#ffaa07ff" and "#aa07ff". In cases where alpha is not a value, it is set to 255.
      */
      explicit color (const std::string& color);
      color (T red, T green, T blue, T alpha);
      color ();

      bool operator == (const color& rhs) const;
      bool operator == (const std::string& rhs) const;
      bool operator != (const color& rhs) const;

      color<float> as_float ();
      color<uint8_t> as_int ();

      /*! Red */
      T r;
      /*! Green */
      T g;
      /*! Blue */
      T b;
      /*! Alpha */
      T a;
  };

  using colorf = color<float>;
  using colori = color<uint8_t>;
} // ns nucleus::tiled
// ===============================================================================
// Implementation
// ===============================================================================
namespace neutrino::tiled::tmx {
  template <typename T>
  color<T>::color (const std::string& color) {
    std::tie (r, g, b, a) = detail::parse_hex_color<T> (color);
  }

  // ---------------------------------------------------------------------------
  template <typename T>
  color<T>::color (T red, T green, T blue, T alpha)
      : r (red), g (green), b (blue), a (alpha) {

  }

  // ---------------------------------------------------------------------------
  template <typename T>
  color<T>::color ()
      : r (detail::rgb_hex_extractor<T>::zero ()),
        g (detail::rgb_hex_extractor<T>::zero ()),
        b (detail::rgb_hex_extractor<T>::zero ()),
        a (detail::rgb_hex_extractor<T>::unit ()) {

  }

  // ---------------------------------------------------------------------------
  template <typename T>
  bool color<T>::operator == (const color& rhs) const {
    return r == rhs.r &&
           g == rhs.g &&
           b == rhs.b &&
           a == rhs.a;
  }

  // ---------------------------------------------------------------------------
  template <typename T>
  bool color<T>::operator == (const std::string& rhs) const {
    return *this == color<T> (rhs);
  }

  // ---------------------------------------------------------------------------
  template <typename T>
  bool color<T>::operator != (const color& rhs) const {
    return !(*this == rhs);
  }

  // ---------------------------------------------------------------------------
  template <typename T>
  color<float> color<T>::as_float () {
    if constexpr (std::is_same<T, float>::value) {
      return *this;
    }
    else {
      return color<float> ((float) r / 255, (float) g / 255, (float) b / 255, (float) a / 255);
    }
  }

  // ---------------------------------------------------------------------------
  template <typename T>
  color<uint8_t> color<T>::as_int () {
    if constexpr (std::is_same<T, float>::value)
      return color<uint8_t> ((float) r * 255, (float) g * 255, (float) b * 255, (float) a * 255);
    else
      return *this;
  }
}

#endif
