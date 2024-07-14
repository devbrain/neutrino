//
// Created by igor on 7/14/24.
//

#ifndef CONFIG_HH
#define CONFIG_HH

#include <string_view>
#include <bsw/macros.hh>

namespace neutrino::utils::detail {
	struct config_var {
	};

	struct config_section {
	};
}

#define CONFIG_VAR(NAME)                                                          \
NAME;                                                                             \
struct PPCAT(config_var_, NAME) : public ::neutrino::utils::detail::config_var {  \
std::string_view name{STRINGIZE(NAME)};                                           \
};                                                                                \
PPCAT(config_var_, NAME) PPCAT(___config_var_,NAME)

#define CONFIG_SECTION(NAME)                                                          \
NAME;                                                                                 \
struct PPCAT(config_sec_, NAME) : public ::neutrino::utils::detail::config_section {  \
std::string_view name{STRINGIZE(NAME)};                                               \
};                                                                                    \
PPCAT(config_sec_, NAME) PPCAT(___config_sec_,NAME)
#endif
