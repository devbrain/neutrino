//
// Created by igor on 17/07/2026.
//

#include <ke/game/model.hh>
#include <ke/assets/registry.hh>

#include <ke/assets/world.hh>

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

    // The background is per-level: its KE_FILL block is derived from the level index.
    auto [background, geometry]
        = rs::build_background(*assets.m_resources, rs::ke_fill_block_for_level(m_level));
    m_visual_world = rs::build_world(assets.levels[m_level], background);

    m_paddle.size = 7;
    m_paddle.state = rs::ke_paddle_state::simple;
    m_paddle.x = geometry.paddle_start.x;
    m_paddle.y = geometry.paddle_start.y;
}

const neutrino::world& model::get_visual_world() const {
    return m_visual_world;
}

const neutrino::physics::world& model::get_physical_world() const {
    return m_physical_world;
}

neutrino::world& model::get_visual_world() {
    return m_visual_world;
}

neutrino::physics::world& model::get_physical_world() {
    return m_physical_world;
}

const paddle_info& model::get_paddle() const {
    return m_paddle;
}

paddle_info& model::get_paddle() {
    return m_paddle;
}

model::model() = default;
