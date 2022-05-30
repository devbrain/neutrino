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
#include <functional>

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

  class abstract_fs {
    public:
      virtual ~abstract_fs();
      virtual std::shared_ptr<abstract_directory> root() = 0;
  };
}

#endif //INCLUDE_EXTRACTOR_ABSTRACT_FS_HH
