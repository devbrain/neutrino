//
// Created by igor on 30/09/2026.
//

#include "it_blocks.hh"
#include "lzss.hh"
#include <algorithm>
#include <sstream>
#include <iomanip>

static void parse_rule_block_words(
    std::span<const uint32_t> rules,
    scramble_header_flags& header,
    uint32_t& precondition_script_offset,
    std::vector<scramble_rule_group>& condition_groups,
    uint32_t& postcondition_script_offset,
    std::vector<scramble_rule_group>& action_groups)
{
    if (rules.empty()) return;
    std::size_t idx = 0;
    const uint32_t h = rules[idx++];
    header.raw_flags = h;
    header.has_precondition_script  = (h & 0x01) != 0;
    header.has_condition_groups     = (h & 0x02) != 0;
    header.randomize_orientation    = (h & 0x04) != 0;
    header.has_postcondition_script = (h & 0x08) != 0;
    header.has_action_groups        = (h & 0x10) != 0;
    header.extra_flags              = static_cast<uint8_t>((h >> 8) & 0xFF);

    if (header.has_precondition_script && idx < rules.size()) {
        precondition_script_offset = rules[idx++];
    }

    if (header.has_condition_groups) {
        while (idx < rules.size()) {
            const uint32_t gh = rules[idx++];
            scramble_rule_group grp;
            grp.header_word = gh;
            grp.count = static_cast<uint16_t>(gh & 0xFFFF);
            grp.is_terminal = (gh & 0x10000) != 0;
            const auto m = static_cast<uint8_t>((gh >> 24) & 0xFF);
            grp.mode = m <= 4 ? static_cast<scramble_group_mode>(m) : scramble_group_mode::generic;

            for (uint16_t i = 0; i < grp.count && idx < rules.size(); ++i) {
                const uint32_t w = rules[idx++];
                scramble_rule_instruction instr;
                instr.raw_word = w;
                instr.y = static_cast<int8_t>((w >> 24) & 0xFF);
                instr.x = static_cast<int8_t>((w >> 16) & 0xFF);
                instr.transform_mode = static_cast<uint8_t>((w >> 12) & 0x0F);
                const auto comb = static_cast<uint8_t>((w >> 10) & 0x03);
                instr.combinator = static_cast<scramble_combinator>(comb);
                instr.invert_match = (w & 0x200) != 0;
                instr.is_class_check = (w & 0x100) != 0;
                instr.target_id = static_cast<uint8_t>(w & 0xFF);
                grp.instructions.push_back(instr);
            }
            condition_groups.push_back(std::move(grp));
            if (grp.is_terminal) {
                break;
            }
        }
    }

    if (header.has_postcondition_script && idx < rules.size()) {
        postcondition_script_offset = rules[idx++];
    }

    if (header.has_action_groups) {
        while (idx < rules.size()) {
            const uint32_t gh = rules[idx++];
            scramble_rule_group grp;
            grp.header_word = gh;
            grp.count = static_cast<uint16_t>(gh & 0xFFFF);
            grp.is_terminal = (gh & 0x10000) != 0;
            const auto m = static_cast<uint8_t>((gh >> 24) & 0xFF);
            grp.mode = m <= 4 ? static_cast<scramble_group_mode>(m) : scramble_group_mode::generic;

            for (uint16_t i = 0; i < grp.count && idx < rules.size(); ++i) {
                const uint32_t w = rules[idx++];
                scramble_rule_instruction instr;
                instr.raw_word = w;
                instr.y = static_cast<int8_t>((w >> 24) & 0xFF);
                instr.x = static_cast<int8_t>((w >> 16) & 0xFF);
                instr.transform_mode = static_cast<uint8_t>((w >> 12) & 0x0F);
                const auto comb = static_cast<uint8_t>((w >> 10) & 0x03);
                instr.combinator = static_cast<scramble_combinator>(comb);
                instr.invert_match = (w & 0x200) != 0;
                instr.is_class_check = (w & 0x100) != 0;
                instr.target_id = static_cast<uint8_t>(w & 0xFF);
                grp.instructions.push_back(instr);
            }
            action_groups.push_back(std::move(grp));
            if (grp.is_terminal) {
                break;
            }
        }
    }
}

