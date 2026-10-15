//
// Created by igor on 11/10/2026.
//

#include "luxor/resources/parser/cfg_parser.hh"
#include "luxor/resources/parser/cfg_scanner_context.h"
#include "luxor/resources/parser/cfg_parser_context.h"
#include "cfg_parser.h"

#include <charconv>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <optional>
#include <cctype>

extern "C" {
void* CfgParseAlloc(void* (* allocProc)(size_t));
void CfgParse(void*, int, token_value_t, cfg_parser_context_t*);
void CfgParseFree(void*, void (* freeProc)(void*));
void CfgParseTrace(FILE* TraceFILE, char* zTracePrompt);
}

namespace luxor::rc {
    namespace {
        bool iequals(std::string_view a, std::string_view b) noexcept {
            return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                [](char ca, char cb) {
                    return std::tolower(static_cast<unsigned char>(ca)) ==
                           std::tolower(static_cast<unsigned char>(cb));
                });
        }

        struct cfg_builder {
            config_document doc;
            std::optional<size_t> current_ini_section_idx;
            std::optional<size_t> active_section_idx;
            std::string current_key;
            std::vector<value> current_values;
            bool has_error{false};
            std::string error_message;

            section* get_active_section() {
                if (active_section_idx.has_value() && *active_section_idx < doc.sections.size()) {
                    return &doc.sections[*active_section_idx];
                }
                return nullptr;
            }
        };

        const property* find_prop(const std::vector<property>& props, std::string_view key) {
            auto it = std::find_if(props.begin(), props.end(),
                [&key](const property& p) { return iequals(p.key, key); });
            return (it != props.end()) ? &(*it) : nullptr;
        }

        std::string_view get_string_prop(const std::vector<property>& props, std::string_view key, std::string_view def) {
            const auto* p = find_prop(props, key);
            if (p && !p->values.empty()) {
                if (const auto* s = std::get_if<std::string>(&p->values[0])) {
                    return *s;
                }
            }
            return def;
        }

        bool get_bool_prop(const std::vector<property>& props, std::string_view key, bool def) {
            const auto* p = find_prop(props, key);
            if (p && !p->values.empty()) {
                if (const auto* b = std::get_if<bool>(&p->values[0])) {
                    return *b;
                }
            }
            return def;
        }

        double get_number_prop(const std::vector<property>& props, std::string_view key, double def) {
            const auto* p = find_prop(props, key);
            if (p && !p->values.empty()) {
                if (const auto* d = std::get_if<double>(&p->values[0])) {
                    return *d;
                }
            }
            return def;
        }

