#include <doctest/doctest.h>

#include <ke/format/archive.hh>
#include <ke/format/bob.hh>
#include <ke/format/dig.hh>
#include <ke/format/tab.hh>
#include <it/it_blocks.hh>
#include <it/it_vm.hh>
#include <it/lzss.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/error.hh>

#include <algorithm>
#include <array>
#include <sstream>

TEST_CASE("KE directory fields use the on-disk little-endian layout") {
    const std::vector<uint8_t> bytes{
        1,0,0,0, 28,0,0,0, 12,0,0,0, 40,0,0,0,
        4,0,0,0, 44,0,0,0, 3,0,0,0,
        0,0, 0,0, 0,0,0,0, 3,0,0,0,
        'A','.','X',0, 'a','b','c'
    };
    std::istringstream input(std::string(bytes.begin(), bytes.end()));
    const auto result = rs::load_resource(input);
    CHECK_FALSE(result.has_errors());
    REQUIRE(result.resources.size() == 1);
    CHECK(result.resources[0].name == "a.x");
    CHECK(result.resources[0].offset == 44);
    CHECK(result.resources[0].size == 3);
    for (size_t n = 0; n < bytes.size(); ++n) {
        CAPTURE(n);
        std::istringstream short_input(std::string(bytes.begin(), bytes.begin() + n));
        CHECK(rs::load_resource(short_input).has_errors());
    }
}

TEST_CASE("KE BOB fields retain 16-bit widths and signed offsets") {
    const std::vector<uint8_t> bytes{
        1,0, 28,0, 0x23,0x01, 0x34,0x02, 28,0, 5,3,
        0xfe,0xff, 3,0, 0xfc,0xff, 5,0,
        0,0, 0,0, 0,0, 0,0, 0,0
    };
    rs::bob_image bob;
    std::string error;
    REQUIRE(bob.parse(bytes, error));
    REQUIRE(bob.frame_count() == 1);
    const auto& frame = bob.frame(0);
    CHECK(frame.width == 0x123);
    CHECK(frame.height == 0x234);
    CHECK(frame.x_off_a == -2);
    CHECK(frame.y_off_a == 3);
    CHECK(frame.x_off_b == -4);
    CHECK(frame.y_off_b == 5);
    for (size_t n = 0; n < bytes.size(); ++n) {
        CAPTURE(n);
        CHECK_FALSE(bob.parse(std::span(bytes).first(n), error));
    }
}

TEST_CASE("KE TAB preserves complete levels and ignores trailing bytes") {
    std::vector<uint8_t> bytes(587);
    bytes[0] = 0x34; bytes[1] = 0x12;
    for (size_t i = 0; i < 8; ++i) bytes[2 + i] = static_cast<uint8_t>(i + 1);
    bytes[10] = 0x80; bytes[11] = 0x42;
    bytes[584] = 0x81; bytes[585] = 0x43;
    bytes[586] = 0xff;
    rs::tab_levels tab;
    std::string error;
    REQUIRE(tab.parse(bytes, error));
    REQUIRE(tab.levels.size() == 1);
    CHECK(tab.levels[0].spawn_period == 0x1234);
    CHECK(tab.levels[0].spawn_seq[7] == 8);
    CHECK(tab.levels[0].cells.front().attribute == 0x80);
    CHECK(tab.levels[0].cells.front().tile == 0x42);
    CHECK(tab.levels[0].cells.back().attribute == 0x81);
    CHECK(tab.levels[0].cells.back().tile == 0x43);
    CHECK_FALSE(tab.parse(std::span(bytes).first(585), error));
}

