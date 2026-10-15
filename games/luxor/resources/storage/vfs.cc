//
// Created by igor on 11/10/2026.
//

#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <stdexcept>

#include "luxor/resources/storage/vfs.hh"

namespace luxor::rc {
    namespace {
        std::string to_lower(std::string_view str) {
            std::string res;
            res.reserve(str.size());
            for (char c : str) {
                res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
            return res;
        }

        std::string normalize_path(std::string_view raw_path) {
            std::string clean;
            clean.reserve(raw_path.size());

            for (char c : raw_path) {
                if (c == '\\') {
                    clean.push_back('/');
                } else {
                    clean.push_back(c);
                }
            }

            while (!clean.empty() && (clean.back() == '\0' ||
                                      std::isspace(static_cast<unsigned char>(clean.back())))) {
                clean.pop_back();
            }

            size_t start = 0;
            while (start < clean.size() && (clean[start] == '/' ||
                                            std::isspace(static_cast<unsigned char>(clean[start])))) {
                ++start;
            }
            if (start > 0) {
                clean = clean.substr(start);
            }

            std::vector<std::string> parts;
            std::istringstream ss(clean);
            std::string token;
            while (std::getline(ss, token, '/')) {
                if (token.empty() || token == ".") {
                    continue;
                }
                if (token == "..") {
                    if (!parts.empty()) {
                        parts.pop_back();
                    }
                } else {
                    parts.push_back(token);
                }
            }

            std::string result;
            for (size_t i = 0; i < parts.size(); ++i) {
                if (i > 0) result += '/';
                result += parts[i];
            }

            return to_lower(result);
        }

        std::string normalize_mount_point(std::string_view raw_mount) {
            return normalize_path(raw_mount);
        }

        // ====================================================================
        // Mount Source Abstraction
        // ====================================================================

        class mount_source {
        public:
            virtual ~mount_source() = default;
            [[nodiscard]] virtual bool has_file(std::string_view rel_path) const = 0;
            [[nodiscard]] virtual std::optional<file_info> stat(std::string_view rel_path) const = 0;
            [[nodiscard]] virtual std::vector<file_info> list_files() const = 0;
            [[nodiscard]] virtual std::vector<uint8_t> read(std::string_view rel_path) const = 0;
            [[nodiscard]] virtual std::unique_ptr<std::istream> open(std::string_view rel_path) const = 0;
        };

        // ====================================================================
        // Archive Mount Source
        // ====================================================================

        class archive_mount_source : public mount_source {
        public:
            explicit archive_mount_source(std::shared_ptr<archive> ar)
                : m_archive(std::move(ar)) {
                if (!m_archive) {
                    throw std::runtime_error("Cannot mount null archive");
                }
            }

            bool has_file(std::string_view rel_path) const override {
                return m_archive->has_file(rel_path);
            }

            std::optional<file_info> stat(std::string_view rel_path) const override {
                return m_archive->stat(rel_path);
            }

            std::vector<file_info> list_files() const override {
                return m_archive->list_files();
            }

            std::vector<uint8_t> read(std::string_view rel_path) const override {
                return m_archive->read(rel_path);
            }

            std::unique_ptr<std::istream> open(std::string_view rel_path) const override {
                if (!m_archive->has_file(rel_path)) {
                    return nullptr;
                }
                auto data = m_archive->read(rel_path);
                return std::make_unique<std::stringstream>(
                    std::string(reinterpret_cast<const char*>(data.data()), data.size()),
                    std::ios::in | std::ios::binary
                );
            }

        private:
            std::shared_ptr<archive> m_archive;
        };

        // ====================================================================
        // Directory Mount Source (Physical directory with case-insensitive index)
        // ====================================================================

        class directory_mount_source : public mount_source {
        public:
            struct entry {
                std::filesystem::path physical_path;
                uint64_t size{0};
                bool is_directory{false};
            };

            explicit directory_mount_source(const std::filesystem::path& dir_path)
                : m_root_path(std::filesystem::canonical(dir_path)) {
                if (!std::filesystem::is_directory(m_root_path)) {
                    throw std::runtime_error("Path is not a directory: " + dir_path.string());
                }
                scan();
            }

            void scan() {
                m_entries.clear();
                std::error_code ec;
                for (const auto& dir_entry : std::filesystem::recursive_directory_iterator(
                         m_root_path,
                         std::filesystem::directory_options::follow_directory_symlink |
                         std::filesystem::directory_options::skip_permission_denied,
                         ec)) {
                    if (ec) continue;
                    auto rel = std::filesystem::relative(dir_entry.path(), m_root_path, ec);
                    if (ec) continue;

                    std::string norm_rel = normalize_path(rel.generic_string());
                    if (norm_rel.empty()) continue;

                    bool is_dir = dir_entry.is_directory(ec);
                    uint64_t sz = 0;
                    if (!is_dir && dir_entry.is_regular_file(ec)) {
                        sz = dir_entry.file_size(ec);
                        if (ec) sz = 0;
                    }

                    m_entries[norm_rel] = entry{
                        dir_entry.path(),
                        sz,
                        is_dir
                    };
                }
            }

            const entry* find_entry(std::string_view rel_path) const {
                std::string norm = normalize_path(rel_path);
                auto it = m_entries.find(norm);
                if (it != m_entries.end()) {
                    return &it->second;
                }

                auto resolved = resolve_case_insensitive(norm);
                if (resolved.has_value()) {
                    auto [inserted_it, _] = m_entries.emplace(norm, std::move(*resolved));
                    return &inserted_it->second;
                }
                return nullptr;
            }

            std::optional<entry> resolve_case_insensitive(std::string_view norm_path) const {
                if (norm_path.empty()) {
                    return entry{m_root_path, 0, true};
                }
                std::filesystem::path current = m_root_path;
                std::istringstream ss{std::string(norm_path)};
                std::string part;
                while (std::getline(ss, part, '/')) {
                    if (part.empty()) continue;
                    bool found_part = false;
                    std::error_code ec;
                    for (const auto& dir_ent : std::filesystem::directory_iterator(current, ec)) {
                        if (ec) break;
                        std::string ent_name = dir_ent.path().filename().string();
                        std::string ent_norm = to_lower(ent_name);
                        if (ent_norm == part) {
                            current = dir_ent.path();
                            found_part = true;
                            break;
                        }
                    }
                    if (!found_part) {
                        return std::nullopt;
                    }
                }

                std::error_code ec;
                bool is_dir = std::filesystem::is_directory(current, ec);
                uint64_t sz = 0;
                if (!is_dir && std::filesystem::is_regular_file(current, ec)) {
                    sz = std::filesystem::file_size(current, ec);
                    if (ec) sz = 0;
                }
                return entry{current, sz, is_dir};
            }

            bool has_file(std::string_view rel_path) const override {
                const auto* e = find_entry(rel_path);
                return e != nullptr && !e->is_directory;
            }

            std::optional<file_info> stat(std::string_view rel_path) const override {
                std::string norm = normalize_path(rel_path);
                if (norm.empty()) {
                    return file_info{"", 0, true};
                }
                const auto* e = find_entry(norm);
                if (!e) return std::nullopt;
                return file_info{norm, e->size, e->is_directory};
            }

            std::vector<file_info> list_files() const override {
                std::vector<file_info> res;
                res.reserve(m_entries.size());
                for (const auto& [name, e] : m_entries) {
                    res.push_back(file_info{name, e.size, e.is_directory});
                }
                return res;
            }

            std::vector<uint8_t> read(std::string_view rel_path) const override {
                const auto* e = find_entry(rel_path);
                if (!e || e->is_directory) {
                    return {};
                }
                std::ifstream ifs(e->physical_path, std::ios::binary);
                if (!ifs.is_open()) {
                    throw std::runtime_error("Failed to open file for reading: " + e->physical_path.string());
                }
                ifs.seekg(0, std::ios::end);
                auto sz = static_cast<size_t>(ifs.tellg());
                ifs.seekg(0, std::ios::beg);
                if (sz == 0) {
                    return {};
                }
                std::vector<uint8_t> data(sz);
                if (!ifs.read(reinterpret_cast<char*>(data.data()), sz)) {
                    throw std::runtime_error("Failed to read file: " + e->physical_path.string());
                }
                return data;
            }

            std::unique_ptr<std::istream> open(std::string_view rel_path) const override {
                const auto* e = find_entry(rel_path);
                if (!e || e->is_directory) {
                    return nullptr;
                }
                auto ifs = std::make_unique<std::ifstream>(e->physical_path, std::ios::binary);
                if (!ifs->is_open()) {
                    return nullptr;
                }
                return ifs;
            }

        private:
            std::filesystem::path m_root_path;
            mutable std::unordered_map<std::string, entry> m_entries;
        };
    }

