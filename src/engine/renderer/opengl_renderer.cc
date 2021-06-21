//
// Created by igor on 14/06/2021.
//

#include <GL/glcorearb.h>
#include <neutrino/engine/renderer/opengl_renderer.hh>
#include <neutrino/sdl/window.hh>
#include <neutrino/utils/exception.hh>

constexpr int gl_major_version = 4;
constexpr int gl_minor_version = 5;
constexpr bool gl_debug = true;
constexpr bool vsync = false;

namespace neutrino::engine {
    struct opengl_renderer::impl {
        explicit impl(sdl::window&& w)
        : window(std::move(w))
        {
            // opengl forawrd debug context flags
            const auto context_flags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG | (gl_debug ? SDL_GL_CONTEXT_DEBUG_FLAG : 0);

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_major_version);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_minor_version);

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);

            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

            context = SDL_GL_CreateContext(window.handle());
            if (vsync)
            {
                SDL_GL_SetSwapInterval(1);
            }
            if (SDL_GL_LoadLibrary(nullptr) != 0) {
                SDL_GL_DeleteContext(context);
                RAISE_EX("Failed to open GL library");
            }
            glClearNamedFramebufferfv = (PFNGLCLEARNAMEDFRAMEBUFFERFVPROC)SDL_GL_GetProcAddress("glClearNamedFramebufferfv");

            SDL_GL_UnloadLibrary();
        }

        ~impl() {
            SDL_GL_DeleteContext(context);
        }
        sdl::window window;
        SDL_GLContext context;
        PFNGLCLEARNAMEDFRAMEBUFFERFVPROC glClearNamedFramebufferfv;

    };
    // ================================================================================
    void opengl_renderer::open (const basic_window& window) {
        auto w = sdl::window::by_id(window.id());
        m_pimpl = std::make_unique<impl>(std::move(w));
    }
    // --------------------------------------------------------------------------------
    opengl_renderer::~opengl_renderer() = default;
    // --------------------------------------------------------------------------------
    void opengl_renderer::clear() {
        const float clear_color[4] = {0.4, 0.4, 0.4, 1};
        m_pimpl->glClearNamedFramebufferfv(0, GL_COLOR, 0, clear_color);
    }
    // --------------------------------------------------------------------------------
    void opengl_renderer::present() {
        m_pimpl->window.swap_opengl_window();
    }
    // --------------------------------------------------------------------------------
    basic_window::window_kind_t opengl_renderer::window_kind() const noexcept
    {
        return basic_window::OPENGL;
    }
    // --------------------------------------------------------------------------------
    void opengl_renderer::invalidate([[maybe_unused]] const basic_window& window)
    {

    }
}