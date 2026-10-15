//
// Created by igor on 11/10/2026.
//

#pragma once

#include <iosfwd>
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <string_view>
#include <filesystem>

#include <luxor/resources/storage/archive.hh>

namespace luxor::rc {
    class vfs {
        public:
            vfs();
            ~vfs();

            vfs(const vfs&) = delete;
            vfs& operator=(const vfs&) = delete;
            vfs(vfs&&) noexcept;
            vfs& operator=(vfs&&) noexcept;

            // Mounting
            // Automatically detects directory or supported archive format from physical path
            void mount(std::string_view mount_point, const std::filesystem::path& physical_path);

            // Mounts an existing archive instance (takes ownership or shares ownership)
            void mount(std::string_view mount_point, std::unique_ptr<archive> ar);
            void mount(std::string_view mount_point, std::shared_ptr<archive> ar);

            // Unmount & Management
            bool unmount(std::string_view mount_point);
            void clear();
            [[nodiscard]] bool empty() const;
            [[nodiscard]] size_t mount_count() const noexcept;

            // Locale / Translation Layer
            void set_locale(std::string_view locale);
            [[nodiscard]] std::string_view locale() const noexcept;

            // Existence & Metadata
            [[nodiscard]] bool exists(std::string_view path) const;
            [[nodiscard]] std::optional<file_info> stat(std::string_view path) const;
            [[nodiscard]] std::vector<file_info> list_files() const;

            // Content Access
            [[nodiscard]] std::vector<uint8_t> read(std::string_view path) const;
            [[nodiscard]] std::unique_ptr<std::istream> open(std::string_view path) const;

            // Dumping
            void dump(const std::filesystem::path& root_dir) const;

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };

    void dump(const vfs& fs, const std::filesystem::path& root_dir);
}
