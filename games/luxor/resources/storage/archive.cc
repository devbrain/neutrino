//
// Created by igor on 07/10/2026.
//

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <sstream>
#include <istream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cctype>
#include <bstream/span_reader.hh>
#include <bstream/stream_reader.hh>
#include <stdexcept>

#include "luxor/resources/storage/archive.hh"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define SINFL_IMPLEMENTATION
#include "sinfl.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "LzmaDec.h"
#include "7zAlloc.h"

namespace luxor::rc {
    namespace {
        // ============================================================================
        // Stream & I/O Helpers
        // ============================================================================

        class stream_pos_guard {
        public:
            explicit stream_pos_guard(std::istream& is)
                : m_is(is), m_pos(is.tellg()) {}

            ~stream_pos_guard() {
                restore();
            }

            void restore() {
                m_is.clear();
                m_is.seekg(m_pos, std::ios::beg);
            }

            stream_pos_guard(const stream_pos_guard&) = delete;
            stream_pos_guard& operator=(const stream_pos_guard&) = delete;

        private:
            std::istream& m_is;
            std::streampos m_pos;
        };

        // Keep the format probes' non-throwing contract while sharing exact reads
        // and little-endian field decoding with the other resource parsers.
        template <typename T>
        bool read_le(std::istream& is, T& val) {
            try {
                bstream::stream_reader reader(is, bstream::endian::little);
                reader >> val;
                return true;
            } catch (const bstream::bstream_error&) {
                return false;
            }
        }

        bool read_bytes(std::istream& is, void* dest, size_t count) {
            try {
                bstream::stream_reader(is).read_exact(dest, count);
                return true;
            } catch (const bstream::bstream_error&) {
                return false;
            }
        }

        std::string read_string(std::istream& is, size_t len) {
            std::string s(len, '\0');
            if (len > 0) {
                if (!read_bytes(is, s.data(), len)) {
                    throw std::runtime_error("Unexpected EOF reading string of length " + std::to_string(len));
                }
            }
            return s;
        }

        uint64_t get_stream_size(std::istream& is) {
            stream_pos_guard guard(is);
            is.seekg(0, std::ios::end);
            return static_cast<uint64_t>(is.tellg());
        }

        void trim_trailing_nulls_and_spaces(std::string& s) {
            while (!s.empty() && (s.back() == '\0' || std::isspace(static_cast<unsigned char>(s.back())))) {
                s.pop_back();
            }
        }

        std::string sanitize_path(std::string_view raw_path) {
            std::string clean;
            clean.reserve(raw_path.size());

            // Normalize slashes
            for (char c : raw_path) {
                clean.push_back(c == '\\' ? '/' : c);
            }

            // Strip trailing null characters and whitespace
            trim_trailing_nulls_and_spaces(clean);

            // Strip leading slashes
            size_t start = 0;
            while (start < clean.size() && clean[start] == '/') {
                ++start;
            }
            if (start > 0) {
                clean = clean.substr(start);
            }

            // Split components and eliminate traversal (../)
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
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
            return result;
        }

        // ============================================================================
        // 7-Zip LZMA C SDK Decompressor (native, no dlopen)
        // ============================================================================

        bool decompress_lzma(const uint8_t* in_data, size_t in_size, uint8_t* out_data, size_t out_size) {
            if (out_size == 0) return true;
            if (in_size < 13) return false;

            SizeT dest_len = out_size;
            SizeT src_len = in_size - 13;
            ELzmaStatus status;
            ISzAlloc alloc = {SzAlloc, SzFree};

            SRes res = LzmaDecode(
                out_data, &dest_len,
                in_data + 13, &src_len,
                in_data, 5,
                LZMA_FINISH_ANY,
                &status,
                &alloc
            );

            return (res == SZ_OK && dest_len == out_size);
        }

        // ============================================================================
        // Decompression Helpers
        // ============================================================================

