//
// Created by igor on 17/07/2026.
//

#include <ke/game/model.hh>
#include <ke/assets/registry.hh>
#include <ke/assets/backdrop.hh>
#include <algorithm>
#include <limits>

static model* s_instance = nullptr;

void level_info::clear() {
    balls.clear();
    enemies.clear();
    animation_ticks = 0;
    hatch_ticks = -1;
    paddle_death_ticks = -1;
    for (const auto& fx : effects) {
        neutrino::unregister_sprite_state(fx.state);
    }

    for (const auto& cp : capsules) {
        neutrino::unregister_sprite_state(cp.state);
    }

    capsules.clear();
    effects.clear();
}

model& model::instance() {
    if (!s_instance) {
        s_instance = new model;
    }
    return *s_instance;
}

int model::get_level() const {
    return m_level;
}

void model::load_level() {

    auto& assets = rs::require_ke_assets();

    // Playfield geometry (walls + paddle start) from the backdrop sprite sets. The backdrop
    // pixels are the scene's concern; the domain keeps only the bounds and the paddle start.
    const rs::playfield_geometry geometry =
        rs::compute_playfield_geometry(assets.board, assets.fill, assets.paddle);
    m_bounds = {geometry.left_margin, geometry.right_margin, geometry.top_margin, geometry.bottom_margin};

    reset_paddle();

    // Domain bricks from the level, on the same grid the sprites are drawn at. The physical
    // world (colliders, ball) is built from this by game_mechanics::load().
    const rs::ke_level& level = assets.levels[m_level];

    m_level_info.balls.clear();
    m_level_info.bricks.clear();

    for (int y = 0; y < rs::ke_level::rows; ++y) {
        for (int x = 0; x < rs::ke_level::cols; ++x) {
            const auto& c = level.at(x, y);
            if (c.tile_id == 0) {
                continue;
            }
            brick b{};
            b.m         = brick::motion::ALIVE;
            b.frame     = static_cast<int>(c.tile_id) - 1; // KE tile id is 1-based
            b.bonus     = c.bonus_type;
            b.bonus_mag = c.bonus_mag;
            b.hits      = (c.kind == rs::brick_kind::indestructible) ? -1 : c.hits;
            b.pos       = {static_cast<float>(rs::ke_grid_x + x * rs::ke_cell_w),
                           static_cast<float>(rs::ke_grid_y + y * rs::ke_cell_h)};
            m_level_info.bricks.push_back(b);
        }
    }
}

void model::reset_paddle() {
    const auto& assets = rs::require_ke_assets();
    const auto geometry = rs::compute_playfield_geometry(assets.board, assets.fill, assets.paddle);
    m_paddle.size = rs::ke_paddle_default_size;
    m_score_shift = 0;
    m_paddle.shield = 0;
    m_paddle.state = rs::ke_paddle_state::simple;
    m_paddle.x = geometry.paddle_start.x;
    m_paddle.y = geometry.paddle_start.y;
    set_paddle_dims_from_frame(rs::ke_paddle_frame(m_paddle.state, m_paddle.size));
    m_paddle.target_x = m_paddle.x + m_paddle.w / 2;
}

void model::restart_game() {
    m_level = 0;
    m_lives = 3;
    m_score = 0;
    load_level();
}

void model::set_paddle_size(int size) {
    m_paddle.size = size;
    auto frame = rs::ke_paddle_frame(m_paddle.state, m_paddle.size);
    set_paddle_dims_from_frame(frame);
}

void model::set_paddle_state(rs::ke_paddle_state state) {
    m_paddle.state = state;
    auto frame = rs::ke_paddle_frame(m_paddle.state, m_paddle.size);
    set_paddle_dims_from_frame(frame);
}


void model::set_paddle_target(int center_x) {
    m_paddle.target_x = center_x;
}

const paddle_info& model::get_paddle() const {
    return m_paddle;
}

paddle_info& model::get_paddle() {
    return m_paddle;
}

const level_info& model::get_level_info() const {
    return m_level_info;
}

level_info& model::get_level_info() {
    return m_level_info;
}

playfield_bounds model::get_bounds() const {
    return m_bounds;
}

int model::get_lives() const {
    return m_lives;
}

long model::get_score() const {
    return m_score;
}

void model::add_life(int n) {
    m_lives += n;
}

void model::add_score(long n) {
    // Saturate instead of overflowing C++ signed arithmetic after repeated x2 pickups.
    const long remaining = std::numeric_limits<long>::max() - m_score;
    if (n > 0) {
        m_score += n > (remaining >> m_score_shift) ? remaining : n << m_score_shift;
    }
}

void model::increase_score_multiplier(int steps) {
    m_score_shift = std::clamp(m_score_shift + steps, 0, std::numeric_limits<long>::digits - 1);
}


model::model() = default;

void model::set_paddle_dims_from_frame(std::size_t frame) {
    const auto& assets = rs::require_ke_assets();
    // The paddle's collider is sized from this frame, so a missing one used to fall back to a
    // 1x1 paddle: physically present, visually absent, and unplayable. A KE_RACK form the game
    // asks for must exist -- fail at the lookup instead.
    const neutrino::rect r = assets.paddle.require_frame_rect(frame);
    m_paddle.h = r.h;
    m_paddle.w = r.w;
}