decoded_block decode_block(int type_id, binary_reader& rdr, std::size_t payload_size, bool is_anim_stream) {
    if (is_anim_stream) {
        animation_stream_block blk;
        std::size_t bytes_read = 0;
        while (bytes_read + 12 <= payload_size && !rdr.eof()) {
            anim_record rec;
            uint8_t raw_type = 0;
            rdr >> raw_type >> rec.subtype >> rec.flag1 >> rec.flag2
                >> rec.comp_size >> rec.decomp_size;
            bytes_read += 12;
            rec.type = static_cast<anim_record_type>(raw_type);

            const bool has_payload = (raw_type >= 1 && raw_type <= 4 && rec.comp_size > 0);
            if (has_payload) {
                const std::size_t read_bytes = std::min<std::size_t>(rec.comp_size, payload_size - bytes_read);
                std::vector<uint8_t> comp_bytes(read_bytes);
                if (read_bytes > 0) {
                    rdr.read(comp_bytes.data(), read_bytes);
                }
                bytes_read += read_bytes;

                if (rec.comp_size != rec.decomp_size && rec.decomp_size > 0) {
                    rec.payload = decompress_lzss(comp_bytes, rec.decomp_size);
                } else {
                    rec.payload = std::move(comp_bytes);
                }
            }
            blk.records.push_back(std::move(rec));
        }
        return blk;
    }

    try {
        switch (type_id) {
            case 15: {
                level_config_block blk;
                rdr >> blk.level_suffix >> blk.config_registers;
                return blk;
            }

            case 16: {
                system_allocation_block blk;
                rdr >> blk.magic
                    >> blk.decomp_multiplier
                    >> blk.graphic_heap_size
                    >> blk.screen_width
                    >> blk.screen_height
                    >> blk.alloc_var1
                    >> blk.alloc_var2
                    >> blk.alloc_var3
                    >> blk.misc_flags
                    >> blk.reserved
                    >> blk.title_cp1251;
                return blk;
            }

            case 17: {
                sizing_metadata_block blk;
                rdr >> blk.global_mode
                    >> blk.col_div
                    >> blk.grid_cols
                    >> blk.grid_rows
                    >> blk.screen_width
                    >> blk.screen_height
                    >> blk.board_alloc_size
                    >> blk.max_zones
                    >> blk.max_sprites
                    >> blk.max_entities
                    >> blk.max_music
                    >> blk.max_ui_pages
                    >> blk.flag_count;
                return blk;
            }

            case 18: {
                menu_config_block blk;
                rdr >> blk.menu_state0 >> blk.menu_state1;
                return blk;
            }

            case 19:
            case 20:
            case 33:
            case 43:
            case 44: {
                script_bytecode_block blk;
                blk.type_id = type_id;
                blk.bytecode.resize(payload_size);
                if (payload_size > 0) {
                    rdr.read(blk.bytecode.data(), payload_size);
                }
                return blk;
            }

            case 24: {
                background_image_block blk;
                rdr >> blk.reserved0
                    >> blk.reserved1
                    >> blk.width
                    >> blk.height
                    >> blk.pixel_size
                    >> blk.total_size;

                if (payload_size >= 24 + 768) {
                    const std::size_t available_pixels = payload_size - 24 - 768;
                    const std::size_t actual_pixel_size = std::min<std::size_t>(blk.pixel_size, available_pixels);
                    blk.pixels.resize(actual_pixel_size);
                    if (actual_pixel_size > 0) {
                        rdr.read(blk.pixels.data(), actual_pixel_size);
                    }
                    if (blk.pixel_size > actual_pixel_size) {
                        rdr.seek(static_cast<std::streamoff>(blk.pixel_size - actual_pixel_size), std::ios_base::cur);
                    }
                    rdr >> blk.palette;
                }
                return blk;
            }

            case 25: {
                level_scrambling_rules_block blk;
                blk.rules.resize(payload_size / 4);
                rdr >> blk.rules;
                parse_rule_block_words(blk.rules, blk.header, blk.precondition_script_offset,
                                       blk.condition_groups, blk.postcondition_script_offset,
                                       blk.action_groups);
                return blk;
            }

            case 32: {
                level_layout_block blk;
                rdr >> blk.layout_type >> blk.symmetry_mask >> blk.behavior_flags >> blk.state_size;
                return blk;
            }

            case 34: {
                level_collision_override_block blk;
                blk.collision_map.resize(payload_size);
                if (payload_size > 0) {
                    rdr.read(blk.collision_map.data(), payload_size);
                }
                return blk;
            }

            case 35: {
                entity_rule_index_block blk;
                blk.offsets.resize(payload_size / 4);
                rdr >> blk.offsets;
                return blk;
            }

            case 42: {
                entity_behavior_rule_block blk;
                blk.rules.resize(payload_size / 4);
                rdr >> blk.rules;
                parse_rule_block_words(blk.rules, blk.header, blk.precondition_script_offset,
                                       blk.condition_groups, blk.postcondition_script_offset,
                                       blk.action_groups);
                return blk;
            }

            case 56: {
                collision_walkability_matrix_block blk;
                rdr >> blk.matrix;
                return blk;
            }

            case 57: {
                tile_candidate_list_block blk;
                if (payload_size > 0) {
                    uint8_t count = 0;
                    rdr >> count;
                    const std::size_t to_read = std::min<std::size_t>(count, payload_size - 1);
                    blk.tile_ids.resize(to_read);
                    if (to_read > 0) {
                        rdr.read(blk.tile_ids.data(), to_read);
                    }
                }
                return blk;
            }

            case 58: {
                tile_rotations_block blk;
                blk.rotation_flags.resize(payload_size);
                if (payload_size > 0) {
                    rdr.read(blk.rotation_flags.data(), payload_size);
                }
                return blk;
            }

            case 64: {
                sprite_metadata_block blk;
                rdr >> blk.flags >> blk.flags_high >> blk.column_count >> blk.direction_count;
                return blk;
            }

            case 66: {
                sprite_frame_table_block blk;
                const std::size_t count = payload_size / 8;
                blk.frames.resize(count);
                for (auto& frame : blk.frames) {
                    rdr >> frame.block_ptr >> frame.x_hotspot >> frame.y_hotspot;
                }
                return blk;
            }

            case 67: {
                sprite_graphic_block blk;
                rdr >> blk.width >> blk.height;
                if (payload_size >= 4) {
                    const std::size_t pixel_bytes = payload_size - 4;
                    blk.pixels.resize(pixel_bytes);
                    if (pixel_bytes > 0) {
                        rdr.read(blk.pixels.data(), pixel_bytes);
                    }
                }
                return blk;
            }

            case 80: {
                audio_settings_block blk;
                rdr >> blk.priority >> blk.volume >> blk.pan >> blk.flags;
                blk.settings = static_cast<uint32_t>(blk.priority)
                             | (static_cast<uint32_t>(blk.volume) << 8)
                             | (static_cast<uint32_t>(blk.pan) << 16)
                             | (static_cast<uint32_t>(blk.flags) << 24);
                return blk;
            }

            case 81: {
                digital_audio_clip_block blk;
                uint32_t pcm_size = 0;
                rdr >> pcm_size;
                const std::size_t actual_size = (payload_size >= 4) ? std::min<std::size_t>(pcm_size, payload_size - 4) : pcm_size;
                blk.pcm_samples.resize(actual_size);
                if (actual_size > 0) {
                    rdr.read(blk.pcm_samples.data(), actual_size);
                }
                return blk;
            }

            case 82: {
                music_track_block blk;
                uint32_t midi_size = 0;
                rdr >> midi_size;
                const std::size_t actual_size = (payload_size >= 4) ? std::min<std::size_t>(midi_size, payload_size - 4) : midi_size;
                blk.midi_stream.resize(actual_size);
                if (actual_size > 0) {
                    rdr.read(blk.midi_stream.data(), actual_size);
                }
                return blk;
            }

            case 96: {
                ui_overlay_action_block blk;
                rdr >> blk.action_rule_word;
                blk.descriptor = blk.action_rule_word;
                return blk;
            }

            case 97: {
                ui_overlay_layout_block blk;
                const std::size_t count = payload_size / 8;
                blk.elements.reserve(count);
                for (std::size_t i = 0; i < count; ++i) {
                    ui_overlay_element el;
                    uint8_t term = 0;
                    rdr >> el.x >> el.y >> el.sprite_index >> el.flags >> term;
                    el.is_terminal = (term != 0);
                    blk.elements.push_back(el);
                }
                return blk;
            }

            case 124:
            case 125:
            case 126: {
                save_game_layout_block blk;
                blk.type_id = type_id;
                uint32_t count = 0;
                rdr >> count;
                blk.segments.reserve(count);
                for (std::size_t i = 0; i < count; ++i) {
                    save_state_segment seg;
                    rdr >> seg.state_offset >> seg.state_size;
                    blk.segments.push_back(seg);
                }
                return blk;
            }

            default:
                break;
        }
    } catch (const binary_reader_error&) {
        // Fall back to generic_asset_block if structured read fails or stream ends prematurely
    }

    generic_asset_block blk;
    blk.type_id = type_id;
    blk.raw_data.resize(payload_size);
    if (payload_size > 0) {
        try {
            rdr.seek(0, std::ios_base::beg);
            rdr.read(blk.raw_data.data(), payload_size);
        } catch (...) {}
    }
    return blk;
}