        bool decompress_deflate(const uint8_t* in, size_t in_size, uint8_t* out, size_t out_size) {
            if (out_size == 0) return true;
            if (in_size == 0) return false;

            std::vector<uint8_t> padded_in;
            padded_in.reserve(in_size + 16);
            padded_in.assign(in, in + in_size);
            padded_in.resize(in_size + 16, 0);
            const uint8_t* p_in = padded_in.data();

            if (in_size >= 2 && p_in[0] == 0x78) {
                int r = sinflate(out, static_cast<int>(out_size), p_in + 2, static_cast<int>(in_size - 2));
                if (r == static_cast<int>(out_size)) {
                    return true;
                }
                if (in_size >= 6) {
                    r = zsinflate(out, static_cast<int>(out_size), p_in, static_cast<int>(in_size));
                    if (r == static_cast<int>(out_size)) {
                        return true;
                    }
                }
            }

            {
                int r = sinflate(out, static_cast<int>(out_size), p_in, static_cast<int>(in_size));
                if (r == static_cast<int>(out_size)) {
                    return true;
                }
            }

            if (in_size > 2) {
                int r = sinflate(out, static_cast<int>(out_size), p_in + 2, static_cast<int>(in_size - 2));
                if (r == static_cast<int>(out_size)) {
                    return true;
                }
            }

            return false;
        }

        // =======================================================================
        // Archive Entry Model
        // =======================================================================
        enum class entry_type : uint32_t {
            Uncompressed = 0,
            Directory = 1,
            Lzma = 6,
            Deflate = 10,
            Sfs = 20,
            Unknown = 0xFFFFFFFF
        };

        struct entry {
            std::string name{};
            entry_type type{entry_type::Unknown};
            uint64_t offset{0};
            uint64_t size{0};  // uncompressed size
            uint64_t zsize{0}; // compressed size
        };

        void sort_entries(std::vector<entry>& entries) {
            std::sort(entries.begin(), entries.end(), [](const entry& a, const entry& b) {
                return a.name < b.name;
            });
        }
    }

    // =======================================================================
    // MJZ (MumboJumbo Zip)
    // =======================================================================
    namespace mjz {
        namespace {
            struct header {
                char magic[4];
                uint32_t version;
                uint32_t file_count;
                uint32_t checksum;
            };

            bstream::stream_reader& operator>>(bstream::stream_reader& reader, header& h) {
                reader >> h.magic >> h.version >> h.file_count >> h.checksum;
                return reader;
            }
        }

        bool probe(std::istream& is, uint64_t file_size) {
            if (file_size < 16) return false;
            stream_pos_guard guard(is);
            header hdr{};
            return read_le(is, hdr) && (std::memcmp(hdr.magic, "MJZ0", 4) == 0);
        }

        std::vector<entry> load(std::istream& is, uint64_t /*file_size*/) {
            header hdr{};
            if (!read_le(is, hdr)) {
                throw std::runtime_error("Failed to read MJZ header.");
            }

            std::vector<entry> entries;
            entries.reserve(hdr.file_count);

            for (uint32_t i = 0; i < hdr.file_count; ++i) {
                entry e;

                if (hdr.version == 0) {
                    char name_buf[128];
                    if (!read_bytes(is, name_buf, sizeof(name_buf))) {
                        throw std::runtime_error("Failed to read MJZ entry #" + std::to_string(i) + " name.");
                    }
                    name_buf[127] = '\0';
                    e.name = sanitize_path(name_buf);

                    uint32_t meta[4];
                    if (!read_le(is, meta)) {
                        throw std::runtime_error("Failed to read MJZ entry #" + std::to_string(i) + " metadata.");
                    }
                    e.type = static_cast<entry_type>(meta[0]);
                    e.offset = meta[1];
                    e.size = meta[2];
                    e.zsize = meta[3];
                } else {
                    uint32_t meta[4];
                    if (!read_le(is, meta)) {
                        throw std::runtime_error("Failed to read MJZ entry #" + std::to_string(i) + " metadata.");
                    }
                    e.type = static_cast<entry_type>(meta[0]);
                    e.offset = meta[1];
                    e.size = meta[2];
                    e.zsize = meta[3];

                    uint8_t name_len = 0;
                    if (!read_le(is, name_len)) {
                        throw std::runtime_error("Failed to read MJZ entry #" + std::to_string(i) + " name length.");
                    }
                    e.name = sanitize_path(read_string(is, name_len));
                }

                entries.push_back(std::move(e));
            }
            return entries;
        }
    }

