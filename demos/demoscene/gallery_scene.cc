#include "gallery_scene.hh"

#include <cmath>
#include <format>
#include <onyx_font/bios_font.hh>
#include <neutrino/video/draw.hh>

#include "effects/wormhole_effect.hh"
#include "effects/voxel_effect.hh"
#include "effects/rotate3d_effect.hh"
#include "effects/bobs_effect.hh"
#include "effects/plasma_effect.hh"
#include "effects/stars_effect.hh"
#include "effects/lens_effect.hh"
#include "effects/fire_effect.hh"
#include "effects/copper_effect.hh"
#include "effects/dypp_effect.hh"
#include "effects/julia_effect.hh"
#include "effects/sinmap_effect.hh"
#include "effects/checkerboard3d_effect.hh"
#include "effects/fadeplot_effect.hh"
#include "effects/trans_glass3d_effect.hh"
#include "effects/dots_wave_effect.hh"
#include "effects/bounce_effect.hh"
#include "effects/field_lines_effect.hh"
#include "effects/spiral_twister_effect.hh"
#include "effects/radar_sweep_effect.hh"
#include "effects/vectorballs_effect.hh"
#include "effects/water_effect.hh"
#include "effects/metaballs_effect.hh"
#include "effects/bump_effect.hh"
#include "effects/rotozoom_effect.hh"
#include "effects/rototunnel_effect.hh"
#include "effects/environcube_effect.hh"
#include "effects/attractors_effect.hh"
#include "effects/gravitational_well_effect.hh"
#include "effects/wavy_ribbon_effect.hh"
#include "effects/multi_sprites_effect.hh"
#include "effects/blinking_stars_effect.hh"
#include "effects/dotmorph_effect.hh"
#include "effects/moire_effect.hh"
#include "effects/linedance_effect.hh"
#include "effects/xor_patterns_effect.hh"
#include "effects/water_reflection_effect.hh"
#include "effects/bouncing_sphere3d_effect.hh"
#include "effects/pixelate_effect.hh"
#include "effects/stretch_scroll_effect.hh"
#include "effects/glenz_vector_effect.hh"
#include "effects/texture_cube_effect.hh"
#include "effects/fractal_landscape_effect.hh"
#include "effects/waving_flag_effect.hh"
#include "effects/dot_tunnel_effect.hh"
#include "effects/mesh_distort_effect.hh"
#include "effects/uv_deformations_effect.hh"

