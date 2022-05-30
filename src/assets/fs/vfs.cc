//
// Created by igor on 29/05/2022.
//

#include <neutrino/assets/fs/vfs.hh>
#include <neutrino/utils/strings/string_tokenizer.hh>
#include <neutrino/utils/exception.hh>


namespace neutrino::assets {

  struct vfs::impl {
    explicit impl(std::unique_ptr<abstract_fs> sys)
    : m_sys(std::move (sys)) {

    }
    std::unique_ptr<abstract_fs> m_sys;
  };

  vfs::vfs(std::unique_ptr<abstract_fs> filesys)
  : m_pimpl(spimpl::make_unique_impl<impl>(std::move(filesys)))
  {

  }

  vfs::~vfs () = default;

  bool vfs::exists (const std::string& path) const {
    utils::string_tokenizer st(path, "/", utils::string_tokenizer::TOK_IGNORE_EMPTY);
    auto root = m_pimpl->m_sys->root();
    ENFORCE(root);
    for (const auto& e : st) {
      if (!root) {
        return false;
      }
      if (root->exists (e)) {
        auto entry = root->load (e);
        if (!entry) {
          RAISE_EX("Internal VFS error");
        }
        if (const auto* dir = std::get_if<std::shared_ptr<abstract_directory>>(&entry.value())) {
          root = *dir;
        } else {
          root = nullptr;
        }
      } else {
        return false;
      }
    }
    return true;
  }

  bool vfs::is_directory (const std::string& path) const {
    utils::string_tokenizer st(path, "/", utils::string_tokenizer::TOK_IGNORE_EMPTY);
    auto root = m_pimpl->m_sys->root();
    ENFORCE(root);
    bool is_dir = true;
    for (const auto& e : st) {
      if (!root) {
        return false;
      }
      if (root->exists (e)) {
        auto entry = root->load (e);
        if (!entry) {
          RAISE_EX("Internal VFS error");
        }
        if (const auto* dir = std::get_if<std::shared_ptr<abstract_directory>>(&entry.value())) {
          root = *dir;
          is_dir = true;
        } else {
          root = nullptr;
          is_dir = false;
        }
      } else {
        return false;
      }
    }
    return is_dir;
  }

  std::unique_ptr<std::istream> vfs::open (const std::string& path) {
    utils::string_tokenizer st(path, "/", utils::string_tokenizer::TOK_IGNORE_EMPTY);
    auto root = m_pimpl->m_sys->root();
    ENFORCE(root);
    std::shared_ptr<abstract_file> f;
    for (const auto& e : st) {
      if (!root) {
        return nullptr;
      }
      if (root->exists (e)) {
        auto entry = root->load (e);
        if (!entry) {
          RAISE_EX("Internal VFS error");
        }
        if (const auto* dir = std::get_if<std::shared_ptr<abstract_directory>>(&entry.value())) {
          root = *dir;
          f = nullptr;
        } else {
          root = nullptr;
          f = *std::get_if<std::shared_ptr<abstract_file>>(&entry.value());
        }
      } else {
        return nullptr;
      }
    }
    if (f) {
      return f->open();
    }
    return nullptr;
  }
}