        std::vector<double> get_numbers_prop(const std::vector<property>& props, std::string_view key) {
            std::vector<double> result;
            const auto* p = find_prop(props, key);
            if (p) {
                result.reserve(p->values.size());
                for (const auto& v : p->values) {
                    if (const auto* d = std::get_if<double>(&v)) {
                        result.push_back(*d);
                    }
                }
            }
            return result;
        }
    } // namespace

    // -------------------------------------------------------------------------
    // section methods
    // -------------------------------------------------------------------------

    bool section::has(std::string_view key) const {
        return find(key) != nullptr;
    }

    const property* section::find(std::string_view key) const {
        return find_prop(properties, key);
    }

    std::string_view section::get_string(std::string_view key, std::string_view def) const {
        return get_string_prop(properties, key, def);
    }

    bool section::get_bool(std::string_view key, bool def) const {
        return get_bool_prop(properties, key, def);
    }

    double section::get_number(std::string_view key, double def) const {
        return get_number_prop(properties, key, def);
    }

    int section::get_int(std::string_view key, int def) const {
        return static_cast<int>(get_number(key, def));
    }

    std::vector<double> section::get_numbers(std::string_view key) const {
        return get_numbers_prop(properties, key);
    }

    const std::vector<value>* section::get_values(std::string_view key) const {
        const auto* p = find(key);
        return p ? &p->values : nullptr;
    }

    // -------------------------------------------------------------------------
    // config_document methods
    // -------------------------------------------------------------------------

    bool config_document::has(std::string_view key) const {
        return find(key) != nullptr;
    }

    const property* config_document::find(std::string_view key) const {
        return find_prop(properties, key);
    }

    std::string_view config_document::get_string(std::string_view key, std::string_view def) const {
        return get_string_prop(properties, key, def);
    }

    bool config_document::get_bool(std::string_view key, bool def) const {
        return get_bool_prop(properties, key, def);
    }

    double config_document::get_number(std::string_view key, double def) const {
        return get_number_prop(properties, key, def);
    }

    int config_document::get_int(std::string_view key, int def) const {
        return static_cast<int>(get_number(key, def));
    }

    std::vector<double> config_document::get_numbers(std::string_view key) const {
        return get_numbers_prop(properties, key);
    }

    const std::vector<value>* config_document::get_values(std::string_view key) const {
        const auto* p = find(key);
        return p ? &p->values : nullptr;
    }

    bool config_document::has_section(std::string_view name) const {
        return find_section(name) != nullptr;
    }

    bool config_document::has_section(std::string_view type, std::string_view name) const {
        return find_section(type, name) != nullptr;
    }

    const section* config_document::find_section(std::string_view name) const {
        auto it = std::find_if(sections.begin(), sections.end(),
            [&name](const section& s) { return iequals(s.name, name); });
        return (it != sections.end()) ? &(*it) : nullptr;
    }

    const section* config_document::find_section(std::string_view type, std::string_view name) const {
        auto it = std::find_if(sections.begin(), sections.end(),
            [&type, &name](const section& s) {
                return iequals(s.type, type) && iequals(s.name, name);
            });
        return (it != sections.end()) ? &(*it) : nullptr;
    }

    std::vector<const section*> config_document::find_sections_by_type(std::string_view type) const {
        std::vector<const section*> result;
        for (const auto& s : sections) {
            if (iequals(s.type, type)) {
                result.push_back(&s);
            }
        }
        return result;
    }

    std::string_view config_document::get_string(std::string_view section_name, std::string_view key, std::string_view def) const {
        return get_section_string(section_name, key, def);
    }

    bool config_document::get_bool(std::string_view section_name, std::string_view key, bool def) const {
        return get_section_bool(section_name, key, def);
    }

    double config_document::get_number(std::string_view section_name, std::string_view key, double def) const {
        return get_section_number(section_name, key, def);
    }

    int config_document::get_int(std::string_view section_name, std::string_view key, int def) const {
        return get_section_int(section_name, key, def);
    }

    std::string_view config_document::get_section_string(std::string_view section_name, std::string_view key, std::string_view def) const {
        const auto* s = find_section(section_name);
        return s ? s->get_string(key, def) : def;
    }

    bool config_document::get_section_bool(std::string_view section_name, std::string_view key, bool def) const {
        const auto* s = find_section(section_name);
        return s ? s->get_bool(key, def) : def;
    }

    double config_document::get_section_number(std::string_view section_name, std::string_view key, double def) const {
        const auto* s = find_section(section_name);
        return s ? s->get_number(key, def) : def;
    }

    int config_document::get_section_int(std::string_view section_name, std::string_view key, int def) const {
        const auto* s = find_section(section_name);
        return s ? s->get_int(key, def) : def;
    }

    // -------------------------------------------------------------------------
    // JSON Serialization
    // -------------------------------------------------------------------------

    void to_json(nlohmann::json& j, const value& val) {
        std::visit([&j](const auto& v) {
            j = v;
        }, val);
    }

    nlohmann::json property::to_json() const {
        nlohmann::json j;
        j["key"] = key;
        auto vals = nlohmann::json::array();
        for (const auto& v : values) {
            nlohmann::json jv;
            luxor::rc::to_json(jv, v);
            vals.push_back(std::move(jv));
        }
        j["values"] = std::move(vals);
        return j;
    }

    void to_json(nlohmann::json& j, const property& prop) {
        j = prop.to_json();
    }

    nlohmann::json section::to_json() const {
        nlohmann::json j;
        j["type"] = type;
        j["name"] = name;
        if (!target.empty()) {
            j["target"] = target;
        }
        auto props = nlohmann::json::array();
        for (const auto& p : properties) {
            props.push_back(p.to_json());
        }
        j["properties"] = std::move(props);
        return j;
    }

    void to_json(nlohmann::json& j, const section& sec) {
        j = sec.to_json();
    }

    nlohmann::json config_document::to_json() const {
        nlohmann::json j;
        auto props = nlohmann::json::array();
        for (const auto& p : properties) {
            props.push_back(p.to_json());
        }
        j["properties"] = std::move(props);

        auto secs = nlohmann::json::array();
        for (const auto& s : sections) {
            secs.push_back(s.to_json());
        }
        j["sections"] = std::move(secs);
        return j;
    }

    void to_json(nlohmann::json& j, const config_document& doc) {
        j = doc.to_json();
    }

    // -------------------------------------------------------------------------
    // parse_config & parse_config_file
    // -------------------------------------------------------------------------

    expected<config_document, std::string> parse_config(std::string_view content) {
        cfg_builder builder;
        cfg_parser_context_t parser_ctx{};
        parser_ctx.user_data = &builder;

        constexpr size_t PADDING = 32;
        std::vector<char> buffer(content.size() + PADDING, '\0');
        std::memcpy(buffer.data(), content.data(), content.size());

        scanner_context_t scanner_ctx{};
        scanner_ctx.input = buffer.data();
        scanner_ctx.cursor = buffer.data();
        scanner_ctx.eof = buffer.data() + content.size();
        scanner_ctx.limit = buffer.data() + content.size() + PADDING;
        scanner_ctx.marker = buffer.data();
        scanner_ctx.line = 1;
        scanner_ctx.column = 1;
        scanner_ctx.last_token = TOKEN_EOL;

        void* parser = CfgParseAlloc(std::malloc);
        if (!parser) {
            return unexpected("Failed to allocate parser");
        }

        token_value_t token{};
        int token_id = 0;
        while ((token_id = cfg_scan_token(&scanner_ctx, &token)) != 0) {
            CfgParse(parser, token_id, token, &parser_ctx);
            if (builder.has_error) {
                break;
            }
        }

        if (!builder.has_error) {
            token_value_t eof_token{scanner_ctx.cursor, scanner_ctx.cursor, scanner_ctx.line, scanner_ctx.column};
            CfgParse(parser, 0, eof_token, &parser_ctx);
        }

        CfgParseFree(parser, std::free);

        if (builder.has_error) {
            return unexpected(builder.error_message);
        }

        return std::move(builder.doc);
    }

    expected<config_document, std::string> parse_config_file(const std::filesystem::path& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open()) {
            return unexpected("Failed to open file: " + path.string());
        }
        std::stringstream ss;
        ss << ifs.rdbuf();
        return parse_config(ss.str());
    }

} // namespace luxor::rc

