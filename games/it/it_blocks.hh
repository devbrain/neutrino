//
// Created by igor on 30/09/2026.
//

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <variant>
#include <span>
#include "binary_reader.hh"
#include "it_vm.hh"

// ============================================================================
// Strongly-typed Game Asset Block Definitions (IT.EXE - GAMOS 1996)
// ============================================================================

/// Type 15: Level suffix and save/directory configuration table (52 bytes)
struct level_config_block {
    std::array<char, 8> level_suffix = {};
    std::array<uint32_t, 11> config_registers = {};
};

/// Type 16: System allocation table, bounds, screen resolution, title (125 bytes)
struct system_allocation_block {
    uint32_t magic = 0;                  // Expected: 0x80000016
    uint32_t decomp_multiplier = 0;       // Left-shifted by 10 (value: 5120 = 5MB)
    uint32_t graphic_heap_size = 0;      // Value: 49152
    uint32_t screen_width = 0;           // Value: 640
    uint32_t screen_height = 0;          // Value: 480
    uint32_t alloc_var1 = 0;             // Value: 40
    uint32_t alloc_var2 = 0;             // Value: 40
    uint32_t alloc_var3 = 0;             // Value: 1
    std::array<uint8_t, 8> misc_flags = {};
    std::array<uint8_t, 72> reserved = {};
    std::array<char, 13> title_cp1251 = {}; // Cyrillic title ("АйТи")
};

/// Type 17: Board sizing & limits metadata (52 bytes)
struct sizing_metadata_block {
    uint32_t global_mode = 0;            // 1
    uint32_t col_div = 0;                // 1
    uint32_t grid_cols = 0;              // 16 columns
    uint32_t grid_rows = 0;              // 12 rows
    uint32_t screen_width = 0;           // 640
    uint32_t screen_height = 0;          // 480
    uint32_t board_alloc_size = 0;       // 307992
    uint32_t max_zones = 0;              // 178
    uint32_t max_sprites = 0;            // 22
    uint32_t max_entities = 0;           // 182
    uint32_t max_music = 0;              // 2
    uint32_t max_ui_pages = 0;           // 4
    uint32_t flag_count = 0;             // 10
};

/// Type 18: Menu configuration registers (8 bytes)
struct menu_config_block {
    uint32_t menu_state0 = 0;
    uint32_t menu_state1 = 0;
};

/// Types 19, 20, 33, 43, 44: Cinematic & render bytecode scripts for VM
struct script_bytecode_block {
    int type_id = 0;
    std::vector<uint8_t> bytecode;

    [[nodiscard]] std::vector<vm_instruction> decode() const {
        return decode_bytecode(bytecode);
    }

    [[nodiscard]] std::string disassemble(std::size_t start_offset = 0) const {
        return disassemble_bytecode(bytecode, start_offset);
    }
};

/// Timeline record types in animation streams (sub_40683C)
enum class anim_record_type : uint8_t {
    config       = 0, // Timing, buffer allocations, markers ("[-=VS=-]"), viewport
    video_frame  = 1, // Video frames (keyframe / delta frame / hold)
    palette      = 2, // 256-color palette update (sub_406748)
    audio_cue    = 3, // Sound trigger / cue (sub_406780)
    audio_stream = 4  // Synchronized digital PCM audio chunk (sub_4067C4)
};

/// 12-byte animation timeline record + payload (sub_40683C)
struct anim_record {
    anim_record_type type = anim_record_type::config;
    uint8_t subtype = 0;          // Video: 1 = keyframe, 0 = delta frame, 2 = loop; Config: 0..6
    uint8_t flag1 = 0;
    uint8_t flag2 = 0;
    uint32_t comp_size = 0;       // Original param1 (compressed payload size or config param)
    uint32_t decomp_size = 0;     // Original param2 (decompressed payload size or config param)
    std::vector<uint8_t> payload; // Decompressed payload (or raw if uncompressed)

