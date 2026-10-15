#include <doctest/doctest.h>
#include <luxor/resources/parser/cfg_parser.hh>

#include <cmath>
#include <filesystem>
#include <string_view>

TEST_CASE("level file (.lvl) parsing") {
    const std::string_view sample_lvl =
        "// Sample Level Configuration\n"
        "mapFile = \"URAEUS NEFERTARI\"\n"
        "\n"
        "// Ball Colors\n"
        "spawnColor_1 = true\n"
        "spawnColor_5 = false\n"
        "spawnStreak = 225\n"
        "\n"
        "// Vise Speed\n"
        "viseMaxSpeed = 800.0\n"
        "viseSpeedMaxBzLerp = 0.9 0.9\n"
        "viseSpeedMidBzLerp = 0.25 0.75\n"
        "gameMode = survival\n"
        "leadDot = .5\n"
        "sciNum = 1.0e-3\n"
        "emptyFlag =\n";

    auto lvl_opt = luxor::rc::parse_level(sample_lvl);
    REQUIRE(lvl_opt.has_value());
    const auto& lvl = *lvl_opt;

    CHECK(lvl.has("mapFile"));
    CHECK(lvl.get_string("mapFile") == "URAEUS NEFERTARI");

    // Ident value test
    CHECK(lvl.has("gameMode"));
    CHECK(lvl.get_string("gameMode") == "survival");

    // Number format tests
    CHECK(lvl.has("leadDot"));
    CHECK(lvl.get_number("leadDot") == 0.5);
    CHECK(lvl.has("sciNum"));
    CHECK(std::abs(lvl.get_number("sciNum") - 0.001) < 1e-6);

    // Empty property preservation
    CHECK(lvl.has("emptyFlag"));
    REQUIRE(lvl.get_values("emptyFlag") != nullptr);
    CHECK(lvl.get_values("emptyFlag")->empty());

    CHECK(lvl.has("spawnColor_1"));
    CHECK(lvl.get_bool("spawnColor_1") == true);

    CHECK(lvl.has("spawnColor_5"));
    CHECK(lvl.get_bool("spawnColor_5") == false);

    CHECK(lvl.has("spawnStreak"));
    CHECK(lvl.get_int("spawnStreak") == 225);

    CHECK(lvl.has("viseMaxSpeed"));
    CHECK(lvl.get_number("viseMaxSpeed") == 800.0);

    CHECK(lvl.has("viseSpeedMaxBzLerp"));
    auto bz = lvl.get_numbers("viseSpeedMaxBzLerp");
    REQUIRE(bz.size() == 2);
    CHECK(bz[0] == 0.9);
    CHECK(bz[1] == 0.9);

    auto mid_bz = lvl.get_numbers("viseSpeedMidBzLerp");
    REQUIRE(mid_bz.size() == 2);
    CHECK(mid_bz[0] == 0.25);
    CHECK(mid_bz[1] == 0.75);

    // Negative tests
    CHECK_FALSE(lvl.has("non_existent_key"));
    CHECK(lvl.get_string("non_existent_key", "default_val") == "default_val");
    CHECK(lvl.get_bool("non_existent_key", true) == true);
    CHECK(lvl.get_number("non_existent_key", 42.0) == 42.0);

    SUBCASE("syntax error reporting with expected") {
        auto bad_parse = luxor::rc::parse_level("invalid = = =");
        CHECK_FALSE(bad_parse.has_value());
        CHECK_FALSE(bad_parse.error().empty());
    }

    SUBCASE("parse embedded level_1_1.lvl") {
        constexpr std::string_view EMBEDDED_LEVEL_1_1 =
            "mapFile = \"URAEUS NEFERTARI\"\n"
            "\n"
            "// Ball Colors\n"
            "spawnColor_1 = true\n"
            "spawnColor_2 = true\n"
            "spawnColor_3 = true\n"
            "spawnColor_4 = true\n"
            "spawnColor_5 = false\n"
            "spawnColor_6 = false\n"
            "spawnColor_7 = false\n"
            "spawnStreak = 225\n"
            "\n"
            "// Powerups\n"
            "powerup_reverse   = true\n"
            "powerup_slow      = true\n"
            "powerup_stop      = true\n"
            "powerup_shotspeed = true\n"
            "powerup_lightning = true\n"
            "powerup_bomb      = true\n"
            "powerup_colorbomb = true\n"
            "powerup_wild      = true\n"
            "\n"
            "// Rewards\n"
            "reward_gem_1      = true\n"
            "reward_gem_2      = true\n"
            "reward_gem_3      = true\n"
            "reward_gem_4      = false\n"
            "reward_gem_5      = false\n"
            "reward_gem_6      = false\n"
            "reward_gem_7      = false\n"
            "reward_gem_8      = false\n"
            "reward_gem_9      = false\n"
            "reward_gem_10     = false\n"
            "reward_gem_11     = false\n"
            "reward_gem_12     = false\n"
            "reward_gem_13     = false\n"
            "reward_gem_14     = false\n"
            "reward_gem_15     = false\n"
            "\n"
            "// Win Condition (collapses and board clear)\n"
            "winCondition = 100\n"
            "\n"
            "// Vise Groups\n"
            "viseGroupCount = 30\n"
            "\n"
            "// Vise Speed\n"
            "viseMaxSpeed = 800.0\n"
            "viseMidMaxSpeed = 75.0\n"
            "viseMidMinSpeed = 75.0\n"
            "viseMinSpeed = 3.0\n"
            "viseSpeedMaxBzLerp = 0.9 0.9\n"
            "viseSpeedMidBzLerp = 0.25 0.75\n"
            "viseSpeedMinBzLerp = 0.25 0.75\n"
            "\n"
            "// Path Distances\n"
            "viseSpawnDistance_1 = 0.6\n"
            "midStartDistance_1 = 0.2\n"
            "midEndDistance_1 = 0.6\n"
            "\n"
            "viseSpawnDistance_2 = 0.6\n"
            "midStartDistance_2 = 0.3\n"
            "midEndDistance_2 = 0.6\n";

        auto real_lvl = luxor::rc::parse_level(EMBEDDED_LEVEL_1_1);
        REQUIRE(real_lvl.has_value());
        CHECK(real_lvl->has("mapFile"));
        CHECK(real_lvl->get_string("mapFile") == "URAEUS NEFERTARI");
        CHECK(real_lvl->has("winCondition"));
        CHECK(real_lvl->get_int("winCondition") == 100);
        CHECK(real_lvl->get_bool("spawnColor_1") == true);
        CHECK(real_lvl->get_bool("spawnColor_5") == false);
        CHECK(real_lvl->get_int("spawnStreak") == 225);
        CHECK(real_lvl->get_number("viseMaxSpeed") == 800.0);
    }
}