    // ====================================================================
    // VFS Implementation
    // ====================================================================

    struct vfs::impl {
        struct mount_record {
            std::string mount_point;
            std::shared_ptr<mount_source> source;
        };

        std::string locale;
        std::vector<mount_record> mounts;

        bool has_file_direct(std::string_view vpath) const {
            if (vpath.empty()) return false;
            for (auto it = mounts.rbegin(); it != mounts.rend(); ++it) {
                if (it->mount_point.empty()) {
                    if (it->source->has_file(vpath)) return true;
                } else {
                    if (vpath.size() > it->mount_point.size() &&
                        vpath.starts_with(it->mount_point) &&
                        vpath[it->mount_point.size()] == '/') {
                        std::string_view rel = vpath.substr(it->mount_point.size() + 1);
                        if (it->source->has_file(rel)) return true;
                    }
                }
            }
            return false;
        }

        std::optional<file_info> stat_direct(std::string_view vpath) const {
            if (vpath.empty()) {
                if (mounts.empty()) return std::nullopt;
                return file_info{"", 0, true};
            }

            // 1. Direct file or explicit directory check in mounts (LIFO)
            for (auto it = mounts.rbegin(); it != mounts.rend(); ++it) {
                if (it->mount_point.empty()) {
                    auto st = it->source->stat(vpath);
                    if (st.has_value()) {
                        st->path = std::string(vpath);
                        return st;
                    }
                } else {
                    if (vpath == it->mount_point) {
                        return file_info{std::string(vpath), 0, true};
                    }
                    if (vpath.size() > it->mount_point.size() &&
                        vpath.starts_with(it->mount_point) &&
                        vpath[it->mount_point.size()] == '/') {
                        std::string_view rel = vpath.substr(it->mount_point.size() + 1);
                        auto st = it->source->stat(rel);
                        if (st.has_value()) {
                            st->path = std::string(vpath);
                            return st;
                        }
                    }
                }
            }

            // 2. Implicit directory check: is vpath a directory containing files or mounts?
            std::string dir_prefix = std::string(vpath) + "/";
            for (const auto& m : mounts) {
                if (m.mount_point.starts_with(dir_prefix)) {
                    return file_info{std::string(vpath), 0, true};
                }
                if (m.mount_point.empty()) {
                    auto files = m.source->list_files();
                    for (const auto& f : files) {
                        if (f.path.starts_with(dir_prefix)) {
                            return file_info{std::string(vpath), 0, true};
                        }
                    }
                } else if (dir_prefix.starts_with(m.mount_point + "/")) {
                    std::string sub_prefix = dir_prefix.substr(m.mount_point.size() + 1);
                    auto files = m.source->list_files();
                    for (const auto& f : files) {
                        if (f.path.starts_with(sub_prefix)) {
                            return file_info{std::string(vpath), 0, true};
                        }
                    }
                }
            }

            return std::nullopt;
        }

