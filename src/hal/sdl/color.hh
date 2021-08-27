//
// Created by igor on 01/06/2020.
//

#ifndef NEUTRINO_SDL_COLOR_HH
#define NEUTRINO_SDL_COLOR_HH

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <cmath>

#include <hal/sdl/sdl2.hh>

namespace neutrino::sdl {
  struct color : public SDL_Color {
    color ();
    color (uint8_t r, uint8_t g, uint8_t b);
    color (uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    explicit color (const SDL_Color &c);
    color &operator= (const SDL_Color &c);

    static color from_hsl (uint8_t h, uint8_t s, uint8_t l);
    static color from_hsv (uint8_t h, uint8_t s, uint8_t v);

    [[nodiscard]] std::tuple<uint8_t, uint8_t, uint8_t> to_hsl () const;
    [[nodiscard]] std::tuple<uint8_t, uint8_t, uint8_t> to_hsv () const;
  };
} // ns sdl

// =========================================================================
// Implementation
// =========================================================================

namespace neutrino::sdl {
  inline color::color ()
      : SDL_Color{0, 0, 0, 0} {

  }
  inline color::color (uint8_t r, uint8_t g, uint8_t b)
      : SDL_Color{r, g, b, 0xFF} {

  }
  inline color::color (uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : SDL_Color{r, g, b, a} {

  }
  inline color::color (const SDL_Color &c)
      : SDL_Color{c.r, c.g, c.b, c.a} {

  }
  inline color &color::operator= (const SDL_Color &c) {
    r = c.r;
    g = c.g;
    b = c.b;
    a = c.a;
    return *this;
  }

  namespace detail {
    static constexpr double EPSILON = 0.001;
  }

  inline color color::from_hsl (uint8_t h_, uint8_t s_, uint8_t l_) {
    double r, g, b, h, s, l; //this function works with floats between 0 and 1
    double temp1, temp2, tempr, tempg, tempb;
    h = h_ / 256.0;
    s = s_ / 256.0;
    l = l_ / 256.0;

    //If saturation is 0, the color is a shade of gray
    if (std::abs (s) < detail::EPSILON) {
      r = g = b = l;
    }
      //If saturation > 0, more complex calculations are needed
    else {
      //Set the temporary values
      if (l < 0.5) {
        temp2 = l * (1 + s);
      }
      else {
        temp2 = (l + s) - (l * s);
      }
      temp1 = 2 * l - temp2;
      tempr = h + 1.0 / 3.0;
      if (tempr > 1) {
        tempr--;
      }
      tempg = h;
      tempb = h - 1.0 / 3.0;
      if (tempb < 0) {
        tempb++;
      }

      //Red
      if (tempr < 1.0 / 6.0) {
        r = temp1 + (temp2 - temp1) * 6.0 * tempr;
      }
      else {
        if (tempr < 0.5) {
          r = temp2;
        }
        else {
          if (tempr < 2.0 / 3.0) {
            r = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempr) * 6.0;
          }
          else {
            r = temp1;
          }
        }
      }

      //Green
      if (tempg < 1.0 / 6.0) {
        g = temp1 + (temp2 - temp1) * 6.0 * tempg;
      }
      else {
        if (tempg < 0.5) {
          g = temp2;
        }
        else {
          if (tempg < 2.0 / 3.0) {
            g = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempg) * 6.0;
          }
          else {
            g = temp1;
          }
        }
      }

      //Blue
      if (tempb < 1.0 / 6.0) {
        b = temp1 + (temp2 - temp1) * 6.0 * tempb;
      }
      else {
        if (tempb < 0.5) {
          b = temp2;
        }
        else {
          if (tempb < 2.0 / 3.0) {
            b = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempb) * 6.0;
          }
          else {
            b = temp1;
          }
        }
      }
    }
    return color (static_cast<uint8_t >(r * 256), static_cast<uint8_t >(g * 256), static_cast<uint8_t >(b * 256));
  }

  inline color color::from_hsv (uint8_t h_, uint8_t s_, uint8_t v_) {
    double r, g, b, h, s, v; //this function works with floats between 0 and 1
    h = h_ / 256.0;
    s = s_ / 256.0;
    v = v_ / 256.0;

    //If saturation is 0, the color is a shade of gray
    if (std::abs (s) < detail::EPSILON) {
      r = g = b = v;
    }
      //If saturation > 0, more complex calculations are needed
    else {
      double f, p, q, t;
      int i;
      h *= 6; //to bring hue to a number between 0 and 6, better for the calculations
      i = int (floor (h));  //e.g. 2.7 becomes 2 and 3.01 becomes 3 or 4.9999 becomes 4
      f = h - i;  //the fractional part of h
      p = v * (1 - s);
      q = v * (1 - (s * f));
      t = v * (1 - (s * (1 - f)));
      switch (i) {
      case 0:r = v;
        g = t;
        b = p;
        break;
      case 1:r = q;
        g = v;
        b = p;
        break;
      case 2:r = p;
        g = v;
        b = t;
        break;
      case 3:r = p;
        g = q;
        b = v;
        break;
      case 4:r = t;
        g = p;
        b = v;
        break;
      default:
        /*case 5:*/ r = v;
        g = p;
        b = q;
        break;

      }
    }
    return color (static_cast<uint8_t >(r * 256), static_cast<uint8_t >(g * 256), static_cast<uint8_t >(b * 256));
  }
  inline
  std::tuple<uint8_t, uint8_t, uint8_t> color::to_hsl () const {
    double fr, fg, fb, h, s, l; //this function works with floats between 0 and 1
    fr = r / 256.0;
    fg = g / 256.0;
    fb = b / 256.0;
    auto maxColor = std::max (fr, std::max (fg, fb));
    auto minColor = std::min (fr, std::min (fg, fb));
    //R == G == B, so it's a shade of gray
    if (std::abs (minColor - maxColor) < detail::EPSILON) {
      h = 0.0; //it doesn't matter what value it has
      s = 0.0;
      l = fr; //doesn't matter if you pick r, g, or b
    }
    else {
      l = (minColor + maxColor) / 2;

      if (l < 0.5) {
        s = (maxColor - minColor) / (maxColor + minColor);
      }
      else {
        s = (maxColor - minColor) / (2.0 - maxColor - minColor);
      }

      if (std::abs (fr - maxColor) < detail::EPSILON) {
        h = (fg - fb) / (maxColor - minColor);
      }
      else {
        if (std::abs (fg - maxColor) < detail::EPSILON) {
          h = 2.0 + (fb - fr) / (maxColor - minColor);
        }
        else {
          h = 4.0 + (fr - fg) / (maxColor - minColor);
        }
      }

      h /= 6; //to bring it to a number between 0 and 1
      if (h < 0) {
        h++;
      }
    }
    return {static_cast<uint8_t>(256 * h), static_cast<uint8_t>(256 * s), static_cast<uint8_t>(256 * l)};
  }

  inline
  std::tuple<uint8_t, uint8_t, uint8_t> color::to_hsv () const {
    double fr, fg, fb, h, s, v; //this function works with floats between 0 and 1
    fr = r / 256.0;
    fg = g / 256.0;
    fb = b / 256.0;
    auto maxColor = std::max (fr, std::max (fg, fb));
    auto minColor = std::min (fr, std::min (fg, fb));
    v = maxColor;
    if (std::abs (maxColor) < detail::EPSILON) //avoid division by zero when the color is black
    {
      s = 0;
    }
    else {
      s = (maxColor - minColor) / maxColor;
    }
    if (std::abs (s) < detail::EPSILON) {
      h = 0; //it doesn't matter what value it has
    }
    else {
      if (std::abs (r - maxColor) < detail::EPSILON) {
        h = (fg - fb) / (maxColor - minColor);
      }
      else {
        if (std::abs (fg - maxColor) < detail::EPSILON) {
          h = 2.0 + (fb - fr) / (maxColor - minColor);
        }
        else {
          h = 4.0 + (fr - fg) / (maxColor - minColor);
        }
      }
      h /= 6.0; //to bring it to a number between 0 and 1
      if (h < 0) {
        h++;
      }
    }
    return {static_cast<uint8_t>(256 * h), static_cast<uint8_t>(256 * s), static_cast<uint8_t>(256 * v)};
  }
}

#endif
