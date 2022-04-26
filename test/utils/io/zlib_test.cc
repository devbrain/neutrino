//
// Created by igor on 08/07/2021.
//

#include <doctest/doctest.h>
#include <sstream>
#include <vector>
#include <neutrino/utils/io/deflating_stream.hh>
#include <neutrino/utils/io/inflating_stream.hh>
#include <neutrino/utils/io/memory_stream_buf.hh>
#include <neutrino/utils/io/stream_copier.hh>
#include <neutrino/utils/strings/string_constant.hh>
#include <neutrino/utils/macros.hh>

using namespace neutrino::utils::io;

template <compression_type_t type>
struct CompTypeHolder {
  static constexpr auto TYPE = type;
};

#define DECLARE_TEST_STRING_PAIR(NAME, T, S1, S2)                   \
  struct PPCAT(PPCAT(NAME,_),T) {                                   \
    static constexpr auto TYPE = compression_type_t::T;             \
    static constexpr auto m1 = neutrino::utils::string_factory(S1); \
    static constexpr auto m2 = neutrino::utils::string_factory(S2); \
  };                                                                \
  TYPE_TO_STRING(PPCAT(PPCAT(NAME,_),T))

#define DECLARE_TEST_STRING_PAIR_ALL(NAME, S1, S2)  \
  DECLARE_TEST_STRING_PAIR(NAME, ZLIB, S1, S2);     \
  DECLARE_TEST_STRING_PAIR(NAME, GZIP, S1, S2);     \
  DECLARE_TEST_STRING_PAIR(NAME, ZLIB_RAW, S1, S2); \
  DECLARE_TEST_STRING_PAIR(NAME, ZSTD, S1, S2);     \
  DECLARE_TEST_STRING_PAIR(NAME, BZ2, S1, S2);

DECLARE_TEST_STRING_PAIR_ALL(T0, "", "");
DECLARE_TEST_STRING_PAIR_ALL(T1, "abcdefabcdefabcdefabcdefabcdefabcdef", "abcdefabcdefabcdefabcdefabcdefabcdef");
DECLARE_TEST_STRING_PAIR_ALL(T2, "a", "a");

#define DECLARE_COMPRESSION_TYPE(T) \
  using T = CompTypeHolder<compression_type_t::T>; \
  TYPE_TO_STRING(T);

DECLARE_COMPRESSION_TYPE(ZLIB);
DECLARE_COMPRESSION_TYPE(GZIP);
DECLARE_COMPRESSION_TYPE(ZLIB_RAW);
DECLARE_COMPRESSION_TYPE(ZSTD);
DECLARE_COMPRESSION_TYPE(BZ2);
DECLARE_COMPRESSION_TYPE(LZMA);

TEST_CASE_TEMPLATE_DEFINE("test deflate 1", T, test_id1) {
  std::stringstream buffer;
  const auto m1 = T::m1.to_string ();
  const auto m2 = T::m2.to_string ();

  deflating_output_stream deflater (buffer, T::TYPE);

  deflater << m1 << std::endl;
  deflater << m2 << std::endl;
  deflater.close ();
  inflating_input_stream inflater (buffer, T::TYPE);
  std::string data;
  inflater >> data;
  REQUIRE (data == m1);
  inflater >> data;
  REQUIRE (data == m2);
}

TEST_CASE_TEMPLATE_DEFINE("test deflate 2", T, test_id2) {

  const auto m1 = T::m1.to_string ();
  const auto m2 = T::m2.to_string ();

  std::stringstream buffer;
  deflating_output_stream deflater (buffer, T::TYPE);
  deflater << m1 << std::endl;
  deflater << m2 << std::endl;
  deflater.close ();
  std::stringstream buffer2;
  inflating_output_stream inflater (buffer2, T::TYPE);
  stream_copier::copy_stream (buffer, inflater);
  inflater.close ();
  std::string data;
  buffer2 >> data;
  REQUIRE (data == m1);
  buffer2 >> data;
  REQUIRE (data == m2);
}

TEST_CASE_TEMPLATE_DEFINE("test deflate 3", T, test_id3) {
  std::stringstream buffer;
  const auto m1 = T::m1.to_string ();
  const auto m2 = T::m2.to_string ();
  buffer << m1 << std::endl;
  buffer << m2 << std::endl;
  deflating_input_stream deflater (buffer, T::TYPE);
  std::stringstream buffer2;
  stream_copier::copy_stream (deflater, buffer2);
  std::stringstream buffer3;
  inflating_output_stream inflater (buffer3, T::TYPE);
  stream_copier::copy_stream (buffer2, inflater);
  inflater.close ();
  std::string data;
  buffer3 >> data;
  REQUIRE (data == m1);
  buffer3 >> data;
  REQUIRE (data == m2);
}