TEST_CASE("ini file (.ini) parsing") {
    const std::string_view sample_ini =
        "[Graphics]\n"
        "; Fullscreen or windowed mode\n"
        "Fullscreen=1\n"
        "ColorBits=32\n"
        "Detail=1\n"
        "\n"
        "[Sound]\n"
        "EffectsVolume=1.0\n"
        "MusicVolume=0.40\n"
        "\n"
        "[Game]\n"
        "ShowLink=0\n"
        "ILink=assets/button_moregames.spr\n"
        "VisitURL=http://www.mumbojumbo.com\n"
        "GraphicsAPI_1=gfx2d_dx8.dll\n"
        "Language=English\n";

    auto ini_res = luxor::rc::parse_ini(sample_ini);
    REQUIRE(ini_res.has_value());
    const auto& doc = *ini_res;

    CHECK(doc.has_section("Graphics"));
    CHECK(doc.has_section("Sound"));
    CHECK(doc.has_section("Game"));
    CHECK_FALSE(doc.has_section("NonExistent"));

    CHECK(doc.get_section_int("Graphics", "Fullscreen") == 1);
    CHECK(doc.get_section_int("Graphics", "ColorBits") == 32);
    CHECK(doc.get_section_int("Graphics", "Detail") == 1);

    CHECK(doc.get_section_number("Sound", "EffectsVolume") == 1.0);
    CHECK(doc.get_section_number("Sound", "MusicVolume") == 0.40);

    CHECK(doc.get_section_int("Game", "ShowLink") == 0);
    CHECK(doc.get_section_string("Game", "ILink") == "assets/button_moregames.spr");
    CHECK(doc.get_section_string("Game", "VisitURL") == "http://www.mumbojumbo.com");
    CHECK(doc.get_section_string("Game", "GraphicsAPI_1") == "gfx2d_dx8.dll");
    CHECK(doc.get_section_string("Game", "Language") == "English");

    // Section direct pointer access
    const auto* g_sec = doc.find_section("Graphics");
    REQUIRE(g_sec != nullptr);
    CHECK(g_sec->has("Fullscreen"));
    CHECK(g_sec->get_int("Fullscreen") == 1);

    SUBCASE("parse embedded mahjong.ini") {
        constexpr std::string_view EMBEDDED_MAHJONG_INI =
            "[Graphics]\n"
            "; Fullscreen or windowed mode\n"
            "Fullscreen=1\n"
            "\n"
            "; Color Depth\n"
            "ColorBits=32\n"
            "\n"
            "; Graphics Detail\n"
            "Detail=1\n"
            "\n"
            "[Sound]\n"
            "; Specify the Sound Effects Volume\n"
            "EffectsVolume=1.0\n"
            "\n"
            "; Specify the Music Volume\n"
            "MusicVolume=0.40\n"
            "\n"
            "[Game]\n"
            "; 3rd Party Customization\n"
            "ShowLink=0\n"
            "ILink=assets/button_moregames.spr\n"
            "VisitURL=http://www.mumbojumbo.com\n"
            "\n"
            "; Specify the graphics API Order for loading\n"
            "GraphicsAPI_1=gfx2d_dx8.dll\n"
            "GraphicsAPI_2=gfx2d_ogl.dll\n"
            "GraphicsAPI_1=gfx2d_dd7.dll\n"
            "\n"
            "; Specify the sound API to load\n"
            "SoundAPI=snd3d_fmod.dll\n"
            "\n"
            "; Profile Specific Settings\n"
            "CurrentProfile=-1\n"
            "\n"
            "; Language Setting\n"
            "Language=English\n";

        auto real_doc = luxor::rc::parse_ini(EMBEDDED_MAHJONG_INI);
        REQUIRE(real_doc.has_value());
        CHECK(real_doc->has_section("Graphics"));
        CHECK(real_doc->has_section("Sound"));
        CHECK(real_doc->has_section("Game"));
        CHECK(real_doc->get_section_int("Graphics", "Fullscreen") == 1);
        CHECK(real_doc->get_section_string("Game", "VisitURL") == "http://www.mumbojumbo.com");
    }
}

