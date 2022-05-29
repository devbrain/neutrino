//
// Created by igor on 27/05/2022.
//

#include <neutrino/assets/fonts/load_fon.hh>
#include <iostream>
#include "assets/pe/exefile.hpp"
#include <neutrino/utils/strings/wchar.hh>

namespace neutrino::assets {
  void load_fon(const std::filesystem::path& pth) {
    pefile::exe_file_c exe(pth.string());
    const auto& rd = exe.resource_directory();
    for (auto ni = rd.names_begin(); ni != rd.names_end(); ++ni) {
      std::cout << ni->id() << " : " << utils::wstring_to_utf8(ni->name()) << std::endl;
    }
  }
}