TEST_CASE_TEMPLATE_DEFINE("test deflate 4", T, test_id4) {
  std::vector<char> buffer (1024);
  memory_output_stream ostr (buffer.data (), static_cast<std::streamsize>(buffer.size ()));
  deflating_output_stream deflater (ostr, T::TYPE, compression_level_t::BEST);
  std::string data (36828, 'x');
  deflater << data;
  deflater.close ();
  memory_input_stream istr (buffer.data (), ostr.chars_written ());
  inflating_input_stream inflater (istr, T::TYPE);
  std::string data2;
  inflater >> data2;
  REQUIRE (data2 == data);
}

static std::string gen_random (std::size_t len) {
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::string tmp_s;
  tmp_s.reserve (len);

  for (std::size_t i = 0; i < len; i++) {
    tmp_s += alphanum[rand () % (sizeof (alphanum) - 1)];
  }

  return tmp_s;
}

TEST_CASE_TEMPLATE_DEFINE("test deflate 5", T, test_id5) {
  std::vector<char> buffer (5 * 36828);
  memory_output_stream ostr (buffer.data (), static_cast<std::streamsize>(buffer.size ()));
  deflating_output_stream deflater (ostr, T::TYPE, compression_level_t::BEST);
  std::string data = gen_random (36828);
  deflater << data;
  deflater.close ();
  memory_input_stream istr (buffer.data (), ostr.chars_written ());
  inflating_input_stream inflater (istr, T::TYPE);
  std::string data2;
  inflater >> data2;
  REQUIRE (data2 == data);
}

TEST_SUITE("zlib") {

#define GEN_T(N, ALG) PPCAT(PPCAT(PPCAT(T, N),_),ALG)
#define GEN_RAW(N) GEN_T(N, ZLIB),  GEN_T(N, ZLIB_RAW), GEN_T(N, ZSTD), GEN_T(N, BZ2)
#define ALGS ZLIB, ZLIB_RAW, GZIP, ZSTD, BZ2

  TEST_CASE_TEMPLATE_INVOKE(test_id1, GEN_RAW (0));
  TEST_CASE_TEMPLATE_INVOKE(test_id1, GEN_RAW (1));
  TEST_CASE_TEMPLATE_INVOKE(test_id1, GEN_RAW (2));

  TEST_CASE_TEMPLATE_INVOKE(test_id2, GEN_RAW (0));
  TEST_CASE_TEMPLATE_INVOKE(test_id2, GEN_RAW (1));
  TEST_CASE_TEMPLATE_INVOKE(test_id2, GEN_RAW (2));

  TEST_CASE_TEMPLATE_INVOKE(test_id3, GEN_RAW (0));
  TEST_CASE_TEMPLATE_INVOKE(test_id3, GEN_RAW (1));
  TEST_CASE_TEMPLATE_INVOKE(test_id3, GEN_RAW (2));

  TEST_CASE_TEMPLATE_INVOKE(test_id4, ALGS);
  TEST_CASE_TEMPLATE_INVOKE(test_id5, ALGS);


  TEST_CASE("testGzip2")
  {
    // created with gzip ("Hello, world!"):
    const unsigned char gzdata[] =
        {
            0x1f, 0x8b, 0x08, 0x08, 0xb0, 0x73, 0xd0, 0x41, 0x00, 0x03, 0x68, 0x77, 0x00, 0xf3, 0x48, 0xcd,
            0xc9, 0xc9, 0xd7, 0x51, 0x28, 0xcf, 0x2f, 0xca, 0x49, 0x51, 0xe4, 0x02, 0x00, 0x18, 0xa7, 0x55,
            0x7b, 0x0e, 0x00, 0x00, 0x00, 0x00
        };

    std::string gzstr ((char*) gzdata, sizeof (gzdata));
    std::istringstream istr (gzstr);
    inflating_input_stream inflater (istr, compression_type_t::GZIP);
    std::string data;
    inflater >> data;
    REQUIRE (data == "Hello,");
    inflater >> data;
    REQUIRE (data == "world!");
  }
}
