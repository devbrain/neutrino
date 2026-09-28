//
// Semantic decode of a TAB grid cell (and level). Turns the two raw TAB bytes
// per cell into the reverse-engineered meaning documented in
// docs/bonuses.md (attributes) and ~/proj/ke_dump/docs/tab.md (§5 tile id),
// so gameplay reads intent instead of re-deriving bit math at every call site.
//

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include <ke/format/tab.hh>

namespace rs {
    // One original game tick. Krypton Egg runs in VGA mode 13h off the ~70 Hz
    // vertical-retrace timer (confirmed against ke.exe), so spawn timing is
    // carried in these -- it converts cleanly to real time and to
    // neutrino::frame_duration via std::chrono::duration_cast.
    using ke_tick = std::chrono::duration <std::int32_t, std::ratio <1, 70>>;

    // Gameplay class of a brick, decoded from the raw tile id (tab.md §5).
    enum class brick_kind : std::uint8_t {
        empty,          // 0x00        -- no brick
        single_hit,     // tier 1      -- destroyed in one hit
        multi_hit,      // tier 2-3    -- drops one durability tier (-0x10) per hit
        indestructible, // 0x91-0xF4   -- border / decorative / animated
        trigger,        // 0xF5-0xF7   -- special trigger brick
        marker,         // 0xF8-0xF9   -- position marker (also drawn)
        special,        // 0xFA-0xFF   -- remaining hit count carried in the attribute
    };

    // Runtime indices into ke.exe's sprite/effect tables, NOT the TAB's 1-based codes.
    // destroy_brick (0x329BA) subtracts one; spawn_bonus (0x2D712) rejects IDs >= 28
    // BEFORE applying its redundant 0x1F mask. See docs/bonuses.md for the evidence.
    enum class bonus : std::uint8_t {
        enlarge_paddle     = 0,  // size += mag, original size index clamped to 0..12
        damage_paddle      = 1,  // consume ONE shield point; kill an unshielded paddle
        score_multiplier   = 2,  // score shift += mag (multiplier *= 2^mag)
        reverse_controls   = 3,  // mirror horizontal input for 64*mag ticks
        shrink_balls       = 4,  // size -= mag for every ball, clamped to 0..5
        glue_paddle        = 5,  // catch balls after the slime animation finishes
        extra_life         = 6,  // lives += mag
        extra_ball         = 7,  // launch ONE new ball from the paddle (ignores mag)
        enlarge_balls      = 8,  // size += mag for every ball, clamped to 0..5
        darkness           = 9,  // palette darkening, timer += 128*mag ticks
        speed_up_all_balls = 10, // increase velocity components, not ball size
        slow_all_balls     = 11,
        autopilot          = 12, // JAO: automatic paddle, 1024*mag ticks
        flying_paddle      = 13, // allow vertical movement between y=24 and y=188
        freeze_paddle      = 14, // disable manual movement for 32*mag ticks
        shield             = 15, // resistance += mag
        cannon             = 16, // projectile type 2, max 1, reload 15 ticks
        power_ball         = 17, // blue balls damage bricks without bouncing
        ghost_balls        = 18, // green balls skip bricks/enemies, timer += 512*mag
        extra_paddle       = 19, // stationary spectre paddle at the pickup x
        rapid_cannon       = 20, // projectile type 2, max 5, reload 15 ticks
        random             = 21, // dispatch another effect; excludes damage and itself
        shrink_paddle      = 22,
        single_gun         = 23, // projectile type 0, max 1, reload 4 ticks
        double_gun         = 24, // projectile type 1, max 1, reload 6 ticks
        rapid_single_gun   = 25, // projectile type 0, max 5, reload 4 ticks
        rapid_double_gun   = 26, // projectile type 1, max 3, reload 6 ticks
        clear_enemies      = 27, // dynamite
        none               = 0xFF,
    };

    inline constexpr std::size_t bonus_count = 28;

    // The 8 enemy types a level's spawn_seq cycles through (tab.md §7). Each indexes the
    // enemy animation table (ke.exe 0x496B8 / KE_NMY.BOB); see enemy_anim.
    enum class enemy : std::uint8_t {
        insectoid = 0,
        green_alien,
        ship_demon,
        ufo_disc,
        green_egg,
        red_egg,
        blue_orb,
        gem_orb,
    };