    [[nodiscard]] bool is_keyframe() const noexcept {
        return type == anim_record_type::video_frame && subtype == 1;
    }
    [[nodiscard]] bool is_delta_frame() const noexcept {
        return type == anim_record_type::video_frame && subtype == 0 && comp_size > 0;
    }
    [[nodiscard]] bool is_hold_frame() const noexcept {
        return type == anim_record_type::video_frame && comp_size == 0;
    }
    [[nodiscard]] bool is_palette() const noexcept {
        return type == anim_record_type::palette;
    }
    [[nodiscard]] bool is_audio() const noexcept {
        return type == anim_record_type::audio_stream || type == anim_record_type::audio_cue;
    }
    [[nodiscard]] bool is_config() const noexcept {
        return type == anim_record_type::config;
    }
};

/// Tag 0x05 / Type 20: Parsed animation & cutscene timeline streaming payload (sub_40683C)
struct animation_stream_block {
    std::vector<anim_record> records;
};

/// Type 24: Background image / movie frame with embedded 256-color palette
struct background_image_block {
    uint32_t reserved0 = 0;
    uint32_t reserved1 = 0;
    uint32_t width = 0;                  // Typically 640
    uint32_t height = 0;                 // Typically 480
    uint32_t pixel_size = 0;
    uint32_t total_size = 0;
    std::vector<uint8_t> pixels;         // Size: width * height
    std::array<uint8_t, 768> palette = {};// 256 RGB triplets
};

/// Boolean combinator flag in rule instruction (Bits 10-11)
enum class scramble_combinator : uint8_t {
    next_or_branch   = 0, // 0x000: On mismatch, advance to next group; on match continue
    must_and         = 1, // 0x400: Must match (AND); on mismatch abort rule evaluation
    satisfy_branch   = 2, // 0x800: Match satisfies rule condition; advance to action phase
    accumulate_coord = 3  // 0xC00: Store matching coordinate in match register
};

/// Scrambling / generation group mode (HIBYTE of group header)
enum class scramble_group_mode : uint8_t {
    unconditional_placement = 0, // Case 0 (sub_403E65): Place all tiles with orientation
    weighted_random         = 1, // Case 1 (sub_403F55): Weighted random rule execution
    single_random_select    = 2, // Case 2 (sub_403FE4): Uniformly select 1 instruction from group
    probabilistic_placement = 3, // Case 3 (sub_40402F): 50% coin-flip placement per instruction
    nested_condition_group  = 4, // Case 4: Condition group initializing evaluation stack
    generic                 = 5  // Other / default switch handler
};

/// A single decoded 32-bit rule instruction or tile placement word
struct scramble_rule_instruction {
    uint32_t raw_word = 0;
    int8_t y = 0;                          // Bits 24-31: Y coordinate / row offset
    int8_t x = 0;                          // Bits 16-23: X coordinate / col offset
    uint8_t transform_mode = 0;             // Bits 12-15: Symmetry/orientation mode (funcs_4028DD / funcs_4028F2)
    scramble_combinator combinator = scramble_combinator::next_or_branch; // Bits 10-11
    bool invert_match = false;             // Bit 9: Negate test result
    bool is_class_check = false;           // Bit 8: Test walkability mask (Type 56) vs exact tile ID
    uint8_t target_id = 0;                 // Bits 0-7: Entity class ID (if is_class_check) or exact Tile ID

    [[nodiscard]] std::string to_string(bool is_condition_context = false) const;
};

/// A group of rule instructions
struct scramble_rule_group {
    uint32_t header_word = 0;
    scramble_group_mode mode = scramble_group_mode::unconditional_placement;
    bool is_terminal = false;              // Bit 16 (0x10000): Final group in list
    uint16_t count = 0;                    // Bits 0-15: Number of instruction words
    std::vector<scramble_rule_instruction> instructions;

    [[nodiscard]] std::string to_string(bool is_condition_context = false) const;
};

