//
// Created by igor on 31/05/2020.
//

#ifndef NEUTRINO_UTILS_EXCEPTION_HH
#define NEUTRINO_UTILS_EXCEPTION_HH

#include <sstream>
#include <string>
#include <stdexcept>
#include <neutrino/sdl/sdl2.hh>

namespace neutrino
{
    namespace detail
    {
        struct sdl_related{};
    }
    class exception : public std::runtime_error
    {
    public:
        template <typename ... Args>
        exception(detail::sdl_related, const char* function, const char* source, int line, Args&&... args);

        template <typename ... Args>
        exception(const char* function, const char* source, int line, Args&&... args);
    private:
        template <typename Discriminator, typename ... Args>
        static std::string _create(Discriminator d, const char* function, const char* source, int line, Args&&... args);

    };
} // ns
// ==========================================================================
// Implementation
// ==========================================================================
namespace neutrino
{

    template<typename ... Args>
    inline
    exception::exception(detail::sdl_related d, const char* function, const char* source, int line, Args&&... args)
    : std::runtime_error(_create(d, function, source, line, std::forward<Args>(args)...))
    {

    }

    template <typename ... Args>
    inline
    exception::exception(const char* function, const char* source, int line, Args&&... args)
    : std::runtime_error(_create(0, function, source, line, std::forward<Args>(args)...))
    {

    }
    // --------------------------------------------------------------------------------------
    template<typename Discriminator, typename ... Args>
    inline
    std::string exception::_create(Discriminator /*d*/, const char* function, const char* source, int line, Args&&... args)
    {
        std::ostringstream os;
        if constexpr (std::is_same_v<Discriminator, detail::sdl_related>)
        {
            os << "SDL error [" << SDL_GetError() << "] at ";
        }
        else
        {
            os << "Error at ";
        }
        os  << function << " " << source << "@" << line;
        if constexpr (sizeof...(args) > 0)
        {
            os << " :";
            ((os << ' ' << std::forward<Args>(args)), ...);
        }
        return os.str();
    }
}

#if defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define RAISE_SDL_EX(...) throw ::neutrino::exception{::neutrino::detail::sdl_related{}, __PRETTY_FUNCTION__, __FILE__, __LINE__,  ##__VA_ARGS__}
#define RAISE_EX(...) throw ::neutrino::exception{__PRETTY_FUNCTION__, __FILE__, __LINE__,  ##__VA_ARGS__}
#endif //SDLPP_EXCEPTION_HH
