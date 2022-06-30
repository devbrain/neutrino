//
// Created by igor on 25/05/2022.
//

#ifndef INCLUDE_EXTRACTOR_ABSTRACT_FS_HH
#define INCLUDE_EXTRACTOR_ABSTRACT_FS_HH

#include <istream>
#include <string>
#include <memory>
#include <variant>
#include <optional>
#include <type_traits>
#include <functional>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {

  class abstract_fs_entry {
    public:
      virtual ~abstract_fs_entry();
      [[nodiscard]] virtual std::string name() const = 0;
  };

  class abstract_file : public abstract_fs_entry{
    public:
      [[nodiscard]] virtual std::unique_ptr<std::istream> open() = 0;
  };

  class abstract_directory : public abstract_fs_entry {
    public:
      using entry_t = std::optional<std::variant<std::shared_ptr<abstract_directory>,
          std::shared_ptr<abstract_file>>>;
    public:
      [[nodiscard]] virtual bool exists(const std::string& name) const = 0;
      [[nodiscard]] virtual entry_t load(const std::string& name) = 0;
      virtual void visit(std::function<void(const entry_t&)> visitor) = 0;
  };

  inline
  bool exists(const abstract_directory::entry_t& e) {
    return e.has_value();
  }

  inline
  bool is_dir(const abstract_directory::entry_t& e) {
    return exists (e) && std::get_if<0>(&e.value()) != nullptr;
  }

  inline
  bool is_file(const abstract_directory::entry_t& e) {
    return exists (e) && std::get_if<1>(&e.value()) != nullptr;
  }

  template <typename T>
  inline
  const T& fs_cast(const abstract_directory::entry_t& e) {

    static_assert (std::is_same_v<T,abstract_directory> || std::is_same_v<T, abstract_file>);

    if (!e.has_value()) {
      RAISE_EX("FS entry does not exists");
    }
    if constexpr (std::is_same_v<T,abstract_directory>) {
      if (const auto* v =  std::get_if<0>(&e.value())) {
        if (v->get()) {
          return *v->get();
        }
        RAISE_EX("FS Directory entry is corrupted");
      }
    } else {
      if constexpr (std::is_same_v<T, abstract_file>) {
        if (const auto* v = std::get_if<1> (&e.value ())) {
          if (v->get ()) {
            return *v->get ();
          }
          RAISE_EX("FS File entry is corrupted");
        }
      }
    }
    RAISE_EX("Should not be here");
  }

  template <typename T>
  inline
  T& fs_cast(abstract_directory::entry_t& e) {

    static_assert (std::is_same_v<T,abstract_directory> || std::is_same_v<T, abstract_file>);

    if (!e.has_value()) {
      RAISE_EX("FS entry does not exists");
    }
    if constexpr (std::is_same_v<T,abstract_directory>) {
      if (auto* v =  std::get_if<0>(&e.value())) {
        if (v->get()) {
          return *v->get();
        }
        RAISE_EX("FS Directory entry is corrupted");
      }
    } else {
      if constexpr (std::is_same_v<T, abstract_file>) {
        if (auto* v = std::get_if<1> (&e.value ())) {
          if (v->get ()) {
            return *v->get ();
          }
          RAISE_EX("FS File entry is corrupted");
        }
      }
    }
    RAISE_EX("Should not be here");
  }

  class abstract_fs {
    public:
      virtual ~abstract_fs();
      virtual std::shared_ptr<abstract_directory> root() = 0;
  };
}

#endif //INCLUDE_EXTRACTOR_ABSTRACT_FS_HH
