//
// Created by igor on 14/07/2026.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string_view>

#include <neutrino/video/sprite/sprite_def.hh>
#include <ke/format/archive.hh>
#include <ke/resources/resources.hh>

namespace rs {
    // KE_RACK tables at 0x48D9C/DF0/E44/E98. There are 13 logical sizes:
    // ordinary 8..20, armed 21..27 (each repeated twice except the last),
    // frozen overlay 28..34 (same repetition), glue overlay 35..47.
    // Keep the reimplementation's 1-based size API; the original uses 0..12.
    enum class ke_paddle_state {
        simple,
        armed,
        caged, // frozen overlay
        turbo, // glue overlay (legacy state name)
    };

    // The consecutive KE_RACK frames one paddle state occupies.
    struct ke_paddle_frame_range {
        std::size_t first; ///< first KE_RACK frame (inclusive)
        std::size_t count; ///< number of form frames in the state
    };

    [[nodiscard]] constexpr ke_paddle_frame_range ke_paddle_range(ke_paddle_state state) noexcept {
        switch (state) {
            case ke_paddle_state::simple: return {8, 13};
            case ke_paddle_state::armed: return {21, 7};
            case ke_paddle_state::caged: return {28, 7};
            case ke_paddle_state::turbo: return {35, 13};
        }
        return {8, 13};
    }

    // The default/start form (size) within a state.
    inline constexpr int ke_paddle_size_count = 13;
    inline constexpr int ke_paddle_default_size = 4; // original index 3, KE_RACK block 11

    // KE_RACK frame for a paddle (@p state, @p size). `size` is 1-based; a size past the
    // state's range clamps to its last frame.
    [[nodiscard]] constexpr std::size_t ke_paddle_frame(ke_paddle_state state, int size) noexcept {
        const ke_paddle_frame_range r = ke_paddle_range(state);
        const int clamped = size < 1 ? 1 : (size > ke_paddle_size_count ? ke_paddle_size_count : size);
        const auto i = static_cast <std::size_t>(clamped - 1);
        return r.first + ((state == ke_paddle_state::armed || state == ke_paddle_state::caged) ? i / 2 : i);
    }

    // KE_SPELL ball sprites, grouped by ball kind. Each kind has ke_ball_size_count
    // consecutive size frames. Frame ranges in the KE_SPELL sheet (inclusive):
    //   ordinary 0..5 | power 6..11 | ghost 12..17 (0x49008/34/60).
    enum class ke_ball_kind {
        ordinary,
        power,
        ghost,
    };

    inline constexpr int ke_ball_size_count = 6; // sizes 0..5 per kind

    // KE_SPELL frame for a ball (@p kind, @p size). `size` is 0-based (0..5), clamped.
    [[nodiscard]] constexpr std::size_t ke_ball_frame(ke_ball_kind kind, int size) noexcept {
        const int s = size < 0 ? 0 : (size >= ke_ball_size_count ? ke_ball_size_count - 1 : size);
        return static_cast <std::size_t>(kind) * ke_ball_size_count + static_cast <std::size_t>(s);
    }

    // A KE sprite animation extracted from ke.exe: the exact BOB block sequence (looping),
    // with a single per-frame duration in game ticks (ke_tick, 70 Hz). count == 1 is a
    // static sprite (its `ticks` is just a long hold). Capacity fits the 30-frame hatch.
    struct ke_anim {
        std::array <std::uint8_t, 32> frames{}; // BOB blocks, valid in [0, count)
        std::uint8_t count = 0;
        std::uint8_t ticks = 0; // per-frame duration in game ticks

        [[nodiscard]] constexpr bool is_static() const noexcept { return count == 1; }
        [[nodiscard]] constexpr const std::uint8_t* begin() const noexcept { return frames.data(); }
        [[nodiscard]] constexpr const std::uint8_t* end() const noexcept { return frames.data() + count; }
        [[nodiscard]] constexpr ke_tick frame_duration() const noexcept { return ke_tick{ticks}; }
    };

    // Runtime-ID order from ke.exe 0x49284; TAB uses ID+1 (docs/bonuses.md).
    // Frames are zero-based KE_SPELL blocks. The sequences were correct even when
    // the attribute decode and several effect names were not.
    inline constexpr std::array <ke_anim, 28> ke_spell_capsule_anim = {
        {
            {{62, 63, 64, 65, 66, 65, 64, 63}, 8, 5}, // 0  enlarge_paddle (ping-pong)
            {{67}, 1, 127}, // 1  damage_paddle
            {{53}, 1, 127}, // 2  score_multiplier
            {{50, 51, 52}, 3, 8}, // 3  reverse_controls
            {{76, 77, 78, 79, 80, 79, 78, 77}, 8, 6}, // 4  shrink_balls (ping-pong)
            {{70}, 1, 127}, // 5  glue_paddle
            {{37}, 1, 127}, // 6  extra_life
            {{48}, 1, 127}, // 7  extra_ball
            {{57, 58, 59, 60, 61, 60, 59, 58}, 8, 6}, // 8  enlarge_balls (ping-pong)
            {{34}, 1, 127}, // 9  darkness
            {{69}, 1, 127}, // 10 speed_up_all_balls
            {{68}, 1, 127}, // 11 slow_all_balls
            {{56}, 1, 127}, // 12 autopilot
            {{54, 55}, 2, 4}, // 13 flying_paddle
            {{41}, 1, 127}, // 14 freeze_paddle
            {{35}, 1, 127}, // 15 shield
            {{46}, 1, 127}, // 16 cannon
            {{38, 39}, 2, 10}, // 17 power_ball
            {{40}, 1, 127}, // 18 ghost_balls
            {{49}, 1, 127}, // 19 extra_paddle
            {{47}, 1, 127}, // 20 rapid_cannon
            {{36}, 1, 127}, // 21 random
            {{71, 72, 73, 74, 75, 74, 73, 72}, 8, 5}, // 22 shrink_paddle (ping-pong)
            {{42}, 1, 127}, // 23 single_gun
            {{44}, 1, 127}, // 24 double_gun
            {{43}, 1, 127}, // 25 rapid_single_gun
            {{45}, 1, 127}, // 26 rapid_double_gun
            {{81, 82, 83}, 3, 3}, // 27 clear_enemies
        }
    };

