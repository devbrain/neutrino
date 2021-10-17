//
// Created by igor on 23/09/2021.
//

#ifndef INCLUDE_NEUTRINO_UTILS_INI_CONFIG_HH
#define INCLUDE_NEUTRINO_UTILS_INI_CONFIG_HH

#include <string_view>
#include <neutrino/utils/macros.hh>

namespace neutrino::utils::detail {
  struct config_var {};
  struct config_section {};
}

#define CONFIG_VAR(NAME)                                                          \
NAME;                                                                             \
struct PPCAT(config_var_, NAME) : public ::neutrino::utils::detail::config_var {  \
    std::string_view name{STRINGIZE(NAME)};                                       \
};                                                                                \
PPCAT(config_var_, NAME) PPCAT(___config_var_,NAME)

#define CONFIG_SECTION(NAME)                                                          \
NAME;                                                                                 \
struct PPCAT(config_sec_, NAME) : public ::neutrino::utils::detail::config_section {  \
    std::string_view name{STRINGIZE(NAME)};                                           \
};                                                                                    \
PPCAT(config_sec_, NAME) PPCAT(___config_sec_,NAME)


#endif //INCLUDE_NEUTRINO_UTILS_INI_CONFIG_HH
