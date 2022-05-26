//
// Created by igor on 25/05/2022.
//

#include <fstream>
#include <map>
#include <cstring>

#include <neutrino/assets/fs/phys_fs.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {
  namespace {
    class phys_fs_file : public abstract_file {
      public:
        explicit phys_fs_file (std::filesystem::path path);

      private:
        [[nodiscard]] std::string name () const override;
        std::unique_ptr<std::istream> open () override;

        std::filesystem::path m_path;
    };

    phys_fs_file::phys_fs_file (std::filesystem::path path)
        : m_path (std::move (path)) {
    }

    std::string phys_fs_file::name () const {
      return m_path.filename ().u8string ();
    }

    std::unique_ptr<std::istream> phys_fs_file::open () {
      return std::make_unique<std::ifstream> (m_path, std::ios::binary);
    }

    struct ci_compare {
      bool operator () (const std::string& a, const std::string& b) const noexcept {
        return ::strcasecmp (a.c_str (), b.c_str ()) < 0;
      }
    };


    class phys_fs_dir : public abstract_directory {
      public:
        explicit phys_fs_dir (std::filesystem::path path);
      private:
        [[nodiscard]] std::string name () const override;
        [[nodiscard]] bool exists (const std::string& name) const override;
        [[nodiscard]] entry_t load (const std::string& name) override;
        void visit (std::function<void (const entry_t&)> visitor) override;
      private:
        using fs_map_t = std::map<std::string, std::filesystem::path, ci_compare>;
        std::filesystem::path m_path;
        fs_map_t m_fs_map;
    };

    phys_fs_dir::phys_fs_dir (std::filesystem::path path)
    : m_path(std::move (path))
    {
      ENFORCE(std::filesystem::is_directory (m_path));
      for (auto const& dir_entry : std::filesystem::directory_iterator{m_path})
      {
        auto p = dir_entry.path();
        m_fs_map.insert (std::make_pair(p.filename().string(), p));
      }
    }

    std::string phys_fs_dir::name () const {
      return m_path.filename().string();
    }

    bool phys_fs_dir::exists (const std::string& name) const {
      return m_fs_map.find (name) != m_fs_map.end();
    }

    phys_fs_dir::entry_t phys_fs_dir::load (const std::string& name) {
      auto itr = m_fs_map.find (name);
      if (itr == m_fs_map.end()) {
        return {};
      }
      if (std::filesystem::is_directory (itr->second)) {
        return std::make_shared<phys_fs_dir>(itr->second);
      }
      return std::make_shared<phys_fs_file>(itr->second);
    }

    void phys_fs_dir::visit (std::function<void (const entry_t&)> visitor) {
      for (auto & itr : m_fs_map) {
        visitor(load (itr.first));
      }
    }
  }

  struct phys_fs::impl {
    explicit impl(const std::filesystem::path& pth)
    : m_dir(std::make_shared<phys_fs_dir>(pth)) {

    }

    std::shared_ptr<phys_fs_dir> m_dir;
  };

  phys_fs::phys_fs(const std::filesystem::path& pth)
  : m_pimpl(spimpl::make_unique_impl<impl>(pth)) {

  }

  phys_fs::~phys_fs() = default;

  std::shared_ptr<abstract_directory> phys_fs::root() {
    return m_pimpl->m_dir;
  }
}