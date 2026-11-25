//
// Created by igor on 25/11/2026.
//

#pragma once

#include <cstdint>
#include <type_traits>

namespace neutrino {
    template<typename T>
    concept command_like = std::is_enum_v <T> &&
                           std::is_same_v <std::underlying_type_t <T>, std::int32_t>;
}