decoded_block decode_block(int type_id, std::span<const uint8_t> data, bool is_anim_stream) {
    span_reader rdr(data);
    auto blk = decode_block(type_id, rdr, data.size(), is_anim_stream);
    // If generic fallback occurred and raw_data is empty, assign it from span
    if (std::holds_alternative<generic_asset_block>(blk)) {
        auto& g = std::get<generic_asset_block>(blk);
        if (g.raw_data.empty() && !data.empty()) {
            g.raw_data.assign(data.begin(), data.end());
        }
    }
    return blk;
}

std::string scramble_rule_instruction::to_string(bool is_condition_context) const {
    std::ostringstream oss;
    oss << "(X: " << std::setw(3) << static_cast<int>(x)
        << ", Y: " << std::setw(3) << static_cast<int>(y) << ") -> ";

    if (is_condition_context) {
        if (invert_match) oss << "NOT ";
        if (is_class_check) {
            oss << "Class 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<int>(target_id) << std::dec << std::setfill(' ')
                << " (Walkability Mask)";
        } else {
            oss << "Tile 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<int>(target_id) << std::dec << std::setfill(' ');
        }
        oss << " [";
        switch (combinator) {
            case scramble_combinator::next_or_branch:   oss << "OR / Branch"; break;
            case scramble_combinator::must_and:         oss << "AND / AbortOnMismatch"; break;
            case scramble_combinator::satisfy_branch:   oss << "SATISFY / ProceedToAction"; break;
            case scramble_combinator::accumulate_coord: oss << "ACCUMULATE"; break;
        }
        oss << "]";
    } else {
        // Placement / Action context
        if (is_class_check) {
            oss << "Random Tile from Class 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<int>(target_id) << std::dec << std::setfill(' ');
        } else {
            oss << "Place Tile ID 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<int>(target_id) << std::dec << std::setfill(' ')
                << " (" << static_cast<int>(target_id) << ")";
        }
    }

    if (transform_mode != 0) {
        oss << ", Transform: " << static_cast<int>(transform_mode);
    }
    return oss.str();
}

