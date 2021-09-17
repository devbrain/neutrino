//
// Created by igor on 02/06/2020.
//

#ifndef NEUTRINO_SDL_IO_HH
#define NEUTRINO_SDL_IO_HH

#include <hal/sdl/sdl2.hh>
#include <hal/sdl/object.hh>
#include <hal/sdl/call.hh>
#include <neutrino/utils/mp/introspection.hh>
#include <type_traits>
#include <cstdint>
#include <string>
#include <cstdio>

namespace neutrino::sdl {
  enum class whence : int {
      SET = RW_SEEK_SET,  /**< Seek from the beginning of data */
      CUR = RW_SEEK_CUR, /**< Seek relative to current read point */
      END = RW_SEEK_END  /**< Seek relative to the end of data */
  };

  class io : public object<SDL_RWops> {
    public:
      // Use this function to prepare a read-only memory buffer for use with RWops.
      io (const void* mem, std::size_t size);
      io (void* mem, std::size_t size);
      io (const std::string& filename, bool read_only);
      io (FILE* fp, bool auto_close);

      io (object <SDL_RWops>&& other);
      io& operator = (object <SDL_RWops>&& other);

      /*
       * ptr - a pointer to a buffer to read data into
       * size - the size of each object to read, in bytes
       * maxnum - the maximum number of objects to be read
       * Returns the number of objects read, or 0 at error or end of file; call SDL_GetError() for more information.
       */
      [[nodiscard]] size_t read (void* ptr,
                                 size_t size,
                                 size_t maxnum);
      /*
       * ptr - a pointer to a buffer containing data to write
       * size - the size of an object to write, in bytes
       * num - the number of objects to write
       * Returns the number of objects written, which will be less than num on error; call SDL_GetError() for more information.
       */
      [[nodiscard]] size_t write (const void* ptr,
                                  size_t size,
                                  size_t maxnum);
      // Returns the offset in the stream after seek, or throws exception
      [[nodiscard]] uint64_t seek (int64_t offset, whence w);
      // Returns the current offset in the stream, or throws exception
      [[nodiscard]] uint64_t tell ();
  };

  namespace detail::rwops {
    GENERATE_HAS_MEMBER_FUNCTION(close);

    GENERATE_HAS_MEMBER_FUNCTION(seek);

    GENERATE_HAS_MEMBER_FUNCTION(read);

    GENERATE_HAS_MEMBER_FUNCTION(write);

    GENERATE_HAS_MEMBER_FUNCTION(size);

    GENERATE_HAS_MEMBER_STATIC(type_id);
  }

  template <class RWImpl>
  class rwops_base : public object<SDL_RWops> {
    public:
      rwops_base ()
          : object<SDL_RWops> (_create (), true) {
        handle ()->hidden.unknown.data1 = this;
      }
      // --------------------------------------------------------------------------
    private:
#define _dENFORCE_TYPE_RWOPS(ret)                                                                           \
    if constexpr (detail::rwops::has_type_id<RWImpl, uint32_t>::value)                                      \
        if (ctx->type != RWImpl::type_id) {                                                                 \
            SDL_SetError("Wrong kind of SDL_RWops. given : %d , expected %d", ctx->type, RWImpl::type_id);  \
            return ret;                                                                                     \
        } ((void)0)

