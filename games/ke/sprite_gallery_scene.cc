//
// See sprite_gallery_scene.hh.
//

#include "sprite_gallery_scene.hh"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <failsafe/enforce.hh>

#include <SDL3/SDL.h>
#include <onyx_font/bios_font.hh>

#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/sprites.hh>
#include <neutrino/video/sprite/cpu_texture_atlas.hh>
#include <neutrino/video/sprite/texture_atlas.hh>

#include "resources/ke_sprites_def.hh"

namespace {
    using neutrino::point;

    // Layout in the (640x480) render space.
    constexpr int cell_w = 122;
    constexpr int cell_h = 52;
    constexpr int spr_x = 6;
    constexpr int spr_y = 4;
    constexpr int name_dy = 42; // name baseline offset within a cell
    constexpr int margin = 6;

    // Register a decoded BOB sheet (surface + frame rects) as a sprite_sheet with one visual
    // per frame, recording the atlas + sheet in @p bundle for teardown.
    neutrino::sprite_sheet_id register_sheet(neutrino::render_bundle& bundle, const rs::tile_sheet_def& tsd) {
        auto dup = tsd.image.duplicate();
        ENFORCE(dup.has_value())("failed to duplicate sheet surface");
        std::vector <neutrino::cpu_texture_atlas_frame> frames;
        frames.reserve(tsd.source_rects.size());
        for (const auto& r : tsd.source_rects) {
            frames.emplace_back(r);
        }
        neutrino::cpu_texture_atlas atlas(std::move(*dup), std::move(frames));

        const neutrino::gpu_texture_atlas_id gpu = neutrino::register_atlas(atlas);
        bundle.atlases.push_back(gpu);
        neutrino::sprite_sheet sheet(gpu, atlas);
        const neutrino::sprite_sheet_id id = neutrino::register_sprite_sheet(std::move(sheet));
        bundle.sheets.push_back(id);
        return id;
    }

    // Build a looping runtime state playing @p anim's block sequence from @p sheet.
    neutrino::sprite_state_id build_state(neutrino::render_bundle& bundle,
                                          neutrino::sprite_sheet_id sheet, const rs::ke_anim& anim) {
        const auto dur = std::chrono::duration_cast <neutrino::sprite_animation_duration>(anim.frame_duration());
        neutrino::sprite_animation animation(true);
        for (const std::uint8_t block : anim) {
            animation.add_frame(neutrino::make_sprite_animation_frame(
                neutrino::make_sprite_appearance(neutrino::visual_ref(sheet, block)), dur));
        }
        const neutrino::sprite_animation_id aid = neutrino::register_sprite_animation(std::move(animation));
        bundle.animations.push_back(aid);
        const neutrino::sprite_state_id state = neutrino::create_sprite_state(aid);
        bundle.states.push_back(state);
        return state;
    }

    // A sprite sheet of the embedded BIOS 8x8 font: one visual per printable ASCII glyph
    // (code 32..126), white on transparent.
    neutrino::sprite_sheet_id build_font(neutrino::render_bundle& bundle) {
        const auto& font = onyx_font::bios_font_8x8();
        constexpr int first = 32;
        constexpr int last = 126;
        constexpr int count = last - first + 1;
        constexpr int gw = 8;
        constexpr int gh = 8;

        auto se = sdlpp::surface::create_rgb(count * gw, gh, sdlpp::pixel_format_enum::RGBA8888);
        ENFORCE(se.has_value())("failed to create font surface");
        sdlpp::surface surf = std::move(*se);
        (void) surf.fill(sdlpp::color{0, 0, 0, 0}); // transparent

        for (int i = 0; i < count; ++i) {
            const auto g = font.get_glyph(static_cast <std::uint8_t>(first + i));
            for (std::uint16_t gy = 0; gy < g.height(); ++gy) {
                for (std::uint16_t gx = 0; gx < g.width(); ++gx) {
                    if (g.pixel(gx, gy)) {
                        SDL_WriteSurfacePixel(surf.get(), i * gw + gx, gy, 225, 225, 235, 255);
                    }
                }
            }
        }

        std::vector <neutrino::cpu_texture_atlas_frame> frames;
        frames.reserve(count);
        for (int i = 0; i < count; ++i) {
            frames.emplace_back(neutrino::rect{i * gw, 0, gw, gh});
        }
        neutrino::cpu_texture_atlas atlas(std::move(surf), std::move(frames));

        const neutrino::gpu_texture_atlas_id gpu = neutrino::register_atlas(atlas);
        bundle.atlases.push_back(gpu);
        neutrino::sprite_sheet sheet(gpu, atlas);
        const neutrino::sprite_sheet_id id = neutrino::register_sprite_sheet(std::move(sheet));
        bundle.sheets.push_back(id);
        return id;
    }