    // =======================================================================
    // NPK (NovaLogic/MumboJumbo NovaPack)
    // =======================================================================
    namespace npk {
        namespace {
            struct header {
                char magic[8];
                uint32_t unknown1;
                uint32_t unknown2;
                char marker[9];
                uint32_t offset;
            };

            bstream::stream_reader& operator>>(bstream::stream_reader& reader, header& h) {
                reader >> h.magic >> h.unknown1 >> h.unknown2 >> h.marker >> h.offset;
                return reader;
            }
        }

        bool probe(std::istream& is, uint64_t file_size) {
            if (file_size < 29) return false;
            stream_pos_guard guard(is);
            header hdr{};
            return read_le(is, hdr) &&
                   std::memcmp(hdr.magic, "NovaPack", 8) == 0 &&
                   std::memcmp(hdr.marker, "DictStart", 9) == 0;
        }

        std::vector<entry> load(std::istream& is, uint64_t /*file_size*/) {
            header hdr{};
            if (!read_le(is, hdr)) {
                throw std::runtime_error("Failed to read NPK header.");
            }

            std::vector<entry> res;
            while (is.tellg() < hdr.offset) {
                uint32_t name_len = 0;
                if (!read_le(is, name_len)) break;
                auto raw_name = read_string(is, name_len);

                uint32_t data_offset = 0;
                uint32_t data_size = 0;
                if (!read_le(is, data_offset) || !read_le(is, data_size)) break;

                if (is.tellg() <= hdr.offset) {
                    res.push_back(entry{
                        .name = sanitize_path(raw_name),
                        .type = entry_type::Uncompressed,
                        .offset = hdr.offset + data_offset,
                        .size = data_size,
                        .zsize = data_size
                    });
                }
            }
            return res;
        }
    }

    // =======================================================================
    // NHP (NovaHackPack)
    // =======================================================================
    namespace nhp {
        namespace {
            struct header {
                char magic[12];       // "NovaHackPack"
                uint32_t dict_offset; // offset to dictionary
                char marker[13];      // "LumpDataBegin"
            };

            bstream::stream_reader& operator>>(bstream::stream_reader& reader, header& h) {
                reader >> h.magic >> h.dict_offset >> h.marker;
                return reader;
            }
        }

        bool probe(std::istream& is, uint64_t file_size) {
            if (file_size < 29) return false;
            stream_pos_guard guard(is);
            header hdr{};
            return read_le(is, hdr) &&
                   std::memcmp(hdr.magic, "NovaHackPack", 12) == 0 &&
                   std::memcmp(hdr.marker, "LumpDataBegin", 13) == 0;
        }

        std::vector<entry> load(std::istream& is, uint64_t file_size) {
            header hdr{};
            if (!read_le(is, hdr)) {
                throw std::runtime_error("Failed to read NHP header.");
            }

            if (hdr.dict_offset > file_size) {
                throw std::runtime_error("NHP dictionary offset is out of bounds.");
            }

            is.seekg(hdr.dict_offset, std::ios::beg);
            if (!is) {
                throw std::runtime_error("Failed to seek to NHP dictionary offset.");
            }

            char dict_marker[15];
            if (!read_bytes(is, dict_marker, sizeof(dict_marker)) || std::memcmp(dict_marker, "DictionaryBegin", 15) != 0) {
                throw std::runtime_error("Invalid NHP DictionaryBegin marker.");
            }

            uint32_t entry_count = 0;
            if (!read_le(is, entry_count)) {
                throw std::runtime_error("Failed to read NHP entry count.");
            }

            std::vector<entry> entries;
            entries.reserve(entry_count);

            for (uint32_t i = 0; i < entry_count; ++i) {
                uint16_t name_len = 0;
                if (!read_le(is, name_len)) {
                    throw std::runtime_error("Failed to read NHP entry #" + std::to_string(i) + " name length.");
                }
                auto raw_name = read_string(is, name_len);

                uint32_t offset = 0;
                uint32_t size = 0;
                if (!read_le(is, offset) || !read_le(is, size)) {
                    throw std::runtime_error("Failed to read NHP entry #" + std::to_string(i) + " metadata.");
                }

                entries.push_back(entry{
                    .name = sanitize_path(raw_name),
                    .type = entry_type::Uncompressed,
                    .offset = offset,
                    .size = size,
                    .zsize = size
                });
            }

            char dict_end[13];
            if (read_bytes(is, dict_end, sizeof(dict_end))) {
                if (std::memcmp(dict_end, "DictionaryEnd", 13) != 0) {
                    throw std::runtime_error("Invalid NHP DictionaryEnd marker.");
                }
            }

            return entries;
        }
    }

