#include <doctest/doctest.h>
#include <neutrino/audio/audio.hh>

#include "test_application.hh"

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
        neutrino::test::test_application test_app("Audio test scaffolding");

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
    }
}
