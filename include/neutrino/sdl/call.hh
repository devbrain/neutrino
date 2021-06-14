//
// Created by igor on 01/06/2020.
//

#ifndef NEUTRINO_SDL_CALL_HH
#define NEUTRINO_SDL_CALL_HH

#include <neutrino/sdl/sdl2.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::sdl
{
    namespace detail
    {
        template<typename R, typename... Args2, typename... Args>
        inline
        auto call_sdl(R(f)(Args2...), const char* /*function*/, const char* /*file*/, int /*line*/, Args&&... args)
        -> decltype(f(std::forward<Args>(args)...))
        {
            return f(std::forward<Args>(args)...);
        }
        // ----------------------------------------------------------------------------------------------
        template<typename R, typename... Args2, typename... Args>
        inline
        auto call_sdl(R* (f)(Args2...), const char* function, const char* file, int line, Args&&... args)
        -> decltype(f(std::forward<Args>(args)...))
        {
            auto r = f(std::forward<Args>(args)...);
            if (r == nullptr)
            {
                throw ::neutrino::exception(::neutrino::detail::sdl_related{}, function, file, line);
            }
            return r;
        }
        // ----------------------------------------------------------------------------------------------
        template<typename... Args2, typename... Args>
        auto call_sdl(int(f)(Args2...), const char* function, const char* file, int line, Args&&... args)
        -> decltype(f(std::forward<Args>(args)...))
        {
            auto r = f(std::forward<Args>(args)...);
            if (r < 0)
            {
                throw ::neutrino::exception(::neutrino::detail::sdl_related{}, function, file, line);
            }
            return r;
        }
        // ----------------------------------------------------------------------------------------------
        template<typename... Args2, typename... Args>
        auto call_sdl(void(f)(Args2...), const char* /*function*/, const char* /*file*/, int /*line*/, Args&&... args)
        -> decltype(f(std::forward<Args>(args)...))
        {
            f(std::forward<Args>(args)...);
        }
    }
#define SAFE_SDL_CALL(F,...) ::neutrino::sdl::detail::call_sdl(F, __PRETTY_FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
}

#endif //SDLPP_CALL_HH