TEST_CASE("particle system file (.psys) parsing") {
    const std::string_view sample_psys =
        "Emitter test\n"
        "{\n"
        "    Type = Sprite\n"
        "    Flags = EF_SPRITE_ANIM_LOOP | EF_USE_COLOR_RATE | EF_PALETTE_LOOP\n"
        "    StartParticles = 1\n"
        "    ParticleRate = 5.000000\n"
        "    Sprite = data\\sprites\\game\\ball_1.spr\n"
        "    PosY = -240.000000\n"
        "    StartVelMin = 0.000000 75.000000\n"
        "}\n"
        "Emitter sparks\n"
        "{\n"
        "    Type = Sprite\n"
        "    Flags = EF_NONE\n"
        "    StartParticles = 50\n"
        "}\n";

    auto psys_res = luxor::rc::parse_psys(sample_psys);
    REQUIRE(psys_res.has_value());
    const auto& doc = *psys_res;

    CHECK(doc.has_section("test"));
    CHECK(doc.has_section("sparks"));
    CHECK(doc.has_section("Emitter", "test"));
    CHECK(doc.has_section("Emitter", "sparks"));

    auto emitters = doc.find_sections_by_type("Emitter");
    CHECK(emitters.size() == 2);

    const auto* test_sec = doc.find_section("Emitter", "test");
    REQUIRE(test_sec != nullptr);
    CHECK(test_sec->type == "Emitter");
    CHECK(test_sec->name == "test");
    CHECK(test_sec->get_string("Type") == "Sprite");
    CHECK(test_sec->get_int("StartParticles") == 1);
    CHECK(test_sec->get_number("ParticleRate") == 5.0);
    CHECK(test_sec->get_string("Sprite") == "data\\sprites\\game\\ball_1.spr");
    CHECK(test_sec->get_number("PosY") == -240.0);

    // Multiple numbers
    auto vel = test_sec->get_numbers("StartVelMin");
    REQUIRE(vel.size() == 2);
    CHECK(vel[0] == 0.0);
    CHECK(vel[1] == 75.0);

    // Flags separated by pipe
    const auto* flags_vals = test_sec->get_values("Flags");
    REQUIRE(flags_vals != nullptr);
    REQUIRE(flags_vals->size() == 3);
    CHECK(std::get<std::string>((*flags_vals)[0]) == "EF_SPRITE_ANIM_LOOP");
    CHECK(std::get<std::string>((*flags_vals)[1]) == "EF_USE_COLOR_RATE");
    CHECK(std::get<std::string>((*flags_vals)[2]) == "EF_PALETTE_LOOP");

    const auto* sparks_sec = doc.find_section("Emitter", "sparks");
    REQUIRE(sparks_sec != nullptr);
    CHECK(sparks_sec->get_int("StartParticles") == 50);
    CHECK(sparks_sec->get_string("Flags") == "EF_NONE");

    SUBCASE("parse embedded default.psys") {
        constexpr std::string_view EMBEDDED_DEFAULT_PSYS =
            "Emitter test\n"
            "{\n"
            "	Type = Sprite\n"
            "	Flags = EF_SPRITE_ANIM_LOOP | EF_USE_COLOR_RATE | EF_PALETTE_LOOP | EF_LIFESPAN_INFINITE\n"
            "	StartParticles = 1\n"
            "	MaxParticles = 1\n"
            "	ParticleRate = 5.000000\n"
            "	Sprite = data\\sprites\\game\\ball_1.spr\n"
            "	ColorRate = 30.000000\n"
            "	AnimMin = 0\n"
            "	AnimMax = 0\n"
            "	AnimRate = 30\n"
            "	FadeInEndTime = 0.000000\n"
            "	FadeOutStartTime = 1.000000\n"
            "	LifespanMin = 1.000000\n"
            "	LifespanMax = 1.000000\n"
            "	RenderDelay = 0.000000\n"
            "	FirstFrameDelay = 0.000000\n"
            "	PosX = 0.000000\n"
            "	PosY = -240.000000\n"
            "	SpawnRadiusMin = 0.000000 0.000000\n"
            "	SpawnRadiusMax = 0.000000 0.000000\n"
            "	StartVelMin = 0.000000 75.000000\n"
            "	StartVelMax = 0.000000 75.000000\n"
            "	Acc = 0.000000 0.000000\n"
            "	DevDelay = 0.000000\n"
            "	DevAngle = 0.000000 0.000000\n"
            "	EmitterVelMin = 0.000000 0.000000\n"
            "	EmitterVelMax = 0.000000 0.000000\n"
            "	EmitterAcc = 0.000000 0.000000\n"
            "	EmitterStart = 0.000000 0.000000\n"
            "	EmitterLifespan = 2.000000 2.000000\n"
            "}\n";

        auto real_doc = luxor::rc::parse_psys(EMBEDDED_DEFAULT_PSYS);
        REQUIRE(real_doc.has_value());
        CHECK(real_doc->has_section("Emitter", "test"));
        auto* real_sec = real_doc->find_section("Emitter", "test");
        REQUIRE(real_sec != nullptr);
        CHECK(real_sec->get_string("Type") == "Sprite");
        CHECK(real_sec->get_number("PosY") == -240.0);
    }

    SUBCASE("parse embedded stage_complete.psys") {
        constexpr std::string_view EMBEDDED_STAGE_COMPLETE_PSYS =
            "Emitter flash\n"
            "{\n"
            "	Type = Sprite\n"
            "	Flags = EF_NONE\n"
            "	StartParticles = 1\n"
            "	MaxParticles = 2\n"
            "	ParticleRate = 25.000000\n"
            "	Sprite = data\\sprites\\particles\\flash.spr\n"
            "	ColorRate = 0.000000\n"
            "	AnimMin = 0\n"
            "	AnimMax = 0\n"
            "	AnimRate = 30\n"
            "	FadeInEndTime = 0.000000\n"
            "	FadeOutStartTime = 1.000000\n"
            "	LifespanMin = 0.250000\n"
            "	LifespanMax = 0.250000\n"
            "	RenderDelay = 0.000000\n"
            "	FirstFrameDelay = 0.000000\n"
            "	PosX = 0.000000\n"
            "	PosY = 0.000000\n"
            "	SpawnRadiusMin = -10.000000 -10.000000\n"
            "	SpawnRadiusMax = 10.000000 10.000000\n"
            "	StartVelMin = 0.000000 0.000000\n"
            "	StartVelMax = 0.000000 0.000000\n"
            "	Acc = 0.000000 0.000000\n"
            "	DevDelay = 0.000000\n"
            "	DevAngle = 0.000000 0.000000\n"
            "	EmitterVelMin = 0.000000 0.000000\n"
            "	EmitterVelMax = 0.000000 0.000000\n"
            "	EmitterAcc = 0.000000 0.000000\n"
            "	EmitterStart = 0.000000 0.000000\n"
            "	EmitterLifespan = 1.000000 1.000000\n"
            "}\n"
            "Emitter sparks\n"
            "{\n"
            "	Type = Sprite\n"
            "	Flags = EF_SPRITE_ANIM_LOOP | EF_SPRITE_RANDOM_FRAME | EF_VEL_POSRELATIVE\n"
            "	StartParticles = 50\n"
            "	MaxParticles = 50\n"
            "	ParticleRate = 0.000000\n"
            "	Sprite = data\\sprites\\particles\\spark_ball_6.spr\n"
            "	ColorRate = 0.000000\n"
            "	AnimMin = 0\n"
            "	AnimMax = 0\n"
            "	AnimRate = 30\n"
            "	FadeInEndTime = 0.000000\n"
            "	FadeOutStartTime = 0.500000\n"
            "	LifespanMin = 0.250000\n"
            "	LifespanMax = 0.750000\n"
            "	RenderDelay = 0.000000\n"
            "	FirstFrameDelay = 0.000000\n"
            "	PosX = 0.000000\n"
            "	PosY = 0.000000\n"
            "	SpawnRadiusMin = 5.000000 5.000000\n"
            "	SpawnRadiusMax = 5.000000 5.000000\n"
            "	StartVelMin = 100.000000 100.000000\n"
            "	StartVelMax = 100.000000 100.000000\n"
            "	Acc = 0.000000 0.000000\n"
            "	DevDelay = 0.000000\n"
            "	DevAngle = 0.000000 0.000000\n"
            "	EmitterVelMin = 0.000000 0.000000\n"
            "	EmitterVelMax = 0.000000 0.000000\n"
            "	EmitterAcc = 0.000000 0.000000\n"
            "	EmitterStart = 0.000000 0.000000\n"
            "	EmitterLifespan = 0.500000 0.500000\n"
            "}\n";

        auto real_doc = luxor::rc::parse_psys(EMBEDDED_STAGE_COMPLETE_PSYS);
        REQUIRE(real_doc.has_value());
        auto all_emitters = real_doc->find_sections_by_type("Emitter");
        CHECK(all_emitters.size() >= 2);
    }
}