/// Header options and control flags (rules[0])
struct scramble_header_flags {
    uint32_t raw_flags = 0;
    bool has_precondition_script = false;  // Bit 0 (0x01): Executed by sub_408E38 before rules
    bool has_condition_groups = false;     // Bit 1 (0x02): Evaluate 2D pattern-matching rules
    bool randomize_orientation = false;    // Bit 2 (0x04): Scramble board orientations (sub_402A68)
    bool has_postcondition_script = false; // Bit 3 (0x08): Post-match script & sound cue
    bool has_action_groups = false;        // Bit 4 (0x10): Grid placement & transformation actions
    uint8_t extra_flags = 0;               // Bits 8-15

    [[nodiscard]] std::string to_string() const;
};

/// Type 25: Level scrambling rules and board generator rules (sub_403A64)
struct level_scrambling_rules_block {
    scramble_header_flags header;
    uint32_t precondition_script_offset = 0;
    std::vector<scramble_rule_group> condition_groups;
    uint32_t postcondition_script_offset = 0;
    std::vector<scramble_rule_group> action_groups;

    std::vector<uint32_t> rules;           // Preserved original 32-bit words for compatibility

    [[nodiscard]] std::string disassemble() const;
};

/// Type 32: Level layout metadata & entity class properties (sub_404EC8, sub_404594)
struct level_layout_block {
    uint8_t layout_type = 0;     // Byte 0: Entity type / layout category
    uint8_t symmetry_mask = 0;   // Byte 1: Allowed D4 orientation/symmetry test mask
    uint8_t behavior_flags = 0;  // Byte 2: Entity behavior and rendering flags
    uint8_t state_size = 0;      // Byte 3: Runtime dynamic state size allocated for entity

    // Compatibility accessors
    [[nodiscard]] uint8_t zone_index() const noexcept { return symmetry_mask; }
    [[nodiscard]] uint16_t flags() const noexcept {
        return static_cast<uint16_t>(behavior_flags | (static_cast<uint16_t>(state_size) << 8));
    }

    [[nodiscard]] std::string to_string() const;
};

/// Type 34: Level collision walkthrough override map
struct level_collision_override_block {
    std::vector<uint8_t> collision_map;
};

/// Type 35: Entity spawner / behavior rule index table (pointers to Type 42 rules)
struct entity_rule_index_block {
    std::vector<uint32_t> offsets; // Pointers / offsets to Type 42 rules in the entity behavior table

    [[nodiscard]] std::string to_string() const;
};
using entity_spawner_header_block = entity_rule_index_block;

/// Type 42: Entity reactive behavior and interaction rule block (sub_403A64, sub_404C64)
struct entity_behavior_rule_block {
    scramble_header_flags header;
    uint32_t precondition_script_offset = 0;
    std::vector<scramble_rule_group> condition_groups;
    uint32_t postcondition_script_offset = 0;
    std::vector<scramble_rule_group> action_groups;

    std::vector<uint32_t> rules; // Preserved original 32-bit words for compatibility

    [[nodiscard]] std::string disassemble() const;
};
using entity_spawner_record_block = entity_behavior_rule_block;

/// Type 56: Collision walkability matrix (23 bytes = 184 bits)
struct collision_walkability_matrix_block {
    std::array<uint8_t, 23> matrix = {};

    /// Checks if a tile/zone ID (0..183) is passable/walkable for this entity class.
    [[nodiscard]] bool is_passable(std::size_t tile_id) const noexcept {
        if (tile_id >= matrix.size() * 8) return false;
        return (matrix[tile_id >> 3] & (1 << (tile_id & 7))) != 0;
    }

    /// Returns a list of all tile IDs marked passable in this matrix.
    [[nodiscard]] std::vector<uint8_t> passable_tiles() const {
        std::vector<uint8_t> list;
        for (std::size_t i = 0; i < matrix.size() * 8; ++i) {
            if (is_passable(i)) {
                list.push_back(static_cast<uint8_t>(i));
            }
        }
        return list;
    }
};

/// Type 57: Tile candidate ID list for puzzle rules
struct tile_candidate_list_block {
    std::vector<uint8_t> tile_ids;
};

