//
// Created by igor on 24/04/2022.
//

#ifndef SRC_UTILS_IO_COMPRESSION_STREAM_COMPRESSION_STREAM_HH
#define SRC_UTILS_IO_COMPRESSION_STREAM_COMPRESSION_STREAM_HH

#include <cstdint>
#include <memory>
#include <cstddef>

#include <neutrino/utils/io/compression_type.hh>

namespace neutrino::utils::io {
  class compression_stream_base {
    public:
      enum flush_mode_t {
        NO_FLUSH,
        PARTIAL_FLUSH,
        SYNC_FLUSH,
        FULL_FLUSH,
        FINISH
      };

      enum status_t {
        OK,
        STREAM_END,
        DATA_ERROR
      };


    public:
      virtual ~compression_stream_base();

      [[nodiscard]] virtual const uint8_t* next_in() const =0;
      [[nodiscard]] virtual std::size_t avail_in() const =0;
      [[nodiscard]] virtual uint8_t* next_out() const =0;
      [[nodiscard]] virtual std::size_t avail_out() const =0;

      virtual void set_next_in(const unsigned char* in) =0;
      virtual void set_avail_in(std::size_t in) =0;
      virtual void set_next_out(const uint8_t* in) =0;
      virtual void set_avail_out(std::size_t in) =0;
      virtual void finalize() = 0;
  };

  class compression_stream : public compression_stream_base {
    public:
      virtual status_t compress(flush_mode_t flags) = 0;

      static std::unique_ptr<compression_stream> create(compression_type_t type,
                                                        compression_level_t level);
  };

  class decompression_stream : public compression_stream_base {
    public:
      virtual status_t decompress(flush_mode_t flags) = 0;
      virtual void reset() = 0;
      static std::unique_ptr<decompression_stream> create(compression_type_t type);

  };


  template <class Impl>
  class compression_stream_impl : public compression_stream, protected Impl {
    public:

      [[nodiscard]] const uint8_t* next_in() const override {
        return static_cast<const Impl*>(this)->next_in();
      };

      [[nodiscard]] std::size_t avail_in() const override {
        return static_cast<const Impl*>(this)->avail_in();
      }

      [[nodiscard]] uint8_t* next_out() const override {
        return static_cast<const Impl*>(this)->next_out();
      }

      [[nodiscard]] std::size_t avail_out() const override {
        return static_cast<const Impl*>(this)->avail_out();
      }

      void set_next_in(const unsigned char* in) override {
        static_cast<Impl*>(this)->set_next_in(in);
      }

      void set_next_out(const uint8_t* in) override {
        static_cast<Impl*>(this)->set_next_out(in);
      }

      void set_avail_in(std::size_t in) override {
        static_cast<Impl*>(this)->set_avail_in(in);
      }

      void set_avail_out(std::size_t in) override {
        static_cast<Impl*>(this)->set_avail_out(in);
      }
  };


  template <class Impl>
  class decompression_stream_impl : public decompression_stream, protected Impl {
    public:

      [[nodiscard]] const uint8_t* next_in() const override {
        return static_cast<const Impl*>(this)->next_in();
      };

      [[nodiscard]] std::size_t avail_in() const override {
        return static_cast<const Impl*>(this)->avail_in();
      }

      [[nodiscard]] uint8_t* next_out() const override {
        return static_cast<const Impl*>(this)->next_out();
      }

      [[nodiscard]] std::size_t avail_out() const override {
        return static_cast<const Impl*>(this)->avail_out();
      }

      void set_next_in(const unsigned char* in) override {
        static_cast<Impl*>(this)->set_next_in(in);
      }

      void set_next_out(const uint8_t* in) override {
        static_cast<Impl*>(this)->set_next_out(in);
      }

      void set_avail_in(std::size_t in) override {
        static_cast<Impl*>(this)->set_avail_in(in);
      }

      void set_avail_out(std::size_t in) override {
        static_cast<Impl*>(this)->set_avail_out(in);
      }
  };





}

#endif //SRC_UTILS_IO_COMPRESSION_STREAM_COMPRESSION_STREAM_HH
