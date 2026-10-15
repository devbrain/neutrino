#include <doctest/doctest.h>
#include <luxor/resources/storage/archive.hh>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {
    std::filesystem::path find_game_archive(const std::string& rel_path) {
        if (const char* env = std::getenv("LUXOR_GAME_ROOT")) {
            auto p = std::filesystem::path(env) / rel_path;
            if (std::filesystem::exists(p)) return p;
        }
        const std::filesystem::path default_root = "/home/igor/games/luxor";
        auto p = default_root / rel_path;
        if (std::filesystem::exists(p)) return p;
        return {};
    }

    void verify_archive(const std::string& rel_path, const std::string& test_file) {
        auto p = find_game_archive(rel_path);
        if (p.empty()) {
            return;
        }

        INFO("Testing archive: " << p);
        CHECK(luxor::rc::archive::probe(p));
        CHECK(luxor::rc::probe(p));

        std::ifstream ifs(p, std::ios::binary);
        REQUIRE(ifs.is_open());

        CHECK(luxor::rc::archive::probe(ifs));
        CHECK(luxor::rc::probe(ifs));

        auto a = luxor::rc::archive::load(ifs);
        REQUIRE(a != nullptr);

        auto a_path = luxor::rc::archive::load(p);
        REQUIRE(a_path != nullptr);

        auto files = a->list_files();
        CHECK_FALSE(files.empty());

        CHECK_FALSE(a->has_file("non_existent_file_12345.xyz"));
        CHECK_FALSE(a->stat("non_existent_file_12345.xyz").has_value());
        CHECK(a->read("non_existent_file_12345.xyz").empty());

        if (!test_file.empty()) {
            CHECK(a->has_file(test_file));
            auto info = a->stat(test_file);
            REQUIRE(info.has_value());
            CHECK(info->size > 0);

            auto data = a->read(test_file);
            CHECK(data.size() == info->size);
        }
    }

    void verify_dump(const std::string& rel_path, const std::filesystem::path& out_dir) {
        auto p = find_game_archive(rel_path);
        if (p.empty()) {
            return;
        }

        INFO("Testing dump for: " << p << " to " << out_dir);
        std::ifstream ifs(p, std::ios::binary);
        REQUIRE(ifs.is_open());

        auto a = luxor::rc::archive::load(ifs);
        REQUIRE(a != nullptr);

        std::filesystem::remove_all(out_dir);
        luxor::rc::dump(a, out_dir);

        auto files = a->list_files();
        for (const auto& f : files) {
            auto dest = out_dir / f.path;
            if (f.is_directory) {
                CHECK(std::filesystem::is_directory(dest));
            } else {
                CHECK(std::filesystem::is_regular_file(dest));
                CHECK(std::filesystem::file_size(dest) == f.size);
            }
        }

        std::filesystem::remove_all(out_dir);
    }
} // namespace

TEST_CASE("embedded mjz archive loading and extraction") {
    // A minimal valid MJZ archive containing 1 file: "test.txt" ("Hello Luxor!")
    const uint8_t EMBEDDED_MJZ[] = {
        0x4d, 0x4a, 0x5a, 0x30, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
        0x08, 0x74, 0x65, 0x73, 0x74, 0x2e, 0x74, 0x78, 0x74, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x4c,
        0x75, 0x78, 0x6f, 0x72, 0x21
    };

    std::string archive_str(reinterpret_cast<const char*>(EMBEDDED_MJZ), sizeof(EMBEDDED_MJZ));
    std::istringstream iss(archive_str);

    CHECK(luxor::rc::archive::probe(iss));
    auto ar = luxor::rc::archive::load(iss);
    REQUIRE(ar != nullptr);

    CHECK(ar->has_file("test.txt"));
    auto st = ar->stat("test.txt");
    REQUIRE(st.has_value());
    CHECK(st->size == 12);
    CHECK_FALSE(st->is_directory);

    auto data = ar->read("test.txt");
    CHECK(std::string(data.begin(), data.end()) == "Hello Luxor!");

    const auto temp_dump = std::filesystem::temp_directory_path() / "luxor_test_mjz_dump";
    std::filesystem::remove_all(temp_dump);
    ar->dump(temp_dump);
    CHECK(std::filesystem::is_regular_file(temp_dump / "test.txt"));
    std::filesystem::remove_all(temp_dump);
}