    // =======================================================================
    // SFS (AAMVHFSS - Ancient Egyptian / Luxor Adventures Filesystem)
    // =======================================================================
    namespace sfs {
        namespace {
            struct header {
                char magic[8];         // "AAMVHFSS"
                uint8_t reserved1[284];
                float version;         // offset 0x124
                uint32_t chunk_size;   // offset 0x128
                uint8_t reserved2[20];
                uint32_t tree_address; // offset 0x140
                uint32_t n_tree_items; // offset 0x144
                uint32_t sfs_n_chunks; // offset 0x148
            };

            struct raw_entry {
                int32_t ptr_to_ptr;     // 0x00
                uint64_t size;          // 0x04
                uint64_t create_time;   // 0x0C
                uint64_t mod_time;      // 0x14
                uint64_t some_time;     // 0x1C
                uint32_t perms;         // 0x24
                int32_t parent;         // 0x28
                uint8_t reserved1[176]; // 0x2C
                uint8_t is_dir;         // 0xDC
                uint8_t reserved2[3];   // 0xDD
                char name[256];         // 0xE0
                uint8_t reserved3[32];  // 0x1E0
            };

            bstream::stream_reader& operator>>(bstream::stream_reader& reader, header& h) {
                reader >> h.magic >> h.reserved1 >> h.version >> h.chunk_size >> h.reserved2
                       >> h.tree_address >> h.n_tree_items >> h.sfs_n_chunks;
                return reader;
            }

            bstream::span_reader& operator>>(bstream::span_reader& reader, raw_entry& re) {
                reader >> re.ptr_to_ptr >> re.size >> re.create_time >> re.mod_time
                       >> re.some_time >> re.perms >> re.parent >> re.reserved1
                       >> re.is_dir >> re.reserved2 >> re.name >> re.reserved3;
                return reader;
            }

            struct sfs_item {
                std::string name;
                int32_t parent{-1};
                uint64_t size{0};
                int32_t ptr_to_ptr{-1};
                bool is_dir{false};
            };
        }

        bool probe(std::istream& is, uint64_t file_size) {
            if (file_size < 0x14C) return false;
            stream_pos_guard guard(is);
            char magic[8]{};
            return read_bytes(is, magic, 8) && std::memcmp(magic, "AAMVHFSS", 8) == 0;
        }

