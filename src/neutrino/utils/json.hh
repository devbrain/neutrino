//
// Created by igor on 06/07/2026.
//

#pragma once

// Single configuration point for nlohmann/json. Always include this header
// instead of <nlohmann/json.hpp>: JSON_DIAGNOSTICS changes the basic_json
// layout, so every translation unit must see the same configuration.

#include <string>
#include <string_view>

#if !defined(JSON_DIAGNOSTICS)
#define JSON_DIAGNOSTICS 1
#endif

#if defined(JSON_NOEXCEPTION)
#undef JSON_NOEXCEPTION
#endif

namespace neutrino::utils {
    [[noreturn]] void json_assert_handler(const char* condition, const char* file, int line);

    // Rethrows the concrete nlohmann exception type; a plain function taking
    // const std::exception& would slice and break catch (const json::exception&).
    template <typename E>
    [[noreturn]] void json_raise(const E& e) {
        throw e;
    }
}

#if !defined(JSON_ASSERT)
#define JSON_ASSERT(cond) ((cond) ? \
        (void)0 : \
        (void)::neutrino::utils::json_assert_handler(#cond, __FILE__, __LINE__))
#endif

#define JSON_THROW_USER ::neutrino::utils::json_raise

#include <nlohmann/json.hpp>

namespace neutrino::utils {
    using json = nlohmann::json;

    /**
     * @brief Trim whitespace and a leading UTF-8 BOM from a document.
     *
     * Use before sniffing a document's format by its first byte.
     */
    [[nodiscard]] std::string trim_document(std::string_view value);
}