std::string scramble_rule_group::to_string(bool is_condition_context) const {
    std::ostringstream oss;
    oss << "Group Mode: ";
    switch (mode) {
        case scramble_group_mode::unconditional_placement: oss << "UnconditionalPlacement (0)"; break;
        case scramble_group_mode::weighted_random:         oss << "WeightedRandom (1)"; break;
        case scramble_group_mode::single_random_select:    oss << "SingleRandomSelect (2)"; break;
        case scramble_group_mode::probabilistic_placement: oss << "ProbabilisticPlacement (3)"; break;
        case scramble_group_mode::nested_condition_group:  oss << "NestedConditionGroup (4)"; break;
        case scramble_group_mode::generic:                 oss << "Generic (" << (header_word >> 24) << ")"; break;
    }
    oss << ", Count: " << count << ", Terminal: " << (is_terminal ? "true" : "false") << "\n";
    for (std::size_t i = 0; i < instructions.size(); ++i) {
        oss << "    [" << std::setw(2) << i << "] " << instructions[i].to_string(is_condition_context) << "\n";
    }
    return oss.str();
}

std::string scramble_header_flags::to_string() const {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << raw_flags << std::dec << std::setfill(' ') << " [";
    std::vector<std::string> flag_names;
    if (has_precondition_script)  flag_names.push_back("PreconditionScript");
    if (has_condition_groups)     flag_names.push_back("ConditionGroups");
    if (randomize_orientation)    flag_names.push_back("RandomizeOrientation");
    if (has_postcondition_script) flag_names.push_back("PostconditionScript");
    if (has_action_groups)        flag_names.push_back("ActionGroups");
    if (extra_flags) {
        std::ostringstream eoss;
        eoss << "Extra:0x" << std::hex << static_cast<int>(extra_flags);
        flag_names.push_back(eoss.str());
    }
    if (flag_names.empty()) oss << "None";
    else {
        for (std::size_t i = 0; i < flag_names.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << flag_names[i];
        }
    }
    oss << "]";
    return oss.str();
}