      static SDL_RWops* _create () {
        auto* ret = SDL_AllocRW ();

        if constexpr (detail::rwops::has_type_id<RWImpl, uint32_t>::value) {
          static_assert (!(
                             (RWImpl::type_id == SDL_RWOPS_WINFILE) ||
                             (RWImpl::type_id == SDL_RWOPS_STDFILE) ||
                             (RWImpl::type_id == SDL_RWOPS_JNIFILE) ||
                             (RWImpl::type_id == SDL_RWOPS_MEMORY) ||
                             (RWImpl::type_id == SDL_RWOPS_MEMORY_RO)),
                         "Predefined RWOps type is used");

          ret->type = RWImpl::type_id;
        }
        else {
          ret->type = SDL_RWOPS_UNKNOWN;
        }

        if constexpr (detail::rwops::has_size<RWImpl, int64_t ()>::value) {
          ret->size = [] (SDL_RWops* ctx) -> int64_t {
            _dENFORCE_TYPE_RWOPS(-1);
            return reinterpret_cast<RWImpl*>(ctx->hidden.unknown.data1)->size ();
          };
        }
        else {
          /* size is a function pointer that reports the stream's total size in bytes.
           * If the stream size can't be determined (either because it doesn't make sense for the stream type, or there was an error),
           * this function returns -1.
           */
          ret->size = [] ([[maybe_unused]] SDL_RWops* ctx) -> int64_t {
            SDL_SetError ("Method size is not implemented");
            return -1;
          };
        }

        if constexpr (detail::rwops::has_seek<RWImpl, int64_t (int64_t, whence)>::value) {
          ret->seek = [] (SDL_RWops* ctx, Sint64 v, int o) -> int64_t {
            _dENFORCE_TYPE_RWOPS(-1);
            return reinterpret_cast<RWImpl*>(ctx->hidden.unknown.data1)->seek (v, static_cast<whence>(o));
          };
        }
        else {
          ret->seek = [] (SDL_RWops*, Sint64, int) -> int64_t {
            SDL_SetError ("Method seek is not implemented");
            return -1;
          };
        }
        if constexpr (detail::rwops::has_read<RWImpl, size_t (void*, size_t, size_t)>::value) {
          ret->read = [] (SDL_RWops* ctx, void* buff, size_t s, size_t n) -> size_t {
            _dENFORCE_TYPE_RWOPS(0);
            return reinterpret_cast<RWImpl*>(ctx->hidden.unknown.data1)->read (buff, s, n);
          };
        }
        else {
          ret->read = [] ([[maybe_unused]] SDL_RWops* ctx, [[maybe_unused]] void* buff, [[maybe_unused]] size_t s,
                          [[maybe_unused]] size_t n) -> size_t {
            SDL_SetError ("Method read is not implemented");
            return 0;
          };
        }
        if constexpr (detail::rwops::has_write<RWImpl, size_t (const void*, size_t, size_t)>::value) {
          ret->write = [] (SDL_RWops* ctx, const void* buff, size_t s, size_t n) -> size_t {
            _dENFORCE_TYPE_RWOPS(0);
            return reinterpret_cast<RWImpl*>(ctx->hidden.unknown.data1)->write (buff, s, n);
          };
        }
        else {
          ret->write = [] ([[maybe_unused]] SDL_RWops* ctx, [[maybe_unused]] const void* buff,
                           [[maybe_unused]] size_t s, [[maybe_unused]] size_t n) -> size_t {
            SDL_SetError ("Method write is not implemented");
            return 0;
          };
        }

        ret->close = [] (SDL_RWops* ctx) -> int {
          _dENFORCE_TYPE_RWOPS(-1);
          //  delete reinterpret_cast<RWImpl*>(ctx->hidden.unknown.data1);

          return 0;
        };
        return ret;
      }
  };
}

// ==========================================================================================
// Implementation
// ==========================================================================================
namespace neutrino::sdl {
  inline
  io::io (const void* mem, std::size_t size)
      : object<SDL_RWops> (SAFE_SDL_CALL(SDL_RWFromConstMem, mem, static_cast<int>(size)), true) {

  }

  // --------------------------------------------------------------------------------------
  inline
  io::io (void* mem, std::size_t size)
      : object<SDL_RWops> (SAFE_SDL_CALL(SDL_RWFromMem, mem, static_cast<int>(size)), true) {

  }

  // --------------------------------------------------------------------------------------
  inline
  io::io (const std::string& filename, bool read_only)
      : object<SDL_RWops> (SAFE_SDL_CALL(SDL_RWFromFile, filename.c_str (), read_only ? "rb" : "wb"), true) {

  }

  // --------------------------------------------------------------------------------------
  inline
  io::io (FILE* fp, bool auto_close)
      : object<SDL_RWops> (SAFE_SDL_CALL(SDL_RWFromFP, fp, static_cast<SDL_bool>(auto_close)), true) {

  }

  // --------------------------------------------------------------------------------------
  inline
  io::io (object <SDL_RWops>&& other)
      : object<SDL_RWops> (std::move (other)) {

  }

  // --------------------------------------------------------------------------------------
  inline
  io& io::operator = (object <SDL_RWops>&& other) {
    if (this != &other) {
      object<SDL_RWops>::operator = (std::move (other));
    }
    return *this;
  }

  // --------------------------------------------------------------------------------------
  inline
  size_t io::read (void* ptr, size_t size, size_t maxnum) {
    return SDL_RWread (handle (), ptr, size, maxnum);
  }

  // --------------------------------------------------------------------------------------
  inline
  size_t io::write (const void* ptr, size_t size, size_t maxnum) {
    return SDL_RWwrite (handle (), ptr, size, maxnum);
  }

  // --------------------------------------------------------------------------------------
  inline
  uint64_t io::seek (int64_t offset, whence w) {
    return static_cast<uint64_t>(SAFE_SDL_CALL(SDL_RWseek, handle (), offset, static_cast<int>(w)));
  }

  // --------------------------------------------------------------------------------------
  inline
  uint64_t io::tell () {
    return static_cast<uint64_t>(SAFE_SDL_CALL(SDL_RWtell, handle ()));
  }
}

#endif