/// Directional port / connection bits in Type 58 and tile orientation flags
enum class tile_connection : uint8_t {
    none  = 0x00,
    north = 0x10, // Bit 4: Connects Up (-Y)
    east  = 0x20, // Bit 5: Connects Right (+X)
    south = 0x40, // Bit 6: Connects Down (+Y)
    west  = 0x80  // Bit 7: Connects Left (-X)
};

/// Topological connection shape classification
enum class tile_shape : uint8_t {
    empty,      // 0 ports
    end_cap,    // 1 port (N, E, S, or W)
    line,       // 2 ports opposite (NS or EW)
    corner,     // 2 ports adjacent (NE, ES, SW, WN)
    t_junction, // 3 ports (NES, ESW, SWN, WNE)
    cross       // 4 ports (NESW)
};

/// Rotation and reflection operations on connection masks (Dihedral Group D4)
namespace tile_transforms {
    /// 90-degree clockwise rotation (sub_402014 / byte_410084): N->E, E->S, S->W, W->N
    [[nodiscard]] constexpr uint8_t rotate_cw_90(uint8_t mask) noexcept {
        const uint8_t dir = (mask >> 4) & 0x0F;
        const auto rotated = static_cast<uint8_t>(((dir << 1) & 0x0E) | ((dir >> 3) & 0x01));
        return static_cast<uint8_t>((rotated << 4) | (mask & 0x0F));
    }

    /// 180-degree rotation (sub_402044 / byte_410094): N<->S, E<->W
    [[nodiscard]] constexpr uint8_t rotate_180(uint8_t mask) noexcept {
        return rotate_cw_90(rotate_cw_90(mask));
    }

    /// 270-degree clockwise / 90-degree CCW rotation (sub_40206C / byte_4100A4): N->W, W->S, S->E, E->N
    [[nodiscard]] constexpr uint8_t rotate_ccw_90(uint8_t mask) noexcept {
        const uint8_t dir = (mask >> 4) & 0x0F;
        const auto rotated = static_cast<uint8_t>(((dir >> 1) & 0x07) | ((dir << 3) & 0x08));
        return static_cast<uint8_t>((rotated << 4) | (mask & 0x0F));
    }

    /// Horizontal reflection / flip across Y axis (sub_40209C): E <-> W
    [[nodiscard]] constexpr uint8_t flip_horizontal(uint8_t mask) noexcept {
        const uint8_t dir = (mask >> 4) & 0x0F;
        const auto flipped = static_cast<uint8_t>((dir & 0x05) | ((dir & 0x02) << 2) | ((dir & 0x08) >> 2));
        return static_cast<uint8_t>((flipped << 4) | (mask & 0x0F));
    }

    /// Vertical reflection / flip across X axis (sub_4020F0): N <-> S
    [[nodiscard]] constexpr uint8_t flip_vertical(uint8_t mask) noexcept {
        const uint8_t dir = (mask >> 4) & 0x0F;
        const auto flipped = static_cast<uint8_t>((dir & 0x0A) | ((dir & 0x01) << 2) | ((dir & 0x04) >> 2));
        return static_cast<uint8_t>((flipped << 4) | (mask & 0x0F));
    }

    /// Return Unicode box-drawing character for a connection mask
    [[nodiscard]] std::string_view to_box_art(uint8_t mask) noexcept;

    /// Return topological shape of connection mask
    [[nodiscard]] constexpr tile_shape get_shape(uint8_t mask) noexcept {
        switch (const uint8_t dir = (mask >> 4) & 0x0F; (dir & 1) + ((dir >> 1) & 1) + ((dir >> 2) & 1) + ((dir >> 3) & 1)) {
            case 0: return tile_shape::empty;
            case 1: return tile_shape::end_cap;
            case 2:
                if (dir == 0x05 || dir == 0x0A) return tile_shape::line;
                return tile_shape::corner;
            case 3: return tile_shape::t_junction;
            case 4: return tile_shape::cross;
            default: return tile_shape::empty;
        }
    }
}