        std::vector<uint8_t> read_direct(std::string_view vpath) const {
            if (vpath.empty()) return {};
            for (auto it = mounts.rbegin(); it != mounts.rend(); ++it) {
                if (it->mount_point.empty()) {
                    if (it->source->has_file(vpath)) {
                        return it->source->read(vpath);
                    }
                } else {
                    if (vpath.size() > it->mount_point.size() &&
                        vpath.starts_with(it->mount_point) &&
                        vpath[it->mount_point.size()] == '/') {
                        std::string_view rel = vpath.substr(it->mount_point.size() + 1);
                        if (it->source->has_file(rel)) {
                            return it->source->read(rel);
                        }
                    }
                }
            }
            return {};
        }

        std::unique_ptr<std::istream> open_direct(std::string_view vpath) const {
            if (vpath.empty()) return nullptr;
            for (auto it = mounts.rbegin(); it != mounts.rend(); ++it) {
                if (it->mount_point.empty()) {
                    if (it->source->has_file(vpath)) {
                        return it->source->open(vpath);
                    }
                } else {
                    if (vpath.size() > it->mount_point.size() &&
                        vpath.starts_with(it->mount_point) &&
                        vpath[it->mount_point.size()] == '/') {
                        std::string_view rel = vpath.substr(it->mount_point.size() + 1);
                        if (it->source->has_file(rel)) {
                            return it->source->open(rel);
                        }
                    }
                }
            }
            return nullptr;
        }
    };

