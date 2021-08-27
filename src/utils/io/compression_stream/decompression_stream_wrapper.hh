//
// Created by igor on 18/08/2021.
//

#ifndef NEUTRINO_DECOMPRESSION_STREAM_WRAPPER_HH
#define NEUTRINO_DECOMPRESSION_STREAM_WRAPPER_HH

#include <cstdint>

namespace neutrino::utils::detail {
  class decompression_stream_wrapper {
    public:
      decompression_stream_wrapper ();
      virtual ~decompression_stream_wrapper ();

      virtual int decompress (int flags = 0) = 0;

      virtual bool stream_end () const = 0;
      virtual bool done () const = 0;

      virtual const uint8_t *next_in () const = 0;
      virtual long avail_in () const = 0;
      virtual uint8_t *next_out () const = 0;
      virtual long avail_out () const = 0;

      virtual void set_next_in (const unsigned char *in) = 0;
      virtual void set_avail_in (const long in) = 0;
      virtual void set_next_out (const uint8_t *in) = 0;
      virtual void set_avail_out (const long in) = 0;
  };
}

#endif //NEUTRINO_COMPRESSION_STREAM_WRAPPER_HH
