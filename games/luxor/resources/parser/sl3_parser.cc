//
// Created by igor on 14/10/2026.
//

#include "sl3_parser.hh"

#include <algorithm>
#include <cctype>

namespace luxor::rc {
    namespace {
        std::string to_lower(std::string_view s) {
            std::string res;
            res.reserve(s.size());
            for (char c : s) {
                res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
            return res;
        }

        std::string normalize_slashes(std::string_view path) {
            std::string res;
            res.reserve(path.size());
            for (char c : path) {
                res.push_back(c == '\\' ? '/' : c);
            }
            return res;
        }
    }

    nlohmann::json sl3_sample::to_json() const {
        nlohmann::json j;
        j["name"] = name;
        j["file"] = file;
        j["volume"] = volume;
        if (!properties.empty()) {
            auto props = nlohmann::json::array();
            for (const auto& p : properties) {
                props.push_back(p.to_json());
            }
            j["properties"] = std::move(props);
        }
        return j;
    }

    nlohmann::json sl3_stream::to_json() const {
        nlohmann::json j;
        j["name"] = name;
        j["file"] = file;
        j["loop"] = loop;
        if (!properties.empty()) {
            auto props = nlohmann::json::array();
            for (const auto& p : properties) {
                props.push_back(p.to_json());
            }
            j["properties"] = std::move(props);
        }
        return j;
    }

    nlohmann::json sl3_playlist::to_json() const {
        nlohmann::json j;
        j["name"] = name;
        j["flags"] = flags;
        j["tracks"] = tracks;
        if (!properties.empty()) {
            auto props = nlohmann::json::array();
            for (const auto& p : properties) {
                props.push_back(p.to_json());
            }
            j["properties"] = std::move(props);
        }
        return j;
    }

    nlohmann::json sl3_file::to_json() const {
        nlohmann::json j;

        auto smp_arr = nlohmann::json::array();
        for (const auto& n : sample_order) {
            auto it = samples.find(to_lower(n));
            if (it != samples.end()) {
                smp_arr.push_back(it->second.to_json());
            }
        }
        j["samples"] = std::move(smp_arr);

        auto str_arr = nlohmann::json::array();
        for (const auto& n : stream_order) {
            auto it = streams.find(to_lower(n));
            if (it != streams.end()) {
                str_arr.push_back(it->second.to_json());
            }
        }
        j["streams"] = std::move(str_arr);

        auto pl_arr = nlohmann::json::array();
        for (const auto& n : playlist_order) {
            auto it = playlists.find(to_lower(n));
            if (it != playlists.end()) {
                pl_arr.push_back(it->second.to_json());
            }
        }
        j["playlists"] = std::move(pl_arr);

        return j;
    }

    void to_json(nlohmann::json& j, const sl3_sample& sample) {
        j = sample.to_json();
    }

    void to_json(nlohmann::json& j, const sl3_stream& stream) {
        j = stream.to_json();
    }

    void to_json(nlohmann::json& j, const sl3_playlist& playlist) {
        j = playlist.to_json();
    }

    void to_json(nlohmann::json& j, const sl3_file& file) {
        j = file.to_json();
    }

    bool sl3_file::has_sample(std::string_view name) const {
        return samples.find(to_lower(name)) != samples.end();
    }

    bool sl3_file::has_stream(std::string_view name) const {
        return streams.find(to_lower(name)) != streams.end();
    }

    bool sl3_file::has_playlist(std::string_view name) const {
        return playlists.find(to_lower(name)) != playlists.end();
    }

    const sl3_sample* sl3_file::find_sample(std::string_view name) const {
        auto it = samples.find(to_lower(name));
        return it != samples.end() ? &it->second : nullptr;
    }

    const sl3_stream* sl3_file::find_stream(std::string_view name) const {
        auto it = streams.find(to_lower(name));
        return it != streams.end() ? &it->second : nullptr;
    }

    const sl3_playlist* sl3_file::find_playlist(std::string_view name) const {
        auto it = playlists.find(to_lower(name));
        return it != playlists.end() ? &it->second : nullptr;
    }

    expected<sl3_file, std::string> parse_sl3(const config_document& doc) {
        sl3_file file;

        // 1. Process flat properties (simple one-line samples/streams)
        for (const auto& prop : doc.properties) {
            if (prop.values.size() >= 2) {
                const auto* type_ptr = std::get_if<std::string>(&prop.values[0]);
                const auto* path_ptr = std::get_if<std::string>(&prop.values[1]);
                if (type_ptr && path_ptr) {
                    std::string type_lower = to_lower(*type_ptr);
                    if (type_lower == "sample") {
                        sl3_sample s;
                        s.name = prop.key;
                        s.file = normalize_slashes(*path_ptr);
                        s.volume = 1.0f;
                        std::string key = to_lower(s.name);
                        file.sample_order.push_back(s.name);
                        file.samples.emplace(std::move(key), std::move(s));
                    } else if (type_lower == "stream") {
                        sl3_stream st;
                        st.name = prop.key;
                        st.file = normalize_slashes(*path_ptr);
                        st.loop = true;
                        std::string key = to_lower(st.name);
                        file.stream_order.push_back(st.name);
                        file.streams.emplace(std::move(key), std::move(st));
                    }
                }
            }
        }

        // 2. Process sections (blocks with configurations)
        for (const auto& sec : doc.sections) {
            std::string type_lower = to_lower(sec.type);
            if (type_lower == "sample") {
                sl3_sample s;
                s.name = sec.name;
                s.file = normalize_slashes(sec.target);
                s.volume = static_cast<float>(sec.get_number("volume", 1.0));
                s.properties = sec.properties;

                std::string key = to_lower(s.name);
                auto it = file.samples.find(key);
                if (it == file.samples.end()) {
                    file.sample_order.push_back(s.name);
                    file.samples.emplace(std::move(key), std::move(s));
                } else {
                    it->second = std::move(s);
                }
            } else if (type_lower == "stream") {
                sl3_stream st;
                st.name = sec.name;
                st.file = normalize_slashes(sec.target);
                st.loop = sec.get_bool("loop", true);
                st.properties = sec.properties;

                std::string key = to_lower(st.name);
                auto it = file.streams.find(key);
                if (it == file.streams.end()) {
                    file.stream_order.push_back(st.name);
                    file.streams.emplace(std::move(key), std::move(st));
                } else {
                    it->second = std::move(st);
                }
            } else if (type_lower == "playlist") {
                sl3_playlist pl;
                pl.name = sec.name;
                pl.flags = std::string(sec.get_string("flags", ""));
                for (const auto& p : sec.properties) {
                    if (p.key == "track") {
                        for (const auto& v : p.values) {
                            if (const auto* str = std::get_if<std::string>(&v)) {
                                pl.tracks.push_back(*str);
                            }
                        }
                    }
                }
                pl.properties = sec.properties;

                std::string key = to_lower(pl.name);
                file.playlist_order.push_back(pl.name);
                file.playlists.emplace(std::move(key), std::move(pl));
            }
        }

        return file;
    }

    expected<sl3_file, std::string> parse_sl3(std::string_view content) {
        auto doc = parse_config(content);
        if (!doc) {
            return unexpected(doc.error());
        }
        return parse_sl3(*doc);
    }

    expected<sl3_file, std::string> parse_sl3_file(const std::filesystem::path& path) {
        auto doc = parse_config_file(path);
        if (!doc) {
            return unexpected(doc.error());
        }
        return parse_sl3(*doc);
    }

} // namespace luxor::rc
