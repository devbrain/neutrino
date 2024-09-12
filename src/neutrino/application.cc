//
// Created by igor on 7/13/24.
//

#include <neutrino/application.hh>
#include <neutrino/config/config_hotkey.hh>
#include <bsw/s11n/s11n.hh>
#include <bsw/exception.hh>
#include <bsw/logger/logger.hh>
#include <bsw/register_at_exit.hh>
#include "bsw/logger/system.hh"
#include "imgui/imgui.hh"

namespace neutrino {

    inline constexpr int FULLSCREEN_PRESSED = 1;

    struct application_config {
        application_config() = default;

        int desired_fps{60};
        bool fullscreen{false};
        unsigned screen_width{800};
        unsigned screen_height{600};
        std::optional <config_hotkey> fullscreen_key{config_hotkey(sdl::scancode::RETURN, sdl::keymod::ALT)};

        SERIALIZATION_SCHEMA(desired_fps, fullscreen, screen_height, screen_height, fullscreen_key)
    };

    static application_config global_config;

    static std::tuple <bool, std::string> load_application_config() {
        if (!config_service::has_config_prefix()) {
            config_service::set_config_prefix(config_service::get_executable_name());
        }
        bool create_cfg_dir = false;
        if (!config_service::is_config_root_exists()) {
            create_cfg_dir = true;
        } else {
            if (config_service::is_config_root_writable()) {
                create_cfg_dir = true;
            }
        }

        if (create_cfg_dir) {
            if (!config_service::make_application_config_dirs()) {
                EVLOG_TRACE(EVLOG_ERROR, "Failed to create application config directory");
            }
        }
        EVLOG_TRACE(EVLOG_INFO, "Using ", config_service::get_path_to_configs(), " for configuration storage");
        auto cfg_name = config_service::get_executable_name() + ".yml";
        if (!config_service::file_exists(cfg_name)) {
            EVLOG_TRACE(EVLOG_WARNING, "Application config file ", cfg_name, " does not exist. using defaults");
        } else {
            EVLOG_TRACE(EVLOG_INFO, "Using config file ", cfg_name, " for application config");
        }
        auto [cfg, loaded] = config_service::load <application_config>(cfg_name);
        global_config = cfg;
        return {loaded, cfg_name};
    }

    static application* s_instance = nullptr;

    struct application_state {
        application_state()
            : is_fullscreen(false) {
        }

        bool is_fullscreen;
    };

    application::application()
        : m_quit_flag(false),
          m_desired_fps(60),
          m_fps(0),
          m_window_id(0),
          m_fullscreen(false),
          m_configured(false),
          m_initialized(false) {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        auto [loaded, cfg_name] = load_application_config();
        m_configured = loaded;
        m_config_name = cfg_name;
        m_desired_fps = global_config.desired_fps;
        m_size.w = global_config.screen_width;
        m_size.h = global_config.screen_height;
        m_fullscreen = global_config.fullscreen;
    }

    application::~application() {
        bsw::logger::shutdown();
        bsw::cleanup_registered_at_exit();
    }

    void application::init(unsigned w, unsigned h, bool fullscreen, int desired_fps) {
        ENFORCE(!m_initialized);
        s_instance = this;
        m_size.w = w;
        m_size.h = h;
        m_desired_fps = desired_fps;
        m_main_window = fullscreen
                            ? sdl::window(static_cast <int>(w), static_cast <int>(h),
                                          sdl::window::flags::FULL_SCREEN_DESKTOP)
                            : sdl::window(static_cast <int>(w), static_cast <int>(h));
        m_fullscreen = fullscreen;
        m_renderer = sdl::renderer(m_main_window);
        if (fullscreen) {
            m_renderer.set_logical_size(w, h);
        }
        m_window_id = m_main_window.id();

        if (!is_configured()) {
            application_config app_cfg;
            app_cfg.desired_fps = desired_fps;
            app_cfg.fullscreen = fullscreen;
            app_cfg.screen_width = w;
            app_cfg.screen_height = h;

            config_service::save(app_cfg, m_config_name);
        }

        m_quit_flag = !user_init_sequence();
        if (global_config.fullscreen_key) {
            m_hotkey_mapper.register_hotkey(*global_config.fullscreen_key, FULLSCREEN_PRESSED);
        }
        m_hotkey_mapper.add_to_reactor(m_event_reactor);
        m_initialized = true;
    }

    void application::init() {
        init(m_size.w, m_size.h, m_fullscreen, m_desired_fps);
    }

