#include <doctest/doctest.h>
#include <luxor/resources/parser/depth_parser.hh>

#include <filesystem>
#include <fstream>

namespace {
    constexpr std::string_view EMBEDDED_DEPTHS_TXT =
        "\"GameBackground\"\n"
        "\"Debug_SplinePathBelow\"\n"
        "\"GamePieceHShadow\"\n"
        "\"GamePieceHidden\"\n"
        "\"GamePieceHiddenOverlay\"\n"
        "\"GamePieceHiddenPsys\"\n"
        "\"GameBackgroundSprites\"\n"
        "\"Debug_SplinePathAbove\"\n"
        "\"Debug_SplinePathText\"\n"
        "\"GamePlayerBaseShadow\"\n"
        "\"GamePieceNShadow\"\n"
        "\"GamePlayerBase\"\n"
        "\"GameSpeedShotPsys\"\n"
        "\"GamePieceNormal\"\n"
        "\"GamePieceNormalPsys\"\n"
        "\"GamePieceDebugText\"\n"
        "\"GameBullet\"\n"
        "\"GameBulletPsys\"\n"
        "\"GameLevelWarningPsys\"\n"
        "\"GamePowerups\"\n"
        "\"GameCollapses\"\n"
        "\"GameScores\"\n"
        "\"BannerBackground\"\n"
        "\"Banner\"\n"
        "\"BannerControls\"\n"
        "\"BannerControls2\"\n"
        "\"HudBackground\"\n"
        "\"Hud\"\n"
        "\"HudControls\"\n"
        "\"HudControls2\"\n"
        "\"HudForegroundMask\"\n"
        "\"MenuBackground\"\n"
        "\"MenuControls\"\n"
        "\"MenuControls2\"\n"
        "\"MenuBackgroundMask\"\n"
        "\"MenuDialog\"\n"
        "\"MenuDialogControls\"\n"
        "\"MenuDialogControls2\"\n"
        "\"MenuDialog2\"\n"
        "\"MenuDialog2Controls\"\n"
        "\"MenuDialog2Controls2\"\n"
        "\"MenuDialog3\"\n"
        "\"MenuDialog3Controls\"\n"
        "\"MenuDialog3Controls2\"\n"
        "\"MenuDialog4\"\n"
        "\"MenuDialog4Controls\"\n"
        "\"MenuDialog4Controls2\"\n"
        "\"PHYSICS\"\n"
        "\"OTHER\"\n";

    constexpr std::string_view EMBEDDED_STAGE_NAMES_TXT =
        "The Quest Begins\n"
        "Chasing the Caravan\n"
        "To The Pyramids\n"
        "Clue in the Ruins\n"
        "The Oasis of Isis\n"
        "Journey to the Sphinx\n"
        "Pillars of Karnak\n"
        "After the High Priest\n"
        "City of the Dead\n"
        "Up the Cataracts\n"
        "The Hermits Advice\n"
        "Temple of the Goddess\n"
        "The Wrath of Set\n";
} // namespace

TEST_CASE("depth_table parsing from embedded depths.txt") {
    auto res = luxor::rc::parse_depths(EMBEDDED_DEPTHS_TXT);
    REQUIRE(res.has_value());
    const auto& dt = *res;

    CHECK(dt.size() == 49);
    CHECK_FALSE(dt.empty());

    // Layer 0 is GameBackground
    CHECK(dt[0] == "GameBackground");
    CHECK(dt.get_name(0) == "GameBackground");
    CHECK(dt.get_index("GameBackground") == 0);
    CHECK(dt.has("GameBackground"));

    // Case-insensitivity test (matches ui2.dll stricmp)
    CHECK(dt.get_index("gamebackground") == 0);
    CHECK(dt.has("gamebackground"));
    CHECK(dt.get_index("GAMEBACKGROUND") == 0);

    // Layer 2 is GamePieceHShadow
    CHECK(dt.get_index("GamePieceHShadow") == 2);
    CHECK(dt.get_name(2) == "GamePieceHShadow");

    // Layer 6 is GameBackgroundSprites
    CHECK(dt.get_index("GameBackgroundSprites") == 6);

    // Layer 13 is GamePieceNormal
    CHECK(dt.get_index("GamePieceNormal") == 13);
    CHECK(dt.get_index("gamepiecenormal") == 13);

    // Last layer is OTHER
    CHECK(dt[48] == "OTHER");
    CHECK(dt.get_index("OTHER") == 48);

    // Out of bounds and non-existent lookups
    CHECK_FALSE(dt.has("NonExistentLayer"));
    CHECK(dt.get_index("NonExistentLayer") == std::nullopt);
    CHECK(dt.get_name(99) == std::nullopt);

    // Special keyword #parent
    CHECK(luxor::rc::depth_table::is_parent("#parent"));
    CHECK(luxor::rc::depth_table::is_parent("#Parent"));
    CHECK(luxor::rc::depth_table::is_parent("#PARENT"));
    CHECK_FALSE(luxor::rc::depth_table::is_parent("GameBackground"));
    CHECK_FALSE(luxor::rc::depth_table::is_parent(""));
}

