//
// Created by igor on 24/04/2022.
//

#ifndef SRC_UTILS_IO_COMPRESSION_STREAM_LIBZ_STREAM_HH
#define SRC_UTILS_IO_COMPRESSION_STREAM_LIBZ_STREAM_HH

#include <thirdparty/zlib/zlib.h>
#include <utils/io/compression_stream/compression_stream.hh>

namespace neutrino::utils::io {
  class libz_stream : public z_stream, public compression_stream {
    public:
      libz_stream(bool is_input, int level = Z_DEFAULT_COMPRESSION);
      ~libz_stream();

      int decompress(const int flags = Z_NO_FLUSH) override;
      int compress(const int flags = Z_NO_FLUSH) override;
      bool stream_end() const override;
      bool done() const override;

      const uint8_t* next_in() const override;
      long avail_in() const override;
      uint8_t* next_out() const override;
      long avail_out() const override;

      void set_next_in(const unsigned char* in) override;
      void set_avail_in(const long in) override;
      void set_next_out(const uint8_t* in) override;
      void set_avail_out(const long in) override;
    private:
      bool m_is_input;
      int m_ret;
  };
}

#endif //SRC_UTILS_IO_COMPRESSION_STREAM_LIBZ_STREAM_HH
