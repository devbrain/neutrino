#include <doctest/doctest.h>
#include <luxor/resources/storage/game.hh>

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdlib>

TEST_CASE("game probe detection") {
    using luxor::rc::game_kind;
    using luxor::rc::probe_game;
    using luxor::rc::game;

    const auto temp_root = std::filesystem::temp_directory_path() / "luxor_test_game_probe";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);

    auto create_mock_game = [&](const std::string& folder, const std::string& sig_file) {
        auto d = temp_root / folder;
        std::filesystem::create_directories(d);
        std::ofstream(d / sig_file) << "mock";
        return d;
    };

    struct mock_test_entry {
        std::string folder;
        std::string sig_file;
        game_kind expected;
    };

    const std::vector<mock_test_entry> mock_tests = {
        {"Luxor", "Luxor.exe", game_kind::luxor_1},
        {"LuxorAR", "Luxor AR.exe", game_kind::luxor_amun_rising},
        {"Luxor 2", "Luxor2.exe", game_kind::luxor_2},
        {"Luxor 3", "Luxor3.exe", game_kind::luxor_3},
        {"LuxorQuestfortheAfterlife", "Luxor - Quest for the Afterlife.exe", game_kind::luxor_quest_for_the_afterlife},
        {"Luxor5thPassage", "Luxor - 5th Passage.exe", game_kind::luxor_5th_passage},
        {"LuxorHD", "Luxor HD.exe", game_kind::luxor_hd},
        {"LuxorAmunRisingHD", "Luxor_AR_HD_x86.exe", game_kind::luxor_amun_rising_hd},
        {"Luxor2HD", "Luxor_2_x86.exe", game_kind::luxor_2_hd},
        {"LuxorEvolved", "Luxor_EV_x86.exe", game_kind::luxor_evolved},
        {"LuxorAdventures", "la.exe", game_kind::luxor_adventures},
        {"LuxorMahjong", "Luxor Mahjong.exe", game_kind::luxor_mahjong},
        {"LuxorBundlePack", "Luxor Bundle Pack.exe", game_kind::luxor_bundle_pack},
        {"LuxorSuperPack", "Bundle.exe", game_kind::luxor_super_pack},
    };

    for (const auto& t : mock_tests) {
        auto path = create_mock_game(t.folder, t.sig_file);
        INFO("Probing mock " << path);
        auto k1 = probe_game(path);
        auto k2 = game::probe(path);
        CHECK(k1 == t.expected);
        CHECK(k2 == t.expected);

        // Probing direct executable path
        auto k_exe = probe_game(path / t.sig_file);
        CHECK(k_exe == t.expected);
    }

    SUBCASE("Case-insensitivity and slash normalization") {
        auto p3 = temp_root / "luxor 3";
        CHECK(probe_game(p3) == game_kind::luxor_3);

        auto p2 = temp_root / "LUXOR 2";
        CHECK(probe_game(p2) == game_kind::luxor_2);

        auto p_mahjong = temp_root / "LuxorMahjong";
        CHECK(probe_game(p_mahjong) == game_kind::luxor_mahjong);
    }

    SUBCASE("Negative probe checks") {
        CHECK(probe_game("/non/existent/game/path/12345") == game_kind::unknown);
        CHECK(probe_game("/tmp") == game_kind::unknown);
    }

    SUBCASE("Probing real installed games if available") {
        const char* env_path = std::getenv("LUXOR_GAME_ROOT");
        const std::filesystem::path real_root = env_path ? env_path : "/home/igor/games/luxor";
        if (std::filesystem::exists(real_root)) {
            const std::vector<std::pair<std::string, game_kind>> real_tests = {
                {"Luxor Bundle Pack/Luxor", game_kind::luxor_1},
                {"Luxor Bundle Pack/LuxorAR", game_kind::luxor_amun_rising},
                {"Luxor 2", game_kind::luxor_2},
                {"Luxor Super Pack/Luxor3", game_kind::luxor_3},
                {"Luxor Super Pack/LuxorQuestfortheAfterlife", game_kind::luxor_quest_for_the_afterlife},
                {"Luxor Super Pack/Luxor5thPassage", game_kind::luxor_5th_passage},
                {"Luxor Super Pack/LuxorHD", game_kind::luxor_hd},
                {"Luxor Super Pack/LuxorAmunRisingHD", game_kind::luxor_amun_rising_hd},
                {"Luxor Super Pack/Luxor2HD", game_kind::luxor_2_hd},
                {"Luxor Super Pack/LuxorEvolved", game_kind::luxor_evolved},
                {"Luxor Super Pack/LuxorAdventures", game_kind::luxor_adventures},
                {"Luxor Super Pack/LuxorMahjong", game_kind::luxor_mahjong},
                {"Luxor Bundle Pack", game_kind::luxor_bundle_pack},
                {"Luxor Super Pack", game_kind::luxor_super_pack},
            };

            for (const auto& [sub, expected] : real_tests) {
                auto p = real_root / sub;
                if (std::filesystem::exists(p)) {
                    CHECK(probe_game(p) == expected);
                    CHECK(game::probe(p) == expected);
                }
            }
        }
    }

    std::filesystem::remove_all(temp_root);
}
