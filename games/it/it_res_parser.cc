//
// Created by igor on 30/09/2026.
//

#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <string_view>
#include <stdexcept>
#include <map>
#include <iomanip>

#include "binary_reader.hh"
#include "lzss.hh"
#include "it_blocks.hh"

struct res_location {
    int res_id;
    std::streamoff offset;
    std::size_t size;
};

static std::vector <res_location> parse_resource_locations(std::istream& is) {
    binary_reader rdr(is);

    static constexpr auto FOOTER_SIZE = 12;
    // Read footer
    rdr.seek(-FOOTER_SIZE, std::ios::end);

    uint32_t max_char_count;
    uint8_t padding[3];
    uint8_t version;
    uint8_t magic[4];
    rdr >> max_char_count >> padding >> version >> magic;

    if (std::string_view(reinterpret_cast <const char*>(magic), 4) != "=VS=") {
        throw std::runtime_error("Bad magic signature (expected '=VS=')");
    }

    if (version != 4) {
        throw std::runtime_error("Unsupported resource format version: " + std::to_string(version));
    }

    const auto metadata_offset = -static_cast <std::streamoff>(FOOTER_SIZE + max_char_count);
    rdr.seek(metadata_offset, std::ios::end);

    uint32_t num_resources = 0;
    uint32_t base_offset = 0;
    rdr >> num_resources >> base_offset;

    if (num_resources == 0) {
        return {};
    }

    const auto entry_table_size = static_cast <std::streamoff>(5) * num_resources;

    // Seek to the start of the resource entry table
    rdr.seek(metadata_offset - entry_table_size, std::ios::end);
    const auto entry_table_start = rdr.tell();

    std::vector <res_location> rc;
    rc.reserve(num_resources);

    for (uint32_t i = 0; i < num_resources; i++) {
        uint32_t res_offset;
        uint8_t res_id;
        rdr >> res_offset >> res_id;

        res_location dt{};
        dt.res_id = static_cast <int>(res_id);
        dt.offset = static_cast <std::streamoff>(base_offset) + res_offset;
        rc.push_back(dt);
    }

    std::sort(rc.begin(), rc.end(), [](const auto& a, const auto& b) {
        return a.offset < b.offset;
    });

    for (std::size_t i = 0; i + 1 < rc.size(); ++i) {
        rc[i].size = static_cast <std::size_t>(rc[i + 1].offset - rc[i].offset);
    }

    if (entry_table_start < rc.back().offset) {
        throw std::runtime_error("Malformed resource table: resource offset exceeds entry table boundary");
    }
    rc.back().size = static_cast <std::size_t>(entry_table_start - rc.back().offset);

    return rc;
}

static int32_t var_decode(binary_reader& br) {
    static constexpr int32_t dword_41020C[] = {
        -67372032, // 0xfc000000
        -263169, // 0xfffcffff
        128, // 0x00000080
        1151, // 0x0000047f
        1152, // 0x00000480
        263295, // 0x0004047f
        263296, // 0x00040480
        67372159, // 0x0404047f
        0 // fallback
    };
    int8_t c;
    br >> c;
    if (c >= 0) {
        return c;
    }

    uint32_t bytes_to_read = 0;
    if (c & 0x20) {
        bytes_to_read = (((static_cast <uint8_t>(c)) >> 2) & 3) + 1;
    } else {
        bytes_to_read = c & 0x1F;
    }

    uint32_t val = 0;
    for (uint32_t i = 0; i < bytes_to_read; ++i) {
        uint8_t x;
        br >> x;
        val |= (static_cast <uint32_t>(x) << (8 * i));
    }

    if (c & 0x20) {
        if (bytes_to_read < 4) {
            val += (static_cast <uint32_t>(c) & 3u) << (8 * bytes_to_read);
        }

        auto sval = static_cast <int32_t>(val);
        if (c & 0x10) {
            sval = dword_41020C[2 * bytes_to_read - 1] - sval;
        } else {
            sval += dword_41020C[2 * bytes_to_read];
        }
        return sval;
    }

    return static_cast <int32_t>(val);
}


