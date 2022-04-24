//
// Created by igor on 24/04/2022.
//

#ifndef SRC_UTILS_IO_COMPRESSION_STREAM_COMPRESSION_STREAM_HH
#define SRC_UTILS_IO_COMPRESSION_STREAM_COMPRESSION_STREAM_HH

#include <cstdint>

namespace neutrino::utils::io {
  class compression_stream {
    public:
      virtual ~compression_stream();
      virtual int decompress(const int _flags = 0) =0;
      virtual int compress(const int _flags = 0) =0;
      virtual bool stream_end() const =0;
      virtual bool done() const =0;

      virtual const uint8_t* next_in() const =0;
      virtual long avail_in() const =0;
      virtual uint8_t* next_out() const =0;
      virtual long avail_out() const =0;

      virtual void set_next_in(const unsigned char* in) =0;
      virtual void set_avail_in(const long in) =0;
      virtual void set_next_out(const uint8_t* in) =0;
      virtual void set_avail_out(const long in) =0;
  };
}

#endif //SRC_UTILS_IO_COMPRESSION_STREAM_COMPRESSION_STREAM_HH
