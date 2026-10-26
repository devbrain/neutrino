#include <doctest/doctest.h>
#include <luxor/resources/parser/spr_parser.hh>

#include <filesystem>
#include <string_view>

TEST_CASE("sprite file (.spr) parsing") {
    SUBCASE("Pattern A: Standard implicit horizontal layout") {
        const std::string_view sample_spr_horizontal =
            "data/bitmaps/game/ball_1.jpg\n"
            "data/bitmaps/game/ball_alpha.tga\n"
            "32 32\n"
            "1\n"
            "4\n"
            "0 0\n";

        auto spr1 = luxor::rc::parse_spr(sample_spr_horizontal);
        REQUIRE(spr1.has_value());
        CHECK(spr1->format == luxor::rc::sprite_format::classic);
        CHECK(spr1->diffuse_texture == "data/bitmaps/game/ball_1.jpg");
        CHECK(spr1->alpha_texture == "data/bitmaps/game/ball_alpha.tga");
        CHECK(spr1->has_alpha == true);
        CHECK(spr1->grid.cell_w == 32);
        CHECK(spr1->grid.cell_h == 32);
        REQUIRE(spr1->anims.size() == 1);
        CHECK(spr1->anims[0].frames.size() == 4);
        CHECK(spr1->total_frames() == 4);

        for (size_t i = 0; i < 4; ++i) {
            const auto* f = spr1->get_frame(0, i);
            REQUIRE(f != nullptr);
            CHECK(f->x == static_cast<int>(i * 32));
            CHECK(f->y == 0);
            CHECK(f->width == 32);
            CHECK(f->height == 32);
        }
    }

    SUBCASE("Pattern B: Button with 4 animation states without alpha") {
        const std::string_view sample_spr_button =
            "assets/button_moregames.jpg\n"
            "none\n"
            "233 51\n"
            "4\n"
            "1\n"
            "0 0\n"
            "1\n"
            "0 51\n"
            "1\n"
            "0 102\n"
            "1\n"
            "0 153\n";

        auto spr2 = luxor::rc::parse_spr(sample_spr_button);
        REQUIRE(spr2.has_value());
        CHECK(spr2->has_alpha == false);
        CHECK(spr2->grid.cell_w == 233);
        CHECK(spr2->grid.cell_h == 51);
        CHECK(spr2->anims.size() == 4);
        CHECK(spr2->total_frames() == 4);
        CHECK(spr2->get_frame(0, 0)->y == 0);
        CHECK(spr2->get_frame(1, 0)->y == 51);
        CHECK(spr2->get_frame(2, 0)->y == 102);
        CHECK(spr2->get_frame(3, 0)->y == 153);
    }

    SUBCASE("Pattern X: Explicit Mode X") {
        const std::string_view sample_spr_mode_x =
            "data/custom.png\n"
            "none\n"
            "16 16\n"
            "1\n"
            "3 X\n"
            "10 20\n"
            "50 60\n"
            "90 100\n";

        auto spr3 = luxor::rc::parse_spr(sample_spr_mode_x);
        REQUIRE(spr3.has_value());
        REQUIRE(spr3->anims.size() == 1);
        REQUIRE(spr3->anims[0].frames.size() == 3);
        CHECK(spr3->get_frame(0, 0)->x == 10);
        CHECK(spr3->get_frame(0, 0)->y == 20);
        CHECK(spr3->get_frame(0, 1)->x == 50);
        CHECK(spr3->get_frame(0, 1)->y == 60);
        CHECK(spr3->get_frame(0, 2)->x == 90);
        CHECK(spr3->get_frame(0, 2)->y == 100);
    }

    SUBCASE("Negative tests") {
        auto bad1 = luxor::rc::parse_spr("only_one_line\n");
        CHECK_FALSE(bad1.has_value());
        CHECK_FALSE(bad1.error().empty());

        auto bad2 = luxor::rc::parse_spr("tex.jpg\nnone\ninvalid_dim\n");
        CHECK_FALSE(bad2.has_value());
    }

    SUBCASE("parse embedded warning.spr") {
        constexpr std::string_view EMBEDDED_WARNING_SPR =
            "data/bitmaps/particles/warning.jpg\n"
            "data/bitmaps/particles/warning_alpha.tga\n"
            "45 45\n"
            "1\n"
            "8\n"
            "0 0\n";

        auto real_spr = luxor::rc::parse_spr(EMBEDDED_WARNING_SPR);
        REQUIRE(real_spr.has_value());
        CHECK(real_spr->grid.cell_w == 45);
        CHECK(real_spr->grid.cell_h == 45);
        CHECK(real_spr->total_frames() == 8);
        CHECK(real_spr->has_alpha == true);
    }
}

