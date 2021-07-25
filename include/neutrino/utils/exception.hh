//
// Created by igor on 31/05/2020.
//

#ifndef NEUTRINO_UTILS_EXCEPTION_HH
#define NEUTRINO_UTILS_EXCEPTION_HH

#include <sstream>
#include <string>
#include <stdexcept>
#include <neutrino/utils/macros.hh>

namespace neutrino
{

    class exception : public std::runtime_error
    {
    public:
        template <typename ... Args>
        exception(const char* function, const char* source, int line, Args&&... args);

    protected:
        explicit exception(const std::string& err)
        : std::runtime_error(err) {}
    private:
        template <typename ... Args>
        static std::string _create(const char* function, const char* source, int line, Args&&... args);

    };
} // ns
// ==========================================================================
// Implementation
// ==========================================================================
namespace neutrino
{


    template <typename ... Args>
    inline
    exception::exception(const char* function, const char* source, int line, Args&&... args)
    : std::runtime_error(_create(function, source, line, std::forward<Args>(args)...))
    {

    }
    // --------------------------------------------------------------------------------------
    template<typename ... Args>
    inline
    std::string exception::_create(const char* function, const char* source, int line, Args&&... args)
    {
        std::ostringstream os;


        os << "Error at ";

        os  << function << " " << source << "@" << line;
        if constexpr (sizeof...(args) > 0)
        {
            os << " :";
            ((os << ' ' << std::forward<Args>(args)), ...);
        }
        return os.str();
    }


    template <typename ... Args>
    inline
    void raise_exception [[noreturn]] (const char* function, const char* source, int line, Args&&... args) {
        throw exception(function, source, line, std::forward<Args>(args)...);
    }
}

#if defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif


#define RAISE_EX(...) ::neutrino::raise_exception(__PRETTY_FUNCTION__, __FILE__, __LINE__,  ##__VA_ARGS__)
#define ENFORCE(expr) while (!(expr)) {::neutrino::raise_exception(__PRETTY_FUNCTION__, __FILE__, __LINE__, "Assertion failed: " STRINGIZE(expr));}
#endif
