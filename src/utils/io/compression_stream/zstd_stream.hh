//
// Created by igor on 24/04/2022.
//

#ifndef SRC_UTILS_IO_COMPRESSION_STREAM_ZSTD_STREAM_HH
#define SRC_UTILS_IO_COMPRESSION_STREAM_ZSTD_STREAM_HH

#include "utils/io/compression_stream/compression_stream.hh"

#define ZSTD_STATIC_LINKING_ONLY   /* ZSTD_isFrame, ZSTD_MAGICNUMBER, ZSTD_customMem */
#include "zstd.h"

namespace neutrino::utils::io {
  class zstd_stream : public compression_stream {
    public:
      zstd_stream (bool is_input = true, int level = ZSTD_defaultCLevel ());
      ~zstd_stream () override;

      int decompress (const int flags) override;
      int compress (const int flags) override;
      bool stream_end () const override;
      bool done () const override;

      const uint8_t* next_in () const override;
      long avail_in () const override;
      uint8_t* next_out () const override;
      long avail_out () const override;

      void set_next_in (const unsigned char* in) override;
      void set_avail_in (const long in) override;
      void set_next_out (const uint8_t* in) override;
      void set_avail_out (const long in) override;
    private:
      void update_inbuffer ();
      void update_outbuffer ();
      void update_stream_state ();
    private:
      bool isInput;
      size_t ret;

      size_t buffInSize;
      void* buffIn;
      size_t buffOutSize;
      void* buffOut;

      ZSTD_DCtx* dctx;
      ZSTD_CCtx* cctx;

      ZSTD_inBuffer input;
      ZSTD_outBuffer output;
  };
}

#endif //SRC_UTILS_IO_COMPRESSION_STREAM_ZSTD_STREAM_HH
