//
// Created by igor on 24/04/2022.
//

#ifndef SRC_UTILS_IO_COMPRESSION_STREAM_BZIP_STREAM_HH
#define SRC_UTILS_IO_COMPRESSION_STREAM_BZIP_STREAM_HH

#include <thirdparty/bzip2/bzlib.h>
#include <utils/io/compression_stream/compression_stream.hh>

namespace neutrino::utils::io {
  class bzip_stream : public bz_stream, public compression_stream {
    public:
      bzip_stream(bool is_input, int level = 9, int wf = 30);
      ~bzip_stream();

      int decompress(flush_mode_t flags) override;
      int compress(flush_mode_t flags) override;
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
      int ret;
  };
}


#endif //SRC_UTILS_IO_COMPRESSION_STREAM_BZIP_STREAM_HH