static std::vector<uint8_t> read_command_block(binary_reader& rdr) {
    int8_t control_signed;
    rdr >> control_signed;
    if (control_signed >= 0) {
        throw std::runtime_error("Invalid command block control byte");
    }

    const auto control = static_cast<uint8_t>(control_signed);
    if (control & 0x40) {
        // Uncompressed block with 5-bit size
        const std::size_t size = control & 0x1F;
        std::vector<uint8_t> data(size);
        if (size > 0) {
            rdr.read(data.data(), size);
        }
        return data;
    } else {
        const std::size_t max_char_count = (control & 3) + 1;
        uint32_t comp_size = 0;
        for (std::size_t i = 0; i < max_char_count; ++i) {
            uint8_t b;
            rdr >> b;
            comp_size |= (static_cast<uint32_t>(b) << (8 * i));
        }

        if (control & 0x0C) {
            // Compressed block (LZSS)
            uint32_t decomp_size = 0;
            for (std::size_t i = 0; i < max_char_count; ++i) {
                uint8_t b;
                rdr >> b;
                decomp_size |= (static_cast<uint32_t>(b) << (8 * i));
            }

            std::vector<uint8_t> comp_data(comp_size);
            if (comp_size > 0) {
                rdr.read(comp_data.data(), comp_size);
            }
            return decompress_lzss(comp_data, decomp_size);
        } else {
            // Uncompressed block with multi-byte size
            std::vector<uint8_t> data(comp_size);
            if (comp_size > 0) {
                rdr.read(data.data(), comp_size);
            }
            return data;
        }
    }
}

static std::vector<uint8_t> read_animation_stream_block(binary_reader& rdr) {
    uint8_t prefix;
    rdr >> prefix;
    if ((prefix & 0xEC) != 0xEC) {
        throw std::runtime_error("Invalid animation stream header prefix");
    }
    const std::size_t offset_len = (prefix & 3) + 1;
    uint32_t size = 0;
    for (std::size_t i = 0; i < offset_len; ++i) {
        uint8_t b;
        rdr >> b;
        size |= (static_cast<uint32_t>(b) << (8 * i));
    }
    std::vector<uint8_t> raw_data(size);
    if (size > 0) {
        rdr.read(raw_data.data(), size);
    }
    return raw_data;
}

static std::vector<addressed_block> parse_resource_stream(binary_reader& rdr, std::size_t max_size = 0) {
    std::vector<addressed_block> blocks;

    uint32_t slot_id = 0;
    uint32_t column_id = 0;
    uint32_t direction_id = 0;
    uint32_t base_offset = 0;
    int block_type_id = 0;

    const auto start_pos = rdr.tell();
    const auto end_pos = (max_size > 0) ? (start_pos + static_cast<std::streamoff>(max_size)) : std::streampos(0);

    uint8_t raw_tag = 0;
    rdr >> raw_tag;

    while (raw_tag != static_cast<uint8_t>(stream_opcode::end_of_stream)) {
        if (max_size > 0 && rdr.tell() >= end_pos) {
            break;
        }

        switch (static_cast<stream_opcode>(raw_tag)) {
            case stream_opcode::param_column_index:
                column_id = static_cast<uint32_t>(var_decode(rdr));
                rdr >> raw_tag;
                break;

            case stream_opcode::param_direction_index:
                direction_id = static_cast<uint32_t>(var_decode(rdr));
                rdr >> raw_tag;
                break;

            case stream_opcode::param_base_offset:
                base_offset = static_cast<uint32_t>(var_decode(rdr));
                rdr >> raw_tag;
                break;

            case stream_opcode::command_block: {
                auto data = read_command_block(rdr);
                const block_address addr{
                    .slot_id = slot_id,
                    .column_id = column_id,
                    .direction_id = direction_id,
                    .base_offset = base_offset
                };
                blocks.emplace_back(addr, decode_block(block_type_id, data, false));
                rdr >> raw_tag;
                break;
            }

            case stream_opcode::animation_stream: {
                auto raw_data = read_animation_stream_block(rdr);
                const block_address addr{
                    .slot_id = slot_id,
                    .column_id = column_id,
                    .direction_id = direction_id,
                    .base_offset = base_offset
                };
                blocks.emplace_back(addr, decode_block(block_type_id, raw_data, true));
                rdr >> raw_tag;
                break;
            }

            case stream_opcode::block_terminator:
                rdr >> raw_tag;
                break;

            default:
                // New block declaration: reset addressing registers (IT.c:4701-4705)
                base_offset = 0;
                direction_id = 0;
                column_id = 0;
                slot_id = 0;

                block_type_id = raw_tag & stream_masks::TYPE_MASK;
                if (raw_tag & stream_masks::NO_SLOT_ARG_FLAG) {
                    rdr >> raw_tag;
                } else {
                    slot_id = static_cast<uint32_t>(var_decode(rdr));
                    rdr >> raw_tag;
                }
                break;
        }
    }

    return blocks;
}

