//
// Created by igor on 08/06/2021.
//
#include <iostream>
#include <neutrino/sdl/window.hh>

#include <neutrino/engine/windows/basic_window.hh>
#include "engine/windows_manager.hh"

namespace neutrino::engine
{
    struct basic_window::impl
    {
        sdl::window::flags_t kind_flag;
        std::optional<window_flags_t> flags;

        int width;
        int height;

        int pos_x;
        int pos_y;

        int before_fullscreen_x;
        int before_fullscreen_y;

        bool windowed;
        bool visible;

        bool mouse_focus;
        bool keyboard_focus;

        sdl::window sdl_window;

        template<typename Flag>
        sdl::window::flags_t get_flag(Flag f, sdl::window::flags_t x)
        {
            if (!flags)
            {
                return sdl::window::flags_t::NONE;
            }
            if (*flags & f)
            {
                return x;
            }
            return sdl::window::flags_t::NONE;
        }
    };

    // ---------------------------------------------------------------------------------------------------
    basic_window::~basic_window() = default;
    // ---------------------------------------------------------------------------------------------------
    basic_window::basic_window()
            : m_pimpl(new impl)
    {
        init(SIMPLE, std::nullopt);
    }
    // ---------------------------------------------------------------------------------------------------
    basic_window::basic_window(window_flags_t flags)
            : m_pimpl(new impl)
    {
        init(SIMPLE, flags);
    }
    // ---------------------------------------------------------------------------------------------------
    basic_window::basic_window(window_kind_t kind, window_flags_t flags)
            : m_pimpl(new impl)
    {
        init(kind, flags);
    }
    // ---------------------------------------------------------------------------------------------------
    void basic_window::init(window_kind_t kind, std::optional<window_flags_t> flags)
    {
        switch (kind)
        {
            case SIMPLE:
                m_pimpl->kind_flag = sdl::window::flags_t::NONE;
                break;
            case OPENGL:
                m_pimpl->kind_flag = sdl::window::flags_t::OPENGL;
                break;
            case VULKAN:
                m_pimpl->kind_flag = sdl::window::flags_t::VULKAN;
                break;
        }

        m_pimpl->flags = flags;
        if (flags)
        {

            m_pimpl->windowed = !(*flags & window_flags_t::FULLSCREEN);
            m_pimpl->visible = !(*flags & window_flags_t::MINIMIZED);
        } else
        {

        }
        m_pimpl->mouse_focus = true;
        m_pimpl->keyboard_focus = true;
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::open(int w, int h)
    {
        open(w, h, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, "");
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::open(int w, int h, int x, int y)
    {
        open(w, h, x, y, "");
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::open(int w, int h, const std::string& title)
    {
        open(w, h, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, title);
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::open(int w, int h, int x, int y, const std::string& title)
    {
        m_pimpl->sdl_window = sdl::window(title, x, y, w, h,
                                          m_pimpl->kind_flag,
                                          m_pimpl->get_flag(window_flags_t::BORDERLESS,
                                                            sdl::window::flags_t::BORDERLESS),
                                          m_pimpl->get_flag(window_flags_t::RESIZABLE, sdl::window::flags_t::RESIZABLE),
                                          m_pimpl->get_flag(window_flags_t::MINIMIZED, sdl::window::flags_t::MINIMIZED),
                                          m_pimpl->get_flag(window_flags_t::MAXIMIZED, sdl::window::flags_t::MAXIMIZED),
                                          m_pimpl->get_flag(window_flags_t::FULLSCREEN,
                                                            sdl::window::flags_t::FULL_SCREEN_DESKTOP),
                                          m_pimpl->get_flag(window_flags_t::FOCUSED, sdl::window::flags_t::INPUT_FOCUS),
                                          m_pimpl->get_flag(window_flags_t::FOCUSED, sdl::window::flags_t::MOUSE_FOCUS),
                                          m_pimpl->get_flag(window_flags_t::FOCUSED,
                                                            sdl::window::flags_t::INPUT_GRABBED)
        );

    //    std::tie(m_pimpl->pos_x, m_pimpl->pos_y) = m_pimpl->sdl_window.position();
        m_pimpl->width = w;
        m_pimpl->height = h;
        windows_manager::instance().attach(m_pimpl->sdl_window, this);

        after_window_opened(m_pimpl->sdl_window.id());
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::after_window_opened([[maybe_unused]] uint32_t window_id) {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::present()
    {
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::focus() noexcept
    {
        if (m_pimpl->visible)
        {
            m_pimpl->sdl_window.show();
        }
        m_pimpl->sdl_window.raise();
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::toggle_fullscreen()
    {

        m_pimpl->windowed = !m_pimpl->windowed;
        int screen_w, screen_h;
        if (m_pimpl->windowed)
        {
            screen_w = m_pimpl->width;
            screen_h = m_pimpl->height;
            SDL_SetWindowFullscreen(m_pimpl->sdl_window.handle(), 0);
            m_pimpl->sdl_window.position(m_pimpl->before_fullscreen_x, m_pimpl->before_fullscreen_y);
            SDL_ShowCursor(1);
        } else
        {
            int i = m_pimpl->sdl_window.display_index();
            SDL_Rect j;
            SDL_GetDisplayBounds(i, &j);
            screen_w = j.w;
            screen_h = j.h;
            std::tie(m_pimpl->before_fullscreen_x, m_pimpl->before_fullscreen_y) = m_pimpl->sdl_window.position();
            SDL_SetWindowFullscreen(m_pimpl->sdl_window.handle(), SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
        on_window_resized(screen_w, screen_h);
    }
    // ----------------------------------------------------------------------------------------------------
    std::string basic_window::title() const {
        return m_pimpl->sdl_window.title();
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::title(const std::string& v) {
        m_pimpl->sdl_window.title(v);
    }
    // ----------------------------------------------------------------------------------------------------
    uint32_t basic_window::id () const noexcept {
        return m_pimpl->sdl_window.id();
    }
    // ----------------------------------------------------------------------------------------------------
    std::tuple<int, int> basic_window::dimensions() const noexcept
    {
        return m_pimpl->sdl_window.size();
    }
    // ----------------------------------------------------------------------------------------------------
    std::tuple<int, int> basic_window::position() const noexcept
    {
        return m_pimpl->sdl_window.position();
    }
    // ----------------------------------------------------------------------------------------------------
    bool basic_window::has_mouse_focus() const noexcept
    {
        return m_pimpl->mouse_focus;
    }
    // ----------------------------------------------------------------------------------------------------
    bool basic_window::has_keyboard_focus() const noexcept
    {
        return m_pimpl->keyboard_focus;
    }
    // ----------------------------------------------------------------------------------------------------
    bool basic_window::visible() const noexcept
    {
        return m_pimpl->visible;
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_mouse_entered()
    {
        m_pimpl->mouse_focus = true;
        this->on_input_focus_changed(m_pimpl->keyboard_focus, m_pimpl->mouse_focus);
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_mouse_leaved()
    {
        m_pimpl->mouse_focus = false;
        this->on_input_focus_changed(m_pimpl->keyboard_focus, m_pimpl->mouse_focus);
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_focus_gained()
    {
        m_pimpl->keyboard_focus = true;
        this->on_input_focus_changed(m_pimpl->keyboard_focus, m_pimpl->mouse_focus);
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_focus_lost()
    {
        m_pimpl->keyboard_focus = false;
        this->on_input_focus_changed(m_pimpl->keyboard_focus, m_pimpl->mouse_focus);
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_shown()
    {
        m_pimpl->visible = true;
        on_visibility_change(m_pimpl->visible);
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_hidden()
    {
        m_pimpl->visible = false;
        on_visibility_change(m_pimpl->visible);
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_exposed()
    {
        present();
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_minimized()
    {
        m_pimpl->visible = false;
        on_visibility_change(m_pimpl->visible);
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_maximized()
    {
        if (!m_pimpl->visible)
        {
            m_pimpl->visible = true;
            on_visibility_change(m_pimpl->visible);
        }
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_moved(int x, int y)
    {
        m_pimpl->pos_x = x;
        m_pimpl->pos_y = y;
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::quit()
    {
        m_pimpl->visible = false;
        m_pimpl->sdl_window.hide();
        on_window_close();
        windows_manager::instance().detach(this);
        before_window_destroy();
        std::unique_ptr<impl> empty;
        std::swap(m_pimpl, empty);
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::before_window_destroy() {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_resized(int w, int h)
    {
        m_pimpl->width = w;
        m_pimpl->height = h;
        on_window_resized(w, h);
        present();
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_window_restored()
    {
        if (!m_pimpl->visible)
        {
            m_pimpl->visible = true;
            on_visibility_change(m_pimpl->visible);
        }
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_input_focus_changed([[maybe_unused]] bool keyboard_focus, [[maybe_unused]] bool mouse_focus)
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_visibility_change([[maybe_unused]] bool is_visible)
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_keyboard_input([[maybe_unused]] const sdl::events::keyboard& ev) {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_mouse_button([[maybe_unused]] const sdl::events::mouse_button& ev) {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_mouse_motion([[maybe_unused]] const sdl::events::mouse_motion& ev) {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_mouse_wheel([[maybe_unused]] const sdl::events::mouse_wheel& ev) {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_touch_button([[maybe_unused]] const sdl::events::touch_device_button& ev) {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_touch_motion([[maybe_unused]] const sdl::events::touch_device_motion& ev) {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_touch_wheel([[maybe_unused]] const sdl::events::touch_device_wheel& ev) {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_window_close()
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::clear()
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::render()
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::_render()
    {
        clear();
        render();
        present();
    }
    // ----------------------------------------------------------------------------------------------------
    void basic_window::on_window_resized([[maybe_unused]] int w, [[maybe_unused]]  int h)
    {

    }
}