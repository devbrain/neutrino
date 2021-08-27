//
// Created by igor on 21/02/2021.
//

#ifndef NEUTRINO_UTILS_WHEREAMI_HH
#define NEUTRINO_UTILS_WHEREAMI_HH

#include <filesystem>

namespace neutrino::utils {
  std::filesystem::path get_executable_path ();
  std::filesystem::path get_module_path ();
}

#endif //NEUTRINO_UTILS_WHEREAMI_HH
