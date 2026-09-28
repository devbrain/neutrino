//
// Created by igor on 17/07/2026.
//

#pragma once

#include <vector>

#include <neutrino/world_space.hh> // world_pos / world_velocity
#include <neutrino/physics/geometry/shapes.hh>
#include <neutrino/video/sprite/sprite_state.hh>
#include <ke/assets/sprites.hh>

// The KE domain model: pure game state. It owns no engine subsystems -- game_mechanics
// drives the physics from this data, and the renderer/actors read it to draw.
//
// Positions and velocities are the STRONG gameplay types (neutrino::world_pos /
// neutrino::world_velocity), not bare float pairs. That is what stops a position being handed
// where a rate belongs -- the mismatch behind the paddle freeze -- and it makes the integration
// steps read as physics: `pos += vel * dt`. The untyped neutrino::world_point survives only at
// the drawing edge (to_world_point, in the scene).

struct paddle_info {
    int x; // resolved left-edge position, written back from the physics each tick
    int y;
    int w;
    int h;
    int size;
    int shield{0}; // enemy hits consume one; a hit at zero destroys the paddle
    rs::ke_paddle_state state{rs::ke_paddle_state::simple};
    int target_x{160}; // desired centre (render x) the player aims at; mechanics moves toward it
    [[nodiscard]] neutrino::physics::aabb box() const {
        return {{x, y}, {x + w, y + h}};
    }
};

struct brick {
    neutrino::world_pos pos; // world pixels (cell -> grid origin + cell*tile)
    int frame; // KE_BRICK sprite frame (0-based local index)
    int hits;

    rs::bonus bonus;
    int bonus_mag;

    // lifecycle: alive (a solid), or flung (dead debris sliding off-screen).
    enum class motion { ALIVE, FLUNG } m = motion::ALIVE;

    neutrino::world_velocity vel{}; // set when flung

};

// A ball. `pos` is written back by the mechanics each frame for drawing; `vel` is the
// game-owned velocity re-applied on each bounce. `kind` + `size` select the KE_SPELL
// sprite (rs::ke_ball_frame). There can be several balls (extra-ball bonus).
struct ball_state {
    rs::ke_ball_kind kind{rs::ke_ball_kind::ordinary};
    int size{0}; // 0..5, selects the sprite within the kind's range

    neutrino::world_pos pos{};
    neutrino::world_velocity vel{};
    int half{2}; // collision half-extent of the (square) ball, world pixels
    bool active{false};
};

struct hit_effect {
    neutrino::world_pos pos; // impact point
    neutrino::sprite_state_id state;
};

struct capsule {
    rs::bonus bonus;
    int mag{};
    neutrino::world_pos pos;
    neutrino::sprite_state_id state;
    int w{};
    int h{};
    bool active{false};

    [[nodiscard]] neutrino::physics::aabb box() const {
        return {{pos.x, pos.y}, {pos.x + w, pos.y + h}};
    }
};

// Original anchor coordinates, not the sprite's top-left. Animation and AI are
// advanced together at 70 Hz so the collision box matches the displayed frame.
struct enemy_state {
    rs::enemy type{rs::enemy::insectoid};
    neutrino::world_pos pos{};
    neutrino::world_velocity vel{};
    int turn_ticks{1};
    int animation_ticks{0};
    int hits{2}; // reserved for weapon damage; a ball kills in one contact
    bool alive{true};

    [[nodiscard]] std::size_t frame() const {
        const auto& anim = alive ? rs::enemy_anim(type) : rs::enemy_death_anim;
        return anim.frames[(animation_ticks / anim.ticks) % anim.count];
    }
};

struct level_info {
    std::vector <brick> bricks;
    std::vector <ball_state> balls;
    std::vector <hit_effect> effects;
    std::vector <capsule> capsules;
    std::vector <enemy_state> enemies;
    unsigned animation_ticks{0}; // original 70 Hz clock for shared HUD/overlay animations
    int hatch_ticks{-1}; // -1 = closed; otherwise progress through KE_NMY hatch animation
    int paddle_death_ticks{-1}; // -1 = playing; nonnegative = death animation/game over

    void clear();
};

// The inner playfield extent (from the level background), in world pixels.
struct playfield_bounds {
    int left{};
    int right{};
    int top{};
    int bottom{};
};

class model {
    public:
        static model& instance();

        [[nodiscard]] int get_level() const;
        void load_level();
        void reset_paddle();
        void restart_game();

        void set_paddle_size(int size);
        void set_paddle_state(rs::ke_paddle_state state);
        // Aim the paddle: record where the player wants its centre (render pixels). The
        // mechanics moves the paddle toward it, and the playfield walls (static bodies) stop
        // it -- there is no clamp here.
        void set_paddle_target(int center_x);

        [[nodiscard]] const paddle_info& get_paddle() const;
        paddle_info& get_paddle();

        [[nodiscard]] const level_info& get_level_info() const;
        level_info& get_level_info();

        [[nodiscard]] playfield_bounds get_bounds() const;

        // Player progress, displayed by the scene. Lives/score persist across levels
        // and lost lives; restart_game resets them.
        [[nodiscard]] int get_lives() const;
        [[nodiscard]] long get_score() const;
        void add_life(int n);
        void increase_score_multiplier(int steps);
        // Base points, multiplied by 2^score_shift (ke.exe's scoring convention).
        void add_score(long n);

    private:
        model();

        void set_paddle_dims_from_frame(std::size_t frame);

    private:
        int m_level = 0;
        int m_lives = 3;
        long m_score = 0;
        int m_score_shift = 0;

        playfield_bounds m_bounds{};
        paddle_info m_paddle;
        level_info m_level_info;
};
