//
// Created by igor on 08/06/2021.
//

#ifndef NEUTRINO_ENGINE_WINDOWS_BASIC_WINDOW_HH
#define NEUTRINO_ENGINE_WINDOWS_BASIC_WINDOW_HH

#include <tuple>
#include <memory>
#include <string>
#include <optional>
#include <cstdint>
#include <bitflags/bitflags.hpp>

#include <neutrino/sdl/events/system_events.hh>

namespace neutrino::engine
{
    namespace detail
    {
        class windows_manager;
    }

    BEGIN_BITFLAGS(window_flags_t)
        FLAG(BORDERLESS)
        FLAG(RESIZABLE)
        FLAG(MINIMIZED)
        FLAG(MAXIMIZED)
        FLAG(FULLSCREEN)
        FLAG(FOCUSED)
    END_BITFLAGS(window_flags_t)

    class basic_window
    {
        friend class detail::windows_manager;
    public:
        enum window_kind_t {
            SIMPLE,
            OPENGL,
            VULKAN
        };
    public:
        basic_window();
        explicit basic_window(window_flags_t flags);

        virtual ~basic_window();

        void open (int w, int h);
        void open (int w, int h, int x, int y);

        void open (int w, int h, const std::string& title);
        void open (int w, int h, int x, int y, const std::string& title);

        //Focuses on window
        void focus() noexcept;


        [[nodiscard]] std::tuple<int, int> dimensions() const noexcept;
        [[nodiscard]] std::tuple<int, int> position() const noexcept;

        [[nodiscard]] bool has_mouse_focus() const noexcept;
        [[nodiscard]] bool has_keyboard_focus() const noexcept;
        [[nodiscard]] bool visible() const noexcept;
        virtual void clear() = 0;
        virtual void present() = 0;

        void toggle_fullscreen();
        [[nodiscard]] std::string title() const;
        void title(const std::string& v);

        [[nodiscard]] uint32_t id () const noexcept;
    protected:
        virtual void after_window_opened(uint32_t window_id);
        virtual void before_window_destroy();

        virtual void on_window_close();
        virtual void on_window_resized(int w, int h);
        virtual void on_input_focus_changed(bool keyboard_focus, bool mouse_focus);
        virtual void on_visibility_change(bool is_visible);

        virtual void on_keyboard_input(const sdl::events::keyboard& ev);
        virtual void on_mouse_button(const sdl::events::mouse_button& ev);
        virtual void on_mouse_motion(const sdl::events::mouse_motion& ev);
        virtual void on_mouse_wheel(const sdl::events::mouse_wheel& ev);
        virtual void on_touch_button(const sdl::events::touch_device_button& ev);
        virtual void on_touch_motion(const sdl::events::touch_device_motion& ev);
        virtual void on_touch_wheel(const sdl::events::touch_device_wheel& ev);

        void quit();

        virtual void on_window_invalidate();
    protected:
        basic_window(window_kind_t kind, window_flags_t flags);
    private:
        void _window_restored();
        void _window_mouse_entered();
        void _window_mouse_leaved();
        void _window_focus_gained();
        void _window_focus_lost();

        void _window_shown();
        void _window_hidden();
        void _window_exposed();
        void _window_minimized();
        void _window_maximized();
        void _window_moved(int x, int y);

        void _window_resized(int w, int h);

        void init(window_kind_t kind, std::optional<window_flags_t> flags);
    private:
        struct impl;

        std::unique_ptr<impl> m_pimpl;
    };
}

#endif
