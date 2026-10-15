//
// Created by igor on 12/10/2026.
//

#include "luxor/resources/parser/depth_parser.hh"

#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace luxor::rc {

    namespace {
        bool iequals(std::string_view a, std::string_view b) noexcept {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                if (std::tolower(static_cast<unsigned char>(a[i])) !=
                    std::tolower(static_cast<unsigned char>(b[i]))) {
                    return false;
                }
            }
            return true;
        }

        std::string_view trim_whitespace(std::string_view s) noexcept {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
                s.remove_prefix(1);
            }
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
                s.remove_suffix(1);
            }
            return s;
        }

        // Strips comments and outer quotes from a single line
        expected<std::string, std::string> process_line(std::string_view line, size_t line_no) {
            line = trim_whitespace(line);
            if (line.empty()) {
                return std::string{};
            }

            // Check whole-line comments
            if (line.starts_with("//") || line.starts_with(';')) {
                return std::string{};
            }

            // If quoted line
            if (line.front() == '"') {
                if (line.size() < 2 || line.back() != '"') {
                    // Check if trailing comment exists after the closing quote
                    size_t closing = line.find('"', 1);
                    if (closing == std::string_view::npos) {
                        return tl::make_unexpected("Line " + std::to_string(line_no) + ": Unclosed quote in \"" + std::string(line) + "\"");
                    }
                    std::string_view content = line.substr(1, closing - 1);
                    std::string_view rest = trim_whitespace(line.substr(closing + 1));
                    if (!rest.empty() && !rest.starts_with("//") && !rest.starts_with(';')) {
                        return tl::make_unexpected("Line " + std::to_string(line_no) + ": Unexpected trailing content after closing quote");
                    }
                    return std::string(content);
                }
                return std::string(line.substr(1, line.size() - 2));
            }

            // Unquoted line: strip inline comments if any
            size_t comment_pos = line.find("//");
            if (comment_pos != std::string_view::npos) {
                line = line.substr(0, comment_pos);
            }
            comment_pos = line.find(';');
            if (comment_pos != std::string_view::npos) {
                line = line.substr(0, comment_pos);
            }

            line = trim_whitespace(line);
            return std::string(line);
        }

        expected<std::vector<std::string>, std::string> parse_lines(std::string_view content) {
            std::vector<std::string> result;
            size_t line_no = 0;
            size_t start = 0;

            while (start < content.size()) {
                size_t end = content.find('\n', start);
                if (end == std::string_view::npos) {
                    end = content.size();
                }

                line_no++;
                std::string_view line = content.substr(start, end - start);
                start = end + 1;

                auto processed = process_line(line, line_no);
                if (!processed) {
                    return tl::make_unexpected(processed.error());
                }

                if (!processed->empty()) {
                    result.push_back(std::move(*processed));
                }
            }

            return result;
        }
    } // namespace

    // =========================================================================
    // depth_layer
    // =========================================================================

    nlohmann::json depth_layer::to_json() const {
        return {
            {"index", index},
            {"name", name}
        };
    }

    void to_json(nlohmann::json& j, const depth_layer& layer) {
        j = layer.to_json();
    }

    // =========================================================================
    // depth_table
    // =========================================================================

    bool depth_table::has(std::string_view name) const {
        return get_index(name).has_value();
    }

    std::optional<uint32_t> depth_table::get_index(std::string_view name) const {
        for (uint32_t i = 0; i < layers.size(); ++i) {
            if (iequals(layers[i], name)) {
                return i;
            }
        }
        return std::nullopt;
    }

    std::optional<std::string_view> depth_table::get_name(uint32_t index) const {
        if (index < layers.size()) {
            return layers[index];
        }
        return std::nullopt;
    }

    bool depth_table::is_parent(std::string_view name) noexcept {
        return iequals(name, "#parent");
    }

    nlohmann::json depth_table::to_json() const {
        nlohmann::json layer_array = nlohmann::json::array();
        for (uint32_t i = 0; i < layers.size(); ++i) {
            layer_array.push_back(depth_layer{i, layers[i]}.to_json());
        }
        return {
            {"layers", std::move(layer_array)},
            {"total_layers", layers.size()}
        };
    }

    void to_json(nlohmann::json& j, const depth_table& table) {
        j = table.to_json();
    }

    expected<depth_table, std::string> parse_depths(std::string_view content) {
        auto raw_lines = parse_lines(content);
        if (!raw_lines) {
            return tl::make_unexpected(raw_lines.error());
        }

        if (raw_lines->size() > depth_table::MAX_LAYERS) {
            return tl::make_unexpected("Exceeded maximum depth layers capacity (" +
                                       std::to_string(raw_lines->size()) + " > " +
                                       std::to_string(depth_table::MAX_LAYERS) + ")");
        }

        depth_table dt;
        for (size_t i = 0; i < raw_lines->size(); ++i) {
            const auto& layer_name = (*raw_lines)[i];
            if (dt.has(layer_name)) {
                return tl::make_unexpected("Duplicate depth layer defined: " + layer_name);
            }
            dt.layers.push_back(layer_name);
        }

        return dt;
    }

    expected<depth_table, std::string> parse_depths_file(const std::filesystem::path& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open()) {
            return tl::make_unexpected("Failed to open file: " + path.string());
        }
        std::ostringstream ss;
        ss << ifs.rdbuf();
        return parse_depths(ss.str());
    }

    // =========================================================================
    // string_list
    // =========================================================================

    bool string_list::has(std::string_view item, bool case_sensitive) const {
        return find_index(item, case_sensitive).has_value();
    }

    std::optional<size_t> string_list::find_index(std::string_view item, bool case_sensitive) const {
        for (size_t i = 0; i < items.size(); ++i) {
            if (case_sensitive) {
                if (items[i] == item) return i;
            } else {
                if (iequals(items[i], item)) return i;
            }
        }
        return std::nullopt;
    }

    nlohmann::json string_list::to_json() const {
        return {
            {"items", items},
            {"count", items.size()}
        };
    }

    void to_json(nlohmann::json& j, const string_list& list) {
        j = list.to_json();
    }

    expected<string_list, std::string> parse_string_list(std::string_view content) {
        auto raw_lines = parse_lines(content);
        if (!raw_lines) {
            return tl::make_unexpected(raw_lines.error());
        }
        return string_list{std::move(*raw_lines)};
    }

    expected<string_list, std::string> parse_string_list_file(const std::filesystem::path& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open()) {
            return tl::make_unexpected("Failed to open file: " + path.string());
        }
        std::ostringstream ss;
        ss << ifs.rdbuf();
        return parse_string_list(ss.str());
    }

} // namespace luxor::rc