    void application::run() {
        if (!m_initialized) {
            if (m_configured) {
                init();
            }
        }
        ENFORCE(m_initialized);
        imgui::auto_init imgui_auto_init(m_main_window, m_renderer);

        static uint64_t frames = 0;
        static std::chrono::milliseconds total_time(0);

        unsigned old_fps = 0;
        const auto screen_ms_per_frame = std::chrono::milliseconds(1 + (1000 / m_desired_fps));
        auto start = sdl::get_ms_since_init();
        while (!need_to_quit()) {
            const auto s = sdl::get_ms_since_init();
            const auto delta_t = s - start;
            start = s;

            if (!internal_run(delta_t)) {
                return;
            }

            const auto e = sdl::get_ms_since_init();
            const auto ms = e - s;
            frames++;
            if (need_to_quit()) {
                if (screen_ms_per_frame > ms) {
                    const auto delta = screen_ms_per_frame - ms;
                    sdl::delay(delta);
                    total_time += delta;
                } else {
                    total_time += ms;
                }
#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
                m_fps = (1000u * frames) / static_cast <unsigned long>(total_time.count());
#if defined(_MSC_VER)
#pragma warning (pop)
#endif
                if (old_fps == 0) {
                    old_fps = m_fps;
                }
                if (frames > 1000u) {
                    frames = 0u;
                    total_time = std::chrono::milliseconds(0);
                }
            }
        }
    }

    void application::quit() {
        m_quit_flag = true;
    }

    unsigned application::get_fps() const {
        return m_fps;
    }

    application& application::instance() {
        ENFORCE(s_instance);
        return *s_instance;
    }

    const texture_atlas& application::get_texture_atlas() const {
        return m_texture_atlas;
    }

    texture_atlas& application::get_texture_atlas() {
        return m_texture_atlas;
    }

    scene_manager& application::get_scene_manager() {
        return m_scene_manager;
    }

    sdl::area_type application::get_window_dimensions() const {
        return m_size;
    }

    sdl::window_id_t application::get_window_id() const {
        return m_window_id;
    }

    bool application::is_configured() const {
        return m_configured;
    }

    void application::init_logger() {
    }

    void application::init_vfs() {
    }

    void application::init_assets() {
    }

    void application::on_error(const std::exception& exception) const {
        EVLOG_TRACE(EVLOG_FATAL, exception.what());
    }

    void application::on_unkown_error() const {
        EVLOG_TRACE(EVLOG_FATAL, "Unknown error");
    }

    void application::on_terminating() {
    }

    void application::on_low_memory() {
    }

    void application::on_before_backgroud() {
    }

    void application::on_in_backgroud() {
    }

    void application::on_before_foreground() {
    }

    void application::on_in_foreground() {
    }

    bool application::need_to_quit() const {
        return m_quit_flag || !s_instance || m_scene_manager.empty();
    }

    bool application::user_init_sequence() {
        try {
            init_logger();
            init_vfs();
            init_assets();
            setup_scenes(m_renderer);
        } catch (const std::exception& e) {
            on_error(e);
            return false;
        } catch (...) {
            on_unkown_error();
            return false;
        }
        return true;
    }

    bool application::internal_run(std::chrono::milliseconds delta_t) {
        imgui::init_imgui(m_main_window, m_renderer);

        try {
            SDL_Event sdl_event;
            m_event_reactor.reset();
            m_scene_manager.clear_events();
            while (SDL_PollEvent(&sdl_event)) {
                imgui::handle_imgui_events(sdl_event);
                bool do_quit = false;
                bool system_event_handeled = true;

                switch (sdl_event.type) {
                    case SDL_QUIT:
                        do_quit = true;
                        break;
                    case SDL_APP_TERMINATING:
                        on_terminating();
                        do_quit = true;
                        break;
                    case SDL_APP_LOWMEMORY:
                        on_low_memory();
                        break;
                    case SDL_APP_WILLENTERBACKGROUND:
                        on_before_backgroud();
                        break;
                    case SDL_APP_DIDENTERBACKGROUND:
                        on_in_backgroud();
                        break;
                    case SDL_APP_WILLENTERFOREGROUND:
                        on_before_foreground();
                        break;
                    case SDL_APP_DIDENTERFOREGROUND:
                        on_in_foreground();
                        break;
                    default:
                        system_event_handeled = false;
                        break;
                }
                if (do_quit) {
                    m_quit_flag = true;
                    break;
                }
                if (!system_event_handeled) {
                    auto internal_event = sdl::map_event(sdl_event);
                    m_event_reactor.handle(internal_event);
                    m_scene_manager.handle_input(internal_event);
                }
            }

            if (auto* s = m_event_reactor.get <hotkey_pressed_event>()) {
                if (s->hotkey_id == FULLSCREEN_PRESSED) {
                    if (m_fullscreen) {
                        m_main_window.set_fullscreen(false);
                        m_fullscreen = false;
                        m_renderer.set_logical_size(m_size.w, m_size.h);
                    } else {
                        m_main_window.set_fullscreen(true);
                        m_fullscreen = true;
                        m_renderer.set_logical_size(m_size.w, m_size.h);
                    }
                }
            }

            m_scene_manager.update(delta_t);
            m_renderer.clear();
            m_scene_manager.render(m_renderer);
            m_renderer.present();
        } catch (const std::exception& e) {
            on_error(e);
            return false;
        } catch (...) {
            on_unkown_error();
            return false;
        }
        return true;
    }
}