        std::vector<entry> load(std::istream& is, uint64_t file_size) {
            header hdr{};
            if (!read_le(is, hdr)) {
                throw std::runtime_error("Failed to read SFS header.");
            }

            if (std::memcmp(hdr.magic, "AAMVHFSS", 8) != 0) {
                throw std::runtime_error("Invalid SFS magic signature.");
            }

            uint32_t chunk_size = hdr.chunk_size ? hdr.chunk_size : 4096;
            if (chunk_size < 512 || chunk_size > 65536) {
                chunk_size = 4096;
            }

            std::vector<sfs_item> raw_items;
            raw_items.reserve(hdr.n_tree_items);

            uint32_t curr_chunk = hdr.tree_address;
            std::vector<uint8_t> chunk_buf(chunk_size);

            while (raw_items.size() < hdr.n_tree_items && curr_chunk != 0 && curr_chunk != 0xFFFFFFFF) {
                if (static_cast<uint64_t>(curr_chunk) * chunk_size + chunk_size > file_size) {
                    throw std::runtime_error("SFS tree chunk out of bounds.");
                }

                is.seekg(static_cast<uint64_t>(curr_chunk) * chunk_size, std::ios::beg);
                if (!read_bytes(is, chunk_buf.data(), chunk_size)) {
                    throw std::runtime_error("Failed to read SFS tree chunk.");
                }

                bstream::span_reader chunk_reader(chunk_buf, bstream::endian::little);
                chunk_reader.seek(0x118);
                const auto next_chunk = chunk_reader.read<uint32_t>();

                size_t items_in_chunk = std::min(static_cast<size_t>(7), static_cast<size_t>(hdr.n_tree_items - raw_items.size()));
                for (size_t j = 0; j < items_in_chunk; ++j) {
                    size_t entry_offset = 0x138 + j * 512;
                    if (entry_offset + 512 > chunk_size) {
                        break;
                    }

                    raw_entry re{};
                    chunk_reader.seek(entry_offset);
                    chunk_reader >> re;

                    sfs_item item;
                    char name_buf[257];
                    std::memcpy(name_buf, re.name, 256);
                    name_buf[256] = '\0';
                    item.name = name_buf;
                    trim_trailing_nulls_and_spaces(item.name);

                    item.parent = re.parent;
                    item.size = re.size;
                    item.ptr_to_ptr = re.ptr_to_ptr;
                    item.is_dir = (re.is_dir != 0);

                    raw_items.push_back(std::move(item));
                }

                curr_chunk = next_chunk;
            }

            // Build full paths
            std::vector<entry> entries;
            entries.reserve(raw_items.size());

            for (size_t i = 0; i < raw_items.size(); ++i) {
                std::string full_path = raw_items[i].name;
                int32_t p = raw_items[i].parent;
                int depth = 0;
                while (p >= 0 && static_cast<size_t>(p) < raw_items.size() && depth++ < 64) {
                    full_path = raw_items[p].name + "/" + full_path;
                    p = raw_items[p].parent;
                }

                entries.push_back(entry{
                    .name = sanitize_path(full_path),
                    .type = raw_items[i].is_dir ? entry_type::Directory : entry_type::Sfs,
                    .offset = raw_items[i].ptr_to_ptr >= 0 ? static_cast<uint64_t>(raw_items[i].ptr_to_ptr) : 0,
                    .size = raw_items[i].size,
                    .zsize = raw_items[i].size
                });
            }

            return entries;
        }
    }

    // =======================================================================
    // PFP (Luxor: 5th Passage Package File - PFPK)
    // =======================================================================
    namespace pfp {
        bool probe(std::istream& is, uint64_t file_size) {
            if (file_size < 8) return false;
            stream_pos_guard guard(is);
            char magic[4]{};
            uint32_t num_files = 0;
            return read_bytes(is, magic, 4) &&
                   std::memcmp(magic, "PFPK", 4) == 0 &&
                   read_le(is, num_files) &&
                   num_files > 0 &&
                   8 + static_cast<uint64_t>(num_files) * 9 <= file_size;
        }

        std::vector<entry> load(std::istream& is, uint64_t file_size) {
            char magic[4]{};
            if (!read_bytes(is, magic, 4) || std::memcmp(magic, "PFPK", 4) != 0) {
                throw std::runtime_error("Invalid PFP header magic (expected PFPK)");
            }

            uint32_t num_files = 0;
            if (!read_le(is, num_files)) {
                throw std::runtime_error("Failed to read PFP file count");
            }

            std::vector<entry> entries;
            entries.reserve(num_files);

            for (uint32_t i = 0; i < num_files; ++i) {
                uint8_t name_len = 0;
                if (!read_le(is, name_len)) {
                    throw std::runtime_error("Unexpected EOF reading PFP entry name length");
                }

                auto raw_name = read_string(is, name_len);

                uint32_t offset = 0;
                uint32_t size = 0;
                if (!read_le(is, offset) || !read_le(is, size)) {
                    throw std::runtime_error("Unexpected EOF reading PFP entry offset/size");
                }

                if (static_cast<uint64_t>(offset) + size > file_size) {
                    throw std::runtime_error("PFP entry out of bounds: " + raw_name);
                }

                entries.push_back(entry{
                    .name = sanitize_path(raw_name),
                    .type = entry_type::Uncompressed,
                    .offset = offset,
                    .size = size,
                    .zsize = size
                });
            }

            return entries;
        }
    }