    void draw_text(neutrino::sprite_sheet_id font, point pos, std::string_view text) {
        int x = pos.x;
        for (const char c : text) {
            if (c >= 32 && c <= 126) {
                (void) neutrino::draw_sprite(point{x, pos.y},
                    neutrino::visual_ref(font, static_cast <std::size_t>(c - 32)));
            }
            x += 8;
        }
    }
}

void sprite_gallery_scene::on_enter() {
    m_font = build_font(m_bundle);

    if (const auto it = m_res.tile_sheets.find("ke_spell"); it != m_res.tile_sheets.end()) {
        const neutrino::sprite_sheet_id spell = register_sheet(m_bundle, it->second);
        for (int i = 0; i < 28; ++i) {
            m_bonus.push_back(build_state(m_bundle, spell, rs::bonus_capsule(static_cast <rs::bonus>(i))));
        }
    }
    if (const auto it = m_res.tile_sheets.find("ke_nmy"); it != m_res.tile_sheets.end()) {
        const neutrino::sprite_sheet_id nmy = register_sheet(m_bundle, it->second);
        for (int i = 0; i < 8; ++i) {
            m_enemy.push_back(build_state(m_bundle, nmy, rs::enemy_anim(static_cast <rs::enemy>(i))));
        }
    }
    m_ready = true;
}

void sprite_gallery_scene::update_physics(neutrino::frame_duration) {
    // States advance from the application update loop; nothing to do here.
}

void sprite_gallery_scene::render(neutrino::frame_duration) {
    if (!m_ready) {
        return;
    }
    const auto v = neutrino::render_size();
    const int cols = std::max(1, (v.width - margin) / cell_w);

    draw_text(m_font, {margin, 2}, "BONUSES");
    const int b_top = 12;
    for (std::size_t i = 0; i < m_bonus.size(); ++i) {
        const int cx = margin + static_cast <int>(i) % cols * cell_w;
        const int cy = b_top + static_cast <int>(i) / cols * cell_h;
        (void) neutrino::draw_sprite(point{cx + spr_x, cy + spr_y}, m_bonus[i]);
        draw_text(m_font, point{cx, cy + name_dy}, rs::bonus_name(static_cast <rs::bonus>(i)));
    }

    const int b_rows = (static_cast <int>(m_bonus.size()) + cols - 1) / cols;
    const int e_top = b_top + b_rows * cell_h + 12;
    draw_text(m_font, {margin, e_top - 10}, "ENEMIES");
    for (std::size_t i = 0; i < m_enemy.size(); ++i) {
        const int cx = margin + static_cast <int>(i) % cols * cell_w;
        const int cy = e_top + static_cast <int>(i) / cols * cell_h;
        (void) neutrino::draw_sprite(point{cx + spr_x, cy + spr_y}, m_enemy[i]);
        draw_text(m_font, point{cx, cy + name_dy}, rs::enemy_name(static_cast <rs::enemy>(i)));
    }
}

void sprite_gallery_scene::handle_action(const sdlpp::event&) {
}

bool sprite_gallery_scene::is_opaque() const {
    return true;
}
