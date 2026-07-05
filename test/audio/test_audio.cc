#include <doctest/doctest.h>
#include <neutrino/application.hh>
#include <neutrino/audio/audio.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>

#include <cstdint>
#include <sstream>
#include <string>

// Minimal valid WAV: 8000 Hz, mono, 16-bit PCM, 200 samples of silence.
static std::string make_test_wav() {
    std::string s;
    auto u32 = [&s](std::uint32_t v) {
        for (int i = 0; i < 4; i++) s.push_back(static_cast<char>((v >> (8 * i)) & 0xff));
    };
    auto u16 = [&s](std::uint16_t v) {
        for (int i = 0; i < 2; i++) s.push_back(static_cast<char>((v >> (8 * i)) & 0xff));
    };
    const std::uint32_t sample_rate = 8000;
    const std::uint16_t channels = 1;
    const std::uint16_t bits = 16;
    const std::uint32_t data_size = 200 * channels * bits / 8;

    s += "RIFF"; u32(36 + data_size); s += "WAVE";
    s += "fmt "; u32(16); u16(1); u16(channels); u32(sample_rate);
    u32(sample_rate * channels * bits / 8); u16(channels * bits / 8); u16(bits);
    s += "data"; u32(data_size); s.append(data_size, '\0');
    return s;
}

TEST_SUITE("neutrino::audio") {
    TEST_CASE("audio lifecycle and speaker creation") {
        // Create config with hidden window if possible, or minimal size
        neutrino::application_config cfg;
        cfg.title = "Audio test scaffolding";
        cfg.width = 100;
        cfg.height = 100;
        cfg.flags = sdlpp::window_flags::hidden;

        // Instantiating the application brings the audio system up; there is
        // no separate init step.
        neutrino::application app(cfg);
        auto* abs_app = static_cast<sdlpp::abstract_application*>(&app);
        abs_app->init_sdl_();
        abs_app->on_init(0, nullptr);

        CHECK(neutrino::audio_active());

        // Volume controls
        neutrino::set_master_volume(0.8f);
        CHECK(neutrino::get_master_volume() == 0.8f);

        neutrino::set_sfx_volume(0.5f);
        CHECK(neutrino::get_sfx_volume() == 0.5f);

        neutrino::set_music_volume(0.7f);
        CHECK(neutrino::get_music_volume() == 0.7f);

        // PC Speaker Stream
        auto speaker = neutrino::create_pc_speaker();
        CHECK(speaker.is_queue_empty());

        speaker.beep(1200.0f);
        CHECK(speaker.queue_size() == 1);
        CHECK_FALSE(speaker.is_queue_empty());

        speaker.clear_queue();
        CHECK(speaker.is_queue_empty());

        // Test MML parsing
        bool ok = speaker.play_mml("T120 O4 L8 C D E F G A B >C");
        CHECK(ok);
        CHECK_FALSE(speaker.is_queue_empty());

        // Music slot is empty by default
        CHECK_FALSE(neutrino::is_music_playing());

        // ------------------------------------------------------------
        // istream-based loading. The stream is consumed during the call
        // and can go out of scope afterwards.
        // ------------------------------------------------------------
        const auto wav = make_test_wav();

        {
            std::istringstream in(wav);
            auto sfx = neutrino::load_sfx(in);
            CHECK_FALSE(sfx.is_playing());
            sfx.play();
            sfx.stop();
        }

        {
            std::istringstream in(wav);
            auto music = neutrino::load_music(in);
            CHECK_FALSE(music.is_playing());
            music.play(/*loop=*/true);
            CHECK(music.is_playing());
            music.stop();
        }

        {
            std::istringstream in(wav);
            neutrino::play_music(in, /*loop=*/true);
            CHECK(neutrino::is_music_playing());
            neutrino::stop_music();
            CHECK_FALSE(neutrino::is_music_playing());
        }

        // ------------------------------------------------------------
        // Video drawing tests (uses same application context)
        // ------------------------------------------------------------
        sdlpp::color red = sdlpp::colors::red;
        auto color_res = neutrino::set_draw_color(red);
        REQUIRE(color_res.has_value());

        auto get_color_res = neutrino::get_draw_color();
        REQUIRE(get_color_res.has_value());
        CHECK(get_color_res.value().r == red.r);
        CHECK(get_color_res.value().g == red.g);
        CHECK(get_color_res.value().b == red.b);

        // Test drawing points
        auto draw_pt_res = neutrino::draw_point(10, 20);
        CHECK(draw_pt_res.has_value());

        auto draw_pt_color_res = neutrino::draw_point(15, 25, sdlpp::colors::green);
        CHECK(draw_pt_color_res.has_value());

        // Test drawing lines
        auto draw_line_res = neutrino::draw_line(0, 0, 50, 50);
        CHECK(draw_line_res.has_value());

        auto draw_line_color_res = neutrino::draw_line(10, 0, 10, 40, sdlpp::colors::blue);
        CHECK(draw_line_color_res.has_value());

        // Test drawing rects
        auto draw_rect_res = neutrino::draw_rect(10, 10, 40, 40);
        CHECK(draw_rect_res.has_value());

        auto draw_rect_color_res = neutrino::draw_rect(5, 5, 25, 25, sdlpp::colors::white);
        CHECK(draw_rect_color_res.has_value());

        // Test drawing filled rects
        auto fill_rect_res = neutrino::draw_rect_fill(15, 15, 30, 30);
        CHECK(fill_rect_res.has_value());

        // Test drawing circles
        auto draw_circle_res = neutrino::draw_circle(50, 50, 10);
        CHECK(draw_circle_res.has_value());

        auto fill_circle_res = neutrino::draw_circle_fill(50, 50, 10, sdlpp::colors::yellow);
        CHECK(fill_circle_res.has_value());

        // Test drawing arrows
        auto draw_arrow_res = neutrino::draw_arrow(neutrino::point{0, 0}, neutrino::point{20, 20});
        CHECK(draw_arrow_res.has_value());

        // Test drawing crosses
        auto draw_cross_res = neutrino::draw_cross(neutrino::point{50, 50}, 5, sdlpp::colors::cyan);
        CHECK(draw_cross_res.has_value());

        abs_app->on_quit();
        abs_app->shutdown_sdl_();
    }
}
