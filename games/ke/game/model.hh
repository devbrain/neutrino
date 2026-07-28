//
// Created by igor on 17/07/2026.
//

#pragma once

#include <vector>

#include <neutrino/world/world_common.hh> // world_point
#include <ke/assets/sprites.hh>

// The KE domain model: pure game state. It owns no engine subsystems -- game_mechanics
// drives the physics from this data, and the renderer/actors read it to draw.

struct paddle_info {
    int x; // resolved left-edge position, written back from the physics each tick
    int y;
    int w;
    int h;
    int size;
    rs::ke_paddle_state state{rs::ke_paddle_state::simple};
    int target_x{160}; // desired centre (render x) the player aims at; mechanics moves toward it
};

struct brick {
    neutrino::world_point pos; // world pixels (cell -> grid origin + cell*tile)
    int frame; // KE_BRICK sprite frame (0-based local index)
    int hits;

    rs::bonus bonus;
    int bonus_mag;

    // lifecycle: alive (a solid), or flung (dead debris sliding off-screen).
    enum class motion { ALIVE, FLUNG } m = motion::ALIVE;

    neutrino::world_point vel{}; // set when flung
};

// A ball. `pos` is written back by the mechanics each frame for drawing; `vel` is the
// game-owned velocity re-applied on each bounce. `kind` + `size` select the KE_SPELL
// sprite (rs::ke_ball_frame). There can be several balls (e.g. a split bonus).
struct ball_state {
    rs::ke_ball_kind kind{rs::ke_ball_kind::ordinary};
    int size{3};    // 0..5, selects the sprite within the kind's range

    neutrino::world_point pos{};
    neutrino::world_point vel{};
    int half{2};    // collision half-extent of the (square) ball, world pixels
    bool active{false};
};

struct hit_effect {
      neutrino::world_point pos;      // impact point
      rs::hit_kind          kind;
      float                 elapsed{0.0f};   // seconds since spawn
  };


struct level_info {
    std::vector<brick> bricks;
    std::vector<ball_state> balls;
    std::vector<hit_effect> effects;
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

    private:
        model();

        void set_paddle_dims_from_frame(std::size_t frame);

    private:
        int m_level = 0;

        playfield_bounds m_bounds{};
        paddle_info m_paddle;
        level_info  m_level_info;
};