static std::string format_rules_disassembly(
    std::string_view title,
    std::size_t word_count,
    const scramble_header_flags& header,
    uint32_t precondition_script_offset,
    const std::vector<scramble_rule_group>& condition_groups,
    uint32_t postcondition_script_offset,
    const std::vector<scramble_rule_group>& action_groups)
{
    std::ostringstream oss;
    oss << "=== " << title << " (" << word_count << " words) ===\n";
    oss << "Header Flags: " << header.to_string() << "\n";

    if (header.has_precondition_script) {
        oss << "Precondition Script Pointer/Offset: 0x" << std::hex << precondition_script_offset << std::dec << "\n";
    }

    if (!condition_groups.empty()) {
        oss << "-- Condition Rule Groups (" << condition_groups.size() << ") --\n";
        for (std::size_t i = 0; i < condition_groups.size(); ++i) {
            oss << "  Condition Group " << i << ": " << condition_groups[i].to_string(true);
        }
    }

    if (header.has_postcondition_script) {
        oss << "Postcondition Script Pointer/Offset: 0x" << std::hex << postcondition_script_offset << std::dec << "\n";
    }

    if (!action_groups.empty()) {
        oss << "-- Action Rule Groups (" << action_groups.size() << ") --\n";
        for (std::size_t i = 0; i < action_groups.size(); ++i) {
            oss << "  Action Group " << i << ": " << action_groups[i].to_string(false);
        }
    }

    return oss.str();
}

std::string level_scrambling_rules_block::disassemble() const {
    return format_rules_disassembly(
        "Level Scrambling / Generation Rules (Type 25)",
        rules.size(), header, precondition_script_offset, condition_groups,
        postcondition_script_offset, action_groups);
}

std::string entity_behavior_rule_block::disassemble() const {
    return format_rules_disassembly(
        "Entity Reactive Behavior Rules (Type 42)",
        rules.size(), header, precondition_script_offset, condition_groups,
        postcondition_script_offset, action_groups);
}

namespace tile_transforms {
    std::string_view to_box_art(uint8_t mask) noexcept {
        switch ((mask >> 4) & 0x0F) {
            case 0x00: return "·";
            case 0x01: return "╵"; // North
            case 0x02: return "╶"; // East
            case 0x03: return "└"; // North + East
            case 0x04: return "╷"; // South
            case 0x05: return "│"; // North + South
            case 0x06: return "┌"; // South + East
            case 0x07: return "├"; // North + East + South
            case 0x08: return "╴"; // West
            case 0x09: return "┘"; // North + West
            case 0x0A: return "─"; // East + West
            case 0x0B: return "┴"; // North + East + West
            case 0x0C: return "┐"; // South + West
            case 0x0D: return "┤"; // North + South + West
            case 0x0E: return "┬"; // East + South + West
            case 0x0F: return "┼"; // North + East + South + West
            default:   return " ";
        }
    }
}

