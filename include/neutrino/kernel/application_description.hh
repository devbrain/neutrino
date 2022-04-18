//
// Created by igor on 18/04/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_APPLICATION_DESCRIPTION_HH
#define INCLUDE_NEUTRINO_KERNEL_APPLICATION_DESCRIPTION_HH
#include <optional>
#include <string>
#include <neutrino/math/point.hh>

namespace neutrino {
  class main_window_description {
    public:
      main_window_description (unsigned w, unsigned h);

      [[nodiscard]] unsigned width () const noexcept;
      [[nodiscard]] unsigned height () const noexcept;

      [[nodiscard]] std::string title () const noexcept;
      void title (std::string ttl);

      [[nodiscard]] std::optional<math::point2d> position () const noexcept;
      void position (int x, int y);

      void resizable (bool f);
      [[nodiscard]] bool resizable () const noexcept;

      void fullscreen (bool f);
      [[nodiscard]] bool fullscreen () const noexcept;

    private:
      unsigned m_width;
      unsigned m_height;
      std::string m_title;
      std::optional<math::point2d> m_pos;

      bool m_resizable;
      bool m_fullscreen;
  };

  class application_description {
    public:
      explicit application_description (main_window_description mw);
      application_description (main_window_description mw, unsigned fps);

      [[nodiscard]] main_window_description get_main_window_description () const noexcept;
      [[nodiscard]] unsigned fps () const noexcept;
    private:
      main_window_description m_main_window_descr;
      unsigned m_fps;
  };
}
#endif //INCLUDE_NEUTRINO_KERNEL_APPLICATION_DESCRIPTION_HH
