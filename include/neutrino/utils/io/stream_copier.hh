//
// Created by igor on 08/07/2021.
//

#ifndef NEUTRINO_UTILS_IO_STREAM_COPIER_HH
#define NEUTRINO_UTILS_IO_STREAM_COPIER_HH

#include <istream>
#include <ostream>
#include <cstddef>

namespace neutrino::utils::io {
    class StreamCopier
            /// This class provides static methods to copy the contents from one stream
            /// into another.
            {
                    public:
                    static std::streamsize copyStream(std::istream& istr, std::ostream& ostr, std::size_t bufferSize = 8192);
                    /// Writes all bytes readable from istr to ostr, using an internal buffer.
                    ///
                    /// Returns the number of bytes copied.


                    static uint64_t copyStream64(std::istream& istr, std::ostream& ostr, std::size_t bufferSize = 8192);
		/// Writes all bytes readable from istr to ostr, using an internal buffer.
		///
		/// Returns the number of bytes copied as a 64-bit unsigned integer.
		///
		/// Note: the only difference to copyStream() is that a 64-bit unsigned
		/// integer is used to count the number of bytes copied.


                    static std::streamsize copyStreamUnbuffered(std::istream& istr, std::ostream& ostr);
                    /// Writes all bytes readable from istr to ostr.
                    ///
                    /// Returns the number of bytes copied.


                    static uint64_t copyStreamUnbuffered64(std::istream& istr, std::ostream& ostr);
		/// Writes all bytes readable from istr to ostr.
		///
		/// Returns the number of bytes copied as a 64-bit unsigned integer.
		///
		/// Note: the only difference to copyStreamUnbuffered() is that a 64-bit unsigned
		/// integer is used to count the number of bytes copied.


                    static std::streamsize copyToString(std::istream& istr, std::string& str, std::size_t bufferSize = 8192);
                    /// Appends all bytes readable from istr to the given string, using an internal buffer.
                    ///
                    /// Returns the number of bytes copied.


                    static uint64_t copyToString64(std::istream& istr, std::string& str, std::size_t bufferSize = 8192);
		/// Appends all bytes readable from istr to the given string, using an internal buffer.
		///
		/// Returns the number of bytes copied as a 64-bit unsigned integer.
		///
		/// Note: the only difference to copyToString() is that a 64-bit unsigned
		/// integer is used to count the number of bytes copied.

            };

}

#endif