    // =======================================================================
    // Data Extraction Helpers
    // =======================================================================
    namespace {
        bool read_sfs_data(const entry& e, std::istream& is, uint64_t file_size,
                           std::vector<uint8_t>& out_data, std::string& err) {
            if (e.size == 0) {
                out_data.clear();
                return true;
            }

            constexpr uint32_t chunk_size = 4096;
            size_t num_chunks = 1;
            if (e.size > 3784) {
                num_chunks = 1 + (e.size - 3784 + 4063) / 4064;
            }

            std::vector<uint32_t> chunk_ptrs;
            chunk_ptrs.reserve(num_chunks);
            uint32_t curr_ptr_chunk = static_cast<uint32_t>(e.offset);

            while (chunk_ptrs.size() < num_chunks && curr_ptr_chunk != 0 && curr_ptr_chunk != 0xFFFFFFFF) {
                size_t needed = num_chunks - chunk_ptrs.size();
                size_t to_read = std::min(needed, static_cast<size_t>(946));

                if (static_cast<uint64_t>(curr_ptr_chunk) * chunk_size + chunk_size > file_size) {
                    err = "SFS pointer table chunk offset out of bounds";
                    return false;
                }

                is.seekg(static_cast<uint64_t>(curr_ptr_chunk) * chunk_size + 0x118, std::ios::beg);
                uint32_t next_ptr_chunk = 0;
                if (!read_le(is, next_ptr_chunk)) {
                    err = "Failed to read next pointer chunk";
                    return false;
                }

                is.seekg(static_cast<uint64_t>(curr_ptr_chunk) * chunk_size + 0x138, std::ios::beg);
                size_t start_idx = chunk_ptrs.size();
                chunk_ptrs.resize(start_idx + to_read);
                for (size_t k = start_idx; k < start_idx + to_read; ++k) {
                    if (!read_le(is, chunk_ptrs[k])) {
                        err = "Failed to read chunk pointers";
                        return false;
                    }
                }

                curr_ptr_chunk = next_ptr_chunk;
            }

            if (chunk_ptrs.size() < num_chunks) {
                err = "Incomplete SFS pointer table";
                return false;
            }

            out_data.clear();
            out_data.reserve(e.size);

            std::vector<uint8_t> chunk_buf(chunk_size);
            for (size_t idx = 0; idx < chunk_ptrs.size(); ++idx) {
                uint32_t c = chunk_ptrs[idx];
                if (static_cast<uint64_t>(c) * chunk_size + chunk_size > file_size) {
                    err = "SFS data chunk offset out of bounds";
                    return false;
                }

                is.seekg(static_cast<uint64_t>(c) * chunk_size, std::ios::beg);
                if (!read_bytes(is, chunk_buf.data(), chunk_size)) {
                    err = "Failed to read SFS data chunk";
                    return false;
                }

                if (idx == 0) {
                    size_t avail = 4096 - 0x138;
                    size_t to_copy = std::min(avail, static_cast<size_t>(e.size - out_data.size()));
                    out_data.insert(out_data.end(), chunk_buf.begin() + 0x138, chunk_buf.begin() + 0x138 + to_copy);
                } else {
                    size_t part1_avail = 0x118;
                    size_t part1_copy = std::min(part1_avail, static_cast<size_t>(e.size - out_data.size()));
                    out_data.insert(out_data.end(), chunk_buf.begin(), chunk_buf.begin() + part1_copy);

                    if (out_data.size() < e.size) {
                        size_t part2_avail = 4096 - 0x138;
                        size_t part2_copy = std::min(part2_avail, static_cast<size_t>(e.size - out_data.size()));
                        out_data.insert(out_data.end(), chunk_buf.begin() + 0x138, chunk_buf.begin() + 0x138 + part2_copy);
                    }
                }
            }

            if (out_data.size() != e.size) {
                err = "SFS extracted size mismatch";
                return false;
            }
            return true;
        }

