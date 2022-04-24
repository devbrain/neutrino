//
// Created by igor on 24/04/2022.
//

#include "zstd_stream.hh"
#include <neutrino/utils/exception.hh>

namespace neutrino::utils::io {
#define THROW RAISE_EX("zstd error: ",  ZSTD_getErrorName(this->ret))

  zstd_stream::zstd_stream (bool is_input, int level)
      : isInput (is_input) {
    if (this->isInput) {
      this->dctx = ZSTD_createDCtx ();
      if (!this->dctx) {
        RAISE_EX("ZSTD_createDCtx() failed!");
      }
    }
    else {
      this->cctx = ZSTD_createCCtx ();
      if (!this->cctx) {
        RAISE_EX("ZSTD_createCCtx() failed!");
      }
      this->ret = ZSTD_CCtx_setParameter (cctx, ZSTD_c_compressionLevel, level);
    }
    if (ZSTD_isError (this->ret)) {
      THROW;
    }
  }

  zstd_stream::~zstd_stream () {
    if (this->isInput) {
      ZSTD_freeDCtx (this->dctx);
    }
    else {
      ZSTD_freeCCtx (this->cctx);
    }
  }

  int zstd_stream::decompress (const int flags) {
    this->update_inbuffer ();
    this->update_outbuffer ();

    this->ret = ZSTD_decompressStream (this->dctx, &output, &input);
    if (ZSTD_isError (this->ret)) {
      THROW;
    }

    this->update_stream_state ();

    return (int) ret;
  }

  int zstd_stream::compress (const int endStream) {
    this->update_inbuffer ();
    this->update_outbuffer ();

    if (endStream) {
      this->ret = ZSTD_endStream (this->cctx, &output);
      if (ZSTD_isError (this->ret)) {
        THROW;
      }
    }
    else {
      this->ret = ZSTD_compressStream2 (this->cctx, &output, &input, ZSTD_e_continue);
      if (ZSTD_isError (this->ret)) {
        THROW;
      }

      this->ret = (input.pos == input.size);
    }

    this->update_stream_state ();

    return (int) ret;
  }

  bool zstd_stream::stream_end () const {
    return this->ret == 0;
  }

  bool zstd_stream::done () const {
    return this->stream_end ();
  }

  const uint8_t* zstd_stream::next_in () const {
    return static_cast<unsigned char*>(this->buffIn);
  }

  long zstd_stream::avail_in () const {
    return this->buffInSize;
  }

  uint8_t* zstd_stream::next_out () const {
    return static_cast<unsigned char*>(this->buffOut);
  }

  long zstd_stream::avail_out () const {
    return this->buffOutSize;
  }

  void zstd_stream::set_next_in (const unsigned char* in) {
    this->buffIn = (void*) in;
  }

  void zstd_stream::set_avail_in (const long in) {
    this->buffInSize = (size_t) in;
  }

  void zstd_stream::set_next_out (const uint8_t* in) {
    this->buffOut = (void*) in;
  }

  void zstd_stream::set_avail_out (const long in) {
    this->buffOutSize = (size_t) in;
  }

  void zstd_stream::update_inbuffer () {
    this->input = {this->buffIn, this->buffInSize, 0};
  }

  void zstd_stream::update_outbuffer () {
    this->output = {this->buffOut, this->buffOutSize, 0};
  }

  void zstd_stream::update_stream_state () {
    this->set_next_out (this->next_out () + this->output.pos);
    this->set_avail_out (this->avail_out () - this->output.pos);
    this->set_next_in (this->next_in () + this->input.pos);
    this->set_avail_in (this->avail_in () - this->input.pos);
  }
}