extern "C" {
void cfg_parser_on_property_key(cfg_parser_context_t* ctx, const token_value_t* token) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    builder->current_key = std::string(token->start, static_cast<size_t>(token->end - token->start));
    builder->current_values.clear();
}

void cfg_parser_on_value_string(cfg_parser_context_t* ctx, const token_value_t* token) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    const char* s = token->start;
    const char* e = token->end;
    if (e - s >= 2 && *s == '"' && *(e - 1) == '"') {
        s++;
        e--;
    }
    std::string text(s, static_cast<size_t>(e - s));
    builder->current_values.emplace_back(std::move(text));
}

void cfg_parser_on_value_number(cfg_parser_context_t* ctx, const token_value_t* token) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    double num = 0.0;
    auto [ptr, ec] = std::from_chars(token->start, token->end, num);
    if (ec != std::errc{}) {
        std::string text(token->start, static_cast<size_t>(token->end - token->start));
        char* end_ptr = nullptr;
        num = std::strtod(text.c_str(), &end_ptr);
    }
    builder->current_values.emplace_back(num);
}

void cfg_parser_on_value_bool(cfg_parser_context_t* ctx, const token_value_t* token) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    const bool b = (std::string_view(token->start, static_cast<size_t>(token->end - token->start)) == "true");
    builder->current_values.emplace_back(b);
}

