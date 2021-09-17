//
// Created by igor on 25/07/2021.
//

#ifndef NEUTRINO_UTILS_NUMBER_PARSER_HH
#define NEUTRINO_UTILS_NUMBER_PARSER_HH

#include <string>

namespace neutrino::utils {
  class number_parser
      /// The NumberParser class provides static methods
      /// for parsing numbers out of strings.
      ///
      /// Note that leading or trailing whitespace is not allowed
      /// in the string. Poco::trim() or Poco::trimInPlace()
      /// can be used to remove leading or trailing whitespace.
  {
    public:
      static constexpr unsigned short NUM_BASE_OCT = 010;
      static constexpr unsigned short NUM_BASE_DEC = 10;
      static constexpr unsigned short NUM_BASE_HEX = 0x10;

      static int parse (const std::string& s, char thousandSeparator = ',');
      /// Parses an integer value in decimal notation from the given string.
      /// Throws a SyntaxException if the string does not hold a number in decimal notation.

      static bool try_parse (const std::string& s, int& value, char thousandSeparator = ',');
      /// Parses an integer value in decimal notation from the given string.
      /// Returns true if a valid integer has been found, false otherwise.
      /// If parsing was not successful, value is undefined.

      static unsigned parse_unsigned (const std::string& s, char thousandSeparator = ',');
      /// Parses an unsigned integer value in decimal notation from the given string.
      /// Throws a SyntaxException if the string does not hold a number in decimal notation.

      static bool try_parse_unsigned (const std::string& s, unsigned& value, char thousandSeparator = ',');
      /// Parses an unsigned integer value in decimal notation from the given string.
      /// Returns true if a valid integer has been found, false otherwise.
      /// If parsing was not successful, value is undefined.

      static unsigned parse_hex (const std::string& s);
      /// Parses an integer value in hexadecimal notation from the given string.
      /// Throws a SyntaxException if the string does not hold a number in
      /// hexadecimal notation.

      static bool try_parse_hex (const std::string& s, unsigned& value);
      /// Parses an unsigned integer value in hexadecimal notation from the given string.
      /// Returns true if a valid integer has been found, false otherwise.
      /// If parsing was not successful, value is undefined.

      static unsigned parseOct (const std::string& s);
      /// Parses an integer value in octal notation from the given string.
      /// Throws a SyntaxException if the string does not hold a number in
      /// hexadecimal notation.

      static bool try_parse_oct (const std::string& s, unsigned& value);
      /// Parses an unsigned integer value in octal notation from the given string.
      /// Returns true if a valid integer has been found, false otherwise.
      /// If parsing was not successful, value is undefined.



      static int64_t parse64 (const std::string& s, char thousandSeparator = ',');
      /// Parses a 64-bit integer value in decimal notation from the given string.
      /// Throws a SyntaxException if the string does not hold a number in decimal notation.

      static bool try_parse_64 (const std::string& s, int64_t& value, char thousandSeparator = ',');
      /// Parses a 64-bit integer value in decimal notation from the given string.
      /// Returns true if a valid integer has been found, false otherwise.
      /// If parsing was not successful, value is undefined.

      static uint64_t parse_unsigned_64 (const std::string& s, char thousandSeparator = ',');
      /// Parses an unsigned 64-bit integer value in decimal notation from the given string.
      /// Throws a SyntaxException if the string does not hold a number in decimal notation.

      static bool try_parse_unsigned_64 (const std::string& s, uint64_t& value, char thousandSeparator = ',');
      /// Parses an unsigned 64-bit integer value in decimal notation from the given string.
      /// Returns true if a valid integer has been found, false otherwise.
      /// If parsing was not successful, value is undefined.

      static uint64_t parse_hex_64 (const std::string& s);
      /// Parses a 64 bit-integer value in hexadecimal notation from the given string.
      /// Throws a SyntaxException if the string does not hold a number in hexadecimal notation.

      static bool try_parse_hex_64 (const std::string& s, uint64_t& value);
      /// Parses an unsigned 64-bit integer value in hexadecimal notation from the given string.
      /// Returns true if a valid integer has been found, false otherwise.
      /// If parsing was not successful, value is undefined.

      static uint64_t parse_oct_64 (const std::string& s);
      /// Parses a 64 bit-integer value in octal notation from the given string.
      /// Throws a SyntaxException if the string does not hold a number in hexadecimal notation.

      static bool try_parse_oct_64 (const std::string& s, uint64_t& value);
      /// Parses an unsigned 64-bit integer value in octal notation from the given string.
      /// Returns true if a valid integer has been found, false otherwise.
      /// If parsing was not successful, value is undefined.


      static double parse_float (const std::string& s, char decimalSeparator = '.', char thousandSeparator = ',');
      /// Parses a double value in decimal floating point notation
      /// from the given string.
      /// Throws a SyntaxException if the string does not hold a floating-point
      /// number in decimal notation.

      static bool
      try_parse_float (const std::string& s, double& value, char decimalSeparator = '.', char thousandSeparator = ',');
      /// Parses a double value in decimal floating point notation
      /// from the given string.
      /// Returns true if a valid floating point number has been found,
      /// false otherwise.
      /// If parsing was not successful, value is undefined.

      static bool parse_bool (const std::string& s);
      /// Parses a bool value in decimal or string notation
      /// from the given string.
      /// Valid forms are: "0", "1", "true", "on", false", "yes", "no", "off".
      /// String forms are NOT case sensitive.
      /// Throws a SyntaxException if the string does not hold a valid bool number

      static bool try_parse_bool (const std::string& s, bool& value);
      /// Parses a bool value in decimal or string notation
      /// from the given string.
      /// Valid forms are: "0", "1", "true", "on", false", "yes", "no", "off".
      /// String forms are NOT case sensitive.
      /// Returns true if a valid bool number has been found,
      /// false otherwise.
      /// If parsing was not successful, value is undefined.
  };

}

#endif
