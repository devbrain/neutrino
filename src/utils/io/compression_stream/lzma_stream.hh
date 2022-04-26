//
// Created by igor on 24/04/2022.
//

#ifndef SRC_UTILS_IO_COMPRESSION_STREAM_LZMA_STREAM_HH
#define SRC_UTILS_IO_COMPRESSION_STREAM_LZMA_STREAM_HH

#include "thirdparty/liblzma/src/liblzma/api/lzma.h"
#include <utils/io/compression_stream/compression_stream.hh>

namespace neutrino::utils::io {
  class lzma_compression_stream : public lzma_stream, public compression_stream {
    public:
      lzma_compression_stream(bool isinput, int level = 2, int flags = 0);
      ~lzma_compression_stream();

      int decompress(flush_mode_t flags) override;
      int compress(flush_mode_t flags  /*LZMA_RUN*/) override;
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
      bool is_input;
      lzma_ret ret;
  };
}


#endif //SRC_UTILS_IO_COMPRESSION_STREAM_LZMA_STREAM_HH