void cfg_parser_on_value_ident(cfg_parser_context_t* ctx, const token_value_t* token) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    std::string text(token->start, static_cast<size_t>(token->end - token->start));
    builder->current_values.emplace_back(std::move(text));
}

void cfg_parser_on_section_header(cfg_parser_context_t* ctx, const token_value_t* token) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    const char* s = token->start;
    const char* e = token->end;
    if (e - s >= 2 && *s == '"' && *(e - 1) == '"') {
        s++;
        e--;
    }
    std::string name(s, static_cast<size_t>(e - s));
    builder->doc.sections.push_back(luxor::rc::section{
        .type = "",
        .name = std::move(name),
        .properties = {}
    });
    size_t idx = builder->doc.sections.size() - 1;
    builder->current_ini_section_idx = idx;
    builder->active_section_idx = idx;
}

void cfg_parser_on_block_header(cfg_parser_context_t* ctx, const token_value_t* type, const token_value_t* name) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    std::string type_str;
    if (type) {
        type_str = std::string(type->start, static_cast<size_t>(type->end - type->start));
    }
    std::string name_str;
    if (name) {
        name_str = std::string(name->start, static_cast<size_t>(name->end - name->start));
    }
    builder->doc.sections.push_back(luxor::rc::section{
        .type = std::move(type_str),
        .name = std::move(name_str),
        .properties = {}
    });
    size_t idx = builder->doc.sections.size() - 1;
    builder->active_section_idx = idx;
}

void cfg_parser_on_attached_block_start(cfg_parser_context_t* ctx) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);

    luxor::rc::property last_prop;
    bool found_prop = false;

    if (auto* sec = builder->get_active_section()) {
        if (!sec->properties.empty()) {
            last_prop = std::move(sec->properties.back());
            sec->properties.pop_back();
            found_prop = true;
        }
    } else if (!builder->doc.properties.empty()) {
        last_prop = std::move(builder->doc.properties.back());
        builder->doc.properties.pop_back();
        found_prop = true;
    }

    if (found_prop) {
        std::string type_str;
        std::string target_str;
        if (!last_prop.values.empty()) {
            if (const auto* s = std::get_if<std::string>(&last_prop.values[0])) {
                type_str = *s;
            }
            if (last_prop.values.size() > 1) {
                if (const auto* s = std::get_if<std::string>(&last_prop.values[1])) {
                    target_str = *s;
                }
            }
        }

        builder->doc.sections.push_back(luxor::rc::section{
            .type = std::move(type_str),
            .name = std::move(last_prop.key),
            .target = std::move(target_str),
            .properties = {}
        });
        builder->active_section_idx = builder->doc.sections.size() - 1;
    }
}

void cfg_parser_on_block_finish(cfg_parser_context_t* ctx) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    builder->active_section_idx = builder->current_ini_section_idx;
}

void cfg_parser_on_property_finish(cfg_parser_context_t* ctx) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    if (!builder->current_key.empty()) {
        auto prop = luxor::rc::property{
            std::move(builder->current_key),
            std::move(builder->current_values)
        };
        if (auto* sec = builder->get_active_section()) {
            sec->properties.push_back(std::move(prop));
        } else {
            builder->doc.properties.push_back(std::move(prop));
        }
        builder->current_key.clear();
        builder->current_values.clear();
    }
}

void cfg_parser_on_syntax_error(cfg_parser_context_t* ctx) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    builder->has_error = true;
    builder->error_message = "Syntax error in configuration file";
}

void cfg_parser_on_parse_failure(cfg_parser_context_t* ctx) {
    auto* builder = static_cast<luxor::rc::cfg_builder*>(ctx->user_data);
    builder->has_error = true;
    if (builder->error_message.empty()) {
        builder->error_message = "Parsing failed";
    }
}
} // extern "C"