/// A decoded tile rotation / directional connection entry
struct tile_rotation_entry {
    uint8_t tile_id = 0;
    uint8_t raw_flags = 0;

    [[nodiscard]] constexpr bool connects_north() const noexcept { return (raw_flags & 0x10) != 0; }
    [[nodiscard]] constexpr bool connects_east()  const noexcept { return (raw_flags & 0x20) != 0; }
    [[nodiscard]] constexpr bool connects_south() const noexcept { return (raw_flags & 0x40) != 0; }
    [[nodiscard]] constexpr bool connects_west()  const noexcept { return (raw_flags & 0x80) != 0; }

    [[nodiscard]] constexpr uint8_t connection_count() const noexcept {
        const uint8_t d = (raw_flags >> 4) & 0x0F;
        return static_cast<uint8_t>((d & 1) + ((d >> 1) & 1) + ((d >> 2) & 1) + ((d >> 3) & 1));
    }

    [[nodiscard]] constexpr tile_shape shape() const noexcept {
        return tile_transforms::get_shape(raw_flags);
    }

    [[nodiscard]] std::string_view box_art() const noexcept {
        return tile_transforms::to_box_art(raw_flags);
    }

    [[nodiscard]] std::string to_string() const;
};

/// Type 58: Tile baseline rotations and 4-way connection table (sub_403A64, sub_40283C)
struct tile_rotations_block {
    std::vector<uint8_t> rotation_flags; // Preserved raw byte array for compatibility

    [[nodiscard]] bool has_tile(uint8_t tile_id) const noexcept {
        return tile_id < rotation_flags.size();
    }

    [[nodiscard]] uint8_t get_flags(uint8_t tile_id) const noexcept {
        return tile_id < rotation_flags.size() ? rotation_flags[tile_id] : 0;
    }

    [[nodiscard]] tile_rotation_entry get_entry(uint8_t tile_id) const noexcept {
        return tile_rotation_entry{tile_id, get_flags(tile_id)};
    }

    [[nodiscard]] bool connects_north(uint8_t tile_id) const noexcept { return (get_flags(tile_id) & 0x10) != 0; }
    [[nodiscard]] bool connects_east(uint8_t tile_id)  const noexcept { return (get_flags(tile_id) & 0x20) != 0; }
    [[nodiscard]] bool connects_south(uint8_t tile_id) const noexcept { return (get_flags(tile_id) & 0x40) != 0; }
    [[nodiscard]] bool connects_west(uint8_t tile_id)  const noexcept { return (get_flags(tile_id) & 0x80) != 0; }

    /// Returns list of all tile IDs in this block that have at least one connection port.
    [[nodiscard]] std::vector<tile_rotation_entry> connected_tiles() const;

    [[nodiscard]] std::string disassemble() const;
};

/// Type 64: Sprite metadata & directional/animation traits (4 bytes)
struct sprite_metadata_block {
    uint8_t flags = 0;           // Byte 0: General traits (Bit 0: is_font, Bit 1..7: layout/format)
    uint8_t flags_high = 0;      // Byte 1: Rendering traits (Bit 0: overlay, 1: flip_x, 2: flip_y, 3: idle_frame, 7: stream) or first_char_code
    uint8_t column_count = 0;    // Byte 2: Animation state/column count (or last_char_code if is_font)
    uint8_t direction_count = 0; // Byte 3: Direction count (1, 4, 8, 9, 12, etc.)

    // Font inspection
    [[nodiscard]] constexpr bool is_font() const noexcept { return (flags & 0x01) != 0; }
    [[nodiscard]] constexpr uint8_t first_char() const noexcept { return is_font() ? flags_high : 0; }
    [[nodiscard]] constexpr uint8_t last_char() const noexcept { return is_font() ? column_count : 0; }