TEST_CASE("sprite_file to_json serialization") {
    SUBCASE("sprite_frame to_json") {
        luxor::rc::sprite_frame f;
        f.x = 10;
        f.y = 20;
        f.width = 32;
        f.height = 32;
        f.u_min = 0.0f;
        f.v_min = 0.0f;
        f.u_max = 0.5f;
        f.v_max = 0.5f;

        auto j = f.to_json();
        CHECK(j["x"] == 10);
        CHECK(j["y"] == 20);
        CHECK(j["width"] == 32);
        CHECK(j["height"] == 32);
        CHECK(j["u_min"] == 0.0f);
        CHECK(j["u_max"] == 0.5f);

        nlohmann::json j_adl = f;
        CHECK(j_adl == j);
    }

    SUBCASE("sprite_anim to_json") {
        luxor::rc::sprite_anim a;
        a.name = "walk";
        a.frames.push_back({0, 0, 16, 16, 0.f, 0.f, 1.f, 1.f});

        auto j = a.to_json();
        CHECK(j["name"] == "walk");
        REQUIRE(j["frames"].is_array());
        CHECK(j["frames"].size() == 1);
        CHECK(j["frames"][0]["width"] == 16);

        nlohmann::json j_adl = a;
        CHECK(j_adl == j);
    }

    SUBCASE("full sprite_file to_json") {
        const std::string_view sample =
            "data/tex.jpg\n"
            "data/alpha.tga\n"
            "32 32\n"
            "1\n"
            "2\n"
            "0 0\n";

        auto spr = luxor::rc::parse_spr(sample);
        REQUIRE(spr.has_value());

        auto j = spr->to_json();
        CHECK(j["format"] == "classic");
        CHECK(j["diffuse_texture"] == "data/tex.jpg");
        CHECK(j["alpha_texture"] == "data/alpha.tga");
        CHECK(j["has_alpha"] == true);
        CHECK(j["grid"]["cell_w"] == 32);
        CHECK(j["grid"]["cell_h"] == 32);
        REQUIRE(j["anims"].is_array());
        CHECK(j["anims"].size() == 1);
        CHECK(j["anims"][0]["frames"].size() == 2);

        nlohmann::json j_adl = *spr;
        CHECK(j_adl == j);
    }
}


TEST_CASE("GVF binary fields are little-endian and truncated properties fail") {
    // Three strings: the animation name, its type, and the Rect property key.
    const std::vector<uint8_t> prefix{
        1,'G','V','F','0','0','2',1, 3,0,0,0,
        4,0,0,0,'w','a','l','k', 4,0,0,0,'A','n','i','m',
        4,0,0,0,'R','e','c','t', 0,0,0,0,0,0,0,0,
        0x11,0,0,0,0,1,0,0,0,
        0x13,2,0,0,0
    };
    auto bytes = prefix;
    bytes.insert(bytes.end(), {3,4, 0,0,0,0, 0,0,0,0x3f, 0,0,0x80,0x3f, 0,0,0x80,0x3e});
    auto parsed = luxor::rc::parse_spr(bytes.data(), bytes.size());
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->anims.size() == 1);
    CHECK(parsed->anims[0].name == "walk");
    REQUIRE(parsed->anims[0].frames.size() == 1);
    CHECK(parsed->anims[0].frames[0].u_min == 0.0f);
    CHECK(parsed->anims[0].frames[0].v_min == 0.5f);
    CHECK(parsed->anims[0].frames[0].u_max == 1.0f);
    CHECK(parsed->anims[0].frames[0].v_max == 0.25f);
    for (auto type : {uint8_t{0}, uint8_t{2}}) {
        bytes = prefix;
        bytes.insert(bytes.end(), {type,1});
        CHECK_FALSE(luxor::rc::parse_spr(bytes.data(), bytes.size()).has_value());
    }
    bytes = prefix;
    bytes.insert(bytes.end(), {3,4,0,0,0});
    CHECK_FALSE(luxor::rc::parse_spr(bytes.data(), bytes.size()).has_value());
}
