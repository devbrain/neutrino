#include <doctest/doctest.h>
#include <luxor/resources/parser/font_parser.hh>

#include <filesystem>
#include <string_view>

TEST_CASE("font file (.font) parsing") {
    SUBCASE("Classic font format") {
        const std::string_view sample_font =
            "data/bitmaps/fonts/game_font.jpg\n"
            "data/bitmaps/fonts/game_font_alpha.tga\n"
            "16\n"
            "1 3\n"
            "A\n"
            "0 0 10\n"
            "B\n"
            "10 0 10\n"
            " \n"
            "20 0 6\n";

        auto font1 = luxor::rc::parse_font(sample_font);
        REQUIRE(font1.has_value());
        CHECK(font1->diffuse_texture == "data/bitmaps/fonts/game_font.jpg");
        CHECK(font1->alpha_texture == "data/bitmaps/fonts/game_font_alpha.tga");
        CHECK(font1->has_alpha == true);
        CHECK(font1->char_height == 16);
        CHECK(font1->glyph_count == 3);
        CHECK(font1->glyphs.size() == 3);

        CHECK(font1->has_glyph('A'));
        CHECK(font1->has_glyph('B'));
        CHECK(font1->has_glyph(' '));
        CHECK_FALSE(font1->has_glyph('C'));

        const auto* g_a = font1->find_glyph('A');
        REQUIRE(g_a != nullptr);
        CHECK(g_a->x == 0);
        CHECK(g_a->y == 0);
        CHECK(g_a->width == 10);
        CHECK(g_a->height == 16);

        const auto* g_space = font1->find_glyph(' ');
        REQUIRE(g_space != nullptr);
        CHECK(g_space->x == 20);
        CHECK(g_space->y == 0);
        CHECK(g_space->width == 6);
        CHECK(g_space->height == 16);

        // Text width calculation
        CHECK(font1->calculate_text_width("AB") == 20);
        CHECK(font1->calculate_text_width("A B") == 26);
        CHECK(font1->calculate_text_width("AB", 2) == 22);
    }

    SUBCASE("Negative tests") {
        auto bad_font = luxor::rc::parse_font("data/bad.jpg\nnone\nnot_a_number\n");
        CHECK_FALSE(bad_font.has_value());
        CHECK_FALSE(bad_font.error().empty());
    }

    SUBCASE("parse embedded game fonts") {
        constexpr std::string_view EMBEDDED_DEBUG_FONT =
            "data\\bitmaps\\fonts\\debug.jpg\n"
            "data\\bitmaps\\fonts\\debug_alpha.tga\n"
            "11\n"
            "1 72\n"
            "A\n0 0 8\nB\n8 0 7\nC\n15 0 7\nD\n22 0 7\nE\n29 0 7\nF\n36 0 6\n"
            "G\n42 0 8\nH\n50 0 7\nI\n57 0 2\nJ\n59 0 6\nK\n65 0 7\nL\n72 0 7\n"
            "M\n79 0 8\nN\n87 0 7\nO\n94 0 8\nP\n102 0 6\nQ\n108 0 9\nR\n117 0 7\n"
            "S\n124 0 7\nT\n131 0 8\nU\n139 0 7\nV\n146 0 8\nW\n154 0 11\nX\n165 0 7\n"
            "Y\n172 0 7\nZ\n179 0 7\na\n186 0 6\nb\n192 0 7\nc\n199 0 6\nd\n205 0 7\n"
            "e\n212 0 6\nf\n218 0 5\ng\n223 0 6\nh\n229 0 6\ni\n235 0 2\nj\n237 0 4\n"
            "k\n241 0 5\nl\n246 0 3\nm\n249 0 9\nn\n258 0 6\no\n264 0 6\np\n270 0 6\n"
            "q\n276 0 6\nr\n282 0 5\ns\n287 0 6\nt\n293 0 5\nu\n298 0 6\nv\n304 0 6\n"
            "w\n310 0 9\nx\n319 0 6\ny\n325 0 7\nz\n332 0 7\n0\n339 0 6\n1\n345 0 4\n"
            "2\n349 0 6\n3\n355 0 6\n4\n361 0 7\n5\n368 0 6\n6\n374 0 7\n7\n381 0 6\n"
            "8\n387 0 6\n9\n393 0 6\n.\n399 0 2\n,\n401 0 3\n:\n404 0 2\n'\n406 0 3\n"
            "!\n409 0 2\n?\n411 0 6\n\\\n417 0 4\n/\n421 0 3\n-\n424 0 4\n \n428 0 5\n";

        auto debug_font = luxor::rc::parse_font(EMBEDDED_DEBUG_FONT);
        REQUIRE(debug_font.has_value());
        CHECK(debug_font->char_height == 11);
        CHECK(debug_font->glyph_count == 72);
        CHECK(debug_font->has_glyph('A'));
        CHECK(debug_font->has_glyph('z'));
        CHECK(debug_font->has_glyph('0'));
        CHECK(debug_font->has_glyph(' '));

        constexpr std::string_view EMBEDDED_HUD_FONT =
            "data\\bitmaps\\fonts\\hud.jpg\n"
            "data\\bitmaps\\fonts\\hud_alpha.tga\n"
            "21\n"
            "1 13\n"
            "0\n0 0 20\n1\n20 0 8\n2\n28 0 15\n3\n43 0 14\n4\n57 0 18\n"
            "5\n75 0 14\n6\n89 0 16\n7\n105 0 15\n8\n120 0 16\n9\n136 0 16\n"
            ",\n152 0 9\n-\n161 0 10\n \n171 0 8\n";

        auto hud_font = luxor::rc::parse_font(EMBEDDED_HUD_FONT);
        REQUIRE(hud_font.has_value());
        CHECK(hud_font->char_height == 21);
        CHECK(hud_font->glyph_count == 13);
        CHECK(hud_font->has_glyph('9'));
        CHECK(hud_font->has_glyph(' '));

        constexpr std::string_view EMBEDDED_DIALOG_SCORE_FONT =
            "data\\bitmaps\\fonts\\dialog_score.jpg\n"
            "data\\bitmaps\\fonts\\dialog_score_alpha.tga\n"
            "35\n"
            "1 12\n"
            "0\n0 0 33\n1\n33 0 14\n2\n47 0 25\n3\n72 0 24\n4\n96 0 30\n"
            "5\n126 0 23\n6\n149 0 27\n7\n176 0 25\n8\n201 0 27\n9\n228 0 28\n"
            ",\n256 0 15\n \n271 0 12\n";

        auto dialog_font = luxor::rc::parse_font(EMBEDDED_DIALOG_SCORE_FONT);
        REQUIRE(dialog_font.has_value());
        CHECK(dialog_font->char_height == 35);
        CHECK(dialog_font->glyph_count == 12);
    }
}