std::string tile_rotation_entry::to_string() const {
    std::ostringstream oss;
    oss << "Tile 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
        << static_cast<int>(tile_id) << std::dec << std::setfill(' ')
        << " (" << std::setw(3) << static_cast<int>(tile_id) << "): ["
        << (connects_north() ? "N" : " ") << " "
        << (connects_east()  ? "E" : " ") << " "
        << (connects_south() ? "S" : " ") << " "
        << (connects_west()  ? "W" : " ") << "] "
        << box_art() << " ";

    switch (shape()) {
        case tile_shape::empty:
            oss << "(Empty)";
            break;
        case tile_shape::end_cap:
            oss << "(End-cap ";
            if (connects_north()) oss << "North";
            else if (connects_east()) oss << "East";
            else if (connects_south()) oss << "South";
            else oss << "West";
            oss << ")";
            break;
        case tile_shape::line:
            oss << "(Line " << (connects_north() ? "NS" : "EW") << ")";
            break;
        case tile_shape::corner:
            oss << "(Corner ";
            if (connects_north() && connects_east()) oss << "NE";
            else if (connects_east() && connects_south()) oss << "SE";
            else if (connects_south() && connects_west()) oss << "SW";
            else oss << "NW";
            oss << ")";
            break;
        case tile_shape::t_junction:
            oss << "(T-Junction ";
            if (!connects_north()) oss << "ESW";
            else if (!connects_east()) oss << "NSW";
            else if (!connects_south()) oss << "NEW";
            else oss << "NES";
            oss << ")";
            break;
        case tile_shape::cross:
            oss << "(4-Way Cross NESW)";
            break;
    }
    oss << " [0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
        << static_cast<int>(raw_flags) << std::dec << std::setfill(' ') << "]";
    return oss.str();
}

std::vector<tile_rotation_entry> tile_rotations_block::connected_tiles() const {
    std::vector<tile_rotation_entry> list;
    for (std::size_t i = 0; i < rotation_flags.size(); ++i) {
        if (rotation_flags[i] != 0) {
            list.push_back(get_entry(static_cast<uint8_t>(i)));
        }
    }
    return list;
}

std::string tile_rotations_block::disassemble() const {
    std::ostringstream oss;
    oss << "=== Tile Rotations & Directional Connections (Type 58, "
        << rotation_flags.size() << " tiles) ===\n";

    const auto connected = connected_tiles();
    for (const auto& entry : connected) {
        oss << "  " << entry.to_string() << "\n";
    }
    oss << "  (Summary: " << connected.size() << " connected tiles, "
        << (rotation_flags.size() - connected.size()) << " empty/isolated)\n";
    return oss.str();
}

std::string level_layout_block::to_string() const {
    std::ostringstream oss;
    oss << "LevelLayout(Type=" << static_cast<int>(layout_type)
        << ", SymmetryMask=0x" << std::hex << static_cast<int>(symmetry_mask)
        << ", Behavior=0x" << static_cast<int>(behavior_flags)
        << ", StateSize=" << std::dec << static_cast<int>(state_size) << ")";
    return oss.str();
}

std::string entity_rule_index_block::to_string() const {
    std::ostringstream oss;
    oss << "EntityRuleIndex(Offsets: [";
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "0x" << std::hex << offsets[i];
    }
    oss << std::dec << "])";
    return oss.str();
}

std::string audio_settings_block::to_string() const {
    std::ostringstream oss;
    oss << "AudioSettings(Priority=" << static_cast<int>(priority)
        << ", Volume=" << static_cast<int>(volume)
        << ", Pan=" << static_cast<int>(pan)
        << ", Flags=0x" << std::hex << static_cast<int>(flags) << std::dec << ")";
    return oss.str();
}

std::string sprite_metadata_block::to_string() const {
    std::ostringstream oss;
    if (is_font()) {
        oss << "Font(FirstChar=0x" << std::hex << static_cast<int>(first_char())
            << " ['" << (first_char() >= 0x20 && first_char() <= 0x7E ? static_cast<char>(first_char()) : '?') << "']"
            << ", LastChar=0x" << static_cast<int>(last_char())
            << ", Glyphs=" << std::dec << (last_char() >= first_char() ? (last_char() - first_char() + 1) : 0)
            << ")";
    } else {
        oss << "Sprite(Cols=" << static_cast<int>(column_count)
            << ", Dirs=" << static_cast<int>(direction_count);

        std::vector<std::string> traits;
        if (has_idle_frame()) traits.push_back("HasIdleFrame");
        if (can_flip_x()) traits.push_back("CanFlipX");
        if (can_flip_y()) traits.push_back("CanFlipY");
        if (is_overlay()) traits.push_back("OverlayOnly");
        if (is_streamed_on_demand()) traits.push_back("StreamOnDemand");

        const uint8_t unk_render = flags_high & ~(0x01 | 0x02 | 0x04 | 0x08 | 0x80);
        if (unk_render) {
            std::ostringstream uoss;
            uoss << "RenderFlags:0x" << std::hex << static_cast<int>(unk_render);
            traits.push_back(uoss.str());
        }
        if (flags != 0) {
            std::ostringstream foss;
            foss << "Flags:0x" << std::hex << static_cast<int>(flags);
            traits.push_back(foss.str());
        }

        if (!traits.empty()) {
            oss << ", Traits=[";
            for (std::size_t i = 0; i < traits.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << traits[i];
            }
            oss << "]";
        }
        oss << ")";
    }
    return oss.str();
}

