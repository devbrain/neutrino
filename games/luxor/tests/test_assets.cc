#include <doctest/doctest.h>
#include <luxor/resources/assets.hh>
#include <luxor/resources/storage/vfs.hh>
#include <sdlpp/video/surface.hh>

#include <filesystem>
#include <fstream>

TEST_CASE("assets singleton and cache functionality") {
    auto& assets = luxor::assets::instance();
    CHECK(&assets == &luxor::rc::assets::get());
    CHECK(&assets == &luxor::rc::assets::instance());

    assets.reset();
    CHECK(assets.get_vfs().empty());
    CHECK(assets.sprite_cache_size() == 0);
    CHECK(assets.image_cache_size() == 0);
    CHECK(assets.font_cache_size() == 0);
    CHECK(assets.depth_cache_size() == 0);
    CHECK(assets.string_list_cache_size() == 0);
    CHECK(assets.config_cache_size() == 0);
    CHECK(assets.obj_cache_size() == 0);
    CHECK(assets.sl3_cache_size() == 0);

    // Setup temporary mock directory
    const auto temp_dir = std::filesystem::temp_directory_path() / "luxor_test_assets_mock";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir / "data" / "sprites");
    std::filesystem::create_directories(temp_dir / "data" / "bitmaps");
    std::filesystem::create_directories(temp_dir / "data" / "fonts");
    std::filesystem::create_directories(temp_dir / "data" / "levels");
    std::filesystem::create_directories(temp_dir / "data" / "uiscript");
    std::filesystem::create_directories(temp_dir / "data" / "maps");

    // 1. Create a dummy 32x32 TGA image
    {
        uint8_t tga_header[18] = {
            0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            32, 0, // Width 32
            32, 0, // Height 32
            32,    // 32 bpp
            8      // 8-bit alpha
        };
        std::vector<uint8_t> tga_data(18 + 32 * 32 * 4, 0xFF);
        std::memcpy(tga_data.data(), tga_header, 18);
        std::ofstream ofs(temp_dir / "data" / "bitmaps" / "test_tex.tga", std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(tga_data.data()), static_cast<std::streamsize>(tga_data.size()));
    }

    // 2. Create a dummy sprite file
    {
        std::ofstream ofs(temp_dir / "data" / "sprites" / "ball.spr");
        ofs << "data/bitmaps/test_tex.tga\n"
            << "none\n"
            << "16 16\n"
            << "1\n"
            << "4\n"
            << "0 0\n";
    }

    // 3. Create a dummy font file
    {
        std::ofstream ofs(temp_dir / "data" / "fonts" / "test.font");
        ofs << "data/bitmaps/test_tex.tga\n"
            << "none\n"
            << "16\n"
            << "1 2\n"
            << "A\n"
            << "0 0 16\n"
            << "B\n"
            << "16 0 16\n";
    }

    // 4. Create dummy depths.txt
    {
        std::ofstream ofs(temp_dir / "data" / "uiscript" / "depths.txt");
        ofs << "background\n"
            << "spheres\n"
            << "shooter\n";
    }

    // 5. Create dummy stage_names.txt
    {
        std::ofstream ofs(temp_dir / "data" / "uiscript" / "stage_names.txt");
        ofs << "Stage 1\n"
            << "Stage 2\n";
    }

    // 6. Create dummy level.lvl
    {
        std::ofstream ofs(temp_dir / "data" / "levels" / "level_1_1.lvl");
        ofs << "mapFile = \"data/maps/test/map.ui\"\n"
            << "Speed = 100\n";
    }

    // 7. Create dummy path.obj
    {
        std::ofstream ofs(temp_dir / "data" / "maps" / "path.obj");
        ofs << "o test_path\n"
            << "v 0 0 0\n"
            << "v 10 20 0\n";
    }

    // Mount and initialize
    CHECK(assets.init(temp_dir));
    CHECK(assets.exists("data/sprites/ball.spr"));
    CHECK(assets.exists("data/bitmaps/test_tex.tga"));

    SUBCASE("Sprite loading and caching") {
        CHECK_FALSE(assets.has_sprite("data/sprites/ball.spr"));
        CHECK(assets.sprite_cache_size() == 0);

        auto spr = assets.get_sprite("data/sprites/ball.spr");
        REQUIRE(spr.has_value());
        CHECK(spr->grid.has_value());
        CHECK(spr->grid->cell_w == 16);
        CHECK(spr->grid->cell_h == 16);
        CHECK(assets.has_sprite("data/sprites/ball.spr"));
        CHECK(assets.sprite_cache_size() == 1);

        // Case-insensitivity & slash normalization check
        const auto* found = assets.find_sprite("DATA\\SPRITES\\BALL.SPR");
        REQUIRE(found != nullptr);
        CHECK(found->grid.has_value());
        CHECK(assets.sprite_cache_size() == 1); // No duplicate insertion
    }

    SUBCASE("Image loading and caching") {
        CHECK_FALSE(assets.has_image("data/bitmaps/test_tex.tga"));
        CHECK(assets.image_cache_size() == 0);

        auto img = assets.get_image("data/bitmaps/test_tex.tga");
        REQUIRE(img != nullptr);
        CHECK(img->is_valid());
        CHECK(img->width() == 32);
        CHECK(img->height() == 32);
        CHECK(assets.has_image("data/bitmaps/test_tex.tga"));
        CHECK(assets.image_cache_size() == 1);

        const auto* found = assets.find_image("data\\bitmaps\\test_tex.tga");
        REQUIRE(found != nullptr);
        CHECK(found->width() == 32);
        CHECK(assets.image_cache_size() == 1);
    }

    SUBCASE("Font loading and caching") {
        CHECK_FALSE(assets.has_font("data/fonts/test.font"));
        auto font = assets.get_font("data/fonts/test.font");
        REQUIRE(font.has_value());
        CHECK(font->char_height == 16);
        CHECK(font->glyph_count == 2);
        CHECK(font->has_glyph('A'));
        CHECK(font->has_glyph('B'));
        CHECK(assets.font_cache_size() == 1);

        const auto* found = assets.find_font("DATA/FONTS/TEST.FONT");
        REQUIRE(found != nullptr);
        CHECK(assets.font_cache_size() == 1);
    }

    SUBCASE("Depths table loading and caching") {
        auto depths = assets.get_depths("data/uiscript/depths.txt");
        REQUIRE(depths.has_value());
        CHECK(depths->size() == 3);
        CHECK(depths->get_index("background") == 0);
        CHECK(depths->get_index("spheres") == 1);
        CHECK(depths->get_index("shooter") == 2);
        CHECK(assets.depth_cache_size() == 1);

        const auto* found = assets.find_depths("data/uiscript/depths.txt");
        REQUIRE(found != nullptr);
        CHECK(assets.depth_cache_size() == 1);
    }

    SUBCASE("String list loading and caching") {
        auto list = assets.get_string_list("data/uiscript/stage_names.txt");
        REQUIRE(list.has_value());
        CHECK(list->size() == 2);
        CHECK((*list)[0] == "Stage 1");
        CHECK((*list)[1] == "Stage 2");
        CHECK(assets.string_list_cache_size() == 1);

        const auto* found = assets.find_string_list("DATA\\UISCRIPT\\STAGE_NAMES.TXT");
        REQUIRE(found != nullptr);
        CHECK(assets.string_list_cache_size() == 1);
    }

    SUBCASE("Config and Level loading and caching") {
        auto lvl = assets.get_level("data/levels/level_1_1.lvl");
        REQUIRE(lvl.has_value());
        CHECK(lvl->get_string("mapFile") == "data/maps/test/map.ui");
        CHECK(lvl->get_int("Speed") == 100);
        CHECK(assets.config_cache_size() == 1);

        const auto* found = assets.find_level("data/levels/level_1_1.lvl");
        REQUIRE(found != nullptr);
        CHECK(assets.config_cache_size() == 1);
    }

    SUBCASE("OBJ mesh loading and caching") {
        auto obj = assets.get_obj("data/maps/path.obj");
        REQUIRE(obj.has_value());
        CHECK(obj->GetAttrib().vertices.size() / 3 == 2);
        CHECK(assets.obj_cache_size() == 1);

        const auto* found = assets.find_obj("data/maps/path.obj");
        REQUIRE(found != nullptr);
        CHECK(assets.obj_cache_size() == 1);
    }

    SUBCASE("Cache clear vs reset") {
        REQUIRE(assets.get_sprite("data/sprites/ball.spr").has_value());
        REQUIRE(assets.get_font("data/fonts/test.font").has_value());
        CHECK(assets.sprite_cache_size() == 1);
        CHECK(assets.font_cache_size() == 1);

        assets.clear_cache();
        CHECK(assets.sprite_cache_size() == 0);
        CHECK(assets.font_cache_size() == 0);
        CHECK_FALSE(assets.get_vfs().empty());

        assets.reset();
        CHECK(assets.get_vfs().empty());
    }

    SUBCASE("Integration with real luxor dump data if available") {
        const std::filesystem::path dump_path = "/home/igor/games/luxor_dump";
        if (std::filesystem::exists(dump_path / "data" / "sprites" / "game" / "ball_1.spr")) {
            assets.reset();
            CHECK(assets.init(dump_path));

            // Test sprite loading
            auto spr = assets.get_sprite("data/sprites/game/ball_1.spr");
            REQUIRE(spr.has_value());
            CHECK(spr->grid.has_value());
            CHECK(spr->grid->cell_w == 32);
            CHECK(spr->grid->cell_h == 32);

            // Test font loading
            auto font = assets.get_font("data/fonts/score0.font");
            REQUIRE(font.has_value());
            CHECK(font->glyph_count > 0);

            // Test level loading
            auto lvl = assets.get_level("data/levels/level_1_1.lvl");
            REQUIRE(lvl.has_value());
            CHECK(lvl->has("mapFile"));

            // Test depths table
            auto depths = assets.get_depths("data/uiscript/depths.txt");
            REQUIRE(depths.has_value());
            CHECK(depths->size() > 0);

            // Test stage names
            auto stages = assets.get_string_list("data/uiscript/stage_names.txt");
            REQUIRE(stages.has_value());
            CHECK(stages->size() > 0);

            // Test OBJ path loading
            auto obj = assets.get_obj("data/maps/navigatingthecataracts/path1.obj");
            REQUIRE(obj.has_value());
            CHECK(obj->GetAttrib().vertices.size() > 0);

            // Test SL3 script loading
            auto sl3_snd = assets.get_sl3("data/sound/sounds.sl3");
            REQUIRE(sl3_snd.has_value());
            CHECK(sl3_snd->samples.size() == 49);
            CHECK(assets.sl3_cache_size() >= 1);

            auto sl3_mus = assets.get_sl3("data/music/music.sl3");
            REQUIRE(sl3_mus.has_value());
            CHECK(sl3_mus->streams.size() == 3);
            CHECK(sl3_mus->playlists.size() == 3);
        }
    }

    assets.reset();
    std::filesystem::remove_all(temp_dir);
}
