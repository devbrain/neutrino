#include <doctest/doctest.h>
#include <luxor/resources/parser/sl3_parser.hh>
#include <luxor/resources/loader.hh>
#include <luxor/resources/assets.hh>

#include <filesystem>
#include <fstream>

TEST_CASE("SL3 parser synthetic syntax") {
    const std::string sl3_text = R"(
// Comments and empty lines
simple_snd = sample data/sound/simple.ogg

hit_snd = sample data/sound/hit.ogg
{
    volume = 0.5
}

bgm = stream data/music/bgm.ogg
{
    loop = false
}

ambient = playlist
{
    flags = REPEAT
    track = bgm
    track = simple_snd
}
)";

    auto result = luxor::rc::parse_sl3(sl3_text);
    REQUIRE(result.has_value());
    const auto& sl3 = *result;

    SUBCASE("Samples") {
        CHECK(sl3.has_sample("simple_snd"));
        CHECK(sl3.has_sample("SIMPLE_SND"));
        const auto* s1 = sl3.find_sample("simple_snd");
        REQUIRE(s1 != nullptr);
        CHECK(s1->name == "simple_snd");
        CHECK(s1->file == "data/sound/simple.ogg");
        CHECK(s1->volume == doctest::Approx(1.0f));

        CHECK(sl3.has_sample("hit_snd"));
        const auto* s2 = sl3.find_sample("HIT_SND");
        REQUIRE(s2 != nullptr);
        CHECK(s2->name == "hit_snd");
        CHECK(s2->file == "data/sound/hit.ogg");
        CHECK(s2->volume == doctest::Approx(0.5f));
    }

    SUBCASE("Streams") {
        CHECK(sl3.has_stream("bgm"));
        CHECK(sl3.has_stream("BGM"));
        const auto* st = sl3.find_stream("bgm");
        REQUIRE(st != nullptr);
        CHECK(st->name == "bgm");
        CHECK(st->file == "data/music/bgm.ogg");
        CHECK(st->loop == false);
    }

    SUBCASE("Playlists") {
        CHECK(sl3.has_playlist("ambient"));
        CHECK(sl3.has_playlist("AMBIENT"));
        const auto* pl = sl3.find_playlist("ambient");
        REQUIRE(pl != nullptr);
        CHECK(pl->name == "ambient");
        CHECK(pl->flags == "REPEAT");
        REQUIRE(pl->tracks.size() == 2);
        CHECK(pl->tracks[0] == "bgm");
        CHECK(pl->tracks[1] == "simple_snd");
    }

    SUBCASE("JSON conversion") {
        auto j = sl3.to_json();
        CHECK(j.contains("samples"));
        CHECK(j.contains("streams"));
        CHECK(j.contains("playlists"));
        CHECK(j["samples"].size() == 2);
        CHECK(j["streams"].size() == 1);
        CHECK(j["playlists"].size() == 1);
    }
}

