//
// Created by igor on 05/07/2026.
//

#include <neutrino/world/tmx_loader.hh>

#include "world/tmx/parser.hh"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace neutrino {
    namespace {
        [[nodiscard]] std::string read_text_file(const std::filesystem::path& path) {
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                throw std::runtime_error("failed to open world file: " + path.string());
            }
            std::ostringstream out;
            out << input.rdbuf();
            return out.str();
        }

        [[nodiscard]] bool path_has_prefix(
            const std::filesystem::path& path,
            const std::filesystem::path& prefix) {
            auto path_it = path.begin();
            auto prefix_it = prefix.begin();
            for (; prefix_it != prefix.end(); ++prefix_it, ++path_it) {
                if (path_it == path.end() || *path_it != *prefix_it) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] std::filesystem::path resolve_external_path(
            const std::filesystem::path& base,
            std::string_view source) {
            const auto relative = std::filesystem::path(std::string(source));
            if (relative.is_absolute()) {
                throw std::runtime_error("external tileset path must be relative: " + relative.string());
            }

            std::error_code ec;
            const auto canonical_base = std::filesystem::weakly_canonical(base, ec);
            if (ec) {
                throw std::runtime_error("failed to resolve world directory: " + base.string());
            }

            const auto candidate = std::filesystem::weakly_canonical(canonical_base / relative, ec);
            if (ec) {
                throw std::runtime_error("failed to resolve external tileset path: " + relative.string());
            }
            if (!path_has_prefix(candidate, canonical_base)) {
                throw std::runtime_error("external tileset path escapes world directory: " + relative.string());
            }
            return candidate;
        }
    }

    world load_tmx_world(std::string_view document, tmx_path_resolver resolver) {
        return world_tmx::parse_document(document, resolver);
    }

    world load_tmx_world_file(const std::filesystem::path& path) {
        auto content = read_text_file(path);
        auto base = path.parent_path();
        return load_tmx_world(content, [base = std::move(base)](std::string_view source) {
            return read_text_file(resolve_external_path(base, source));
        });
    }
}
