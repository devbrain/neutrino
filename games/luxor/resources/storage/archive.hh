//
// Created by igor on 07/10/2026.
//

#pragma once

#include <iosfwd>
#include <memory>
#include <vector>
#include <optional>
#include <filesystem>

namespace luxor::rc {
    struct file_info {
        std::string path;
        uint64_t size{0}; // Uncompressed size in bytes
        bool is_directory{false};
    };

    class archive {
        public:
            ~archive();

            // Format Probing
            [[nodiscard]] static bool probe(std::istream& is);
            [[nodiscard]] static bool probe(const std::filesystem::path& path);

            // Loading
            static std::unique_ptr <archive> load(std::istream& is);
            static std::unique_ptr <archive> load(const std::filesystem::path& path);

            // Existence & Metadata
            [[nodiscard]] bool has_file(std::string_view path) const;
            [[nodiscard]] std::optional<file_info> stat(std::string_view path) const;
            [[nodiscard]] std::vector<file_info> list_files() const;

            // Content Extraction
            [[nodiscard]] std::vector<uint8_t> read(std::string_view path) const;
            void dump(const std::filesystem::path& root_dir) const;
        private:
            archive(std::istream& is, uint64_t file_size);
            archive(std::unique_ptr<std::istream> owned_stream, uint64_t file_size);
        private:
            struct impl;

            std::unique_ptr<impl> m_pimpl;
    };

    [[nodiscard]] bool probe(std::istream& is);
    [[nodiscard]] bool probe(const std::filesystem::path& path);

    void dump(const archive& ar, const std::filesystem::path& root_dir);
    void dump(const archive* ar, const std::filesystem::path& root_dir);
    void dump(const std::unique_ptr<archive>& ar, const std::filesystem::path& root_dir);
    void extract(const archive& ar, const std::filesystem::path& root_dir);
}