        bool read_entry_data(const entry& e, std::istream& is,
                             uint64_t file_size, std::vector<uint8_t>& out_data, std::string& err) {
            if (e.type == entry_type::Directory) {
                out_data.clear();
                return true;
            }

            if (e.type == entry_type::Sfs) {
                return read_sfs_data(e, is, file_size, out_data, err);
            }

            if (static_cast<uint64_t>(e.offset) + e.zsize > file_size) {
                err = "Offset out of bounds (" + std::to_string(e.offset) + " + " +
                      std::to_string(e.zsize) + " > " + std::to_string(file_size) + ")";
                return false;
            }

            std::vector<uint8_t> compressed_data(e.zsize);
            try {
                bstream::stream_reader reader(is);
                reader.seek(e.offset);
                reader.read_exact(compressed_data.data(), compressed_data.size());
            } catch (const bstream::bstream_error&) {
                err = "Failed to read compressed bytes from archive";
                return false;
            }

            out_data.resize(e.size);

            switch (e.type) {
                case entry_type::Uncompressed:
                    if (e.size != e.zsize) {
                        err = "Uncompressed entry size mismatch (size=" + std::to_string(e.size) +
                              ", zsize=" + std::to_string(e.zsize) + ")";
                        return false;
                    }
                    if (e.size > 0) {
                        std::memcpy(out_data.data(), compressed_data.data(), e.size);
                    }
                    return true;

                case entry_type::Deflate:
                    if (!decompress_deflate(compressed_data.data(), e.zsize, out_data.data(), e.size)) {
                        err = "Deflate decompression failed";
                        return false;
                    }
                    return true;

                case entry_type::Lzma:
                    if (!decompress_lzma(compressed_data.data(), e.zsize, out_data.data(), e.size)) {
                        err = "LZMA decompression failed";
                        return false;
                    }
                    return true;

                default:
                    err = "Unsupported entry type: " + std::to_string(static_cast<uint32_t>(e.type));
                    return false;
            }
        }
    }

    // =======================================================================
    // Format Probers Table & Entry Loader
    // =======================================================================
    namespace {
        struct format_prober {
            const char* name;
            bool (*probe)(std::istream& is, uint64_t file_size);
            std::vector<entry> (*load)(std::istream& is, uint64_t file_size);
        };

        const format_prober PROBERS[] = {
            { "MJZ", mjz::probe, mjz::load },
            { "NPK", npk::probe, npk::load },
            { "NHP", nhp::probe, nhp::load },
            { "SFS", sfs::probe, sfs::load },
            { "PFP", pfp::probe, pfp::load }
        };

        std::vector<entry> load_archive_entries(std::istream& is, uint64_t file_size) {
            const auto initial_pos = is.tellg();
            for (const auto& prober : PROBERS) {
                is.clear();
                is.seekg(initial_pos, std::ios::beg);
                if (prober.probe(is, file_size)) {
                    is.clear();
                    is.seekg(initial_pos, std::ios::beg);
                    auto entries = prober.load(is, file_size);
                    sort_entries(entries);
                    return entries;
                }
            }
            throw std::runtime_error("Unknown archive type");
        }
    }

    // =======================================================================
    // archive::impl & Implementation
    // =======================================================================
    struct archive::impl {
        std::unique_ptr<std::istream> owned_file;
        std::istream& file;
        uint64_t file_size;
        std::vector<entry> entries;

        impl(std::istream& is, uint64_t size)
            : owned_file(nullptr), file(is), file_size(size) {}

        impl(std::unique_ptr<std::istream> owned_stream, uint64_t size)
            : owned_file(std::move(owned_stream)), file(*owned_file), file_size(size) {}

        [[nodiscard]] const entry* find_entry(std::string_view path) const {
            const auto clean_path = sanitize_path(path);
            const auto it = std::lower_bound(entries.begin(), entries.end(), clean_path,
                                             [](const entry& e, const std::string& val) {
                                                 return e.name < val;
                                             });
            if (it != entries.end() && it->name == clean_path) {
                return &(*it);
            }
            return nullptr;
        }
    };

    archive::~archive() = default;

    archive::archive(std::istream& is, uint64_t file_size)
        : m_pimpl(std::make_unique<impl>(is, file_size)) {
    }

    archive::archive(std::unique_ptr<std::istream> owned_stream, uint64_t file_size)
        : m_pimpl(std::make_unique<impl>(std::move(owned_stream), file_size)) {
    }

    bool archive::probe(std::istream& is) {
        if (!is.good()) {
            return false;
        }
        const auto file_size = get_stream_size(is);
        stream_pos_guard guard(is);

        for (const auto& prober : PROBERS) {
            guard.restore();
            if (prober.probe(is, file_size)) {
                return true;
            }
        }
        return false;
    }

    bool archive::probe(const std::filesystem::path& path) {
        std::ifstream ifs(path, std::ios::binary);
        return ifs.is_open() && probe(ifs);
    }

