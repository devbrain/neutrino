//
// Created by igor on 11/10/2026.
//

#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <iosfwd>

namespace luxor::rc {
    enum class game_kind {
        unknown = 0,
        luxor_1,                       // Luxor (2005)
        luxor_amun_rising,             // Luxor: Amun Rising (2006)
        luxor_2,                       // Luxor 2 (2006)
        luxor_3,                       // Luxor 3 (2007)
        luxor_quest_for_the_afterlife, // Luxor: Quest for the Afterlife / Luxor 4 (2008)
        luxor_5th_passage,             // Luxor: 5th Passage (2010)
        luxor_hd,                      // Luxor HD (2011)
        luxor_amun_rising_hd,          // Luxor: Amun Rising HD (2012)
        luxor_2_hd,                    // Luxor 2 HD (2013)
        luxor_evolved,                 // Luxor Evolved (2012)
        luxor_adventures,              // Luxor Adventures (2009)
        luxor_mahjong,                 // Luxor Mahjong (2006)
        luxor_bundle_pack,             // Luxor Bundle Pack
        luxor_super_pack               // Luxor Super Pack
    };

    [[nodiscard]] std::string_view to_string(game_kind kind) noexcept;
    [[nodiscard]] std::string_view to_display_name(game_kind kind) noexcept;

    std::ostream& operator<<(std::ostream& os, game_kind kind);

    class game {
        public:
            [[nodiscard]] static game_kind probe(const std::filesystem::path& path);
    };

    [[nodiscard]] game_kind probe_game(const std::filesystem::path& path);
}
