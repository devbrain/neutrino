#pragma once

#include <exception>
#include <memory>
#include <neutrino/simplex/application.hh>
#include <sdlpp/app/entry_point.hh>

namespace neutrino::simplex::detail {
    struct application_access {
        [[nodiscard]] static sdlpp::abstract_application* get(application& app) noexcept {
            return app.get_application();
        }
    };

    template<typename Factory>
    SDL_AppResult app_init(void** appstate, int argc, char* argv[], Factory&& create) noexcept {
        *appstate = nullptr;
        try {
            // Publish ownership before initialization: SDL calls AppQuit even
            // when SDL or the renderer fails to initialize.
            std::unique_ptr<application> app = create();
            auto* runtime = application_access::get(*app);
            *appstate = app.release();
            return sdlpp::detail::app_init(runtime, argc, argv);
        } catch (const std::exception& error) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Simplex application construction failed: %s", error.what());
        } catch (...) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Simplex application construction failed: unknown exception");
        }
        return SDL_APP_FAILURE;
    }

    inline SDL_AppResult app_iterate(void* appstate) noexcept {
        if (!appstate) return SDL_APP_FAILURE;
        return sdlpp::detail::app_iterate(
            application_access::get(*static_cast<application*>(appstate)));
    }

    inline SDL_AppResult app_event(void* appstate, SDL_Event* event) noexcept {
        if (!appstate) return SDL_APP_FAILURE;
        return sdlpp::detail::app_handle_event(
            application_access::get(*static_cast<application*>(appstate)), event);
    }

    inline void app_quit(void* appstate) noexcept {
        std::unique_ptr<application> app(static_cast<application*>(appstate));
        if (!app) return;
        application_access::get(*app)->on_quit();
        // Destroy the user object, then its adapter/window/renderer. The
        // adapter's abstract_application base releases its SDL init guard last.
        // sdlpp::detail::app_quit would shut SDL down before these destructors.
    }
}

/// Include this header in exactly one source file, where SIMPLEX_MAIN is used.
/// Arguments are evaluated inside the initialization exception handler; argc
/// and argv may be forwarded to the application's constructor.
#define SIMPLEX_MAIN(AppClass, ...) \
    extern "C" SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) { \
        return neutrino::simplex::detail::app_init(appstate, argc, argv, [&] { \
            return std::make_unique<AppClass>(__VA_ARGS__); \
        }); \
    } \
    extern "C" SDL_AppResult SDL_AppIterate(void* appstate) { \
        return neutrino::simplex::detail::app_iterate(appstate); \
    } \
    extern "C" SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) { \
        return neutrino::simplex::detail::app_event(appstate, event); \
    } \
    extern "C" void SDL_AppQuit(void* appstate, SDL_AppResult) { \
        neutrino::simplex::detail::app_quit(appstate); \
    }