namespace demoscene {

gallery_scene::gallery_scene() {
    m_effects.push_back(std::make_unique<wormhole_effect>());
    m_effects.push_back(std::make_unique<voxel_effect>());
    m_effects.push_back(std::make_unique<rotate3d_effect>());
    m_effects.push_back(std::make_unique<bobs_effect>());
    m_effects.push_back(std::make_unique<plasma_effect>());
    m_effects.push_back(std::make_unique<stars_effect>());
    m_effects.push_back(std::make_unique<lens_effect>());
    m_effects.push_back(std::make_unique<fire_effect>());
    m_effects.push_back(std::make_unique<copper_effect>());
    m_effects.push_back(std::make_unique<dypp_effect>());
    m_effects.push_back(std::make_unique<julia_effect>());
    m_effects.push_back(std::make_unique<sinmap_effect>());
    m_effects.push_back(std::make_unique<checkerboard3d_effect>());
    m_effects.push_back(std::make_unique<fadeplot_effect>());
    m_effects.push_back(std::make_unique<trans_glass3d_effect>());
    m_effects.push_back(std::make_unique<dots_wave_effect>());
    m_effects.push_back(std::make_unique<bounce_effect>());
    m_effects.push_back(std::make_unique<field_lines_effect>());
    m_effects.push_back(std::make_unique<spiral_twister_effect>());
    m_effects.push_back(std::make_unique<radar_sweep_effect>());
    m_effects.push_back(std::make_unique<vectorballs_effect>());
    m_effects.push_back(std::make_unique<water_effect>());
    m_effects.push_back(std::make_unique<metaballs_effect>());
    m_effects.push_back(std::make_unique<bump_effect>());
    m_effects.push_back(std::make_unique<rotozoom_effect>());
    m_effects.push_back(std::make_unique<rototunnel_effect>());
    m_effects.push_back(std::make_unique<environcube_effect>());
    m_effects.push_back(std::make_unique<attractors_effect>());
    m_effects.push_back(std::make_unique<gravitational_well_effect>());
    m_effects.push_back(std::make_unique<wavy_ribbon_effect>());
    m_effects.push_back(std::make_unique<multi_sprites_effect>());
    m_effects.push_back(std::make_unique<blinking_stars_effect>());
    m_effects.push_back(std::make_unique<dotmorph_effect>());
    m_effects.push_back(std::make_unique<moire_effect>());
    m_effects.push_back(std::make_unique<linedance_effect>());
    m_effects.push_back(std::make_unique<xor_patterns_effect>());
    m_effects.push_back(std::make_unique<water_reflection_effect>());
    m_effects.push_back(std::make_unique<bouncing_sphere3d_effect>());
    m_effects.push_back(std::make_unique<pixelate_effect>());
    m_effects.push_back(std::make_unique<stretch_scroll_effect>());
    m_effects.push_back(std::make_unique<glenz_vector_effect>());
    m_effects.push_back(std::make_unique<texture_cube_effect>());
    m_effects.push_back(std::make_unique<fractal_landscape_effect>());
    m_effects.push_back(std::make_unique<waving_flag_effect>());
    m_effects.push_back(std::make_unique<dot_tunnel_effect>());
    m_effects.push_back(std::make_unique<mesh_distort_effect>());
    m_effects.push_back(std::make_unique<uv_deformations_effect>());
}

void gallery_scene::on_enter() {
    if (!m_effects.empty() && m_active_index < m_effects.size()) {
        m_effects[m_active_index]->on_enter();
    }
}

void gallery_scene::on_exit() {
    if (!m_effects.empty() && m_active_index < m_effects.size()) {
        m_effects[m_active_index]->on_exit();
    }
}

void gallery_scene::switch_effect(size_t index) {
    if (index >= m_effects.size() || index == m_active_index) return;

    m_effects[m_active_index]->on_exit();
    m_active_index = index;
    m_effects[m_active_index]->on_enter();
}

void gallery_scene::next_effect() {
    switch_effect((m_active_index + 1) % m_effects.size());
}

void gallery_scene::prev_effect() {
    switch_effect((m_active_index + m_effects.size() - 1) % m_effects.size());
}

void gallery_scene::fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // FPS counter calculation
    m_frame_count++;
    m_fps_timer += dt_sec;
    if (m_fps_timer >= 0.5f) {
        m_fps = static_cast<float>(m_frame_count) / m_fps_timer;
        m_frame_count = 0;
        m_fps_timer = 0.0f;
    }

    // Gallery navigation keys
    if (in.pressed(sdlpp::scancode::tab) || in.pressed(sdlpp::scancode::pagedown)) next_effect();
    if (in.pressed(sdlpp::scancode::backspace) || in.pressed(sdlpp::scancode::pageup)) prev_effect();
    if (in.pressed(sdlpp::scancode::f1)) m_show_hud = !m_show_hud;

    // Direct jump keys
    if (in.pressed(sdlpp::scancode::num_1)) switch_effect(0);
    if (in.pressed(sdlpp::scancode::num_2)) switch_effect(1);
    if (in.pressed(sdlpp::scancode::num_3)) switch_effect(2);
    if (in.pressed(sdlpp::scancode::num_4)) switch_effect(3);
    if (in.pressed(sdlpp::scancode::num_5)) switch_effect(4);
    if (in.pressed(sdlpp::scancode::num_6)) switch_effect(5);
    if (in.pressed(sdlpp::scancode::num_7)) switch_effect(6);
    if (in.pressed(sdlpp::scancode::num_8)) switch_effect(7);
    if (in.pressed(sdlpp::scancode::num_9)) switch_effect(8);
    if (in.pressed(sdlpp::scancode::num_0)) switch_effect(9);
    if (in.pressed(sdlpp::scancode::minus)) switch_effect(10);
    if (in.pressed(sdlpp::scancode::equals)) switch_effect(11);
    if (in.pressed(sdlpp::scancode::leftbracket)) switch_effect(12);
    if (in.pressed(sdlpp::scancode::rightbracket)) switch_effect(13);
    if (in.pressed(sdlpp::scancode::backslash)) switch_effect(14);
    if (in.pressed(sdlpp::scancode::semicolon)) switch_effect(15);
    if (in.pressed(sdlpp::scancode::apostrophe)) switch_effect(16);
    if (in.pressed(sdlpp::scancode::comma)) switch_effect(17);
    if (in.pressed(sdlpp::scancode::period)) switch_effect(18);
    if (in.pressed(sdlpp::scancode::slash)) switch_effect(19);

