//
// Created by igor on 25/05/2022.
//

#ifndef INCLUDE_EXTRACTOR_PHYS_FS_HH
#define INCLUDE_EXTRACTOR_PHYS_FS_HH

#include <neutrino/assets/fs/abstract_fs.hh>
#include <neutrino/utils/spimpl.h>
#include <filesystem>

namespace neutrino::assets {
  class phys_fs : public abstract_fs {
    public:
      explicit phys_fs(const std::filesystem::path& pth);

      ~phys_fs() override;
      std::shared_ptr<abstract_directory> root();
    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;
  };
}

#endif //INCLUDE_EXTRACTOR_PHYS_FS_HH
