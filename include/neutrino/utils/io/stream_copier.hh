//
// Created by igor on 08/07/2021.
//

#ifndef NEUTRINO_UTILS_IO_STREAM_COPIER_HH
#define NEUTRINO_UTILS_IO_STREAM_COPIER_HH

#include <istream>
#include <ostream>
#include <cstddef>

namespace neutrino::utils::io {
  /// This class provides static methods to copy the contents from one stream
  /// into another.
  struct stream_copier {
    /// Writes all bytes readable from istr to ostr, using an internal buffer.
    ///
    /// Returns the number of bytes copied.
    static std::streamsize copy_stream (std::istream &istr, std::ostream &ostr, std::size_t bufferSize = 8192);

    /// Writes all bytes readable from istr to ostr.
    ///
    /// Returns the number of bytes copied.
    static std::streamsize copy_stream_unbuffered (std::istream &istr, std::ostream &ostr);

    /// Appends all bytes readable from istr to the given string, using an internal buffer.
    ///
    /// Returns the number of bytes copied.
    static std::streamsize copy_to_string (std::istream &istr, std::string &str, std::size_t bufferSize = 8192);
  };

}

#endif