int main(int argc, char* argv[]) {
    const char* path = argc > 1 ? argv[1] : "/home/igor/games/it/IT.EXE";

    std::ifstream ifs(path, std::ios::binary | std::ios::in);
    if (!ifs) {
        std::cerr << "Failed to open " << path << std::endl;
        return 1;
    }

    try {
        const auto resources = parse_resource_locations(ifs);
        std::cout << "Successfully parsed " << resources.size() << " resources from " << path << ":\n";
        binary_reader rdr(ifs);
        for (const auto& res : resources) {
            std::cout << "  Resource " << res.res_id
                << ": offset = " << res.offset
                << ", size = " << res.size << " bytes\n";
             // 1. Bulk-read the whole resource into memory (one read per resource)
            rdr.seek(res.offset);
            std::vector <uint8_t> res_bytes(res.size);
            rdr.read(res_bytes.data(), res_bytes.size());

            // 2. Wrap it in a zero-copy stream
            span_reader mem_rdr(res_bytes);

            // 3. Parse with binary_reader exactly the same way!
            const auto blocks = parse_resource_stream(mem_rdr, res.size);
            std::cout << "    -> Extracted and decoded " << blocks.size() << " blocks\n";

            std::map<std::string, std::size_t> type_counts;
            for (const auto& [addr, blk] : blocks) {
                std::visit([&]<typename T0>(const T0& b) {
                    using T = std::decay_t<T0>;
                    if constexpr (std::is_same_v<T, system_allocation_block>) {
                        type_counts["system_allocation_block (16)"]++;
                    } else if constexpr (std::is_same_v<T, level_config_block>) {
                        type_counts["level_config_block (15)"]++;
                    } else if constexpr (std::is_same_v<T, sizing_metadata_block>) {
                        type_counts["sizing_metadata_block (17)"]++;
                    } else if constexpr (std::is_same_v<T, menu_config_block>) {
                        type_counts["menu_config_block (18)"]++;
                    } else if constexpr (std::is_same_v<T, script_bytecode_block>) {
                        type_counts["script_bytecode_block (19/20/33/43/44)"]++;
                    } else if constexpr (std::is_same_v<T, animation_stream_block>) {
                        type_counts["animation_stream_block (Tag 0x05)"]++;
                    } else if constexpr (std::is_same_v<T, background_image_block>) {
                        type_counts["background_image_block (24)"]++;
                    } else if constexpr (std::is_same_v<T, level_scrambling_rules_block>) {
                        type_counts["level_scrambling_rules_block (25)"]++;
                    } else if constexpr (std::is_same_v<T, level_layout_block>) {
                        type_counts["level_layout_block (32)"]++;
                    } else if constexpr (std::is_same_v<T, level_collision_override_block>) {
                        type_counts["level_collision_override_block (34)"]++;
                    } else if constexpr (std::is_same_v<T, entity_rule_index_block>) {
                        type_counts["entity_rule_index_block (35)"]++;
                    } else if constexpr (std::is_same_v<T, entity_behavior_rule_block>) {
                        type_counts["entity_behavior_rule_block (42)"]++;
                    } else if constexpr (std::is_same_v<T, collision_walkability_matrix_block>) {
                        type_counts["collision_walkability_matrix_block (56)"]++;
                    } else if constexpr (std::is_same_v<T, tile_candidate_list_block>) {
                        type_counts["tile_candidate_list_block (57)"]++;
                    } else if constexpr (std::is_same_v<T, tile_rotations_block>) {
                        type_counts["tile_rotations_block (58)"]++;
                    } else if constexpr (std::is_same_v<T, sprite_metadata_block>) {
                        type_counts["sprite_metadata_block (64)"]++;
                    } else if constexpr (std::is_same_v<T, sprite_frame_table_block>) {
                        type_counts["sprite_frame_table_block (66)"]++;
                    } else if constexpr (std::is_same_v<T, sprite_graphic_block>) {
                        type_counts["sprite_graphic_block (67)"]++;
                    } else if constexpr (std::is_same_v<T, audio_settings_block>) {
                        type_counts["audio_settings_block (80)"]++;
                    } else if constexpr (std::is_same_v<T, digital_audio_clip_block>) {
                        type_counts["digital_audio_clip_block (81)"]++;
                    } else if constexpr (std::is_same_v<T, music_track_block>) {
                        type_counts["music_track_block (82)"]++;
                    } else if constexpr (std::is_same_v<T, ui_overlay_action_block>) {
                        type_counts["ui_overlay_action_block (96)"]++;
                    } else if constexpr (std::is_same_v<T, ui_overlay_layout_block>) {
                        type_counts["ui_overlay_layout_block (97)"]++;
                    } else if constexpr (std::is_same_v<T, save_game_layout_block>) {
                        type_counts["save_game_layout_block (124/125/126)"]++;
                    } else {
                        type_counts["generic_asset_block (unknown)"]++;
                    }
                }, blk);
            }
            for (const auto& [name, count] : type_counts) {
                std::cout << "       - " << name << ": " << count << "\n";
            }

            for (const auto& [addr, blk] : blocks) {
                if (std::holds_alternative<animation_stream_block>(blk)) {
                    const auto& anim = std::get<animation_stream_block>(blk);
                    std::size_t configs = 0, keyframes = 0, deltas = 0, holds = 0, palettes = 0, audios = 0;
                    for (const auto& r : anim.records) {
                        if (r.is_config()) configs++;
                        else if (r.is_keyframe()) keyframes++;
                        else if (r.is_delta_frame()) deltas++;
                        else if (r.is_hold_frame()) holds++;
                        else if (r.is_palette()) palettes++;
                        else if (r.is_audio()) audios++;
                    }
                    std::cout << "       [Animation Stream Details " << addr.to_string() << "] " << anim.records.size() << " timeline records:\n"
                              << "         * Config / Timing records: " << configs << "\n"
                              << "         * Video Keyframes:         " << keyframes << "\n"
                              << "         * Video Delta frames:      " << deltas << "\n"
                              << "         * Video Hold frames:       " << holds << "\n"
                              << "         * 256-color Palettes:      " << palettes << "\n"
                              << "         * Audio Chunks:            " << audios << "\n";
                } else if (std::holds_alternative<script_bytecode_block>(blk)) {
                    static bool printed_sample = false;
                    const auto& scr = std::get<script_bytecode_block>(blk);
                    if (!printed_sample && scr.bytecode.size() > 20 && std::any_of(scr.bytecode.begin(), scr.bytecode.end(), [](uint8_t b) { return b != 0; })) {
                        printed_sample = true;
                        std::cout << "\n       [Sample Script Disassembly (Type " << scr.type_id << ", "
                                  << scr.bytecode.size() << " bytes) " << addr.to_string() << "]:\n";
                        std::cout << scr.disassemble() << "\n";
                    }
                } else if (std::holds_alternative<level_scrambling_rules_block>(blk)) {
                    // rules printed
                } else if (std::holds_alternative<level_layout_block>(blk)) {
                    static int lay_idx = 0;
                    const auto& lay = std::get<level_layout_block>(blk);
                    std::cout << "       [Type 32 Level Layout #" << ++lay_idx << " " << addr.to_string() << "]: "
                              << lay.to_string() << "\n";
                } else if (std::holds_alternative<entity_rule_index_block>(blk)) {
                    const auto& spw_hdr = std::get<entity_rule_index_block>(blk);
                    std::cout << "       [Type 35 Entity Rule Index " << addr.to_string() << ": " << spw_hdr.offsets.size() << " offsets]: ";
                    for (auto off : spw_hdr.offsets) std::cout << "0x" << std::hex << off << " ";
                    std::cout << std::dec << "\n";
                } else if (std::holds_alternative<entity_behavior_rule_block>(blk)) {
                    static int ent_idx = 0;
                    const auto& ent = std::get<entity_behavior_rule_block>(blk);
                    std::cout << "       [Type 42 Entity Behavior Rules #" << ++ent_idx << " " << addr.to_string() << " (" << ent.rules.size() << " words)]: "
                              << "Header=0x" << std::hex << ent.header.raw_flags << std::dec
                              << ", CondGroups=" << ent.condition_groups.size()
                              << ", ActGroups=" << ent.action_groups.size();
                    if (!ent.action_groups.empty()) {
                        for (const auto& grp : ent.action_groups) {
                            for (const auto& instr : grp.instructions) {
                                std::cout << " | " << instr.to_string(false);
                            }
                        }
                    }
                    std::cout << "\n";
                } else if (std::holds_alternative<tile_rotations_block>(blk)) {
                    // rotations printed
                } else if (std::holds_alternative<sprite_metadata_block>(blk)) {
                    static int spr_idx = 0;
                    const auto& spr = std::get<sprite_metadata_block>(blk);
                    std::cout << "       [Type 64 Sprite Meta #" << ++spr_idx << " " << addr.to_string() << "]: "
                              << spr.to_string() << "\n";
                } else if (std::holds_alternative<audio_settings_block>(blk)) {
                    const auto& a = std::get<audio_settings_block>(blk);
                    std::cout << "       [Type 80 Audio Settings " << addr.to_string() << "]: " << a.to_string() << "\n";
                } else if (std::holds_alternative<digital_audio_clip_block>(blk)) {
                    static int snd_idx = 0;
                    const auto& snd = std::get<digital_audio_clip_block>(blk);
                    const auto wav = snd.to_wav();
                    std::cout << "       [Type 81 Digital Audio #" << ++snd_idx << " " << addr.to_string() << "]: "
                              << snd.pcm_samples.size() << " PCM samples (11025 Hz mono, "
                              << std::fixed << std::setprecision(2) << (snd.pcm_samples.size() / 11.025) << " ms), "
                              << "RIFF WAV container: " << wav.size() << " bytes\n" << std::defaultfloat;
                } else if (std::holds_alternative<music_track_block>(blk)) {
                    static int mus_idx = 0;
                    const auto& mus = std::get<music_track_block>(blk);
                    const auto smf = mus.to_mid();
                    std::cout << "       [Type 82 Music Track #" << ++mus_idx << " " << addr.to_string() << "]: "
                              << mus.midi_stream.size() << " bytes custom MIDI, Standard MIDI File (SMF): "
                              << smf.size() << " bytes\n";
                } else if (std::holds_alternative<ui_overlay_action_block>(blk)) {
                    const auto& d = std::get<ui_overlay_action_block>(blk);
                    std::cout << "       [Type 96 UI Overlay Action " << addr.to_string() << "]: " << d.to_string() << "\n";
                } else if (std::holds_alternative<ui_overlay_layout_block>(blk)) {
                    static int d_idx = 0;
                    const auto& d = std::get<ui_overlay_layout_block>(blk);
                    std::cout << "       [Type 97 UI Overlay Layout #" << ++d_idx << " " << addr.to_string() << " (" << d.elements.size() << " elements)]: ";
                    for (const auto& el : d.elements) {
                        std::cout << el.to_string() << " ";
                    }
                    std::cout << "\n";
                } else if (std::holds_alternative<save_game_layout_block>(blk)) {
                    const auto& sav = std::get<save_game_layout_block>(blk);
                    std::cout << "       [" << sav.to_string() << "] " << addr.to_string() << "\n";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing resources: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
