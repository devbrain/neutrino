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

using namespace neutrino::utils::io;

TEST_SUITE("zlib") {
    TEST_CASE("testDeflate1")
    {
        std::stringstream buffer;
        deflating_output_stream deflater(buffer);
        deflater << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        deflater << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        deflater.close();
        inflating_input_stream inflater(buffer);
        std::string data;
        inflater >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
        inflater >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
    }

    TEST_CASE("testDeflate2") {
        std::stringstream buffer;
        deflating_output_stream deflater(buffer);
        deflater << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        deflater << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        deflater.close();
        std::stringstream buffer2;
        inflating_output_stream inflater(buffer2);
        stream_copier::copy_stream(buffer, inflater);
        inflater.close();
        std::string data;
        buffer2 >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
        buffer2 >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
    }
    TEST_CASE("testDeflate3")
    {
        std::stringstream buffer;
        buffer << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        buffer << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        deflating_input_stream deflater(buffer);
        std::stringstream buffer2;
        stream_copier::copy_stream(deflater, buffer2);
        std::stringstream buffer3;
        inflating_output_stream inflater(buffer3);
        stream_copier::copy_stream(buffer2, inflater);
        inflater.close();
        std::string data;
        buffer3 >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
        buffer3 >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
    }
    TEST_CASE("testDeflate4")
    {
        std::vector<char> buffer(1024);
        memory_output_stream ostr(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        deflating_output_stream deflater(ostr, -10, ZLIB_BEST_SPEED);
        std::string data(36828, 'x');
        deflater << data;
        deflater.close();
        memory_input_stream istr(buffer.data(), ostr.chars_written());
        inflating_input_stream inflater(istr, -10);
        std::string data2;
        inflater >> data2;
        REQUIRE (data2 == data);
    }

    TEST_CASE("testGzip1")
    {
        std::stringstream buffer;
        deflating_output_stream deflater(buffer, deflating_stream_buf::STREAM_GZIP);
        deflater << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        deflater << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        deflater.close();
        inflating_input_stream inflater(buffer, inflating_stream_buf::STREAM_GZIP);
        std::string data;
        inflater >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
        inflater >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
    }


    TEST_CASE("testGzip2")
    {
        // created with gzip ("Hello, world!"):
        const unsigned char gzdata[] =
                {
                        0x1f, 0x8b, 0x08, 0x08, 0xb0, 0x73, 0xd0, 0x41, 0x00, 0x03, 0x68, 0x77, 0x00, 0xf3, 0x48, 0xcd,
                        0xc9, 0xc9, 0xd7, 0x51, 0x28, 0xcf, 0x2f, 0xca, 0x49, 0x51, 0xe4, 0x02, 0x00, 0x18, 0xa7, 0x55,
                        0x7b, 0x0e, 0x00, 0x00, 0x00, 0x00
                };

        std::string gzstr((char*) gzdata, sizeof(gzdata));
        std::istringstream istr(gzstr);
        inflating_input_stream inflater(istr, inflating_stream_buf::STREAM_GZIP);
        std::string data;
        inflater >> data;
        REQUIRE (data == "Hello,");
        inflater >> data;
        REQUIRE (data == "world!");
    }


    TEST_CASE("testGzip3")
    {
        std::stringstream buffer;
        deflating_output_stream deflater1(buffer, deflating_stream_buf::STREAM_GZIP);
        deflater1 << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        deflater1 << "abcdefabcdefabcdefabcdefabcdefabcdef" << std::endl;
        deflater1.close();
        deflating_output_stream deflater2(buffer, deflating_stream_buf::STREAM_GZIP);
        deflater2 << "bcdefabcdefabcdefabcdefabcdefabcdefa" << std::endl;
        deflater2 << "bcdefabcdefabcdefabcdefabcdefabcdefa" << std::endl;
        deflater2.close();
        inflating_input_stream inflater(buffer, inflating_stream_buf::STREAM_GZIP);
        std::string data;
        inflater >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
        inflater >> data;
        REQUIRE (data == "abcdefabcdefabcdefabcdefabcdefabcdef");
        data.clear();
        inflater >> data;
        REQUIRE (data.empty());
        REQUIRE (inflater.eof());
        inflater.reset();
        inflater >> data;
        REQUIRE (data == "bcdefabcdefabcdefabcdefabcdefabcdefa");
        inflater >> data;
        REQUIRE (data == "bcdefabcdefabcdefabcdefabcdefabcdefa");
    }
}
