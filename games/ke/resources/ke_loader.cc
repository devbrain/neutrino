//
// Created by igor on 28/06/2026.
//

#include <iostream>
#include <neutrino/video/image_loader.hh>
#include <neutrino/video/sprite/cpu_texture_atlas.hh>

#include <failsafe/logger.hh>
#include <sdlpp/video/palette.hh>

#include "resources/ke_loader.hh"
#include "resources/ke_bob.hh"
#include "resources/binary_reader.hh"

#include <optional>

namespace rs {
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
                "Invalid header: directory_size_bytes (" + std::to_string(hdr.directory_size_bytes) +
                ") is less than required by resource_count (" + std::to_string(req_dir_size) + ")"
            });
            return false;
        }

        uint64_t dir_end = static_cast <uint64_t>(hdr.header_size) + hdr.directory_size_bytes;
        if (hdr.name_table_offset < dir_end) {
            diagnostics.push_back({
                diagnostic::severity_level::error,
                "Invalid header: name_table_offset (" + std::to_string(hdr.name_table_offset) +
                ") overlaps with directory section (ends at " + std::to_string(dir_end) + ")"
            });
            return false;
        }

        uint64_t name_table_end = static_cast <uint64_t>(hdr.name_table_offset) + hdr.name_table_size_bytes;
        if (hdr.data_offset < name_table_end) {
            diagnostics.push_back({
                diagnostic::severity_level::error,
                "Invalid header: data_offset (" + std::to_string(hdr.data_offset) +
                ") overlaps with name table section (ends at " + std::to_string(name_table_end) + ")"
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
                    "Truncated read while reading directory entry " + std::to_string(i)
                });
                break;
            }
            out.push_back(de);
        }
        return out;
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
                    "Resource entry " + std::to_string(i) + " has out-of-bounds name offset: " + std::to_string(
                        name_offset)
                });
                continue;
            }

            if (!is.seekg(name_offset, std::ios::beg)) {
                result.diagnostics.push_back({
                    diagnostic::severity_level::warning,
                    "Failed to seek to name offset " + std::to_string(name_offset) + " for entry " + std::to_string(i)
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
                    "Resource entry " + std::to_string(i) + " has out-of-bounds data range: start=" +
                    std::to_string(res_start) + ", size=" + std::to_string(res_size)
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
                    "Truncated stream while reading resource name for entry " + std::to_string(i)
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

    namespace {
        // Read a resource's raw bytes from the archive stream.
        std::vector <std::uint8_t> read_resource(std::istream& is, const resource& r) {
            std::vector <std::uint8_t> data(r.size);
            is.seekg(r.offset, std::ios::beg);
            is.read(reinterpret_cast <std::istream::char_type*>(data.data()), r.size);
            return data;
        }

        std::optional <sdlpp::palette> palette_from_colors(std::span <const sdlpp::color> colors) {
            if (colors.size() < 256) {
                return std::nullopt;
            }

            auto pal = sdlpp::palette::create(256);
            if (!pal) {
                return std::nullopt;
            }
            if (!pal->set_colors(colors.first(256))) {
                return std::nullopt;
            }
            return std::move(*pal);
        }

        std::optional <sdlpp::palette> palette_from_rgb_bytes(std::span <const std::uint8_t> rgb) {
            if (rgb.size() < 256 * 3) {
                return std::nullopt;
            }

            std::vector <sdlpp::color> colors;
            colors.reserve(256);
            for (std::size_t i = 0; i < 256; ++i) {
                colors.emplace_back(
                    rgb[i * 3 + 0],
                    rgb[i * 3 + 1],
                    rgb[i * 3 + 2],
                    255);
            }
            return palette_from_colors(colors);
        }
    }

    std::optional <game_resources> parse(std::istream& is) {
        auto result = load_resource(is);
        if (result.has_errors()) {
            for (const auto& diag : result.diagnostics) {
                if (diag.severity == diagnostic::severity_level::error) {
                    LOG_ERROR(diag.message);
                } else {
                    LOG_WARN(diag.message);
                }
            }
            return std::nullopt;
        }

        game_resources gr;
        std::map <std::string, sdlpp::palette> pals;
        for (const auto& r : result.resources) {
            if (r.name.find(".gif") != std::string::npos) {
                auto data = read_resource(is, r);
                gr.backdrops.emplace(r.name, neutrino::cpu_texture_atlas(neutrino::load_image(data)));
            }
            if (r.name.find(".pal") != std::string::npos) {
                auto data = read_resource(is, r);
                if (auto pal = palette_from_rgb_bytes(data)) {
                    pals.emplace(r.name, std::move(*pal));
                }
            }
        }

        for (const auto& r : result.resources) {
            if (auto idx = r.name.find(".bob"); idx != std::string::npos) {
                auto data = read_resource(is, r);
                bob_image bob;

                if (std::string err; !bob.parse(data, err)) {
                    LOG_ERROR("Error while parsing", r.name, ":", err);
                    return std::nullopt;
                }
                tile_sheet_def tl_def;

                sdlpp::image::sdl_surface_adapter adaptor(tl_def.image);
                std::string err;
                auto base = r.name.substr(0, idx);
                bool bob_rc = false;

                if (auto itr = pals.find(base + ".pal"); itr != pals.end()) {
                    // 1. sibling <base>.pal
                    bob_rc = bob.decode_atlas(itr->second, adaptor, err, tl_def.source_rects);
                } else {
                    if (auto itr_gif = gr.backdrops.find(base + ".gif"); itr_gif != gr.backdrops.end()) {
                        // 2. sibling <base>.gif global colour table (the GIF decoder yields indexed8 + palette)
                        const auto& gif_srf = itr_gif->second.surface();
                        if (gif_srf.format() == sdlpp::pixel_format_enum::INDEX8) {
                            bob_rc = bob.decode_atlas(gif_srf.get_palette(), adaptor, err, tl_def.source_rects);
                        }
                    } else {
                        if (auto itr_play = pals.find("ke_play.pal"); itr_play != pals.end()) {
                            // 3. gameplay palette
                            bob_rc = bob.decode_atlas(itr_play->second, adaptor, err, tl_def.source_rects);
                        } else {
                            bob_rc = bob.decode_atlas(bob_grayscale_palette(), adaptor, err, tl_def.source_rects);
                        }
                    }
                }
                if (!bob_rc) {
                    LOG_ERROR("Error while decoding", r.name, ":", err);
                    return std::nullopt;
                }
                tl_def.origins.reserve(bob.frame_count());
                for (std::size_t i = 0; i < bob.frame_count(); ++i) {
                    const bob_frame& f = bob.frame(i);
                    tl_def.origins.emplace_back(f.x_off_a, f.y_off_a);
                }

                gr.tile_sheets.emplace(base, std::move(tl_def));
            }
        }
        for (const auto& r : result.resources) {
            if (r.name.find(".tab") != std::string::npos) {
                const auto data = read_resource(is, r);
                tab_levels tab;
                std::string err;
                if (!tab.parse(data, err)) {
                    LOG_ERROR("Error while parsing TAB in ", r.name, ":", err);
                    return std::nullopt;
                }
                gr.levels.reserve(tab.levels.size());
                for (const auto& lvl : tab.levels) {
                    gr.levels.emplace_back(ke_level::decode(lvl));
                }
            }
        }
        LOG_DEBUG("Loaded ", gr.backdrops.size(), "backdrops");
        LOG_DEBUG("Loaded ", pals.size(), "palettes");
        return gr;
    }
}
