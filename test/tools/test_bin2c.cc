//
// Created by igor on 26/11/2026.
//

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

// Made at build time by neutrino_embed_files() (test/CMakeLists.txt).
#include "bytes_bin.hh"
#include "empty_bin.hh"
#include "hello_txt.hh"
#include "renamed_hello.hh"

namespace {
    [[nodiscard]] std::vector <std::uint8_t> file_bytes(const char* name) {
        std::ifstream in(std::string{NEUTRINO_EMBED_TEST_DATA_DIR} + "/" + name, std::ios::binary);
        REQUIRE(in);
        return {std::istreambuf_iterator <char>(in), std::istreambuf_iterator <char>()};
    }

    [[nodiscard]] std::vector <std::uint8_t> to_vector(std::span <const std::uint8_t> bytes) {
        return {bytes.begin(), bytes.end()};
    }
}

TEST_SUITE("bin2c / neutrino_embed_files") {
    TEST_CASE("an embedded file holds the file's bytes, in the namespace given") {
        CHECK(to_vector(neutrino_test::embedded::hello_txt_data()) == file_bytes("hello.txt"));
        CHECK(to_vector(neutrino_test::embedded::bytes_bin_data()) == file_bytes("bytes.bin"));
        REQUIRE(neutrino_test::embedded::bytes_bin.size() == 256);
        for (std::size_t i = 0; i < 256; ++i) {
            CHECK(neutrino_test::embedded::bytes_bin[i] == i);
        }
    }

    TEST_CASE("the array is constexpr, so it can be used at compile time") {
        static_assert(neutrino_test::embedded::hello_txt.size() == 15);
        static_assert(neutrino_test::embedded::hello_txt[0] == 'n');
        static_assert(std::is_same_v <decltype(neutrino_test::embedded::hello_txt)::value_type, std::uint8_t>);
    }

    TEST_CASE("an empty file is an empty array") {
        static_assert(neutrino_test::embedded::empty_bin.empty());
        CHECK(neutrino_test::embedded::empty_bin_data().empty());
    }

    TEST_CASE("a symbol of its own, without a namespace") {
        CHECK(to_vector(renamed_hello_data()) == file_bytes("hello.txt"));
    }
}