    vfs::vfs() : m_pimpl(std::make_unique<impl>()) {}
    vfs::~vfs() = default;
    vfs::vfs(vfs&&) noexcept = default;
    vfs& vfs::operator=(vfs&&) noexcept = default;

    void vfs::mount(std::string_view mount_point, const std::filesystem::path& physical_path) {
        std::error_code ec;
        if (!std::filesystem::exists(physical_path, ec)) {
            throw std::runtime_error("Cannot mount non-existent path: " + physical_path.string());
        }

        const std::string norm_mp = normalize_mount_point(mount_point);

        if (std::filesystem::is_directory(physical_path, ec)) {
            auto src = std::make_shared<directory_mount_source>(physical_path);
            m_pimpl->mounts.push_back({norm_mp, std::move(src)});
        } else if (archive::probe(physical_path)) {
            auto ar = archive::load(physical_path);
            auto src = std::make_shared<archive_mount_source>(std::move(ar));
            m_pimpl->mounts.push_back({norm_mp, std::move(src)});
        } else {
            throw std::runtime_error("Cannot mount " + physical_path.string() + ": unsupported archive format or invalid file");
        }
    }

    void vfs::mount(std::string_view mount_point, std::unique_ptr<archive> ar) {
        if (!ar) {
            throw std::runtime_error("Cannot mount null archive");
        }
        const std::string norm_mp = normalize_mount_point(mount_point);
        auto src = std::make_shared<archive_mount_source>(std::shared_ptr<archive>(std::move(ar)));
        m_pimpl->mounts.push_back({norm_mp, std::move(src)});
    }

    void vfs::mount(std::string_view mount_point, std::shared_ptr<archive> ar) {
        if (!ar) {
            throw std::runtime_error("Cannot mount null archive");
        }
        const std::string norm_mp = normalize_mount_point(mount_point);
        auto src = std::make_shared<archive_mount_source>(std::move(ar));
        m_pimpl->mounts.push_back({norm_mp, std::move(src)});
    }

    bool vfs::unmount(std::string_view mount_point) {
        const std::string norm_mp = normalize_mount_point(mount_point);
        for (auto it = m_pimpl->mounts.rbegin(); it != m_pimpl->mounts.rend(); ++it) {
            if (it->mount_point == norm_mp) {
                m_pimpl->mounts.erase(std::next(it).base());
                return true;
            }
        }
        return false;
    }

    void vfs::clear() {
        m_pimpl->mounts.clear();
    }

    bool vfs::empty() const {
        return m_pimpl->mounts.empty();
    }

    size_t vfs::mount_count() const noexcept {
        return m_pimpl->mounts.size();
    }

    void vfs::set_locale(std::string_view locale) {
        m_pimpl->locale = normalize_path(locale);
    }

    std::string_view vfs::locale() const noexcept {
        return m_pimpl->locale;
    }

    bool vfs::exists(std::string_view path) const {
        return stat(path).has_value();
    }

    std::optional<file_info> vfs::stat(std::string_view path) const {
        const std::string vpath = normalize_path(path);
        if (vpath.empty()) {
            if (m_pimpl->mounts.empty()) return std::nullopt;
            return file_info{"", 0, true};
        }

        if (!m_pimpl->locale.empty()) {
            const std::string loc_prefix = m_pimpl->locale + "/";
            if (!vpath.starts_with(loc_prefix)) {
                std::string loc_vpath = loc_prefix + vpath;
                auto st = m_pimpl->stat_direct(loc_vpath);
                if (st.has_value()) {
                    st->path = vpath;
                    return st;
                }
            }
        }

        return m_pimpl->stat_direct(vpath);
    }