    // Dispatch update to active effect
    if (!m_effects.empty() && m_active_index < m_effects.size()) {
        m_effects[m_active_index]->update(dt, in);
    }
}

void gallery_scene::handle_action(const sdlpp::event& ev) {
    (void)ev;
}

void gallery_scene::draw_bios_text(int px, int py, std::string_view text, const sdlpp::color& color) {
    const auto& font = onyx_font::bios_font_8x8();
    int pen_x = px;

    for (char c : text) {
        auto glyph = font.get_glyph(static_cast<uint8_t>(c));
        for (uint16_t y = 0; y < glyph.height(); ++y) {
            for (uint16_t x = 0; x < glyph.width(); ++x) {
                if (glyph.pixel(x, y)) {
                    neutrino::draw_point(neutrino::point{pen_x + x, py + y}, color);
                }
            }
        }
        pen_x += 8;
    }
}

void gallery_scene::render() {
    constexpr int target_width = 640;
    constexpr int target_height = 400; // Mode 13h doubled (320x200 -> 640x400)
    const neutrino::rect viewport{0, 0, target_width, target_height};

    // 1. Render active demoscene effect
    if (!m_effects.empty() && m_active_index < m_effects.size()) {
        m_effects[m_active_index]->render(viewport);
    }

    // 2. Render educational HUD overlay
    if (m_show_hud) {
        render_hud(viewport);
    }
}

void gallery_scene::render_hud(const neutrino::rect& viewport) {
    const auto& current = m_effects[m_active_index];

    // Header Panel: Effect Title & Provenance
    const neutrino::rect header_rect{10, 8, viewport.w - 20, 48};
    neutrino::draw_rect_fill(header_rect, sdlpp::color{10, 15, 25, 210});
    neutrino::draw_rect(header_rect, sdlpp::color{60, 90, 140, 255});

    const std::string title_line = std::format("[{}/{}] {}  ({})",
        m_active_index + 1, m_effects.size(), current->name(), current->original_file());
    draw_bios_text(header_rect.x + 8, header_rect.y + 8, title_line, sdlpp::colors::yellow);

    const std::string author_line = std::format("Author: {} | Formula: {}",
        current->author(), current->math_formula());
    draw_bios_text(header_rect.x + 8, header_rect.y + 26, author_line, sdlpp::color{140, 200, 255, 255});

    // Footer Panel: Controls & Telemetry
    const neutrino::rect footer_rect{10, viewport.h - 44, viewport.w - 20, 36};
    neutrino::draw_rect_fill(footer_rect, sdlpp::color{10, 15, 25, 210});
    neutrino::draw_rect(footer_rect, sdlpp::color{60, 90, 140, 255});

    const std::string controls_line = std::format("Controls: {}", current->controls_hint());
    draw_bios_text(footer_rect.x + 8, footer_rect.y + 6, controls_line, sdlpp::color{230, 230, 230, 255});

    const std::string nav_line = std::format("Tab/PgDn: Next | Bksp/PgUp: Prev (1..{}) | F1: Toggle HUD | FPS: {:.1f}",
        m_effects.size(), m_fps);
    draw_bios_text(footer_rect.x + 8, footer_rect.y + 20, nav_line, sdlpp::color{120, 240, 160, 255});
}

} // namespace demoscene
