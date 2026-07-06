//
// Created by igor on 06/07/2026.
//

#include "utils/json.hh"

#include <failsafe/exception.hh>

#include <cctype>

namespace neutrino::utils {
    namespace {
        [[nodiscard]] std::string_view trim_view(std::string_view value) {
            while (!value.empty() && std::isspace(static_cast <unsigned char>(value.front()))) {
                value.remove_prefix(1);
            }
            while (!value.empty() && std::isspace(static_cast <unsigned char>(value.back()))) {
                value.remove_suffix(1);
            }
            return value;
        }
    }

    [[noreturn]] void json_assert_handler(const char* condition, const char* file, int line) {
        THROW_LOGIC("JSON assertion <", condition, "> failed at", file, ":", line);
    }

    std::string trim_document(std::string_view value) {
        auto clean = trim_view(value);
        if (clean.size() >= 3 &&
            static_cast <unsigned char>(clean[0]) == 0xefu &&
            static_cast <unsigned char>(clean[1]) == 0xbbu &&
            static_cast <unsigned char>(clean[2]) == 0xbfu) {
            clean.remove_prefix(3);
            clean = trim_view(clean);
        }
        return std::string{clean};
    }
}
