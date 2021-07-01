//
// Created by igor on 26/06/2021.
//

#include <neutrino/hal/video/window.hh>
#include "windows_manager.hh"
#include "window_impl.hh"

namespace neutrino::hal {
    window::~window() = default;
    // ---------------------------------------------------------------------------------------------------
    window::window()
            : m_pimpl(spimpl::make_unique_impl<detail::window>())
    {
        init(SIMPLE, std::nullopt);
    }
    // ---------------------------------------------------------------------------------------------------
    window::window(window_flags_t flags)
            : m_pimpl(spimpl::make_unique_impl<detail::window>())
    {
        init(SIMPLE, flags);
    }
    // ---------------------------------------------------------------------------------------------------
    window::window(window_kind_t kind, window_flags_t flags)
            : m_pimpl(spimpl::make_unique_impl<detail::window>())
    {
        init(kind, flags);
    }
    // ---------------------------------------------------------------------------------------------------
    void window::init(window_kind_t kind, std::optional<window_flags_t> flags)
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
    void window::open(int w, int h)
    {
        open(w, h, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, "");
    }
    // ----------------------------------------------------------------------------------------------------
    void window::open(int w, int h, int x, int y)
    {
        open(w, h, x, y, "");
    }
    // ----------------------------------------------------------------------------------------------------
    void window::open(int w, int h, const std::string& title)
    {
        open(w, h, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, title);
    }
    // ----------------------------------------------------------------------------------------------------
    void window::open(int w, int h, int x, int y, const std::string& title)
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

        m_pimpl->width = w;
        m_pimpl->height = h;

        windows_manager::instance().attach(m_pimpl->sdl_window, this);

        after_window_opened(m_pimpl->sdl_window.id());
    }
    // ----------------------------------------------------------------------------------------------------
    void window::after_window_opened([[maybe_unused]] uint32_t window_id) {

    }
    // ----------------------------------------------------------------------------------------------------
    void window::present()
    {
    }
    // ----------------------------------------------------------------------------------------------------
    void window::focus() noexcept
    {
        if (m_pimpl->visible)
        {
            m_pimpl->sdl_window.show();
        }
        m_pimpl->sdl_window.raise();
    }
    // ----------------------------------------------------------------------------------------------------
    void window::toggle_fullscreen()
    {

        m_pimpl->windowed = !m_pimpl->windowed;
        if (m_pimpl->windowed)
        {
            SDL_SetWindowFullscreen(m_pimpl->sdl_window.handle(), 0);
            m_pimpl->sdl_window.position(m_pimpl->before_fullscreen_x, m_pimpl->before_fullscreen_y);
            SDL_ShowCursor(1);
        } else
        {
            std::tie(m_pimpl->before_fullscreen_x, m_pimpl->before_fullscreen_y) = m_pimpl->sdl_window.position();
            SDL_SetWindowFullscreen(m_pimpl->sdl_window.handle(), SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
    }
    // ----------------------------------------------------------------------------------------------------
    std::string window::title() const {
        return m_pimpl->sdl_window.title();
    }
    // ----------------------------------------------------------------------------------------------------
    void window::title(const std::string& v) {
        m_pimpl->sdl_window.title(v);
    }
    // ----------------------------------------------------------------------------------------------------
    uint32_t window::id () const noexcept {
        return m_pimpl->sdl_window.id();
    }
    // ----------------------------------------------------------------------------------------------------
    std::tuple<int, int> window::dimensions() const noexcept
    {
        return m_pimpl->sdl_window.size();
    }
    // ----------------------------------------------------------------------------------------------------
    std::tuple<int, int> window::position() const noexcept
    {
        return m_pimpl->sdl_window.position();
    }
    // ----------------------------------------------------------------------------------------------------
    bool window::has_mouse_focus() const noexcept
    {
        return m_pimpl->mouse_focus;
    }
    // ----------------------------------------------------------------------------------------------------
    bool window::has_keyboard_focus() const noexcept
    {
        return m_pimpl->keyboard_focus;
    }
    // ----------------------------------------------------------------------------------------------------
    bool window::visible() const noexcept
    {
        return m_pimpl->visible;
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_mouse_entered()
    {
        m_pimpl->mouse_focus = true;
        this->on_input_focus_changed(m_pimpl->keyboard_focus, m_pimpl->mouse_focus);
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_mouse_leaved()
    {
        m_pimpl->mouse_focus = false;
        this->on_input_focus_changed(m_pimpl->keyboard_focus, m_pimpl->mouse_focus);
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_focus_gained()
    {
        m_pimpl->keyboard_focus = true;
        this->on_input_focus_changed(m_pimpl->keyboard_focus, m_pimpl->mouse_focus);
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_focus_lost()
    {
        m_pimpl->keyboard_focus = false;
        this->on_input_focus_changed(m_pimpl->keyboard_focus, m_pimpl->mouse_focus);
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_shown()
    {
        m_pimpl->visible = true;
        on_visibility_change(m_pimpl->visible);
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_hidden()
    {
        m_pimpl->visible = false;
        on_visibility_change(m_pimpl->visible);
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_exposed()
    {
        present();
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_minimized()
    {
        m_pimpl->visible = false;
        on_visibility_change(m_pimpl->visible);
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_maximized()
    {
        if (!m_pimpl->visible)
        {
            m_pimpl->visible = true;
            on_visibility_change(m_pimpl->visible);
        }
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_moved(int x, int y)
    {
        m_pimpl->pos_x = x;
        m_pimpl->pos_y = y;
    }
    // ----------------------------------------------------------------------------------------------------
    void window::quit()
    {
        m_pimpl->visible = false;
        m_pimpl->sdl_window.hide();
        on_window_close();
        windows_manager::instance().detach(this);
        before_window_destroy();
        spimpl::unique_impl_ptr<detail::window> empty;
        std::swap(m_pimpl, empty);
    }
    // ----------------------------------------------------------------------------------------------------
    void window::before_window_destroy() {

    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_resized(int w, int h)
    {
        m_pimpl->width = w;
        m_pimpl->height = h;
        on_window_invalidate();
        on_window_resized(w, h);
        present();
    }
    // ----------------------------------------------------------------------------------------------------
    void window::_window_restored()
    {
        if (!m_pimpl->visible)
        {
            m_pimpl->visible = true;
            on_visibility_change(m_pimpl->visible);
        }
    }
    // ----------------------------------------------------------------------------------------------------
    void window::on_input_focus_changed([[maybe_unused]] bool keyboard_focus, [[maybe_unused]] bool mouse_focus)
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void window::on_visibility_change([[maybe_unused]] bool is_visible)
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void window::on_keyboard_input([[maybe_unused]] const events::keyboard& ev) {

    }
    // ----------------------------------------------------------------------------------------------------
    void window::on_pointer_input([[maybe_unused]]const events::pointer& ev){

    }
    // ----------------------------------------------------------------------------------------------------
    void window::on_window_close()
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void window::on_window_invalidate()
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void window::clear()
    {

    }
    // ----------------------------------------------------------------------------------------------------
    void window::on_window_resized([[maybe_unused]] int w, [[maybe_unused]]  int h)
    {

    }

}
