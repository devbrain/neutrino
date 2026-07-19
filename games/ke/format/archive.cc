//
// Created by igor on 28/06/2026.
//

#include <iostream>
#include <failsafe/message.hh>
#include <neutrino/video/image_loader.hh>
#include <neutrino/video/sprite/cpu_texture_atlas.hh>

#include <ke/format/archive.hh>
#include <ke/format/bob.hh>
#include <ke/format/binary_reader.hh>

namespace rs {
    namespace {
        struct ke_header {
            uint32_t resource_count;
            uint32_t header_size;
            uint32_t directory_size_bytes;
            uint32_t name_table_offset;
            uint32_t name_table_size_bytes;
            uint32_t data_offset;
            uint32_t data_offset_size_bytes;
        };

        binary_reader& operator >>(binary_reader& bio, ke_header& h) {
            bio >> h.resource_count >> h.header_size >> h.directory_size_bytes >> h.name_table_offset >> h.
                name_table_size_bytes
                >> h.data_offset >> h.data_offset_size_bytes;

            return bio;
        }

        struct ke_dir_entry {
            uint16_t name_offset;
            uint16_t unknown;
            uint32_t data_rel_offset;
            uint32_t data_size;
        };

        binary_reader& operator >>(binary_reader& bio, ke_dir_entry& h) {
            bio >> h.name_offset >> h.unknown >> h.data_rel_offset >> h.data_size;
            return bio;
        }

        bool validate_header(const ke_header& hdr, uint64_t file_size, std::vector <diagnostic>& diagnostics) {
            if (hdr.header_size < 28) {
                diagnostics.push_back({
                    diagnostic::severity_level::error, "Invalid header: header_size must be at least 28 bytes"
                });
                return false;
            }

            uint64_t req_dir_size = static_cast <uint64_t>(hdr.resource_count) * sizeof(ke_dir_entry);
            if (hdr.directory_size_bytes < req_dir_size) {
                diagnostics.push_back({
                    diagnostic::severity_level::error,
                    failsafe::build_message("Invalid header: directory_size_bytes", hdr.directory_size_bytes,
                                            "is less than required by resource_count", req_dir_size)
                });
                return false;
            }

            uint64_t dir_end = static_cast <uint64_t>(hdr.header_size) + hdr.directory_size_bytes;
            if (hdr.name_table_offset < dir_end) {
                diagnostics.push_back({
                    diagnostic::severity_level::error,
                    failsafe::build_message("Invalid header: name_table_offset", hdr.name_table_offset,
                                            "overlaps directory section ending at", dir_end)
                });
                return false;
            }

            uint64_t name_table_end = static_cast <uint64_t>(hdr.name_table_offset) + hdr.name_table_size_bytes;
            if (hdr.data_offset < name_table_end) {
                diagnostics.push_back({
                    diagnostic::severity_level::error,
                    failsafe::build_message("Invalid header: data_offset", hdr.data_offset,
                                            "overlaps name table section ending at", name_table_end)
                });
                return false;
            }

            if (file_size != -1) {
                if (hdr.header_size > file_size) {
                    diagnostics.push_back({
                        diagnostic::severity_level::error, "Invalid header: header_size exceeds file size"
                    });
                    return false;
                }
                if (dir_end > file_size) {
                    diagnostics.push_back({
                        diagnostic::severity_level::error, "Invalid header: directory section extends beyond file size"
                    });
                    return false;
                }
                if (name_table_end > file_size) {
                    diagnostics.push_back({
                        diagnostic::severity_level::error, "Invalid header: name table section extends beyond file size"
                    });
                    return false;
                }
                uint64_t data_end = static_cast <uint64_t>(hdr.data_offset) + hdr.data_offset_size_bytes;
                if (data_end > file_size) {
                    diagnostics.push_back({
                        diagnostic::severity_level::error, "Invalid header: data section extends beyond file size"
                    });
                    return false;
                }
            }

            return true;
        }