TEST_CASE("KE DIG reads descriptors and exports the exact single-sample format") {
    const std::vector<uint8_t> bytes{
        'D','I','G','*', 1,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        40,0,0,0, 3,0,0,0, 0x11,0x2b, 0x34,0x12,
        0,0,0,0, 0,0,0,0, 0,128,255
    };
    auto stream = musac::io_from_memory(bytes.data(), bytes.size());
    rs::dig_decoder decoder;
    decoder.open(stream.get());
    CHECK(decoder.sample_count() == 1);
    CHECK(decoder.get_rate() == 11025);
    CHECK(decoder.extract_sample(0) == bytes);
    for (size_t n : {size_t{19}, size_t{39}, size_t{42}}) {
        CAPTURE(n);
        auto short_stream = musac::io_from_memory(bytes.data(), n);
        rs::dig_decoder invalid;
        CHECK_THROWS_AS(invalid.open(short_stream.get()), musac::decoder_error);
    }
}

TEST_CASE("IT fields and truncated fallback use bounded little-endian reads") {
    const std::array<uint8_t, 4> bytes{0x12,0x34,0x56,0x78};
    auto block = decode_block(96, bytes);
    REQUIRE(std::holds_alternative<ui_overlay_action_block>(block));
    CHECK(std::get<ui_overlay_action_block>(block).action_rule_word == 0x78563412);
    for (size_t n = 0; n < bytes.size(); ++n) {
        CAPTURE(n);
        const auto prefix = std::span(bytes).first(n);
        auto truncated = decode_block(96, prefix);
        REQUIRE(std::holds_alternative<generic_asset_block>(truncated));
        CHECK(std::get<generic_asset_block>(truncated).raw_data == std::vector<uint8_t>(prefix.begin(), prefix.end()));
    }
    const std::array<uint8_t, 3> prefixed{0xff,0x12,0x34};
    bstream::span_reader reader(prefixed);
    reader.skip(1);
    auto truncated = decode_block(96, reader, 2);
    REQUIRE(std::holds_alternative<generic_asset_block>(truncated));
    CHECK(std::get<generic_asset_block>(truncated).raw_data == std::vector<uint8_t>{0x12,0x34});
}

TEST_CASE("IT bytecode retains original bytes and rejects truncated operands") {
    const std::array<uint8_t, 6> bytes{0x1d,0x78,0x56,0x34,0x12,0x00};
    const auto instructions = decode_bytecode(bytes);
    REQUIRE(instructions.size() == 2);
    CHECK(instructions[0].param == 0x12345678);
    CHECK(instructions[0].raw_bytes == std::vector<uint8_t>(bytes.begin(), bytes.begin() + 5));
    CHECK(instructions[1].opcode == vm_opcode::RET);
    for (size_t n = 1; n < 5; ++n) {
        CHECK(decode_bytecode(std::span(bytes).first(n)).empty());
    }
    const std::array<uint8_t, 5> lzss{3,'A','B',2,1};
    CHECK(decompress_lzss(lzss, 6) == std::vector<uint8_t>{'A','B','A','B','A','B'});
}

TEST_CASE("IT exports little-endian WAV and big-endian MIDI headers") {
    digital_audio_clip_block audio;
    audio.pcm_samples = {0,128,255};
    const std::vector<uint8_t> expected_wav{
        'R','I','F','F',39,0,0,0,'W','A','V','E','f','m','t',' ',
        16,0,0,0,1,0,1,0,0x11,0x2b,0,0,0x11,0x2b,0,0,
        1,0,8,0,'d','a','t','a',3,0,0,0,0,128,255
    };
    CHECK(audio.to_wav() == expected_wav);
    music_track_block music;
    CHECK(music.to_mid().empty());
    music.midi_stream = {0xff}; // end-of-track event
    const auto midi = music.to_mid();
    const std::vector<uint8_t> header{'M','T','h','d',0,0,0,6,0,0,0,1,0,100,'M','T','r','k'};
    REQUIRE(midi.size() >= 22);
    CHECK(std::equal(header.begin(), header.end(), midi.begin()));
    const uint32_t track_size = (uint32_t(midi[18]) << 24) | (uint32_t(midi[19]) << 16)
                             | (uint32_t(midi[20]) << 8) | midi[21];
    CHECK(track_size == midi.size() - 22);
}