    // Rendering & Directional traits (when not font)
    [[nodiscard]] constexpr bool is_overlay() const noexcept { return (flags_high & 0x01) != 0; }
    [[nodiscard]] constexpr bool can_flip_x() const noexcept { return (flags_high & 0x02) != 0; }
    [[nodiscard]] constexpr bool can_flip_y() const noexcept { return (flags_high & 0x04) != 0; }
    [[nodiscard]] constexpr bool has_idle_frame() const noexcept { return (flags_high & 0x08) != 0; }
    [[nodiscard]] constexpr bool is_streamed_on_demand() const noexcept { return (flags_high & 0x80) != 0; }

    [[nodiscard]] std::string to_string() const;
};

/// Type 66: Sprite frame layout table
struct sprite_frame_record {
    uint32_t block_ptr = 0;
    int16_t x_hotspot = 0;
    int16_t y_hotspot = 0;
};

struct sprite_frame_table_block {
    std::vector<sprite_frame_record> frames;
};

/// Type 67: Palette-indexed sprite pixel frame
struct sprite_graphic_block {
    uint16_t width = 0;
    uint16_t height = 0;
    std::vector<uint8_t> pixels;         // Size: width * height
};

/// Type 80: Audio settings setup table (4 bytes: priority, volume, pan, flags)
struct audio_settings_block {
    uint8_t priority = 0;
    uint8_t volume = 0;
    uint8_t pan = 0;
    uint8_t flags = 0;
    uint32_t settings = 0; // Compatibility field mirroring all 4 bytes

    [[nodiscard]] std::string to_string() const;
};

/// Type 81: Digital audio clip (unsigned 8-bit mono PCM, 11025 Hz)
struct digital_audio_clip_block {
    static constexpr uint32_t DEFAULT_SAMPLE_RATE = 11025;
    static constexpr uint16_t NUM_CHANNELS = 1;
    static constexpr uint16_t BITS_PER_SAMPLE = 8;

    std::vector<uint8_t> pcm_samples;

    /// Export PCM samples to standard RIFF WAVE (.wav) container
    [[nodiscard]] std::vector<uint8_t> to_wav(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) const;
};

/// Type 82: Music track (custom MIDI event stream)
struct music_track_block {
    std::vector<uint8_t> midi_stream;

    /// Convert custom MIDI stream with LE varints & post-event deltas to Standard MIDI File (.mid)
    [[nodiscard]] std::vector<uint8_t> to_mid() const;
};

/// Type 96: UI overlay action / menu rule word (sub_406AEC -> sub_403A64)
struct ui_overlay_action_block {
    uint32_t action_rule_word = 0;
    uint32_t descriptor = 0; // Compatibility field mirroring action_rule_word

    [[nodiscard]] std::string to_string() const;
};
using dialog_index_block = ui_overlay_action_block;

/// A single 8-byte UI overlay element / sprite placement record (sub_406F20)
struct ui_overlay_element {
    int16_t x = 0;
    int16_t y = 0;
    uint16_t sprite_index = 0;
    uint8_t flags = 0;
    bool is_terminal = false;

    [[nodiscard]] std::string to_string() const;
};

/// Type 97: UI overlay element layout (sub_406F20, sub_406D80)
struct ui_overlay_layout_block {
    std::vector<ui_overlay_element> elements;
    std::vector<std::string> strings; // Compatibility field

    [[nodiscard]] std::string disassemble() const;
};
using dialog_text_block = ui_overlay_layout_block;

/// Types 124, 125, 126: Save game state layout segment definitions
struct save_state_segment {
    uint32_t state_offset = 0;
    uint32_t state_size = 0;
};

struct save_game_layout_block {
    int type_id = 0;
    std::vector<save_state_segment> segments;

    [[nodiscard]] std::string to_string() const;
};

/// Fallback for unspecialized or future block types
struct generic_asset_block {
    int type_id = 0;
    std::vector<uint8_t> raw_data;
};