    std::vector<file_info> vfs::list_files() const {
        std::unordered_map<std::string, file_info> unified_map;

        // Traverse from oldest to newest mount, so newer mounts overwrite older entries (LIFO overlay)
        for (const auto& m : m_pimpl->mounts) {
            auto files = m.source->list_files();
            for (auto& f : files) {
                std::string vpath;
                if (m.mount_point.empty()) {
                    vpath = f.path;
                } else {
                    if (f.path.empty()) {
                        vpath = m.mount_point;
                    } else {
                        vpath = m.mount_point + "/" + f.path;
                    }
                }
                vpath = normalize_path(vpath);
                if (vpath.empty()) continue;

                f.path = vpath;
                unified_map[vpath] = f;
            }

            if (!m.mount_point.empty()) {
                if (unified_map.find(m.mount_point) == unified_map.end()) {
                    unified_map[m.mount_point] = file_info{m.mount_point, 0, true};
                }
            }
        }

        if (!m_pimpl->locale.empty()) {
            std::string prefix = m_pimpl->locale + "/";
            std::vector<std::string> loc_keys;
            for (const auto& [k, _] : unified_map) {
                if (k == m_pimpl->locale || k.starts_with(prefix)) {
                    loc_keys.push_back(k);
                }
            }
            for (const auto& k : loc_keys) {
                auto info = std::move(unified_map[k]);
                unified_map.erase(k);
                if (k != m_pimpl->locale) {
                    std::string mapped = k.substr(prefix.size());
                    if (!mapped.empty()) {
                        info.path = mapped;
                        unified_map[mapped] = std::move(info);
                    }
                }
            }
        }

        std::vector<file_info> result;
        result.reserve(unified_map.size());
        for (auto& [_, info] : unified_map) {
            result.push_back(std::move(info));
        }

        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return a.path < b.path;
        });

        return result;
    }

    std::vector<uint8_t> vfs::read(std::string_view path) const {
        const std::string vpath = normalize_path(path);
        if (vpath.empty()) {
            return {};
        }

        if (!m_pimpl->locale.empty()) {
            const std::string loc_prefix = m_pimpl->locale + "/";
            if (!vpath.starts_with(loc_prefix)) {
                std::string loc_vpath = loc_prefix + vpath;
                if (m_pimpl->has_file_direct(loc_vpath)) {
                    return m_pimpl->read_direct(loc_vpath);
                }
            }
        }

        return m_pimpl->read_direct(vpath);
    }

    std::unique_ptr<std::istream> vfs::open(std::string_view path) const {
        const std::string vpath = normalize_path(path);
        if (vpath.empty()) {
            return nullptr;
        }

        if (!m_pimpl->locale.empty()) {
            const std::string loc_prefix = m_pimpl->locale + "/";
            if (!vpath.starts_with(loc_prefix)) {
                std::string loc_vpath = loc_prefix + vpath;
                if (m_pimpl->has_file_direct(loc_vpath)) {
                    return m_pimpl->open_direct(loc_vpath);
                }
            }
        }

        return m_pimpl->open_direct(vpath);
    }

    void vfs::dump(const std::filesystem::path& root_dir) const {
        if (!root_dir.empty()) {
            std::filesystem::create_directories(root_dir);
        }

        const auto files = list_files();
        for (const auto& f : files) {
            std::filesystem::path dest_path = root_dir.empty() ? std::filesystem::path(f.path) : (root_dir / f.path);

            if (f.is_directory) {
                std::filesystem::create_directories(dest_path);
                continue;
            }

            if (dest_path.has_parent_path()) {
                std::filesystem::create_directories(dest_path.parent_path());
            }

            auto data = read(f.path);
            std::ofstream out(dest_path, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) {
                throw std::runtime_error("Failed to open file for writing: " + dest_path.string());
            }

            if (!data.empty()) {
                out.write(reinterpret_cast<const char*>(data.data()), data.size());
                if (!out) {
                    throw std::runtime_error("Failed to write data to file: " + dest_path.string());
                }
            }
        }
    }

    void dump(const vfs& fs, const std::filesystem::path& root_dir) {
        fs.dump(root_dir);
    }
}
