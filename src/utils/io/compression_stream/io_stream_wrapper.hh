//
// Created by igor on 18/08/2021.
//

#ifndef NEUTRINO_IO_STREAM_WRAPPER_HH
#define NEUTRINO_IO_STREAM_WRAPPER_HH

#include <cstdint>

namespace neutrino::utils::detail {
  class io_stream_wrapper {
    public:
      io_stream_wrapper () = default;
      virtual ~io_stream_wrapper ();

      virtual const uint8_t* next_in () const = 0;
      virtual long avail_in () const = 0;
      virtual uint8_t* next_out () const = 0;
      virtual long avail_out () const = 0;

      virtual void set_next_in (const unsigned char* in) = 0;
      virtual void set_avail_in (const long in) = 0;
      virtual void set_next_out (const uint8_t* in) = 0;
      virtual void set_avail_out (const long in) = 0;
  };
}

#endif //NEUTRINO_IO_STREAM_WRAPPER_HH