    // Enemy animation per spawn type (0..7), from ke.exe's 0x496B8 descriptor table (matches
    // tab.md §7). Frames are 0-based KE_NMY blocks; ship_demon (type 2) is the long one.
    inline constexpr std::array <ke_anim, 8> ke_nmy_enemy_anim = {
        {
            {{0, 1}, 2, 6}, // 0 insectoid
            {{2, 3, 4, 3}, 4, 15}, // 1 green_alien
            {{5, 5, 5, 5, 6, 7, 64, 65, 64, 7, 6}, 11, 3}, // 2 ship_demon
            {{8, 9, 10}, 3, 8}, // 3 ufo_disc
            {{11, 12, 13, 12}, 4, 15}, // 4 green_egg
            {{14, 15, 16, 15}, 4, 10}, // 5 red_egg
            {{17, 18, 19, 20, 21, 22}, 6, 10}, // 6 blue_orb
            {{23, 24, 25, 26, 27, 28, 29, 30}, 8, 10}, // 7 gem_orb
        }
    };

    inline constexpr ke_anim hit_wall_anim = {
        {84, 85, 86, 87, 88, 89}, 6, 6
    };

    // KE_NMY descriptors 0x49384 (hatch) and 0x49634 (enemy explosion).
    inline constexpr ke_anim enemy_hatch_anim = {
        {52,53,54,55,56,57,58,59,60,61,62,63,63,63,63,63,63,63,63,62,61,60,59,58,57,56,55,54,53,52}, 30, 5
    };
    inline constexpr ke_anim enemy_death_anim = {
        {31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51}, 21, 2
    };
    inline constexpr ke_anim paddle_death_anim = {{5,4,3,2,1,0}, 6, 4}; // KE_RACK, 0x48EEC

    inline constexpr ke_anim hit_brick_anim = {
        {18, 19, 20, 21, 22, 23}, 6, 6
    };

    enum class hit_kind { wall, brick };

    inline constexpr float ke_tick_seconds = 1.0f / 70.0f; // ke_tick is 70 Hz

    [[nodiscard]] constexpr const ke_anim& hit_anim(hit_kind k) noexcept {
        return k == hit_kind::wall ? hit_wall_anim : hit_brick_anim;
    }

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
        constexpr std::string_view names[bonus_count] = {
            "enlarge", "damage", "score x2", "reverse", "small balls", "glue",
            "extra life", "extra ball", "large balls", "darkness", "speed up", "slow down",
            "autopilot", "fly", "freeze", "shield", "cannon", "power ball",
            "ghost balls", "extra paddle", "rapid cannon", "random", "shrink", "single gun",
            "double gun", "rapid single", "rapid double", "dynamite",
        };
        const auto i = static_cast <std::size_t>(b);
        return i < bonus_count ? names[i] : "none";
    }

    // Stream a bonus by its short debug name (see bonus_name), e.g. LOG_INFO("caught ", b).
    // Found by ADL; defined in sprites.cc so this header need not pull in <ostream>.
    std::ostream& operator<<(std::ostream& os, bonus b);

    [[nodiscard]] constexpr std::string_view enemy_name(enemy e) noexcept {
        constexpr std::string_view names[8] = {
            "insectoid", "green alien", "ship demon", "ufo disc",
            "green egg", "red egg", "blue orb", "gem orb",
        };
        return names[static_cast <std::size_t>(e) & 7u];
    }

    // Convert a decoded BOB sheet into a sprite_def: an image plus one visual per frame,
    // named "0".."N-1". Each visual's pivot is its BOB per-frame offset (so variable-size
    // animation frames stay aligned) unless @p top_left_origin, which pins the pivot to
    // (0,0) for tiles placed by their top-left corner (the backdrop). Clips are layered on
    // by define_*.
    [[nodiscard]] neutrino::sprite_def to_sprite_def(const tile_sheet_def& sheet,
                                                     bool top_left_origin = false);

    // Build the KE actor sprite sets (paddle / bricks / balls) from @p gr into the published
    // ke_assets. The backdrop sheets stay in @p gr for the backdrop module to read.
    // @pre set_ke_assets() has been called and the application is ready.
    void define_sprites(const game_resources& gr);

    // The counterpart to define_sprites: unregister the 30 animations it registered (the two hit
    // effects + the 28 capsule loops).
    //
    // This is not optional bookkeeping. A registered animation counts as a USER of the sprite
    // sheet its frames come from, so releasing the sprite sets while the animations are still
    // registered trips "Cannot unregister sprite sheet while it is still used" -- and that abort
    // comes out of a destructor, which is noexcept, so it terminates rather than throwing.
    //
    // Call from application::teardown(), before the ke_assets storage goes away and while the
    // sprite services are still published. (The longer-term fix is to move these animations into
    // sprite_def::clips, which the engine already supports, so the sets own them and there is
    // nothing to hand-unregister -- see the roadmap's "not an engine gap" section.)
    // @pre set_ke_assets() has been called.
    void release_sprites();
}