        std::vector <ke_dir_entry> read_directory(binary_reader& bio, const ke_header& hdr,
                                                  std::vector <diagnostic>& diagnostics) {
            if (!bio) {
                diagnostics.push_back({
                    diagnostic::severity_level::error, "Stream is in a failed state before reading directory"
                });
                return {};
            }

            if (!bio.stream().seekg(hdr.header_size, std::ios::beg)) {
                diagnostics.push_back({diagnostic::severity_level::error, "Failed to seek to directory offset"});
                return {};
            }

            std::vector <ke_dir_entry> out;
            out.reserve(hdr.resource_count);

            for (uint32_t i = 0; i < hdr.resource_count; i++) {
                ke_dir_entry de{};
                bio >> de;
                if (!bio) {
                    diagnostics.push_back({
                        diagnostic::severity_level::error,
                        failsafe::build_message("Truncated read while reading directory entry", i)
                    });
                    break;
                }
                out.push_back(de);
            }
            return out;
        }
    }

    load_result load_resource(std::istream& is) {
        load_result result;

        // Query file size if seeking is supported
        is.seekg(0, std::ios::end);
        auto file_size_pos = is.tellg();
        uint64_t file_size = (file_size_pos >= 0) ? static_cast <uint64_t>(file_size_pos) : -1;
        is.seekg(0, std::ios::beg);

        binary_reader bio(is, std::endian::little);
        ke_header hdr{};
        bio >> hdr;
        if (!bio) {
            result.diagnostics.push_back({diagnostic::severity_level::error, "Failed to read file header"});
            return result;
        }

        if (!validate_header(hdr, file_size, result.diagnostics)) {
            return result;
        }

        auto directory = read_directory(bio, hdr, result.diagnostics);
        if (result.has_errors()) {
            return result;
        }

        result.resources.reserve(directory.size());

        for (std::size_t i = 0; i < directory.size(); ++i) {
            const auto& d = directory[i];

            // Prevent integer overflow with 64-bit casting
            const int64_t name_offset = static_cast <int64_t>(hdr.name_table_offset) + d.name_offset;

            // Validate name offset limits
            if (file_size != -1 && name_offset >= file_size) {
                result.diagnostics.push_back({
                    diagnostic::severity_level::warning,
                    failsafe::build_message("Resource entry", i, "has out-of-bounds name offset:", name_offset)
                });
                continue;
            }

            if (!is.seekg(name_offset, std::ios::beg)) {
                result.diagnostics.push_back({
                    diagnostic::severity_level::warning,
                    failsafe::build_message("Failed to seek to name offset", name_offset, "for entry", i)
                });
                continue;
            }

            resource r{};

            // Safe offset calculation and validation
            uint64_t res_start = static_cast <uint64_t>(hdr.data_offset) + d.data_rel_offset;
            uint64_t res_size = d.data_size;
            if (file_size != -1 && (res_start + res_size > file_size)) {
                result.diagnostics.push_back({
                    diagnostic::severity_level::warning,
                    failsafe::build_message("Resource entry", i, "has out-of-bounds data range: start", res_start,
                                            "size", res_size)
                });
                continue; // Skip invalid resource data range
            }
            r.offset = static_cast <uint32_t>(res_start);
            r.size = static_cast <uint32_t>(res_size);

            // Establish safe name parsing boundaries
            constexpr uint64_t max_name_len = 256;
            uint64_t name_bytes_left = max_name_len;

            if (file_size != -1) {
                uint64_t name_table_end = static_cast <uint64_t>(hdr.name_table_offset) + hdr.name_table_size_bytes;
                if (name_table_end > file_size) {
                    name_table_end = file_size;
                }
                if (name_offset < name_table_end) {
                    uint64_t table_limit = name_table_end - name_offset;
                    if (table_limit < name_bytes_left) {
                        name_bytes_left = table_limit;
                    }
                }
            }

            bool name_read_failed = false;
            while (name_bytes_left > 0) {
                char ch{};
                bio >> ch;
                if (!bio) {
                    name_read_failed = true;
                    break;
                }
                name_bytes_left--;
                if (ch >= 'A' && ch <= 'Z') {
                    ch += ('a' - 'A');
                }
                if (!ch) {
                    break;
                }
                r.name += ch;
            }

            if (name_read_failed) {
                result.diagnostics.push_back({
                    diagnostic::severity_level::warning,
                    failsafe::build_message("Truncated stream while reading resource name for entry", i)
                });
                continue;
            }

            result.resources.push_back(r);
        }

        std::sort(result.resources.begin(), result.resources.end(), [](const auto& a, const auto& b) {
            return a.name < b.name;
        });

        return result;
    }
}
