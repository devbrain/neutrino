//
// Debug scene: draws every bonus capsule (KE_SPELL) and enemy (KE_NMY) as an animated
// sprite with its name below, using an in-engine BIOS 8x8 font sheet.
//

#pragma once

#include <vector>

#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/sprite/render_bundle.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>
#include <neutrino/video/sprite/sprite_state.hh>

#include "resources/ke_loader.hh"

class sprite_gallery_scene : public neutrino::base_scene {
    public:
        explicit sprite_gallery_scene(const rs::game_resources& res) : m_res(res) {}

        void on_enter() override;
        void update_physics(neutrino::frame_duration delta_t) override;
        void render(neutrino::frame_duration time_since_last_frame) override;
        void handle_action(const sdlpp::event& ev) override;
        [[nodiscard]] bool is_opaque() const override;

    private:
        const rs::game_resources& m_res;
        // Owns every registered resource this scene builds (atlases, sheets, animations,
        // states) and tears them down in dependency order when the scene is destroyed.
        neutrino::render_bundle m_bundle;
        neutrino::sprite_sheet_id m_font{};
        std::vector <neutrino::sprite_state_id> m_bonus; // one per bonus (0..27)
        std::vector <neutrino::sprite_state_id> m_enemy; // one per enemy (0..7)
        bool m_ready = false;
};