    std::unique_ptr<archive> archive::load(std::istream& is) {
        if (!is.good()) {
            throw std::runtime_error("Invalid or unreadable input stream for archive.");
        }
        const auto file_size = get_stream_size(is);
        auto entries = load_archive_entries(is, file_size);

        auto ret_archive = std::unique_ptr<archive>(new archive(is, file_size));
        ret_archive->m_pimpl->entries = std::move(entries);
        return ret_archive;
    }

    std::unique_ptr<archive> archive::load(const std::filesystem::path& path) {
        auto ifs = std::make_unique<std::ifstream>(path, std::ios::binary);
        if (!ifs->is_open()) {
            throw std::runtime_error("Failed to open archive file: " + path.string());
        }
        const auto file_size = get_stream_size(*ifs);
        try {
            auto entries = load_archive_entries(*ifs, file_size);
            auto ret_archive = std::unique_ptr<archive>(new archive(std::move(ifs), file_size));
            ret_archive->m_pimpl->entries = std::move(entries);
            return ret_archive;
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + ": " + path.string());
        }
    }

    bool archive::has_file(std::string_view path) const {
        return m_pimpl->find_entry(path) != nullptr;
    }

    std::optional<file_info> archive::stat(std::string_view path) const {
        const auto* e = m_pimpl->find_entry(path);
        if (!e) {
            return std::nullopt;
        }
        return file_info{
            .path = e->name,
            .size = e->size,
            .is_directory = e->type == entry_type::Directory
        };
    }

    std::vector<file_info> archive::list_files() const {
        std::vector<file_info> res(m_pimpl->entries.size());
        std::transform(m_pimpl->entries.begin(), m_pimpl->entries.end(), res.begin(), [](const auto& e) {
            return file_info{
                .path = e.name,
                .size = e.size,
                .is_directory = e.type == entry_type::Directory
            };
        });
        return res;
    }

    std::vector<uint8_t> archive::read(std::string_view path) const {
        const auto* e = m_pimpl->find_entry(path);
        if (!e) {
            return {};
        }
        std::vector<uint8_t> out;
        std::string err;
        if (!read_entry_data(*e, m_pimpl->file, m_pimpl->file_size, out, err)) {
            throw std::runtime_error("Error reading " + std::string(path) + ": " + err);
        }
        return out;
    }

    void archive::dump(const std::filesystem::path& root_dir) const {
        if (!root_dir.empty()) {
            std::filesystem::create_directories(root_dir);
        }

        std::vector<uint8_t> buffer;
        for (const auto& e : m_pimpl->entries) {
            std::filesystem::path dest_path = root_dir.empty() ? std::filesystem::path(e.name) : (root_dir / e.name);

            if (e.type == entry_type::Directory) {
                std::filesystem::create_directories(dest_path);
                continue;
            }

            if (dest_path.has_parent_path()) {
                std::filesystem::create_directories(dest_path.parent_path());
            }

            std::string err;
            if (!read_entry_data(e, m_pimpl->file, m_pimpl->file_size, buffer, err)) {
                throw std::runtime_error("Error reading " + e.name + " from archive: " + err);
            }

            std::ofstream out(dest_path, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) {
                throw std::runtime_error("Failed to open file for writing: " + dest_path.string());
            }

            if (!buffer.empty()) {
                out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                if (!out) {
                    throw std::runtime_error("Failed to write data to file: " + dest_path.string());
                }
            }
        }
    }

    // =======================================================================
    // Free Function Overloads
    // =======================================================================
    bool probe(std::istream& is) {
        return archive::probe(is);
    }

    bool probe(const std::filesystem::path& path) {
        return archive::probe(path);
    }

    void dump(const archive& ar, const std::filesystem::path& root_dir) {
        ar.dump(root_dir);
    }

    void dump(const archive* ar, const std::filesystem::path& root_dir) {
        if (ar) {
            ar->dump(root_dir);
        }
    }

    void dump(const std::unique_ptr<archive>& ar, const std::filesystem::path& root_dir) {
        if (ar) {
            ar->dump(root_dir);
        }
    }

    void extract(const archive& ar, const std::filesystem::path& root_dir) {
        ar.dump(root_dir);
    }
}
