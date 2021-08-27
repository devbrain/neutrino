//
// Created by igor on 09/06/2021.
//

#ifndef NEUTRINO_WINDOWS_MANAGER_HH
#define NEUTRINO_WINDOWS_MANAGER_HH

#include <hal/sdl/events/system_events.hh>
#include <hal/sdl/window.hh>
#include <neutrino/utils/singleton.hh>
#include <neutrino/utils/observer.hh>
#include <vector>

namespace neutrino::hal {
  class window;

  namespace detail {
    class windows_manager : public utils::observer<sdl::events::window_shown,
                                                   sdl::events::window_hidden,
                                                   sdl::events::window_exposed,
                                                   sdl::events::window_minimized,
                                                   sdl::events::window_maximized,
                                                   sdl::events::window_restored,
                                                   sdl::events::window_mouse_entered,
                                                   sdl::events::window_mouse_leaved,
                                                   sdl::events::window_focus_gained,
                                                   sdl::events::window_focus_lost,
                                                   sdl::events::window_close,
                                                   sdl::events::window_moved,
                                                   sdl::events::window_resized,

                                                   sdl::events::keyboard,
                                                   sdl::events::mouse_button,
                                                   sdl::events::mouse_motion,
                                                   sdl::events::mouse_wheel,
                                                   sdl::events::touch_device_button,
                                                   sdl::events::touch_device_motion,
                                                   sdl::events::touch_device_wheel
    > {
      public:
        windows_manager ();

        [[nodiscard]] bool has_windows () const noexcept;

        void clear ();
        void present ();

        void attach (sdl::window &sdl_window, neutrino::hal::window *engine_window);
        void detach (neutrino::hal::window *engine_window);
      private:
        void on_event (const sdl::events::window_shown &ev) override;
        void on_event (const sdl::events::window_hidden &ev) override;
        void on_event (const sdl::events::window_exposed &ev) override;
        void on_event (const sdl::events::window_minimized &ev) override;
        void on_event (const sdl::events::window_maximized &ev) override;
        void on_event (const sdl::events::window_restored &ev) override;
        void on_event (const sdl::events::window_mouse_entered &ev) override;
        void on_event (const sdl::events::window_mouse_leaved &ev) override;
        void on_event (const sdl::events::window_focus_gained &ev) override;
        void on_event (const sdl::events::window_focus_lost &ev) override;
        void on_event (const sdl::events::window_close &ev) override;
        void on_event (const sdl::events::window_moved &ev) override;
        void on_event (const sdl::events::window_resized &ev) override;

        void on_event (const sdl::events::keyboard &ev) override;
        void on_event (const sdl::events::mouse_button &ev) override;
        void on_event (const sdl::events::mouse_motion &ev) override;
        void on_event (const sdl::events::mouse_wheel &ev) override;
        void on_event (const sdl::events::touch_device_button &ev) override;
        void on_event (const sdl::events::touch_device_motion &ev) override;
        void on_event (const sdl::events::touch_device_wheel &ev) override;
      private:
        std::vector<window *> m_windows;
    };
  } // ns detail
  using windows_manager = utils::singleton<detail::windows_manager>;
}

#endif //NEUTRINO_WINDOWS_MANAGER_HH
