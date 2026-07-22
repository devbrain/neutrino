//
// Created by igor on 17/07/2026.
//

#include <ke/game/model.hh>
#include <ke/assets/registry.hh>
#include <ke/assets/backdrop.hh>

static model* s_instance = nullptr;

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

    m_paddle.size = 7;
    m_paddle.state = rs::ke_paddle_state::simple;
    m_paddle.x = geometry.paddle_start.x;
    m_paddle.y = geometry.paddle_start.y;

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

model::model() = default;