TEST_CASE("depth_table comments, unquoted lines, and formatting") {
    const std::string_view sample_with_comments =
        "// Depth Layer Configuration\n"
        "; Luxor render order\n"
        "\n"
        "\"LayerA\" // first layer\n"
        "LayerB ; second unquoted layer\n"
        "   \"LayerC\"   \n"
        "\n"
        "LayerD\n";

    auto res = luxor::rc::parse_depths(sample_with_comments);
    REQUIRE(res.has_value());
    CHECK(res->size() == 4);
    CHECK((*res)[0] == "LayerA");
    CHECK((*res)[1] == "LayerB");
    CHECK((*res)[2] == "LayerC");
    CHECK((*res)[3] == "LayerD");
    CHECK(res->get_index("layerc") == 2);
}

TEST_CASE("depth_table error validation") {
    SUBCASE("duplicate layer name") {
        const std::string_view dupes =
            "\"Background\"\n"
            "\"Foreground\"\n"
            "\"background\"\n"; // duplicate case-insensitive
        auto res = luxor::rc::parse_depths(dupes);
        CHECK_FALSE(res.has_value());
    }

    SUBCASE("unclosed quote") {
        const std::string_view unclosed =
            "\"ValidLayer\"\n"
            "\"BrokenLayer\n";
        auto res = luxor::rc::parse_depths(unclosed);
        CHECK_FALSE(res.has_value());
    }

    SUBCASE("exceeded maximum capacity (96 layers)") {
        std::string too_many;
        for (int i = 0; i < 97; ++i) {
            too_many += "\"Layer_" + std::to_string(i) + "\"\n";
        }
        auto res = luxor::rc::parse_depths(too_many);
        CHECK_FALSE(res.has_value());
    }
}

TEST_CASE("string_list parsing from embedded stage_names.txt") {
    auto res = luxor::rc::parse_string_list(EMBEDDED_STAGE_NAMES_TXT);
    REQUIRE(res.has_value());
    const auto& sl = *res;

    CHECK(sl.size() == 13);
    CHECK_FALSE(sl.empty());

    CHECK(sl[0] == "The Quest Begins");
    CHECK(sl[1] == "Chasing the Caravan");
    CHECK(sl[12] == "The Wrath of Set");

    // Search case-insensitive
    CHECK(sl.has("the quest begins", false));
    CHECK(sl.find_index("the quest begins", false) == 0);

    // Search case-sensitive
    CHECK_FALSE(sl.has("the quest begins", true));
    CHECK(sl.find_index("the quest begins", true) == std::nullopt);
    CHECK(sl.has("The Quest Begins", true));
    CHECK(sl.find_index("The Quest Begins", true) == 0);
}

TEST_CASE("depth_table and string_list to_json serialization") {
    SUBCASE("depth_table JSON") {
        auto res = luxor::rc::parse_depths("\"Bg\"\n\"Fg\"\n");
        REQUIRE(res.has_value());
        auto j = res->to_json();

        REQUIRE(j.contains("layers"));
        REQUIRE(j["layers"].is_array());
        CHECK(j["layers"].size() == 2);
        CHECK(j["layers"][0]["index"] == 0);
        CHECK(j["layers"][0]["name"] == "Bg");
        CHECK(j["layers"][1]["index"] == 1);
        CHECK(j["layers"][1]["name"] == "Fg");
        CHECK(j["total_layers"] == 2);

        // ADL serialization
        nlohmann::json j_adl = *res;
        CHECK(j_adl == j);
    }

    SUBCASE("string_list JSON") {
        auto res = luxor::rc::parse_string_list("ItemA\nItemB\nItemC\n");
        REQUIRE(res.has_value());
        auto j = res->to_json();

        REQUIRE(j.contains("items"));
        REQUIRE(j["items"].is_array());
        CHECK(j["items"].size() == 3);
        CHECK(j["items"][0] == "ItemA");
        CHECK(j["count"] == 3);

        nlohmann::json j_adl = *res;
        CHECK(j_adl == j);
    }
}

TEST_CASE("depth_table and string_list file parsing") {
    const auto temp_file = std::filesystem::temp_directory_path() / "luxor_test_depths_temp.txt";
    {
        std::ofstream ofs(temp_file);
        ofs << EMBEDDED_DEPTHS_TXT;
    }

    auto dt = luxor::rc::parse_depths_file(temp_file);
    REQUIRE(dt.has_value());
    CHECK(dt->size() == 49);

    auto sl = luxor::rc::parse_string_list_file(temp_file);
    REQUIRE(sl.has_value());
    CHECK(sl->size() == 49);

    std::filesystem::remove(temp_file);

    // Negative: non-existent file
    CHECK_FALSE(luxor::rc::parse_depths_file("/non/existent/path/999.txt").has_value());
    CHECK_FALSE(luxor::rc::parse_string_list_file("/non/existent/path/999.txt").has_value());
}