TEST_CASE("config_document to_json serialization") {
    SUBCASE("property to_json") {
        luxor::rc::property p{"speed", {1.5, 2.5}};
        auto j = p.to_json();
        CHECK(j["key"] == "speed");
        REQUIRE(j["values"].is_array());
        CHECK(j["values"].size() == 2);
        CHECK(j["values"][0] == 1.5);
        CHECK(j["values"][1] == 2.5);

        // ADL serialization
        nlohmann::json j_adl = p;
        CHECK(j_adl == j);
    }

    SUBCASE("section to_json") {
        luxor::rc::section s;
        s.type = "Emitter";
        s.name = "sparks";
        s.properties.push_back({"Count", {50.0}});
        s.properties.push_back({"Enabled", {true}});
        s.properties.push_back({"Name", {std::string("Fire")}});

        auto j = s.to_json();
        CHECK(j["type"] == "Emitter");
        CHECK(j["name"] == "sparks");
        REQUIRE(j["properties"].is_array());
        CHECK(j["properties"].size() == 3);
        CHECK(j["properties"][0]["key"] == "Count");
        CHECK(j["properties"][0]["values"][0] == 50.0);
        CHECK(j["properties"][1]["key"] == "Enabled");
        CHECK(j["properties"][1]["values"][0] == true);
        CHECK(j["properties"][2]["key"] == "Name");
        CHECK(j["properties"][2]["values"][0] == "Fire");

        nlohmann::json j_adl = s;
        CHECK(j_adl == j);
    }

    SUBCASE("full document to_json") {
        const std::string_view sample =
            "title = \"Game\"\n"
            "fullscreen = true\n"
            "\n"
            "[Display]\n"
            "width = 800\n"
            "height = 600\n";

        auto doc = luxor::rc::parse_config(sample);
        REQUIRE(doc.has_value());

        auto j = doc->to_json();
        REQUIRE(j.contains("properties"));
        REQUIRE(j.contains("sections"));
        CHECK(j["properties"].size() == 2);
        CHECK(j["properties"][0]["key"] == "title");
        CHECK(j["properties"][0]["values"][0] == "Game");
        CHECK(j["properties"][1]["key"] == "fullscreen");
        CHECK(j["properties"][1]["values"][0] == true);

        CHECK(j["sections"].size() == 1);
        CHECK(j["sections"][0]["name"] == "Display");
        CHECK(j["sections"][0]["properties"].size() == 2);
        CHECK(j["sections"][0]["properties"][0]["key"] == "width");
        CHECK(j["sections"][0]["properties"][0]["values"][0] == 800.0);

        // ADL serialization test
        nlohmann::json j_adl = *doc;
        CHECK(j_adl == j);
    }
}

