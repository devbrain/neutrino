//
// Created by igor on 29/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_FS_VFS_HH
#define INCLUDE_NEUTRINO_ASSETS_FS_VFS_HH

#include <string>
#include <neutrino/assets/fs/abstract_fs.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::assets {
  class vfs {
    public:
      explicit vfs(std::unique_ptr<abstract_fs> filesys);
      ~vfs();

      [[nodiscard]] bool exists(const std::string& path) const;
      [[nodiscard]] bool is_directory(const std::string& path) const;
      std::unique_ptr<std::istream> open(const std::string& path);

    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_FS_VFS_HH