std::vector<uint8_t> digital_audio_clip_block::to_wav(uint32_t sample_rate) const {
    const uint32_t pcm_len = static_cast<uint32_t>(pcm_samples.size());
    const uint32_t riff_size = pcm_len + 36;
    const uint16_t num_channels = NUM_CHANNELS;
    const uint16_t bits_per_sample = BITS_PER_SAMPLE;
    const uint16_t block_align = num_channels * (bits_per_sample / 8);
    const uint32_t byte_rate = sample_rate * block_align;

    std::vector<uint8_t> wav;
    wav.reserve(44 + pcm_len);

    auto append16 = [&](uint16_t val) {
        wav.push_back(static_cast<uint8_t>(val & 0xFF));
        wav.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    };
    auto append32 = [&](uint32_t val) {
        wav.push_back(static_cast<uint8_t>(val & 0xFF));
        wav.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        wav.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        wav.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    };
    auto append_bytes = [&](std::string_view s) {
        for (char c : s) wav.push_back(static_cast<uint8_t>(c));
    };

    append_bytes("RIFF");
    append32(riff_size);
    append_bytes("WAVE");
    append_bytes("fmt ");
    append32(16); // Subchunk1Size
    append16(1);  // AudioFormat (1 = PCM)
    append16(num_channels);
    append32(sample_rate);
    append32(byte_rate);
    append16(block_align);
    append16(bits_per_sample);
    append_bytes("data");
    append32(pcm_len);
    wav.insert(wav.end(), pcm_samples.begin(), pcm_samples.end());

    return wav;
}

static std::pair<uint32_t, std::size_t> read_custom_midi_varint(std::span<const uint8_t> data, std::size_t idx) {
    if (idx >= data.size()) return {0, 0};
    const uint8_t v0 = data[idx];
    if (v0 & 0x80) {
        if (idx + 1 >= data.size()) return {0, 0};
        const uint8_t v1 = data[idx + 1];
        const uint32_t val = (static_cast<uint32_t>(v1) << 7) | (v0 & 0x7F);
        return {val, 2};
    } else {
        return {v0, 1};
    }
}

static void append_midi_vlq(std::vector<uint8_t>& out, uint32_t val) {
    if (val == 0) {
        out.push_back(0);
        return;
    }
    std::vector<uint8_t> tmp;
    while (val > 0) {
        tmp.push_back(static_cast<uint8_t>(val & 0x7F));
        val >>= 7;
    }
    for (std::size_t i = tmp.size(); i > 0; --i) {
        uint8_t b = tmp[i - 1];
        if (i > 1) b |= 0x80;
        out.push_back(b);
    }
}