// ============================================================================
// Variant of all Decoded Block Types
// ============================================================================
using decoded_block = std::variant<
    level_config_block,                  // Type 15
    system_allocation_block,             // Type 16
    sizing_metadata_block,               // Type 17
    menu_config_block,                   // Type 18
    script_bytecode_block,               // Types 19, 20 (script), 33, 43, 44
    animation_stream_block,              // Tag 0x05 / Type 20 (timeline)
    background_image_block,              // Type 24
    level_scrambling_rules_block,        // Type 25
    level_layout_block,                  // Type 32
    level_collision_override_block,      // Type 34
    entity_rule_index_block,             // Type 35 (alias: entity_spawner_header_block)
    entity_behavior_rule_block,          // Type 42 (alias: entity_spawner_record_block)
    collision_walkability_matrix_block,  // Type 56
    tile_candidate_list_block,           // Type 57
    tile_rotations_block,                // Type 58
    sprite_metadata_block,               // Type 64
    sprite_frame_table_block,            // Type 66
    sprite_graphic_block,                // Type 67
    audio_settings_block,                // Type 80
    digital_audio_clip_block,            // Type 81
    music_track_block,                   // Type 82
    ui_overlay_action_block,             // Type 96 (alias: dialog_index_block)
    ui_overlay_layout_block,             // Type 97 (alias: dialog_text_block)
    save_game_layout_block,              // Types 124, 125, 126
    generic_asset_block                  // Unknown / fallback
>;

/// Stream command opcodes / tags processed in resource containers (IT.c:4600-4712)
enum class stream_opcode : uint8_t {
    end_of_stream         = 0x00, // 0: Terminate resource container stream
    param_column_index    = 0x01, // 1: Read secondary index / animation column / rule slot (DstBuf)
    param_direction_index = 0x02, // 2: Read tertiary index / direction angle (v13)
    param_base_offset     = 0x03, // 3: Read memory/script base offset modifier (v14)
    command_block         = 0x04, // 4: Read block payload (uncompressed or LZSS)
    animation_stream      = 0x05, // 5: Read animation & cutscene timeline stream payload
    block_terminator      = 0xFF  // 255: End of block definition (triggers block handler)
};

namespace stream_masks {
    constexpr uint8_t TYPE_MASK        = 0x7F; // Lower 7 bits encode the Asset Type ID (a1)
    constexpr uint8_t NO_SLOT_ARG_FLAG = 0x80; // When set, block has no trailing slot varint
}

/// Stream addressing registers (sub_405514 / sub_405150)
struct block_address {
    uint32_t slot_id = 0;      // v11 / a6: Target Instance / Slot ID (e.g. Sprite ID, Level ID, Sound ID)
    uint32_t column_id = 0;    // dst_buf / a5: Sub-index / Animation Column / Rule Slot
    uint32_t direction_id = 0; // v13 / a4: Tertiary index / Direction Angle (0..8)
    uint32_t base_offset = 0;  // v14 / a3: Base offset / Script modifier

    [[nodiscard]] std::string to_string() const;
};

/// Addressed block pair combining location/routing registers with decoded content
using addressed_block = std::pair<block_address, decoded_block>;

/**
 * @brief Decodes a raw asset payload from a binary_reader into its corresponding strongly-typed struct.
 *
 * @param type_id Asset type ID (a1).
 * @param rdr binary_reader positioned at the start of the block payload.
 * @param payload_size Byte size of the payload.
 * @param is_anim_stream True if extracted from Tag 0x05 (animation stream).
 * @return decoded_block variant containing the concrete struct.
 */
decoded_block decode_block(int type_id, binary_reader& rdr, std::size_t payload_size, bool is_anim_stream = false);

/**
 * @brief Decodes a raw asset payload into its corresponding strongly-typed struct.
 *
 * @param type_id Asset type ID (a1).
 * @param data Uncompressed or decompressed byte payload.
 * @param is_anim_stream True if extracted from Tag 0x05 (animation stream).
 * @return decoded_block variant containing the concrete struct.
 */
decoded_block decode_block(int type_id, std::span<const uint8_t> data, bool is_anim_stream = false);