TEST_CASE("archive negative probe") {
    std::istringstream not_archive("This is definitely not a Luxor archive file of any format.");
    CHECK_FALSE(luxor::rc::archive::probe(not_archive));
    CHECK_FALSE(luxor::rc::probe(not_archive));

    std::istringstream empty_stream("");
    CHECK_FALSE(luxor::rc::archive::probe(empty_stream));

    CHECK_FALSE(luxor::rc::archive::probe("/non/existent/path/12345.dat"));
    CHECK_FALSE(luxor::rc::probe("/non/existent/path/12345.dat"));
}

TEST_CASE("archive formats loading and reading") {
    SUBCASE("NHP archive (Luxor 3)") {
        verify_archive("Luxor Super Pack/Luxor3/data.nhp", "data/AnimDefs.txt");
        verify_archive("Luxor Super Pack/Luxor3/data.nhp", "data/gfx/balls/black/ball0000.png");
    }

    SUBCASE("NHP archive (Luxor Adventures L1)") {
        verify_archive("Luxor Super Pack/LuxorAdventures/L1/data.nhp", "data/AnimDefs.txt");
    }

    SUBCASE("NPK archive (Luxor: Quest for the Afterlife)") {
        verify_archive("Luxor Super Pack/LuxorQuestfortheAfterlife/data.npk", "AnimDefs.txt");
    }

    SUBCASE("MJZ archive (Luxor 2)") {
        verify_archive("Luxor 2/data.mjz", "data/shooter_path.obj");
    }

    SUBCASE("SFS archive (Luxor Adventures L2)") {
        verify_archive("Luxor Super Pack/LuxorAdventures/L2/resources.sfs", "Attributes.xml");
        verify_archive("Luxor Super Pack/LuxorAdventures/L2/resources.sfs", "win/UnlimHOStatisticsScreen.win");
        verify_archive("Luxor Super Pack/LuxorAdventures/L2/resources.sfs", "sfx/music/theme_classic.ogg");
    }

    SUBCASE("PFP archive (Luxor: 5th Passage)") {
        verify_archive("Luxor Super Pack/Luxor5thPassage/assets/assets.pfp", "audio/music/blast_mode.ogg");
        verify_archive("Luxor Super Pack/Luxor5thPassage/assets/assets.pfp", "backgrounds/bar.png");
        verify_archive("Luxor Super Pack/Luxor5thPassage/assets/assets.pfp", "blast_mode_levels/baran/easy.xml");
    }
}

TEST_CASE("archive dumping") {
    const auto temp_root = std::filesystem::temp_directory_path();
    SUBCASE("Dump MJZ") {
        verify_dump("Luxor 2/data.mjz", temp_root / "luxor_test_dump_mjz");
    }

    SUBCASE("Dump NPK") {
        verify_dump("Luxor Super Pack/LuxorQuestfortheAfterlife/data.npk", temp_root / "luxor_test_dump_npk");
    }

    SUBCASE("Dump NHP") {
        verify_dump("Luxor Super Pack/Luxor3/data.nhp", temp_root / "luxor_test_dump_nhp");
    }

    SUBCASE("Dump SFS") {
        verify_dump("Luxor Super Pack/LuxorAdventures/L2/resources.sfs", temp_root / "luxor_test_dump_sfs");
    }

    SUBCASE("Dump PFP") {
        verify_dump("Luxor Super Pack/Luxor5thPassage/assets/assets.pfp", temp_root / "luxor_test_dump_pfp");
    }
}