    // A decoded grid cell: the meaning of the two TAB bytes for cell (x, y).
    struct ke_cell {
        std::uint8_t tile_id = 0;    // raw tile byte
        std::uint8_t attribute = 0;  // raw attribute byte
        brick_kind   kind = brick_kind::empty;
        int          graphic = -1;   // KE_BRICK.BOB block (tile_id - 1); -1 when empty
        int          hits = 0;       // hits to destroy: 1-3 destructible, attr>>2 special, 0 otherwise
        std::uint8_t colour = 0;     // brick colour/family (tile low nibble), preserved across damage
        bonus        bonus_type = bonus::none; // dropped bonus (docs/bonuses.md); none = no bonus
        // Bonus strength = (attr & 3) + 1, range 1-4. Scales the effect: paddle grow/shrink
        // steps, extra lives, ball-speed delta, etc. (some, e.g. extra_ball, ignore it).
        // Meaningful only with a real bonus -- 0 when drops_bonus is false.
        std::uint8_t bonus_mag = 0;
        bool         drops_bonus = false;         // spawns a capsule when destroyed (== bonus_type != none)
        bool         counts_toward_clear = false; // destructible brick (0x01-0x90)

        [[nodiscard]] constexpr bool is_empty() const noexcept {
            return kind == brick_kind::empty;
        }

        // Decode one raw (tile id, attribute) pair. See tab.md §4/§5.
        [[nodiscard]] static constexpr ke_cell decode(std::uint8_t tile_id, std::uint8_t attribute) noexcept {
            ke_cell c;
            c.tile_id = tile_id;
            c.attribute = attribute;

            if (tile_id == 0x00) {
                return c; // empty: kind = empty, graphic = -1
            }

            // Every non-empty tile is drawn; the graphic is the 1-based block minus one.
            c.graphic = static_cast <int>(tile_id) - 1;

            if (tile_id <= 0x90) {
                // Destructible: three colour groups of 0x30 (A:0x01-0x30, B:0x31-0x60,
                // C:0x61-0x90). Within a group the high nibble is the durability tier
                // (1-3) and the low nibble the colour; a hit subtracts 0x10 (one tier),
                // preserving the colour.
                const int tier = ((static_cast <int>(tile_id) - 1) % 0x30) / 0x10 + 1; // 1..3
                c.kind = (tier == 1) ? brick_kind::single_hit : brick_kind::multi_hit;
                c.hits = tier;
                c.colour = static_cast <std::uint8_t>(tile_id & 0x0F);
                c.counts_toward_clear = true;

                // TAB codes 1..28 become runtime IDs 0..27. Codes 0 and 29..63
                // never spawn a capsule; high bits must not wrap into another effect.
                const auto code = static_cast <std::size_t>(attribute >> 2);
                if (tile_id > 0x30 && tile_id < 0x61 && code >= 1 && code <= bonus_count) {
                    c.drops_bonus = true;
                    c.bonus_type = static_cast <bonus>(code - 1);
                    c.bonus_mag = static_cast <std::uint8_t>((attribute & 0x03) + 1);
                }
                return c;
            }

            if (tile_id <= 0xF4) {
                c.kind = brick_kind::indestructible; // 0x91-0xF4
                return c;
            }
            if (tile_id <= 0xF7) {
                c.kind = brick_kind::trigger; // 0xF5-0xF7
                return c;
            }
            if (tile_id <= 0xF9) {
                c.kind = brick_kind::marker; // 0xF8-0xF9
                return c;
            }

            // 0xFA-0xFF: special bricks whose remaining hit count is held in attr>>2.
            c.kind = brick_kind::special;
            c.hits = static_cast <int>(attribute >> 2);
            return c;
        }

        [[nodiscard]] static constexpr ke_cell decode(const tab_cell& cell) noexcept {
            return decode(cell.tile, cell.attribute);
        }
    };

    // A decoded level: the semantic brick grid plus the spawn schedule (tab.md
    // §2/§7). Mirrors tab_level's 18x16 row-major layout.
    struct ke_level {
        static constexpr int cols = tab_level::cols; // 18
        static constexpr int rows = tab_level::rows; // 16

        std::array <ke_cell, static_cast <std::size_t>(cols) * rows> cells{};
        ke_tick spawn_period{0};              // enemy spawn countdown reload (§7)
        std::array <enemy, 8> spawn_seq{}; // enemy type cycle (tab.md §7); see enemy_anim

        [[nodiscard]] constexpr const ke_cell& at(int x, int y) const {
            return cells[static_cast <std::size_t>(y) * cols + x];
        }

        // Decode a raw parsed TAB level into semantic cells + spawn metadata.
        [[nodiscard]] static constexpr ke_level decode(const tab_level& raw) {
            ke_level lvl;
            lvl.spawn_period = ke_tick{raw.spawn_period};
            for (std::size_t i = 0; i < lvl.spawn_seq.size(); ++i) {
                lvl.spawn_seq[i] = static_cast <enemy>(raw.spawn_seq[i]);
            }
            for (std::size_t i = 0; i < lvl.cells.size(); ++i) {
                lvl.cells[i] = ke_cell::decode(raw.cells[i]);
            }
            return lvl;
        }
    };
} // namespace rs
