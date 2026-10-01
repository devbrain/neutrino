#pragma once

#include <memory>
#include <vector>

#include <neutrino/scene/base_scene.hh>
#include <sdlpp/video/color.hh>
#include "effects/effect_base.hh"

namespace demoscene {

/**
 * @brief Master coordinator scene for the Neutrino Demoscene Gallery.
 *
 * Responsibilities:
 * - Hosts and switches between all demoscene effects (Wormhole, Voxel, Rotate3D, Bobs, Plasma, Stars, Lens).
 * - Dispatches input and fixed updates to the active effect.
 * - Renders the educational retro HUD (formula, 1994 DOS notes, interactive controls) using embedded BIOS font.
 */
class gallery_scene final : public neutrino::base_scene {
public:
    gallery_scene();
    ~gallery_scene() override = default;

    void on_enter() override;
    void on_exit() override;

    void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render() override;
    void handle_action(const sdlpp::event& ev) override;

    [[nodiscard]] bool is_opaque() const override {
        return true;
    }

    void switch_effect(size_t index);
    void next_effect();
    void prev_effect();

private:
    void render_hud(const neutrino::rect& viewport);
    void draw_bios_text(int px, int py, std::string_view text, const sdlpp::color& color);

    std::vector<std::unique_ptr<effect_base>> m_effects;
    size_t m_active_index = 0;
    bool m_show_hud = true;

    // Live telemetry
    float m_fps = 60.0f;
    float m_fps_timer = 0.0f;
    int m_frame_count = 0;
};

} // namespace demoscene
