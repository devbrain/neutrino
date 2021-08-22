//
// Created by igor on 18/08/2021.
//

#ifndef NEUTRINO_ZLIB_STREAM_WRAPPER_HH
#define NEUTRINO_ZLIB_STREAM_WRAPPER_HH

#include <thirdparty/zlib/zlib.h>
#include "compression_stream_wrapper.hh"
#include "decompression_stream_wrapper.hh"

namespace neutrino::utils::detail {
    class zlib_compression_stream_wrapper : public compression_stream_wrapper {
    public:

    };

    class zlib_decompression_stream_wrapper : public decompression_stream_wrapper {
    public:

    };
}


#endif //NEUTRINO_ZLIB_STREAM_WRAPPER_HH
