//
// Created by igor on 26/06/2021.
//

#ifndef NEUTRINO_HAL_WINDOW_HH
#define NEUTRINO_HAL_WINDOW_HH

#include <tuple>
#include <optional>

#include <neutrino/utils/spimpl.h>
#include <neutrino/hal/events/events.hh>
#include <bitflags/bitflags.hpp>
#include <neutrino/neutrino_config.h>

namespace neutrino::hal {

    namespace detail
    {
        class  windows_manager;
        struct window;
    }

    BEGIN_BITFLAGS(window_flags_t)
        FLAG(BORDERLESS)
        FLAG(RESIZABLE)
        FLAG(MINIMIZED)
        FLAG(MAXIMIZED)
        FLAG(FULLSCREEN)
        FLAG(FOCUSED)
    END_BITFLAGS(window_flags_t)

    class surface;
    class renderer;

    class window
    {
        friend class surface;
        friend class renderer;
        friend class detail::windows_manager;
    public:
        enum window_kind_t {
            SIMPLE
#if defined(NEUTRINO_HAS_OPENGL)
            ,
            OPENGL
#endif
#if defined(NEUTRINO_HAS_VULKAN)
            ,
            VULKAN
#endif
        };

        using handle_t = uint32_t;
    public:
        virtual ~window();

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

        [[nodiscard]] handle_t id () const noexcept;
    protected:
        virtual void after_window_opened();

        virtual void on_window_close();
        virtual void on_window_resized(int w, int h);
        virtual void on_input_focus_changed(bool keyboard_focus, bool mouse_focus);
        virtual void on_visibility_change(bool is_visible);

        virtual void on_keyboard_input(const events::keyboard& ev);
        virtual void on_pointer_input(const events::pointer& ev);

        void quit();

        virtual void on_window_invalidate();
    protected:
        explicit window(window_kind_t kind);
        window(window_kind_t kind, window_flags_t flags);
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
    protected:
        spimpl::unique_impl_ptr<detail::window> m_pimpl;
    };

    // =====================================================================================
    class window_2d : public window {
    public:
        window_2d();
        explicit window_2d(window_flags_t flags);

        [[nodiscard]] renderer get_renderer() const;
    private:
        void clear() override;
        void present() override;
    };

#if defined(NEUTRINO_HAS_OPENGL)
    class window_opengl : public window {
    public:
        window_opengl();
        explicit window_opengl(window_flags_t flags);
    private:
        void present() override;
    };
#endif

#if defined(NEUTRINO_HAS_VULKAN)
    class window_vulkan : public window {
    public:
        window_vulkan();
        explicit window_vulkan(window_flags_t flags);
    };
#endif
}

#endif