std::vector<uint8_t> music_track_block::to_mid() const {
    if (midi_stream.empty()) return {};

    std::span<const uint8_t> stream = midi_stream;
    std::size_t idx = 0;

    // Skip optional 0xF8 stream prefix
    if (idx < stream.size() && stream[idx] == 0xF8) {
        idx++;
        auto [_, bytes_read] = read_custom_midi_varint(stream, idx);
        idx += bytes_read;
        idx++;
    }

    struct midi_event {
        uint32_t delta = 0;
        std::vector<uint8_t> data;
    };
    std::vector<midi_event> events;
    uint32_t current_delta = 0;
    uint8_t running_status = 0;

    while (idx < stream.size()) {
        uint8_t status = stream[idx];
        if (status <= 0x7F) {
            if (running_status == 0) break;
            status = running_status;
        } else {
            idx++;
        }
        running_status = status;

        if (status == 0xFF) {
            events.push_back({current_delta, {0xFF, 0x2F, 0x00}});
            break;
        }

        bool has_delta = true;
        if (status == 0xF0 || status == 0xF7) {
            auto [length, bytes_read] = read_custom_midi_varint(stream, idx);
            idx += bytes_read;
            std::vector<uint8_t> ev_data;
            ev_data.push_back(status);
            append_midi_vlq(ev_data, length);
            const std::size_t read_bytes = std::min<std::size_t>(length, stream.size() - idx);
            ev_data.insert(ev_data.end(), stream.begin() + idx, stream.begin() + idx + read_bytes);
            idx += read_bytes;
            events.push_back({current_delta, std::move(ev_data)});
        } else {
            if (idx >= stream.size()) break;
            const uint8_t data1 = stream[idx++];
            const uint8_t v3 = status & 0xF0;
            if (v3 == 0xC0 || v3 == 0xD0) {
                events.push_back({current_delta, {status, data1}});
            } else {
                if (idx >= stream.size()) break;
                const uint8_t data2 = stream[idx++];
                events.push_back({current_delta, {status, data1, static_cast<uint8_t>(data2 & 0x7F)}});
                if (data2 & 0x80) {
                    has_delta = false;
                }
            }
        }

        if (has_delta) {
            if (idx < stream.size()) {
                auto [delay, bytes_read] = read_custom_midi_varint(stream, idx);
                idx += bytes_read;
                current_delta = delay;
            } else {
                current_delta = 0;
            }
        } else {
            current_delta = 0;
        }
    }

    // Compile MTrk chunk
    std::vector<uint8_t> track_data;
    // Prepend tempo event: 60 BPM (1,000,000 microseconds / quarter note)
    append_midi_vlq(track_data, 0);
    const uint8_t tempo_bytes[] = {0xFF, 0x51, 0x03, 0x0F, 0x42, 0x40};
    track_data.insert(track_data.end(), std::begin(tempo_bytes), std::end(tempo_bytes));

    for (const auto& ev : events) {
        append_midi_vlq(track_data, ev.delta);
        track_data.insert(track_data.end(), ev.data.begin(), ev.data.end());
    }

    // Standard MIDI Header: Format 0, 1 track, division 100
    std::vector<uint8_t> smf;
    auto append32_be = [&](uint32_t val) {
        smf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
        smf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        smf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        smf.push_back(static_cast<uint8_t>(val & 0xFF));
    };
    auto append16_be = [&](uint16_t val) {
        smf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        smf.push_back(static_cast<uint8_t>(val & 0xFF));
    };

    // "MThd"
    smf.push_back('M'); smf.push_back('T'); smf.push_back('h'); smf.push_back('d');
    append32_be(6);
    append16_be(0);
    append16_be(1);
    append16_be(100);

    // "MTrk"
    smf.push_back('M'); smf.push_back('T'); smf.push_back('r'); smf.push_back('k');
    append32_be(static_cast<uint32_t>(track_data.size()));
    smf.insert(smf.end(), track_data.begin(), track_data.end());

    return smf;
}

std::string ui_overlay_action_block::to_string() const {
    std::ostringstream oss;
    oss << "UIOverlayAction(RuleWord=0x" << std::hex << action_rule_word << std::dec << ")";
    return oss.str();
}

std::string ui_overlay_element::to_string() const {
    std::ostringstream oss;
    oss << "Element(X=" << x << ", Y=" << y
        << ", Sprite=0x" << std::hex << sprite_index
        << ", Flags=0x" << static_cast<int>(flags) << std::dec
        << (is_terminal ? ", Terminal" : "") << ")";
    return oss.str();
}

std::string ui_overlay_layout_block::disassemble() const {
    std::ostringstream oss;
    oss << "=== UI Overlay Layout (" << elements.size() << " elements) ===\n";
    for (std::size_t i = 0; i < elements.size(); ++i) {
        oss << "  [" << i << "] " << elements[i].to_string() << "\n";
    }
    return oss.str();
}

std::string save_game_layout_block::to_string() const {
    std::ostringstream oss;
    oss << "SaveGameLayout(Type " << type_id << ", " << segments.size() << " segments: ";
    for (std::size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "[Offset=0x" << std::hex << segments[i].state_offset
            << ", Size=" << std::dec << segments[i].state_size << "]";
    }
    oss << ")";
    return oss.str();
}

std::string block_address::to_string() const {
    std::ostringstream oss;
    oss << "[Slot=" << slot_id;
    if (column_id != 0 || direction_id != 0) {
        oss << ", Col=" << column_id << ", Dir=" << direction_id;
    }
    if (base_offset != 0) {
        oss << ", Base=0x" << std::hex << base_offset << std::dec;
    }
    oss << "]";
    return oss.str();
}
