//
// Created by igor on 26/11/2026.
//

#pragma once

#include <string_view>

namespace neutrino::input_detail {
    /// @brief @p text without leading and trailing blanks (spaces, tabs, CR, LF).
    [[nodiscard]] constexpr std::string_view trim(std::string_view text) noexcept {
        constexpr std::string_view blanks = " \t\r\n";
        const auto first = text.find_first_not_of(blanks);
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = text.find_last_not_of(blanks);
        return text.substr(first, last - first + 1);
    }
}