TEST_CASE("SL3 parser with real dump files") {
    const std::filesystem::path dump_path = "/home/igor/games/luxor_dump";
    const auto music_sl3_path = dump_path / "data" / "music" / "music.sl3";
    const auto sounds_sl3_path = dump_path / "data" / "sound" / "sounds.sl3";

    if (std::filesystem::exists(music_sl3_path)) {
        SUBCASE("music.sl3") {
            auto result = luxor::rc::parse_sl3_file(music_sl3_path);
            REQUIRE(result.has_value());
            const auto& sl3 = *result;

            CHECK(sl3.stream_order.size() == 3);
            CHECK(sl3.has_stream("level_music"));
            CHECK(sl3.has_stream("menu_music"));
            CHECK(sl3.has_stream("danger_music"));

            const auto* lvl = sl3.find_stream("level_music");
            REQUIRE(lvl != nullptr);
            CHECK(lvl->file == "data/music/Giza-Music-Level.ogg");
            CHECK(lvl->loop == true);

            CHECK(sl3.playlist_order.size() == 3);
            CHECK(sl3.has_playlist("music_menu"));
            CHECK(sl3.has_playlist("music_level"));
            CHECK(sl3.has_playlist("music_danger"));

            const auto* pl = sl3.find_playlist("music_menu");
            REQUIRE(pl != nullptr);
            CHECK(pl->flags == "REPEAT");
            REQUIRE(pl->tracks.size() == 1);
            CHECK(pl->tracks[0] == "menu_music");
        }
    }

    if (std::filesystem::exists(sounds_sl3_path)) {
        SUBCASE("sounds.sl3") {
            auto result = luxor::rc::parse_sl3_file(sounds_sl3_path);
            REQUIRE(result.has_value());
            const auto& sl3 = *result;

            CHECK(sl3.samples.size() == 49);
            CHECK(sl3.has_sample("collapse_1"));
            CHECK(sl3.has_sample("collapse_5"));
            CHECK(sl3.has_sample("highlight"));
            CHECK(sl3.has_sample("bullet_swap"));

            const auto* c1 = sl3.find_sample("collapse_1");
            REQUIRE(c1 != nullptr);
            CHECK(c1->file == "data/sound/collapse_1.ogg");
            CHECK(c1->volume == doctest::Approx(1.0f));

            const auto* hl = sl3.find_sample("highlight");
            REQUIRE(hl != nullptr);
            CHECK(hl->file == "data/sound/button_highlight.ogg");
            CHECK(hl->volume == doctest::Approx(0.1f));
        }
    }
}

TEST_CASE("SL3 assets and loader integration") {
    auto& assets = luxor::assets::instance();
    assets.reset();

    const auto temp_dir = std::filesystem::temp_directory_path() / "luxor_test_sl3_mock";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir / "data" / "sound");
    std::filesystem::create_directories(temp_dir / "data" / "music");

    {
        std::ofstream ofs(temp_dir / "data" / "sound" / "sounds.sl3");
        ofs << "click = sample data/sound/click.ogg\n"
            << "beep = sample data/sound/beep.ogg\n"
            << "{\n"
            << "    volume = 0.4\n"
            << "}\n";
    }

    {
        std::ofstream ofs(temp_dir / "data" / "music" / "music.sl3");
        ofs << "intro = stream data/music/intro.ogg\n"
            << "{\n"
            << "    loop = true\n"
            << "}\n"
            << "pl_intro = playlist\n"
            << "{\n"
            << "    flags = REPEAT\n"
            << "    track = intro\n"
            << "}\n";
    }

    CHECK(assets.init(temp_dir));

    SUBCASE("get_sl3 and find_sl3 caching") {
        CHECK_FALSE(assets.has_sl3("data/sound/sounds.sl3"));
        CHECK(assets.sl3_cache_size() == 0);

        auto sl3 = assets.get_sl3("data/sound/sounds.sl3");
        REQUIRE(sl3.has_value());
        CHECK(assets.has_sl3("data/sound/sounds.sl3"));
        CHECK(assets.sl3_cache_size() == 1);
        CHECK(sl3->has_sample("click"));
        CHECK(sl3->has_sample("beep"));

        const auto* found = assets.find_sl3("DATA\\SOUND\\SOUNDS.SL3");
        REQUIRE(found != nullptr);
        CHECK(assets.sl3_cache_size() == 1);
        CHECK(found->has_sample("beep"));
    }

    SUBCASE("play_cue and play_playlist lookup") {
        // Without audio device initialized in headless tests, play_sound/play_music gracefully no-ops,
        // but play_cue and play_playlist will return true if the cue is recognized.
        CHECK(assets.play_cue("click", "data/sound/sounds.sl3"));
        CHECK(assets.play_cue("beep", "data/sound/sounds.sl3"));
        CHECK_FALSE(assets.play_cue("non_existent_cue", "data/sound/sounds.sl3"));

        CHECK(assets.play_playlist("pl_intro", "data/music/music.sl3"));
        CHECK_FALSE(assets.play_playlist("non_existent_pl", "data/music/music.sl3"));
    }

    assets.reset();
    std::filesystem::remove_all(temp_dir);
}
