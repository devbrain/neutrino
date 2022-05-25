//
// Created by igor on 25/05/2022.
//

#ifndef INCLUDE_EXTRACTOR_EXPLODE_EXE_FILE_HH
#define INCLUDE_EXTRACTOR_EXPLODE_EXE_FILE_HH

#include <vector>

namespace neutrino::ext {
  bool explode_exe_file(const char* input, std::size_t input_size, std::vector<char>& output);
}

#endif //INCLUDE_EXTRACTOR_EXPLODE_EXE_FILE_HH
