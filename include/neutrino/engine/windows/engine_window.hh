//
// Created by igor on 13/06/2021.
//

#ifndef NEUTRINO_ENGINE_WINDOWS_ENGINE_WINDOW_HH
#define NEUTRINO_ENGINE_WINDOWS_ENGINE_WINDOW_HH

#include <neutrino/engine/observer.hh>
#include <neutrino/engine/events/events.hh>
#include <neutrino/engine/windows/basic_window.hh>

namespace neutrino::engine {
    class engine_window : public basic_window, public publisher<events::all_events_t> {
    public:
        engine_window();
        explicit engine_window(window_flags_t flags);
    protected:
        engine_window(window_kind_t kind, window_flags_t flags);
    protected:
         virtual void present() = 0;
         virtual void render() = 0;
    private:
         void on_window_close() override;

         void on_window_resized(int w, int h) override;
         void on_input_focus_changed(bool keyboard_focus, bool mouse_focus) override;
         void on_visibility_change(bool is_visible) override;

         void on_keyboard_input(const sdl::events::keyboard& ev) override;
         void on_mouse_button(const sdl::events::mouse_button& ev) override;
         void on_mouse_motion(const sdl::events::mouse_motion& ev) override;
         void on_mouse_wheel(const sdl::events::mouse_wheel& ev) override;
         void on_touch_button(const sdl::events::touch_device_button& ev) override;
         void on_touch_motion(const sdl::events::touch_device_motion& ev) override;
         void on_touch_wheel(const sdl::events::touch_device_wheel& ev) override;
    };
}

#endif
