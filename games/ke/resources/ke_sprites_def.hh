//
// Created by igor on 14/07/2026.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <neutrino/video/sprite/sprite_def.hh>
#include "resources/ke_loader.hh"
#include "resources/game_resources.hh"

namespace rs {
    // KE_RACK paddle layout: the size frames run consecutively -- size 1 is frame 5, and
    // there are 15 sizes (frames 5..19). The default/start size is 7 (frame 11).
    inline constexpr std::size_t ke_paddle_first_frame  = 5;
    inline constexpr int         ke_paddle_size_count   = 15;
    inline constexpr int         ke_paddle_default_size = 7;

    // Visual index of a paddle @p size (1..ke_paddle_size_count).
    [[nodiscard]] constexpr std::size_t ke_paddle_frame(int size) noexcept {
        return ke_paddle_first_frame + static_cast <std::size_t>(size - 1);
    }

    // A KE sprite animation extracted from ke.exe: the exact BOB block sequence (looping),
    // with a single per-frame duration in game ticks (ke_tick, 70 Hz). count == 1 is a
    // static sprite (its `ticks` is just a long hold). Capacity fits the longest sequence
    // (the 11-frame ship-demon).
    struct ke_anim {
        std::array <std::uint8_t, 12> frames{}; // BOB blocks, valid in [0, count)
        std::uint8_t                  count = 0;
        std::uint8_t                  ticks = 0; // per-frame duration in game ticks

        [[nodiscard]] constexpr bool is_static() const noexcept { return count == 1; }
        [[nodiscard]] constexpr const std::uint8_t* begin() const noexcept { return frames.data(); }
        [[nodiscard]] constexpr const std::uint8_t* end() const noexcept { return frames.data() + count; }
        [[nodiscard]] constexpr ke_tick frame_duration() const noexcept { return ke_tick{ticks}; }
    };

    // Bonus capsule animation per bonus, extracted from ke.exe's 0x49284 sprite descriptor
    // table (verified: each first block matches tab.md §4, and the same extractor decodes the
    // §7 enemy table correctly). Frames are 0-based KE_SPELL blocks; four bonuses (enlarge,
    // slow_ball, fast_ball, shrink_paddle) ping-pong. Indexed by the bonus enum value (0..27);
    // bonus::none has no entry.
    inline constexpr std::array <ke_anim, 28> ke_spell_capsule_anim = {{
        {{62, 63, 64, 65, 66, 65, 64, 63}, 8, 5},   // 0  enlarge_paddle (ping-pong)
        {{67}, 1, 127},                             // 1  gun_laser
        {{53}, 1, 127},                             // 2  score_multiplier
        {{50, 51, 52}, 3, 8},                       // 3  open_exit
        {{76, 77, 78, 79, 80, 79, 78, 77}, 8, 6},   // 4  slow_ball (ping-pong)
        {{70}, 1, 127},                             // 5  area_explosion
        {{37}, 1, 127},                             // 6  extra_life
        {{48}, 1, 127},                             // 7  extra_ball
        {{57, 58, 59, 60, 61, 60, 59, 58}, 8, 6},   // 8  fast_ball (ping-pong)
        {{34}, 1, 127},                             // 9  timed_paddle
        {{69}, 1, 127},                             // 10 speed_up_all_balls
        {{68}, 1, 127},                             // 11 slow_all_balls
        {{56}, 1, 127},                             // 12 clear_effects
        {{54, 55}, 2, 4},                           // 13 ball_relaunch
        {{41}, 1, 127},                             // 14 gun_directional
        {{35}, 1, 127},                             // 15 laser_gun
        {{46}, 1, 127},                             // 16 paddle_transform_a
        {{38, 39}, 2, 10},                          // 17 through_ball
        {{40}, 1, 127},                             // 18 catch_ball
        {{49}, 1, 127},                             // 19 warp
        {{47}, 1, 127},                             // 20 paddle_transform_b
        {{36}, 1, 127},                             // 21 random
        {{71, 72, 73, 74, 75, 74, 73, 72}, 8, 5},   // 22 shrink_paddle (ping-pong)
        {{42}, 1, 127},                             // 23 paddle_transform_c
        {{44}, 1, 127},                             // 24 paddle_transform_d
        {{43}, 1, 127},                             // 25 paddle_transform_e
        {{45}, 1, 127},                             // 26 paddle_transform_f
        {{81, 82, 83}, 3, 3},                       // 27 clear_enemies
    }};

    // Enemy animation per spawn type (0..7), from ke.exe's 0x496B8 descriptor table (matches
    // tab.md §7). Frames are 0-based KE_NMY blocks; ship_demon (type 2) is the long one.
    inline constexpr std::array <ke_anim, 8> ke_nmy_enemy_anim = {{
        {{0, 1}, 2, 6},                             // 0 insectoid
        {{2, 3, 4, 3}, 4, 15},                      // 1 green_alien
        {{5, 5, 5, 5, 6, 7, 64, 65, 64, 7, 6}, 11, 3}, // 2 ship_demon
        {{8, 9, 10}, 3, 8},                         // 3 ufo_disc
        {{11, 12, 13, 12}, 4, 15},                  // 4 green_egg
        {{14, 15, 16, 15}, 4, 10},                  // 5 red_egg
        {{17, 18, 19, 20, 21, 22}, 6, 10},          // 6 blue_orb
        {{23, 24, 25, 26, 27, 28, 29, 30}, 8, 10},  // 7 gem_orb
    }};

    // The capsule animation for a dropped bonus. @pre @p b is a real bonus (0..27), not none.
    [[nodiscard]] constexpr const ke_anim& bonus_capsule(bonus b) noexcept {
        return ke_spell_capsule_anim[static_cast <std::size_t>(b)];
    }

    // The animation for a spawned @p type (a spawn_seq entry).
    [[nodiscard]] constexpr const ke_anim& enemy_anim(enemy type) noexcept {
        return ke_nmy_enemy_anim[static_cast <std::size_t>(type)];
    }

    // Short display names (for debug UIs). Not identity -- that is the enum.
    [[nodiscard]] constexpr std::string_view bonus_name(bonus b) noexcept {
        constexpr std::string_view names[28] = {
            "enlarge", "gun/laser", "score up", "open exit", "slow ball", "explosion",
            "extra life", "extra ball", "fast ball", "timed pad", "speed all", "slow all",
            "clear fx", "relaunch", "gun dir", "laser gun", "transform A", "fire ball",
            "catch ball", "warp", "transform B", "random", "shrink", "transform C",
            "transform D", "transform E", "transform F", "kill enemy",
        };
        const auto i = static_cast <std::size_t>(b);
        return i < 28 ? names[i] : "none";
    }

    [[nodiscard]] constexpr std::string_view enemy_name(enemy e) noexcept {
        constexpr std::string_view names[8] = {
            "insectoid", "green alien", "ship demon", "ufo disc",
            "green egg", "red egg", "blue orb", "gem orb",
        };
        return names[static_cast <std::size_t>(e) & 7u];
    }

    // Convert a decoded BOB sheet into a sprite_def: an image plus one visual per frame,
    // named "0".."N-1", each carrying its BOB per-frame offset as the pivot origin (so
    // variable-size animation frames stay aligned). Clips are layered on by define_*.
    [[nodiscard]] neutrino::sprite_def to_sprite_def(const tile_sheet_def& sheet);

    // Build every KE object definition from @p gr into the published ke_assets: the brick
    // tileset + background (define_blocks) and the paddle sprite def (define_paddle).
    // @pre set_ke_assets() has been called and the application is ready.
    void define_sprites(const game_resources& gr);
}
