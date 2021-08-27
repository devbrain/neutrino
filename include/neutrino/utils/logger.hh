//
// Created by igor on 01/07/2020.
//

#ifndef NEUTRINO_UTILS_LOGGER_HH
#define NEUTRINO_UTILS_LOGGER_HH

#include <variant>
#include <neutrino/sdl/sdl2.hh>
#include <sstream>

template <typename T0, typename ... Ts>
inline
std::ostream &operator<< (std::ostream &s,
                          std::variant<T0, Ts...> const &v) {
  std::visit ([&] (auto &&arg) { s << arg; }, v);
  return s;
}

namespace neutrino::logger {
  enum class priority {
      DEBUG = SDL_LOG_PRIORITY_DEBUG,
      VERBOSE = SDL_LOG_PRIORITY_VERBOSE,
      INFO = SDL_LOG_PRIORITY_INFO,
      WARN = SDL_LOG_PRIORITY_WARN,
      ERROR = SDL_LOG_PRIORITY_ERROR,
      CRITICAL = SDL_LOG_PRIORITY_CRITICAL
  };

  template <typename Arg, typename ... Args>
  inline
  void trace (priority prio, const char *const func, const char *const file, int line, Arg &&arg, Args &&... args) {
    static auto logger_prio = SDL_LogGetPriority (SDL_LOG_CATEGORY_APPLICATION);
    if (static_cast<int>(prio) < logger_prio) {
      return;
    }

    std::ostringstream out;
    out << std::forward<Arg> (arg);
    ((out << std::forward<Args> (args)), ...);
    SDL_LogMessage (SDL_LOG_CATEGORY_APPLICATION, static_cast<SDL_LogPriority>(prio),
                    "%s %s@%d : %s",
                    func, file, line, out.str ().c_str ());
  }

  inline
  void set_default (priority prio) {
    SDL_LogSetPriority (SDL_LOG_CATEGORY_APPLICATION, static_cast<SDL_LogPriority>(prio));
  }
}

#define LOG_TRACE(PRIO, ...) ::neutrino::logger::trace(::neutrino::logger::priority:: PRIO, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)

#endif
