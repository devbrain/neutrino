//
// Created by igor on 24/04/2022.
//

#ifndef SRC_UTILS_IO_COMPRESSION_STREAM_LIBZ_STREAM_HH
#define SRC_UTILS_IO_COMPRESSION_STREAM_LIBZ_STREAM_HH

#include <utils/io/compression_stream/compression_stream.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::utils::io {
  namespace detail {
    struct libz_impl;
  }

  class libz_stream {
    public:

    public:
      libz_stream();
      ~libz_stream();

      [[nodiscard]] const uint8_t* next_in() const;
      [[nodiscard]] std::size_t avail_in() const;
      [[nodiscard]] uint8_t* next_out() const;
      [[nodiscard]] std::size_t avail_out() const;

      void set_next_in(const unsigned char* in);
      void set_avail_in(std::size_t in);
      void set_next_out(const uint8_t* in);
      void set_avail_out(std::size_t in);
    protected:
      spimpl::unique_impl_ptr<detail::libz_impl> m_pimpl;
  };

  enum class libz_kind_t {
    ZLIB,
    GZIP,
    RAW,
    ZSTD
  };

  class libz_compressor : public compression_stream_impl<libz_stream> {
    public:
      libz_compressor(libz_kind_t kind, compression_level_t level);
      status_t compress(flush_mode_t flags) override;
      void finalize() override;
  };

  class libz_decompressor : public decompression_stream_impl<libz_stream> {
    public:
      explicit libz_decompressor(libz_kind_t kind);
      status_t decompress(flush_mode_t flags) override;
      void finalize() override;
      void reset() override;
  };
}

#endif //SRC_UTILS_IO_COMPRESSION_STREAM_LIBZ_STREAM_HH
