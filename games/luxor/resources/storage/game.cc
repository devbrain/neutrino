//
// Created by igor on 11/10/2026.
//

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <sstream>

#include "luxor/resources/storage/game.hh"

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

        std::filesystem::path resolve_case_insensitive(const std::filesystem::path& input_path) {
            std::string clean_str;
            for (char c : input_path.string()) {
                clean_str.push_back(c == '\\' ? '/' : c);
            }
            std::filesystem::path p(clean_str);

            std::error_code ec;
            if (std::filesystem::exists(p, ec)) {
                return p;
            }

            std::filesystem::path current = p.is_absolute() ? p.root_path() : std::filesystem::current_path();
            std::filesystem::path rel = p.is_absolute() ? std::filesystem::relative(p, p.root_path(), ec) : p;
            if (ec) {
                rel = p;
            }

            for (const auto& part : rel) {
                std::string part_str = part.string();
                if (part_str.empty() || part_str == ".") continue;
                if (part_str == "..") {
                    current = current.parent_path();
                    continue;
                }

                std::string target_lower = to_lower(part_str);
                bool found = false;

                for (const auto& entry : std::filesystem::directory_iterator(current, ec)) {
                    if (ec) break;
                    if (to_lower(entry.path().filename().string()) == target_lower) {
                        current = entry.path();
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    return input_path; // Cannot resolve component, return original
                }
            }

            return current;
        }

        bool has_file_ci(const std::filesystem::path& base_dir, std::string_view rel_path) {
            std::filesystem::path current = base_dir;
            std::istringstream ss{std::string(rel_path)};
            std::string part;
            std::error_code ec;

            while (std::getline(ss, part, '/')) {
                if (part.empty() || part == ".") continue;
                std::string target_lower = to_lower(part);
                bool found = false;

                for (const auto& entry : std::filesystem::directory_iterator(current, ec)) {
                    if (ec) break;
                    if (to_lower(entry.path().filename().string()) == target_lower) {
                        current = entry.path();
                        found = true;
                        break;
                    }
                }

                if (!found) return false;
            }

            return std::filesystem::exists(current, ec);
        }

        bool string_contains_ci(std::string_view haystack, std::string_view needle) {
            std::string h = to_lower(haystack);
            std::string n = to_lower(needle);
            return h.find(n) != std::string::npos;
        }
    }

    std::string_view to_string(game_kind kind) noexcept {
        switch (kind) {
            case game_kind::luxor_1:                       return "luxor_1";
            case game_kind::luxor_amun_rising:             return "luxor_amun_rising";
            case game_kind::luxor_2:                       return "luxor_2";
            case game_kind::luxor_3:                       return "luxor_3";
            case game_kind::luxor_quest_for_the_afterlife: return "luxor_quest_for_the_afterlife";
            case game_kind::luxor_5th_passage:             return "luxor_5th_passage";
            case game_kind::luxor_hd:                      return "luxor_hd";
            case game_kind::luxor_amun_rising_hd:          return "luxor_amun_rising_hd";
            case game_kind::luxor_2_hd:                    return "luxor_2_hd";
            case game_kind::luxor_evolved:                 return "luxor_evolved";
            case game_kind::luxor_adventures:              return "luxor_adventures";
            case game_kind::luxor_mahjong:                 return "luxor_mahjong";
            case game_kind::luxor_bundle_pack:             return "luxor_bundle_pack";
            case game_kind::luxor_super_pack:              return "luxor_super_pack";
            case game_kind::unknown:
            default:                                       return "unknown";
        }
    }

    std::string_view to_display_name(game_kind kind) noexcept {
        switch (kind) {
            case game_kind::luxor_1:                       return "Luxor";
            case game_kind::luxor_amun_rising:             return "Luxor: Amun Rising";
            case game_kind::luxor_2:                       return "Luxor 2";
            case game_kind::luxor_3:                       return "Luxor 3";
            case game_kind::luxor_quest_for_the_afterlife: return "Luxor: Quest for the Afterlife";
            case game_kind::luxor_5th_passage:             return "Luxor: 5th Passage";
            case game_kind::luxor_hd:                      return "Luxor HD";
            case game_kind::luxor_amun_rising_hd:          return "Luxor: Amun Rising HD";
            case game_kind::luxor_2_hd:                    return "Luxor 2 HD";
            case game_kind::luxor_evolved:                 return "Luxor Evolved";
            case game_kind::luxor_adventures:              return "Luxor Adventures";
            case game_kind::luxor_mahjong:                 return "Luxor Mahjong";
            case game_kind::luxor_bundle_pack:             return "Luxor Bundle Pack";
            case game_kind::luxor_super_pack:              return "Luxor Super Pack";
            case game_kind::unknown:
            default:                                       return "Unknown";
        }
    }

    std::ostream& operator<<(std::ostream& os, game_kind kind) {
        return os << to_string(kind);
    }

    game_kind game::probe(const std::filesystem::path& path) {
        return probe_game(path);
    }

    game_kind probe_game(const std::filesystem::path& raw_path) {
        const std::filesystem::path resolved_path = resolve_case_insensitive(raw_path);
        std::error_code ec;

        if (!std::filesystem::exists(resolved_path, ec)) {
            return game_kind::unknown;
        }

        std::filesystem::path dir = resolved_path;
        std::string filename_lower;

        if (!std::filesystem::is_directory(resolved_path, ec)) {
            filename_lower = to_lower(resolved_path.filename().string());
            dir = resolved_path.parent_path();
        }

        // Direct check if filename itself identifies a game
        if (!filename_lower.empty()) {
            if (filename_lower == "luxor mahjong.exe") return game_kind::luxor_mahjong;
            if (filename_lower == "la.exe" || filename_lower == "resources.sfs") return game_kind::luxor_adventures;
            if (filename_lower == "luxor - 5th passage.exe" || filename_lower == "assets.pfp") return game_kind::luxor_5th_passage;
            if (filename_lower == "luxor - quest for the afterlife.exe" || filename_lower == "luxor4.pdb" || filename_lower == "data.npk") return game_kind::luxor_quest_for_the_afterlife;
            if (filename_lower == "luxor3.exe") return game_kind::luxor_3;
            if (filename_lower == "luxor_ar_hd_x86.exe") return game_kind::luxor_amun_rising_hd;
            if (filename_lower == "luxor_2_x86.exe") return game_kind::luxor_2_hd;
            if (filename_lower == "luxor_ev_x86.exe") return game_kind::luxor_evolved;
            if (filename_lower == "luxor hd.exe") return game_kind::luxor_hd;
            if (filename_lower == "luxor ar.exe" || filename_lower == "luxorar.ini") return game_kind::luxor_amun_rising;
            if (filename_lower == "luxor2.exe") return game_kind::luxor_2;
            if (filename_lower == "luxor.exe" || filename_lower == "luxor.ini") return game_kind::luxor_1;
            if (filename_lower == "bundle.exe") return game_kind::luxor_super_pack;
            if (filename_lower == "luxor bundle pack.exe") return game_kind::luxor_bundle_pack;
        }

        // Collect all entries in the directory (lowercased)
        std::unordered_set<std::string> lowers;
        std::vector<std::string> subdirs;

        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) break;
            std::string name = to_lower(entry.path().filename().string());
            lowers.insert(name);
            if (entry.is_directory(ec)) {
                subdirs.push_back(name);
            }
        }

        // 1. Luxor Mahjong
        if (lowers.count("luxor mahjong.exe") || lowers.count("mahjong.ini")) {
            return game_kind::luxor_mahjong;
        }

        // 2. Luxor Adventures
        if (lowers.count("la.exe") ||
            (lowers.count("l1") && lowers.count("l2")) ||
            lowers.count("resources.sfs") ||
            has_file_ci(dir, "l2/resources.sfs") ||
            (lowers.count("data.nhp") && lowers.count("dxrender.dll"))) {
            return game_kind::luxor_adventures;
        }

        // Check if current directory is inside LuxorAdventures (e.g. L1 or L2)
        if (dir.has_parent_path()) {
            std::error_code parent_ec;
            for (const auto& pentry : std::filesystem::directory_iterator(dir.parent_path(), parent_ec)) {
                if (to_lower(pentry.path().filename().string()) == "la.exe") {
                    return game_kind::luxor_adventures;
                }
            }
        }

        // 3. Luxor: 5th Passage
        if (lowers.count("luxor - 5th passage.exe") || has_file_ci(dir, "assets/assets.pfp")) {
            return game_kind::luxor_5th_passage;
        }

        // 4. Luxor: Quest for the Afterlife (Luxor 4)
        if (lowers.count("luxor - quest for the afterlife.exe") ||
            lowers.count("luxor4.pdb") ||
            lowers.count("data.npk")) {
            return game_kind::luxor_quest_for_the_afterlife;
        }

        // 5. Luxor 3
        if (lowers.count("luxor3.exe") ||
            (lowers.count("data.nhp") && !lowers.count("dxrender.dll"))) {
            return game_kind::luxor_3;
        }

        // 6. Luxor: Amun Rising HD
        if (lowers.count("luxor_ar_hd_x86.exe")) {
            return game_kind::luxor_amun_rising_hd;
        }

        // 7. Luxor 2 HD
        if (lowers.count("luxor_2_x86.exe")) {
            return game_kind::luxor_2_hd;
        }

        // 8. Luxor Evolved
        if (lowers.count("luxor_ev_x86.exe")) {
            return game_kind::luxor_evolved;
        }

        // 9. Luxor HD
        if (lowers.count("luxor hd.exe")) {
            return game_kind::luxor_hd;
        }

        // Check for other HD / Evolved executable names if variations exist
        for (const auto& f : lowers) {
            if (f.ends_with(".exe")) {
                if (string_contains_ci(f, "ar_hd") || (string_contains_ci(f, "amun") && string_contains_ci(f, "hd"))) {
                    return game_kind::luxor_amun_rising_hd;
                }
                if (string_contains_ci(f, "luxor_2") && string_contains_ci(f, "hd")) {
                    return game_kind::luxor_2_hd;
                }
                if (string_contains_ci(f, "luxor_ev") || string_contains_ci(f, "evolved")) {
                    return game_kind::luxor_evolved;
                }
            }
        }

        // 10. Luxor: Amun Rising (Original)
        if (lowers.count("luxor ar.exe") || lowers.count("luxorar.ini") || lowers.count("luxorar.log")) {
            return game_kind::luxor_amun_rising;
        }

        // 11. Luxor 2 (Original)
        if (lowers.count("luxor2.exe") || (lowers.count("data.mjz") && lowers.count("engine.dll"))) {
            return game_kind::luxor_2;
        }

        // 12. Luxor (Original / Luxor 1)
        if (lowers.count("luxor.exe") || lowers.count("luxor.ini") || lowers.count("luxor.log") ||
            has_file_ci(dir, "data/data.mjz") || has_file_ci(dir, "data/english.mjz")) {
            return game_kind::luxor_1;
        }

        // 13. Luxor Super Pack
        if (lowers.count("bundle.exe")) {
            return game_kind::luxor_super_pack;
        }
        size_t super_pack_hits = 0;
        for (const auto& sub : subdirs) {
            if (sub == "luxor3" || sub == "luxorhd" || sub == "luxor2hd" ||
                sub == "luxoradventures" || sub == "luxor5thpassage" ||
                sub == "luxoramunrisinghd" || sub == "luxorevolved" ||
                sub == "luxormahjong" || sub == "luxorquestfortheafterlife") {
                ++super_pack_hits;
            }
        }
        if (super_pack_hits >= 2) {
            return game_kind::luxor_super_pack;
        }

        // 14. Luxor Bundle Pack
        if (lowers.count("luxor bundle pack.exe")) {
            return game_kind::luxor_bundle_pack;
        }
        bool has_sub_luxor = false;
        bool has_sub_luxorar = false;
        for (const auto& sub : subdirs) {
            if (sub == "luxor") has_sub_luxor = true;
            if (sub == "luxorar") has_sub_luxorar = true;
        }
        if (has_sub_luxor && has_sub_luxorar) {
            return game_kind::luxor_bundle_pack;
        }

        return game_kind::unknown;
    }
}