TEST_CASE("font_file to_json serialization") {
    SUBCASE("font_glyph to_json") {
        luxor::rc::font_glyph g;
        g.codepoint = U'A';
        g.character = "A";
        g.x = 10;
        g.y = 5;
        g.width = 12;
        g.height = 16;

        auto j = g.to_json();
        CHECK(j["codepoint"] == static_cast<uint32_t>('A'));
        CHECK(j["character"] == "A");
        CHECK(j["x"] == 10);
        CHECK(j["y"] == 5);
        CHECK(j["width"] == 12);
        CHECK(j["height"] == 16);

        nlohmann::json j_adl = g;
        CHECK(j_adl == j);
    }

    SUBCASE("full font_file to_json") {
        const std::string_view sample =
            "data/font.jpg\n"
            "data/font_alpha.tga\n"
            "16\n"
            "1 2\n"
            "A\n"
            "0 0 10\n"
            "B\n"
            "10 0 10\n";

        auto font = luxor::rc::parse_font(sample);
        REQUIRE(font.has_value());

        auto j = font->to_json();
        CHECK(j["diffuse_texture"] == "data/font.jpg");
        CHECK(j["alpha_texture"] == "data/font_alpha.tga");
        CHECK(j["has_alpha"] == true);
        CHECK(j["char_height"] == 16);
        CHECK(j["rows"] == 1);
        CHECK(j["glyph_count"] == 2);
        REQUIRE(j["glyphs"].is_array());
        CHECK(j["glyphs"].size() == 2);
        CHECK(j["glyphs"][0]["character"] == "A");
        CHECK(j["glyphs"][0]["width"] == 10);
        CHECK(j["glyphs"][1]["character"] == "B");

        nlohmann::json j_adl = *font;
        CHECK(j_adl == j);
    }
}

